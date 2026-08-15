/* ============================================================
 *                        TEST  006
 * ============================================================
 *
 *  AUDIO NODE  -  wired Ethernet bring-up only
 *
 *  Board : ESP32-S3-WROOM-1  N16R8
 *  Net   : W5500 over SPI
 *
 *  WHAT THIS BUILD DOES
 *    - prints the test number and board facts at boot
 *    - resets and starts the W5500
 *    - waits for a DHCP address and prints it
 *    - proves DNS works by resolving a hostname
 *    - proves TCP works by opening a socket
 *    - prints a status line every 5 seconds, forever
 *
 *  WHAT THIS BUILD DOES NOT DO
 *    - no audio, no radio, no I2S, no DAC
 *    - nothing is written to the PCM5102A
 *    That is deliberate. One thing at a time.
 *
 *  WIRING EXPECTED BY THIS CODE
 *    W5500 SCLK -> GPIO 12
 *    W5500 MOSI -> GPIO 11
 *    W5500 MISO -> GPIO 10
 *    W5500 SCS  -> GPIO  9
 *    W5500 INT  -> GPIO  8
 *    W5500 RST  -> GPIO  7
 *    W5500 5V and GND from the board supply, common ground
 *
 *  NOT TESTED ON HARDWARE. Build to confirm.
 * ============================================================ */

#define TEST_NUMBER 6

#include <Arduino.h>
#include <SPI.h>
#include <ETH.h>

/* ---------- pin map ---------- */

#define PIN_ETH_SCK   12
#define PIN_ETH_MOSI  11
#define PIN_ETH_MISO  10
#define PIN_ETH_CS     9
#define PIN_ETH_INT    8
#define PIN_ETH_RST    7

/* ---------- settings ---------- */

#define ETH_PHY_ADDRESS   1
#define ETH_SPI_MHZ       12
#define STATUS_EVERY_MS   5000
#define DNS_TEST_HOST     "example.com"
#define TCP_TEST_PORT     80

/* ---------- state ---------- */

static bool  g_linkUp      = false;
static bool  g_haveAddress = false;
static bool  g_stackTested = false;
static unsigned long g_lastStatus = 0;
static unsigned long g_bootMillis = 0;

/* ---------- helpers ---------- */

static void banner()
{
  Serial.println();
  Serial.println(F("============================================================"));
  Serial.printf ("  AUDIO NODE   -   TEST %03d   -   ethernet bring-up\n", TEST_NUMBER);
  Serial.println(F("============================================================"));
}

static void printResetReason()
{
  esp_reset_reason_t r = esp_reset_reason();
  const char *name = "unknown";

  switch (r) {
    case ESP_RST_POWERON:  name = "power on";              break;
    case ESP_RST_EXT:      name = "external reset pin";    break;
    case ESP_RST_SW:       name = "software restart";      break;
    case ESP_RST_PANIC:    name = "PANIC or exception";    break;
    case ESP_RST_INT_WDT:  name = "interrupt watchdog";    break;
    case ESP_RST_TASK_WDT: name = "task watchdog";         break;
    case ESP_RST_WDT:      name = "other watchdog";        break;
    case ESP_RST_BROWNOUT: name = "BROWNOUT - power dip";  break;
    case ESP_RST_SDIO:     name = "sdio";                  break;
    default: break;
  }

  Serial.printf("reset ...... %s\n", name);

  if (r == ESP_RST_BROWNOUT) {
    Serial.println(F("             ^^ the supply sagged. check the 5V feed."));
  }
  if (r == ESP_RST_PANIC || r == ESP_RST_TASK_WDT || r == ESP_RST_INT_WDT) {
    Serial.println(F("             ^^ the previous run crashed."));
  }
}

static void printBoardFacts()
{
  Serial.printf("chip ....... %s  rev %d  %d core(s)  %d MHz\n",
                ESP.getChipModel(),
                ESP.getChipRevision(),
                ESP.getChipCores(),
                getCpuFrequencyMhz());

  Serial.printf("flash ...... %u bytes\n", (unsigned) ESP.getFlashChipSize());

  size_t psram = ESP.getPsramSize();
  if (psram == 0) {
    Serial.println(F("psram ...... NOT FOUND  <-- check memory_type in platformio.ini"));
  } else {
    Serial.printf("psram ...... %u bytes free %u\n",
                  (unsigned) psram, (unsigned) ESP.getFreePsram());
  }

  Serial.printf("heap ....... %u bytes free\n", (unsigned) ESP.getFreeHeap());
}

static void printPinMap()
{
  Serial.println(F("w5500 pins . sck 12  mosi 11  miso 10  cs 9  int 8  rst 7"));
}

/* ---------- network events ---------- */

