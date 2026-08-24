/* ============================================================
   Bg_Bed  —  listener  —  TEST 011  —  gain reduced to x8
   ============================================================ */

/*  WHAT CHANGED FROM TEST 010
    --------------------------
    Only one number:  REC_GAIN went from 32 to 8.

    TEST 010 clipped.  The level table showed 0.0 dBFS in the
    middle seconds, which is the top of the range - the waveform
    was flattened off there and no longer resembles your voice.
    Clipping cannot be undone afterwards.  It sounds harsh and
    crackly, and that is what you would hear.

    x32 is about +30 dB.  It was chosen for speech at ordinary
    distance.  Speaking close to the microphone is perhaps 12 dB
    louder than that, which is exactly the amount by which it
    overflowed.

    x8 is about +18 dB.  Close speech should now land near
    -12 dBFS, which is a strong recording with headroom left.
    Speech from across the bed will land nearer -30 dBFS, which
    is quiet but clean - and wav_convert.py normalises it
    afterwards, which costs nothing because no information was
    thrown away.

    THE RULE WORTH REMEMBERING
    -------------------------
    Too quiet can be fixed later.  Too loud cannot.

    THE TEST TO RUN
    ---------------
    Speak at the distance you actually care about - roughly
    where your head will be on the pillow, not with your mouth
    against the board.

    Check the printed clip count afterwards.  It should say
    "no clipping".  If it still clips, tell me and I will drop
    to x4.

    NOTHING HERE HAS RUN ON HARDWARE.
*/

#define TEST_NUMBER 11

#include <Arduino.h>
#include <driver/i2s_std.h>
#include <esp_heap_caps.h>
#include <string.h>

/* ---------- pins ---------- */
#define PIN_I2S_BCK   40
#define PIN_I2S_WS    41
#define PIN_I2S_SD    42

/* ---------- recording ---------- */
#define SAMPLE_RATE   16000
#define REC_SECONDS   10
#define CHANNELS      2
#define REC_GAIN      8           /* about +18 dB */

#define TOTAL_SAMPLES (SAMPLE_RATE * REC_SECONDS * CHANNELS)
#define TOTAL_BYTES   (TOTAL_SAMPLES * 2)

#define FRAMES_BLOCK  512

static i2s_chan_handle_t rxChan = NULL;
static int32_t  block[FRAMES_BLOCK * 2];
static int16_t *rec = NULL;

/* ---------- base64 ---------- */

