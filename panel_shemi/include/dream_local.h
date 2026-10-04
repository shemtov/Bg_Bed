#pragma once

// Local Dreams path: control/status over the existing coordinated A5 bus.
// No audio samples travel on RS485. AudioNode pulls WAV over the wired link.
static constexpr uint8_t NODE_LISTENER = 8;
static constexpr uint8_t TYPE_DREAM_CMD = 0x50;
static constexpr uint8_t TYPE_DREAM_ACK = 0x51;
static constexpr uint8_t DREAM_START = 1;
static constexpr uint8_t DREAM_STOP = 2;
static constexpr uint8_t DREAM_STATUS = 3;
static constexpr uint8_t DREAM_LIST = 4;
static constexpr uint8_t DREAM_PLAY_LATEST = 8;
static constexpr uint8_t DREAM_STOP_PLAY = 9;
static constexpr uint8_t DREAM_PLAY_STATUS = 10;
static constexpr uint8_t DREAM_PLAY_SELECTED = 11;
static constexpr uint8_t DREAM_PAUSE_PLAY = 12;
static constexpr uint8_t DREAM_RESUME_PLAY = 13;
static constexpr uint8_t DREAM_SEEK_PLAY = 14;
static constexpr uint8_t DREAM_VOLUME_PLAY = 15;
static constexpr uint8_t DREAM_SEEK_TO_PLAY = 16;
static constexpr uint8_t DREAM_MAX_FILES = 64;

struct DreamRequest { uint8_t target; uint8_t command; uint8_t index; uint8_t part; uint16_t key; };
struct DreamReply {
  uint8_t target;
  uint8_t command;
  uint8_t result;
  uint8_t state;
  uint8_t latest;
  uint8_t index;
  uint8_t part;
  uint8_t count;
  uint8_t data[4];
  uint16_t positionSeconds;
  uint16_t durationSeconds;
};
struct DreamEntry {
  uint8_t yy, month, day, hour, minute, second, suffix;
  uint8_t parts;
  uint32_t bytes;
  uint16_t key;
};
static DreamEntry dreamEntries[DREAM_MAX_FILES] = {};
static uint8_t dreamCount = 255;
static uint8_t dreamPage = 0;
static bool dreamBrowserOpen = false;
static bool dreamListPending = false;
static uint32_t dreamListSince = 0;
static lv_obj_t *dreamBrowser = nullptr;
static lv_obj_t *dreamBrowserInfo = nullptr;
static lv_obj_t *dreamRowLabel[4] = {};
static lv_obj_t *dreamBaseButtons[11] = {};

static QueueHandle_t dreamRequestQueue = nullptr;
static QueueHandle_t dreamReplyQueue = nullptr;
static lv_obj_t *dreamStatusLabel = nullptr;
static lv_obj_t *dreamErrorLabel = nullptr;
static lv_obj_t *dreamErrorDot = nullptr;
static lv_obj_t *dreamRecordLabel = nullptr;
static lv_obj_t *dreamPlayLabel = nullptr;
static lv_obj_t *dreamSecondLabel = nullptr;
static lv_obj_t *dreamThirdLabel = nullptr;
static lv_obj_t *dreamRecordDot = nullptr;
static lv_obj_t *dreamMicLeftBar = nullptr, *dreamMicRightBar = nullptr;
static uint8_t dreamMicLeftLevel = 0, dreamMicRightLevel = 0;
static lv_obj_t *dreamHomeDot = nullptr;
static lv_obj_t *dreamProgress = nullptr, *dreamTimeLabel = nullptr;
static lv_obj_t *dreamPauseLabel = nullptr, *dreamVolumeLabel = nullptr;
static lv_obj_t *dreamVolumeSlider = nullptr;
static lv_obj_t *dreamWaitCard = nullptr;
static lv_obj_t *dreamWaitLabel = nullptr;
static lv_obj_t *dreamWaitDots[8] = {};
static bool dreamWaitRequested = false;
static bool dreamWaitVisible = false;
static uint32_t dreamWaitSince = 0, dreamWaitLastSecond = 0xFFFFFFFFU;
static uint8_t dreamWaitLastPhase = 0xFF;
static bool dreamRecorderKnown = false;
static bool dreamRecording = false;
static bool dreamLatestAvailable = false;
static bool dreamAudioKnown = false;
static uint8_t dreamAudioState = 0;
static uint16_t dreamPositionSeconds = 0, dreamDurationSeconds = 0;
static uint8_t dreamVolume = 11;
static uint32_t dreamRecorderReplyMs = 0;
static uint32_t dreamAudioReplyMs = 0;
static const char *dreamMessage = "Checking Listener...";
// A playback error survives routine Listener and AudioNode status replies.
// Clear it only when another play is requested or playback actually starts.
static const char *dreamPlaybackError = nullptr;

static bool dreamQueue(uint8_t target, uint8_t command, uint8_t index = 0,
                       uint8_t part = 0, uint16_t key = 0) {
  if (!dreamRequestQueue) return false;
  const DreamRequest request{target, command, index, part, key};
  return xQueueSend(dreamRequestQueue, &request, 0) == pdTRUE;
}