static void onNetEvent(arduino_event_id_t event)
{
  switch (event) {

    case ARDUINO_EVENT_ETH_START:
      Serial.println(F("eth ........ driver started"));
      ETH.setHostname("audionode");
      break;

    case ARDUINO_EVENT_ETH_CONNECTED:
      g_linkUp = true;
      Serial.println(F("eth ........ LINK UP  (cable and switch are good)"));
      break;

    case ARDUINO_EVENT_ETH_GOT_IP:
      g_haveAddress = true;
      Serial.println();
      Serial.println(F("------------------------------------------------------------"));
      Serial.print  (F("  IP ADDRESS   "));  Serial.println(ETH.localIP());
      Serial.print  (F("  NETMASK      "));  Serial.println(ETH.subnetMask());
      Serial.print  (F("  GATEWAY      "));  Serial.println(ETH.gatewayIP());
      Serial.print  (F("  DNS          "));  Serial.println(ETH.dnsIP());
      Serial.print  (F("  MAC          "));  Serial.println(ETH.macAddress());
      Serial.printf (  "  SPEED        %d Mbps %s duplex\n",
                       ETH.linkSpeed(),
                       ETH.fullDuplex() ? "full" : "half");
      Serial.println(F("------------------------------------------------------------"));
      Serial.println();
      break;

    case ARDUINO_EVENT_ETH_LOST_IP:
      g_haveAddress = false;
      Serial.println(F("eth ........ address lost"));
      break;

    case ARDUINO_EVENT_ETH_DISCONNECTED:
      g_linkUp      = false;
      g_haveAddress = false;
      g_stackTested = false;
      Serial.println(F("eth ........ LINK DOWN  (cable pulled, or switch off)"));
      break;

    case ARDUINO_EVENT_ETH_STOP:
      g_linkUp      = false;
      g_haveAddress = false;
      Serial.println(F("eth ........ driver stopped"));
      break;

    default:
      break;
  }
}

/* ---------- one-shot proof that the whole stack works ---------- */

static void testStack()
{
  Serial.println(F("test ....... connecting to " DNS_TEST_HOST));
  Serial.println(F("             this needs dns AND routing to both work"));

  NetworkClient client;

  if (!client.connect(DNS_TEST_HOST, TCP_TEST_PORT, 8000)) {
    Serial.println(F("test ....... FAILED"));
    Serial.println(F("             the name did not resolve, or the socket did not open."));
    Serial.println(F("             check the gateway and dns printed above."));
    Serial.println(F("             a wrong dns is the usual cause."));
    return;
  }

  IPAddress addr = client.remoteIP();
  client.stop();

  Serial.print(F("test ....... reached "));
  Serial.println(addr);
  Serial.println();
  Serial.println(F("  *** NETWORK IS FULLY UP - name resolved, socket opened ***"));
  Serial.println();
}

/* ---------- setup ---------- */

void setup()
{
  Serial.begin(115200);

  unsigned long t0 = millis();
  while (!Serial && (millis() - t0) < 2000) {
    delay(10);
  }
  delay(300);

  g_bootMillis = millis();

  banner();
  printResetReason();
  printBoardFacts();
  printPinMap();
  Serial.println();

  Network.onEvent(onNetEvent);

  Serial.println(F("w5500 ...... holding reset low"));
  pinMode(PIN_ETH_RST, OUTPUT);
  digitalWrite(PIN_ETH_RST, LOW);
  delay(20);
  digitalWrite(PIN_ETH_RST, HIGH);
  delay(100);
  Serial.println(F("w5500 ...... reset released, waited 100 ms"));

  Serial.println(F("spi ........ starting"));
  SPI.begin(PIN_ETH_SCK, PIN_ETH_MISO, PIN_ETH_MOSI);

  Serial.println(F("eth ........ calling begin"));

  bool ok = ETH.begin(ETH_PHY_W5500,
                      ETH_PHY_ADDRESS,
                      PIN_ETH_CS,
                      PIN_ETH_INT,
                      PIN_ETH_RST,
                      SPI,
                      ETH_SPI_MHZ);

  if (!ok) {
    Serial.println();
    Serial.println(F("  !!! ETH.begin RETURNED FALSE !!!"));
    Serial.println(F("  the chip did not answer over SPI. that is wiring or power,"));
    Serial.println(F("  not networking. check in this order:"));
    Serial.println(F("    1. red led on the underside of the module lit?"));
    Serial.println(F("    2. common ground between module and board?"));
    Serial.println(F("    3. miso and mosi not swapped?"));
    Serial.println(F("    4. cs really on gpio 9?"));
    Serial.println();
  } else {
    Serial.println(F("eth ........ begin ok, waiting for link and dhcp"));
    Serial.println(F("             (green led on the socket = cable is live)"));
  }

  Serial.println();
}

/* ---------- loop ---------- */

void loop()
{
  if (g_haveAddress && !g_stackTested) {
    g_stackTested = true;
    testStack();
  }

  unsigned long now = millis();

  if (now - g_lastStatus >= STATUS_EVERY_MS) {
    g_lastStatus = now;

    unsigned long up = (now - g_bootMillis) / 1000;

    Serial.printf("[%5lus] link %s   address %s   heap %u",
                  up,
                  g_linkUp      ? "UP  " : "DOWN",
                  g_haveAddress ? "yes" : "no ",
                  (unsigned) ESP.getFreeHeap());

    if (g_haveAddress) {
      Serial.print(F("   "));
      Serial.print(ETH.localIP());
    }

    Serial.println();
  }

  delay(10);
}

/* ============================================================
 *                        TEST  006
 *              AUDIO NODE - ethernet bring-up only
 *            no audio in this build. not tested on hardware.
 * ============================================================ */