static const char B64[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static void dumpBase64(const uint8_t *data, size_t len)
{
    char line[80];
    int  col = 0;
    size_t i = 0;

    while (i < len) {
        uint32_t v = 0;
        int have = 0;

        for (int k = 0; k < 3; k++) {
            v <<= 8;
            if (i < len) { v |= data[i++]; have++; }
        }

        line[col++] = B64[(v >> 18) & 0x3F];
        line[col++] = B64[(v >> 12) & 0x3F];
        line[col++] = (have > 1) ? B64[(v >> 6) & 0x3F] : '=';
        line[col++] = (have > 2) ? B64[v & 0x3F]        : '=';

        if (col >= 76) { line[col] = 0; Serial.println(line); col = 0; }
    }

    if (col > 0) { line[col] = 0; Serial.println(line); }
}

/* ---------- 24-bit sample, with gain, down to 16-bit ---------- */

static inline int16_t shrink(int32_t raw32, uint32_t *clipCount)
{
    int32_t v = raw32 >> 8;              /* 24-bit signed */
    int32_t g = (v * REC_GAIN) >> 8;     /* gain, then to 16-bit range */

    if (g >  32767) { (*clipCount)++; return  32767; }
    if (g < -32768) { (*clipCount)++; return -32768; }
    return (int16_t)g;
}

/* ---------- I2S ---------- */

static bool startI2S()
{
    i2s_chan_config_t chanCfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
    chanCfg.auto_clear = false;

    if (i2s_new_channel(&chanCfg, NULL, &rxChan) != ESP_OK) return false;

    i2s_std_clk_config_t  clkCfg  = I2S_STD_CLK_DEFAULT_CONFIG(SAMPLE_RATE);
    i2s_std_slot_config_t slotCfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(
                                        I2S_DATA_BIT_WIDTH_32BIT,
                                        I2S_SLOT_MODE_STEREO);

    i2s_std_gpio_config_t gpioCfg;
    memset(&gpioCfg, 0, sizeof(gpioCfg));
    gpioCfg.mclk = I2S_GPIO_UNUSED;
    gpioCfg.bclk = (gpio_num_t)PIN_I2S_BCK;
    gpioCfg.ws   = (gpio_num_t)PIN_I2S_WS;
    gpioCfg.dout = I2S_GPIO_UNUSED;
    gpioCfg.din  = (gpio_num_t)PIN_I2S_SD;
    gpioCfg.invert_flags.mclk_inv = false;
    gpioCfg.invert_flags.bclk_inv = false;
    gpioCfg.invert_flags.ws_inv   = false;

    i2s_std_config_t stdCfg;
    memset(&stdCfg, 0, sizeof(stdCfg));
    stdCfg.clk_cfg  = clkCfg;
    stdCfg.slot_cfg = slotCfg;
    stdCfg.gpio_cfg = gpioCfg;

    if (i2s_channel_init_std_mode(rxChan, &stdCfg) != ESP_OK) return false;
    if (i2s_channel_enable(rxChan) != ESP_OK) return false;

    return true;
}

/* ---------- setup ---------- */

void setup()
{
    Serial.begin(921600);
    delay(1500);

    Serial.println();
    Serial.println("=========================================");
    Serial.printf ("  Bg_Bed  listener  TEST %03d\n", TEST_NUMBER);
    Serial.printf ("  record %d s, gain x%d before 16-bit\n",
                   REC_SECONDS, REC_GAIN);
    Serial.println("=========================================");

    rec = (int16_t *)heap_caps_malloc(TOTAL_BYTES, MALLOC_CAP_SPIRAM);
    if (!rec) {
        Serial.println("  PSRAM allocation FAILED.  Halted.");
        while (true) delay(1000);
    }

    if (!startI2S()) {
        Serial.println("  I2S did not start.  Halted.");
        while (true) delay(1000);
    }

    size_t junk = 0;
    for (int i = 0; i < 12; i++)
        i2s_channel_read(rxChan, block, sizeof(block), &junk, 200);

    Serial.println();
    Serial.println("  PLAN:");
    Serial.println("    seconds 1-5   talk close to the LEFT mic");
    Serial.println("    seconds 6-10  talk close to the RIGHT mic");
    Serial.println("  Speak from pillow distance, not touching the board.");
    Serial.println();

    for (int s = 3; s > 0; s--) {
        Serial.printf("  starting in %d ...\n", s);
        delay(1000);
    }

    Serial.println();
    Serial.println("  ***  RECORDING - LEFT MIC FIRST  ***");

    size_t   written = 0;
    uint32_t clipped = 0;
    bool     halfway = false;

    while (written < TOTAL_SAMPLES) {

        size_t got = 0;
        if (i2s_channel_read(rxChan, block, sizeof(block), &got, 300) != ESP_OK)
            continue;

        size_t frames = got / (2 * sizeof(int32_t));

        for (size_t i = 0; i < frames && written < TOTAL_SAMPLES; i++) {
            rec[written++] = shrink(block[i * 2 + 0], &clipped);
            rec[written++] = shrink(block[i * 2 + 1], &clipped);
        }

        if (!halfway && written >= TOTAL_SAMPLES / 2) {
            halfway = true;
            Serial.println("  ***  NOW THE RIGHT MIC  ***");
        }
    }

    Serial.println("  ***  DONE  ***");

    if (clipped > 0) {
        Serial.printf("  %lu samples clipped - tell me and I will lower the gain\n",
                      (unsigned long)clipped);
    } else {
        Serial.println("  no clipping");
    }

    Serial.println();
    Serial.println("  Dumping.  About 15 seconds.");
    Serial.println();
    delay(300);

    Serial.println("-----BEGIN AUDIO-----");
    Serial.printf ("rate=%d channels=%d bits=16 bytes=%d\n",
                   SAMPLE_RATE, CHANNELS, TOTAL_BYTES);
    dumpBase64((const uint8_t *)rec, TOTAL_BYTES);
    Serial.println("-----END AUDIO-----");

    Serial.println();
    Serial.println("  Finished.  Press RST to record again.");
}

void loop()
{
    delay(1000);
}

/* ============================================================
   END  —  Bg_Bed  —  listener  —  TEST 011
   ============================================================ */