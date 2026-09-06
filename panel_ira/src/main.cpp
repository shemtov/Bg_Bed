// ============================================================
// PANEL IRA - TEST 089 - MATCHED PAIR + SEIKO NIGHT SCREENSAVER
// ============================================================
//
// Board: Waveshare ESP32-S3-Touch-LCD-7B, 1024 x 600
//
// WHAT CHANGED IN TEST 083:
//   Fixes TEST082 build failure: the generated setup Serial.println()
//   line lost its closing quote/parenthesis during text replacement.
//   TEST083 uses safe whole-line replacements and preserves valid C++.
//   Ira still uses the full 7-inch UI and receives clock/date from
//   Shemi over onboard RS485 (RX GPIO15, TX GPIO16, 115200 8N1).
//
// NOT YET COMPILED OR RUN ON HARDWARE.
//
//
// WHAT CHANGED vs TEST 055
//   THE LONG PRESS NEVER FIRED, and the approach was the fault. I
//   leaned on LV_EVENT_LONG_PRESSED, which only fires if the input
//   driver's long_press_time is what I set it to - a field inside
//   Waveshare's port that I set blind at boot and cannot verify
//   from here. Depending on it was the mistake.
//
//   THE PANEL TIMES THE HOLD ITSELF NOW. millis() on press, millis()
//   on release, subtract. Under 2 seconds flips the power; 2 seconds
//   or more opens the detail screen. No LVGL setting involved, so
//   there is nothing left to be silently wrong.
//
//   IT ACTS ON RELEASE, not at the 2 second mark. A press that
//   triggers while your finger is still down needs a timer running
//   against every tile; measuring on release is exact, needs
//   nothing running, and cannot fire twice.
//
//   AND IT PRINTS THE MEASURED HOLD - "[tile] AC held 2340 ms" - so
//   if 2 seconds turns out to be the wrong threshold for you, we
//   will be reading real numbers instead of guessing again.
//
//   PRESS_LOST is handled too: slide off the tile and nothing
//   happens, which is the standard way out of a press you did not
//   mean.
//
// WHAT CHANGED IN TEST 054 vs TEST 053
//   Alarm screen:
//     - "Hour" and "Minute" titles above their plus/minus groups,
//       the same treatment the Clock tab got in TEST 050
//     - the whole setter is CENTRED, horizontally and vertically -
//       it sat 110 px from the top before, which put it in the
//       upper third rather than the middle
//
//   THE FLEX ROW IS GONE, and it had to be: a flex row lays its
//   children out in a line and cannot put a heading over one of its
//   own columns. Absolute positions instead, exactly as the Clock
//   tab needed for the same reason.
//
//   Centred means centred on the SCREEN, so the numbers sit level
//   with the middle of the glass, not with the space left over
//   under the switches.
//
// WHAT CHANGED IN TEST 052 vs TEST 051
//   HANDS: thinner, SQUARE ended, and hollow to the tip.
//     - lv_line rounded caps turned OFF, so the ends are flat
//       rectangles instead of domes
//     - hour 20 px outer over a 12 px dark core, minute 12 over 6.
//       Was 30 over 12 and 20 over 8 - that is what made them heavy
//     - THE TRIANGLE TIPS ARE GONE. They were the weight and the
//       roundness both, and a hollow rectangle cannot end in a
//       solid point without stopping being hollow. The core now
//       runs almost to the end, so the tip reads open.
//
//   SECOND HAND IS RED, keeping the green lume dot.
//
//   NINE GETS ITS MARKER BACK - a lying pill, matching three.
//
//   THE DATE WINDOW MOVED to the right, midway between the centre
//   and the three o'clock pill, which is where the freed space is
//   now that nine is a marker again.
//
//   TEMPERATURE RIGHT, HUMIDITY LEFT was already done in TEST 050 -
//   which never ran, because it did not compile. Nothing to change;
//   it arrives with this build.
//
// WHAT CHANGED IN TEST 051 vs TEST 050
//   TEST 050 would not compile: refreshStations() was placed with
//   the radio widget pointers, above kStation and radioStation
//   which it reads. It sits just before refreshRadio now, after
//   both. Third member of the use-before-declaration family
//   tonight - so the delivery audit no longer takes a hand-made
//   list of names, it derives every file-scope variable and
//   function from the file itself and checks all of them.
//
// WHAT CHANGED IN TEST 050 vs TEST 049
//   Five prompts of remarks, all in one test.
//
//   EVERYWHERE
//     - back button is a SQUARE, 56 by 56, teal. "Square" was said
//       twice and clearly; the later garbled sentence asked for a
//       nicer colour, so amber went and teal came.
//     - the flip switch: the knob travels to the ENDS of a wider
//       pill, and when OFF the word disappears - knob position
//       says it alone. ON still says ON.
//
//   RADIO
//     - now-playing name at the top: gone
//     - title is Hebrew, drawn with font_hebrew_28
//     - station buttons are DULL until chosen; the chosen one gets
//       its full colour plus a white frame - much stronger than
//       the rest, as asked
//     - the audionode note under the stations: gone
//     - MAX at the right end of the volume slider
//
//   MASSAGE
//     - the pattern-name status at the top right (the "something I
//       do not know what sign it is"): gone. It was lblMassMode.
//     - the Massage caption beside the switch: gone
//     - the lying figure dropped well below the sliders
//
//   SETTINGS, CLOCK
//     - a title above every plus/minus group - Hour, Minute, Day,
//       Month, Year - bigger and brighter than the old grey
//     - the 12 hour switch moved to the bottom, caption under it
//
//   SETTINGS, DISPLAY
//     - "Response speed" now says what it is for: the reaction of
//       the AUTO BRIGHTNESS
//     - the switch caption sits UNDER the switch, bigger
//
//   SCREENSAVER
//     - hands rebuilt as SKELETON hands: two long bright rails with
//       a dark gap between them - an outer line with a face-dark
//       line over its middle - and a FILLED triangle tip, wider
//       than before
//     - the twelve pip is filled solid
//     - three and six are pills now, like nine was - three lying,
//       six standing, both pointing at the centre
//     - the date window moved to the LEFT, taking nine's place
//     - temperature RIGHT, humidity LEFT - swapped as asked - and
//       both drawn a step larger through transform zoom, because 48
//       is the largest font this build carries
//
// WHAT CHANGED IN TEST 049 vs TEST 048
//   TEST 048 would not compile. The new tile handlers - evACShort,
//   evFanShort - sit early in the file with the other tile code,
//   but fanSpeed, acTemp, refreshFan and refreshAC are all defined
//   hundreds of lines below them. The same use-before-declaration
//   mistake as TEST 032, caught by the compiler instead of the
//   audit because the audit only watched variables, not functions.
//
//   fanSpeed and acTemp moved up beside fanOn and acPower, where
//   they should have lived all along. refreshFan and refreshAC are
//   forward declared with the other forwards. The audit now checks
//   function calls too.
//
// WHAT CHANGED IN TEST 048 vs TEST 047
//   One line. Where the Seiko carries its red text below the
//   centre - MAGNETIC RESISTANT on the photograph - this dial says
//
//       Ira & Shemi Bengiat
//
//   in the same red, same place, small like on the watch. Spelled
//   the way Shemi spelled it. If it should be Hebrew instead, that
//   is a pre-reversed string away - say so.
//
// WHAT CHANGED IN TEST 047 vs TEST 046
//   The watch face redone against the photograph Shemi sent - a
//   Seiko Prospex Tuna. What was taken from it:
//
//   HANDS. The hour hand is BROAD - a wide shaft with a pointed
//   tip. The minute hand is longer and slimmer, same pointed tip.
//   The second hand is a thin line with a LUME DOT near the tip and
//   a counterweight tail past the centre - that dot is the most
//   recognisable thing on the watch and it moves with the hand,
//   realigned every second.
//
//   A DATE WINDOW AT THREE, where the Tuna has its date: a small
//   framed rectangle showing the day of the month, updated when the
//   day changes. The dot at three gave its place to it.
//
//   A HORIZONTAL PILL AT NINE, as on the photograph, instead of a
//   round dot.
//
//   The pips at twelve and six stay as Shemi specified them - the
//   six o'clock pip is his instruction and it stands even though
//   the Tuna itself carries a dot there.
//
//   Dots remain at 1, 2, 4, 5, 7, 8, 10, 11.
//
// WHAT CHANGED IN TEST 046 vs TEST 045
//   TWO FACTS BEFORE THE FEATURES, checked in the repo rather than
//   assumed:
//     - The AC detail screen here was a stub. The old panel's full
//       one - modes, fan speeds, swing, the honest "shows what was
//       last commanded" note - is now ported: Cool/Heat/Dry/Fan/
//       Auto, Low/Med/High/Auto, Swing on/off, temp 16-30, room
//       reading beside the target.
//     - There is NO fan screen in the old panel to copy. buildFan
//       does not exist in that file; the fan screen here was
//       written fresh for this board in TEST 017. It stays as it
//       is. If TEST 082 had one, it never reached the repo.
//
//   SHORT PRESS AND LONG PRESS ON THE HOME TILES, for fan and AC:
//     - a short press flips the power and stays on the home screen
//     - holding for 2 seconds opens the detail screen
//   The touch driver's long press threshold is set to 2000 ms at
//   boot - the LVGL default is 400, which is a twitch, not a hold.
//   Massage and radio tiles keep their single press - Shemi asked
//   for the two devices you mostly just switch on and off.
//
//   The AC power on the tile and the one on the screen go through
//   ONE function, so the two can never disagree about what "on"
//   means.
//
// WHAT CHANGED IN TEST 045 vs TEST 044
//   Watch face again, per Shemi:
//     - the dots doubled: 40 px core, about 6 mm, glow scaled with
//       them
//     - the arrowheads cut down - they were slabs; a dive watch
//       hand is mostly shaft with a modest tip
//     - a second pip at SIX, mirrored: flat edge at the bottom,
//       apex pointing up at the centre. The dot at six gave its
//       place to it, the same way twelve's did.
//     - temperature TOP LEFT, humidity TOP RIGHT, both in the same
//       green as the face, both whole numbers - no decimal
//     - the temperature was already in the largest font this build
//       carries (48). Dropping the decimal makes the digits the
//       whole string, which reads larger without a new font.
//
//   A photograph of the watch to imitate is on its way from Shemi;
//   this pass covers what was already specified in words.
//
// WHAT CHANGED IN TEST 044 vs TEST 043
//   Massage screen, reworked to what you asked:
//
//   THE TWELVE COLOURS ARE GONE. Tiles are back to the quiet dark
//   grey, and the chosen one lights green - the TEST 081 behaviour.
//   kPatCol was deleted outright, not left as dead weight.
//
//   THE TITLE IS HEBREW NOW - the word everyone actually says,
//   with a geresh, stored pre-reversed like every Hebrew string
//   here. It is drawn with font_hebrew_28, which also makes it a
//   little larger than the 26 point Latin titles. 28 is the only
//   Hebrew size compiled into this build; noticeably bigger than
//   that means generating a new font, which is a separate job -
//   say so if you want it.
//
//   THE ZONE NAMES UNDER THE SLIDERS ARE GONE. In their place a
//   small figure lies under the four sliders, head at the left
//   under the first, feet at the right under the fourth - so the
//   mapping is drawn instead of written. Head, torso, legs, from
//   LVGL primitives, nothing clickable.
//
//   THE RED KIBUI BUTTON IS GONE. One flip switch, the same one the
//   radio has, and nothing else. Its handler went with it.
//
//   THE TIMER SLIDER ENDS AT THE 60 BUTTON, same rule as the radio:
//   buttons at 20, 162 and 304, each 130 wide, row ends at 434, so
//   the slider is 414 wide from 20.
//
//   THE "press the running one again" HINTS ARE GONE, from both
//   screens. The behaviour stays; the label explaining it does not.
//
// WHAT CHANGED IN TEST 043 vs TEST 042
//   THE TOGGLE IS A FLIP SWITCH NOW. The ON/OFF button told you the
//   state in words; this one shows it in position, like the switch
//   on a wall. A pill shaped track with a round knob: knob LEFT and
//   grey track when off, knob RIGHT and green track when on, the
//   word sitting on the empty side. Still a tap and only a tap -
//   the knob does not drag, pressing anywhere flips it.
//
//   All six toggles get this - radio, massage, alarm armed, every
//   day, 12 hour, auto brightness - because two different switch
//   styles on one panel is one too many. The handlers, again,
//   unchanged: same LV_STATE_CHECKED underneath.
//
//   RADIO SCREEN LAYOUT, as asked:
//     - the sleep slider now ends exactly where the 60 minute
//       button ends. Buttons sit at 24, 164 and 304, each 128 wide,
//       so the row ends at 432 - the slider is 408 wide from 24.
//     - the radio flip switch moved to the right side of the
//       screen, clear of the timer row.
//
//   AND ONE RULE INTO THE PROJECT KNOWLEDGE, not this file: when
//   the radio starts, audionode fades the volume from silence up to
//   the set level over about 1.5 seconds. Never straight in at full
//   set volume. The panel only sends the play command; the ramp
//   belongs to the node that owns the DAC. Recorded so it is built
//   that way when audionode grows its bus interface.
//
// WHAT CHANGED IN TEST 042 vs TEST 041
//   TEST 041 would not compile. buildSettings() had two locals both
//   called sb - the "Set time and date" button in the Clock tab, and
//   the "Screensaver brightness" caption I added to the Display tab
//   in TEST 039. Both tabs are built inside the SAME function, so
//   they share one scope. The second one is sbCap now.
//
//   I checked every other builder the same way afterwards. The two
//   apparent duplicates in buildMassage and buildRadio are each a
//   local b inside a separate for loop, which are separate scopes -
//   those are fine.
//
// WHAT CHANGED IN TEST 041 vs TEST 040
//   Massage screen:
//     - a timer slider, 5 to 60 minutes, beside the three buttons,
//       working the same way the radio one does
//     - a large ON / OFF toggle on the right
//     - a different colour for each of the twelve patterns
//     - the numbers above the zone sliders removed
//     - Hebrew zone names under them instead of 1 2 3 4
//   And the back arrow is amber now, on every screen.
//
//   THE PATTERN COLOURS ARE NOT FROM TEST 081, and I checked before
//   choosing. That version built all twelve tiles with no bg_color
//   at all and only lit the selected one, so there was nothing to
//   copy. These twelve are mine, spread around the wheel so no two
//   neighbours in the grid are close.
//
//   THE ZONE NAMES ARE FROM TEST 075 and are stored back to front,
//   like every other Hebrew string here - LVGL has no bidirectional
//   text. Do not reorder them in an editor.
//
//   THE MASSAGE TIMER ACTUALLY RUNS NOW. It was a number that got
//   printed and nothing else. It counts down, shows the minutes
//   left, and switches the massage off at zero - rollover safe, the
//   same way the radio timer is.
//
//   IT IS NOT STORED. The EEPROM record is a full 16 byte page and
//   the next byte would push it over into a second page, which the
//   AT24C32 does not write through - it wraps. Not worth restructing
//   the record for a value you set per session anyway.
//
// WHAT CHANGED IN TEST 040 vs TEST 039
//   EVERY lv_switch IS GONE. There were five - auto brightness, 12
//   hour, alarm armed, alarm repeat - and they are small, the state
//   is hard to read across a room, and they invite a drag.
//
//   Replaced by a large button that carries its own state: 150 by
//   56, green and reading ON when set, dark grey and reading OFF
//   when not. LV_OBJ_FLAG_CHECKABLE makes a press toggle it, so it
//   is a tap and only a tap - there is nothing to slide.
//
//   The handlers did not have to change. They already read
//   LV_STATE_CHECKED, and a checkable button carries exactly the
//   same state a switch did.
//
//   RADIO SCREEN: the Stop button at the top right is gone, and the
//   sleep row's Off button is now a RADIO ON / OFF toggle in its
//   place. Switching the radio off also cancels the sleep timer -
//   there is nothing left for it to switch off.
//
//   Turning it on with no station chosen selects the first one,
//   because a power switch that appears to do nothing is worse than
//   one that makes an obvious choice.
//
//   TO CANCEL THE TIMER WITHOUT STOPPING THE RADIO, press the
//   active preset again. It is written on the screen, because a
//   hidden gesture is not a feature.
//
//   THE SLEEP SLIDER NOW STOPS AT 60 MINUTES, matching the three
//   buttons. It ran to 120 before, which put most of its travel
//   past anything the buttons offered.
//
// WHAT CHANGED IN TEST 039 vs TEST 038
//   TWO SLIDERS ON THE DISPLAY TAB, and a correction first: in TEST
//   081 the screensaver DELAY was a dropdown, ddSaver, with four
//   options. The slider there, range 1 to 60, was "Night backlight
//   (screensaver)" - the screensaver's brightness. So you were
//   remembering a real slider, but it controlled the other thing.
//
//   Both are sliders now:
//     - delay: Never, 15 s, 30 s, 1, 2, 5, 10, 20, 30 minutes
//     - brightness: 1 to 60 percent, replacing the hard coded 8
//
//   THE BRIGHTNESS ONE WAS A GENUINE GAP. SAV_BRIGHT was a constant
//   in the source, so the one number that decides how much light
//   sits next to a sleeping head could only be changed by
//   rebuilding.
//
//   NOTHING STORED IS LOST, and that took some care. Byte 12 held a
//   delay index into a five entry table; the new table has nine, so
//   the same number now means something different. Rather than bump
//   the record version and wipe every setting you have tuned, the
//   new delay goes in byte 14 as index PLUS ONE - so zero means
//   "never written", and an older record falls back to byte 12 and
//   is translated. The brightness rides in the high nibble of byte
//   12, which was always zero before and reads as "use the default".
//
//   The slider moves in nine steps rather than freely. A screensaver
//   delay of 7 minutes 23 seconds is not a thing anyone wants, and
//   a free slider makes hitting exactly 5 minutes a fiddle.
//
// WHAT CHANGED IN TEST 038 vs TEST 037
//   THE MISSING TEMPERATURE WAS NOT THE SENSOR. lv_conf.h has
//   LV_SPRINTF_USE_FLOAT 0, so LVGL's own printf DISCARDS %f
//   silently - which is why the screen showed a degree sign and a
//   percent sign with nothing in front of them.
//
//   It was wrong in four places, not one: the screensaver
//   temperature and humidity, the lux reading in Settings, and the
//   room temperature on the AC screen. All four now format with the
//   C library snprintf and hand LVGL a finished string. Every one
//   of them had been printing an empty number since the day it was
//   written.
//
//   NUMERALS REPLACED BY DOTS. Twenty pixels across, which on a 7
//   inch 1024 wide panel is about 3 mm - the screen is roughly 154
//   mm wide, so 6.65 pixels per mm. Glowing, in three concentric
//   circles: wide and faint, medium, then the bright core.
//
//   The separate ring of small tick marks is gone with them. Two
//   rings of markers was one too many once the numerals left.
//
//   THE PIP AT TWELVE IS INVERTED - flat edge at the top, point
//   downwards, as asked - and the dot at twelve is dropped so the
//   pip stands alone there, which is what a dive bezel does.
//
//   THE ARROWHEADS ARE TRIANGLES NOW, not a thick stub of line.
//   Three points, closed, drawn with a wide stroke: LVGL cannot
//   fill a polygon, but at this size the strokes meet in the middle
//   and it reads as solid. The corners are recomputed as the hands
//   turn, so the triangle always points along the hand.
//
// WHAT CHANGED IN TEST 037 vs TEST 036
//   THE STATION LIST IS THE REAL ONE, recovered from TEST 069 in
//   the repo rather than invented: Kan 88, Kan Gimel, Galgalatz,
//   Eco 99, Reshet Bet, Kol HaMusica, Groove Salad, Drone Zone.
//   The colours are the ones already chosen there - Kan's own brand
//   colours for the first two - so every button is a different
//   colour, as asked. My eight placeholders are gone.
//
//   The old note said Kan 88 and Kan Gimel carry real stream URLs,
//   pulled from the data-player-hls-src attribute on kan.org.il.
//   Those URLs are NOT in this panel and this panel plays nothing;
//   they belong on audionode when the bus exists.
//
//   THE LABELS ARE 26 POINT, up from 16, which is the largest this
//   lv_conf.h builds for Latin text.
//
//   HEBREW: NOT DONE, AND THIS IS THE REASON. There is no
//   bidirectional engine, so Hebrew has to be stored back to front
//   by hand. "Kan 88" and "Eco 99" mix Hebrew with digits, and the
//   digits must NOT be reversed while the letters must - a rule I
//   would be applying blind, with no way to check it here. Say the
//   word and I will write them out, but I am not guessing at the
//   spelling of your own radio stations.
//
//   A RUNNING SLEEP TIMER IS NOW VISIBLE FROM ACROSS THE ROOM: a
//   green badge with the minutes left, blinking once a second, on
//   the radio screen AND on the radio tile of the home screen. The
//   question "am I on a timer" gets answered without opening
//   anything.
//
//   IT BLINKS BY TOGGLING OPACITY, not by rebuilding. On an RGB
//   panel anything that redraws a large area every second shows as
//   a twitch; a 60 pixel badge changing opacity does not.
//
// WHAT CHANGED IN TEST 036 vs TEST 035
//   The screensaver reworked to what you asked for:
//     - hour numerals glowing green
//     - a dive watch pip at twelve
//     - broad bright arrowheads on the hour and minute hands
//     - temperature large on the right, one decimal
//     - humidity on the left, whole percent
//     - the digital time at the bottom removed
//     - a small red alarm clock at bottom right when one is armed
//
//   MONTSERRAT 48 ENABLED in lib/lv_conf.h. 26 was the largest
//   built, and 26 is not "much bigger". Second line touched in that
//   vendor file, after LV_MEM_SIZE.
//
//   LVGL HAS NO GLOW, so it is faked: a dark green copy of each
//   numeral offset two pixels in four directions, with the bright
//   one on top. On a dark face that reads as a halo. It costs five
//   labels per numeral, sixty in total, which is why the 128 KB
//   heap mattered.
//
//   THE ARROWHEADS ARE SEPARATE LINES, not wider hands. A hand
//   drawn at double width for its whole length is a slab; a short
//   double width segment at the tip reads as an arrow. Two objects
//   per hand instead of one.
//
//   TEMPERATURE IS CLAMPED TO ONE DECIMAL AND TO A SANE RANGE. The
//   SHT3x is checksummed, but a checksum only proves the bytes
//   arrived intact - so anything outside -10..60 draws as "--"
//   rather than as a confident wrong number beside a sleeping head.
//
//   THE ALARM CLOCK IS THE SAME 160x160 PHOTOGRAPH, drawn at 37
//   percent with lv_img_set_zoom. No second asset, no extra flash.
//
// WHAT CHANGED IN TEST 035 vs TEST 034
//   A proper sleep timer on the radio screen: 10, 30 and 60 minute
//   buttons, a slider for anything else up to two hours, an Off
//   button, and the time remaining counting down in minutes.
//   The old fixed "Sleep 30" button is gone.
//
//   THE COUNTDOWN IS ROLLOVER SAFE. millis() wraps after about 49
//   days, and a panel meant to sit on a headboard for years WILL
//   cross that. Comparing millis() >= end fails exactly once when
//   it wraps - the radio would stop dead in the middle of a night,
//   months from now, for no visible reason. Subtracting first and
//   testing the sign is correct across the wrap.
//
//   SETTING IT WHILE THE RADIO IS OFF DOES NOTHING. There is
//   nothing to stop, and a timer counting down against silence only
//   creates a surprise later.
//
//   STOPPING THE RADIO CANCELS THE TIMER, so it cannot fire against
//   a station you started afterwards and expected to keep playing.
//
//   THE CHOSEN LENGTH IS REMEMBERED in byte 13 of the EEPROM record,
//   which was padding. An older record reads 0 and is corrected to
//   30 minutes on load. No version bump.
//
//   AND IT STILL COMMANDS NOTHING. audionode holds the audio and the
//   bus to reach it does not exist. The timer runs, the state
//   changes, the screen says so. That is the honest extent of it.
//
// WHAT CHANGED IN TEST 034 vs TEST 033
//   A second switch on the alarm screen: every day, or once.
//
//   ONCE DISARMS WHEN IT STARTS RINGING, not when you stop it. That
//   way it cannot fire again tomorrow no matter what you do with it
//   this morning - stop it, snooze it, ignore it until it gives up,
//   or walk out of the room.
//
//   SNOOZE STILL WORKS AFTER IT DISARMS. Snooze runs off its own
//   target rather than the armed flag, so a once-only alarm can
//   still be pushed nine minutes and will still come back this
//   morning. It just will not be there tomorrow.
//
//   STORED AS "ONCE", NOT AS "REPEAT". Bit 3 of the flags byte was
//   always zero in existing records, and every alarm until now
//   repeated daily. Storing the flag the other way round would make
//   every saved alarm silently become once-only on the first boot
//   after flashing. So 0 means every day, which is what those
//   records meant. No version bump, nothing lost.
//
// WHAT CHANGED IN TEST 033 vs TEST 032
//   TEST 032 would not compile. lastTouchMs was declared inside the
//   screensaver block, which sits near the end of the file, while
//   evAnyPress and evSaverAfter both use it hundreds of lines
//   earlier. When I hoisted saverAfterMs and kSaverOpts above the
//   EEPROM code I moved two of the five and left three behind.
//   saverOn and blBeforeSaver had the same problem waiting one
//   error further on. All five now sit together.
//
// WHAT CHANGED IN TEST 032 vs TEST 031
//   The boot photograph is back. include/boot_photo.h has been
//   sitting in the repo unused since the port - two 800x480
//   baseline JPEGs, yours and Ira's, with PANEL_ID choosing.
//
//   THE OLD DRAW ROUTE DOES NOT EXIST HERE. TEST 081 handed each
//   decoded block straight to gfx->draw16bitRGBBitmap from
//   Arduino_GFX, and this board does not use that library at all -
//   it is esp_lcd plus LVGL. Same decoder, different destination:
//   JPEGDEC writes into a PSRAM buffer, and that buffer is shown as
//   an ordinary LVGL image.
//
//   IT DECODES INTO PSRAM, NOT INTO LVGL'S POOL. 800x480 at two
//   bytes is 768,000 - six times the whole LVGL heap. PSRAM has 8 MB
//   and the buffer is handed back the moment the photo leaves the
//   screen, so nothing stays resident.
//
//   THE JPEG IS COPIED OUT OF FLASH FIRST. JPEGDEC needs a writable
//   pointer and PROGMEM is not one. Same reason TEST 081 did it.
//
//   RGB565_LITTLE_ENDIAN, to match LV_COLOR_16_SWAP 0. Getting this
//   wrong gives a recognisable photograph in wrong colours.
//
//   IF ANYTHING FAILS IT JUST GOES TO THE HOME SCREEN. No allocation
//   for the buffer, no decoder, a corrupt file - the panel says so
//   on serial and carries on. A boot screen is not worth a boot
//   loop.
//
//   PANEL_ID 1 HERE. Setting it to 2 builds Ira's panel from this
//   same file and shows her photograph instead, which is how you
//   can tell at a glance which board you just flashed.
//
// WHAT CHANGED IN TEST 031 vs TEST 030
//   THE DRAWN CLOCK IS GONE. The tile carries your photograph now -
//   the red one when armed, the black one when off. Cut from the
//   picture you sent, 160x160, in include/alarm_img.h.
//
//   NO ALPHA CHANNEL. The background is baked to TILE_BG_OFF
//   instead. Alpha costs 50 percent more flash and a blend on every
//   redraw, and these two images only ever sit on that one tile
//   colour. If the tile colour ever changes, the images have to be
//   regenerated - that is the price, and it is the right trade
//   here.
//
//   RGB565 LITTLE ENDIAN, LV_IMG_CF_TRUE_COLOR - matched to
//   tile_img.h rather than assumed. lv_conf.h has LV_COLOR_DEPTH 16
//   and LV_COLOR_16_SWAP 0; getting the byte order wrong gives a
//   recognisable picture in wrong colours, which is a confusing
//   failure to chase.
//
//   THE SCREENSAVER IS BACK, ported from buildAnalog() in TEST 081
//   and rescaled from 800x480 to 1024x600: round face, twelve
//   ticks, the numbers 1 to 12, hour, minute and second hands, and
//   below them the time in figures with temperature and humidity.
//
//   THE SECOND HAND MOVES ONCE A SECOND, THE REST ONLY WHEN THEY
//   CHANGE. Redrawing all three every second on an RGB panel is
//   what made the old screen twitch.
//
//   IT DIMS. The screensaver has its own brightness, well under the
//   normal one, because it is the thing that sits lit next to a
//   sleeping head. Touch anywhere returns to the home screen and
//   restores the brightness.
//
//   THE IDLE DELAY IS A SETTING, in the Display tab, with Never as
//   an option so it can be switched off.
//
// WHAT CHANGED IN TEST 030 vs TEST 029
//   The alarm clock was drawn and you could not see it. Most likely
//   because I chose 0x1A1A1A for the OFF body against a tile
//   background of almost the same darkness - black on black.
//
//   OFF IS NOW GREY, NOT BLACK. 0x565A66 against the tile, with a
//   lighter rim around the body and the bells. Armed is still red.
//   The two states are unmistakable and BOTH are visible.
//
//   THE SET TIME IS PRINTED ON THE TILE, under the clock, so the
//   home screen answers "when is it set for" without opening
//   anything - and so the tile is obviously not empty.
//
//   THE BELLS SHAKE WHILE IT RINGS. Four pixels left and right, in
//   step with the motor pulse. This is the thing a drawn icon can
//   do that a photograph cannot, and it was the argument for
//   drawing it.
//
//   AND IT SAYS SO ON SERIAL: "[alarm] icon built" at boot, with
//   the state. If that line does not appear, the icon is not being
//   constructed and the problem is somewhere else entirely - no
//   more guessing from here about whether it exists.
//
// WHAT CHANGED IN TEST 029 vs TEST 028
//   Still said Thursday on a Friday. The arithmetic was already
//   right - Zeller returns h=6 for 4 September 2026 and the mapping
//   gives kDow[6], "Fri". The wrong day was not being calculated.
//   IT WAS BEING READ.
//
//   now.dow came from DS3231 register 0x03, and that register is
//   only ever written when the clock is set. The 5 that was put
//   there before the TEST 017 fix has been sitting in the chip ever
//   since, and every boot faithfully read it back.
//
//   THE REGISTER IS NO LONGER THE SOURCE OF TRUTH. Day of week is
//   derived entirely from the date, so there is no reason to trust
//   a byte that can be stale, wrong, or set by some other device
//   sharing the chip. It is computed on every read now.
//
//   This kills the whole family of failures rather than the one
//   instance, and it corrects the display without you setting the
//   clock again.
//
//   The register is still WRITTEN when the clock is set, so anything
//   else that ever reads this chip sees a sensible value.
//
// WHAT CHANGED IN TEST 028 vs TEST 027
//   TEST 027 runs. A 12 hour option added, as a switch in the Clock
//   tab, stored with everything else.
//
//   THE CHIP STAYS ON 24 HOUR NO MATTER WHAT THE SCREEN SHOWS. The
//   DS3231 hour register carries a 12/24 mode bit, and writing 12
//   hour values into it means converting in both directions around
//   midnight and noon - the two times that break every naive
//   conversion. One representation in the hardware, converted only
//   where it is drawn.
//
//   IT REUSES A SPARE BIT rather than bumping the EEPROM version.
//   Bit 2 of the flags byte was previously always zero, so an
//   existing record reads as 24 hour, which is what it was. No
//   version change, no settings lost.
//
//   MIDNIGHT IS 12 AM AND NOON IS 12 PM. Written out because the
//   obvious formula, h % 12, turns both into 0 and prints "0:15 AM".
//
// WHAT CHANGED IN TEST 027 vs TEST 026
//   THE BOOT LOOP IS SOLVED, and the decoded backtrace named it:
//       lv_obj_add_style   lv_obj_style.c:96
//       lv_btn_create      lv_btn.c:53
//       setBtn             main.cpp:1830
//       navBack            main.cpp:1954
//
//   Line 96 is  obj->styles[j] = obj->styles[j - 1];  - a write
//   through the pointer that lv_mem_realloc had just returned on
//   the line above. The register dump closes it: EXCVADDR was 0x38,
//   which is index 7 times the 8 byte style record. styles was
//   NULL. THE REALLOC FAILED. LVGL had run out of memory.
//
//   48 KB was not enough for seven screens. buildHome and
//   buildSettings fit; it died part way into the fourth.
//
//   TWO CHANGES, AND BOTH ARE NEEDED.
//
//   1. LV_MEM_SIZE 48K -> 128K in lib/lv_conf.h. That file belongs
//      to Waveshare and this is the one line touched in it. It has
//      to be there: LV_MEM_SIZE is defined outright, so a -D in
//      build_flags loses to the header.
//
//   2. SCREENS ARE BUILT ON DEMAND. Building all seven at boot to
//      show one was wasteful even when it fit. Each is built the
//      first time it is opened and kept afterwards, so the panel
//      only pays for what has been used.
//
//   AND IT REPORTS ITS OWN MEMORY NOW. lv_mem_monitor after every
//   screen build, plus an m command on serial. Guessing at how much
//   room is left is what put us here.
//
// WHAT CHANGED IN TEST 026 vs TEST 025
//   Step prints during setup, kept - they cost nothing and they
//   name the failing stage instantly.
//
// WHAT CHANGED IN TEST 025 vs TEST 024
//   TEST 025 got the panic message:
//       Guru Meditation Error: Core 1 panic'ed (LoadProhibited)
//       EXCVADDR: 0x00000038
//   after "[cfg] empty - using defaults".
//
//   LoadProhibited with EXCVADDR 0x38 is a READ THROUGH A NULL
//   POINTER - something was NULL and a field 56 bytes into it was
//   read. Not memory exhaustion, which would assert in lv_mem, and
//   not the EEPROM, which had already printed its result.
//
//   Everything up to that point succeeded: panel ok, all four
//   device handles ok, the EEPROM read and reported itself empty.
//   The crash is in the seven build functions that run next, and
//   none of them gets far enough to print.
//
//   THIS VERSION ONLY PRINTS. A line before and after each build
//   step, so the next boot names the function instead of me
//   guessing which of seven it is. No behaviour changes.
//
// WHAT CHANGED IN TEST 025 vs TEST 024
//   Nothing in this file - only the exception decoder in
//   platformio.ini.
//
// WHAT CHANGED IN TEST 024 vs TEST 023
//   NOTHING IN THIS FILE. The only change in TEST 025 is one line
//   in platformio.ini - the exception decoder - so that the boot
//   loop prints line numbers instead of raw addresses.
//
//   The board compiles and then reboots in a loop. I am not
//   rewriting anything until the panic message says where it dies.
//   The two candidates I would otherwise be guessing between are
//   the EEPROM read at boot (new in TEST 022) and LVGL running out
//   of heap now that seven screens are built and kept alive at once
//   (LV_MEM_SIZE is 48 KB). Guessing between them by rewriting is
//   exactly how the last two rounds went wrong.
//
// WHAT CHANGED IN TEST 024 vs TEST 023
//   TEST 022 and 023 would not compile. One line, mine.
//
//   eeWaitReady() passed `handle` to i2c_master_probe. `handle` is
//   a DEV_I2C_Port, which is a STRUCT - the bus pointer is the .bus
//   member inside it. probeAddr() three hundred lines up gets this
//   right and has done since TEST 010; I wrote the new function
//   from memory instead of copying the one already working in the
//   same file. Known-good beats deduced, and I did not follow it.
//
//   Also cleaned: three warnings about LV_PART_ITEMS |
//   LV_STATE_CHECKED. LVGL declares the two as separate anonymous
//   enums and this toolchain deprecates a bitwise operation between
//   them, so both are cast to lv_style_selector_t first, which is
//   the type the function wants anyway. Warnings only - they were
//   never what stopped the build.
//
// WHAT CHANGED IN TEST 023 vs TEST 022
//   The alarm tile is a drawn alarm clock now - two bells, a round
//   body, a white face, hands and feet. RED WHEN ARMED, BLACK WHEN
//   OFF, exactly as asked.
//
//   IT IS DRAWN, NOT AN IMAGE, and that turns out to be the better
//   answer anyway. tile_img.h alone is nearly 4 MB; this costs a
//   few dozen bytes. It recolours between red and black by changing
//   two style properties instead of carrying two bitmaps. It scales
//   to any size without re-exporting. And when the ring animation
//   goes in later, the bells can shake - a photograph cannot.
//
//   THE PARTS ARE NOT CLICKABLE. lv_obj_create makes a clickable
//   object by default, so nine shapes stacked on the tile would
//   swallow every touch and the tile would stop responding. Each
//   part has CLICKABLE cleared, which makes it invisible to hit
//   testing and lets the press land on the tile underneath.
//
//   THE HANDS POINT AT THE ALARM TIME. Not decoration - the tile
//   shows when it is set for, so the home screen answers the
//   question without opening anything. Redrawn only when the time
//   changes, never on a timer.
//
// WHAT CHANGED IN TEST 022 vs TEST 021
//   Every tuned value is now kept in the AT24C32 at 0x57 - the 4 KB
//   EEPROM on the RTC module, found in the very first scan and
//   unused until now. Stored: auto brightness on/off, darkest,
//   brightest, response speed, haptic strength, haptic length,
//   alarm time and whether the alarm is armed.
//
//   IT IS NOT FLASH AND IT DOES NOT CARE ABOUT REFLASHING. Loading
//   new firmware wipes the ESP32, not the chip on the clock module.
//   The settings survive a rebuild, which is exactly why this is
//   the right place for them.
//
//   WRITES ARE DELAYED THREE SECONDS, AND THAT IS THE WHOLE POINT.
//   A slider sends VALUE_CHANGED dozens of times while a finger
//   moves. Writing on each one would be dozens of write cycles for
//   one adjustment. The change marks the settings dirty; three
//   quiet seconds later it is written once. An AT24C32 is good for
//   about a million cycles, and this keeps a lifetime of adjusting
//   nowhere near it.
//
//   IT WAITS FOR THE WRITE CYCLE BY POLLING, NOT BY GUESSING. The
//   chip stops answering for roughly 5 ms after a write while it
//   burns the page, so the code probes it until it acknowledges,
//   with a ceiling in case it never does. A blind delay is either
//   too short and corrupts, or too long and stalls the UI.
//
//   MAGIC, VERSION AND CHECKSUM. A blank EEPROM is all 0xFF, and a
//   layout change makes yesterday's bytes mean something else.
//   Either one loads as garbage - a backlight of 255, an alarm at
//   minute 65535. Anything that does not match falls back to the
//   built-in defaults and says so on serial.
//
//   SIXTEEN BYTES AT ADDRESS 0, one page. The AT24C32 writes 32
//   bytes at a time and a write that crosses a page boundary wraps
//   to the start of the page instead of continuing - a classic way
//   to corrupt the front of a record. Staying inside one page
//   avoids the question entirely.
//
//   THE ADDRESS IS TWO BYTES, high then low. The AT24C02 on some
//   other RTC modules takes one. Sending one byte to this chip
//   writes to the wrong place and reads back nonsense.
//
// WHAT CHANGED IN TEST 021 vs TEST 020
//   The automatic brightness now reacts quickly, and how quickly is
//   a slider rather than a number I picked.
//
//   TEST 019 WAS TOO SLOW AND THE ARITHMETIC SAYS SO. It ran once a
//   second and moved one point per run, so 12 percent to 100 was 88
//   seconds. Turning on a lamp and waiting a minute and a half is
//   not a screen following the room.
//
//   IT RUNS EVERY 100 ms NOW, on its own timer, and the lux is read
//   there too. Ten reads a second of two bytes each is nothing on a
//   400 kHz bus.
//
//   THE SENSOR IS THE FLOOR, NOT THE CODE. The BH1750 in high
//   resolution finishes a conversion about every 120 ms, so nothing
//   can respond faster than that no matter where the slider sits.
//   Low resolution mode would be 16 ms but quantises to 4 lx, which
//   is useless at the 1 to 50 lx end where a bedroom actually
//   lives. High resolution stays.
//
//   ONE SLIDER, 1 TO 10, moves both halves together: the averaging
//   constant and how far the backlight may travel per tick. At 1 it
//   is the slow smooth behaviour of TEST 019. At 10 the full range
//   crosses in about a second, which is as close to instant as the
//   sensor allows.
//
//   THE FEEDBACK RISK GROWS WITH SPEED, and this is the part worth
//   reading twice. If the BH1750 can see the screen, brighter means
//   higher reading means brighter. TEST 019 was slow enough that
//   the loop could barely sustain itself. A fast loop oscillates
//   readily. IF THE BRIGHTNESS PUMPS UP AND DOWN ON ITS OWN, turn
//   the speed down first to confirm that is what it is, then move
//   the sensor. The code cannot fix a sensor that sees its own
//   output.
//
// WHAT CHANGED IN TEST 020 vs TEST 019
//   An alarm, on the spare tile. It rings by pulsing the panel
//   motor, as asked. Audio through audionode goes in when the bus
//   exists.
//
//   THE RED ALARM ICON IS NOT IN THE REPO, so the tile carried
//   LV_SYMBOL_BELL. Replaced in TEST 023 by a drawn clock.
//
//   TWO THINGS YOU DID NOT ANSWER, so I chose and I am saying so
//   rather than burying it: ONE alarm, and it repeats EVERY DAY.
//   Weekday selection and a second alarm for Ira are both small
//   additions on top of this, not rewrites.
//
//   IT WILL NOT RING ON AN UNSET CLOCK. If the oscillator stop flag
//   is up the time registers still return a number and it is
//   meaningless, so an alarm armed against it would go off at a
//   random moment. The check is the same one the display uses.
//
//   IT FIRES ON THE MINUTE CHANGING, not on the minute matching.
//   Matching would re-trigger every second for sixty seconds. The
//   very first tick after boot only records the minute and never
//   fires, so powering up inside the alarm minute does not set it
//   off.
//
//   EVERYTHING IS MINUTE OF DAY, so snooze crosses midnight without
//   a special case. 23:55 plus nine minutes is 00:04, and the
//   arithmetic does that on its own.
//
//   THE RING PULSE IGNORES THE TOUCH SETTINGS. Feedback tuned down
//   to 20 percent for a pleasant tap would make a useless alarm, so
//   the ring uses its own strength and length.
//
//   IT GIVES UP AFTER TWENTY MINUTES. An alarm nobody is there to
//   stop should stop by itself rather than buzz at an empty room
//   until the power goes.
//
//   HONEST LIMIT: a motor on the headboard is a weak alarm, and you
//   have already said the tick is barely noticeable. This will not
//   reliably wake you. The mattress motors over RS-485 are the real
//   answer, and they also give each side its own silent alarm.
//
// WHAT CHANGED IN TEST 019 vs TEST 018
//   The BH1750 now drives the backlight. Brighter room, brighter
//   screen. Two sliders set the ends of the range and a switch
//   turns the whole thing off.
//
//   THE MAPPING IS LOGARITHMIC, NOT LINEAR, and this is the
//   difference between working and useless. The eye responds to
//   light roughly logarithmically, and lux does not: a lit bedroom
//   is around 50 lx, an office 400, a window on a grey day 2000.
//   Mapped linearly to 0-100, everything from dusk to indoors sits
//   in the bottom few percent and the screen is dim all evening,
//   then pinned at maximum the moment a lamp comes on. log10 of the
//   lux is mapped instead, from 1 lx to 1000 lx.
//
//   IT IS SMOOTHED AND IT MOVES SLOWLY. Your own logs had lux
//   swinging 31 to 222 inside one minute - someone walking past,
//   a shadow, a cloud. Driving the backlight from that directly
//   makes the screen breathe. So the lux runs through an
//   exponential average, and the backlight then walks toward its
//   target one point per second. A cloud is invisible; walking into
//   the room takes a few seconds to answer, which is what a person
//   expects anyway.
//
//   THERE IS A DEADBAND. Without one the backlight hunts by a point
//   forever, and on this panel a backlight step is visible.
//
//   THE FEEDBACK LOOP IS THE REAL HAZARD, and it depends on where
//   you mounted the sensor. If the BH1750 can see the screen or its
//   reflection, brighter screen means higher reading means brighter
//   screen. The slow time constant and the deadband make that
//   oscillation hard to sustain, but they do not remove it. IF THE
//   BRIGHTNESS EVER PUMPS SLOWLY UP AND DOWN ON ITS OWN, that is
//   this loop, and the fix is the sensor's position, not the code.
//
//   TWO SLIDERS, ONE SWITCH, NO THIRD SLIDER. With Auto on, Min and
//   Max are the ends of the range the room maps onto. With Auto
//   off, Max IS the fixed manual brightness and Min is ignored. So
//   the manual control from TEST 016 has not been lost - it is the
//   same slider wearing its other hat.
//
// WHAT CHANGED IN TEST 018 vs TEST 017
//   A third settings tab, Touch, with two sliders: pulse strength
//   and pulse length. Same two numbers the v and h serial commands
//   set, now under a finger.
//
//   THE TEST PULSE FIRES ON RELEASE, NOT ON VALUE CHANGE. A slider
//   sends VALUE_CHANGED dozens of times while the finger moves, and
//   firing on each one would buzz continuously through the drag.
//   The number updates live; only the vibration waits for the
//   finger to lift, so what you feel is the value you chose.
//
//   Length runs 20 to 500 ms. Below about 40 ms an ERM has not
//   spun up and there is nothing to feel - that is the whole reason
//   TEST 012 seemed dead - so the bottom of the range is left in as
//   a usable "almost nothing" rather than clamped away.
//
//   THREE TUNED VALUES ARE NOW UNSAVED: backlight, strength and
//   length, all back to defaults on reboot. The AT24C32 at 0x57 is
//   the place for them and is still unused. Not built without
//   being asked for.
//
// WHAT CHANGED IN TEST 017 vs TEST 016
//   A REAL BUG, MINE. The day of week was one short - it said Thu
//   for Friday 4 September 2026. Zeller itself was right and
//   returned h=6, which IS Friday. The mapping from h to the
//   DS3231's 1..7 was wrong: ((h + 5) %% 7) + 1 instead of
//   ((h + 6) %% 7) + 1. Checked against 1 January 2000, a Saturday,
//   which now comes out Sat. The wrong day is already written into
//   the chip, so the clock has to be set once more after flashing.
//
//   Removed, as asked: the 1024 x 600 label, the seconds in the
//   time, the century in the year, and the I2C line at the bottom
//   of the home screen.
//
//   THE ONE SECOND I2C SCAN IS ALSO GONE, not just its label. It
//   existed to feed that line. Leaving it running to write to
//   nothing would burn bus time every second for no reader. The
//   serial commands survive and now do the scan on demand:
//       i   the named table
//       s   full sweep 0x08..0x77
//
//   FOUR SCREENS: Massage, Radio, Air conditioner, Fan.
//
//   THE HEBREW STRINGS ARE STORED REVERSED. They are copied
//   verbatim from TEST 081, where the letters are written back to
//   front on purpose because LVGL has no bidirectional text and
//   would otherwise draw Hebrew left to right. Do not "fix" them in
//   an editor - they are correct as they stand and will break if
//   reordered.
//
//   NONE OF THESE SCREENS CONTROLS ANYTHING YET, and this is the
//   honest state of it. There is no RS-485 bus, no bedbox firmware
//   and no IR transmitter on this panel. Every control changes a
//   variable and prints to serial. They are real interfaces waiting
//   for a transport, not simulations pretending to work.
//
//   THE RADIO STATION LIST IS A PLACEHOLDER except Galgalatz, which
//   is the one station with a URL verified end to end on audionode.
//   The other seven are names on tiles. Replace them when the real
//   list exists.
//
//   THE AC SCREEN SHOWS THE ROOM TEMPERATURE FROM THE SHT3x, so the
//   target can be set against something real rather than guessed.
//
// WHAT CHANGED IN TEST 016 vs TEST 015
//   A settings screen, ported from buildSettings() in TEST 081.
//   Two tabs for now - Clock and Display - with room for Massage,
//   Files and About later, exactly as the old one had.
//
//   FONTS: 26 is the largest available. The Waveshare lv_conf.h
//   enables only 12, 14, 16 and 26, and the vendor file stays
//   untouched by decision. The old screen used 40 for the digits
//   and 28 for the +/- signs; both drop. The BUTTONS are still
//   large - a touch target is set by its size, not its font - so
//   the screen is no harder to use, only less dramatic.
//
//   24 HOUR, so the screen and the driver speak the same language.
//   The old screen was 12 hour with an AM/PM toggle, which meant
//   converting in two directions around a register whose bit 6
//   already encodes the mode. One representation, no conversion.
//
//   THE TABVIEW IS PAINTED BY HAND. This is not decoration. LVGL's
//   default theme leaves a tabview and its bar near white, which on
//   this screen at night is a slab of light in a dark bedroom. TEST
//   073 hit this on the old panel; the same treatment is carried
//   over.
//
//   THE FIELDS SEED FROM THE CHIP on every entry to the screen, via
//   LV_EVENT_SCREEN_LOAD_START. Seeding once at boot would mean
//   opening settings later showed stale numbers and pressing Set
//   would quietly move a correct clock backwards.
//
//   THE YEAR IS CLAMPED TO 2000-2099 because the DS3231 stores two
//   digits and a century bit that this driver does not use. A year
//   outside that range cannot round trip through the register.
//
//   NOT PERSISTENT. The backlight slider takes effect immediately
//   but is not stored, so it returns to the default on reboot. The
//   AT24C32 at 0x57 is the obvious place for it and is still
//   unused. Left out on purpose rather than added unasked.
//
//   HAPTIC ON THE TAB BUTTONS DOES NOT FIRE. The buttons this file
//   creates carry EVENT_BUBBLE and reach the screen handler; the
//   ones LVGL builds inside the tabview do not. Known, small, and
//   not worth reaching into tabview internals for.
//
// WHAT CHANGED IN TEST 015 vs TEST 014
//   Half the minute lines said sht FAIL (fetch NACK) and the other
//   half read correctly, alternating. My bug, introduced in TEST
//   013.
//
//   A NACK ON FETCH IN PERIODIC MODE MEANS "NO NEW MEASUREMENT
//   YET". It is documented behaviour, not an error. The sensor is
//   in 0.5 mps - one measurement every two seconds - and the data
//   is cleared once read. shtRead() runs every second. So every
//   other fetch necessarily lands between measurements and is
//   correctly not acknowledged.
//
//   TEST 013 treated that as a failure: it cleared the reading, set
//   shtOK false, and counted toward a rearm that was never needed.
//   The visible cost was the climate line on screen flickering to
//   -- and back every other second.
//
//   NOW: a NACK keeps the last good value and changes nothing. The
//   reading is only declared stale after SHT_STALE_MS with no
//   successful fetch, and only then does the rearm fire. That is a
//   real fault - a sensor knocked out of periodic mode - as opposed
//   to the sensor working exactly as configured.
//
//   The values themselves were never in doubt: 25.7 C drifting down
//   to 25.24 over twenty minutes, 59 %RH throughout, and the DS3231
//   tracking within half a degree.
//
// WHAT CHANGED IN TEST 014 vs TEST 013
//   The tick was felt but barely. Turned up to maximum.
//
//   DURATION MATTERED MORE THAN DUTY, and this is the real fault
//   in TEST 012. A coin ERM needs 50 to 80 ms to reach full speed.
//   The pulse was 40 ms end to end, so it FINISHED BEFORE THE MOTOR
//   GOT THERE. What reached the finger was the spin up, never the
//   vibration. Duration is now 150 ms, which is past full speed
//   with room to be felt.
//
//   Duty is 100 percent. At 36 mA measured this costs the 5 V rail
//   nothing. The motor's rated voltage is not printed on it, so
//   full rail continuously would be a gamble - but at 150 ms per
//   touch the duty cycle over any minute is a fraction of a
//   percent, and brush wear does not accumulate at that rate.
//
//   THE KICK IS GONE. It existed to break stiction before dropping
//   to 70 percent. At 100 percent there is nothing to drop to, so
//   it became a no-op.
//
//   LIVE TUNING, same pattern as n/f/b:
//       v<0-100>   strength
//       h<ms>      pulse length
//   Feel is not a thing to guess at from here, and rebuilding to
//   change one number wastes your afternoon. Both print what they
//   land on so the final values can be written into the file.
//
//   IF IT IS STILL WEAK, THE FIRMWARE IS NO LONGER THE LIMIT. This
//   is as hard as the part goes. Past here it is mounting: a motor
//   on foam, on soft tape, or hanging on its wires puts its energy
//   into moving itself instead of the panel.
//
// WHAT CHANGED IN TEST 013 vs TEST 012
//   The SHT3x was NEVER BROKEN. TEST 012 printed sht 0.00 C once
//   and I called it a systematic failure. It was not. The minute
//   line fires on the first tick where the RTC reads, and on that
//   tick the measurement command has only just been sent - the
//   collection happens on the NEXT tick. So the first line can only
//   ever print a failure. Every line after it was correct: 26.7 C,
//   56 %RH, and the DS3231 converging to within 0.2 C of it.
//
//   I drew a conclusion from one sample taken at the one moment it
//   could not have worked. The atomic fetch and the self rearming
//   below are worth keeping as robustness, but they fixed nothing,
//   and calling them a fix would have been false.
//
// WHAT CHANGED IN TEST 012 vs TEST 011
//   TEST 012 ran. BH1750 reads, DS3231 answers, backlight and
//   touch fine. Two things were wrong.
//
//   1. monitor_filters had "echo", which is not a real filter.
//      PlatformIO printed "Skipping unknown filters" and ignored
//      it. My mistake. Removed. Typing stays blind; send_on_enter
//      still buffers the line and sends it on Enter.
//
//   2. THE SHT3x READ FAILED EVERY TIME, and the printout proved
//      it rather than merely suggesting it: the value was 0.00 C.
//      The conversion is -45 + 175 * raw/65535, so even an all
//      zero reading prints -45.00. A clean 0.00 can only come from
//      the shtOK ? value : 0.0f branch. The read never succeeded.
//
//      HYPOTHESIS, labelled as one: the presence scanner killed it.
//      It probes 0x44 once a second - a START, an address, a STOP,
//      no command. TEST 012 left the sensor holding a single shot
//      result between two one second ticks, and a foreign write in
//      that gap discards it. It also explains why the BH1750
//      survives: its state is a running mode, not a one shot result
//      that can be thrown away.
//
//      THE FIX DOES NOT RELY ON THAT HYPOTHESIS. The sensor now
//      runs in periodic mode and the result is fetched in ONE
//      atomic transaction - command and read joined by a repeated
//      START, which no other transfer can get between. There is no
//      longer any state that has to survive from one bus access to
//      the next, so there is nothing left to lose.
//
//      0.5 measurements per second, high repeatability. Periodic
//      mode also drops self heating below what single shot on
//      demand was doing.
//
//      AND IT REARMS ITSELF. Five consecutive failures resend the
//      periodic mode command. If anything ever does knock the
//      sensor out of it, the panel recovers instead of showing --
//      until the next reboot.
//
//      DIAGNOSTICS ADDED. Two different failures - a NACK and a bad
//      checksum - printed identically before. Now the minute line
//      names which one happened.
//
// WHAT CHANGED IN TEST 012 vs TEST 011
//   A coin vibration motor on GPIO 6 gives a short tick on every
//   touch, anywhere on the screen.
//
//   MEASURED, not assumed: 22 mA at 3.3 V, 36 mA at 5 V, both with
//   the motor spinning. Under 1 mA on IN, which is a MOSFET gate,
//   not a bipolar base. 535 mV across the motor pads in diode mode,
//   so the flyback diode is already on the module. Nothing external
//   is needed. VCC comes off the same 5 V that feeds the board, so
//   the ground is common by construction.
//
//   LEDC, NOT digitalWrite. Not for the current - 36 mA is nothing.
//   For control. The motor's rated voltage is not printed anywhere
//   and the AliExpress text is machine generated and demonstrably
//   wrong about the part, so running it at a duty cycle instead of
//   a rail is the honest way to avoid committing to a number nobody
//   stated. 70 percent of 5 V is about the 3.3 V that already
//   worked.
//
//   ARDUINO CORE 3.x LEDC API. ledcSetup and ledcAttachPin are
//   gone. It is ledcAttach(pin, freq, bits), and ledcWrite takes
//   the PIN, not a channel number. Passing a channel silently
//   writes to the wrong pin.
//
//   20 kHz, above hearing. A motor driven in the audible band
//   whines, and this sits on a headboard.
//
//   THERE IS A STARTING KICK. An ERM will not begin to turn at 70
//   percent from standstill - static friction plus an off-centre
//   weight needs more. So the pulse opens at 100 percent for 12 ms
//   and then drops to 70 for the rest. Without it the tick feels
//   late and weak, or does not happen at all. Set HAPTIC_KICK_MS to
//   0 to remove it.
//
//   THE PULSE IS A TIMER, NOT A DELAY. Same reason the SHT3x read
//   is split across ticks: a delay inside the LVGL task can show as
//   a dropped frame on an RGB panel. The touch returns immediately
//   and the motor stops on its own.
//
//   ONE HANDLER FOR THE WHOLE SCREEN. Every tile gets
//   LV_OBJ_FLAG_EVENT_BUBBLE, so a press on a tile travels up to
//   the screen, and a press on bare background lands there
//   directly. One callback covers both, and every tile added later
//   is covered without touching this code again.
//
//   LV_EVENT_PRESSED, not CLICKED. Pressed fires the moment the
//   finger lands. Clicked waits for the release. Feedback that
//   arrives on release is not feedback.
//
//   GPIO 6 FLOATS DURING RESET, so the motor twitches at every
//   boot. Left in deliberately - it is a usable sign that the
//   system came up. It is not under firmware control: it also
//   happens on a watchdog reset or a brownout recovery, so a boot
//   loop would buzz in a loop. A 10k from IN to GND removes it.
//
// NOT YET COMPILED, NOT YET RUN ON HARDWARE.
//
// WHAT CHANGED IN TEST 011 vs TEST 010
//   TEST 010 proved the bus on hardware. Answering: 0x24 IO
//   expander, 0x5D touch, 0x44 SHT3x, 0x68 DS3231, 0x57 AT24C32.
//   The presence scan stays exactly as it was and still runs.
//
//   Added: real drivers for the RTC and the climate sensor, a
//   reading line across the top, and a way to set the clock.
//
//   BH1750 DID NOT ANSWER. Neither 0x23 nor 0x5C. Its driver is
//   written anyway and it degrades to "--". Plug the module in and
//   it starts reading with no code change.
//
//   THE OSCILLATOR STOP FLAG IS CHECKED, and this matters. The
//   DS3231 sets bit 7 of register 0x0F when the oscillator has
//   stopped - a dead backup cell, or a first power-up. The time
//   registers still return a number after that, and the number is
//   meaningless. Without the check the panel shows a confident
//   wrong time. With it, it says CLOCK NOT SET and waits.
//
//   THE SHT3x READ IS SPLIT ACROSS TWO TICKS. A single-shot high
//   repeatability measurement needs about 15 ms between the
//   command and the result. Sleeping for it inside the LVGL timer
//   would stall the task and can show as a dropped frame on an RGB
//   panel. So one tick sends the command, the next tick collects
//   it. The sensor holds the result until read. Nothing blocks.
//
//   CLOCK STRETCHING IS NOT USED for the same class of reason: it
//   would hold SCL low, and the touch controller is on that wire.
//
//   READING EVERY 5 SECONDS, NOT EVERY SECOND. The SHT3x heats
//   itself slightly when measuring. At high repeatability, once a
//   second, that self-heating is measurable in a reading that is
//   supposed to be room temperature. Five seconds is far more than
//   a bedroom needs.
//
//   SETTING THE TIME IS OVER SERIAL, NOT ON SCREEN. Deliberate
//   order: prove the driver against real registers first, then put
//   an interface on a thing that is known to work. The on-screen
//   setter comes back afterwards.
//       T2026-09-03 13:45:00
//   Day of week is computed, not typed. The command also clears
//   the oscillator stop flag, because a fresh set is exactly the
//   moment the old warning stops being true.
//
//   The serial line buffer was 8 characters. A time string is 19.
//   Raised to 40.
//
//   THE DS3231 HAS ITS OWN TEMPERATURE SENSOR and it is shown on
//   serial next to the SHT3x. It is only good to about 3 degrees
//   and it sits on a warm board, so it is not the room reading -
//   it is a cross-check. Two independent sensors disagreeing by
//   fifteen degrees means something is wrong with one of them.
//
//   0x57 IS 4 KB OF FREE EEPROM on the RTC module, and it survives
//   a flash erase. Noted as an asset. Not used yet.
//
// WHAT CHANGED IN TEST 010 vs TEST 009
//   The home screen is untouched. Same eight tiles, same 200 px
//   artwork, same positions, same live dim and backlight tuning.
//   Nothing about the layout moved.
//
//   What is added is a presence line above the status line, and a
//   named table on serial. This is the first time the sensors hang
//   on the bus, and an address that answers proves the wiring
//   before a single driver exists to be blamed.
//
//   MODULES ON HAND, confirmed from photographs of the boards:
//     DS3231M RTC   OPEN-SMART, CR1220 fitted      expect 0x68
//     SHT3x         purple breakout, SHT3x marked  expect 0x44
//     BH1750        GY-302 V322                    expect 0x23
//     one blue 7-pin board, ADD RST ALE SDA SCL VCC GND,
//       chip marking unreadable in the photograph - the scan
//       is what identifies it, not a guess
//
//   0x24 and 0x5D MUST appear. They are the IO expander and the
//   touch controller, both already on this bus and both working.
//   If either goes missing the fault is the bus itself, not a
//   sensor, and nothing further should be built until it is back.
//
// WHY THE SCAN RUNS INSIDE THE LVGL TASK
//   Touch is read from the LVGL task. Scanning from loop() would
//   put two tasks on I2C_NUM_0 at once. Everything that touches
//   the bus now happens in one place, in one order. The serial
//   sweep command sets a flag; the timer does the work.
//
// WHY IT PROBES AND DOES NOT WRITE
//   A write would change state. 0x00 to a BH1750 is Power Down,
//   and a stray byte to the GT911 can leave its register pointer
//   somewhere unexpected and cost you the touchscreen.
//
// WHY NOT DEV_I2C_Read_Byte
//   Every helper in lib/i2c is wrapped in ESP_ERROR_CHECK. A NACK
//   from an empty address is normal and expected during a scan,
//   but ESP_ERROR_CHECK turns it into a panic - which would mean a
//   reboot loop on every pass. i2c_master_probe is called directly
//   and its return value is checked.
//
// THE BUS IS BORROWED, NEVER RE-CREATED
//   DEV_I2C_Init() calls i2c_new_master_bus and is already called
//   once inside touch_gt911_init(). Calling it again panics. The
//   handle is taken from the global that i2c.cpp keeps.
//
// NOT YET COMPILED, NOT YET RUN ON HARDWARE.
//
// WHAT THIS IS
//   The home screen from the old panel, laid out for the new one.
//   Tiles and touch only. No sensors, no clock, no RS485, no photos.
//   Look at it, decide whether the layout is right, and only then
//   port the deeper screens.
//
// WHY 4 x 2 AND NOT 3 x 2
//   The old grid was three 200 px tiles across, 22 px apart, starting
//   at x = 78. That is 644 px of tiles inside 800, leaving 78 px each
//   side.
//
//   Four of the same tiles is 866 px inside 1024, leaving 79 px each
//   side. Within one pixel of the old margin. So the wider screen
//   buys a whole extra column without changing a single dimension,
//   and every icon stays at its native 200 x 200 - no scaling, no
//   softening.
//
//   Vertically 600 - 416 = 184 spare, split as a taller status strip
//   above and breathing room below.
//
// THE TWO NEW SLOTS
//   Position 3 on the top row is the ceiling fan. Its 433 MHz
//   protocol is already decoded: protocol 2, 29 bits, address
//   0x3D755, fourteen buttons captured.
//
//   And it turns out the artwork already exists. fan_img.h was
//   sitting in include/ - 200x200, cut from a photograph of the
//   actual fan in the room. It was made for TEST 082, the delivery
//   that was never pushed and was assumed lost. The code went, the
//   picture survived. So the fan tile is a real photograph like the
//   other five, not a placeholder symbol.
//
//   Position 3 on the bottom row is left free on purpose.
//
// WHAT CHANGED vs TEST 008
//   A single dim value was applied to every tile whatever its state,
//   so on and off differed only by the artwork swap. Feedback from
//   the screen: keep the lit state as it is and push the unlit state
//   further down, so the difference is obvious at a glance rather
//   than something you have to read.
//
//   So the dim level is now a pair - dimOn and dimOff - and the tile
//   background darkens as well when unlit. Two effects stacking:
//   the picture recedes and the panel behind it recedes with it.
//
//   Both are tunable live, same as the backlight:
//       n<v>   dim when ON    0..255
//       f<v>   dim when OFF   0..255
//       b<v>   backlight      0..100
//
//   Defaults are a starting point only. Real calibration waits for
//   the BH1750 - a value picked by eye in daylight is wrong at night,
//   and with the sensor in place brightness becomes a curve rather
//   than a number. No point doing it twice.
//
// WHAT CHANGED IN TEST 008 vs TEST 007  -  A HARDWARE FACT WORTH KEEPING
//   The backlight control is INVERTED. Measured, not guessed:
//   b0 gave a bright, readable screen and b100 gave a dark one.
//
//   IO_EXTENSION_Pwm_Output() does not take brightness. It takes how
//   much to DIM. So every build from TEST 003 to TEST 007 called it
//   with 100 at startup and ran the panel at full dim, which is why
//   even d0 looked washed out. The artwork was never the problem and
//   neither was the dim value - the screen was simply turned down.
//
//   From here the code inverts it: applyBacklight(100) writes 0 to
//   the expander and gives full brightness. b100 is bright, b0 is
//   off, which is what anyone would expect.
//
//   Leave this inversion in place. It is a property of the board,
//   and it will silently break every screen that gets ported if it
//   is ever removed.
//
// WHAT CHANGED IN TEST 007 vs TEST 006
//   Dim 255 gave a black screen, dim 0 still looked dull. That rules
//   the dimming out: 0 means no darkening at all, so if the artwork
//   is still weak at 0 the problem was never the dim value.
//
//   The remaining suspect is the backlight. TEST 002 proved the
//   IO expander drives it - the slider visibly changed brightness.
//   From TEST 003 on it was set once in setup() and never again,
//   with no way to check or change it. So it is now a live command
//   too, and the two are separated:
//       d<n>   tile dim      0..255
//       b<n>   backlight     0..100
//   Change one, watch, change the other. Whichever moves the picture
//   is the one that matters.
//
//   Default dim is now 10, chosen from the live tuning.
//
//   The monitor also does not echo what is typed. That is a
//   platformio.ini setting, not code: monitor_filters = echo.
//
// WHAT CHANGED IN TEST 006 vs TEST 005
//   TEST 005 ran and all eight tiles came up, but the artwork was
//   almost black. TILE_DIM_OPA was 110, a value tuned on the old
//   800x480 Guition panel. This screen has a different backlight and
//   a different panel, so the number did not carry over. Carrying it
//   was reasonable; not checking it was not.
//
//   Rather than guess a new value and rebuild five times, the dim
//   level is now adjustable from the serial monitor while the screen
//   is running. Type a number, watch the tiles change, and when it
//   looks right that number goes into the code as the new default.
//
//   Default lowered from 110 to 40 as a starting point.
//
// WHAT CHANGED IN TEST 005 vs TEST 004
//   TEST 004 would not compile: lv_font_montserrat_40 and _20 are not
//   built. The Waveshare lv_conf.h enables only 12, 14, 16 and 26.
//   The old panel had its own lv_conf.h with more sizes turned on,
//   and I wrote against that from memory instead of reading the one
//   actually in use.
//
//   So 40 becomes 26 and 20 becomes 16. Their lv_conf.h is left
//   untouched on purpose: it is a config file and editing it is
//   legitimate, but every edit to the vendor port layer is one more
//   thing to remember and restore if it is ever updated. Fitting our
//   code to what exists costs nothing here.
//
//   To get bigger symbols later, set LV_FONT_MONTSERRAT_40 to 1 in
//   lib/lv_conf.h. Not needed for this.
//
// WHAT IS DELIBERATELY MISSING
//   Every tile prints to serial instead of acting. Nothing is wired
//   to the bus yet, and a tile that silently does nothing is worse
//   than one that says so.
//
// NOT YET RUN ON HARDWARE.
//
// ============================================================