// Called only from the existing panel-owned RS485 service slot, never from LVGL.
static bool dreamService() {
  DreamRequest request{};
  if (!dreamRequestQueue ||
      xQueueReceive(dreamRequestQueue, &request, 0) != pdTRUE) return false;
  TimeFrame tx{};
  tx.start = FRAME_START;
  tx.src = PANEL_ID;
  tx.dst = request.target;
  tx.type = request.target == NODE_LISTENER ? TYPE_DREAM_CMD : TYPE_AUDIO_CMD;
  const uint16_t seq = rsAudioSeq++;
  tx.seqLo = uint8_t(seq);
  tx.seqHi = uint8_t(seq >> 8);
  tx.yearLo = request.command;
  if (request.command == DREAM_LIST && request.target == NODE_LISTENER) {
    tx.yearHi = request.index;
    tx.month = request.part;
  }
  if (request.target == NODE_AUDIO) {
    if (request.command == DREAM_PLAY_SELECTED) {
      tx.yearHi = request.index;
      tx.month = uint8_t(request.key);
      tx.day = uint8_t(request.key >> 8);
    } else if (request.command == DREAM_SEEK_PLAY ||
               request.command == DREAM_SEEK_TO_PLAY ||
               request.command == DREAM_VOLUME_PLAY) {
      tx.yearHi = request.index; // seek: 0=back, 1=forward; volume: 0..21
    } else {
      tx.month = 11; // existing AudioNode volume scale, approximately half
      tx.day = 0;    // dream playback does not use a radio fade
    }
  }
  tx.crc = rsFrameCrc(tx);
  rsSendFrame(tx);
  Serial.printf("[DREAM TX] dst=%u cmd=%u seq=%u\n",
                request.target, request.command, seq);

  DreamReply reply{request.target, request.command, 255, 0, 0,
                   request.index, request.part, 0, {0, 0, 0, 0}, 0, 0};
  const uint32_t began = millis();
  // Ira's complete transaction must fit inside Shemi's 900 ms bus grant.
  const uint32_t timeout = request.target == NODE_AUDIO ? 650 : 500;
  TimeFrame rx{};
  while (millis() - began < timeout) {
    if (!rsReadFrame(rx)) { delay(1); continue; }
    if (rx.crc != rsFrameCrc(rx)) continue;
    if (rsDispatchBedboxReply(rx)) continue;
    if (rsDispatchAcReply(rx)) continue;
    if (rx.src != request.target || rx.dst != PANEL_ID ||
        rx.type != (request.target == NODE_LISTENER ? TYPE_DREAM_ACK : TYPE_AUDIO_ACK) ||
        rsFrameSeq(rx) != seq || rx.yearLo != request.command) continue;
    reply.result = rx.yearHi;
    if (request.command == DREAM_LIST && request.target == NODE_LISTENER) {
      reply.count = rx.month;
      reply.data[0] = rx.day; reply.data[1] = rx.hour;
      reply.data[2] = rx.minute; reply.data[3] = rx.second;
    } else {
      reply.state = rx.month;
      reply.latest = rx.day;
      if (request.target == NODE_LISTENER) {
        reply.data[0] = rx.hour;   // left microphone VU, 0..100
        reply.data[1] = rx.minute; // right microphone VU, 0..100
      }
      if (request.target == NODE_AUDIO) {
        reply.positionSeconds = uint16_t(rx.hour) | (uint16_t(rx.minute & 15) << 8);
        reply.durationSeconds = uint16_t(rx.minute >> 4) | (uint16_t(rx.second) << 4);
      }
    }
    break;
  }
  Serial.printf("[DREAM RX] dst=%u cmd=%u result=%u state=%u latest=%u\n",
                reply.target, reply.command, reply.result, reply.state, reply.latest);
  if (dreamReplyQueue) xQueueOverwrite(dreamReplyQueue, &reply);
  return true;
}

static uint16_t dreamFilenameKey(const DreamEntry &e) {
  char filename[36];
  if (e.suffix)
    snprintf(filename, sizeof(filename), "/%02u%02u%02u_%02u%02u%02u_%u.WAV",
             e.yy, e.month, e.day, e.hour, e.minute, e.second, e.suffix);
  else
    snprintf(filename, sizeof(filename), "/%02u%02u%02u_%02u%02u%02u.WAV",
             e.yy, e.month, e.day, e.hour, e.minute, e.second);
  uint16_t crc = 0xFFFF;
  for (const char *p = filename; *p; ++p) {
    crc ^= uint8_t(*p);
    for (uint8_t bit = 0; bit < 8; ++bit)
      crc = (crc & 1) ? (crc >> 1) ^ 0xA001 : crc >> 1;
  }
  return crc;
}

static void dreamResetCatalog() {
  memset(dreamEntries, 0, sizeof(dreamEntries));
  dreamCount = 255;
  dreamListPending = false;
}

static bool dreamPlayRank(uint8_t rank) {
  if (dreamRecording) { dreamMessage = "Stop and save recording before playback."; return false; }
  if (dreamCount != 255 && rank >= dreamCount) {
    dreamMessage = "This recording is not on the SD card.";
    return false;
  }
  if (rank >= DREAM_MAX_FILES || dreamCount == 255 || dreamEntries[rank].parts != 7) {
    dreamMessage = "Loading saved files; tap again shortly.";
    return false;
  }
  if (!dreamQueue(NODE_AUDIO, DREAM_PLAY_SELECTED, rank, 0, dreamEntries[rank].key)) {
    dreamMessage = "Busy. Try again.";
    return false;
  }
  dreamPlaybackError = nullptr;
  dreamMessage = "Connecting to selected WAV...";
  dreamWaitRequested = true;
  dreamWaitSince = millis();
  return true;
}

static void dreamSecondClicked(lv_event_t *) { dreamPlayRank(1); }
static void dreamThirdClicked(lv_event_t *) { dreamPlayRank(2); }
static void dreamRowClicked(lv_event_t *event) {
  const uint8_t row = uint8_t(uintptr_t(lv_event_get_user_data(event)));
  const uint8_t index = dreamPage * 4 + row;
  if (index >= DREAM_MAX_FILES || dreamCount == 255 || index >= dreamCount ||
      dreamEntries[index].parts != 7) {
    dreamPlayRank(index);
    return;
  }
  if (dreamPlayRank(index) && dreamBrowserOpen) {
    dreamBrowserOpen = false;
    lv_obj_add_flag(dreamBrowser, LV_OBJ_FLAG_HIDDEN);
    for (lv_obj_t *button : dreamBaseButtons)
      if (button) lv_obj_clear_flag(button, LV_OBJ_FLAG_HIDDEN);
    accessibilityPrepare(lv_scr_act());
  }
}
static void dreamFilesClicked(lv_event_t *) {
  dreamBrowserOpen = !dreamBrowserOpen;
  if (dreamBrowserOpen) {
    dreamPage = 0;
    dreamResetCatalog();
    for (lv_obj_t *button : dreamBaseButtons)
      if (button) lv_obj_add_flag(button, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(dreamBrowser, LV_OBJ_FLAG_HIDDEN);
  } else {
    lv_obj_add_flag(dreamBrowser, LV_OBJ_FLAG_HIDDEN);
    for (lv_obj_t *button : dreamBaseButtons)
      if (button) lv_obj_clear_flag(button, LV_OBJ_FLAG_HIDDEN);
  }
  // Rebuild the enlarged hit map now. The global panel accessibility pass
  // otherwise keeps the covered PLAY buttons in front of these four controls.
  accessibilityPrepare(lv_scr_act());
}
static void dreamCloseBrowserOnLeave(lv_event_t *) {
  if (!dreamBrowserOpen) return;
  dreamBrowserOpen = false;
  lv_obj_add_flag(dreamBrowser, LV_OBJ_FLAG_HIDDEN);
  for (lv_obj_t *button : dreamBaseButtons)
    if (button) lv_obj_clear_flag(button, LV_OBJ_FLAG_HIDDEN);
}
static void dreamPrevClicked(lv_event_t *) {
  if (dreamPage) --dreamPage;
}
static void dreamNextClicked(lv_event_t *) {
  if (dreamCount != 255 && (dreamPage + 1) * 4 < dreamCount) ++dreamPage;
}
static void dreamRefreshClicked(lv_event_t *) { dreamResetCatalog(); }

static void dreamRecordClicked(lv_event_t *) {
  if (!dreamQueue(NODE_LISTENER, dreamRecording ? DREAM_STOP : DREAM_START))
    dreamMessage = "Busy. Try again.";
  else dreamMessage = dreamRecording ? "Saving recording..." : "Starting recording...";
}
static void dreamPlayClicked(lv_event_t *) {
  if (dreamRecording) { dreamMessage = "Stop and save recording before playback."; return; }
  if (!dreamLatestAvailable) {
    dreamMessage = "No saved WAV confirmed yet.";
    return;
  }
  if (!dreamQueue(NODE_AUDIO, DREAM_PLAY_LATEST)) dreamMessage = "Busy. Try again.";
  else {
    dreamPlaybackError = nullptr;
    dreamMessage = "Connecting to saved WAV...";
    dreamWaitRequested = true;
    dreamWaitSince = millis();
  }
}

static void dreamPauseClicked(lv_event_t *) {
  if (dreamAudioState != 2 && dreamAudioState != 4) return;
  const uint8_t command = dreamAudioState == 4 ? DREAM_RESUME_PLAY : DREAM_PAUSE_PLAY;
  if (!dreamQueue(NODE_AUDIO, command)) dreamMessage = "Busy. Try again.";
  else dreamMessage = command == DREAM_PAUSE_PLAY ? "Pausing..." : "Resuming...";
}
static void dreamStopClicked(lv_event_t *) {
  if (!dreamQueue(NODE_AUDIO, DREAM_STOP_PLAY)) dreamMessage = "Busy. Try again.";
  else { dreamMessage = "Stopping playback..."; dreamWaitRequested = false; }
}
static void dreamSeekClicked(lv_event_t *event) {
  if (dreamAudioState != 2 && dreamAudioState != 4) return;
  const uint8_t direction = uint8_t(uintptr_t(lv_event_get_user_data(event)));
  if (!dreamQueue(NODE_AUDIO, DREAM_SEEK_PLAY, direction)) dreamMessage = "Busy. Try again.";
  else dreamMessage = direction ? "Forward 10 seconds..." : "Back 10 seconds...";
}
static void dreamVolumeReleased(lv_event_t *event) {
  const uint8_t value = uint8_t(lv_slider_get_value(lv_event_get_target(event)));
  if (!dreamQueue(NODE_AUDIO, DREAM_VOLUME_PLAY, value)) dreamMessage = "Busy. Try again.";
  else dreamVolume = value;
}
static void dreamProgressReleased(lv_event_t *event) {
  if (dreamAudioState != 2 && dreamAudioState != 4) return;
  const uint8_t percent = uint8_t(lv_slider_get_value(lv_event_get_target(event)));
  if (!dreamQueue(NODE_AUDIO, DREAM_SEEK_TO_PLAY, percent))
    dreamMessage = "Busy. Try again.";
  else dreamMessage = "Moving to selected time...";
}

static lv_obj_t *dreamPanel(lv_obj_t *screen, int x, int y, int w, int h) {
  lv_obj_t *panel = lv_obj_create(screen);
  lv_obj_set_pos(panel, x, y);
  lv_obj_set_size(panel, w, h);
  lv_obj_set_style_bg_color(panel, lv_color_hex(0x132843), 0);
  lv_obj_set_style_bg_opa(panel, LV_OPA_COVER, 0);
  lv_obj_set_style_border_color(panel, lv_color_hex(0x3C7899), 0);
  lv_obj_set_style_border_width(panel, 2, 0);
  lv_obj_set_style_radius(panel, 18, 0);
  lv_obj_clear_flag(panel, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
  return panel;
}

static void dreamStyleButton(lv_obj_t *button, uint32_t bg, uint32_t border) {
  lv_obj_set_style_bg_color(button, lv_color_hex(bg), 0);
  lv_obj_set_style_border_color(button, lv_color_hex(border), 0);
  lv_obj_set_style_border_width(button, 2, 0);
  lv_obj_set_style_radius(button, 16, 0);
  lv_obj_set_style_shadow_width(button, 0, 0);
}

// Design 8: recorder and playback at left; a tall saved-files rectangle at right.
static void dreamBuildControls(lv_obj_t *screen, lv_obj_t *picture) {
  lv_obj_set_style_bg_color(screen, lv_color_hex(0x0A1221), 0);
  dreamPanel(screen, 18, 82, 672, 472);
  dreamPanel(screen, 704, 82, 302, 472);
  lv_img_set_zoom(picture, 112);
  lv_obj_align(picture, LV_ALIGN_TOP_LEFT, 38, 182);
  lv_obj_move_foreground(picture);

  dreamRecordDot = lv_obj_create(screen);
  lv_obj_remove_style_all(dreamRecordDot);
  lv_obj_set_size(dreamRecordDot, 38, 38);
  lv_obj_set_style_radius(dreamRecordDot, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_bg_color(dreamRecordDot, lv_color_hex(0xF01828), 0);
  lv_obj_set_style_bg_opa(dreamRecordDot, LV_OPA_COVER, 0);
  lv_obj_align_to(dreamRecordDot, picture, LV_ALIGN_BOTTOM_RIGHT, -10, -8);
  lv_obj_clear_flag(dreamRecordDot, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(dreamRecordDot, LV_OBJ_FLAG_HIDDEN);

  dreamStatusLabel = lv_label_create(screen);
  lv_obj_set_width(dreamStatusLabel, 620);
  lv_label_set_long_mode(dreamStatusLabel, LV_LABEL_LONG_WRAP);
  lv_obj_set_style_text_align(dreamStatusLabel, LV_TEXT_ALIGN_LEFT, 0);
  lv_obj_set_style_text_font(dreamStatusLabel, &lv_font_montserrat_26, 0);
  lv_obj_set_style_text_color(dreamStatusLabel, lv_color_hex(0xE8EFFA), 0);
  lv_obj_align(dreamStatusLabel, LV_ALIGN_TOP_LEFT, 40, 91);

  // A visible wave while the verified WAV is being transferred. The Audio
  // Node reports "connecting" throughout transfer; no fake percentage.
  dreamWaitCard = lv_obj_create(screen);
  lv_obj_set_size(dreamWaitCard, 385, 69);
  lv_obj_align(dreamWaitCard, LV_ALIGN_TOP_LEFT, 284, 167);
  lv_obj_set_style_bg_color(dreamWaitCard, lv_color_hex(0x173453), 0);
  lv_obj_set_style_bg_opa(dreamWaitCard, LV_OPA_COVER, 0);
  lv_obj_set_style_border_color(dreamWaitCard, lv_color_hex(0x23D5E8), 0);
  lv_obj_set_style_border_width(dreamWaitCard, 2, 0);
  lv_obj_set_style_radius(dreamWaitCard, 17, 0);
  lv_obj_clear_flag(dreamWaitCard, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
  for (uint8_t i = 0; i < 8; ++i) {
    dreamWaitDots[i] = lv_obj_create(dreamWaitCard);
    lv_obj_remove_style_all(dreamWaitDots[i]);
    lv_obj_set_size(dreamWaitDots[i], 14, 14);
    lv_obj_align(dreamWaitDots[i], LV_ALIGN_TOP_LEFT, 31 + 39 * i, 9);
    lv_obj_set_style_radius(dreamWaitDots[i], LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_opa(dreamWaitDots[i], LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(dreamWaitDots[i], lv_color_hex(0x27839F), 0);
    lv_obj_clear_flag(dreamWaitDots[i], LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
  }
  dreamWaitLabel = lv_label_create(dreamWaitCard);
  lv_label_set_text(dreamWaitLabel, "PREPARING 00:00");
  lv_obj_set_style_text_font(dreamWaitLabel, &lv_font_montserrat_26, 0);
  lv_obj_set_style_text_color(dreamWaitLabel, lv_color_hex(0xD4F9FF), 0);
  lv_obj_align(dreamWaitLabel, LV_ALIGN_BOTTOM_MID, 0, -1);
  lv_obj_add_flag(dreamWaitCard, LV_OBJ_FLAG_HIDDEN);

  dreamErrorDot = lv_obj_create(screen);
  lv_obj_remove_style_all(dreamErrorDot);
  lv_obj_set_size(dreamErrorDot, 18, 18);
  lv_obj_set_style_radius(dreamErrorDot, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_bg_color(dreamErrorDot, lv_color_hex(0xF01828), 0);
  lv_obj_set_style_bg_opa(dreamErrorDot, LV_OPA_COVER, 0);
  lv_obj_align(dreamErrorDot, LV_ALIGN_TOP_LEFT, 43, 128);
  lv_obj_clear_flag(dreamErrorDot, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(dreamErrorDot, LV_OBJ_FLAG_HIDDEN);
  dreamErrorLabel = lv_label_create(screen);
  lv_obj_set_width(dreamErrorLabel, 590);
  lv_label_set_long_mode(dreamErrorLabel, LV_LABEL_LONG_WRAP);
  lv_obj_set_style_text_font(dreamErrorLabel, &lv_font_montserrat_26, 0);
  lv_obj_set_style_text_color(dreamErrorLabel, lv_color_hex(0xFF6670), 0);
  lv_obj_align(dreamErrorLabel, LV_ALIGN_TOP_LEFT, 72, 122);
  lv_label_set_text(dreamErrorLabel, "");

  lv_obj_t *savedTitle = lv_label_create(screen);
  lv_label_set_text(savedTitle, "RECORDINGS");
  lv_obj_set_style_text_font(savedTitle, &lv_font_montserrat_26, 0);
  lv_obj_set_style_text_color(savedTitle, lv_color_hex(0x00D6EB), 0);
  lv_obj_align(savedTitle, LV_ALIGN_TOP_LEFT, 727, 101);

  lv_obj_t *recordButton = lv_btn_create(screen);
  dreamBaseButtons[0] = recordButton;
  lv_obj_set_size(recordButton, 230, 186); // three times the original 60-pixel height
  lv_obj_align(recordButton, LV_ALIGN_TOP_LEFT, 36, 345);
  dreamStyleButton(recordButton, 0xBA2548, 0xFF799B);
  lv_obj_add_event_cb(recordButton, dreamRecordClicked, LV_EVENT_CLICKED, nullptr);
  dreamRecordLabel = lv_label_create(recordButton);
  lv_obj_set_style_text_font(dreamRecordLabel, &lv_font_montserrat_26, 0);
  lv_obj_align(dreamRecordLabel, LV_ALIGN_TOP_MID, 0, 18);
  lv_obj_t *leftLabel = lv_label_create(recordButton);
  lv_label_set_text(leftLabel, "L");
  lv_obj_align(leftLabel, LV_ALIGN_TOP_LEFT, 10, 91);
  lv_obj_t *rightLabel = lv_label_create(recordButton);
  lv_label_set_text(rightLabel, "R");
  lv_obj_align(rightLabel, LV_ALIGN_TOP_LEFT, 10, 128);
  for (uint8_t i = 0; i < 2; ++i) {
    lv_obj_t *track = lv_obj_create(recordButton);
    lv_obj_remove_style_all(track);
    lv_obj_set_size(track, 164, 19);
    lv_obj_align(track, LV_ALIGN_TOP_LEFT, 28, i ? 130 : 93);
    lv_obj_set_style_bg_color(track, lv_color_hex(0x172338), 0);
    lv_obj_set_style_bg_opa(track, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(track, 8, 0);
    lv_obj_clear_flag(track, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_t *fill = lv_obj_create(track);
    lv_obj_remove_style_all(fill);
    lv_obj_set_size(fill, 1, 19);
    lv_obj_align(fill, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_set_style_bg_opa(fill, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(fill, 8, 0);
    lv_obj_clear_flag(fill, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    if (i) dreamMicRightBar = fill; else dreamMicLeftBar = fill;
  }

  lv_obj_t *playButton = lv_btn_create(screen);
  dreamBaseButtons[1] = playButton;
  lv_obj_set_size(playButton, 265, 76);
  lv_obj_align(playButton, LV_ALIGN_TOP_LEFT, 723, 154);
  dreamStyleButton(playButton, 0x00B6D1, 0x74EAF7);
  lv_obj_add_event_cb(playButton, dreamPlayClicked, LV_EVENT_CLICKED, nullptr);
  dreamPlayLabel = lv_label_create(playButton);
  lv_obj_center(dreamPlayLabel);
  lv_obj_set_style_text_font(dreamPlayLabel, &lv_font_montserrat_26, 0);

  lv_obj_t *second = lv_btn_create(screen);
  dreamBaseButtons[2] = second;
  lv_obj_set_size(second, 265, 76);
  lv_obj_align(second, LV_ALIGN_TOP_LEFT, 723, 242);
  dreamStyleButton(second, 0x204B70, 0x537995);
  lv_obj_add_event_cb(second, dreamSecondClicked, LV_EVENT_CLICKED, nullptr);
  dreamSecondLabel = lv_label_create(second);
  lv_obj_center(dreamSecondLabel);
  lv_obj_set_style_text_font(dreamSecondLabel, &lv_font_montserrat_26, 0);

  lv_obj_t *third = lv_btn_create(screen);
  dreamBaseButtons[3] = third;
  lv_obj_set_size(third, 265, 76);
  lv_obj_align(third, LV_ALIGN_TOP_LEFT, 723, 330);
  dreamStyleButton(third, 0x204B70, 0x537995);
  lv_obj_add_event_cb(third, dreamThirdClicked, LV_EVENT_CLICKED, nullptr);
  dreamThirdLabel = lv_label_create(third);
  lv_obj_center(dreamThirdLabel);
  lv_obj_set_style_text_font(dreamThirdLabel, &lv_font_montserrat_26, 0);

  lv_obj_t *files = lv_btn_create(screen);
  dreamBaseButtons[4] = files;
  lv_obj_set_size(files, 265, 79);
  lv_obj_align(files, LV_ALIGN_TOP_LEFT, 723, 429);
  dreamStyleButton(files, 0x103C42, 0x50BDBB);
  lv_obj_add_event_cb(files, dreamFilesClicked, LV_EVENT_CLICKED, nullptr);
  lv_obj_t *filesLabel = lv_label_create(files);
  lv_label_set_text(filesLabel, "ALL FILES");
  lv_obj_center(filesLabel);
  lv_obj_set_style_text_font(filesLabel, &lv_font_montserrat_26, 0);

  dreamProgress = lv_slider_create(screen);
  dreamBaseButtons[10] = dreamProgress;
  lv_obj_set_size(dreamProgress, 385, 27);
  lv_obj_align(dreamProgress, LV_ALIGN_TOP_LEFT, 284, 250);
  lv_slider_set_range(dreamProgress, 0, 100);
  lv_slider_set_value(dreamProgress, 0, LV_ANIM_OFF);
  lv_obj_add_event_cb(dreamProgress, dreamProgressReleased, LV_EVENT_RELEASED, nullptr);
  dreamTimeLabel = lv_label_create(screen);
  lv_obj_set_style_text_color(dreamTimeLabel, lv_color_hex(0xC5D7EE), 0);
  lv_obj_align(dreamTimeLabel, LV_ALIGN_TOP_LEFT, 490, 284);
  lv_label_set_text(dreamTimeLabel, "00:00 / 00:00");

  const char *transportNames[] = {"-10s", "PAUSE", "+10s", "STOP"};
  lv_event_cb_t transportEvents[] = {dreamSeekClicked, dreamPauseClicked,
                                      dreamSeekClicked, dreamStopClicked};
  for (uint8_t i = 0; i < 4; ++i) {
    lv_obj_t *button = lv_btn_create(screen);
    dreamBaseButtons[5 + i] = button;
    lv_obj_set_size(button, 90, 63);
    lv_obj_align(button, LV_ALIGN_TOP_LEFT, 284 + 98 * i, 346);
    dreamStyleButton(button, 0x204B70, 0x537995);
    lv_obj_add_event_cb(button, transportEvents[i], LV_EVENT_CLICKED,
                        (void *)(uintptr_t)(i == 2));
    lv_obj_t *label = lv_label_create(button);
    lv_label_set_text(label, transportNames[i]);
    lv_obj_center(label);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_26, 0);
    if (i == 1) dreamPauseLabel = label;
  }
  dreamVolumeLabel = lv_label_create(screen);
  lv_obj_align(dreamVolumeLabel, LV_ALIGN_TOP_LEFT, 289, 439);
  lv_obj_set_style_text_color(dreamVolumeLabel, lv_color_hex(0xD6E7F9), 0);
  lv_label_set_text(dreamVolumeLabel, "Volume 11/21");
  dreamVolumeSlider = lv_slider_create(screen);
  dreamBaseButtons[9] = dreamVolumeSlider;
  lv_obj_set_size(dreamVolumeSlider, 373, 27);
  lv_obj_align(dreamVolumeSlider, LV_ALIGN_TOP_LEFT, 289, 486);
  lv_slider_set_range(dreamVolumeSlider, 0, 21);
  lv_slider_set_value(dreamVolumeSlider, dreamVolume, LV_ANIM_OFF);
  lv_obj_add_event_cb(dreamVolumeSlider, dreamVolumeReleased, LV_EVENT_RELEASED, nullptr);

  dreamBrowser = lv_obj_create(screen);
  lv_obj_set_size(dreamBrowser, 950, 390);
  lv_obj_align(dreamBrowser, LV_ALIGN_TOP_MID, 0, 80);
  lv_obj_set_style_bg_color(dreamBrowser, lv_color_hex(0x172338), 0);
  lv_obj_set_style_bg_opa(dreamBrowser, LV_OPA_COVER, 0);
  lv_obj_clear_flag(dreamBrowser, LV_OBJ_FLAG_SCROLLABLE);
  dreamBrowserInfo = lv_label_create(dreamBrowser);
  lv_obj_align(dreamBrowserInfo, LV_ALIGN_TOP_MID, 0, 0);
  lv_obj_set_style_text_font(dreamBrowserInfo, &lv_font_montserrat_26, 0);
  for (uint8_t i = 0; i < 4; ++i) {
    lv_obj_t *row = lv_btn_create(dreamBrowser);
    lv_obj_set_size(row, 870, 62);
    lv_obj_align(row, LV_ALIGN_TOP_MID, 0, 42 + 67 * i);
    lv_obj_add_event_cb(row, dreamRowClicked, LV_EVENT_CLICKED, (void *)(uintptr_t)i);
    dreamRowLabel[i] = lv_label_create(row);
    lv_obj_center(dreamRowLabel[i]);
    lv_obj_set_style_text_font(dreamRowLabel[i], &lv_font_montserrat_26, 0);
  }
  const char *names[] = {"PREV", "NEXT", "REFRESH", "CLOSE"};
  lv_event_cb_t callbacks[] = {dreamPrevClicked, dreamNextClicked,
                               dreamRefreshClicked, dreamFilesClicked};
  for (uint8_t i = 0; i < 4; ++i) {
    lv_obj_t *button = lv_btn_create(dreamBrowser);
    lv_obj_set_size(button, 190, 60);
    lv_obj_align(button, LV_ALIGN_BOTTOM_LEFT, 15 + 225 * i, -2);
    lv_obj_add_event_cb(button, callbacks[i], LV_EVENT_CLICKED, nullptr);
    lv_obj_t *label = lv_label_create(button);
    lv_label_set_text(label, names[i]);
    lv_obj_center(label);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_26, 0);
  }
  lv_obj_add_flag(dreamBrowser, LV_OBJ_FLAG_HIDDEN);
}

static void dreamBuildHomeDot(lv_obj_t *tile) {
  dreamHomeDot = lv_obj_create(tile);
  lv_obj_remove_style_all(dreamHomeDot);
  lv_obj_set_size(dreamHomeDot, 45, 45);
  lv_obj_align(dreamHomeDot, LV_ALIGN_BOTTOM_RIGHT, -16, -16);
  lv_obj_set_style_radius(dreamHomeDot, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_bg_color(dreamHomeDot, lv_color_hex(0xF01828), 0);
  lv_obj_set_style_bg_opa(dreamHomeDot, LV_OPA_COVER, 0);
  lv_obj_clear_flag(dreamHomeDot, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(dreamHomeDot, LV_OBJ_FLAG_HIDDEN);
}

static void dreamCatalogTick(uint32_t nowMs) {
  if (!scrDreams || lv_scr_act() != scrDreams || dreamCount == 0) return;
  if (dreamListPending) {
    if (nowMs - dreamListSince < 7000) return;
    dreamListPending = false;
  }
  const uint8_t first = dreamBrowserOpen ? dreamPage * 4 : 0;
  const uint8_t last = dreamBrowserOpen ? first + 4 : 3;
  for (uint8_t index = first; index < last && index < DREAM_MAX_FILES; ++index) {
    if (dreamCount != 255 && index >= dreamCount) return;
    DreamEntry &entry = dreamEntries[index];
    if (entry.parts == 7) continue;
    for (uint8_t part = 0; part < 3; ++part) {
      if (entry.parts & (1 << part)) continue;
      if (dreamQueue(NODE_LISTENER, DREAM_LIST, index, part)) {
        dreamListPending = true;
        dreamListSince = nowMs;
      }
      return;
    }
  }
}

static void dreamPaintCatalog() {
  if (dreamSecondLabel) lv_label_set_text(dreamSecondLabel,
      "2ND LATEST");
  if (dreamThirdLabel) lv_label_set_text(dreamThirdLabel,
      "3RD LATEST");
  if (!dreamBrowserOpen || !dreamBrowserInfo) return;
  char info[90];
  if (dreamCount == 255) snprintf(info, sizeof(info), "Reading Listener SD card...");
  else snprintf(info, sizeof(info), "SAVED WAV: %u  |  PAGE %u/%u  |  Tap a file to play",
                dreamCount, dreamPage + 1, (dreamCount + 3) / 4 ? (dreamCount + 3) / 4 : 1);
  lv_label_set_text(dreamBrowserInfo, info);
  for (uint8_t row = 0; row < 4; ++row) {
    const uint8_t index = dreamPage * 4 + row;
    char label[95];
    if (dreamCount != 255 && index >= dreamCount)
      label[0] = '\0';
    else if (dreamEntries[index].parts != 7)
      snprintf(label, sizeof(label), "%u. Loading...", index + 1);
    else {
      const DreamEntry &e = dreamEntries[index];
      if (e.suffix)
        snprintf(label, sizeof(label), "%u. %02u/%02u/%02u  %02u:%02u:%02u #%u  %.1f MB",
                 index + 1, e.day, e.month, e.yy, e.hour, e.minute, e.second,
                 e.suffix, double(e.bytes) / 1048576.0);
      else
        snprintf(label, sizeof(label), "%u. %02u/%02u/%02u  %02u:%02u:%02u  %.1f MB",
                 index + 1, e.day, e.month, e.yy, e.hour, e.minute, e.second,
                 double(e.bytes) / 1048576.0);
    }
    lv_label_set_text(dreamRowLabel[row], label);
  }
}

// LVGL task: only confirmed replies change the dot and player indication.
static void dreamUiTick() {
  DreamReply reply{};
  if (dreamReplyQueue && xQueueReceive(dreamReplyQueue, &reply, 0) == pdTRUE) {
    if (reply.target == NODE_LISTENER && reply.command == DREAM_LIST) {
      dreamListPending = false;
      if (reply.result == 0 && reply.index < DREAM_MAX_FILES && reply.part < 3) {
        if (dreamCount != 255 && dreamCount != reply.count) dreamResetCatalog();
        dreamCount = reply.count;
        DreamEntry &e = dreamEntries[reply.index];
        if (reply.part == 0) {
          e.yy = reply.data[0]; e.month = reply.data[1];
          e.day = reply.data[2]; e.hour = reply.data[3];
        } else if (reply.part == 1) {
          e.minute = reply.data[0]; e.second = reply.data[1]; e.suffix = reply.data[2];
        } else {
          e.bytes = uint32_t(reply.data[0]) | (uint32_t(reply.data[1]) << 8) |
                    (uint32_t(reply.data[2]) << 16) | (uint32_t(reply.data[3]) << 24);
        }
        e.parts |= 1 << reply.part;
        if (e.parts == 7) e.key = dreamFilenameKey(e);
      } else if (reply.result == 1) dreamCount = reply.count;
      else dreamMessage = reply.result == 255 ? "Listener file list timeout." :
                                                    "Listener SD busy; refresh after saving.";
    } else if (reply.target == NODE_LISTENER) {
      dreamRecorderKnown = reply.result != 255;
      dreamRecorderReplyMs = millis();
      if (dreamRecorderKnown) {
        dreamRecording = reply.state == 1;
        dreamLatestAvailable = reply.latest != 0;
        dreamMicLeftLevel = reply.data[0] > 100 ? 100 : reply.data[0];
        dreamMicRightLevel = reply.data[1] > 100 ? 100 : reply.data[1];
        if (reply.command == DREAM_STOP && reply.result == 0) dreamResetCatalog();
      }
      dreamMessage = reply.result == 255 ? "Listener not responding." :
                     reply.result == 0 ? (dreamRecording ? "Recording WAV on Listener SD" :
                       (dreamLatestAvailable ? "Choose a recording to play." : "Ready to record")) :
                     "Listener rejected command; check its terminal.";
    } else {
      dreamAudioKnown = reply.result != 255;
      dreamAudioReplyMs = millis();
      if (reply.result == 0) dreamAudioState = reply.state;
      if (reply.result != 0 || reply.command == DREAM_STOP_PLAY ||
          dreamAudioState == 2 || dreamAudioState == 3 || dreamAudioState == 4)
        dreamWaitRequested = false;
      if (dreamAudioKnown && reply.result == 0) {
        dreamVolume = reply.latest;
        dreamPositionSeconds = reply.positionSeconds;
        dreamDurationSeconds = reply.durationSeconds;
        if (reply.command == DREAM_STOP_PLAY) {
          dreamPositionSeconds = dreamDurationSeconds = 0;
        }
      }
      if (reply.result == 255) dreamPlaybackError = "AudioNode not responding.";
      else if (reply.result == 1 &&
               (reply.command == DREAM_PLAY_LATEST || reply.command == DREAM_PLAY_SELECTED))
        dreamPlaybackError = "Wired link unavailable. Check AudioNode and Listener.";
      else if (reply.result != 0 &&
               (reply.command == DREAM_PLAY_LATEST || reply.command == DREAM_PLAY_SELECTED))
        dreamPlaybackError = "AudioNode rejected the playback request.";
      else if (reply.result == 0 && dreamAudioState == 3)
        dreamPlaybackError = "Audio stream failed. AudioNode terminal shows the cause.";
      else if (reply.result == 0 && dreamAudioState == 2)
        dreamPlaybackError = nullptr;
      dreamMessage = reply.result == 255 ? "AudioNode not responding." :
                     reply.result == 1 && (reply.command == DREAM_PLAY_LATEST ||
                         reply.command == DREAM_PLAY_SELECTED) ?
                         "Wired playback unavailable; check both nodes." :
                     reply.result != 0 ? "AudioNode rejected playback command." :
                     dreamAudioState == 3 ? "Audio stream failed; check wired link." :
                     dreamAudioState == 4 ? "Paused" :
                     dreamAudioState == 2 ? "Playing saved WAV" :
                     dreamAudioState == 1 ? "Connecting to saved WAV..." :
                     !dreamRecorderKnown ? "Checking Listener..." :
                     dreamRecording ? "Recording WAV on Listener SD" :
                     dreamLatestAvailable ? "Choose a recording to play." : "Ready to record";
    }
    if (reply.result == 0 && dreamAudioKnown && dreamAudioState == 2)
      dreamMessage = "Playing saved WAV";
  }
  const uint32_t nowMs = millis();
  if (dreamRecorderKnown && nowMs - dreamRecorderReplyMs > 7000) {
    dreamRecorderKnown = false;
    dreamMessage = "Listener status unknown.";
  }
  if (dreamAudioKnown && nowMs - dreamAudioReplyMs > 7000) dreamAudioKnown = false;
  if (dreamWaitCard) {
    const bool waiting = scrDreams && lv_scr_act() == scrDreams &&
                         !dreamPlaybackError && !dreamRecording &&
                         (dreamWaitRequested || dreamAudioState == 1);
    if (waiting) {
      lv_obj_clear_flag(dreamWaitCard, LV_OBJ_FLAG_HIDDEN);
      if (!dreamWaitVisible) {
        dreamWaitVisible = true;
        if (!dreamWaitSince) dreamWaitSince = nowMs;
        dreamWaitLastSecond = 0xFFFFFFFFU;
        dreamWaitLastPhase = 0xFF;
      }
      const uint32_t seconds = (nowMs - dreamWaitSince) / 1000U;
      if (dreamWaitLabel && seconds != dreamWaitLastSecond) {
        dreamWaitLastSecond = seconds;
        lv_label_set_text_fmt(dreamWaitLabel, "PREPARING %02lu:%02lu",
                              (unsigned long)(seconds / 60U),
                              (unsigned long)(seconds % 60U));
        lv_obj_align(dreamWaitLabel, LV_ALIGN_BOTTOM_MID, 0, -1);
      }
      const uint8_t phase = (nowMs / 180U) % 8U;
      if (phase != dreamWaitLastPhase) {
        dreamWaitLastPhase = phase;
        for (uint8_t i = 0; i < 8; ++i) if (dreamWaitDots[i]) {
          const uint8_t distance = (i + 8U - phase) % 8U;
          const bool peak = distance == 0;
          const bool near = distance == 1 || distance == 7;
          lv_obj_set_style_bg_color(dreamWaitDots[i],
              lv_color_hex(peak ? 0xFFD15C : near ? 0x28E4EF : 0x27839F), 0);
          lv_obj_set_size(dreamWaitDots[i], peak ? 21 : near ? 17 : 14,
                          peak ? 21 : near ? 17 : 14);
          lv_obj_align(dreamWaitDots[i], LV_ALIGN_TOP_LEFT,
                       31 + 39 * i, peak ? 4 : near ? 7 : 9);
        }
      }
    } else {
      lv_obj_add_flag(dreamWaitCard, LV_OBJ_FLAG_HIDDEN);
      dreamWaitVisible = false;
      dreamWaitSince = 0;
    }
  }
  if (dreamStatusLabel)
    lv_label_set_text(dreamStatusLabel,
        dreamPlaybackError && !dreamRecording && dreamAudioState != 1 && dreamAudioState != 2 ?
        "Playback unavailable" : dreamMessage);
  if (dreamErrorLabel) lv_label_set_text(dreamErrorLabel,
                                          dreamPlaybackError ? dreamPlaybackError : "");
  if (dreamErrorDot) {
    if (dreamPlaybackError) lv_obj_clear_flag(dreamErrorDot, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(dreamErrorDot, LV_OBJ_FLAG_HIDDEN);
  }
  if (dreamRecordLabel) lv_label_set_text(dreamRecordLabel,
                                          dreamRecording && dreamRecorderKnown ? "STOP & SAVE" : "RECORD");
  if (dreamPlayLabel) lv_label_set_text(dreamPlayLabel,
                                        "PLAY LATEST");
  if (dreamPauseLabel)
    lv_label_set_text(dreamPauseLabel, dreamAudioState == 4 ? "RESUME" : "PAUSE");
  if (dreamProgress && !lv_obj_has_state(dreamProgress, LV_STATE_PRESSED)) {
    if (dreamDurationSeconds) lv_slider_set_value(dreamProgress,
        (int32_t(dreamPositionSeconds) * 100) / dreamDurationSeconds,
        LV_ANIM_OFF);
    else lv_slider_set_value(dreamProgress, 0, LV_ANIM_OFF);
  }
  if (dreamTimeLabel) lv_label_set_text_fmt(dreamTimeLabel, "%02u:%02u / %02u:%02u",
      dreamPositionSeconds / 60, dreamPositionSeconds % 60,
      dreamDurationSeconds / 60, dreamDurationSeconds % 60);
  if (dreamVolumeLabel) lv_label_set_text_fmt(dreamVolumeLabel, "Volume %u/21", dreamVolume);
  if (dreamVolumeSlider && !lv_obj_has_state(dreamVolumeSlider, LV_STATE_PRESSED))
    lv_slider_set_value(dreamVolumeSlider, dreamVolume, LV_ANIM_OFF);
  static uint32_t lastCatalogPaint = 0;
  if (nowMs - lastCatalogPaint >= 500) {
    dreamPaintCatalog();
    lastCatalogPaint = nowMs;
  }
  if (dreamBaseButtons[0]) {
    const bool active = dreamRecording && dreamRecorderKnown;
    const bool pulse = ((nowMs / 350) & 1U) == 0;
    lv_obj_set_style_bg_color(dreamBaseButtons[0],
        lv_color_hex(active ? (pulse ? 0xF33154 : 0xA51F48) : 0xBA2548), 0);
    lv_obj_set_style_border_color(dreamBaseButtons[0],
        lv_color_hex(active ? (pulse ? 0xFFE288 : 0xFF7796) : 0xFF799B), 0);
    lv_obj_set_style_border_width(dreamBaseButtons[0], active ? 7 : 2, 0);
    lv_obj_t *bars[2] = {dreamMicLeftBar, dreamMicRightBar};
    const uint8_t levels[2] = {dreamMicLeftLevel, dreamMicRightLevel};
    for (uint8_t i = 0; i < 2; ++i) if (bars[i]) {
      const uint8_t level = active ? levels[i] : 0;
      const uint32_t tint = level < 15 ? 0x376FE8 :
                            level < 45 ? 0x2FD59F :
                            level < 73 ? 0xF4CB55 : 0xF14E57;
      lv_obj_set_width(bars[i], active ? 2 + (uint16_t(level) * 162) / 100 : 1);
      lv_obj_set_style_bg_color(bars[i], lv_color_hex(tint), 0);
    }
  }
  if (dreamRecordDot) {
    if (dreamRecording && dreamRecorderKnown) lv_obj_clear_flag(dreamRecordDot, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(dreamRecordDot, LV_OBJ_FLAG_HIDDEN);
  }
  if (dreamHomeDot) {
    if (dreamRecording && dreamRecorderKnown) lv_obj_clear_flag(dreamHomeDot, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(dreamHomeDot, LV_OBJ_FLAG_HIDDEN);
  }
  static uint32_t lastListenerPoll = 0;
  static uint32_t lastAudioPoll = 0;
  if ((scrDreams && lv_scr_act() == scrDreams) || dreamRecording) {
    if (nowMs - lastListenerPoll >= (dreamRecording ? 500U : 3000U)) {
      dreamQueue(NODE_LISTENER, DREAM_STATUS);
      lastListenerPoll = nowMs;
    }
  }
  if (scrDreams && lv_scr_act() == scrDreams && nowMs - lastAudioPoll >= 3500) {
    dreamQueue(NODE_AUDIO, DREAM_PLAY_STATUS);
    lastAudioPoll = nowMs;
  }
  dreamCatalogTick(nowMs);
}