#include <Arduino.h>
#include <math.h>   // log10f, for the logarithmic lux mapping
#include "lvgl_port.h"
#include "rgb_lcd_port.h"
#include "io_extension.h"
#include "i2c.h"        // DEV_I2C_Port, driver/i2c_master.h

#include "tile_img.h"       // img_bed_off/on, img_radio_off/on,
                            // img_ac_off/on, img_shade_open/closed
#include "lamp_img.h"       // img_lamp
#include "fan_img.h"        // img_fan - recovered from TEST 082
#include "font_hebrew.h"     // font_hebrew_28
#include "alarm_img.h"      // img_alarm_on / img_alarm_off
#include "boot_photo.h"     // BOOT_SHEMI_JPG / BOOT_IRA_JPG
#include <JPEGDEC.h>

// 1 = this panel, Shemi. 2 builds Ira's from the same file and
// shows her photograph, so the picture on screen tells you which
// board you just flashed.
#define PANEL_ID 2
#define RS485_RX_PIN 15
#define RS485_TX_PIN 16
#define RS485_BAUD   115200
#define RS485        Serial1

static constexpr uint8_t NODE_SHEMI  = 1;
static constexpr uint8_t NODE_IRA    = 2;
static constexpr uint8_t FRAME_START = 0xA5;
static constexpr uint8_t TYPE_TIME   = 0x10;
static constexpr uint8_t TYPE_ACK    = 0x11;

struct TimeFrame {
  uint8_t start;
  uint8_t src;
  uint8_t dst;
  uint8_t type;
  uint8_t seqLo;
  uint8_t seqHi;
  uint8_t yearLo;
  uint8_t yearHi;
  uint8_t month;
  uint8_t day;
  uint8_t hour;
  uint8_t minute;
  uint8_t second;
  uint8_t crc;
};

static_assert(sizeof(TimeFrame) == 14, "TimeFrame must be exactly 14 bytes");

static uint32_t rsGoodTimes = 0;
static uint32_t rsBadFrames = 0;
static uint32_t rsReplies   = 0;

static uint8_t rsFrameCrc(const TimeFrame &f) {
  const uint8_t *p = reinterpret_cast<const uint8_t *>(&f);
  uint8_t c = 0;
  for (size_t i = 0; i < sizeof(TimeFrame) - 1; ++i) c ^= p[i];
  return c;
}

static uint16_t rsFrameSeq(const TimeFrame &f) {
  return uint16_t(f.seqLo) | (uint16_t(f.seqHi) << 8);
}

static uint16_t rsFrameYear(const TimeFrame &f) {
  return uint16_t(f.yearLo) | (uint16_t(f.yearHi) << 8);
}

static TimeFrame rsMakeAck(uint16_t seq) {
  TimeFrame f{};
  f.start = FRAME_START;
  f.src = NODE_IRA;
  f.dst = NODE_SHEMI;
  f.type = TYPE_ACK;
  f.seqLo = uint8_t(seq & 0xFF);
  f.seqHi = uint8_t((seq >> 8) & 0xFF);
  f.crc = rsFrameCrc(f);
  return f;
}

static void rsSendFrame(const TimeFrame &f) {
  RS485.write(reinterpret_cast<const uint8_t *>(&f), sizeof(f));
  RS485.flush();
}

static bool rsReadFrame(TimeFrame &out) {
  static uint8_t buf[sizeof(TimeFrame)];
  static size_t pos = 0;

  while (RS485.available()) {
    const uint8_t b = uint8_t(RS485.read());

    if (pos == 0) {
      if (b != FRAME_START) continue;
      buf[pos++] = b;
      continue;
    }

    buf[pos++] = b;

    if (pos == sizeof(TimeFrame)) {
      memcpy(&out, buf, sizeof(out));
      pos = 0;
      return true;
    }
  }
  return false;
}

static bool rsTimeFieldsValid(const TimeFrame &f) {
  const uint16_t y = rsFrameYear(f);
  return y >= 2000 && y <= 2099 &&
         f.month >= 1 && f.month <= 12 &&
         f.day >= 1 && f.day <= 31 &&
         f.hour <= 23 && f.minute <= 59 && f.second <= 59;
}

#define TEST_NUMBER 89

// ------------------------------------------------------------
// Layout. Every number here is derived, not guessed - see the
// header note on why four columns fit exactly.
// ------------------------------------------------------------

#define SCREEN_W    1024
#define SCREEN_H     600

#define TILE_SZ      200
#define TILE_GAPX     22
#define TILE_GAPY     16
#define TILE_COLS      4
#define TILE_ROWS      2

// (1024 - (4*200 + 3*22)) / 2 = 79
#define TILE_X0       79
// status strip above, a little air below
#define TILE_Y0      108

// How far the tile artwork is pulled toward black, 0..255. Carried
// over from TEST 080 unchanged: 110 takes the glare off the white
// bed without making the icons hard to read.
// Runtime, not a #define, so it can be tuned live. 0 is the original
// artwork, 255 is a black square.
// Lit tiles keep the artwork close to original. Unlit tiles sink
// well back. The gap between the two is the whole point.
#define DIM_ON_DEFAULT   10
#define DIM_OFF_DEFAULT 120
static uint8_t dimOn  = DIM_ON_DEFAULT;
static uint8_t dimOff = DIM_OFF_DEFAULT;

// The panel behind the artwork moves too, so an unlit tile recedes
// twice over.
#define TILE_BG_ON  0x181C22
#define TILE_BG_OFF 0x0C0F13

// Backlight, 0..100, straight through the IO expander at 0x24.
#define BACKLIGHT_DEFAULT 100
static uint8_t backlight = BACKLIGHT_DEFAULT;

// ---- automatic brightness ----
//
// AUTO_LUX_LO / HI are the ends of the mapped range in lux. 1 lx is
// a dark room, 1000 lx is a bright one indoors; beyond that the
// screen is already at maximum and more light changes nothing.
#define AUTO_LUX_LO      1.0f
#define AUTO_LUX_HI      1000.0f
#define AUTO_TICK_MS     100     // the controller's own timer
#define AUTO_DEADBAND    1       // points; below this, do not move

// One slider drives both halves of the response. Speed 1 is the
// slow, smooth behaviour; speed 10 crosses the whole range in about
// a second, which is as fast as a 120 ms sensor conversion allows.
#define AUTO_SPEED_MIN   1
#define AUTO_SPEED_MAX   10
static uint8_t autoSpeed = 6;

// ---- screensaver ----
// Here rather than with the screensaver code, because the EEPROM
// record below has to see them and it comes first in the file.
// 0 means never.
// Nine steps, not a free slider. Nobody wants a screensaver delay
// of seven minutes and twenty three seconds, and a free slider
// makes landing exactly on five minutes a fiddle.
static const uint32_t kSaverOpts[9] = {
  0, 15000UL, 30000UL, 60000UL, 120000UL,
  300000UL, 600000UL, 1200000UL, 1800000UL
};
static const char *kSaverNames[9] = {
  "Never", "15 sec", "30 sec", "1 min", "2 min",
  "5 min", "10 min", "20 min", "30 min"
};
// The five entry table byte 12 was written against, kept only so an
// older record can be translated instead of thrown away.
static const uint8_t kSaverLegacy[5] = { 0, 2, 3, 5, 6 };

static uint8_t  saverIdx     = 5;          // 5 minutes
static uint32_t saverAfterMs = 300000UL;

// Was a constant in the source until now, which meant the one
// number deciding how much light sits beside a sleeping head could
// only be changed by rebuilding.
static uint8_t  saverBright  = 8;          // percent

// These three live here too, and for the same reason: the touch
// handler and the brightness controller both use them and both come
// long before the screensaver code in this file.
// Stored as "once" rather than "repeat". Bit 3 of the flags byte
// was always zero before, and every alarm until now repeated daily -
// so zero has to keep meaning every day, or every saved alarm would
// quietly become once-only on the first boot after flashing.
static bool     alarmOnce     = false;

// ---- radio sleep timer ----
// sleepEnd is a millis() deadline, 0 meaning off. Compared by
// subtraction, never by >=, because millis() wraps after about 49
// days and a headboard panel will cross that.
static uint8_t  sleepMin = 30;
static uint32_t sleepEnd = 0;

static bool     saverOn       = false;
static uint32_t lastTouchMs   = 0;
static uint8_t  blBeforeSaver = 100;

// ---- clock display ----
// Display only. The DS3231 is written in 24 hour form whatever this
// says, so nothing in the driver has to know about it.
static bool use12h = false;

// h is 0..23. Returns 1..12 and sets suffix to "AM" or "PM".
// Written out rather than using h % 12, which maps both midnight
// and noon to zero and prints "0:15 AM".
static uint8_t to12(uint8_t h, const char **suffix) {
  *suffix = (h < 12) ? "AM" : "PM";
  if (h == 0)  return 12;      // midnight
  if (h == 12) return 12;      // noon
  return (uint8_t)(h % 12);
}

// ---- touch feedback, tunable ----
// Declared here rather than beside the motor code so the EEPROM
// record below can see every stored value in one place.
#define HAPTIC_DUTY_PCT  100     // full rail; 36 mA measured, the rail shrugs
#define HAPTIC_BODY_MS   150     // past the 50-80 ms an ERM needs to spin up
static uint8_t  hapDutyPct = HAPTIC_DUTY_PCT;
static uint16_t hapLenMs   = HAPTIC_BODY_MS;

static inline float autoAlpha() { return 0.04f * autoSpeed; }   // 0.04 .. 0.40
static inline int   autoStep()  { return autoSpeed; }           // points per tick

static bool    autoBright = true;
static uint8_t blMin      = 12;
static uint8_t blMax      = 100;
static float   luxSmooth  = -1.0f;   // negative = not seeded yet

// Hebrew is stored pre-reversed in the font header, so the string is
// written here in visual order and needs no bidi engine.
#define HEB_BEDROOM  "\xD7\x94\xD7\xA0\xD7\x99\xD7\xA9\x20\xD7\xA8\xD7\x93\xD7\x97"
#define TITLE_HOME   "BG " HEB_BEDROOM

// ------------------------------------------------------------

static lv_obj_t *scrHome     = NULL;
static lv_obj_t *scrSettings = NULL;

// These two live on the settings screen but are refreshed by
// sensorTick, which is defined further up the file - so they are
// declared here rather than beside the rest of the settings widgets.
static lv_obj_t *lblNowTime  = NULL;   // live clock, Clock tab
static lv_obj_t *lblLuxLive  = NULL;   // live lux, Display tab
static lv_obj_t *lblAcRoom   = NULL;   // live room temperature, AC screen

// The other screens. Declared here because the home tile handlers
// further down have to load them, and they are built much later.
static lv_obj_t *scrMassage  = NULL;
static lv_obj_t *scrRadio    = NULL;
static lv_obj_t *scrAC       = NULL;
static lv_obj_t *scrFan      = NULL;
static lv_obj_t *scrAlarm    = NULL;

// ---- alarm ----
//
// Everything is minute of day, 0..1439. Snooze then crosses
// midnight with no special case: 23:55 plus nine is 00:04 and the
// modulo does it.
#define ALARM_SNOOZE_MIN  9
#define ALARM_GIVEUP_MS   (20UL * 60UL * 1000UL)  // stop buzzing at an empty room
#define ALARM_RING_EVERY  2000UL                  // ms between ring pulses
#define ALARM_RING_PCT    100                     // ignores the touch settings
#define ALARM_RING_MS     400

static bool     alarmOn      = false;
static int      alarmMin     = 7 * 60;   // 07:00
static int      snoozeUntil  = -1;       // minute of day, -1 = none
static bool     ringing      = false;
static uint32_t ringStart    = 0;
static uint32_t lastRingPulse= 0;
static int      lastMinSeen  = -1;
static bool     alarmFirstTick = true;
static lv_obj_t *lblStatus   = NULL;
static lv_obj_t *lblClock    = NULL;   // time, top centre
static lv_obj_t *lblClimate  = NULL;   // temperature, humidity, lux

// ------------------------------------------------------------
// I2C presence scan
//
// The bus already exists. DEV_I2C_Init() ran inside
// touch_gt911_init() and stored the handle in a file-scope global
// in lib/i2c/i2c.cpp. It is borrowed here. Calling DEV_I2C_Init()
// a second time would call i2c_new_master_bus twice and panic.
// ------------------------------------------------------------
extern DEV_I2C_Port handle;          // defined in lib/i2c/i2c.cpp

struct Known {
  uint8_t     addr;
  const char *name;
  bool        required;   // must answer, or the bus itself is broken
};

// Only these are polled every second. A full 0x08..0x77 sweep is
// 112 transactions and would stall the LVGL task long enough to
// drop a frame on an RGB panel. Type 's' on serial for that.
static const Known kKnown[] = {
  { 0x14, "GT911 touch alt",  false },
  { 0x23, "BH1750 lux",       false },
  { 0x24, "IO expander",      true  },
  { 0x38, "AHT20",            false },
  { 0x44, "SHT3x",            false },
  { 0x45, "SHT3x alt",        false },
  { 0x57, "AT24C32 eeprom",   false },
  { 0x5A, "DRV2605L haptic",  false },
  { 0x5C, "BH1750 alt",       false },
  { 0x5D, "GT911 touch",      true  },
  { 0x68, "DS3231 rtc",       false },
  { 0x76, "BMx280",           false },
  { 0x77, "BMx280 alt",       false },
};
static const int kKnownCount = sizeof(kKnown) / sizeof(kKnown[0]);

static bool present[kKnownCount];
static bool scanReady    = false;   // one pass has completed

// A probe is an address byte and a stop. Nothing is written, so no
// device changes state.
static bool probeAddr(uint8_t addr) {
  if (handle.bus == NULL) return false;
  return i2c_master_probe(handle.bus, addr, 50) == ESP_OK;
}

static void printTable() {
  Serial.println();
  Serial.println("=== I2C ===");
  for (int i = 0; i < kKnownCount; i++) {
    Serial.printf("  0x%02X  %-18s %-6s %s\n",
                  kKnown[i].addr,
                  kKnown[i].name,
                  present[i] ? "FOUND" : "--",
                  kKnown[i].required
                      ? (present[i] ? "(must be)" : "(MISSING - BUS FAULT)")
                      : "");
  }
  Serial.println();
}

// Full sweep. Runs only on request, only from the LVGL task.
static void fullSweep() {
  Serial.println();
  Serial.println("=== full sweep 0x08..0x77 ===");
  int n = 0;
  for (uint8_t a = 0x08; a <= 0x77; a++) {
    if (probeAddr(a)) { Serial.printf("  0x%02X\n", a); n++; }
  }
  Serial.printf("  %d device(s)\n\n", n);
}

// The scan no longer runs on a timer. It existed to feed a label on
// the home screen, and that label is gone; a scan writing to nobody
// would just take bus time once a second. It is now on demand from
// serial - i for the named table, s for a full sweep - and runs
// inside the LVGL task like everything else that touches the bus.
static bool scanWanted  = false;   // set from serial, served by sensorTick
static bool sweepWanted = false;

static void i2cScanNow(bool sweep) {
  if (sweep) fullSweep();
  for (int i = 0; i < kKnownCount; i++) present[i] = probeAddr(kKnown[i].addr);
  scanReady = true;
  printTable();
}

// ------------------------------------------------------------
// Device handles.
//
// One handle per sensor, created once. Creating a handle for a
// device that is not plugged in is harmless - it is only a config
// entry. The reads fail and the display shows "--".
//
// 0x24 and 0x5D are deliberately NOT added here. They already have
// handles owned by the IO expander and the touch driver.
// ------------------------------------------------------------
static i2c_master_dev_handle_t devRTC  = NULL;   // DS3231  0x68
static i2c_master_dev_handle_t devSHT  = NULL;   // SHT3x   0x44
static i2c_master_dev_handle_t devLUX  = NULL;   // BH1750  0x23

// ------------------------------------------------------------
// DS3231
// ------------------------------------------------------------
#define RTC_ADDR      0x68
#define RTC_REG_TIME  0x00
#define RTC_REG_STAT  0x0F   // bit 7 = oscillator stop flag
#define RTC_REG_TEMP  0x11

struct Clock {
  uint8_t  sec, min, hour, dow, day, month;
  uint16_t year;
  bool     valid;      // read succeeded
  bool     wasStopped; // oscillator stop flag was set
};
static Clock now = { 0, 0, 0, 1, 1, 1, 2000, false, true };
static float rtcTempC = 0.0f;
static bool  rtcTempOK = false;

static uint8_t bcd2dec(uint8_t v) { return (uint8_t)((v >> 4) * 10 + (v & 0x0F)); }
static uint8_t dec2bcd(uint8_t v) { return (uint8_t)(((v / 10) << 4) | (v % 10)); }

static bool rtcReadRegs(uint8_t reg, uint8_t *buf, size_t len) {
  if (!devRTC) return false;
  return i2c_master_transmit_receive(devRTC, &reg, 1, buf, len, 100) == ESP_OK;
}

static bool rtcWriteRegs(uint8_t reg, const uint8_t *data, size_t len) {
  if (!devRTC || len > 16) return false;
  uint8_t tx[17];
  tx[0] = reg;
  memcpy(tx + 1, data, len);
  return i2c_master_transmit(devRTC, tx, len + 1, 100) == ESP_OK;
}

static uint8_t dayOfWeek(uint16_t y, uint8_t m, uint8_t d);

static uint16_t rsTxSeq = 0;
static uint32_t rsTxGoodAck = 0;
static uint32_t rsTxTimeout = 0;
static uint32_t rsTxBadAck = 0;

static bool rs485WaitForTimeAck(uint16_t wantedSeq) {
#if PANEL_ID == 1
  const uint32_t startMs = millis();
  TimeFrame rx{};

  while (millis() - startMs < 400) {
    if (!rsReadFrame(rx)) {
      delay(1);
      continue;
    }

    if (rx.crc != rsFrameCrc(rx)) {
      ++rsTxBadAck;
      continue;
    }

    const uint16_t seq = rsFrameSeq(rx);
    if (rx.src == NODE_IRA &&
        rx.dst == NODE_SHEMI &&
        rx.type == TYPE_ACK &&
        seq == wantedSeq) {
      ++rsTxGoodAck;
      Serial.printf("[485] RX IRA TIME ACK seq=%u RTT=%lu ms good=%lu timeout=%lu bad=%lu\n",
                    wantedSeq,
                    (unsigned long)(millis() - startMs),
                    (unsigned long)rsTxGoodAck,
                    (unsigned long)rsTxTimeout,
                    (unsigned long)rsTxBadAck);
      return true;
    }
    ++rsTxBadAck;
  }

  ++rsTxTimeout;
  Serial.printf("[485] TIMEOUT waiting IRA TIME ACK seq=%u timeout=%lu bad=%lu\n",
                wantedSeq,
                (unsigned long)rsTxTimeout,
                (unsigned long)rsTxBadAck);
#endif
  return false;
}

static void rs485ClockSendTick() {
#if PANEL_ID == 1
  static uint32_t lastSendMs = 0;
  const uint32_t ms = millis();
  if (ms - lastSendMs < 1000) return;
  lastSendMs = ms;

  if (!now.valid || now.wasStopped) {
    Serial.println("[RTC] CLOCK NOT READY - nothing sent");
    return;
  }

  TimeFrame tx{};
  tx.start  = FRAME_START;
  tx.src    = NODE_SHEMI;
  tx.dst    = NODE_IRA;
  tx.type   = TYPE_TIME;
  tx.seqLo  = uint8_t(rsTxSeq & 0xFF);
  tx.seqHi  = uint8_t((rsTxSeq >> 8) & 0xFF);
  tx.yearLo = uint8_t(now.year & 0xFF);
  tx.yearHi = uint8_t((now.year >> 8) & 0xFF);
  tx.month  = now.month;
  tx.day    = now.day;
  tx.hour   = now.hour;
  tx.minute = now.min;
  tx.second = now.sec;
  tx.crc    = rsFrameCrc(tx);

  rsSendFrame(tx);
  Serial.printf("[RTC] %04u-%02u-%02u %02u:%02u:%02u -> [485] TX TIME seq=%u\n",
                now.year, now.month, now.day,
                now.hour, now.min, now.sec, rsTxSeq);

  rs485WaitForTimeAck(rsTxSeq);
  ++rsTxSeq;
#endif
}
static void rs485ClockPoll() {
#if PANEL_ID == 2
  TimeFrame rx{};

  while (rsReadFrame(rx)) {
    if (rx.crc != rsFrameCrc(rx)) {
      ++rsBadFrames;
      Serial.printf("[485] BAD CRC bad=%lu\n", (unsigned long)rsBadFrames);
      continue;
    }

    if (rx.dst != NODE_IRA && rx.dst != 0) continue;
    if (rx.src != NODE_SHEMI || rx.type != TYPE_TIME) continue;

    const uint16_t seq = rsFrameSeq(rx);

    if (!rsTimeFieldsValid(rx)) {
      ++rsBadFrames;
      Serial.printf("[485] INVALID TIME seq=%u bad=%lu\n",
                    seq, (unsigned long)rsBadFrames);
      continue;
    }

    now.year  = rsFrameYear(rx);
    now.month = rx.month;
    now.day   = rx.day;
    now.hour  = rx.hour;
    now.min   = rx.minute;
    now.sec   = rx.second;
    now.dow   = dayOfWeek(now.year, now.month, now.day);
    now.valid = true;
    now.wasStopped = false;

    ++rsGoodTimes;
    Serial.printf("[TIME] %04u-%02u-%02u %02u:%02u:%02u RX seq=%u good=%lu bad=%lu\n",
                  now.year, now.month, now.day,
                  now.hour, now.min, now.sec,
                  seq,
                  (unsigned long)rsGoodTimes,
                  (unsigned long)rsBadFrames);

    delay(2);
    TimeFrame ack = rsMakeAck(seq);
    rsSendFrame(ack);
    ++rsReplies;
    Serial.printf("[485] TX IRA TIME ACK seq=%u replies=%lu\n",
                  seq, (unsigned long)rsReplies);
  }
#endif
}

static void rtcRead() {
#if PANEL_ID == 2
  return;  // Ira clock comes from Shemi over RS485.
#endif
  uint8_t b[7];
  if (!rtcReadRegs(RTC_REG_TIME, b, 7)) { now.valid = false; return; }

  now.sec   = bcd2dec(b[0] & 0x7F);
  now.min   = bcd2dec(b[1] & 0x7F);
  // Bit 6 set means 12-hour mode. The setter always writes 24-hour,
  // but a module fresh out of a bag can arrive in 12-hour mode, so
  // it is handled rather than assumed.
  if (b[2] & 0x40) {
    uint8_t h = bcd2dec(b[2] & 0x1F);
    if (b[2] & 0x20) { if (h != 12) h = (uint8_t)(h + 12); }
    else             { if (h == 12) h = 0; }
    now.hour = h;
  } else {
    now.hour = bcd2dec(b[2] & 0x3F);
  }
  // Register 0x03 is deliberately IGNORED. It only changes when
  // somebody sets the clock, so a wrong value written once stays
  // wrong forever - which is exactly what happened here. The day of
  // week follows from the date with no ambiguity, so compute it.
  // (The register is still written by rtcSet, for anything else
  // that reads this chip.)
  now.day   = bcd2dec(b[4] & 0x3F);
  now.month = bcd2dec(b[5] & 0x1F);
  now.year  = (uint16_t)(2000 + bcd2dec(b[6]));
  now.dow   = dayOfWeek(now.year, now.month, now.day);
  now.valid = true;

  uint8_t st;
  if (rtcReadRegs(RTC_REG_STAT, &st, 1)) now.wasStopped = (st & 0x80) != 0;

  uint8_t t[2];
  if (rtcReadRegs(RTC_REG_TEMP, t, 2)) {
    rtcTempC  = (float)(int8_t)t[0] + ((t[1] >> 6) * 0.25f);
    rtcTempOK = true;
  }
}

// Zeller. The RTC keeps a day-of-week counter but has no idea what
// a calendar is, so it must be handed the right number.
static uint8_t dayOfWeek(uint16_t y, uint8_t m, uint8_t d) {
  if (m < 3) { m = (uint8_t)(m + 12); y = (uint16_t)(y - 1); }
  uint16_t k = y % 100, j = y / 100;
  int h = (d + (13 * (m + 1)) / 5 + k + k / 4 + j / 4 + 5 * j) % 7;
  // h is 0 Saturday, 1 Sunday ... 6 Friday. The DS3231 counter here
  // is 1 Sunday .. 7 Saturday, which is +6 mod 7, not +5. The +5 in
  // TEST 011 through 016 shifted every day back by one.
  return (uint8_t)(((h + 6) % 7) + 1);
}

static const char *kDow[8] = { "", "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };

static bool rtcSet(uint16_t y, uint8_t mo, uint8_t d,
                   uint8_t h, uint8_t mi, uint8_t se) {
  if (y < 2000 || y > 2099 || mo < 1 || mo > 12 || d < 1 || d > 31 ||
      h > 23 || mi > 59 || se > 59) return false;

  uint8_t b[7];
  b[0] = dec2bcd(se);
  b[1] = dec2bcd(mi);
  b[2] = dec2bcd(h);              // bit 6 clear = 24-hour
  b[3] = dayOfWeek(y, mo, d);
  b[4] = dec2bcd(d);
  b[5] = dec2bcd(mo);
  b[6] = dec2bcd((uint8_t)(y - 2000));
  if (!rtcWriteRegs(RTC_REG_TIME, b, 7)) return false;

  // Clear the oscillator stop flag. The time is now known good, so
  // the warning it carries has stopped being true.
  uint8_t st;
  if (rtcReadRegs(RTC_REG_STAT, &st, 1)) {
    st &= (uint8_t)~0x80;
    rtcWriteRegs(RTC_REG_STAT, &st, 1);
  }
  now.wasStopped = false;
  return true;
}

// ------------------------------------------------------------
// AT24C32 - the 4 KB EEPROM on the RTC module, address 0x57
//
// This is where every tuned value lives. It is not part of the
// ESP32, so reflashing the firmware does not touch it.
// ------------------------------------------------------------
#define EE_ADDR       0x57
#define EE_BASE       0x0000
#define EE_LEN        16        // one record, well inside one 32 byte page
#define EE_MAGIC0     0xB6
#define EE_MAGIC1     0x3D
#define EE_VERSION    1
#define EE_SETTLE_MS  3000UL    // quiet time before a write
#define EE_WRITE_MS   12        // ceiling on the write cycle poll

static i2c_master_dev_handle_t devEE = NULL;
static bool     cfgDirty      = false;
static uint32_t cfgTouchedAt  = 0;

static void cfgTouched() { cfgDirty = true; cfgTouchedAt = millis(); }

// After a page write the chip stops answering for about 5 ms while
// it burns. Poll until it acknowledges rather than guessing a delay
// that is either too short and corrupts, or too long and stalls.
static bool eeWaitReady() {
  // handle is a DEV_I2C_Port struct; the bus pointer is .bus inside
  // it. Same form probeAddr() has used since TEST 010.
  if (handle.bus == NULL) return false;
  uint32_t t0 = millis();
  while (millis() - t0 < EE_WRITE_MS) {
    if (i2c_master_probe(handle.bus, EE_ADDR, 5) == ESP_OK) return true;
    delay(1);
  }
  return false;
}

static uint8_t cfgSum(const uint8_t *b) {
  uint16_t sum = 0;
  for (int i = 0; i < EE_LEN - 1; i++) sum += b[i];
  return (uint8_t)(sum & 0xFF);
}

static bool cfgSave() {
  if (!devEE) return false;

  uint8_t b[EE_LEN];
  memset(b, 0, sizeof(b));
  b[0] = EE_MAGIC0;
  b[1] = EE_MAGIC1;
  b[2] = EE_VERSION;
  b[3] = (uint8_t)((autoBright ? 0x01 : 0) | (alarmOn   ? 0x02 : 0) |
                   (use12h     ? 0x04 : 0) | (alarmOnce ? 0x08 : 0));
  b[4] = blMin;
  b[5] = blMax;
  b[6] = autoSpeed;
  b[7] = hapDutyPct;
  b[8] = (uint8_t)(hapLenMs & 0xFF);
  b[9] = (uint8_t)(hapLenMs >> 8);
  b[10] = (uint8_t)(alarmMin & 0xFF);
  b[11] = (uint8_t)(alarmMin >> 8);
  // Byte 12: high nibble is the screensaver brightness in steps of
  // 4 percent. It was always zero before, and zero reads as "use
  // the default". The low nibble is the old five entry delay index,
  // kept only so an older record can still be translated.
  b[12] = (uint8_t)(((saverBright / 4) & 0x0F) << 4);
  b[13] = sleepMin;      // byte 13 was padding; 0 reads as the default
  // Byte 14: the new delay index PLUS ONE, so zero still means
  // "never written" and the fallback above can fire.
  b[14] = (uint8_t)(saverIdx + 1);
  b[EE_LEN - 1] = cfgSum(b);

  // Two address bytes, high then low. The AT24C02 on some other RTC
  // modules takes one; sending one byte here writes to the wrong
  // place and reads back nonsense.
  uint8_t tx[2 + EE_LEN];
  tx[0] = (uint8_t)(EE_BASE >> 8);
  tx[1] = (uint8_t)(EE_BASE & 0xFF);
  memcpy(tx + 2, b, EE_LEN);

  if (i2c_master_transmit(devEE, tx, sizeof(tx), 100) != ESP_OK) {
    Serial.println("[cfg] write failed");
    return false;
  }
  eeWaitReady();
  Serial.println("[cfg] saved");
  return true;
}

static void cfgLoad() {
  if (!devEE) { Serial.println("[cfg] no EEPROM - using defaults"); return; }

  uint8_t a[2] = { (uint8_t)(EE_BASE >> 8), (uint8_t)(EE_BASE & 0xFF) };
  uint8_t b[EE_LEN];
  if (i2c_master_transmit_receive(devEE, a, 2, b, EE_LEN, 100) != ESP_OK) {
    Serial.println("[cfg] read failed - using defaults");
    return;
  }

  // A blank EEPROM is all 0xFF and a changed layout makes old bytes
  // mean something else. Either loads as garbage, so all three
  // checks have to pass before a single value is believed.
  if (b[0] != EE_MAGIC0 || b[1] != EE_MAGIC1) {
    Serial.println("[cfg] empty - using defaults, will save on first change");
    return;
  }
  if (b[2] != EE_VERSION) {
    Serial.printf("[cfg] version %u, expected %u - using defaults\n",
                  b[2], EE_VERSION);
    return;
  }
  if (b[EE_LEN - 1] != cfgSum(b)) {
    Serial.println("[cfg] checksum bad - using defaults");
    return;
  }

  autoBright = (b[3] & 0x01) != 0;
  alarmOn    = (b[3] & 0x02) != 0;
  // Bit 2 was always zero before TEST 028, so an older record reads
  // as 24 hour - which is what it was. No version bump needed.
  use12h     = (b[3] & 0x04) != 0;
  alarmOnce  = (b[3] & 0x08) != 0;   // 0 = every day, as older records meant
  blMin      = b[4];
  blMax      = b[5];
  autoSpeed  = b[6];
  hapDutyPct = b[7];
  hapLenMs   = (uint16_t)(b[8] | (b[9] << 8));
  alarmMin   = (int)(b[10] | (b[11] << 8));
  if (b[14] >= 1 && b[14] <= 9) {
    saverIdx = (uint8_t)(b[14] - 1);
  } else {
    // Written before TEST 039. Translate the old five entry index
    // rather than throw the setting away.
    uint8_t old = b[12] & 0x0F;
    saverIdx = (old <= 4) ? kSaverLegacy[old] : 5;
  }
  saverAfterMs = kSaverOpts[saverIdx];

  { uint8_t sb = (uint8_t)(((b[12] >> 4) & 0x0F) * 4);
    saverBright = (sb >= 1 && sb <= 60) ? sb : 8; }

  sleepMin     = (b[13] >= 1 && b[13] <= 60) ? b[13] : 30;

  // Range checks even after the checksum passes. A good checksum
  // only proves the bytes are the ones that were written, not that
  // they were sensible when they were.
  if (blMin < 1   || blMin > 100)  blMin = 12;
  if (blMax < 1   || blMax > 100)  blMax = 100;
  if (blMax < blMin)               blMax = blMin;
  if (autoSpeed < AUTO_SPEED_MIN || autoSpeed > AUTO_SPEED_MAX) autoSpeed = 6;
  if (hapDutyPct > 100)            hapDutyPct = 100;
  if (hapLenMs < 20 || hapLenMs > 500) hapLenMs = 150;
  if (alarmMin < 0 || alarmMin > 1439) alarmMin = 7 * 60;

  Serial.printf("[cfg] loaded: auto %d  %u-%u%%  speed %u  "
                "haptic %u%%/%ums  alarm %02d:%02d %s\n",
                autoBright, blMin, blMax, autoSpeed, hapDutyPct, hapLenMs,
                alarmMin / 60, alarmMin % 60, alarmOn ? "on" : "off");
}

// Called once a second from sensorTick, which already runs in the
// LVGL task - the same task that owns the bus.
static void cfgTick() {
  if (!cfgDirty) return;
  if (millis() - cfgTouchedAt < EE_SETTLE_MS) return;
  cfgDirty = false;
  cfgSave();
}

// ------------------------------------------------------------
// SHT3x
//
// Periodic mode, and the result is fetched in ONE transaction:
// the fetch command and the six data bytes are joined by a
// repeated START, so no other transfer on the bus can land in the
// middle of it. TEST 012 split the command and the read across two
// separate ticks, and something in the gap was discarding the
// pending measurement.
//
// 0.5 measurements per second, high repeatability. Slow on purpose
// - the sensor warms itself while measuring, and a bedroom does not
// change temperature in one second.
// ------------------------------------------------------------
#define SHT_ADDR      0x44
#define SHT_CMD_BREAK 0x3093   // leave any periodic mode
#define SHT_CMD_RESET 0x30A2   // soft reset - defined, not used
#define SHT_CMD_PER   0x2032   // periodic, 0.5 mps, high repeatability
#define SHT_CMD_FETCH 0xE000   // fetch the last result

// SHT_STALE_MS must be comfortably longer than the measurement
// interval, or normal gaps look like faults. At 0.5 mps a new value
// arrives every 2 s, so 15 s is roughly seven missed measurements -
// long past coincidence.
#define SHT_STALE_MS 15000

static float shtTempC = 0.0f, shtRH = 0.0f;
static bool     shtOK = false;          // have a value worth showing
static uint32_t shtLastGood = 0;        // millis() of the last good fetch
static const char *shtMsg = "not started";

// CRC-8, polynomial 0x31, init 0xFF. Sensirion checksums every
// value; an unchecked reading off a cable is a guess.
static uint8_t shtCRC(const uint8_t *d, int len) {
  uint8_t c = 0xFF;
  for (int i = 0; i < len; i++) {
    c ^= d[i];
    for (int b = 0; b < 8; b++)
      c = (c & 0x80) ? (uint8_t)((c << 1) ^ 0x31) : (uint8_t)(c << 1);
  }
  return c;
}

static bool shtCmd(uint16_t cmd) {
  if (!devSHT) return false;
  uint8_t b[2] = { (uint8_t)(cmd >> 8), (uint8_t)(cmd & 0xFF) };
  return i2c_master_transmit(devSHT, b, 2, 100) == ESP_OK;
}

// Called at boot and again after repeated failures. Break first:
// a sensor already in periodic mode from a previous run rejects a
// new mode command until it is stopped.
static void shtBegin() {
  if (!devSHT) { shtMsg = "no handle"; return; }
  shtLastGood = millis();
  shtCmd(SHT_CMD_BREAK);
  delay(2);
  bool ok = shtCmd(SHT_CMD_PER);
  shtMsg = ok ? "armed" : "arm failed";
}

static void shtRead() {
  if (!devSHT) { shtOK = false; shtMsg = "no handle"; return; }

  uint8_t cmd[2] = { (uint8_t)(SHT_CMD_FETCH >> 8),
                     (uint8_t)(SHT_CMD_FETCH & 0xFF) };
  uint8_t b[6];

  // One transaction. Repeated START between the command and the
  // read, so nothing can get in between.
  if (i2c_master_transmit_receive(devSHT, cmd, 2, b, 6, 100) != ESP_OK) {
    // NOT A FAILURE. In periodic mode a fetch with no new
    // measurement ready is deliberately not acknowledged. Keep the
    // last value and say nothing.
    if (shtOK && (millis() - shtLastGood) > SHT_STALE_MS) {
      shtOK  = false;
      shtMsg = "stale - rearming";
      shtBegin();
    } else if (!shtOK && (millis() - shtLastGood) > SHT_STALE_MS) {
      shtMsg = "no data - rearming";
      shtLastGood = millis();     // do not rearm again for another window
      shtBegin();
    }
    return;
  }

  if (shtCRC(b, 2) != b[2] || shtCRC(b + 3, 2) != b[5]) {
    // This one IS a fault. The sensor answered and the bytes are
    // corrupt, which is electrical: cable length, pull-ups, noise.
    shtMsg = "CRC bad";
    return;
  }

  uint16_t rawT = (uint16_t)((b[0] << 8) | b[1]);
  uint16_t rawH = (uint16_t)((b[3] << 8) | b[4]);
  shtTempC = -45.0f + 175.0f * ((float)rawT / 65535.0f);
  shtRH    = 100.0f * ((float)rawH / 65535.0f);
  shtOK       = true;
  shtMsg      = "ok";
  shtLastGood = millis();
}

// ------------------------------------------------------------
// BH1750 - not answering yet, driver ready for when it is
// ------------------------------------------------------------
#define LUX_ADDR 0x23

static float luxVal = 0.0f;
static bool  luxOK = false;
static bool  luxStarted = false;

static void luxBegin() {
  if (!devLUX) return;
  const uint8_t cmd = 0x10;   // continuous, high resolution, ~120 ms
  luxStarted = (i2c_master_transmit(devLUX, &cmd, 1, 100) == ESP_OK);
}

static void luxRead() {
  if (!devLUX) { luxOK = false; return; }
  if (!luxStarted) { luxBegin(); if (!luxStarted) { luxOK = false; return; } }
  uint8_t b[2];
  if (i2c_master_receive(devLUX, b, 2, 100) != ESP_OK) {
    luxOK = false; luxStarted = false; return;   // retry the mode command later
  }
  luxVal = (float)((b[0] << 8) | b[1]) / 1.2f;
  luxOK  = true;
}

static lv_obj_t *tileBedImg   = NULL;
static lv_obj_t *tileRadioImg = NULL;
static lv_obj_t *tileACImg    = NULL;
static lv_obj_t *tileShadeImg = NULL;
static lv_obj_t *tileLampImg  = NULL;
static lv_obj_t *tileFanImg   = NULL;

// Each photographic tile is tracked as a button plus its image plus
// a pointer to the flag that says whether it is lit, so one function
// can repaint all of them.
struct Tile {
  lv_obj_t *btn;
  lv_obj_t *img;
  bool     *state;
};
static Tile  tiles[8];
static int   tileCount = 0;

// Panel-side state. Nothing reports back yet, so this is the only
// truth the screen has - exactly as on the old panel.
static bool bedRunning  = false;
static bool radioOn     = false;
static bool acPower     = false;
static int  acTemp      = 24;
static bool shadeOpen   = true;
static bool lampOn      = false;
static bool fanOn       = false;
static int  fanSpeed    = 0;
// What the short press returns to. 2 is the middle speed, used only
// if the fan has never been set this session.
static int  lastFanSpeed = 0;

// ------------------------------------------------------------
// Defined below setup(), declared here so setup() can call them.
static void applyBacklight(uint8_t v);
static void applyBacklightQuiet(uint8_t v);
static void printHelp();
static void refreshHomeTiles();
static void refreshAC();
static void refreshFan();
static void buildSettings();
static void buildMassage();
static void buildRadio();
static void buildAC();
static void buildFan();
static void buildAlarm();
static void lvMem(const char *what);
static void autoBrightTick();
static void saverTick();
static void sleepTick();
static void massTick();
static void saverEnter();
static void fastTick(lv_timer_t *t);
static void cfgTouched();
static void cfgTick();
static void alarmTick();
static void refreshAlarm();
static void sensorTick(lv_timer_t *t);

static void say(const char *what) {
  Serial.printf("[tile] %s\n", what);
  if (lblStatus) lv_label_set_text(lblStatus, what);
}

// paintTile() is defined further down, next to the other drawing
// helpers, so it is declared here for the event handlers above it.
static void paintTile(const struct Tile &t);

// Find the tile that owns an image object, so a handler can repaint
// just its own tile without a second lookup table.
static void repaintOwner(lv_obj_t *img) {
  for (int i = 0; i < tileCount; i++)
    if (tiles[i].img == img) { paintTile(tiles[i]); return; }
}

static void evBed(lv_event_t *e) {
  if (!scrMassage) { buildMassage(); lvMem("massage"); }          // built the first time it is opened
  if (scrMassage) lv_scr_load(scrMassage); else say("massage screen failed to build");
}

static void evRadio(lv_event_t *e) {
  if (!scrRadio) { buildRadio(); lvMem("radio"); }          // built the first time it is opened
  if (scrRadio) lv_scr_load(scrRadio); else say("radio screen failed to build");
}

// Fan and AC tiles: a short press flips the power and stays here, a
// 2 second hold opens the detail screen.
//
// THE HOLD IS TIMED HERE. LV_EVENT_LONG_PRESSED depends on the
// input driver's long_press_time, which lives inside Waveshare's
// port; TEST 046 set it blind at boot and the long press never
// fired. millis() on press and on release is exact and depends on
// nothing.
#define TILE_HOLD_MS 2000

static uint32_t tilePressMs = 0;

static void evTilePress(lv_event_t *e) { tilePressMs = millis(); }

// True if this was a hold. Also prints the measured time, so a
// threshold that turns out wrong can be changed against real
// numbers instead of guesses.
static bool tileWasHeld(const char *what) {
  uint32_t held = millis() - tilePressMs;
  Serial.printf("[tile] %s held %lu ms\n", what, (unsigned long)held);
  Serial.flush();
  return (tilePressMs != 0) && (held >= TILE_HOLD_MS);
}

static void evACRelease(lv_event_t *e) {
  bool hold = tileWasHeld("AC");
  tilePressMs = 0;
  if (hold) {
    if (!scrAC) { buildAC(); lvMem("AC"); }
    if (scrAC) lv_scr_load(scrAC); else say("AC screen failed to build");
    return;
  }
  acPower = !acPower;
  refreshAC();
  refreshHomeTiles();
  say(acPower ? "AC on" : "AC off");
}

// Slide off the tile and nothing happens - the standard way out of
// a press you did not mean.
static void evTileLost(lv_event_t *e) { tilePressMs = 0; }

static void evShade(lv_event_t *e) {
  shadeOpen = !shadeOpen;
  lv_img_set_src(tileShadeImg, shadeOpen ? &img_shade_open : &img_shade_closed);
  repaintOwner(tileShadeImg);
  say(shadeOpen ? "shutters open" : "shutters closed");
}

static void evLamp(lv_event_t *e) {
  lampOn = !lampOn;
  // The lamp has one photograph and two colours, applied once on
  // press. Nothing repaints on a timer - that was the old jitter.
  // The lamp is tinted rather than blackened - amber when lit, cold
  // grey when not - so it needs its own recolour, then the shared
  // opacity and background rule on top.
  lv_obj_set_style_img_recolor(tileLampImg,
      lv_color_hex(lampOn ? 0xE8A030 : 0x9098A0), 0);
  repaintOwner(tileLampImg);
  say(lampOn ? "lamp on" : "lamp off");
}

static void evFanRelease(lv_event_t *e) {
  bool hold = tileWasHeld("fan");
  tilePressMs = 0;
  if (hold) {
    if (!scrFan) { buildFan(); lvMem("fan"); }
    if (scrFan) lv_scr_load(scrFan); else say("fan screen failed to build");
    return;
  }
  if (fanSpeed > 0) { lastFanSpeed = fanSpeed; fanSpeed = 0; }
  else              { fanSpeed = (lastFanSpeed > 0) ? lastFanSpeed : 2; }
  fanOn = (fanSpeed > 0);
  refreshFan();
  refreshHomeTiles();
  say(fanOn ? "fan on" : "fan off");
}

// evFanLong went with the LVGL long-press approach; evFanRelease
// above does the same job from its own timing.

static void evSettings(lv_event_t *e) {
  if (!scrSettings) { buildSettings(); lvMem("settings"); }
  if (scrSettings) lv_scr_load(scrSettings);
  else             say("settings screen failed to build");
}
static void evSpare(lv_event_t *e) {
  if (!scrAlarm) { buildAlarm(); lvMem("alarm"); }
  if (scrAlarm) lv_scr_load(scrAlarm); else say("alarm screen failed to build");
}

// ------------------------------------------------------------

static int tileX(int c) { return TILE_X0 + c * (TILE_SZ + TILE_GAPX); }
static int tileY(int r) { return TILE_Y0 + r * (TILE_SZ + TILE_GAPY); }

// ------------------------------------------------------------
// TOUCH FEEDBACK
//
// Coin ERM motor module on GPIO 6. IN to GPIO 6, VCC to the same
// 5 V that feeds the board, GND common.
//
// Numbers that came from a meter, not from a listing:
//   22 mA at 3.3 V spinning
//   36 mA at 5 V spinning
//   under 1 mA into IN, so the gate is a MOSFET
//   535 mV across the motor pads in diode mode - flyback present
// ------------------------------------------------------------
#define HAPTIC_PIN       6
#define HAPTIC_FREQ      20000   // above hearing - this sits on a headboard
#define HAPTIC_BITS      8
// The two tunables live further up, beside the brightness ones,
// because the EEPROM code above needs to see all of them.

static lv_timer_t *hapTimer = NULL;
static bool        hapReady = false;

static inline uint32_t hapDuty() {
  return ((1u << HAPTIC_BITS) - 1) * hapDutyPct / 100;
}

// Ends the tick. The timer parks itself - nothing runs between
// touches.
static void hapTick(lv_timer_t *t) {
  ledcWrite(HAPTIC_PIN, 0);
  lv_timer_pause(t);
}

// Retriggering during a pulse restarts it rather than stacking
// timers, so a fast series of taps cannot leave the motor latched
// on.
static void hapticFire(uint8_t pct, uint16_t ms) {
  if (!hapReady || !hapTimer) return;
  if (pct > 100) pct = 100;
  ledcWrite(HAPTIC_PIN, ((1u << HAPTIC_BITS) - 1) * pct / 100);
  lv_timer_set_period(hapTimer, ms);
  lv_timer_reset(hapTimer);
  lv_timer_resume(hapTimer);
}

static void hapticPulse() { hapticFire(hapDutyPct, hapLenMs); }

static void hapticInit() {
  hapReady = ledcAttach(HAPTIC_PIN, HAPTIC_FREQ, HAPTIC_BITS);
  if (hapReady) ledcWrite(HAPTIC_PIN, 0);
  hapTimer = lv_timer_create(hapTick, hapLenMs, NULL);
  lv_timer_pause(hapTimer);
  Serial.printf("haptic: GPIO %d  %s  %u%% duty  %u ms\n",
                HAPTIC_PIN, hapReady ? "attached" : "ATTACH FAILED",
                hapDutyPct, hapLenMs);
}

// One handler for the entire screen. Tiles bubble their events up
// to it; bare background reaches it directly. Any tile added later
// is covered with no change here.
static void evAnyPress(lv_event_t *e) {
  (void)e;
  lastTouchMs = millis();       // any touch anywhere postpones the screensaver
  hapticPulse();
}

// ------------------------------------------------------------

static lv_obj_t *makeTile(int col, int row, uint32_t bg) {
  lv_obj_t *b = lv_btn_create(scrHome);
  lv_obj_set_size(b, TILE_SZ, TILE_SZ);
  lv_obj_set_pos(b, tileX(col), tileY(row));
  lv_obj_set_style_bg_color(b, lv_color_hex(bg), 0);
  lv_obj_set_style_radius(b, 16, 0);
  lv_obj_set_style_pad_all(b, 0, 0);
  lv_obj_set_style_shadow_width(b, 0, 0);
  lv_obj_clear_flag(b, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(b, LV_OBJ_FLAG_EVENT_BUBBLE);   // press reaches scrHome
  return b;
}

static lv_obj_t *addImg(lv_obj_t *parent, const lv_img_dsc_t *src) {
  lv_obj_t *im = lv_img_create(parent);
  lv_img_set_src(im, src);
  lv_obj_center(im);
  lv_obj_set_style_img_recolor(im, lv_color_black(), 0);
  lv_obj_set_style_img_recolor_opa(im, dimOff, 0);
  return im;
}

// Paint one tile according to its own state. Both the artwork and
// the panel behind it move together.
static void paintTile(const struct Tile &t) {
  bool on = t.state ? *t.state : false;
  lv_obj_set_style_img_recolor_opa(t.img, on ? dimOn : dimOff, 0);
  lv_obj_set_style_bg_color(t.btn,
      lv_color_hex(on ? TILE_BG_ON : TILE_BG_OFF), 0);
}

// Repaint everything. Called from the serial handler, so it takes
// the LVGL mutex itself.
static void repaintAll() {
  if (!lvgl_port_lock(-1)) return;
  for (int i = 0; i < tileCount; i++) paintTile(tiles[i]);
  lvgl_port_unlock();
}

static void addSymbol(lv_obj_t *parent, const char *sym, uint32_t colour) {
  lv_obj_t *l = lv_label_create(parent);
  lv_label_set_text(l, sym);
  lv_obj_set_style_text_font(l, &lv_font_montserrat_26, 0);
  lv_obj_set_style_text_color(l, lv_color_hex(colour), 0);
  lv_obj_center(l);
}

// ------------------------------------------------------------
// One second tick. Everything here touches I2C, so like the
// presence scan it lives in the LVGL task and nowhere else.
//
// The SHT3x is measured every fifth tick and read on the sixth, so
// the sensor is idle most of the time and never heats itself into
// the reading.
// ------------------------------------------------------------
static void sensorTick(lv_timer_t *t) {
  (void)t;
  static uint8_t phase = 0;

  rtcRead();

  shtRead();
  phase++;

  // luxRead and autoBrightTick moved to their own 100 ms timer -
  // once a second is far too coarse to follow a room.
  alarmTick();
  saverTick();
  sleepTick();
  massTick();
  cfgTick();

  if (sweepWanted)     { sweepWanted = false; i2cScanNow(true);  }
  else if (scanWanted) { scanWanted  = false; i2cScanNow(false); }

  char c[48];
  if (now.valid && !now.wasStopped) {
    if (use12h) {
      const char *sfx;
      uint8_t h12 = to12(now.hour, &sfx);
      snprintf(c, sizeof(c), "%u:%02u %s   %s %02u/%02u/%02u",
               h12, now.min, sfx,
               kDow[now.dow <= 7 ? now.dow : 0], now.day, now.month,
               (unsigned)(now.year % 100));
    } else {
      snprintf(c, sizeof(c), "%02u:%02u   %s %02u/%02u/%02u",
               now.hour, now.min,
               kDow[now.dow <= 7 ? now.dow : 0], now.day, now.month,
               (unsigned)(now.year % 100));
    }
  } else if (now.valid) {
    snprintf(c, sizeof(c), "CLOCK NOT SET");
  } else {
    snprintf(c, sizeof(c), "NO RTC");
  }
  if (lblClock) {
    lv_label_set_text(lblClock, c);
    lv_obj_set_style_text_color(lblClock,
        lv_color_hex((now.valid && !now.wasStopped) ? 0xFFFFFF : 0xD05050), 0);
  }

  // \xC2\xB0 is the degree sign. Verified present: the built-in
  // montserrat fonts here are built with -r 0x20-0x7F,0xB0.
  char m[64];
  int o = 0;
  if (shtOK) o += snprintf(m + o, sizeof(m) - o,
                           "%.1f \xC2\xB0" "C   %.0f %%RH", shtTempC, shtRH);
  else       o += snprintf(m + o, sizeof(m) - o, "-- \xC2\xB0" "C   -- %%RH");
  if (luxOK) snprintf(m + o, sizeof(m) - o, "   %.0f lx", luxVal);
  if (lblClimate) lv_label_set_text(lblClimate, m);

  // The settings screen is built once and kept, so these exist even
  // while the home screen is showing. Updating them costs two label
  // writes and means the screen is already correct when it appears.
  if (lblNowTime) {
    if (now.valid && !now.wasStopped)
      if (use12h) {
        const char *sfx;
        uint8_t h12 = to12(now.hour, &sfx);
        lv_label_set_text_fmt(lblNowTime, "now  %u:%02u %s", h12, now.min, sfx);
      } else {
        lv_label_set_text_fmt(lblNowTime, "now  %02u:%02u", now.hour, now.min);
      }
    else
      lv_label_set_text(lblNowTime, "clock not set");
  }
  if (lblLuxLive) {
    // NOT lv_label_set_text_fmt: LVGL's printf has
    // LV_SPRINTF_USE_FLOAT 0 and throws %f away without a word.
    if (luxOK) {
      char t[32]; snprintf(t, sizeof(t), "room light  %.0f lx", luxVal);
      lv_label_set_text(lblLuxLive, t);
    } else lv_label_set_text(lblLuxLive, "room light  -- lx");
  }
  if (lblAcRoom) {
    if (shtOK) {
      char t[24]; snprintf(t, sizeof(t), "%.1f \xC2\xB0" "C", shtTempC);
      lv_label_set_text(lblAcRoom, t);
    } else lv_label_set_text(lblAcRoom, "--");
  }

  // Serial once a minute, so the monitor stays usable for tuning.
  static uint8_t lastMin = 255;
  if (now.valid && now.min != lastMin) {
    lastMin = now.min;
    if (shtOK) {
      Serial.printf("[%02u:%02u:%02u] sht %.2f C  %.1f %%RH | rtc %.2f C | lux %s%s\n",
                    now.hour, now.min, now.sec, shtTempC, shtRH,
                    rtcTempOK ? rtcTempC : 0.0f,
                    luxOK ? String(luxVal, 0).c_str() : "--",
                    now.wasStopped ? "  [CLOCK NOT SET]" : "");
    } else {
      // Naming the failure. A NACK and a bad checksum are different
      // problems and printed the same before.
      Serial.printf("[%02u:%02u:%02u] sht -- (%s) | rtc %.2f C | lux %s%s\n",
                    now.hour, now.min, now.sec, shtMsg,
                    rtcTempOK ? rtcTempC : 0.0f,
                    luxOK ? String(luxVal, 0).c_str() : "--",
                    now.wasStopped ? "  [CLOCK NOT SET]" : "");
    }
  }
}

// ------------------------------------------------------------

// ============================================================
// SETTINGS SCREEN
//
// Ported from buildSettings() in TEST 081, with three forced
// changes:
//   fonts - 26 is the largest this lv_conf.h builds
//   24 hour - one representation, no AM/PM conversion
//   1024x600 - every coordinate from the 800x480 original moves
// ============================================================
#define SET_PAGE_BG 0x1B1E2A
#define SET_BAR_BG  0x141621
#define SET_TEXT    0xFFFFFF
#define SET_DIM     0x8A94A0
#define SET_GREEN   0x209040
// The back arrow was the same grey as everything else. Amber, on
// every screen, so it is the one thing that always stands out.
#define BACK_COL    0x1F7A8C   // teal; the amber went in TEST 050

// Working copy of the date and time being edited. Deliberately
// separate from `now`, which is what the chip says - so a half
// finished edit never leaks into the displayed clock.
static uint8_t  setH = 12, setMi = 0, setD = 1, setMo = 1;
static uint16_t setY = 2026;

static lv_obj_t *lblSetH = NULL, *lblSetMi = NULL;
static lv_obj_t *lblSetD = NULL, *lblSetMo = NULL, *lblSetY = NULL;
static lv_obj_t *lblSetStatus = NULL;
static lv_obj_t *lblBlVal = NULL;                 // brightness in use now
static lv_obj_t *sldBlMin = NULL, *lblBlMin = NULL;
static lv_obj_t *sldBlMax = NULL, *lblBlMax = NULL;
static lv_obj_t *swAuto   = NULL;
static lv_obj_t *sw12h    = NULL;
static lv_obj_t *sldSpeed = NULL, *lblAutoSpeed = NULL;
static lv_obj_t *sldSaverAfter = NULL, *lblSaverAfter = NULL;
static lv_obj_t *sldSaverBright = NULL, *lblSaverBright = NULL;
static lv_obj_t *sldHapV = NULL, *lblHapV = NULL;
static lv_obj_t *sldHapH = NULL, *lblHapH = NULL;

static uint8_t daysInMonth(uint8_t m, uint16_t y) {
  static const uint8_t d[13] = { 0,31,28,31,30,31,30,31,31,30,31,30,31 };
  if (m == 2 && ((y % 4 == 0 && y % 100 != 0) || y % 400 == 0)) return 29;
  return (m >= 1 && m <= 12) ? d[m] : 31;
}

static void refreshSetLabels() {
  // Clamp the day AFTER a month or year change: stepping from 31
  // March to February would otherwise leave an impossible date that
  // the RTC would accept and then report back as nonsense.
  uint8_t dim = daysInMonth(setMo, setY);
  if (setD > dim) setD = dim;

  if (lblSetH) {
    if (use12h) {
      const char *sfx;
      uint8_t h12 = to12(setH, &sfx);
      lv_label_set_text_fmt(lblSetH, "%u %s", h12, sfx);
    } else {
      lv_label_set_text_fmt(lblSetH, "%02u", setH);
    }
  }
  if (lblSetMi) lv_label_set_text_fmt(lblSetMi, "%02u", setMi);
  if (lblSetD)  lv_label_set_text_fmt(lblSetD,  "%u",   setD);
  if (lblSetMo) lv_label_set_text_fmt(lblSetMo, "%u",   setMo);
  if (lblSetY)  lv_label_set_text_fmt(lblSetY,  "%u",   setY);
}

// Pull the fields from the chip. Called on every entry to the
// screen, not once at boot - otherwise a later visit shows stale
// numbers and Set would move a correct clock backwards.
static void seedFromRTC() {
  if (now.valid && !now.wasStopped) {
    setH = now.hour; setMi = now.min;
    setD = now.day;  setMo = now.month; setY = now.year;
  }
  refreshSetLabels();
  if (lblSetStatus) lv_label_set_text(lblSetStatus, "");
}

static void evSetHUp (lv_event_t *e) { setH  = (uint8_t)((setH + 1) % 24); refreshSetLabels(); }
static void evSetHDn (lv_event_t *e) { setH  = (uint8_t)((setH + 23) % 24); refreshSetLabels(); }
static void evSetMiUp(lv_event_t *e) { setMi = (uint8_t)((setMi + 1) % 60); refreshSetLabels(); }
static void evSetMiDn(lv_event_t *e) { setMi = (uint8_t)((setMi + 59) % 60); refreshSetLabels(); }
static void evSetDUp (lv_event_t *e) { setD  = (uint8_t)(setD  % daysInMonth(setMo, setY) + 1); refreshSetLabels(); }
static void evSetDDn (lv_event_t *e) { setD  = (uint8_t)(setD <= 1 ? daysInMonth(setMo, setY) : setD - 1); refreshSetLabels(); }
static void evSetMoUp(lv_event_t *e) { setMo = (uint8_t)(setMo % 12 + 1); refreshSetLabels(); }
static void evSetMoDn(lv_event_t *e) { setMo = (uint8_t)(setMo <= 1 ? 12 : setMo - 1); refreshSetLabels(); }
// 2000-2099: the DS3231 stores two digits plus a century bit this
// driver does not use, so anything outside cannot round trip.
static void evSetYUp (lv_event_t *e) { if (setY < 2099) setY++; refreshSetLabels(); }
static void evSetYDn (lv_event_t *e) { if (setY > 2000) setY--; refreshSetLabels(); }

static void ev12h(lv_event_t *e) {
  use12h = lv_obj_has_state(lv_event_get_target(e), LV_STATE_CHECKED);
  cfgTouched();
  refreshSetLabels();
  refreshAlarm();
  Serial.printf("clock display: %s\n", use12h ? "12 hour" : "24 hour");
}

static void evDoSetTime(lv_event_t *e) {
  bool ok = rtcSet(setY, setMo, setD, setH, setMi, 0);
  if (lblSetStatus) {
    lv_label_set_text(lblSetStatus, ok ? "clock set" : "write failed");
    lv_obj_set_style_text_color(lblSetStatus,
        lv_color_hex(ok ? SET_GREEN : 0xD05050), 0);
  }
  Serial.printf("set from screen: %04u-%02u-%02u %02u:%02u  %s\n",
                setY, setMo, setD, setH, setMi, ok ? "ok" : "FAILED");
  if (ok) rtcRead();
}

// With Auto on, Max is the top of the mapped range. With Auto off
// it IS the brightness. One slider, two jobs, so there is no third
// slider and no ambiguity about which one is in charge.
static void evBlMax(lv_event_t *e) {
  blMax = (uint8_t)lv_slider_get_value(lv_event_get_target(e));
  cfgTouched();
  if (blMax < blMin) blMax = blMin;
  if (lblBlMax) lv_label_set_text_fmt(lblBlMax, "%u%%", blMax);
  if (!autoBright) applyBacklightQuiet(blMax);
}

static void evBlMin(lv_event_t *e) {
  blMin = (uint8_t)lv_slider_get_value(lv_event_get_target(e));
  cfgTouched();
  if (blMin > blMax) blMin = blMax;
  if (lblBlMin) lv_label_set_text_fmt(lblBlMin, "%u%%", blMin);
}

static void evSaverAfter(lv_event_t *e) {
  int i = lv_slider_get_value(lv_event_get_target(e));
  if (i < 0) i = 0;
  if (i > 8) i = 8;
  saverIdx     = (uint8_t)i;
  saverAfterMs = kSaverOpts[saverIdx];
  lastTouchMs  = millis();
  cfgTouched();
  if (lblSaverAfter) lv_label_set_text(lblSaverAfter, kSaverNames[saverIdx]);
  Serial.printf("screensaver after %s\n", kSaverNames[saverIdx]);
}

static void evSaverBright(lv_event_t *e) {
  saverBright = (uint8_t)lv_slider_get_value(lv_event_get_target(e));
  cfgTouched();
  if (lblSaverBright) lv_label_set_text_fmt(lblSaverBright, "%u%%", saverBright);
  // If the screensaver is showing, let the change be felt at once -
  // otherwise you are choosing a night brightness blind.
  if (saverOn) applyBacklightQuiet(saverBright);
}

static void evAutoSpeed(lv_event_t *e) {
  autoSpeed = (uint8_t)lv_slider_get_value(lv_event_get_target(e));
  cfgTouched();
  if (lblAutoSpeed) lv_label_set_text_fmt(lblAutoSpeed, "%u", autoSpeed);
  Serial.printf("auto brightness speed %u\n", autoSpeed);
}

static void evAutoBright(lv_event_t *e) {
  autoBright = lv_obj_has_state(lv_event_get_target(e), LV_STATE_CHECKED);
  cfgTouched();
  Serial.printf("auto brightness %s\n", autoBright ? "on" : "off");
  if (!autoBright) applyBacklight(blMax);
  else luxSmooth = -1.0f;      // reseed, so it does not crawl from stale data
}

// The number follows the finger; the vibration waits for it to
// lift. VALUE_CHANGED fires dozens of times during a drag, and a
// pulse on each one is a continuous buzz, not feedback.
static void evHapVChg(lv_event_t *e) {
  hapDutyPct = (uint8_t)lv_slider_get_value(lv_event_get_target(e));
  cfgTouched();
  if (lblHapV) lv_label_set_text_fmt(lblHapV, "%u%%", hapDutyPct);
}
static void evHapHChg(lv_event_t *e) {
  hapLenMs = (uint16_t)lv_slider_get_value(lv_event_get_target(e));
  cfgTouched();
  if (lblHapH) lv_label_set_text_fmt(lblHapH, "%u ms", hapLenMs);
}
static void evHapTry(lv_event_t *e) {
  hapticPulse();
  Serial.printf("haptic now %u%%  %u ms\n", hapDutyPct, hapLenMs);
}

static void evGoHome(lv_event_t *e) {
  refreshHomeTiles();
  lv_scr_load(scrHome);
}

static void evSettingsShown(lv_event_t *e) { seedFromRTC(); }

// Every button this file makes bubbles its events to the screen, so
// the one haptic handler covers them all.
static lv_obj_t *setBtn(lv_obj_t *parent, int w, int h, const char *txt,
                        lv_event_cb_t cb, uint32_t bg) {
  lv_obj_t *b = lv_btn_create(parent);
  lv_obj_set_size(b, w, h);
  lv_obj_set_style_bg_color(b, lv_color_hex(bg), 0);
  lv_obj_add_flag(b, LV_OBJ_FLAG_EVENT_BUBBLE);
  lv_obj_add_event_cb(b, cb, LV_EVENT_CLICKED, NULL);
  lv_obj_t *l = lv_label_create(b);
  lv_label_set_text(l, txt);
  lv_obj_set_style_text_font(l, &lv_font_montserrat_26, 0);
  lv_obj_center(l);
  return b;
}

static lv_obj_t *setBtnU(lv_obj_t *parent, int w, int h, const char *txt,
                         lv_event_cb_t cb, uint32_t bg, int val) {
  lv_obj_t *b = lv_btn_create(parent);
  lv_obj_set_size(b, w, h);
  lv_obj_set_style_bg_color(b, lv_color_hex(bg), 0);
  lv_obj_set_style_radius(b, 10, 0);
  lv_obj_set_style_shadow_width(b, 0, 0);
  lv_obj_add_flag(b, LV_OBJ_FLAG_EVENT_BUBBLE);
  lv_obj_add_event_cb(b, cb, LV_EVENT_CLICKED, (void *)(intptr_t)val);
  lv_obj_t *l = lv_label_create(b);
  lv_label_set_text(l, txt);
  lv_obj_set_style_text_font(l, &lv_font_montserrat_16, 0);
  lv_obj_center(l);
  return b;
}

// A switch you can read across a room and cannot drag.
//
// It shows its state by POSITION, like the switch on a wall: knob
// left and grey track when off, knob right and green track when on,
// the word on the empty side. LV_OBJ_FLAG_CHECKABLE makes any press
// flip it - the knob does not drag. The handlers are unchanged: a
// checkable button carries exactly the state lv_switch did.
static void toggleFace(lv_obj_t *b) {
  bool on = lv_obj_has_state(b, LV_STATE_CHECKED);
  lv_obj_t *l = lv_obj_get_child(b, 0);     // the word
  lv_obj_t *k = lv_obj_get_child(b, 1);     // the knob
  if (l) {
    // OFF says nothing - the knob's position says it alone.
    lv_label_set_text(l, on ? "ON" : "");
    lv_obj_set_style_text_color(l, lv_color_hex(0x06281A), 0);
    lv_obj_align(l, LV_ALIGN_LEFT_MID, 24, 0);
  }
  if (k) {
    // To the ENDS of the pill, not near them.
    lv_obj_align(k, on ? LV_ALIGN_RIGHT_MID : LV_ALIGN_LEFT_MID,
                 on ? -5 : 5, 0);
    lv_obj_set_style_bg_color(k, lv_color_hex(on ? 0xFFFFFF : 0x8A94A2), 0);
  }
}

static void evToggleFace(lv_event_t *e) { toggleFace(lv_event_get_target(e)); }

static lv_obj_t *makeToggle(lv_obj_t *par, lv_event_cb_t cb, bool on) {
  lv_obj_t *b = lv_btn_create(par);
  lv_obj_set_size(b, 210, 64);                     // long, so the knob TRAVELS
  lv_obj_set_style_radius(b, 32, 0);               // a pill, not a button
  lv_obj_set_style_shadow_width(b, 0, 0);
  lv_obj_set_style_bg_color(b, lv_color_hex(0x2A3040), 0);
  lv_obj_set_style_bg_color(b, lv_color_hex(0x1FA85A), LV_STATE_CHECKED);
  lv_obj_set_style_border_width(b, 2, 0);
  lv_obj_set_style_border_color(b, lv_color_hex(0x4A5262), 0);
  lv_obj_set_style_border_color(b, lv_color_hex(0x9BF0BE), LV_STATE_CHECKED);
  lv_obj_add_flag(b, LV_OBJ_FLAG_CHECKABLE);
  lv_obj_add_flag(b, LV_OBJ_FLAG_EVENT_BUBBLE);

  // Child 0: the word. Child 1: the knob. toggleFace counts on this
  // order.
  lv_obj_t *l = lv_label_create(b);
  lv_label_set_text(l, "OFF");
  lv_obj_set_style_text_font(l, &lv_font_montserrat_26, 0);

  lv_obj_t *k = lv_obj_create(b);
  lv_obj_remove_style_all(k);
  lv_obj_set_size(k, 46, 46);
  lv_obj_set_style_radius(k, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_bg_opa(k, LV_OPA_COVER, 0);
  lv_obj_set_style_shadow_width(k, 6, 0);
  lv_obj_set_style_shadow_color(k, lv_color_black(), 0);
  lv_obj_clear_flag(k, LV_OBJ_FLAG_CLICKABLE);     // presses reach the pill

  if (on) lv_obj_add_state(b, LV_STATE_CHECKED);
  // The face updates first, then the caller's handler runs.
  lv_obj_add_event_cb(b, evToggleFace, LV_EVENT_VALUE_CHANGED, NULL);
  lv_obj_add_event_cb(b, cb,           LV_EVENT_VALUE_CHANGED, NULL);
  toggleFace(b);
  return b;
}

static lv_obj_t *setBigLabel(lv_obj_t *parent, const char *txt) {
  lv_obj_t *l = lv_label_create(parent);
  lv_label_set_text(l, txt);
  lv_obj_set_style_text_font(l, &lv_font_montserrat_26, 0);
  lv_obj_set_style_text_color(l, lv_color_hex(SET_TEXT), 0);
  return l;
}

// setBtn, setBtnU and evGoHome are defined just above, with the
// settings screen. setBtnU exists because setBtn attaches its
// callback with no user data - anything that needs to carry a value
// must use setBtnU, since attaching the same handler twice would
// fire it twice, once with a null payload.

// ============================================================
// MASSAGE / RADIO / AIR CONDITIONER / FAN
//
// Ported from TEST 081, rescaled from 800x480 to 1024x600 and
// written to the fonts this lv_conf.h actually builds - 12, 14, 16
// and 26.
//
// NOTHING HERE COMMANDS ANY HARDWARE YET. There is no RS-485 bus,
// no bedbox firmware and no IR transmitter on this panel. Every
// control moves a variable and prints to serial. They are real
// interfaces waiting for a transport, not simulations pretending
// to work.
// ============================================================

// THE HEBREW IS STORED BACK TO FRONT ON PURPOSE. LVGL has no
// bidirectional text engine and would draw these left to right, so
// the letters are pre-reversed. Copied byte for byte from TEST 081.
// Reordering them in an editor breaks them.
#define P_MAPAL     "\xD7\x9C\xD7\xA4\xD7\x9E"                          // ׳³ֲ׳³ג‚×׳³ֲ
#define P_ALIYA     "\xD7\x94\xD7\x99\xD7\x99\xD7\x9C\xD7\xA2"          // ׳³ֲ¢׳³ֲ׳³ג„¢׳³ג„¢׳³ג€
#define P_NADNEDA   "\xD7\x94\xD7\x93\xD7\xA0\xD7\x93\xD7\xA0"          // ׳³ֲ ׳³ג€׳³ֲ ׳³ג€׳³ג€
#define P_MALE      "\xD7\x90\xD7\x9C\xD7\x9E"                          // ׳³ֲ׳³ֲ׳³ֲ
#define P_ALACHSON  "\xD7\x9F\xD7\x95\xD7\xA1\xD7\x9B\xD7\x9C\xD7\x90"  // ׳³ֲ׳³ֲ׳³ג€÷׳³ֲ¡׳³ג€¢׳³ֲ
#define P_TZAD      "\xD7\x93\xD7\xA6"                                  // ׳³ֲ¦׳³ג€
#define P_SICHRUR   "\xD7\xA8\xD7\x95\xD7\xA8\xD7\x97\xD7\xA1"          // ׳³ֲ¡׳³ג€”׳³ֲ¨׳³ג€¢׳³ֲ¨
#define P_LISHA     "\xD7\x94\xD7\xA9\xD7\x99\xD7\x9C"                  // ׳³ֲ׳³ג„¢׳³ֲ©׳³ג€
#define P_DOFEK     "\xD7\xA7\xD7\xA4\xD7\x95\xD7\x93"                  // ׳³ג€׳³ג€¢׳³ג‚×׳³ֲ§
#define P_NESHIMA   "\xD7\x94\xD7\x9E\xD7\x99\xD7\xA9\xD7\xA0"          // ׳³ֲ ׳³ֲ©׳³ג„¢׳³ֲ׳³ג€
#define P_GESHEM    "\xD7\x9D\xD7\xA9\xD7\x92"                          // ׳³ג€™׳³ֲ©׳³ֲ
#define P_AKRAI     "\xD7\x99\xD7\x90\xD7\xA8\xD7\xA7\xD7\x90"          // ׳³ֲ׳³ֲ§׳³ֲ¨׳³ֲ׳³ג„¢
#define M_OFF       "\xD7\x99\xD7\x95\xD7\x91\xD7\x9B"                  // ׳³ג€÷׳³ג„¢׳³ג€˜׳³ג€¢׳³ג„¢ (unused since TEST 044)

// Zone names, from TEST 075 in the repo. Pre-reversed like every
// other Hebrew string here.
#define Z_HEAD      "\xD7\xA9\xD7\x90\xD7\xA8"                          // ׳³ֲ¨׳³ֲ׳³ֲ©
#define Z_UPPER     "\xD7\x9F\xD7\x95\xD7\x99\xD7\x9C\xD7\xA2 \xD7\x92\xD7\x91"   // ׳³ג€™׳³ג€˜ ׳³ֲ¢׳³ֲ׳³ג„¢׳³ג€¢׳³ֲ
#define Z_LOWER     "\xD7\x9F\xD7\x95\xD7\xAA\xD7\x97\xD7\xAA \xD7\x92\xD7\x91"   // ׳³ג€™׳³ג€˜ ׳³ֳ—׳³ג€”׳³ֳ—׳³ג€¢׳³ֲ
#define Z_LEGS      "\xD7\x9D\xD7\x99\xD7\x9C\xD7\x92\xD7\xA8"                  // ׳³ֲ¨׳³ג€™׳³ֲ׳³ג„¢׳³ג„¢׳³ֲ
// The screen title, with a geresh, pre-reversed. font_hebrew_28
// covers ASCII 0x20-0x7E, so the plain apostrophe is in the font.
#define MASS_TITLE  "\x27\xD7\x96\xD7\x90\xD7\xA1\xD7\x9E"                  // ׳³ֲ׳³ֲ¡׳³ֲ׳³ג€“\x27
#define RADIO_TITLE "\xD7\x95\xD7\x99\xD7\x93\xD7\xA8"                      // ׳³ֲ¨׳³ג€׳³ג„¢׳³ג€¢

static lv_obj_t *patTile[12];
static lv_obj_t *sldZone[4];
static int  massPattern = -1;
static int  massTimer   = 30;          // minutes chosen
static uint32_t massEnd = 0;           // millis deadline, 0 = not running
static lv_obj_t *lblMassMode = NULL, *lblMassTimer = NULL;
static lv_obj_t *sldMassTimer = NULL, *swMassage = NULL;

// The twelve colours from TEST 041 lasted three tests - Shemi asked
// for the quiet TEST 081 look back: dark tiles, the chosen one lit.
static const char *kPatName[12] = {
  P_MAPAL,    P_ALIYA,   P_NADNEDA, P_MALE,
  P_ALACHSON, P_TZAD,    P_SICHRUR, P_LISHA,
  P_DOFEK,    P_NESHIMA, P_GESHEM,  P_AKRAI
};

static lv_obj_t *lblRadioVol = NULL;
static lv_obj_t *radioBtn[8] = { NULL };

static lv_obj_t *lblSleep = NULL, *sldSleep = NULL, *swRadio = NULL;
static lv_obj_t *badgeRadio = NULL, *badgeHome = NULL;   // "on a timer"

// A badge that answers "am I on a timer" from across the room. Two
// of them: one on the radio screen, one on the home screen's radio
// tile, so the question never needs a screen change.
static lv_obj_t *makeTimerBadge(lv_obj_t *par) {
  lv_obj_t *b = lv_obj_create(par);
  lv_obj_remove_style_all(b);
  lv_obj_set_size(b, 96, 40);
  lv_obj_set_style_radius(b, 20, 0);
  lv_obj_set_style_bg_opa(b, LV_OPA_COVER, 0);
  lv_obj_set_style_bg_color(b, lv_color_hex(0x12B886), 0);
  lv_obj_clear_flag(b, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_clear_flag(b, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_t *l = lv_label_create(b);
  lv_label_set_text(l, "--");
  lv_obj_set_style_text_font(l, &lv_font_montserrat_16, 0);
  lv_obj_set_style_text_color(l, lv_color_black(), 0);
  lv_obj_center(l);
  lv_obj_add_flag(b, LV_OBJ_FLAG_HIDDEN);
  return b;
}

// Blinks by toggling opacity rather than rebuilding. On an RGB
// panel anything that redraws a large area every second shows as a
// twitch; 96 by 40 pixels changing opacity does not.
static void updateTimerBadge(lv_obj_t *b, int32_t leftMs, bool blinkOn) {
  if (!b) return;
  if (leftMs <= 0) { lv_obj_add_flag(b, LV_OBJ_FLAG_HIDDEN); return; }
  lv_obj_clear_flag(b, LV_OBJ_FLAG_HIDDEN);
  lv_obj_t *l = lv_obj_get_child(b, 0);
  if (l) lv_label_set_text_fmt(l, "%ld min", (long)((leftMs + 59999) / 60000));
  lv_obj_set_style_bg_opa(b, blinkOn ? LV_OPA_COVER : LV_OPA_50, 0);
}
static int  radioVolume  = 10;
static int  radioStation = -1;

static lv_obj_t *lblAcPower = NULL, *lblAcTemp = NULL;

static lv_obj_t *lblFanSpeed = NULL;

// Recovered from TEST 069 in the repo, not invented. The colours
// came with them - Kan's own brand colours for the first two - so
// every button is a different colour.
//
// TEST 069 also noted that Kan 88 and Kan Gimel have real stream
// URLs, taken from the data-player-hls-src attribute on kan.org.il.
// Those URLs are not here: this panel plays nothing, and they
// belong on audionode.
struct Station { const char *name; uint32_t colour; };
static const Station kStation[8] = {
  { "Kan 88",       0x8C24FF },
  { "Kan Gimel",    0xFF931E },
  { "Galgalatz",    0x2C58A8 },
  { "Eco 99",       0x2E8A5C },
  { "Reshet Bet",   0x5F6673 },
  { "Kol HaMusica", 0x7A5BB5 },
  { "Groove Salad", 0x1D7A94 },
  { "Drone Zone",   0xA04F7E },
};

// The home tiles used to toggle their own state and repaint on the
// spot. They navigate now, so the state they show is set on the sub
// screens and has to be refreshed on the way back. Once per
// navigation, never on a timer - a large image repainted repeatedly
// is what made the old panel twitch.
static void refreshHomeTiles() {
  if (tileBedImg) {
    lv_img_set_src(tileBedImg, bedRunning ? &img_bed_on : &img_bed_off);
    repaintOwner(tileBedImg);
  }
  if (tileRadioImg) {
    lv_img_set_src(tileRadioImg, radioOn ? &img_radio_on : &img_radio_off);
    repaintOwner(tileRadioImg);
  }
  if (tileACImg) {
    lv_img_set_src(tileACImg, acPower ? &img_ac_on : &img_ac_off);
    repaintOwner(tileACImg);
  }
  if (tileFanImg) repaintOwner(tileFanImg);
}

static lv_obj_t *navBack(lv_obj_t *scr, const char *title) {
  lv_obj_set_style_bg_color(scr, lv_color_hex(0x0D0F16), 0);
  lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_event_cb(scr, evAnyPress, LV_EVENT_PRESSED, NULL);

  lv_obj_t *b = setBtn(scr, 56, 56, "<", evGoHome, BACK_COL);   // a SQUARE
  lv_obj_align(b, LV_ALIGN_TOP_LEFT, 14, 8);

  lv_obj_t *t = lv_label_create(scr);
  lv_label_set_text(t, title);
  lv_obj_set_style_text_font(t, &lv_font_montserrat_26, 0);
  lv_obj_set_style_text_color(t, lv_color_white(), 0);
  lv_obj_align(t, LV_ALIGN_TOP_MID, 0, 20);
  return t;
}

// ---------------- Massage ----------------

static void refreshPatTiles() {
  for (int i = 0; i < 12; i++) {
    if (!patTile[i]) continue;
    lv_obj_set_style_bg_color(patTile[i],
        lv_color_hex(i == massPattern ? 0x1F6F4A : 0x232735), 0);
  }
  if (lblMassMode)
    lv_label_set_text(lblMassMode, massPattern < 0 ? "off"
                                                   : kPatName[massPattern]);
  if (lblMassTimer) {
    if (massEnd == 0)
      lv_label_set_text_fmt(lblMassTimer, "Timer  off      (set %d min)",
                            massTimer);
    else {
      int32_t left = (int32_t)(massEnd - millis());
      if (left < 0) left = 0;
      lv_label_set_text_fmt(lblMassTimer, "Timer  %ld min left",
                            (long)((left + 59999) / 60000));
    }
    lv_obj_set_style_text_color(lblMassTimer,
        lv_color_hex(massEnd ? 0x2BD4A0 : 0x8A94A0), 0);
  }
  if (swMassage) {
    bool on = lv_obj_has_state(swMassage, LV_STATE_CHECKED);
    if (on != bedRunning) {
      if (bedRunning) lv_obj_add_state(swMassage, LV_STATE_CHECKED);
      else            lv_obj_clear_state(swMassage, LV_STATE_CHECKED);
      toggleFace(swMassage);
    }
  }
}

static void evPreset(lv_event_t *e) {
  int i = (int)(intptr_t)lv_event_get_user_data(e);
  massPattern = (massPattern == i) ? -1 : i;
  bedRunning  = (massPattern >= 0);
  if (!bedRunning) massEnd = 0;
  refreshPatTiles();
  Serial.printf("[massage] pattern %d\n", massPattern);
}

static void evZone(lv_event_t *e) {
  int z = (int)(intptr_t)lv_event_get_user_data(e);
  int v = lv_slider_get_value(lv_event_get_target(e));
  Serial.printf("[massage] zone %d = %d\n", z + 1, v);
}

// Rollover safe, exactly like the radio timer: millis() wraps after
// about 49 days and this panel will cross that.
static void massStart(int minutes) {
  if (minutes < 5)  minutes = 5;
  if (minutes > 60) minutes = 60;
  massTimer = minutes;
  if (sldMassTimer) lv_slider_set_value(sldMassTimer, massTimer, LV_ANIM_ON);
  if (!bedRunning) {
    massEnd = 0;
    refreshPatTiles();
    Serial.println("[massage] timer set, but the massage is off");
    return;
  }
  massEnd = millis() + (uint32_t)massTimer * 60000UL;
  if (massEnd == 0) massEnd = 1;        // 0 is the "not running" marker
  refreshPatTiles();
  Serial.printf("[massage] timer %d min\n", massTimer);
}

static void evTimerBtn(lv_event_t *e) {
  int m = (int)(intptr_t)lv_event_get_user_data(e);
  // Pressing the running preset again cancels it, same as the radio.
  if (massEnd != 0 && m == massTimer) {
    massEnd = 0;
    refreshPatTiles();
    Serial.println("[massage] timer cancelled");
    return;
  }
  massStart(m);
}

static void evMassTimerSld(lv_event_t *e) {
  massTimer = lv_slider_get_value(lv_event_get_target(e));
  refreshPatTiles();
}
static void evMassTimerDone(lv_event_t *e) { massStart(massTimer); }

static void evMassPower(lv_event_t *e) {
  bedRunning = lv_obj_has_state(lv_event_get_target(e), LV_STATE_CHECKED);
  if (!bedRunning) { massPattern = -1; massEnd = 0; }
  else if (massPattern < 0) massPattern = 0;   // a switch that does nothing is worse
  refreshPatTiles();
  Serial.printf("[massage] %s\n", bedRunning ? "on" : "off");
}

// Once a second, from sensorTick.
static void massTick() {
  if (massEnd == 0) return;
  if ((int32_t)(massEnd - millis()) > 0) { refreshPatTiles(); return; }
  massEnd = 0;
  massPattern = -1;
  bedRunning = false;
  refreshPatTiles();
  Serial.println("[massage] timer expired - massage off");
}

// The red kibui button went in TEST 044. One flip switch, the
// same one the radio has, and nothing else.


static void buildMassage() {
  scrMassage = lv_obj_create(NULL);
  // navBack returns the title label, so the Hebrew font can be put
  // on it - 28 point, a little larger than the 26 point Latin
  // titles, and the only Hebrew size compiled into this build.
  lv_obj_t *tt = navBack(scrMassage, MASS_TITLE);
  lv_obj_set_style_text_font(tt, &font_hebrew_28, 0);

  // lblMassMode stood at the top right showing the chosen pattern -
  // the "something, I do not know what sign it is". Gone, per Shemi.

  const int TW = 168, TH = 120, TX = 20, TY = 78, TGX = 12, TGY = 12;
  for (int i = 0; i < 12; i++) {
    lv_obj_t *b = lv_btn_create(scrMassage);
    lv_obj_set_size(b, TW, TH);
    lv_obj_align(b, LV_ALIGN_TOP_LEFT,
                 TX + (i % 4) * (TW + TGX), TY + (i / 4) * (TH + TGY));
    lv_obj_set_style_radius(b, 10, 0);
    lv_obj_set_style_shadow_width(b, 0, 0);
    lv_obj_add_flag(b, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_add_event_cb(b, evPreset, LV_EVENT_CLICKED, (void *)(intptr_t)i);
    lv_obj_t *l = lv_label_create(b);
    lv_label_set_text(l, kPatName[i]);
    lv_obj_set_style_text_font(l, &font_hebrew_28, 0);
    lv_obj_center(l);
    patTile[i] = b;
  }

  // Four upright zone sliders. Numbered, not named: a Hebrew word
  // will not fit legibly in a 34 px column, and 1 head, 2 upper
  // back, 3 lower back, 4 legs is the order of the bed itself.
  const int SW = 34, SH = 300, SX = 776, SY = 108, SGX = 58;
  for (int z = 0; z < 4; z++) {
    int x = SX + z * SGX;

    sldZone[z] = lv_slider_create(scrMassage);
    lv_obj_set_size(sldZone[z], SW, SH);      // taller than wide = upright
    lv_obj_align(sldZone[z], LV_ALIGN_TOP_LEFT, x, SY);
    lv_slider_set_range(sldZone[z], 0, 100);
    lv_obj_set_style_bg_color(sldZone[z], lv_color_hex(0x1A1E2A), LV_PART_MAIN);
    lv_obj_set_style_bg_color(sldZone[z], lv_color_hex(0x12B886), LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(sldZone[z], lv_color_hex(0x2BD4A0), LV_PART_KNOB);
    lv_obj_add_flag(sldZone[z], LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_add_event_cb(sldZone[z], evZone, LV_EVENT_VALUE_CHANGED,
                        (void *)(intptr_t)z);

  }

  // Instead of names: a figure lying under the sliders, head at the
  // left under the first, feet at the right under the fourth. The
  // mapping is drawn, not written. Sliders span x 776..984, so the
  // figure does too.
  {
    const uint32_t FIG = 0x8A94A2;
    // Well BELOW the sliders (they end at y 408), so the man reads
    // as a legend for the columns above him, not part of them.
    struct { int x, y, w, h, r; } part[4] = {
      { 776, 468, 32, 32, LV_RADIUS_CIRCLE },   // head
      { 812, 472, 96, 24, 12 },                 // torso
      { 906, 476, 58, 16, 8 },                  // thighs
      { 962, 478, 24, 12, 6 },                  // feet
    };
    for (int i = 0; i < 4; i++) {
      lv_obj_t *o = lv_obj_create(scrMassage);
      lv_obj_remove_style_all(o);
      lv_obj_set_size(o, part[i].w, part[i].h);
      lv_obj_set_style_radius(o, part[i].r, 0);
      lv_obj_set_style_bg_opa(o, LV_OPA_COVER, 0);
      lv_obj_set_style_bg_color(o, lv_color_hex(FIG), 0);
      lv_obj_clear_flag(o, LV_OBJ_FLAG_CLICKABLE);
      lv_obj_align(o, LV_ALIGN_TOP_LEFT, part[i].x, part[i].y);
    }
  }

  lblMassTimer = lv_label_create(scrMassage);
  lv_obj_set_style_text_font(lblMassTimer, &lv_font_montserrat_16, 0);
  lv_obj_align(lblMassTimer, LV_ALIGN_TOP_LEFT, 20, 470);

  const int tm[3] = { 15, 30, 60 };
  for (int i = 0; i < 3; i++) {
    char cap[8];
    snprintf(cap, sizeof(cap), "%d", tm[i]);
    lv_obj_t *b = setBtnU(scrMassage, 130, 52, cap, evTimerBtn, 0x1F5A8A, tm[i]);
    lv_obj_align(b, LV_ALIGN_TOP_LEFT, 20 + i * 142, 498);
  }

  sldMassTimer = lv_slider_create(scrMassage);
  // Ends exactly where the 60 button ends: buttons at 20, 162, 304,
  // each 130 wide, so the row ends at 434. 20 + 414 = 434.
  lv_obj_set_size(sldMassTimer, 414, 26);
  lv_slider_set_range(sldMassTimer, 5, 60);
  lv_slider_set_value(sldMassTimer, massTimer, LV_ANIM_OFF);
  lv_obj_align(sldMassTimer, LV_ALIGN_TOP_LEFT, 20, 562);
  lv_obj_add_event_cb(sldMassTimer, evMassTimerSld,  LV_EVENT_VALUE_CHANGED, NULL);
  lv_obj_add_event_cb(sldMassTimer, evMassTimerDone, LV_EVENT_RELEASED,      NULL);


  // Into the button row as well: buttons end at 434, so the switch
  // starts at 454 and the slider below still stops at 434.
  swMassage = makeToggle(scrMassage, evMassPower, bedRunning);
  lv_obj_align(swMassage, LV_ALIGN_TOP_LEFT, 454, 494);

  lv_obj_t *mcap = lv_label_create(scrMassage);
  lv_label_set_text(mcap, "Massage");
  lv_obj_set_style_text_font(mcap, &lv_font_montserrat_16, 0);
  lv_obj_set_style_text_color(mcap, lv_color_hex(0xD8DEE8), 0);
  lv_obj_align(mcap, LV_ALIGN_TOP_LEFT, 454, 470);

  refreshPatTiles();
}

// ---------------- Radio ----------------

// Rollover safe. millis() wraps after about 49 days; comparing
// millis() >= sleepEnd fails exactly once when it does, and the
// radio stops in the middle of a night months from now for no
// visible reason. Subtract first, test the sign.
static int32_t sleepLeftMs() {
  if (sleepEnd == 0) return -1;
  return (int32_t)(sleepEnd - millis());
}

// Dull until chosen. The chosen one gets its full colour and a
// white frame - much stronger than the rest.
static void refreshStations() {
  for (int i = 0; i < 8; i++) {
    if (!radioBtn[i]) continue;
    bool sel = (i == radioStation);
    lv_color_t full = lv_color_hex(kStation[i].colour);
    lv_color_t dull = lv_color_mix(full, lv_color_hex(0x1A1E2A), 96);
    lv_obj_set_style_bg_color(radioBtn[i], sel ? full : dull, 0);
    lv_obj_set_style_border_width(radioBtn[i], sel ? 4 : 0, 0);
    lv_obj_set_style_border_color(radioBtn[i], lv_color_white(), 0);
  }
}

static void refreshRadio() {
  refreshStations();
  if (lblRadioVol)
    lv_label_set_text_fmt(lblRadioVol, "Volume  %d", radioVolume);

  {
    static bool blink = false;
    blink = !blink;
    int32_t l = sleepLeftMs();
    updateTimerBadge(badgeRadio, l, blink);
    updateTimerBadge(badgeHome,  l, blink);
  }

  if (lblSleep) {
    int32_t left = sleepLeftMs();
    if (left <= 0) {
      lv_label_set_text_fmt(lblSleep, "Sleep timer  off      (set %u min)",
                            sleepMin);
      lv_obj_set_style_text_color(lblSleep, lv_color_hex(0x8A94A0), 0);
    } else {
      // Round up, so a timer with 30 seconds left reads "1 min"
      // rather than "0 min" for half a minute.
      lv_label_set_text_fmt(lblSleep, "Sleep timer  %ld min left",
                            (long)((left + 59999) / 60000));
      lv_obj_set_style_text_color(lblSleep, lv_color_hex(0x2BD4A0), 0);
    }
  }
}

static void evStation(lv_event_t *e) {
  radioStation = (int)(intptr_t)lv_event_get_user_data(e);
  radioOn = true;
  if (swRadio) lv_obj_add_state(swRadio, LV_STATE_CHECKED);
  if (swRadio) toggleFace(swRadio);
  refreshRadio();
  Serial.printf("[radio] %s\n", kStation[radioStation].name);
}

static void evRadioVol(lv_event_t *e) {
  radioVolume = lv_slider_get_value(lv_event_get_target(e));
  refreshRadio();
  Serial.printf("[radio] volume %d\n", radioVolume);
}

// evRadioStop went with the Stop button. The radio power toggle
// does the same job and shows its state while doing it.

static void sleepStart(uint8_t minutes) {
  if (minutes < 1)   minutes = 1;
  if (minutes > 60)  minutes = 60;
  sleepMin = minutes;
  cfgTouched();
  if (sldSleep) lv_slider_set_value(sldSleep, sleepMin, LV_ANIM_ON);

  if (!radioOn) {
    // Nothing to switch off. A timer counting down against silence
    // only produces a surprise later.
    sleepEnd = 0;
    refreshRadio();
    Serial.println("[radio] sleep set, but the radio is off");
    return;
  }
  sleepEnd = millis() + (uint32_t)sleepMin * 60000UL;
  if (sleepEnd == 0) sleepEnd = 1;      // 0 is the "off" marker
  refreshRadio();
  Serial.printf("[radio] sleep in %u min\n", sleepMin);
}

static void evSleepPreset(lv_event_t *e) {
  uint8_t m = (uint8_t)(intptr_t)lv_event_get_user_data(e);
  // Pressing the running preset again cancels it. Written on the
  // screen too - a hidden gesture is not a feature.
  if (sleepEnd != 0 && m == sleepMin) {
    sleepEnd = 0;
    refreshRadio();
    Serial.println("[radio] sleep cancelled");
    return;
  }
  sleepStart(m);
}

static void evSleepSlider(lv_event_t *e) {
  sleepMin = (uint8_t)lv_slider_get_value(lv_event_get_target(e));
  cfgTouched();
  refreshRadio();
}

// Dragging chooses the length; lifting the finger starts it, the
// same way the haptic sliders behave.
static void evSleepSliderDone(lv_event_t *e) { sleepStart(sleepMin); }

static void evRadioPower(lv_event_t *e) {
  bool on = lv_obj_has_state(lv_event_get_target(e), LV_STATE_CHECKED);
  radioOn = on;
  if (on) {
    // A power switch that appears to do nothing is worse than one
    // that makes an obvious choice.
    if (radioStation < 0) radioStation = 0;
    Serial.printf("[radio] on - %s\n", kStation[radioStation].name);
  } else {
    radioStation = -1;
    sleepEnd = 0;              // nothing left for the timer to switch off
    Serial.println("[radio] off");
  }
  refreshRadio();
}

// Once a second, from sensorTick.
static void sleepTick() {
  if (sleepEnd == 0) return;
  if (sleepLeftMs() > 0) { refreshRadio(); return; }
  updateTimerBadge(badgeRadio, -1, false);
  updateTimerBadge(badgeHome,  -1, false);
  sleepEnd = 0;
  radioStation = -1;
  radioOn = false;
  if (swRadio) { lv_obj_clear_state(swRadio, LV_STATE_CHECKED); toggleFace(swRadio); }
  refreshRadio();
  Serial.println("[radio] sleep timer expired - radio off");
}

static void buildRadio() {
  scrRadio = lv_obj_create(NULL);
  lv_obj_t *rt = navBack(scrRadio, RADIO_TITLE);
  lv_obj_set_style_text_font(rt, &font_hebrew_28, 0);

  const int SW = 230, SH = 110, SX = 24, SY = 92, SG = 16;
  for (int i = 0; i < 8; i++) {
    lv_obj_t *b = setBtnU(scrRadio, SW, SH, kStation[i].name, evStation,
                          kStation[i].colour, i);
    radioBtn[i] = b;
    lv_obj_align(b, LV_ALIGN_TOP_LEFT,
                 SX + (i % 4) * (SW + SG), SY + (i / 4) * (SH + SG));
    // setBtnU labels at 16; these want to be readable from the bed.
    lv_obj_set_style_text_font(lv_obj_get_child(b, 0),
                               &lv_font_montserrat_26, 0);
  }

  lblRadioVol = lv_label_create(scrRadio);
  lv_obj_set_style_text_font(lblRadioVol, &lv_font_montserrat_16, 0);
  lv_obj_set_style_text_color(lblRadioVol, lv_color_hex(0x8A94A0), 0);
  lv_obj_align(lblRadioVol, LV_ALIGN_TOP_LEFT, 24, 352);

  lv_obj_t *sl = lv_slider_create(scrRadio);
  lv_obj_set_size(sl, 620, 26);
  lv_slider_set_range(sl, 0, 21);
  lv_slider_set_value(sl, radioVolume, LV_ANIM_OFF);
  lv_obj_align(sl, LV_ALIGN_TOP_LEFT, 24, 382);
  lv_obj_add_flag(sl, LV_OBJ_FLAG_EVENT_BUBBLE);
  lv_obj_add_event_cb(sl, evRadioVol, LV_EVENT_VALUE_CHANGED, NULL);

  lv_obj_t *vm = lv_label_create(scrRadio);
  lv_label_set_text(vm, "MAX");
  lv_obj_set_style_text_font(vm, &lv_font_montserrat_16, 0);
  lv_obj_set_style_text_color(vm, lv_color_hex(0xC8CDDB), 0);
  lv_obj_align(vm, LV_ALIGN_TOP_LEFT, 652, 386);

  lblSleep = lv_label_create(scrRadio);
  lv_obj_set_style_text_font(lblSleep, &lv_font_montserrat_16, 0);
  lv_obj_align(lblSleep, LV_ALIGN_TOP_LEFT, 24, 434);

  badgeRadio = makeTimerBadge(scrRadio);
  // Was at 400, which the switch now sits on. Out to the right,
  // where nothing else reaches.
  lv_obj_align(badgeRadio, LV_ALIGN_TOP_RIGHT, -24, 458);

  const uint8_t pre[3] = { 10, 30, 60 };
  for (int i = 0; i < 3; i++) {
    char cap[10];
    snprintf(cap, sizeof(cap), "%u min", pre[i]);
    lv_obj_t *b = setBtnU(scrRadio, 128, 54, cap, evSleepPreset,
                          0x1F5A8A, pre[i]);
    lv_obj_align(b, LV_ALIGN_TOP_LEFT, 24 + i * 140, 462);
  }
  // In the button row, immediately right of the 60 button, which
  // ends at 432. The slider below still ends at 432 too.
  swRadio = makeToggle(scrRadio, evRadioPower, radioOn);
  lv_obj_align(swRadio, LV_ALIGN_TOP_LEFT, 452, 458);

  lv_obj_t *rp = lv_label_create(scrRadio);
  lv_label_set_text(rp, "Radio");
  lv_obj_set_style_text_font(rp, &lv_font_montserrat_16, 0);
  lv_obj_set_style_text_color(rp, lv_color_hex(0xD8DEE8), 0);
  lv_obj_align(rp, LV_ALIGN_TOP_LEFT, 452, 434);


  sldSleep = lv_slider_create(scrRadio);
  // Ends exactly where the 60 minute button ends: buttons at 24,
  // 164, 304, each 128 wide, so the row ends at 432. 24 + 408 = 432.
  lv_obj_set_size(sldSleep, 408, 26);
  lv_slider_set_range(sldSleep, 1, 60);   // matches the three buttons
  lv_slider_set_value(sldSleep, sleepMin, LV_ANIM_OFF);
  lv_obj_align(sldSleep, LV_ALIGN_TOP_LEFT, 24, 536);
  lv_obj_add_event_cb(sldSleep, evSleepSlider,     LV_EVENT_VALUE_CHANGED, NULL);
  lv_obj_add_event_cb(sldSleep, evSleepSliderDone, LV_EVENT_RELEASED,      NULL);

  refreshRadio();
}

// ---------------- Air conditioner ----------------

// From the old panel, unchanged wording - these are the Gree's own
// five modes and four fan steps.
static const char *AC_MODE_NAME[5] = { "Cool", "Heat", "Dry", "Fan", "Auto" };
static const char *AC_FAN_NAME[4]  = { "Low", "Med", "High", "Auto" };
static int  acMode  = 0;
static int  acFanSp = 3;               // auto
static bool acSwing = false;
static lv_obj_t *acModeBtn[5] = { NULL };
static lv_obj_t *acFanBtn[4]  = { NULL };
static lv_obj_t *acSwingBtn   = NULL;

static void refreshAC() {
  if (lblAcPower) lv_label_set_text(lblAcPower, acPower ? "ON" : "OFF");
  if (lblAcTemp)  lv_label_set_text_fmt(lblAcTemp, "%d", acTemp);
  for (int i = 0; i < 5; i++)
    if (acModeBtn[i])
      lv_obj_set_style_bg_color(acModeBtn[i],
          lv_color_hex(i == acMode ? 0x2080FF : 0x2A3346), 0);
  for (int i = 0; i < 4; i++)
    if (acFanBtn[i])
      lv_obj_set_style_bg_color(acFanBtn[i],
          lv_color_hex(i == acFanSp ? 0x2080FF : 0x2A3346), 0);
  if (acSwingBtn)
    lv_obj_set_style_bg_color(acSwingBtn,
        lv_color_hex(acSwing ? 0x2080FF : 0x2A3346), 0);
}


// The tile and the screen both come through here, so the two can
// never disagree about what "on" means.
static void acSetPower(bool on) {
  acPower = on;
  refreshAC();
  refreshHomeTiles();
  Serial.printf("[ac] %s\n", acPower ? "on" : "off");
}

static void evAcPwr (lv_event_t *e) { acSetPower(!acPower); }
static void evAcMode(lv_event_t *e) {
  acMode = (int)(intptr_t)lv_event_get_user_data(e);
  refreshAC();
  Serial.printf("[ac] mode %s\n", AC_MODE_NAME[acMode]);
}
static void evAcFan(lv_event_t *e) {
  acFanSp = (int)(intptr_t)lv_event_get_user_data(e);
  refreshAC();
  Serial.printf("[ac] fan %s\n", AC_FAN_NAME[acFanSp]);
}
static void evAcSwing(lv_event_t *e) {
  acSwing = !acSwing;
  refreshAC();
  Serial.printf("[ac] swing %s\n", acSwing ? "on" : "off");
}
static void evAcUp  (lv_event_t *e) { if (acTemp < 30) acTemp++; refreshAC();
                                      Serial.printf("[ac] target %d\n", acTemp); }
static void evAcDn  (lv_event_t *e) { if (acTemp > 16) acTemp--; refreshAC();
                                      Serial.printf("[ac] target %d\n", acTemp); }

static void buildAC() {
  scrAC = lv_obj_create(NULL);
  navBack(scrAC, "Air conditioner");

  lv_obj_t *pw = setBtn(scrAC, 150, 56, "OFF", evAcPwr, 0x2A3040);
  lv_obj_align(pw, LV_ALIGN_TOP_RIGHT, -20, 8);
  lblAcPower = lv_obj_get_child(pw, 0);

  lv_obj_t *dn = setBtn(scrAC, 110, 110, "-", evAcDn, 0x2A3040);
  lv_obj_align(dn, LV_ALIGN_TOP_LEFT, 120, 150);

  lblAcTemp = lv_label_create(scrAC);
  lv_obj_set_style_text_font(lblAcTemp, &lv_font_montserrat_26, 0);
  lv_obj_set_style_text_color(lblAcTemp, lv_color_white(), 0);
  lv_obj_align(lblAcTemp, LV_ALIGN_TOP_LEFT, 268, 188);

  lv_obj_t *deg = lv_label_create(scrAC);
  lv_label_set_text(deg, "\xC2\xB0" "C");
  lv_obj_set_style_text_font(deg, &lv_font_montserrat_16, 0);
  lv_obj_set_style_text_color(deg, lv_color_hex(0x8A94A0), 0);
  lv_obj_align(deg, LV_ALIGN_TOP_LEFT, 320, 194);

  lv_obj_t *up = setBtn(scrAC, 110, 110, "+", evAcUp, 0x2A3040);
  lv_obj_align(up, LV_ALIGN_TOP_LEFT, 380, 150);

  // The room reading, so the target is set against something real.
  lv_obj_t *rc = lv_label_create(scrAC);
  lv_label_set_text(rc, "room now");
  lv_obj_set_style_text_font(rc, &lv_font_montserrat_16, 0);
  lv_obj_set_style_text_color(rc, lv_color_hex(0x8A94A0), 0);
  lv_obj_align(rc, LV_ALIGN_TOP_LEFT, 600, 160);

  lblAcRoom = lv_label_create(scrAC);
  lv_label_set_text(lblAcRoom, "--");
  lv_obj_set_style_text_font(lblAcRoom, &lv_font_montserrat_26, 0);
  lv_obj_set_style_text_color(lblAcRoom, lv_color_hex(0x60D0FF), 0);
  lv_obj_align(lblAcRoom, LV_ALIGN_TOP_LEFT, 600, 190);

  // Mode row - the Gree's five, from the old panel.
  lv_obj_t *ml = lv_label_create(scrAC);
  lv_label_set_text(ml, "Mode");
  lv_obj_set_style_text_font(ml, &lv_font_montserrat_16, 0);
  lv_obj_set_style_text_color(ml, lv_color_hex(0x8A94A0), 0);
  lv_obj_align(ml, LV_ALIGN_TOP_LEFT, 24, 320);
  for (int i = 0; i < 5; i++) {
    acModeBtn[i] = setBtnU(scrAC, 160, 62, AC_MODE_NAME[i], evAcMode,
                           0x2A3346, i);
    lv_obj_align(acModeBtn[i], LV_ALIGN_TOP_LEFT, 120 + i * 174, 306);
  }

  // Fan row - the indoor blower, not the ceiling fan.
  lv_obj_t *fl = lv_label_create(scrAC);
  lv_label_set_text(fl, "Fan");
  lv_obj_set_style_text_font(fl, &lv_font_montserrat_16, 0);
  lv_obj_set_style_text_color(fl, lv_color_hex(0x8A94A0), 0);
  lv_obj_align(fl, LV_ALIGN_TOP_LEFT, 24, 404);
  for (int i = 0; i < 4; i++) {
    acFanBtn[i] = setBtnU(scrAC, 160, 62, AC_FAN_NAME[i], evAcFan,
                          0x2A3346, i);
    lv_obj_align(acFanBtn[i], LV_ALIGN_TOP_LEFT, 120 + i * 174, 390);
  }

  lv_obj_t *sw = lv_label_create(scrAC);
  lv_label_set_text(sw, "Swing");
  lv_obj_set_style_text_font(sw, &lv_font_montserrat_16, 0);
  lv_obj_set_style_text_color(sw, lv_color_hex(0x8A94A0), 0);
  lv_obj_align(sw, LV_ALIGN_TOP_LEFT, 24, 488);
  acSwingBtn = setBtn(scrAC, 160, 62, "Swing", evAcSwing, 0x2A3346);
  lv_obj_align(acSwingBtn, LV_ALIGN_TOP_LEFT, 120, 474);

  // The old screen's honest note, kept word for word where it
  // matters: this AC does not report back.
  lv_obj_t *note = lv_label_create(scrAC);
  lv_label_set_text(note,
    "Shows what was last commanded - this AC does not report back.\n"
    "Gree protocol is decoded; the IR emitter lives in bedbox_shemi.");
  lv_obj_set_style_text_font(note, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(note, lv_color_hex(0x66707E), 0);
  lv_obj_align(note, LV_ALIGN_BOTTOM_MID, 0, -8);

  refreshAC();
}

// ---------------- Fan ----------------

static void refreshFan() {
  if (!lblFanSpeed) return;
  if (fanSpeed == 0) lv_label_set_text(lblFanSpeed, "off");
  else               lv_label_set_text_fmt(lblFanSpeed, "speed %d", fanSpeed);
}

static void evFanSpeed(lv_event_t *e) {
  fanSpeed = (int)(intptr_t)lv_event_get_user_data(e);
  fanOn = (fanSpeed > 0);
  refreshFan();
  Serial.printf("[fan] speed %d\n", fanSpeed);
}

static void buildFan() {
  scrFan = lv_obj_create(NULL);
  navBack(scrFan, "Fan");

  lblFanSpeed = lv_label_create(scrFan);
  lv_obj_set_style_text_font(lblFanSpeed, &lv_font_montserrat_26, 0);
  lv_obj_set_style_text_color(lblFanSpeed, lv_color_hex(0x60D0FF), 0);
  lv_obj_align(lblFanSpeed, LV_ALIGN_TOP_MID, 0, 90);

  const char *lbl[4] = { "off", "1", "2", "3" };
  for (int i = 0; i < 4; i++) {
    lv_obj_t *b = setBtnU(scrFan, 200, 150, lbl[i], evFanSpeed,
                          i == 0 ? 0x8F1F1F : 0x232735, i);
    lv_obj_align(b, LV_ALIGN_TOP_LEFT, 60 + i * 226, 180);
  }

  lv_obj_t *note = lv_label_create(scrFan);
  lv_label_set_text(note,
    "Westinghouse SQ19D-F20, 433 MHz, protocol 2, 29 bits.\n"
    "Codes are captured but the fan has never answered a transmission,\n"
    "and this panel has no transmitter. One way in any case - the\n"
    "screen shows what it commanded, not what the fan did.");
  lv_obj_set_style_text_font(note, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(note, lv_color_hex(0x66707E), 0);
  lv_obj_align(note, LV_ALIGN_BOTTOM_MID, 0, -40);

  refreshFan();
}

// ------------------------------------------------------------

// ------------------------------------------------------------
// The alarm clock on the home tile.
//
// Your photograph: red when armed, black when off. The drawn
// version from TEST 023 is gone.
// ------------------------------------------------------------
static lv_obj_t *acImg = NULL, *acWhen = NULL;

static void alarmIconColour() {
  if (acImg) lv_img_set_src(acImg, alarmOn ? &img_alarm_on : &img_alarm_off);
  if (acWhen) {
    if (use12h) {
      const char *sfx;
      uint8_t h12 = to12((uint8_t)(alarmMin / 60), &sfx);
      lv_label_set_text_fmt(acWhen, "%u:%02d %s", h12, alarmMin % 60, sfx);
    } else {
      lv_label_set_text_fmt(acWhen, "%02d:%02d", alarmMin / 60, alarmMin % 60);
    }
    lv_obj_set_style_text_color(acWhen,
        lv_color_hex(alarmOn ? 0xFF9090 : 0x8A90A0), 0);
  }
}

// A photograph cannot shake its own bells, so the whole picture
// nudges instead, in step with the ring pulse.
static void alarmIconShake(bool on) {
  static bool phase = false;
  int dx = 0;
  if (on) { phase = !phase; dx = phase ? 5 : -5; }
  if (acImg) lv_obj_align(acImg, LV_ALIGN_CENTER, dx, -12);
}

static void alarmIconHands() { }   // the photograph has its own

static void buildAlarmIcon(lv_obj_t *tile) {
  acImg = lv_img_create(tile);
  lv_img_set_src(acImg, &img_alarm_off);
  lv_obj_align(acImg, LV_ALIGN_CENTER, 0, -12);
  lv_obj_clear_flag(acImg, LV_OBJ_FLAG_CLICKABLE);

  acWhen = lv_label_create(tile);
  lv_obj_set_style_text_font(acWhen, &lv_font_montserrat_16, 0);
  lv_obj_clear_flag(acWhen, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_align(acWhen, LV_ALIGN_BOTTOM_MID, 0, -6);

  alarmIconColour();
  Serial.printf("[alarm] icon built from photo, %s, set for %02d:%02d\n",
                alarmOn ? "ARMED (red)" : "off (black)",
                alarmMin / 60, alarmMin % 60);
  Serial.flush();
}

// ============================================================
// BOOT PHOTOGRAPH
//
// Same decoder TEST 081 used, different destination. That version
// handed each block straight to Arduino_GFX, which this board does
// not have. Here JPEGDEC fills a PSRAM buffer and LVGL shows it.
// ============================================================
#define BOOT_PHOTO_W   800
#define BOOT_PHOTO_H   480
#define BOOT_PHOTO_MS  3000

static JPEGDEC        jpegDec;
static uint16_t      *bootPix   = NULL;      // PSRAM, freed after the photo
static lv_obj_t      *scrBoot   = NULL;
static lv_img_dsc_t   bootDsc;

// JPEGDEC hands back blocks, not scanlines, so each row of each
// block is copied into the right place in the full frame.
static int bootDrawCb(JPEGDRAW *p) {
  if (!bootPix) return 0;
  for (int row = 0; row < p->iHeight; row++) {
    int y = p->y + row;
    if (y < 0 || y >= BOOT_PHOTO_H) continue;
    int x = p->x;
    int w = p->iWidth;
    if (x < 0) { w += x; x = 0; }
    if (x + w > BOOT_PHOTO_W) w = BOOT_PHOTO_W - x;
    if (w <= 0) continue;
    memcpy(&bootPix[y * BOOT_PHOTO_W + x],
           &p->pPixels[row * p->iWidth],
           (size_t)w * 2);
  }
  return 1;
}

// Returns true only if there is a photograph on the screen.
static bool showBootPhoto() {
#if PANEL_ID == 2
  const uint8_t *src = BOOT_IRA_JPG;   const uint32_t len = BOOT_IRA_JPG_LEN;
  const char    *who = "Ira";
#else
  const uint8_t *src = BOOT_SHEMI_JPG; const uint32_t len = BOOT_SHEMI_JPG_LEN;
  const char    *who = "Shemi";
#endif

  // 768,000 bytes - six times the whole LVGL heap, so it goes in
  // PSRAM and comes straight back afterwards.
  bootPix = (uint16_t *)ps_malloc((size_t)BOOT_PHOTO_W * BOOT_PHOTO_H * 2);
  if (!bootPix) { Serial.println("boot photo: no PSRAM"); return false; }
  memset(bootPix, 0, (size_t)BOOT_PHOTO_W * BOOT_PHOTO_H * 2);

  // JPEGDEC needs a writable pointer and PROGMEM is not one.
  uint8_t *jbuf = (uint8_t *)malloc(len);
  if (!jbuf) {
    Serial.printf("boot photo: no heap for %u bytes\n", (unsigned)len);
    free(bootPix); bootPix = NULL;
    return false;
  }
  memcpy_P(jbuf, src, len);

  bool ok = false;
  if (jpegDec.openRAM(jbuf, (int)len, bootDrawCb)) {
    jpegDec.setPixelType(RGB565_LITTLE_ENDIAN);   // matches LV_COLOR_16_SWAP 0
    ok = jpegDec.decode(0, 0, 0);
    jpegDec.close();
  }
  free(jbuf);

  if (!ok) {
    Serial.println("boot photo: DECODE FAILED");
    free(bootPix); bootPix = NULL;
    return false;
  }

  bootDsc.header.cf        = LV_IMG_CF_TRUE_COLOR;
  bootDsc.header.always_zero = 0;
  bootDsc.header.reserved  = 0;
  bootDsc.header.w         = BOOT_PHOTO_W;
  bootDsc.header.h         = BOOT_PHOTO_H;
  bootDsc.data_size        = (uint32_t)BOOT_PHOTO_W * BOOT_PHOTO_H * 2;
  bootDsc.data             = (const uint8_t *)bootPix;

  scrBoot = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(scrBoot, lv_color_black(), 0);
  lv_obj_clear_flag(scrBoot, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_t *im = lv_img_create(scrBoot);
  lv_img_set_src(im, &bootDsc);
  lv_obj_center(im);
  lv_scr_load(scrBoot);

  Serial.printf("boot photo: panel %d, %s, shown\n", PANEL_ID, who);
  return true;
}

// The screen and its 768 KB go back the moment the photo leaves.
// Nothing about it stays resident.
static void dropBootPhoto() {
  if (scrBoot) { lv_obj_del(scrBoot); scrBoot = NULL; }
  if (bootPix) { free(bootPix); bootPix = NULL; }
}

// ============================================================
// SCREENSAVER - the analog clock face
//
// Ported from buildAnalog() in TEST 081 and rescaled from 800x480
// to 1024x600. Round face, twelve ticks, the numbers 1..12, three
// hands, and underneath the time in figures with temperature and
// humidity.
// ============================================================
#define SAV_CX      500
#define SAV_CY      300
#define SAV_R       250
#define SAV_FACE_R  202

static lv_obj_t *scrSaver = NULL;
static lv_obj_t *savHandH = NULL, *savHandM = NULL, *savHandS = NULL;
static lv_obj_t *savCoreH = NULL, *savCoreM = NULL;
static lv_obj_t *savTemp = NULL, *savHum = NULL, *savAlarmImg = NULL;
static lv_obj_t *savSecDot = NULL, *savDate = NULL, *savDow = NULL;
static lv_obj_t *savName = NULL;

static lv_point_t savPtH[2], savPtM[2], savPtS[2];
static lv_point_t savPtHC[2], savPtMC[2];
static lv_point_t savRingPts[60][2];

#define SAV_GREEN   0x91A87D
#define SAV_GREEN2  0xA9BC96
#define SAV_FACE    0x03111B
#define SAV_FACE2   0x061D2B
#define SAV_RING    0x2C2D2D
#define SAV_RING_HI 0x575650
#define SAV_HAND    0x8A887F
#define SAV_RED     0xB61C22

static lv_obj_t *savHand(lv_obj_t *par, int w, uint32_t col) {
  lv_obj_t *l = lv_line_create(par);
  lv_obj_set_style_line_color(l, lv_color_hex(col), 0);
  lv_obj_set_style_line_width(l, w, 0);
  lv_obj_set_style_line_rounded(l, false, 0);
  lv_obj_clear_flag(l, LV_OBJ_FLAG_CLICKABLE);
  return l;
}

static void savSeg(lv_obj_t *o, lv_point_t *p, float deg, int from, int to) {
  float r = deg * 0.01745329f;
  p[0].x = (lv_coord_t)(SAV_CX + from * sinf(r));
  p[0].y = (lv_coord_t)(SAV_CY - from * cosf(r));
  p[1].x = (lv_coord_t)(SAV_CX + to   * sinf(r));
  p[1].y = (lv_coord_t)(SAV_CY - to   * cosf(r));
  lv_line_set_points(o, p, 2);
}

static void saverUpdate() {
  if (!scrSaver || !now.valid) return;

  float as = now.sec * 6.0f;
  if (savHandS) savSeg(savHandS, savPtS, as, -58, SAV_FACE_R - 12);

  if (savSecDot) {
    float r = as * 0.01745329f;
    int dr = SAV_FACE_R - 34;
    lv_obj_align(savSecDot, LV_ALIGN_TOP_LEFT,
                 SAV_CX + (int)(dr * sinf(r)) - 9,
                 SAV_CY - (int)(dr * cosf(r)) - 9);
  }

  static uint8_t lastMin = 255;
  if (now.min != lastMin) {
    lastMin = now.min;
    float am = now.min * 6.0f;
    float ah = ((now.hour % 12) * 30.0f) + (now.min * 0.5f);

    if (savHandM) savSeg(savHandM, savPtM,  am, -28, SAV_FACE_R - 20);
    if (savCoreM) savSeg(savCoreM, savPtMC, am, -16, SAV_FACE_R - 28);
    if (savHandH) savSeg(savHandH, savPtH,  ah, -24, SAV_FACE_R - 74);
    if (savCoreH) savSeg(savCoreH, savPtHC, ah, -12, SAV_FACE_R - 82);

    if (savDate) lv_label_set_text_fmt(savDate, "%u", now.day);
    if (savDow) {
      const char *d = kDow[now.dow <= 7 ? now.dow : 0];
      lv_label_set_text(savDow, d);
    }
  }

  if (savTemp) {
    if (shtOK && shtTempC > -10.0f && shtTempC < 60.0f) {
      char t[24]; snprintf(t, sizeof(t), "%.0f\xC2\xB0", shtTempC);
      lv_label_set_text(savTemp, t);
    } else lv_label_set_text(savTemp, "--");
  }

  if (savHum) {
    if (shtOK && shtRH >= 0.0f && shtRH <= 100.0f) {
      char t[16]; snprintf(t, sizeof(t), "%.0f%%", shtRH);
      lv_label_set_text(savHum, t);
    } else lv_label_set_text(savHum, "--");
  }

  if (savAlarmImg) {
    if (alarmOn) lv_obj_clear_flag(savAlarmImg, LV_OBJ_FLAG_HIDDEN);
    else         lv_obj_add_flag(savAlarmImg,   LV_OBJ_FLAG_HIDDEN);
  }
}

static void saverExit(lv_event_t *e) {
  if (!saverOn) return;
  saverOn = false;
  lastTouchMs = millis();
  applyBacklightQuiet(blBeforeSaver);
  if (scrHome) { refreshHomeTiles(); lv_scr_load(scrHome); }
  Serial.println("[saver] out");
}

static void savDot(lv_obj_t *par, int cx, int cy, int d) {
  lv_obj_t *outer = lv_obj_create(par);
  lv_obj_remove_style_all(outer);
  lv_obj_set_size(outer, d + 8, d + 8);
  lv_obj_set_style_radius(outer, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_bg_opa(outer, LV_OPA_COVER, 0);
  lv_obj_set_style_bg_color(outer, lv_color_hex(0x444740), 0);
  lv_obj_align(outer, LV_ALIGN_TOP_LEFT, cx - (d + 8)/2, cy - (d + 8)/2);

  lv_obj_t *core = lv_obj_create(par);
  lv_obj_remove_style_all(core);
  lv_obj_set_size(core, d, d);
  lv_obj_set_style_radius(core, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_bg_opa(core, LV_OPA_COVER, 0);
  lv_obj_set_style_bg_color(core, lv_color_hex(SAV_GREEN), 0);
  lv_obj_align(core, LV_ALIGN_TOP_LEFT, cx - d/2, cy - d/2);
}

static void buildSaver() {
  scrSaver = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(scrSaver, lv_color_hex(0x01070C), 0);
  lv_obj_clear_flag(scrSaver, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_event_cb(scrSaver, saverExit, LV_EVENT_PRESSED, NULL);

  // 8 mm-looking gunmetal inner bezel/ring, centered exactly.
  lv_obj_t *bezel = lv_obj_create(scrSaver);
  lv_obj_remove_style_all(bezel);
  lv_obj_set_size(bezel, SAV_R * 2, SAV_R * 2);
  lv_obj_set_style_radius(bezel, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_bg_opa(bezel, LV_OPA_COVER, 0);
  lv_obj_set_style_bg_color(bezel, lv_color_hex(SAV_RING), 0);
  lv_obj_set_style_border_width(bezel, 8, 0);
  lv_obj_set_style_border_color(bezel, lv_color_hex(SAV_RING_HI), 0);
  lv_obj_align(bezel, LV_ALIGN_TOP_LEFT, SAV_CX - SAV_R, SAV_CY - SAV_R);

  // Black minute-track ring inside the metal bezel.
  lv_obj_t *track = lv_obj_create(scrSaver);
  lv_obj_remove_style_all(track);
  lv_obj_set_size(track, 440, 440);
  lv_obj_set_style_radius(track, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_bg_opa(track, LV_OPA_COVER, 0);
  lv_obj_set_style_bg_color(track, lv_color_hex(0x050607), 0);
  lv_obj_align(track, LV_ALIGN_TOP_LEFT, SAV_CX - 220, SAV_CY - 220);

  // Deep blue-black sunburst-like dial.
  lv_obj_t *face = lv_obj_create(scrSaver);
  lv_obj_remove_style_all(face);
  lv_obj_set_size(face, SAV_FACE_R * 2, SAV_FACE_R * 2);
  lv_obj_set_style_radius(face, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_bg_opa(face, LV_OPA_COVER, 0);
  lv_obj_set_style_bg_color(face, lv_color_hex(SAV_FACE), 0);
  lv_obj_set_style_bg_grad_color(face, lv_color_hex(SAV_FACE2), 0);
  lv_obj_set_style_bg_grad_dir(face, LV_GRAD_DIR_VER, 0);
  lv_obj_align(face, LV_ALIGN_TOP_LEFT,
               SAV_CX - SAV_FACE_R, SAV_CY - SAV_FACE_R);

  // 60 minute ticks on the inner ring; 5-minute ticks are broader.
  for (int i = 0; i < 60; ++i) {
    const float a = i * 6.0f * 0.01745329f;
    const int ro = 214;
    const int ri = (i % 5 == 0) ? 195 : 203;

    savRingPts[i][0].x = SAV_CX + (lv_coord_t)(ri * sinf(a));
    savRingPts[i][0].y = SAV_CY - (lv_coord_t)(ri * cosf(a));
    savRingPts[i][1].x = SAV_CX + (lv_coord_t)(ro * sinf(a));
    savRingPts[i][1].y = SAV_CY - (lv_coord_t)(ro * cosf(a));

    lv_obj_t *tick = lv_line_create(scrSaver);
    lv_line_set_points(tick, savRingPts[i], 2);
    lv_obj_set_style_line_color(tick,
        lv_color_hex(i % 5 == 0 ? 0x7A7B76 : 0x4B4C49), 0);
    lv_obj_set_style_line_width(tick, i % 5 == 0 ? 5 : 2, 0);
    lv_obj_set_style_line_rounded(tick, false, 0);
    lv_obj_clear_flag(tick, LV_OBJ_FLAG_CLICKABLE);
  }

  // Lume hour markers: slightly smaller than the previous version.
  for (int n = 1; n <= 11; ++n) {
    if (n == 3 || n == 6 || n == 9) continue;
    float a = n * 30.0f * 0.01745329f;
    int rr = 166;
    savDot(scrSaver,
           SAV_CX + (int)(sinf(a) * rr),
           SAV_CY - (int)(cosf(a) * rr),
           27);
  }

  // Twelve triangle.
  {
    static lv_point_t tri[4];
    tri[0] = { (lv_coord_t)(SAV_CX - 21), (lv_coord_t)(SAV_CY - 184) };
    tri[1] = { (lv_coord_t)(SAV_CX + 21), (lv_coord_t)(SAV_CY - 184) };
    tri[2] = { (lv_coord_t)SAV_CX,        (lv_coord_t)(SAV_CY - 145) };
    tri[3] = tri[0];

    lv_obj_t *t = lv_line_create(scrSaver);
    lv_line_set_points(t, tri, 4);
    lv_obj_set_style_line_color(t, lv_color_hex(SAV_GREEN), 0);
    lv_obj_set_style_line_width(t, 13, 0);
    lv_obj_set_style_line_rounded(t, true, 0);
  }

  // 3, 6, 9 markers.
  {
    struct M { int x, y, w, h; } m[3] = {
      { SAV_CX + 151, SAV_CY - 12, 46, 24 },
      { SAV_CX - 12,  SAV_CY + 145, 24, 46 },
      { SAV_CX - 197, SAV_CY - 12, 46, 24 }
    };
    for (int i = 0; i < 3; ++i) {
      lv_obj_t *o = lv_obj_create(scrSaver);
      lv_obj_remove_style_all(o);
      lv_obj_set_size(o, m[i].w, m[i].h);
      lv_obj_set_style_radius(o, 8, 0);
      lv_obj_set_style_bg_opa(o, LV_OPA_COVER, 0);
      lv_obj_set_style_bg_color(o, lv_color_hex(SAV_GREEN), 0);
      lv_obj_align(o, LV_ALIGN_TOP_LEFT, m[i].x, m[i].y);
    }
  }

  // Day + date window, pulled inward toward the center.
  {
    lv_obj_t *dw = lv_obj_create(scrSaver);
    lv_obj_remove_style_all(dw);
    lv_obj_set_size(dw, 118, 46);
    lv_obj_set_style_radius(dw, 4, 0);
    lv_obj_set_style_bg_opa(dw, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(dw, lv_color_hex(0xB9B8B0), 0);
    lv_obj_set_style_border_width(dw, 3, 0);
    lv_obj_set_style_border_color(dw, lv_color_hex(0x454642), 0);
    lv_obj_align(dw, LV_ALIGN_TOP_LEFT, SAV_CX + 60, SAV_CY - 23);

    savDate = lv_label_create(dw);
    lv_label_set_text(savDate, "--");
    lv_obj_set_style_text_font(savDate, &lv_font_montserrat_26, 0);
    lv_obj_set_style_text_color(savDate, lv_color_black(), 0);
    lv_obj_align(savDate, LV_ALIGN_LEFT_MID, 12, 0);

    savDow = lv_label_create(dw);
    lv_label_set_text(savDow, "---");
    lv_obj_set_style_text_font(savDow, &lv_font_montserrat_26, 0);
    lv_obj_set_style_text_color(savDow, lv_color_black(), 0);
    lv_obj_align(savDow, LV_ALIGN_RIGHT_MID, -8, 0);
  }

  // Realistic broad luminous hands + thin red seconds hand.
  savHandH = savHand(scrSaver, 20, SAV_HAND);
  savCoreH = savHand(scrSaver, 12, SAV_GREEN);
  savHandM = savHand(scrSaver, 14, SAV_HAND);
  savCoreM = savHand(scrSaver,  8, SAV_GREEN);
  savHandS = savHand(scrSaver,  3, SAV_RED);

  savSecDot = lv_obj_create(scrSaver);
  lv_obj_remove_style_all(savSecDot);
  lv_obj_set_size(savSecDot, 18, 18);
  lv_obj_set_style_radius(savSecDot, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_bg_opa(savSecDot, LV_OPA_COVER, 0);
  lv_obj_set_style_bg_color(savSecDot, lv_color_hex(SAV_GREEN2), 0);
  lv_obj_set_style_border_width(savSecDot, 2, 0);
  lv_obj_set_style_border_color(savSecDot, lv_color_hex(SAV_RED), 0);

  lv_obj_t *pin = lv_obj_create(scrSaver);
  lv_obj_remove_style_all(pin);
  lv_obj_set_size(pin, 18, 18);
  lv_obj_set_style_radius(pin, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_bg_opa(pin, LV_OPA_COVER, 0);
  lv_obj_set_style_bg_color(pin, lv_color_hex(SAV_RED), 0);
  lv_obj_align(pin, LV_ALIGN_TOP_LEFT, SAV_CX - 9, SAV_CY - 9);

  // Panel identity only; same visual design on both.
  savName = lv_label_create(scrSaver);
  lv_label_set_text(savName, PANEL_ID == 1 ? "ימש" : "אריע");
  lv_obj_set_style_text_font(savName, &font_hebrew_28, 0);
  lv_obj_set_style_text_color(savName, lv_color_hex(0xA5161C), 0);
  lv_obj_align(savName, LV_ALIGN_TOP_LEFT, SAV_CX - 45, SAV_CY + 82);

  // Night readouts, same lume colour as hour markers.
  savHum = lv_label_create(scrSaver);
  lv_label_set_text(savHum, "--");
  lv_obj_set_style_text_font(savHum, &lv_font_montserrat_48, 0);
  lv_obj_set_style_text_color(savHum, lv_color_hex(SAV_GREEN), 0);
  lv_obj_align(savHum, LV_ALIGN_TOP_LEFT, 62, 22);

  // Humidity: three rain drops, clearer than one isolated drop.
  const int dropX[3] = { 12, 27, 42 };
  const int dropY[3] = { 31, 42, 31 };
  for (int di = 0; di < 3; ++di) {
    lv_obj_t *drop = lv_obj_create(scrSaver);
    lv_obj_remove_style_all(drop);
    lv_obj_set_size(drop, 13, 18);
    lv_obj_set_style_radius(drop, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_opa(drop, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(drop, lv_color_hex(SAV_GREEN), 0);
    lv_obj_align(drop, LV_ALIGN_TOP_LEFT, dropX[di], dropY[di]);
  }

  savTemp = lv_label_create(scrSaver);
  lv_label_set_text(savTemp, "--");
  lv_obj_set_style_text_font(savTemp, &lv_font_montserrat_48, 0);
  lv_obj_set_style_text_color(savTemp, lv_color_hex(SAV_GREEN), 0);
  lv_obj_align(savTemp, LV_ALIGN_TOP_RIGHT, -54, 22);

  // Thermometer on the RIGHT side of the temperature, at the screen edge.
  lv_obj_t *thermStem = lv_obj_create(scrSaver);
  lv_obj_remove_style_all(thermStem);
  lv_obj_set_size(thermStem, 10, 30);
  lv_obj_set_style_radius(thermStem, 5, 0);
  lv_obj_set_style_border_width(thermStem, 2, 0);
  lv_obj_set_style_border_color(thermStem, lv_color_hex(SAV_GREEN), 0);
  lv_obj_align(thermStem, LV_ALIGN_TOP_RIGHT, -18, 26);

  lv_obj_t *thermBulb = lv_obj_create(scrSaver);
  lv_obj_remove_style_all(thermBulb);
  lv_obj_set_size(thermBulb, 18, 18);
  lv_obj_set_style_radius(thermBulb, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_border_width(thermBulb, 2, 0);
  lv_obj_set_style_border_color(thermBulb, lv_color_hex(SAV_GREEN), 0);
  lv_obj_align(thermBulb, LV_ALIGN_TOP_RIGHT, -14, 50);

  savAlarmImg = lv_img_create(scrSaver);
  lv_img_set_src(savAlarmImg, &img_alarm_on);
  lv_img_set_zoom(savAlarmImg, 80);
  lv_obj_align(savAlarmImg, LV_ALIGN_BOTTOM_RIGHT, -22, -16);
  lv_obj_add_flag(savAlarmImg, LV_OBJ_FLAG_HIDDEN);

  saverUpdate();
}
static void saverEnter() {
  if (saverOn || saverAfterMs == 0) return;
  if (!scrSaver) { buildSaver(); lvMem("saver"); }
  if (!scrSaver) return;
  saverOn = true;
  blBeforeSaver = backlight;
  saverUpdate();
  lv_scr_load(scrSaver);
  applyBacklightQuiet(saverBright);
  Serial.println("[saver] in");
}

// Once a second, from sensorTick.
static void saverTick() {
  if (saverOn) { saverUpdate(); return; }
  if (saverAfterMs == 0) return;
  if (millis() - lastTouchMs > saverAfterMs) saverEnter();
}

// ---------------- Alarm ----------------

static lv_obj_t *lblAlarmH = NULL, *lblAlarmM = NULL, *lblAlarmState = NULL;
static lv_obj_t *swAlarm = NULL, *swRepeat = NULL;
static lv_obj_t *btnSnooze = NULL, *btnStopRing = NULL;

static void refreshAlarm() {
  alarmIconColour();
  alarmIconHands();

  if (lblAlarmH) {
    if (use12h) {
      const char *sfx;
      uint8_t h12 = to12((uint8_t)(alarmMin / 60), &sfx);
      lv_label_set_text_fmt(lblAlarmH, "%u %s", h12, sfx);
    } else {
      lv_label_set_text_fmt(lblAlarmH, "%02d", alarmMin / 60);
    }
  }
  if (lblAlarmM) lv_label_set_text_fmt(lblAlarmM, "%02d", alarmMin % 60);

  if (lblAlarmState) {
    if (ringing) {
      lv_label_set_text(lblAlarmState, "RINGING");
      lv_obj_set_style_text_color(lblAlarmState, lv_color_hex(0xE05050), 0);
    } else if (snoozeUntil >= 0) {
      if (use12h) {
        const char *sfx;
        uint8_t h12 = to12((uint8_t)(snoozeUntil / 60), &sfx);
        lv_label_set_text_fmt(lblAlarmState, "snoozed until %u:%02d %s",
                              h12, snoozeUntil % 60, sfx);
      } else {
        lv_label_set_text_fmt(lblAlarmState, "snoozed until %02d:%02d",
                              snoozeUntil / 60, snoozeUntil % 60);
      }
      lv_obj_set_style_text_color(lblAlarmState, lv_color_hex(0xE0A040), 0);
    } else if (!alarmOn) {
      lv_label_set_text(lblAlarmState, "off");
      lv_obj_set_style_text_color(lblAlarmState, lv_color_hex(0x8A94A0), 0);
    } else if (!now.valid || now.wasStopped) {
      // An alarm armed against a clock that was never set would go
      // off at a random moment. Say so instead of pretending.
      lv_label_set_text(lblAlarmState, "clock not set - will not ring");
      lv_obj_set_style_text_color(lblAlarmState, lv_color_hex(0xE05050), 0);
    } else {
      lv_label_set_text(lblAlarmState,
                        alarmOnce ? "armed, once" : "armed, every day");
      lv_obj_set_style_text_color(lblAlarmState, lv_color_hex(0x2BD4A0), 0);
    }
  }

  if (btnSnooze)   ringing ? lv_obj_clear_flag(btnSnooze,   LV_OBJ_FLAG_HIDDEN)
                           : lv_obj_add_flag(btnSnooze,     LV_OBJ_FLAG_HIDDEN);
  if (btnStopRing) ringing ? lv_obj_clear_flag(btnStopRing, LV_OBJ_FLAG_HIDDEN)
                           : lv_obj_add_flag(btnStopRing,   LV_OBJ_FLAG_HIDDEN);
}

static void startRinging() {
  ringing       = true;
  ringStart     = millis();
  lastRingPulse = 0;
  snoozeUntil   = -1;
  Serial.printf("[alarm] ringing at %02u:%02u\n", now.hour, now.min);

  // Disarm HERE, not when it is stopped. Once means once no matter
  // what happens next - stopped, snoozed, ignored until it gives
  // up, or nobody in the room. Snooze runs off its own target, so a
  // once-only alarm can still be pushed nine minutes this morning.
  if (alarmOnce && alarmOn) {
    alarmOn = false;
    if (swAlarm) lv_obj_clear_state(swAlarm, LV_STATE_CHECKED);
    cfgTouched();
    Serial.println("[alarm] once - disarmed for tomorrow");
  }
  // The screen may never have been opened, and an alarm is exactly
  // when it has to appear by itself.
  if (!scrAlarm) { buildAlarm(); lvMem("alarm"); }
  if (scrAlarm) lv_scr_load(scrAlarm);
  refreshAlarm();
}

static void stopRinging() {
  ringing = false;
  alarmIconShake(false);      // put the bells back straight
  refreshAlarm();
  Serial.println("[alarm] stopped");
}

static void evAlarmSnooze(lv_event_t *e) {
  ringing     = false;
  alarmIconShake(false);
  int base    = now.hour * 60 + now.min;
  snoozeUntil = (base + ALARM_SNOOZE_MIN) % 1440;   // crosses midnight by itself
  refreshAlarm();
  Serial.printf("[alarm] snooze to %02d:%02d\n", snoozeUntil / 60, snoozeUntil % 60);
}

static void evAlarmStop(lv_event_t *e) { snoozeUntil = -1; stopRinging(); }

static void evAlarmHUp(lv_event_t *e) { alarmMin = (alarmMin + 60) % 1440; refreshAlarm(); cfgTouched(); }
static void evAlarmHDn(lv_event_t *e) { alarmMin = (alarmMin + 1380) % 1440; refreshAlarm(); cfgTouched(); }
static void evAlarmMUp(lv_event_t *e) { alarmMin = (alarmMin / 60) * 60 + (alarmMin % 60 + 1) % 60;  refreshAlarm(); cfgTouched(); }
static void evAlarmMDn(lv_event_t *e) { alarmMin = (alarmMin / 60) * 60 + (alarmMin % 60 + 59) % 60; refreshAlarm(); cfgTouched(); }

static void evAlarmRepeat(lv_event_t *e) {
  // Checked means every day, so the switch reads the way the label
  // does rather than inverted.
  alarmOnce = !lv_obj_has_state(lv_event_get_target(e), LV_STATE_CHECKED);
  cfgTouched();
  refreshAlarm();
  Serial.printf("[alarm] %s\n", alarmOnce ? "once" : "every day");
}

static void evAlarmSwitch(lv_event_t *e) {
  alarmOn = lv_obj_has_state(lv_event_get_target(e), LV_STATE_CHECKED);
  cfgTouched();
  if (!alarmOn) { snoozeUntil = -1; ringing = false; }
  refreshAlarm();
  Serial.printf("[alarm] %s\n", alarmOn ? "armed" : "off");
}

// Once a second, from sensorTick.
static void alarmTick() {
  // A clock that was never set returns a meaningless number, and an
  // alarm against it fires at random. Same check the display uses.
  if (!now.valid || now.wasStopped) { ringing = false; return; }

  int mod = now.hour * 60 + now.min;

  if (alarmFirstTick) {
    // Record the minute without acting on it, so powering up inside
    // the alarm minute does not set it off.
    lastMinSeen = mod;
    alarmFirstTick = false;
  } else if (mod != lastMinSeen) {
    lastMinSeen = mod;
    // On the minute CHANGING, not matching - matching would
    // re-trigger every second for a full minute.
    int target = (snoozeUntil >= 0) ? snoozeUntil : (alarmOn ? alarmMin : -1);
    if (target >= 0 && mod == target) startRinging();
  }

  if (!ringing) return;

  if (millis() - ringStart > ALARM_GIVEUP_MS) {
    Serial.println("[alarm] gave up");
    stopRinging();
    return;
  }
  if (millis() - lastRingPulse >= ALARM_RING_EVERY) {
    lastRingPulse = millis();
    hapticFire(ALARM_RING_PCT, ALARM_RING_MS);   // ignores the touch settings
    alarmIconShake(true);
  }
}

static void buildAlarm() {
  scrAlarm = lv_obj_create(NULL);
  navBack(scrAlarm, "Alarm");

  swAlarm = makeToggle(scrAlarm, evAlarmSwitch, alarmOn);
  lv_obj_align(swAlarm, LV_ALIGN_TOP_RIGHT, -28, 14);

  lv_obj_t *swCap = lv_label_create(scrAlarm);
  lv_label_set_text(swCap, "Alarm");
  lv_obj_set_style_text_font(swCap, &lv_font_montserrat_16, 0);
  lv_obj_set_style_text_color(swCap, lv_color_hex(0x8A94A0), 0);
  lv_obj_align(swCap, LV_ALIGN_TOP_RIGHT, -252, 36);

  // Added in TEST 034 and never actually built: that edit targeted a
  // label removed in TEST 030, found nothing, and changed nothing.
  // The flag and its EEPROM byte were there the whole time; only the
  // control was missing.
  swRepeat = makeToggle(scrAlarm, evAlarmRepeat, !alarmOnce);
  lv_obj_align(swRepeat, LV_ALIGN_TOP_RIGHT, -28, 92);

  lv_obj_t *rcap = lv_label_create(scrAlarm);
  lv_label_set_text(rcap, "Every day");
  lv_obj_set_style_text_font(rcap, &lv_font_montserrat_16, 0);
  lv_obj_set_style_text_color(rcap, lv_color_hex(0x8A94A0), 0);
  lv_obj_align(rcap, LV_ALIGN_TOP_RIGHT, -252, 114);

  // Absolute positions, not a flex row: a flex row lays its
  // children out in a line and cannot put a heading over one of its
  // own columns. Same reason the Clock tab was rebuilt this way.
  //
  // The block is 620 wide and 168 tall (title + buttons), centred
  // on the SCREEN - so it sits level with the middle of the glass,
  // not with whatever space the switches left over.
  {
    const int BW = 620, BH = 168;
    const int BX = (1024 - BW) / 2;          // 202
    const int BY = (600  - BH) / 2;          // 216
    // group: [-] 96, value 96, [+] 96 with 12 px gaps -> 300 wide
    struct { const char *cap; int x; lv_event_cb_t dn, up; lv_obj_t **lbl; }
    g[2] = {
      { "Hour",   BX,       evAlarmHDn, evAlarmHUp, &lblAlarmH },
      { "Minute", BX + 320, evAlarmMDn, evAlarmMUp, &lblAlarmM },
    };
    for (int i = 0; i < 2; i++) {
      lv_obj_t *cap = lv_label_create(scrAlarm);
      lv_label_set_text(cap, g[i].cap);
      lv_obj_set_style_text_font(cap, &lv_font_montserrat_16, 0);
      lv_obj_set_style_text_color(cap, lv_color_hex(0xD8DEE8), 0);
      lv_obj_align(cap, LV_ALIGN_TOP_LEFT, g[i].x + 128, BY);

      lv_obj_t *dn = setBtn(scrAlarm, 96, 96, "-", g[i].dn, 0x2A3040);
      lv_obj_align(dn, LV_ALIGN_TOP_LEFT, g[i].x, BY + 30);

      *g[i].lbl = setBigLabel(scrAlarm, "--");
      lv_obj_align(*g[i].lbl, LV_ALIGN_TOP_LEFT, g[i].x + 126, BY + 56);

      lv_obj_t *up = setBtn(scrAlarm, 96, 96, "+", g[i].up, 0x2A3040);
      lv_obj_align(up, LV_ALIGN_TOP_LEFT, g[i].x + 204, BY + 30);
    }

    lv_obj_t *colon = setBigLabel(scrAlarm, ":");
    lv_obj_align(colon, LV_ALIGN_TOP_LEFT, BX + 306, BY + 56);
  }

  lblAlarmState = lv_label_create(scrAlarm);
  lv_obj_set_style_text_font(lblAlarmState, &lv_font_montserrat_26, 0);
  lv_obj_align(lblAlarmState, LV_ALIGN_TOP_MID, 0, 396);

  btnSnooze = setBtn(scrAlarm, 280, 80, "Snooze 9", evAlarmSnooze, 0x1F5A8A);
  lv_obj_align(btnSnooze, LV_ALIGN_BOTTOM_LEFT, 60, -40);

  btnStopRing = setBtn(scrAlarm, 280, 80, "Stop", evAlarmStop, 0x8F1F1F);
  lv_obj_align(btnStopRing, LV_ALIGN_BOTTOM_RIGHT, -60, -40);

  lv_obj_t *note = lv_label_create(scrAlarm);
  lv_label_set_text(note,
    "It rings by pulsing the panel motor. A motor on the headboard is\n"
    "a weak alarm and may not wake you. The mattress motors over\n"
    "RS-485 are the real answer, and give each side its own.");
  lv_obj_set_style_text_font(note, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(note, lv_color_hex(0x66707E), 0);
  lv_obj_align(note, LV_ALIGN_BOTTOM_MID, 0, -130);

  refreshAlarm();
}

// ------------------------------------------------------------

static void buildSettings() {
  scrSettings = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(scrSettings, lv_color_hex(0x0D0F16), 0);
  lv_obj_clear_flag(scrSettings, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_event_cb(scrSettings, evAnyPress, LV_EVENT_PRESSED, NULL);
  lv_obj_add_event_cb(scrSettings, evSettingsShown,
                      LV_EVENT_SCREEN_LOAD_START, NULL);

  lv_obj_t *back = setBtn(scrSettings, 56, 56, "<", evGoHome, BACK_COL);
  lv_obj_align(back, LV_ALIGN_TOP_LEFT, 14, 8);

  lv_obj_t *ttl = lv_label_create(scrSettings);
  lv_label_set_text(ttl, "Settings");
  lv_obj_set_style_text_font(ttl, &lv_font_montserrat_26, 0);
  lv_obj_set_style_text_color(ttl, lv_color_hex(SET_TEXT), 0);
  lv_obj_align(ttl, LV_ALIGN_TOP_MID, 0, 20);

  lv_obj_t *tv = lv_tabview_create(scrSettings, LV_DIR_TOP, 52);
  lv_obj_set_size(tv, SCREEN_W, SCREEN_H - 72);
  lv_obj_align(tv, LV_ALIGN_BOTTOM_MID, 0, 0);

  // LVGL's default theme leaves a tabview near white. On this panel
  // at night that is a slab of light in a dark room, so every part
  // is painted explicitly. Learned the hard way in TEST 073.
  lv_obj_set_style_bg_color(tv, lv_color_hex(SET_PAGE_BG), 0);
  lv_obj_set_style_bg_opa(tv, LV_OPA_COVER, 0);

  lv_obj_t *bar = lv_tabview_get_tab_btns(tv);
  lv_obj_set_style_bg_color(bar, lv_color_hex(SET_BAR_BG), 0);
  lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, 0);
  lv_obj_set_style_text_color(bar, lv_color_hex(SET_DIM), 0);
  lv_obj_set_style_text_font(bar, &lv_font_montserrat_16, 0);
  // LVGL declares LV_PART_* and LV_STATE_* as two separate anonymous
  // enums, and this toolchain deprecates OR-ing across enum types.
  // Cast to what the function actually takes.
  const lv_style_selector_t ITEM_CHECKED =
      (lv_style_selector_t)LV_PART_ITEMS | (lv_style_selector_t)LV_STATE_CHECKED;

  lv_obj_set_style_text_color(bar, lv_color_hex(SET_TEXT), ITEM_CHECKED);
  lv_obj_set_style_bg_color(bar, lv_color_hex(SET_PAGE_BG), ITEM_CHECKED);
  lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, ITEM_CHECKED);

  lv_obj_t *tClock = lv_tabview_add_tab(tv, "Clock");
  lv_obj_t *tDisp  = lv_tabview_add_tab(tv, "Display");
  lv_obj_t *tTouch = lv_tabview_add_tab(tv, "Touch");

  lv_obj_t *pages[3] = { tClock, tDisp, tTouch };
  for (int i = 0; i < 3; i++) {
    lv_obj_set_style_bg_color(pages[i], lv_color_hex(SET_PAGE_BG), 0);
    lv_obj_set_style_bg_opa(pages[i], LV_OPA_COVER, 0);
    lv_obj_set_style_text_color(pages[i], lv_color_hex(SET_TEXT), 0);
    lv_obj_clear_flag(pages[i], LV_OBJ_FLAG_SCROLLABLE);
  }
  lv_obj_t *cont = lv_tabview_get_content(tv);
  lv_obj_set_style_bg_color(cont, lv_color_hex(SET_PAGE_BG), 0);
  lv_obj_set_style_bg_opa(cont, LV_OPA_COVER, 0);

  // ---------------- Clock ----------------

  lblNowTime = lv_label_create(tClock);
  lv_label_set_text(lblNowTime, "--:--:--");
  lv_obj_set_style_text_font(lblNowTime, &lv_font_montserrat_26, 0);
  lv_obj_set_style_text_color(lblNowTime, lv_color_hex(SET_DIM), 0);
  lv_obj_align(lblNowTime, LV_ALIGN_TOP_MID, 0, 4);

  // Every plus/minus group carries its TITLE ABOVE IT - Hour,
  // Minute, Day, Month, Year - brighter and larger than the old
  // small grey captions that used to trail the year row. Absolute
  // positions, because a flex row cannot put a heading over its own
  // column.
  {
    struct Col { const char *cap; int x; int w; };
    // time row: [-] 76, value 96, [+] 76 with 8 px gaps -> 264 wide
    static const Col tc[2] = { { "Hour", 150, 264 }, { "Minute", 470, 264 } };
    lv_event_cb_t tdn[2] = { evSetHDn, evSetMiDn };
    lv_event_cb_t tup[2] = { evSetHUp, evSetMiUp };
    lv_obj_t **tlb[2] = { &lblSetH, &lblSetMi };
    for (int i = 0; i < 2; i++) {
      lv_obj_t *cap = lv_label_create(tClock);
      lv_label_set_text(cap, tc[i].cap);
      lv_obj_set_style_text_font(cap, &lv_font_montserrat_16, 0);
      lv_obj_set_style_text_color(cap, lv_color_hex(0xD8DEE8), 0);
      lv_obj_align(cap, LV_ALIGN_TOP_LEFT, tc[i].x + tc[i].w / 2 - 24, 40);

      lv_obj_t *dn = setBtn(tClock, 76, 72, "-", tdn[i], 0x2A3040);
      lv_obj_align(dn, LV_ALIGN_TOP_LEFT, tc[i].x, 66);
      *tlb[i] = setBigLabel(tClock, "--");
      lv_obj_align(*tlb[i], LV_ALIGN_TOP_LEFT, tc[i].x + 100, 84);
      lv_obj_t *up = setBtn(tClock, 76, 72, "+", tup[i], 0x2A3040);
      lv_obj_align(up, LV_ALIGN_TOP_LEFT, tc[i].x + 188, 66);
    }
    lv_obj_t *colon = setBigLabel(tClock, ":");
    lv_obj_align(colon, LV_ALIGN_TOP_LEFT, 432, 84);

    // date row: [-] 64, value 88, [+] 64 -> 232 wide
    static const Col dc[3] = { { "Day", 90, 232 }, { "Month", 396, 232 },
                               { "Year", 702, 232 } };
    lv_event_cb_t ddn[3] = { evSetDDn, evSetMoDn, evSetYDn };
    lv_event_cb_t dup[3] = { evSetDUp, evSetMoUp, evSetYUp };
    lv_obj_t **dlb[3] = { &lblSetD, &lblSetMo, &lblSetY };
    for (int i = 0; i < 3; i++) {
      lv_obj_t *cap = lv_label_create(tClock);
      lv_label_set_text(cap, dc[i].cap);
      lv_obj_set_style_text_font(cap, &lv_font_montserrat_16, 0);
      lv_obj_set_style_text_color(cap, lv_color_hex(0xD8DEE8), 0);
      lv_obj_align(cap, LV_ALIGN_TOP_LEFT, dc[i].x + dc[i].w / 2 - 24, 160);

      lv_obj_t *dn = setBtn(tClock, 64, 66, "-", ddn[i], 0x2A3040);
      lv_obj_align(dn, LV_ALIGN_TOP_LEFT, dc[i].x, 186);
      *dlb[i] = setBigLabel(tClock, "--");
      lv_obj_align(*dlb[i], LV_ALIGN_TOP_LEFT, dc[i].x + 84, 202);
      lv_obj_t *up = setBtn(tClock, 64, 66, "+", dup[i], 0x2A3040);
      lv_obj_align(up, LV_ALIGN_TOP_LEFT, dc[i].x + 168, 186);
    }
  }

  // My big TEST 050 edit swallowed this button along with the flex
  // rows it replaced - caught by the after-check, restored here.
  lv_obj_t *sb = setBtn(tClock, 360, 72, "Set time and date",
                        evDoSetTime, SET_GREEN);
  lv_obj_align(sb, LV_ALIGN_TOP_MID, 0, 282);

  // The 12 hour switch lives at the BOTTOM now, caption under it.
  sw12h = makeToggle(tClock, ev12h, use12h);
  lv_obj_align(sw12h, LV_ALIGN_BOTTOM_RIGHT, -28, -46);

  lv_obj_t *c12 = lv_label_create(tClock);
  lv_label_set_text(c12, "12 hour (AM / PM)");
  lv_obj_set_style_text_font(c12, &lv_font_montserrat_16, 0);
  lv_obj_set_style_text_color(c12, lv_color_hex(0xD8DEE8), 0);
  lv_obj_align(c12, LV_ALIGN_BOTTOM_RIGHT, -50, -14);

  lblSetStatus = lv_label_create(tClock);
  lv_label_set_text(lblSetStatus, "");
  lv_obj_set_style_text_font(lblSetStatus, &lv_font_montserrat_16, 0);
  lv_obj_align(lblSetStatus, LV_ALIGN_TOP_MID, 0, 352);

  // ---------------- Display ----------------

  swAuto = makeToggle(tDisp, evAutoBright, autoBright);
  lv_obj_align(swAuto, LV_ALIGN_TOP_RIGHT, -28, 10);

  // Under the switch, not beside it, and named for what it does.
  lv_obj_t *au = lv_label_create(tDisp);
  lv_label_set_text(au, "Auto brightness");
  lv_obj_set_style_text_font(au, &lv_font_montserrat_16, 0);
  lv_obj_set_style_text_color(au, lv_color_hex(0xD8DEE8), 0);
  lv_obj_align(au, LV_ALIGN_TOP_RIGHT, -60, 82);

  lv_obj_t *mn = lv_label_create(tDisp);
  lv_label_set_text(mn, "Darkest  (in a dark room)");
  lv_obj_set_style_text_font(mn, &lv_font_montserrat_16, 0);
  lv_obj_align(mn, LV_ALIGN_TOP_LEFT, 28, 18);

  sldBlMin = lv_slider_create(tDisp);
  lv_obj_set_size(sldBlMin, 700, 26);
  lv_slider_set_range(sldBlMin, 1, 100);
  lv_slider_set_value(sldBlMin, blMin, LV_ANIM_OFF);
  lv_obj_align(sldBlMin, LV_ALIGN_TOP_LEFT, 28, 46);
  lv_obj_add_event_cb(sldBlMin, evBlMin, LV_EVENT_VALUE_CHANGED, NULL);

  lblBlMin = lv_label_create(tDisp);
  lv_label_set_text_fmt(lblBlMin, "%u%%", blMin);
  lv_obj_set_style_text_font(lblBlMin, &lv_font_montserrat_26, 0);
  lv_obj_set_style_text_color(lblBlMin, lv_color_hex(SET_TEXT), 0);
  lv_obj_align(lblBlMin, LV_ALIGN_TOP_LEFT, 620, 38);

  lv_obj_t *mx = lv_label_create(tDisp);
  lv_label_set_text(mx, "Brightest  (also the manual level when Auto is off)");
  lv_obj_set_style_text_font(mx, &lv_font_montserrat_16, 0);
  lv_obj_align(mx, LV_ALIGN_TOP_LEFT, 28, 94);

  sldBlMax = lv_slider_create(tDisp);
  lv_obj_set_size(sldBlMax, 700, 26);
  lv_slider_set_range(sldBlMax, 1, 100);
  lv_slider_set_value(sldBlMax, blMax, LV_ANIM_OFF);
  lv_obj_align(sldBlMax, LV_ALIGN_TOP_LEFT, 28, 122);
  lv_obj_add_event_cb(sldBlMax, evBlMax, LV_EVENT_VALUE_CHANGED, NULL);

  lblBlMax = lv_label_create(tDisp);
  lv_label_set_text_fmt(lblBlMax, "%u%%", blMax);
  lv_obj_set_style_text_font(lblBlMax, &lv_font_montserrat_26, 0);
  lv_obj_set_style_text_color(lblBlMax, lv_color_hex(SET_TEXT), 0);
  lv_obj_align(lblBlMax, LV_ALIGN_TOP_LEFT, 620, 114);

  lv_obj_t *sp = lv_label_create(tDisp);
  lv_label_set_text(sp, "Auto brightness - response speed  (1 gentle, 10 immediate)");
  lv_obj_set_style_text_font(sp, &lv_font_montserrat_16, 0);
  lv_obj_align(sp, LV_ALIGN_TOP_LEFT, 28, 170);

  sldSpeed = lv_slider_create(tDisp);
  lv_obj_set_size(sldSpeed, 700, 26);
  lv_slider_set_range(sldSpeed, AUTO_SPEED_MIN, AUTO_SPEED_MAX);
  lv_slider_set_value(sldSpeed, autoSpeed, LV_ANIM_OFF);
  lv_obj_align(sldSpeed, LV_ALIGN_TOP_LEFT, 28, 198);
  lv_obj_add_event_cb(sldSpeed, evAutoSpeed, LV_EVENT_VALUE_CHANGED, NULL);

  lblAutoSpeed = lv_label_create(tDisp);
  lv_label_set_text_fmt(lblAutoSpeed, "%u", autoSpeed);
  lv_obj_set_style_text_font(lblAutoSpeed, &lv_font_montserrat_26, 0);
  lv_obj_set_style_text_color(lblAutoSpeed, lv_color_hex(SET_TEXT), 0);
  lv_obj_align(lblAutoSpeed, LV_ALIGN_TOP_LEFT, 620, 190);

  lv_obj_t *sv = lv_label_create(tDisp);
  lv_label_set_text(sv, "Screensaver after");
  lv_obj_set_style_text_font(sv, &lv_font_montserrat_16, 0);
  lv_obj_align(sv, LV_ALIGN_TOP_LEFT, 28, 246);

  sldSaverAfter = lv_slider_create(tDisp);
  lv_obj_set_size(sldSaverAfter, 560, 26);
  lv_slider_set_range(sldSaverAfter, 0, 8);
  lv_slider_set_value(sldSaverAfter, saverIdx, LV_ANIM_OFF);
  lv_obj_align(sldSaverAfter, LV_ALIGN_TOP_LEFT, 28, 274);
  lv_obj_add_event_cb(sldSaverAfter, evSaverAfter, LV_EVENT_VALUE_CHANGED, NULL);

  lblSaverAfter = lv_label_create(tDisp);
  lv_label_set_text(lblSaverAfter, kSaverNames[saverIdx]);
  lv_obj_set_style_text_font(lblSaverAfter, &lv_font_montserrat_26, 0);
  lv_obj_set_style_text_color(lblSaverAfter, lv_color_hex(SET_TEXT), 0);
  lv_obj_align(lblSaverAfter, LV_ALIGN_TOP_LEFT, 620, 266);

  // Was also called sb, which collided with the Set time button in
  // the Clock tab - same function, same scope.
  lv_obj_t *sbCap = lv_label_create(tDisp);
  lv_label_set_text(sbCap, "Screensaver brightness");
  lv_obj_set_style_text_font(sbCap, &lv_font_montserrat_16, 0);
  lv_obj_align(sbCap, LV_ALIGN_TOP_LEFT, 28, 322);

  sldSaverBright = lv_slider_create(tDisp);
  lv_obj_set_size(sldSaverBright, 560, 26);
  lv_slider_set_range(sldSaverBright, 1, 60);
  lv_slider_set_value(sldSaverBright, saverBright, LV_ANIM_OFF);
  lv_obj_align(sldSaverBright, LV_ALIGN_TOP_LEFT, 28, 350);
  lv_obj_add_event_cb(sldSaverBright, evSaverBright, LV_EVENT_VALUE_CHANGED, NULL);

  lblSaverBright = lv_label_create(tDisp);
  lv_label_set_text_fmt(lblSaverBright, "%u%%", saverBright);
  lv_obj_set_style_text_font(lblSaverBright, &lv_font_montserrat_26, 0);
  lv_obj_set_style_text_color(lblSaverBright, lv_color_hex(SET_TEXT), 0);
  lv_obj_align(lblSaverBright, LV_ALIGN_TOP_LEFT, 620, 342);

  lblLuxLive = lv_label_create(tDisp);
  lv_label_set_text(lblLuxLive, "room light  -- lx");
  lv_obj_set_style_text_font(lblLuxLive, &lv_font_montserrat_26, 0);
  lv_obj_set_style_text_color(lblLuxLive, lv_color_hex(0x60D0FF), 0);
  lv_obj_align(lblLuxLive, LV_ALIGN_TOP_LEFT, 28, 404);

  lv_obj_t *nowCap = lv_label_create(tDisp);
  lv_label_set_text(nowCap, "backlight now");
  lv_obj_set_style_text_font(nowCap, &lv_font_montserrat_16, 0);
  lv_obj_set_style_text_color(nowCap, lv_color_hex(SET_DIM), 0);
  lv_obj_align(nowCap, LV_ALIGN_TOP_LEFT, 500, 398);

  lblBlVal = lv_label_create(tDisp);
  lv_label_set_text_fmt(lblBlVal, "%u%%", backlight);
  lv_obj_set_style_text_font(lblBlVal, &lv_font_montserrat_26, 0);
  lv_obj_set_style_text_color(lblBlVal, lv_color_hex(SET_TEXT), 0);
  lv_obj_align(lblBlVal, LV_ALIGN_TOP_LEFT, 500, 424);

  // ---------------- Touch ----------------

  lv_obj_t *hv = lv_label_create(tTouch);
  lv_label_set_text(hv, "Pulse strength");
  lv_obj_set_style_text_font(hv, &lv_font_montserrat_16, 0);
  lv_obj_align(hv, LV_ALIGN_TOP_LEFT, 28, 26);

  sldHapV = lv_slider_create(tTouch);
  lv_obj_set_size(sldHapV, 740, 26);
  lv_slider_set_range(sldHapV, 0, 100);
  lv_slider_set_value(sldHapV, hapDutyPct, LV_ANIM_OFF);
  lv_obj_align(sldHapV, LV_ALIGN_TOP_LEFT, 28, 66);
  lv_obj_add_event_cb(sldHapV, evHapVChg, LV_EVENT_VALUE_CHANGED, NULL);
  lv_obj_add_event_cb(sldHapV, evHapTry,  LV_EVENT_RELEASED,      NULL);

  lblHapV = lv_label_create(tTouch);
  lv_label_set_text_fmt(lblHapV, "%u%%", hapDutyPct);
  lv_obj_set_style_text_font(lblHapV, &lv_font_montserrat_26, 0);
  lv_obj_set_style_text_color(lblHapV, lv_color_hex(SET_TEXT), 0);
  lv_obj_align(lblHapV, LV_ALIGN_TOP_LEFT, 800, 56);

  lv_obj_t *hh = lv_label_create(tTouch);
  lv_label_set_text(hh, "Pulse length");
  lv_obj_set_style_text_font(hh, &lv_font_montserrat_16, 0);
  lv_obj_align(hh, LV_ALIGN_TOP_LEFT, 28, 130);

  sldHapH = lv_slider_create(tTouch);
  lv_obj_set_size(sldHapH, 740, 26);
  lv_slider_set_range(sldHapH, 20, 500);
  lv_slider_set_value(sldHapH, hapLenMs, LV_ANIM_OFF);
  lv_obj_align(sldHapH, LV_ALIGN_TOP_LEFT, 28, 170);
  lv_obj_add_event_cb(sldHapH, evHapHChg, LV_EVENT_VALUE_CHANGED, NULL);
  lv_obj_add_event_cb(sldHapH, evHapTry,  LV_EVENT_RELEASED,      NULL);

  lblHapH = lv_label_create(tTouch);
  lv_label_set_text_fmt(lblHapH, "%u ms", hapLenMs);
  lv_obj_set_style_text_font(lblHapH, &lv_font_montserrat_26, 0);
  lv_obj_set_style_text_color(lblHapH, lv_color_hex(SET_TEXT), 0);
  lv_obj_align(lblHapH, LV_ALIGN_TOP_LEFT, 800, 160);

  lv_obj_t *tb = setBtn(tTouch, 260, 68, "Try it", evHapTry, 0x1F5A8A);
  lv_obj_align(tb, LV_ALIGN_TOP_LEFT, 28, 240);

  lv_obj_t *thint = lv_label_create(tTouch);
  lv_label_set_text(thint,
    "The number follows your finger; the buzz waits for you to lift it.\n"
    "Below about 40 ms the motor has not spun up and there is little\n"
    "to feel. Both are kept in the clock module's EEPROM, so they\n"
    "survive a reflash.");
  lv_obj_set_style_text_font(thint, &lv_font_montserrat_16, 0);
  lv_obj_set_style_text_color(thint, lv_color_hex(SET_DIM), 0);
  lv_obj_align(thint, LV_ALIGN_TOP_LEFT, 28, 330);

  lv_obj_t *hint = lv_label_create(tDisp);
  lv_label_set_text(hint,
    "If the brightness pumps up and down on its own, the sensor can "
    "see the screen - move the sensor, not the numbers.");
  lv_obj_set_style_text_font(hint, &lv_font_montserrat_16, 0);
  lv_obj_set_style_text_color(hint, lv_color_hex(SET_DIM), 0);
  lv_obj_align(hint, LV_ALIGN_BOTTOM_LEFT, 28, -8);
}

// ------------------------------------------------------------

static void buildHome() {
  scrHome = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(scrHome, lv_color_hex(0x101418), 0);
  lv_obj_clear_flag(scrHome, LV_OBJ_FLAG_SCROLLABLE);

  // PRESSED, not CLICKED. Pressed fires when the finger lands.
  // Clicked waits for the release, and feedback on release is not
  // feedback.
  lv_obj_add_event_cb(scrHome, evAnyPress, LV_EVENT_PRESSED, NULL);

  lv_obj_t *title = lv_label_create(scrHome);
  lv_label_set_text(title, PANEL_ID == 1 ? "ימש" : "אריע");
  lv_obj_set_style_text_font(title, &font_hebrew_28, 0);
  lv_obj_set_style_text_color(title, lv_color_white(), 0);
  lv_obj_align(title, LV_ALIGN_TOP_LEFT, 24, 20);

  lv_obj_t *tn = lv_label_create(scrHome);
  lv_label_set_text_fmt(tn, "TEST %03d", TEST_NUMBER);
  lv_obj_set_style_text_font(tn, &lv_font_montserrat_16, 0);
  lv_obj_set_style_text_color(tn, lv_color_hex(0x556070), 0);
  lv_obj_align(tn, LV_ALIGN_TOP_MID, 0, 24);

  // The band between the title row and the tiles was empty. The
  // tiles start at y=108 and nothing else lives up here, so the
  // readings cost no layout at all.
  lblClock = lv_label_create(scrHome);
  lv_label_set_text(lblClock, "--:--:--");
  lv_obj_set_style_text_font(lblClock, &lv_font_montserrat_26, 0);
  lv_obj_set_style_text_color(lblClock, lv_color_white(), 0);
  lv_obj_align(lblClock, LV_ALIGN_TOP_MID, 0, 54);

  lblClimate = lv_label_create(scrHome);
  lv_label_set_text(lblClimate, "waiting for the sensors");
  lv_obj_set_style_text_font(lblClimate, &lv_font_montserrat_16, 0);
  lv_obj_set_style_text_color(lblClimate, lv_color_hex(0x8A94A0), 0);
  lv_obj_align(lblClimate, LV_ALIGN_TOP_RIGHT, -24, 58);

  // --- row 0: bed, radio, air conditioner, fan ---
  lv_obj_t *t;

  t = makeTile(0, 0, TILE_BG_OFF);
  tileBedImg = addImg(t, &img_bed_off);
  tiles[tileCount++] = { t, tileBedImg, &bedRunning };
  lv_obj_add_event_cb(t, evBed, LV_EVENT_CLICKED, NULL);

  t = makeTile(1, 0, TILE_BG_OFF);
  tileRadioImg = addImg(t, &img_radio_off);
  tiles[tileCount++] = { t, tileRadioImg, &radioOn };
  // The same badge on the tile, so "am I on a timer" is answered
  // without opening the radio screen at all.
  badgeHome = makeTimerBadge(t);
  lv_obj_align(badgeHome, LV_ALIGN_BOTTOM_MID, 0, -8);
  lv_obj_add_event_cb(t, evRadio, LV_EVENT_CLICKED, NULL);

  t = makeTile(2, 0, TILE_BG_OFF);
  tileACImg = addImg(t, &img_ac_off);
  tiles[tileCount++] = { t, tileACImg, &acPower };
  lv_obj_add_event_cb(t, evTilePress,  LV_EVENT_PRESSED,    NULL);
  lv_obj_add_event_cb(t, evACRelease,  LV_EVENT_RELEASED,   NULL);
  lv_obj_add_event_cb(t, evTileLost,   LV_EVENT_PRESS_LOST, NULL);

  t = makeTile(3, 0, TILE_BG_OFF);
  tileFanImg = addImg(t, &img_fan);
  tiles[tileCount++] = { t, tileFanImg, &fanOn };
  lv_obj_add_event_cb(t, evTilePress,  LV_EVENT_PRESSED,    NULL);
  lv_obj_add_event_cb(t, evFanRelease, LV_EVENT_RELEASED,   NULL);
  lv_obj_add_event_cb(t, evTileLost,   LV_EVENT_PRESS_LOST, NULL);

  // --- row 1: lamp, shutters, settings, spare ---
  t = makeTile(0, 1, TILE_BG_OFF);
  tileLampImg = addImg(t, &img_lamp);
  lv_obj_set_style_img_recolor(tileLampImg, lv_color_hex(0x9098A0), 0);
  tiles[tileCount++] = { t, tileLampImg, &lampOn };
  lv_obj_add_event_cb(t, evLamp, LV_EVENT_CLICKED, NULL);

  t = makeTile(1, 1, TILE_BG_OFF);
  tileShadeImg = addImg(t, &img_shade_open);
  tiles[tileCount++] = { t, tileShadeImg, &shadeOpen };
  lv_obj_add_event_cb(t, evShade, LV_EVENT_CLICKED, NULL);

  t = makeTile(2, 1, 0x232A33);
  addSymbol(t, LV_SYMBOL_SETTINGS, 0x8A94A0);
  lv_obj_add_event_cb(t, evSettings, LV_EVENT_CLICKED, NULL);

  t = makeTile(3, 1, TILE_BG_OFF);
  buildAlarmIcon(t);
  lv_obj_add_event_cb(t, evSpare, LV_EVENT_CLICKED, NULL);

  // Status line under the grid. Nothing is wired to the bus, so
  // every tile says what it WOULD do rather than pretending.
  lblStatus = lv_label_create(scrHome);
  lv_label_set_text(lblStatus, "nothing is wired to the bus yet");
  lv_obj_set_style_text_font(lblStatus, &lv_font_montserrat_16, 0);
  lv_obj_set_style_text_color(lblStatus, lv_color_hex(0x707C88), 0);
  lv_obj_align(lblStatus, LV_ALIGN_BOTTOM_MID, 0, -22);

  // Presence line. Blue while both required addresses answer, red
  // the moment one of them stops - that is a bus fault, and it
  // should be visible from across the room without a serial cable.
  repaintAll();
  lv_scr_load(scrHome);

  // Owned by LVGL, runs in the LVGL task. One second is slow enough
  // that plugging a module in and watching it appear feels live,
  // and cheap enough that thirteen probes cost nothing.
  lv_timer_create(sensorTick, 1000, NULL);
  lv_timer_create(fastTick, AUTO_TICK_MS, NULL);
}

// ------------------------------------------------------------

void setup() {
  Serial.begin(115200);
  RS485.begin(RS485_BAUD, SERIAL_8N1, RS485_RX_PIN, RS485_TX_PIN);
  delay(100);
  while (RS485.available()) RS485.read();
#if PANEL_ID == 1
  Serial.println("RS485 clock source: RX GPIO15 TX GPIO16 115200");
#else
  Serial.println("RS485 remote clock: RX GPIO15 TX GPIO16 115200");
#endif
  delay(500);

  Serial.println();
  Serial.println("=== PANEL IRA - TEST 089 - MATCHED PAIR + SEIKO NIGHT SCREENSAVER ===");
  Serial.printf("psram: %lu free of %lu\n",
                (unsigned long)ESP.getFreePsram(),
                (unsigned long)ESP.getPsramSize());

  static esp_lcd_panel_handle_t panel = NULL;
  static esp_lcd_touch_handle_t tp    = NULL;

  tp    = touch_gt911_init();
  panel = waveshare_esp32_s3_rgb_lcd_init();
  wavesahre_rgb_lcd_bl_on();
  Serial.println(tp    ? "touch ok" : "TOUCH FAILED");
  Serial.println(panel ? "panel ok" : "PANEL FAILED");

  ESP_ERROR_CHECK(lvgl_port_init(panel, tp));

  // Everything that touches the bus now happens INSIDE the lock.
  // lvgl_port_init() has already started the LVGL task, and that
  // task drives the GT911 on these same two wires. Earlier tests
  // did this initialisation outside the lock and got away with it;
  // it was a race waiting to be lost, and it costs nothing to close.
  //
  // The order matters too: the settings are read BEFORE the screens
  // are built, because every slider and switch takes its starting
  // position from a stored value. Loading afterwards would leave
  // each widget showing a default the system does not actually
  // have.
  if (lvgl_port_lock(-1)) {
    DEV_I2C_Set_Slave_Addr(&devRTC, RTC_ADDR);
    DEV_I2C_Set_Slave_Addr(&devSHT, SHT_ADDR);
    DEV_I2C_Set_Slave_Addr(&devLUX, LUX_ADDR);
    DEV_I2C_Set_Slave_Addr(&devEE,  EE_ADDR);
    Serial.printf("handles: rtc %s  sht %s  lux %s  eeprom %s\n",
                  devRTC ? "ok" : "FAIL", devSHT ? "ok" : "FAIL",
                  devLUX ? "ok" : "FAIL", devEE  ? "ok" : "FAIL");

    cfgLoad();
    Serial.println("step: luxBegin");   Serial.flush();  luxBegin();
    Serial.println("step: shtBegin");   Serial.flush();  shtBegin();

    // Serial.flush() before each one matters. Without it the last
    // line can still be sitting in the UART buffer when the CPU
    // panics, and the very step that crashed is the one that never
    // appears - which points the finger at the previous function.
    // Only the home screen at boot. The rest are built the first
    // time they are opened - see evBed, evRadio, evAC, evFan,
    // evSettings and evSpare.
    // The photograph first, so something is on the screen while the
    // rest is built.
    Serial.println("step: bootPhoto");  Serial.flush();  showBootPhoto();

    Serial.println("step: buildHome");  Serial.flush();  buildHome();
    lvMem("home");
    lvgl_port_unlock();
  }

  // Only the level the screen comes up at. With Auto on, the
  // controller takes it over within 100 ms.
  applyBacklight(blMax);
  hapticInit();

  // Held OUTSIDE the LVGL lock. Sleeping with the mutex held would
  // stall the display task for the whole three seconds.
  if (scrBoot) {
    delay(BOOT_PHOTO_MS);
    if (lvgl_port_lock(-1)) {
      if (scrHome) lv_scr_load(scrHome);
      dropBootPhoto();
      lvgl_port_unlock();
    }
    Serial.println("boot photo: done, memory returned");
  }
  lastTouchMs = millis();      // do not count the boot photo as idle time

  Serial.println();
  Serial.println("Eight tiles, 200 px each, native size - no scaling.");
  Serial.println("Every press prints here and on the status line.");
  Serial.println("Judge the spacing and the margins, not the behaviour.");
  Serial.println();
  printHelp();
}

// v is BRIGHTNESS, 0 dark to 100 bright.
// The expander wants the opposite, so it is inverted here and
// nowhere else. Every other line in the project can now talk in
// plain brightness.
// The automatic path calls this once a second. applyBacklight logs
// a line on every call, which at that rate would bury the monitor,
// so the automatic mover uses the quiet one and only the slider and
// the serial command announce themselves.
static void applyBacklightQuiet(uint8_t v) {
  if (v > 100) v = 100;
  backlight = v;
  IO_EXTENSION_Pwm_Output(100 - backlight);
  if (lblBlVal) lv_label_set_text_fmt(lblBlVal, "%u%%", backlight);
}

static void applyBacklight(uint8_t v) {
  applyBacklightQuiet(v);
  Serial.printf("backlight = %u %% brightness  (expander gets %u)\n",
                backlight, 100 - backlight);
}

// Once a second. Everything slow in here is deliberate.
static void autoBrightTick() {
  // The screensaver sets its own level on purpose. Letting the
  // automatic mover walk it back up would light the room at 3 am.
  if (saverOn) return;
  if (!autoBright || !luxOK) return;
  if (blMax < blMin) return;                    // nonsense range, do nothing

  // Exponential average. The first good reading seeds it directly,
  // otherwise the screen would crawl up from zero at every boot.
  if (luxSmooth < 0.0f) luxSmooth = luxVal;
  else luxSmooth += autoAlpha() * (luxVal - luxSmooth);

  // Logarithmic, because the eye is. Linear here would leave the
  // screen dim all evening and pinned at full the moment a lamp
  // comes on.
  float l = luxSmooth;
  if (l < AUTO_LUX_LO) l = AUTO_LUX_LO;
  if (l > AUTO_LUX_HI) l = AUTO_LUX_HI;
  float f = log10f(l / AUTO_LUX_LO) / log10f(AUTO_LUX_HI / AUTO_LUX_LO);

  int target = blMin + (int)((blMax - blMin) * f + 0.5f);
  if (target < blMin) target = blMin;
  if (target > blMax) target = blMax;

  int diff = target - (int)backlight;
  if (diff > -AUTO_DEADBAND && diff < AUTO_DEADBAND) return;  // no hunting

  // Walk, do not jump - a backlight step is visible on this panel -
  // but the size of the stride is yours to set. Clamped to the
  // target so a large stride cannot overshoot and bounce.
  int step = autoStep();
  int next = (diff > 0) ? (int)backlight + step : (int)backlight - step;
  if (diff > 0 && next > target) next = target;
  if (diff < 0 && next < target) next = target;
  if (next < 0)   next = 0;
  if (next > 100) next = 100;
  applyBacklightQuiet((uint8_t)next);
}

// Ten times a second. Only the lux read and the backlight mover
// live here; the RTC and the SHT3x stay on the one second tick,
// where they belong.
static void fastTick(lv_timer_t *t) {
  (void)t;
  luxRead();
  autoBrightTick();
}

// Facts instead of estimates. lv_mem_monitor walks LVGL's own pool,
// which is the pool that ran out.
static void lvMem(const char *what) {
  lv_mem_monitor_t m;
  lv_mem_monitor(&m);
  Serial.printf("lvgl mem after %-14s used %6u of %6u  (%u%%)  "
                "largest free block %u\n",
                what, (unsigned)(m.total_size - m.free_size),
                (unsigned)m.total_size, (unsigned)m.used_pct,
                (unsigned)m.free_biggest_size);
  Serial.flush();
}

static void printHelp() {
  Serial.println();
  Serial.println("  n<v>   dim when ON    0..255   0 = full colour");
  Serial.println("  f<v>   dim when OFF   0..255   255 = black");
  Serial.println("  b<v>   backlight      0..100   100 = brightest");
  Serial.println("  i      print the I2C table now");
  Serial.println("  m      LVGL memory in use");
  Serial.println("  v<pct> haptic strength 0-100, fires a test tick");
  Serial.println("  h<ms>  haptic pulse length, fires a test tick");
  Serial.println("  T<...> set the clock:  T2026-09-03 13:45:00");
  Serial.println("  s      full sweep 0x08..0x77");
  Serial.println("  ?      this help");
  Serial.printf("  now: on %u, off %u, backlight %u %%\n",
                dimOn, dimOff, backlight);
  Serial.println();
  Serial.println("  The expander's PWM is inverted and the code already");
  Serial.println("  compensates. These numbers are plain brightness.");
}

void loop() {
#if PANEL_ID == 1
  rs485ClockSendTick();
#else
  rs485ClockPoll();
#endif
  static String line = "";

  while (Serial.available()) {
    char c = (char)Serial.read();

    if (c == '\n' || c == '\r') {
      Serial.println();                 // echo, in case the filter is off
      if (line.length()) {
        line.trim();
        line.toLowerCase();
        if (line == "?") {
          printHelp();
        } else if (line.charAt(0) == 't' && line.length() >= 19) {
          // T2026-09-03 13:45:00
          int  Y = line.substring(1, 5).toInt();
          int  M = line.substring(6, 8).toInt();
          int  D = line.substring(9, 11).toInt();
          int  h = line.substring(12, 14).toInt();
          int  m = line.substring(15, 17).toInt();
          int  sec = (line.length() >= 20) ? line.substring(18, 20).toInt() : 0;
          if (rtcSet((uint16_t)Y, (uint8_t)M, (uint8_t)D,
                     (uint8_t)h, (uint8_t)m, (uint8_t)sec)) {
            Serial.printf("clock set: %04d-%02d-%02d %02d:%02d:%02d  (%s)\n",
                          Y, M, D, h, m, sec,
                          kDow[dayOfWeek((uint16_t)Y, (uint8_t)M, (uint8_t)D)]);
          } else {
            Serial.println("  rejected. format: T2026-09-03 13:45:00");
          }
        } else if (line.charAt(0) == 'v' && line.length() > 1) {
          int p = line.substring(1).toInt();
          if (p < 0) p = 0;
          if (p > 100) p = 100;
          hapDutyPct = (uint8_t)p;
          Serial.printf("  haptic strength %u%%\n", hapDutyPct);
          cfgTouched();
          hapticPulse();                       // feel it immediately
        } else if (line.charAt(0) == 'h' && line.length() > 1) {
          int ms = line.substring(1).toInt();
          if (ms < 5) ms = 5;
          if (ms > 2000) ms = 2000;
          hapLenMs = (uint16_t)ms;
          Serial.printf("  haptic length %u ms\n", hapLenMs);
          cfgTouched();
          hapticPulse();
        } else if (line == "m") {
          lvMem("request");
        } else if (line == "i") {
          // loop() is the Arduino task. The bus belongs to the LVGL
          // task, where the touch driver also lives. So the request
          // is flagged here and served there - one task on the bus,
          // always.
          scanWanted = true;
          Serial.println("  scanning on the next tick");
        } else if (line == "s") {
          sweepWanted = true;
          Serial.println("  sweeping on the next tick");
        } else if (line.charAt(0) == 'n') {
          int v = line.substring(1).toInt();
          if (v >= 0 && v <= 255) {
            dimOn = (uint8_t)v; repaintAll();
            Serial.printf("dim ON  = %u\n", dimOn);
          } else Serial.println("  0..255");
        } else if (line.charAt(0) == 'f') {
          int v = line.substring(1).toInt();
          if (v >= 0 && v <= 255) {
            dimOff = (uint8_t)v; repaintAll();
            Serial.printf("dim OFF = %u\n", dimOff);
          } else Serial.println("  0..255");
        } else if (line.charAt(0) == 'b') {
          int v = line.substring(1).toInt();
          if (v >= 0 && v <= 100) {
            // A manual command has to win, or the automatic mover
            // would walk it straight back within seconds.
            autoBright = false;
            if (swAuto) lv_obj_clear_state(swAuto, LV_STATE_CHECKED);
            blMax = (uint8_t)v;
            if (sldBlMax) lv_slider_set_value(sldBlMax, blMax, LV_ANIM_OFF);
            if (lblBlMax) lv_label_set_text_fmt(lblBlMax, "%u%%", blMax);
            applyBacklight((uint8_t)v);
            Serial.println("  auto brightness switched off by hand");
            cfgTouched();
          }
          else Serial.println("  backlight is 0..100");
        } else {
          Serial.println("  n<v>, f<v>, b<v>, v<pct>, h<ms>, i, s, m, T<time> or ?");
        }
        line = "";
      }
    } else if (line.length() < 40) {
      line += c;
      Serial.print(c);                  // echo each character as typed
    }
  }
  delay(20);
}

// ============================================================
// END PANEL IRA - TEST 083 - 7B FULL UI + RS485 REMOTE CLOCK
// ============================================================
