// ============================================================
//                        TEST  078
// ============================================================
//  IRA PANEL - Waveshare ESP32-S3-Touch-LCD-4.3
//  ESP32-S3-WROOM-1 N16R8, 800x480 RGB, GT911 touch, CH422G expander
//
//  WHAT CHANGED IN TEST 048  -  SCREENSAVER POLISH
//
//    Lowercase c on the temperature; dial + hands dimmed ~20%; the
//    arrow head is 1.5x wider with a small green dot in its middle.
//
//  WHAT CHANGED IN TEST 047  -  BIGGER TEMPERATURE, MOVED DOWN
//
//    The screensaver temperature grows ~2 mm (temp_big regenerated at
//    100 px, was 75) and drops 3 mm (y 40 -> 65). Nothing else touched.
//
//  WHAT CHANGED IN TEST 046  -  DIM THE DREAM ICON ON THE SAVER
//
//    The dreamcatcher on the screensaver is recoloured toward black at
//    ~55% (so ~45% brightness) so it does not glare at night. Render
//    time only - the image is untouched, and the Dreams tile and the
//    header keep their own look.
//
//  WHAT CHANGED IN TEST 045  -  BARBED CLOCK HANDS
//
//    The screensaver hour + minute hands are now rotated images with a
//    filled barbed (Casio/Seiko-diver) arrow tip - base 1.5x wide. The
//    round dial markings are untouched (they live in img_dial). Second
//    hand stays a thin red line. UNTESTED: if the hands point the wrong
//    way on the real screen, the fix is one line (angle direction).
//
//  WHAT CHANGED IN TEST 044  -  NEW DREAM ICON (SLEEPING FIGURE)
//
//    dream_img.h now holds the sleeping-figure drawing instead of the
//    dreamcatcher photo. Same 175x175 dark-ground format, so the
//    screensaver, the Dreams tile and everything using img_dream just
//    show the new icon - no code change beyond this note.
//
//  WHAT CHANGED IN TEST 043  -  SCREENSAVER LAYOUT NUDGE
//
//    On the screensaver the clock moves 5 mm (43 px) to the right, and
//    the temperature and the dreamcatcher move 5 mm to the left. Just
//    positions - nothing else changed.
//
//  WHAT CHANGED IN TEST 042  -  FAN TILE OFF/ON PHOTOS
//
//    The fan tile now swaps between two photos made from the real
//    ceiling fan: off (light dark, room dimmed) and on (centre light
//    glowing warm) - exactly like the AC tile. No more brightness
//    trick; a real two-photo toggle driven by fan power.
//
//  WHAT CHANGED IN TEST 041  -  FAN TILE + FAN CONTROL SCREEN
//
//    The fan tile now behaves like the AC: a short press toggles the
//    fan on/off, a 3 s hold opens a fan control screen with power, six
//    speed buttons, an off-timer (Off/1h/2h/3h) and a Light toggle.
//    The tile still uses img_fan (dim when off, bright when on) until
//    the "fan on" photo arrives, then it becomes a two-photo toggle.
//
//  WHAT CHANGED IN TEST 040  -  SHADE TILE + AC LIGHT COMMAND FIX
//
//    The shade tile now flips open<->closed on tap and starts a 20 s
//    travel (second press sends STOP), copied from panel_shemi. Also
//    fixed the AC Light command number: it was 16, the same as the AC
//    off-timer (CMD_AC_TIMER), so Light is now 17. No Room screen here.
//
//  WHAT CHANGED IN TEST 039  -  AC DEFAULTS ON WHEN THE SCREEN OPENS
//
//    Holding the AC tile 3 s to open the control screen now turns the
//    AC ON by default (if it was off), and refreshes the room
//    temperature (live from the SHT31 on the I2C bus) at the same time.
//
//  WHAT CHANGED IN TEST 038  -  AC OFF-TIMER (1h / 2h / 3h)
//
//    The AC control screen gains a Timer row: Off / 1h / 2h / 3h, in
//    the same style as mode and fan. Sends CMD_AC_TIMER (stubbed).
//
//  WHAT CHANGED IN TEST 037  -  AC SCREEN COPIED VERBATIM FROM SHEMI
//
//    The invented AC screen is gone. buildAC / refreshAC / handlers /
//    mkBtn are now copied exactly from panel_shemi - same layout, room
//    temperature, mode/fan/swing. Only the bus send is a serial stub.
//
//    The AC control screen gains a "Swing" toggle - the vane at the
//    bottom of the unit that spreads the air. Blue when on.
//
//  WHAT CHANGED IN TEST 035  -  LIGHT ON/OFF + FULL AC CONTROL SCREEN
//
//    The light tile now recolours the lamp photo grey (off) / amber
//    (on), toggled on tap. The AC tile: a short press toggles off/on;
//    a 3-second hold opens a full AC control screen (temperature +/-,
//    mode, fan speed). All UI only - commands print to serial.
//
//  WHAT CHANGED IN TEST 034  -  TEMPERATURE HALVED + SOFT LIGHT BLUE
//
//    temp_big regenerated at 75 px (half of 150) - digits ~54 px, a
//    comfortable middle size. Colour is now a soft light blue
//    (0x88AACC) so it does not glare in the dark.
//
//  WHAT CHANGED IN TEST 033  -  BIG TEMPERATURE + FULL AC REMOTE
//
//    Screensaver temperature is now ~108 px (temp_big, a real LVGL
//    font from DejaVuSans-Bold) - readable across the room. The AC
//    tile now shows the FULL remote; the OFF state has a plain grey
//    screen with no writing, the ON state the orange-lit screen.
//
//  WHAT CHANGED IN TEST 032  -  AC TILE, OFF / ON
//
//    The AC tile now shows the Tadiran remote: grey screen (off) or
//    orange-lit screen (on), softened so it is not glossy. A short
//    press toggles it and prints the command it WOULD send. The
//    3-second long-press -> full AC screen comes in a later step.
//
//  WHAT CHANGED IN TEST 031  -  ANALOG CLOCK ON THE SCREENSAVER
//
//    The screensaver now shows the analog clock face (from Shemi's
//    dial) with hands on the LEFT, and the dreamcatcher + temperature
//    on the RIGHT. The hands are a placeholder 10:10 until real time
//    arrives over the bus.
//
//  WHAT CHANGED IN TEST 030  -  SCREENSAVER WITH THE DREAMSAVER
//
//    After SAVER_IDLE_MS of no touch, the home fades to a dark
//    screensaver: real temperature from the SHT31, a placeholder
//    clock (real time arrives over the bus later), and the
//    dreamcatcher. HOLD the dreamcatcher to record a dream straight
//    from the screensaver; tap anywhere else to wake back home.
//    Recording is still the shell (audio needs the listener + bus).
//
//  WHAT CHANGED IN TEST 029  -  DREAMSAVER (UI shell)
//
//    The empty (3,1) slot now holds a dreamcatcher tile that opens a
//    Dreams screen with two tiles: "Record Dream" (press-and-hold)
//    and "Play Dreams". Interface only - the audio belongs to the
//    listener + audionode over the RS-485 bus, not built yet. The
//    07:00 gate on Play is inactive here (panel_ira has no clock).
//
//  WHAT CHANGED IN TEST 028  -  THE SAME SCREEN AS SHEMI'S
//
//    The home screen is now a copy of panel_shemi's, down to the
//    pixel: a 4 x 2 grid of 175 px tiles, 20 px apart across and 16
//    down, starting 20 from the left and 74 from the top.
//
//      row 0:  massage   radio   air conditioning   fan
//      row 1:  light     shades  settings           -
//
//    Same artwork, same order, same colours, same eighth slot left
//    empty. You two swap sides of the bed, and a panel that looks
//    different depending on which side you woke up on is worse than
//    no panel at all.
//
//    GONE: the "info" button and the brightness slider. Brightness is
//    parked - the honest answer there is a wire to the MP3302's EN
//    pin, and until that is done a slider that cannot really dim is
//    just something to be disappointed by.
//
//    Every tile opens the "not yet" screen. None of them do anything
//    else yet, and that is deliberate: the layout is settled now, so
//    the screens behind it can be filled in one at a time without the
//    front page ever moving again.
//
//    NEEDS ONE LINE IN platformio.ini:  -DLV_FONT_MONTSERRAT_40=1
//    (the settings tile draws its gear at 40 px, same as Shemi's)
//
//  WHAT CHANGED IN TEST 027  -  THE FAN HAS ITS PICTURE
//
//    Made from the photograph of the fan actually hanging in the
//    room. White blades on a white background, so they were cut out
//    by luminance rather than colour, then redrawn in off-white on a
//    slate-violet ground - the same treatment and the same weight as
//    the other five, so it does not look like a guest.
//
//      bed #4A3E2E amber    radio #2E4238 green
//      ac  #2C3C4A blue     fan   #3A3348 violet
//
//    The placeholder tile code is gone. All six tiles are artwork now.
//
//    NEW FILE:  irapanel/include/fan_img.h
//
//  WHAT CHANGED IN TEST 026  -  THE PWM EXPERIMENT IS OVER
//
//    Switching the backlight over I2C flickers, and it cannot be
//    fixed. Every touch read takes the same bus, lands in the middle
//    of a pulse, and stretches it. esp_timer corrected the schedule
//    but not the collision. Two masters, one bus, one of them needing
//    microsecond timing - it was never going to hold.
//
//    So it is removed. The backlight is on or off, as the hardware
//    intended, and the slider goes back to the black overlay, which
//    is steady and costs nothing even if it does not reach true dark.
//
//    THE REAL FIX, unchanged from the first time it came up: one wire
//    from the MP3302's EN pin to a spare GPIO, and hardware PWM. GPIO6
//    is free and comes out on the Sensor AD connector. That is a
//    daylight job with a fine tip, not a midnight one, and it is the
//    only thing that will give Ira a screen that truly goes dark.
//
//    ALSO: tapping a tile now opens a screen that says, in plain
//    words, that it is not built yet - instead of only printing to a
//    serial port nobody is watching at 2 am.
//
//  WHAT CHANGED IN TEST 025  -  SIX TILES
//
//    Light and fan added, so the home screen is a 3 x 2 grid instead
//    of a 2 x 2 block. Both are 433 MHz devices whose transmitter
//    lives on the audio node, and that board has not arrived - so
//    both tiles send their command down the bus and nothing answers
//    yet. That is deliberate: on the day the node arrives, nothing on
//    this screen has to change.
//
//    LIGHT uses img_lamp out of lamp_img.h, which is already in the
//    repository - the Morning Glory lamp, 200x200.
//
//      COPY panel/include/lamp_img.h INTO irapanel/include/
//
//    FAN has no artwork yet, so it gets a tile that says so plainly
//    rather than a wrong picture. It is drawn, not an image, and it
//    is deliberately styled to look unfinished - a placeholder that
//    looks finished is a placeholder that never gets replaced.
//
//    Tiles are 180 px now, not 200, so six fit with room left for the
//    brightness slider.
//
//  WHAT CHANGED IN TEST 024  -  STEADY LIGHT, AND THE HOME SCREEN
//
//    THE FLICKER, AND WHY IT HAPPENED
//    A task that busy-waits between edges is at the mercy of the
//    scheduler. Every time the touch read took the I2C mutex, or the
//    1 ms FreeRTOS tick landed mid-pulse, the on-time stretched by a
//    couple of hundred microseconds. At a 2 ms period that is a ten
//    percent swing in brightness, arriving at random. That is exactly
//    what an eye reads as flicker.
//
//    Now the edges are driven by esp_timer instead. Each edge schedules
//    the next one at an ABSOLUTE deadline, so a late edge does not push
//    the following one late as well - the error is corrected rather
//    than accumulated. And if the mutex is busy the edge is simply
//    skipped instead of waited for, which loses one cycle out of a
//    thousand rather than stretching one visibly.
//
//    Honest note: this is still a backlight being switched over a
//    shared I2C bus, and it will never be as clean as a wire from the
//    MP3302's EN pin to a hardware PWM output. If a trace of flicker
//    survives at the lowest settings, that wire remains the real fix.
//
//    THE HOME SCREEN
//    Four tiles - massage, radio, air conditioning, shades - in a 2x2
//    block, using tile_img.h straight out of Shemi's panel. Same
//    artwork, same order, because the two of you swap sides and an
//    interface that changes with the side of the bed is worse than no
//    interface at all.
//
//    COPY panel/include/tile_img.h INTO irapanel/include/
//
//    Touching a tile does nothing yet except say so on serial. The
//    screens behind them come next, and they are a copy of yours.
//
//  WHAT CHANGED IN TEST 023
//    Backlight PWM raised from 250 Hz to 500 Hz. Still well inside
//    the MP3302's stated 200 Hz - 1 kHz dimming window, and twice as
//    far above the frequency at which an eye notices flicker.
//
//    The cost is twice as many expander writes: 1000 a second instead
//    of 500. At roughly 60 us each that is about 6% of the I2C bus,
//    which the touch controller will still not notice.
//
//    If it is smooth here, 1 kHz is available too - one number.
//
//  WHAT CHANGED IN TEST 022  -  REAL DIMMING, NO SOLDERING IRON
//
//    The black overlay was not dark enough, as expected: it dims the
//    CONTENT, never the lamps, so the glass keeps glowing.
//
//    But the MP3302 datasheet says something useful about the pin
//    this board already controls:
//
//      "EN - ON/OFF Control and Dimming Command Input. To use PWM
//       dimming, apply a 200Hz to 1KHz square wave signal."
//
//    EN is not just an on/off switch. It is the dimming input. And EN
//    is wired to EXIO2 of the CH422G - the very line that already
//    switches the backlight from firmware.
//
//    So the backlight CAN be dimmed with no hardware change at all:
//    blink EXIO2 over I2C at 250 Hz with a duty cycle. One expander
//    write takes about 60 us, so 500 writes a second is under 3% of
//    the bus. The touch controller will not notice.
//
//    HOW IT IS DONE SAFELY
//      - a dedicated FreeRTOS task on core 0 does nothing but the
//        blinking, so LVGL taking 20 ms to render cannot stutter it
//      - EVERY I2C transaction in this file is now inside a mutex,
//        because two cores sharing one Wire object without one is a
//        corrupted transfer waiting to happen
//      - at 0% and 100% the task stops toggling entirely and just
//        holds the line, so a static backlight costs nothing
//
//    If this flickers at very low settings, the fallback is still a
//    wire to the MP3302 - but it is worth an hour before a soldering
//    iron comes near a TSOT23-5 at midnight.
//
//  WHAT CHANGED IN TEST 021  -  IT WORKS. TIDYING UP.
//
//    The missing line was -DLV_TICK_CUSTOM=1 in platformio.ini.
//    Without a tick source LVGL never believes time has passed, so
//    it renders nothing and never polls the input device. The touch
//    chip was reporting perfect coordinates on serial the whole time
//    into a library that was not listening. Shemi's panel has had
//    that flag since the beginning; this file never got it.
//
//    Both diagnostics are now removed, and the crosshair had to go
//    for a second reason: it was drawn STRAIGHT to the panel, behind
//    LVGL's back. LVGL then repainted the area it had scribbled on,
//    and the two fought over the same pixels every frame. That is the
//    flicker - "the screen is screaming" - and it is the same lesson
//    already written down in this project: repeated redrawing of an
//    RGB panel shows as a visible twitch. Never draw around LVGL once
//    LVGL owns the screen.
//
//    What is left is the hardware layer, finished and quiet:
//      RGB display, GT911 touch to five points, CH422G backlight,
//      I2C scan, boot photograph, software dimmer.
//
//    Next comes the interface, which is almost entirely a copy of
//    Shemi's panel.
//
//  WHAT CHANGED IN TEST 020  -  IS LVGL DRAWING AT ALL?
//
//    The touch chip now works: real, moving coordinates on serial.
//    But nothing on the glass responds. That splits into exactly two
//    possibilities and this build tells them apart without anyone
//    having to describe a screen.
//
//    1. A LIVE COUNTER. A big number at the bottom of the page that
//       increments once a second, driven by an LVGL timer and nothing
//       else. If that number moves, LVGL is drawing and the problem
//       is only in how touch reaches it. If the number is frozen,
//       LVGL is not drawing at all and touch was never the issue.
//
//    2. A CROSSHAIR DRAWN DIRECTLY. Every touch also draws a small
//       cross straight onto the panel with the graphics library,
//       bypassing LVGL completely. If the cross follows your finger
//       but the LVGL label does not, the display hardware and the
//       touch are both fine and LVGL's input path is the fault.
//
//    Between the two, one look at the screen names the culprit.
//
//  WHAT CHANGED IN TEST 019  -  bb_captouch CANNOT WORK ON THIS BOARD
//
//    The report finally showed the truth:
//
//        bb_captouch: init rc=0, sensor type 7
//        [E][Wire.cpp:513] requestFrom(): i2cRead returned Error -1
//
//    Type 7 is not a GT911. The library auto-detects by probing the
//    addresses that touch controllers usually live at - and one of
//    those is 0x38, which on THIS board is the CH422G's output
//    register. The expander answers, the library concludes it has
//    found a FocalTech part, and then talks to the GT911 in a
//    protocol it does not speak. Hence Error -1 and 0xFF everywhere.
//
//    Its documentation confirms there is no way out: four methods,
//    auto-detection only, no way to force the type. So bb_captouch is
//    removed. Not a bad library - just the wrong one for a board that
//    has an IO expander squatting on 0x38.
//
//    Back to the direct driver, which demonstrably TALKS to the chip
//    (it read the product id as "911"), plus the one thing that was
//    found and never actually verified as fixed:
//
//      THE CONFIG SAYS 10 TOUCH POINTS. The GT911 supports 1 to 5.
//      TEST 008 spotted it, TEST 009 wrote a corrected table in
//      chunks - and then TEST 010 deleted the whole thing before the
//      result was ever read back. That loose end is tied here: the
//      table is rewritten, the checksum recalculated, and the value
//      READ BACK and printed in the report that repeats, so it can
//      no longer scroll away unseen.
//
//  WHAT CHANGED IN TEST 018
//    The same broken string literal as before - a newline escape lost
//    its backslash while the file was being edited, splitting a
//    printf across two lines. Repaired, and this time the whole file
//    was swept for the same fault rather than just that one line.
//    My typo twice over; nothing else changed.
//
//  WHAT CHANGED IN TEST 017
//
//    It boots, and bb_captouch is running. But every sample reads:
//
//        touch: 255 point(s), first at 65535 , 65535
//
//    255 is 0xFF and 65535 is 0xFFFF. Those are not coordinates, they
//    are untouched memory. getSamples() is reporting success while
//    writing nothing, which means the I2C read inside it is failing
//    and the struct is being left exactly as it was found.
//
//    Three things here:
//
//    1. A SANITY GUARD. The GT911 tracks at most 5 points. Anything
//       above that is discarded rather than fed to LVGL, because a
//       touch at 65535,65535 would send the cursor off the screen.
//
//    2. THE STRUCT IS CLEARED before every call, so garbage cannot be
//       mistaken for data ever again.
//
//    3. THE INIT RESULT IS IN THE REPEATING REPORT. The rc and the
//       sensor type printed once at boot have scrolled away every
//       time. They are now printed every 20 seconds with everything
//       else, so we can finally read them.
//
//    If the guard reports garbage continuously, the driver is
//    re-initialised once after ten seconds - worth a try, and it
//    costs nothing if it does not help.
//
//  WHAT CHANGED IN TEST 016  -  A GREY SCREEN IS A HANG
//
//    TEST 015 left the panel grey with nothing on it. Grey means the
//    backlight came on and then nothing was ever drawn - in other
//    words setup() started and never finished.
//
//    The new arrival in setup() is bbct.init(). A touch library that
//    cannot find its controller can sit in a retry loop, and if it
//    does, every line after it - LVGL, the test page, the whole
//    interface - never runs. One unlucky library call took the
//    display with it.
//
//    Two changes so that can never happen again:
//
//    1. ORDER. LVGL is started and the test page is drawn BEFORE the
//       touch driver is initialised. Whatever touch does, there is
//       already a readable screen. A panel that shows a page with
//       "touch: not ready" on it tells you far more than a grey one.
//
//    2. SIGNPOSTS. Every step in setup() prints before it runs. If
//       the board hangs again, the last line printed names the exact
//       call that did it, instead of us guessing from a colour.
//
//  WHAT CHANGED IN TEST 015
//    i2cScan() disappeared a second time. It had been sitting inside
//    the touch section, so every time that section was replaced it
//    went with it. That is a filing mistake, not a coding one, and it
//    has cost two builds.
//
//    It now lives in its own block, above everything touch-related,
//    where nothing that happens to the touch driver can reach it.
//
//  WHAT CHANGED IN TEST 014
//    A broken string literal in the touch debug line - a newline
//    escape lost its backslash when I edited the file, which split
//    the string across two lines. Purely my typo. Nothing else moved.
//
//  WHAT CHANGED IN TEST 013  -  USE THE LIBRARY, STOP WRITING ONE
//
//    Found on GitHub: Westcott1/Waveshare-ESP32-S3-Touch-LCD-4.3-and-
//    Arduino. Somebody running THIS EXACT BOARD, with the same pin map
//    used here, wrote:
//
//        "Touch is controlled by a GT911, and works well with the
//         bb_captouch library.  SDA=8, SCL=9, IRQ=4"
//
//    He also confirms LVGL 8.x with Arduino core 2.x, which is what
//    platformio.ini already pins.
//
//    So the hand-written driver is gone. bb_captouch handles the whole
//    GT911 start-up - address detection, reset timing, the register
//    dance - and it is the same library already used on Shemi's panel.
//    Twelve builds of reasoning from a datasheet, replaced by four
//    lines from a library that other people have already debugged.
//
//    The CH422G still runs the backlight and still releases TP_RST
//    before bb_captouch starts, because that reset line is on the
//    expander and no library can reach it.
//
//    ADD TO platformio.ini:   bitbank2/bb_captouch
//
//  WHAT CHANGED IN TEST 012  -  ORDER OF OPERATIONS
//
//    Touch worked an hour before any of this, running Waveshare's own
//    firmware. So the flex is seated, the glass is fine, and the
//    hardware was never the problem. I sent you looking at connectors
//    for nothing.
//
//    Comparing against Waveshare's own start-up sequence shows one
//    real difference, and it is not a register value - it is ORDER.
//
//      Waveshare:  expander -> TP_RST high -> wait -> RGB bus -> LCD
//      Mine:       expander -> probe touch -> RGB bus -> LCD
//
//    The touch controller sits on a flex that runs alongside the RGB
//    flex, and the RGB bus starting up is the loudest electrical
//    event on this board. A GT911 that was initialised BEFORE that
//    happens can latch into a state where it scans normally and
//    detects nothing - which is exactly ready=1, points=0, forever.
//
//    So now:
//      1. expander up, backlight OFF, both resets released
//      2. RGB panel started, boot photo drawn, backlight ON
//      3. ONLY THEN pulse TP_RST on EXIO1, with INT held low, and
//         probe the GT911
//
//    The reset is a real pulse this time, driven through the CH422G,
//    which is already proven to work because it is the same chip and
//    the same register that switches the backlight - and the backlight
//    does switch.
//
//  WHAT CHANGED IN TEST 011
//    i2cScan() went out with the bathwater when the old touch code
//    was cut. Put back, unchanged. Nothing else differs from 010.
//
//  WHAT CHANGED IN TEST 010  -  BACK TO WHAT IS PROVEN
//
//    Nine attempts went into resets, wake commands, config tables and
//    checksums. None of them belonged there. Shemi's panel has been
//    driving a GT911 at 0x5D for months, and its touch routine does
//    NONE of those things:
//
//      - no reset sequence
//      - no command register write
//      - no config table, no checksum
//      - probe the address, read the status, read four bytes, clear
//
//    That is the whole driver. This file now uses exactly that code,
//    copied from panel/src/main.cpp rather than reasoned about from
//    a datasheet. Known-good beats deduced - which is already written
//    down as a lesson in this project's own handout.
//
//    Everything I added is removed, including the config write, which
//    may itself have upset the chip.
//
//    >>> POWER-CYCLE THE BOARD BEFORE JUDGING THIS BUILD. <<<
//    Pull the USB out, count to five, put it back. A reset button
//    press is not enough - the config table written by TEST 008 and
//    009 lives until the chip loses power.
//
//    Address probing now tries 0x5D and then 0x14, because which one
//    answers depends on the level of the INT line at power-up and
//    that is a hardware detail, not a decision for firmware.
//
//  WHAT CHANGED IN TEST 009
//
//    TEST 008 found the illegal value - touch count 10 where the
//    GT911 allows 1 to 5 - and rewrote it. The chip still reported
//    10 afterwards, because the write never reached it.
//
//    THE WIRE BUFFER. The config write was one transaction of 186
//    bytes. The ESP32's Wire buffer is 128 bytes by default. Every
//    byte past 128 is DISCARDED, and endTransmission() still returns
//    success, so the code reported "write ok" while nothing had been
//    written. A silent truncation that lies about it afterwards.
//
//    Fixed three ways, belt and braces:
//      - the buffer is enlarged first
//      - the table is written in 16-byte chunks, each with its own
//        register address, so no single transaction is ever large
//      - and the table is READ BACK and printed, so the firmware can
//        no longer claim a success it did not achieve
//
//  WHAT CHANGED IN TEST 008
//
//    The config finally printed, and it contains an illegal value:
//
//      GT911 config : ver 0x43, 800 x 480, 10 points
//      cfg bytes    : 43 20 03 E0 01 0A 3D ...
//                                    ^^
//
//    Byte 0x804C is the touch count. The GT911 supports 1 to 5
//    points. This chip has 10 in that field - out of range. A
//    controller told to track ten fingers on hardware that can track
//    five scans happily and reports nothing, which is precisely the
//    symptom: ready=1, points=0, forever.
//
//    TEST 006 tried to catch a bad config but only tested for ZERO
//    points. Ten is just as invalid and slipped straight through.
//    That was my error in the condition, not a new fault.
//
//    Now: anything outside 1..5 is rewritten to 5, with the checksum
//    recalculated, and the chip is told to adopt the table.
//
//    Also noted for later, not fixed yet: Module_Switch1 is 0x3D,
//    which has the X2Y bit set. That means this panel reports its
//    axes swapped. It does not stop detection, so it is left alone
//    until a finger actually registers - one problem at a time.
//
//  WHAT CHANGED IN TEST 007
//
//    Two housekeeping fixes so nothing is hidden any more, and one
//    conclusion.
//
//    1. THE I2C LIST WAS TRUNCATED. The buffer was 96 characters and
//       the CH422G alone answers on 26 addresses, so the list stopped
//       at 0x39 and 0x5D was never shown - even though the GT911 was
//       answering perfectly. The buffer is now large enough.
//
//    2. THE CONFIG REPORT SCROLLED AWAY. It printed once during
//       setup(), which is exactly when nobody is watching. It is now
//       part of the report that repeats every 20 seconds.
//
//    AND THE CONCLUSION, which is not a software one:
//
//    status 0x80 with points=0, every single time, means the chip
//    finishes each scan cleanly and finds nothing. Sleep, address,
//    I2C protocol and config have all now been proven good. A
//    controller that scans perfectly and never sees a finger is a
//    controller whose SENSOR is not attached. The touch glass reaches
//    the GT911 through its own flat flex cable, separate from the
//    display ribbon. If that flex is not fully seated and its black
//    latch closed, this is exactly what you get: a healthy chip
//    scanning empty air.
//
//  WHAT CHANGED IN TEST 006
//
//    The raw dump settled it:  80 00 00 00 ...  ready=1 points=0.
//    Byte 0 having its top bit set means the GT911 IS scanning and
//    IS finishing each scan cleanly. It simply finds nothing. That
//    clears sleep, addressing and the I2C protocol - all three are
//    now proven good - and leaves exactly one suspect: the config
//    table inside the chip.
//
//    The GT911 keeps resolution, touch count, sensitivity and the
//    channel map in a 185-byte table at 0x8047. If that table is
//    blank, or set for a different panel, the chip scans forever and
//    detects nothing while reporting perfect health.
//
//    So this build READS the table, PRINTS it, and if the resolution
//    is not 800x480 it writes a corrected copy back - including the
//    checksum, without which the chip ignores the whole thing.
//
//    Colour bars removed, as asked. They did their job: the panel is
//    wired RGB and the display path is innocent.
//
//  WHAT CHANGED IN TEST 005
//
//    TEST 004 got the GT911 talking - "GT911 id : 911" proves the
//    repeated-start fix worked and the chip is alive and addressed
//    correctly. It still reported no finger.
//
//    1. THE COMMAND REGISTER. 0x8040 tells the GT911 what to do.
//       A chip that has been left in sleep, or that came up in an
//       odd state, will happily answer its product id and never scan
//       the panel. Writing 0x00 there means "read coordinate state" -
//       normal operation. This is now done right after reset, and it
//       is the most likely single cause of the silence.
//
//    2. RAW DUMP FOR THE FIRST MINUTE. Every half second the ten
//       bytes from 0x814E are printed exactly as they come off the
//       bus, plus the config version. If touch still does not work
//       we will be reading the chip's own words instead of guessing
//       a fifth time.
//
//    3. THE DIM SHEET IS NOT CREATED UNTIL IT IS NEEDED. It is a
//       full-screen object on the top layer, and while it should be
//       transparent to input, it is the one thing in this file that
//       could swallow a touch. During bring-up it now only exists
//       once the slider is moved, so it cannot be a suspect.
//
//  WHAT CHANGED IN TEST 004
//
//    Colour bars correct, so the panel is wired RGB and that whole
//    line of enquiry is closed. Touch answered on I2C but never
//    reported a finger. Two causes, both fixed here.
//
//    1. REPEATED START. Reading a GT911 register means: write the
//       two address bytes, then read - with NO stop condition in
//       between. Wire.endTransmission() sends a stop by default,
//       which resets the chip's internal pointer, so every read came
//       back as zeros. Every read now ends with endTransmission(false).
//       This is the single most common way GT911 code fails while
//       looking completely correct.
//
//    2. THE INT PIN WAS FLOATING. GPIO4 is the touch interrupt line
//       and it takes part in the reset sequence: it must be held LOW
//       while reset is released, which is also what selects address
//       0x5D. It is now driven properly and only then released to an
//       input.
//
//    Also added: the raw status byte is printed whenever it changes,
//    so if this still does not work we can see what the chip is
//    actually saying instead of guessing again.
//
//  WHAT CHANGED IN TEST 003
//
//    THE BYTE ORDER FIX IN TEST 002 WAS WRONG, and here is why it
//    changed nothing: removing setPixelType() does not select big
//    endian. JPEGDEC's own default IS little endian. So before the
//    "fix" the pair was LITTLE + draw16bitBe..., and after it the
//    pair was still LITTLE + draw16bitBe... Identical. My mistake.
//
//    The correct pairing is one of exactly two combinations:
//        LITTLE endian  ->  draw16bitRGBBitmap()
//        BIG endian     ->  draw16bitBeRGBBitmap()
//    This build uses the first, set explicitly at both ends so no
//    default can drift underneath it again.
//
//    AND A COLOUR BAR TEST, so this stops being guesswork.
//    Before the photograph, four labelled bars are drawn straight to
//    the panel by the graphics library - no JPEG involved at all:
//
//        RED   GREEN   BLUE   WHITE
//
//    If those four are correct and the photo is still wrong, the
//    fault is in the JPEG path. If the BARS are wrong too - red
//    showing as blue, say - then the panel itself is wired BGR and
//    the fix is somewhere else entirely. One glance separates two
//    problems that look identical from the serial port.
//
//  WHAT CHANGED IN TEST 002
//    First run on real hardware. Three findings, all fixed here.
//
//    1. BYTE ORDER. The photo appeared - recognisable, but with the
//       colours wrong and far too much white. Classic swapped bytes:
//       setPixelType() asked JPEGDEC for LITTLE endian while the draw
//       call was draw16bitBeRGBBitmap(), which wants BIG. The two
//       disagreed. setPixelType is now gone, so JPEGDEC uses its own
//       big-endian default and the two finally agree.
//
//    2. THE BANNER WAS NEVER SEEN. monitor_rts/dtr are 0, so opening
//       the serial monitor no longer resets the board and everything
//       printed during setup() had already scrolled away. The whole
//       boot report is now REPEATED every 20 seconds in loop(), so it
//       can be read whenever the monitor happens to be open.
//
//    3. The I2C scan result is now also drawn on the test page in
//       large type, because the glass is always visible and the
//       serial monitor is not.
//
//    Also: if the boot photo fails to decode the screen no longer
//    stays black. It says so, in words, on the panel.
//
//  WHAT THIS IS
//    The hardware layer, and nothing else. Screen, touch, backlight,
//    I2C, boot photo, and a test page that tells you on the glass
//    whether each piece answered. No massage, no radio, no clock -
//    those come next, and they are almost entirely copied from
//    Shemi's panel because the UI above this layer is identical.
//
//    The one thing this file exists to prove: that this board can
//    be driven at all. Everything after it is easy.
//
//  WHY THIS IS A SEPARATE PROJECT FROM panel/
//    Different board, different everything below LVGL:
//      - the RGB pin map is completely different
//      - touch is on GPIO8/9, not GPIO19/20
//      - the backlight is NOT a PWM pin. It is one bit inside a
//        CH422G I2C expander: on or off, nothing between.
//      - it needs Arduino core 3.x; Shemi's panel is pinned to 2.x
//    Trying to serve both from one platformio.ini is how afternoons
//    get lost. Two projects, one shared UI, deliberately.
//
//  THE BACKLIGHT, AND THE DECISION ALREADY TAKEN
//    Because the backlight cannot dim, night dimming on THIS panel
//    is done in software: a black overlay laid over the whole screen
//    at an opacity we choose. It darkens the CONTENT, not the lamps,
//    so the glass will still glow faintly at its darkest. That was
//    accepted knowingly. If it turns out not to be dark enough for
//    Ira, the hardware answer is one wire to the MP3302 boost chip's
//    control point - and nothing in this file has to change for that.
//
//  BH1750 WARNING - READ BEFORE WIRING
//    On Shemi's panel the light sensor sits at 0x23. Do NOT use 0x23
//    here. The CH422G expander answers on several addresses in the
//    0x20-0x3F block, and a collision on I2C does not fail cleanly -
//    it produces wrong readings that look plausible. Tie the BH1750
//    ADDR pin HIGH to move it to 0x5C. This file expects 0x5C.
//
//  PIN MAP - taken from Waveshare's own published mapping, not guessed
// ============================================================

#define TEST_NUMBER 78

#include <Arduino.h>
#include <string.h>       // TEST 049: strcmp for tile routing
#include <Wire.h>
#include <Arduino_GFX_Library.h>
#include <lvgl.h>
#include <JPEGDEC.h>
#include <esp_timer.h>

#include "boot_photo.h"      // the same header Shemi's panel uses
#include "tile_img.h"        // TEST 024: bed, radio, AC, shades - from panel/
#include "lamp_img.h"        // TEST 025: the lamp, also from panel/
#include "fan_img.h"         // TEST 027: the ceiling fan, from its photograph
#include "dream_img.h"        // TEST 029: the dreamcatcher tile for the Dreamsaver
#include "dial_img.h"
#include "hand_img.h"        // TEST 045: barbed clock hands (hour + minute)         // TEST 031: the screensaver clock face (from panel_shemi)
#include "ac_img.h"           // TEST 032: the AC tile off/on photos
#include "fan_tile_img.h"    // TEST 042: the fan tile off/on photos (real fan)
#include "temp_big.h"         // TEST 033: big LVGL font for the temperature
#include "font_hebrew.h"     // TEST 049: Hebrew glyphs (for the massage screen)

// ------------------------------------------------------------
//  Identity. 2 = Ira. This picks the boot photograph and, later,
//  the address this panel answers to on the RS-485 bus.
// ------------------------------------------------------------
#define PANEL_ID   2
#define BUS_ADDR   2

#define SCREEN_W   800
#define SCREEN_H   480

// ------------------------------------------------------------
//  I2C - shared by touch, the expander, and our two sensors
// ------------------------------------------------------------
#define I2C_SDA     8
#define I2C_SCL     9
#define I2C_HZ      400000

#define ADDR_GT911  0x5D     // touch (0x14 if INT is high at reset)
// GPIO4 is the GT911 interrupt line. Left alone on purpose: the
// working panel never touches it and neither does this file now.
#define ADDR_SHT31  0x44     // temperature + humidity, on a short arm BELOW the box
#define ADDR_BH1750 0x5C     // light - NOT 0x23, see the warning above

// ------------------------------------------------------------
//  CH422G IO expander
//    It is not a normal register device. Each function has its own
//    I2C address, and you write a single byte to it.
//      0x24  system / mode      write 0x01 to make EXIO pins outputs
//      0x38  output register    one bit per EXIO pin
//    EXIO map on this board:
//      bit0 EXIO0  -
//      bit1 EXIO1  touch reset
//      bit2 EXIO2  DISP / backlight enable   <-- the one that matters
//      bit3 EXIO3  LCD reset
//      bit4 EXIO4  SD card chip select
//      bit5 EXIO5  USB / CAN select
// ------------------------------------------------------------
#define CH422G_MODE 0x24
#define CH422G_OUT  0x38

#define EXIO_TP_RST (1 << 1)
#define EXIO_DISP   (1 << 2)
#define EXIO_LCD_RST (1 << 3)

uint8_t exioState = 0;

// TEST 022: one mutex around every I2C transaction. The backlight
// task on core 0 and the touch reads on core 1 share one Wire object,
// and Wire is not thread safe.
SemaphoreHandle_t i2cMutex = NULL;
inline void i2cLock()   { if (i2cMutex) xSemaphoreTake(i2cMutex, portMAX_DELAY); }  // TEST 059: back to wait-forever; the 100 ms timeout could abort the GT911 flag-clear and leave a stuck touch
inline void i2cUnlock() { if (i2cMutex) xSemaphoreGive(i2cMutex); }

// Backlight PWM state. 0 = off, 100 = full.
volatile int  blPercent = 100;
volatile bool blRunning = false;

// ------------------------------------------------------------
//  The RGB panel. These twenty pins are why almost nothing else
//  is free on this board.
// ------------------------------------------------------------
Arduino_ESP32RGBPanel *rgbpanel = new Arduino_ESP32RGBPanel(
   5 /* DE */,  3 /* VSYNC */, 46 /* HSYNC */,  7 /* PCLK */,
   1 /* R0 */,  2 /* R1 */,   42 /* R2 */,     41 /* R3 */,  40 /* R4 */,
  39 /* G0 */,  0 /* G1 */,   45 /* G2 */,     48 /* G3 */,  47 /* G4 */, 21 /* G5 */,
  14 /* B0 */, 38 /* B1 */,   18 /* B2 */,     17 /* B3 */,  10 /* B4 */,
   0 /* hsync_polarity */, 40 /* hsync_front_porch */,
  48 /* hsync_pulse_width */, 88 /* hsync_back_porch */,
   0 /* vsync_polarity */, 13 /* vsync_front_porch */,
   3 /* vsync_pulse_width */, 32 /* vsync_back_porch */,
   1 /* pclk_active_neg */, 16000000 /* prefer_speed */
);

Arduino_RGB_Display *gfx = new Arduino_RGB_Display(
  SCREEN_W, SCREEN_H, rgbpanel, 0 /* rotation */, true /* auto_flush */);

// ------------------------------------------------------------
//  State discovered at boot, shown on the test page
// ------------------------------------------------------------
bool haveTouch = false, haveSHT31 = false, haveBH1750 = false, haveCH422 = false;
char i2cLine[220] = "";   // TEST 007: was 96, which truncated the list

JPEGDEC jpeg;

// TEST 016: print before doing, so a hang names itself.
static int  bootStep = 0;
static void step(const char *what) {
  Serial.printf("[%d] %s ...\n", ++bootStep, what);
  Serial.flush();
}

// ---- forward declarations, so order of definition never matters ----
int  jpegDrawCb(JPEGDRAW *p);
bool showBootPhoto();
void colourBarTest();
void bootReport();
void setDim(int pct);
void i2cScan();
void backlightSet(int pct);
void touchResetPulse();
bool touchBegin();
bool touchGet(int &x, int &y, int &count);
bool gtProbe();
void gtFixConfig();
lv_obj_t *lblTouch = NULL, *dimLayer = NULL;
extern lv_obj_t *scrTest;   // TEST 055: forward decl so lvTouch can guard on it (defined later)
extern bool saverActive;   // TEST 072: forward decl so lvTouch can guard on it (defined later)
int dimPct = 0;                       // 0 = full brightness, 90 = nearly black

// ============================================================
//  CH422G
// ============================================================
bool ch422Write(uint8_t addr, uint8_t value) {
  i2cLock();
  Wire.beginTransmission(addr);
  Wire.write(value);
  bool ok = (Wire.endTransmission() == 0);
  i2cUnlock();
  return ok;
}

// Raw, already inside the lock. Used by the backlight task so it does
// not take the mutex twice per edge.
inline void ch422WriteRaw(uint8_t addr, uint8_t value) {
  Wire.beginTransmission(addr);
  Wire.write(value);
  Wire.endTransmission();
}

bool ch422Begin() {
  // Put every EXIO pin into push-pull output mode.
  if (!ch422Write(CH422G_MODE, 0x01)) return false;
  // Release both resets, backlight OFF for now - the screen is not
  // initialised yet and a bright flash of noise is not a nice way
  // for a panel to greet anyone.
  exioState = EXIO_TP_RST | EXIO_LCD_RST;
  return ch422Write(CH422G_OUT, exioState);
}

// TEST 012: a real reset pulse on EXIO1, done AFTER the RGB panel is
// running. INT is held low across the pulse, which is also what selects
// I2C address 0x5D.
void touchResetPulse() {
  const int TP_INT = 4;
  pinMode(TP_INT, OUTPUT);
  digitalWrite(TP_INT, LOW);

  exioState &= ~EXIO_TP_RST;            // hold the controller in reset
  ch422Write(CH422G_OUT, exioState);
  delay(20);

  exioState |= EXIO_TP_RST;             // release, INT still low
  ch422Write(CH422G_OUT, exioState);
  delay(10);

  digitalWrite(TP_INT, LOW);
  delay(60);
  pinMode(TP_INT, INPUT);               // give the line back to the chip
  delay(120);                           // GT911 needs time before it answers
  Serial.println("GT911: reset pulse done, after the RGB bus was up");
}

void backlight(bool on) {
  if (on) exioState |=  EXIO_DISP;
  else    exioState &= ~EXIO_DISP;
  ch422Write(CH422G_OUT, exioState);
}

// TEST 026: kept as a name so nothing else has to change, but the
// backlight is now simply on or off. Anything between needs the wire.
void backlightSet(int pct) {
  blPercent = constrain(pct, 0, 100);
  backlight(blPercent > 0);
}

// ============================================================
//  I2C scan
//
//  Deliberately in its own section, ABOVE the touch code. It lived
//  inside the touch block before and was deleted twice by edits that
//  had nothing to do with it.
// ============================================================
void i2cScan() {
  int n = 0;
  char *p = i2cLine; *p = 0;
  for (uint8_t a = 1; a < 127; a++) {
    i2cLock();
    Wire.beginTransmission(a);
    int rc = Wire.endTransmission();
    i2cUnlock();
    if (rc == 0) {
      n++;
      if (a == ADDR_SHT31)        haveSHT31  = true;
      if (a == ADDR_BH1750)       haveBH1750 = true;
      if (a == 0x24 || a == 0x38) haveCH422  = true;
      int used = p - i2cLine;
      if (used < (int)sizeof(i2cLine) - 8)
        p += snprintf(p, sizeof(i2cLine) - used, "0x%02X ", a);
    }
  }
  Serial.printf("I2C: %d device%s: %s\n", n, n == 1 ? "" : "s", i2cLine);
  if (n == 0) strcpy(i2cLine, "NOTHING ON THE BUS - check SDA 8 / SCL 9");
}

// ============================================================
//  GT911 touch  -  direct, because bb_captouch is fooled by the
//  CH422G answering on 0x38.
//
//  Pins, from Waveshare's own table:
//     GPIO8 TP_SDA   GPIO9 TP_SCL   GPIO4 TP_IRQ   EXIO1 TP_RST
// ============================================================
#define GT911_ADDR1 0x5D
#define GT911_ADDR2 0x14
#define GT_CFG_START 0x8047
#define GT_CFG_LEN   184

uint8_t  gtAddr = 0;
bool     touchReady = false;
int      cfgPoints = -1;        // what the chip says after the fix
int      cfgW = -1, cfgH = -1;

bool gtWrite(uint16_t reg, uint8_t val) {
  i2cLock();
  Wire.beginTransmission(gtAddr);
  Wire.write(reg >> 8); Wire.write(reg & 0xFF); Wire.write(val);
  bool ok = (Wire.endTransmission() == 0);
  i2cUnlock();
  return ok;
}

int gtRead(uint16_t reg, uint8_t *buf, int len) {
  i2cLock();
  Wire.beginTransmission(gtAddr);
  Wire.write(reg >> 8); Wire.write(reg & 0xFF);
  if (Wire.endTransmission(false) != 0) { i2cUnlock(); return -1; }
  int n = Wire.requestFrom((int)gtAddr, len);
  for (int i = 0; i < n; i++) buf[i] = Wire.read();
  i2cUnlock();
  return n;
}

bool gtProbe() {
  const uint8_t a[2] = { GT911_ADDR1, GT911_ADDR2 };
  for (int i = 0; i < 2; i++) {
    i2cLock();
    Wire.beginTransmission(a[i]);
    int rc = Wire.endTransmission();
    i2cUnlock();
    if (rc == 0) {
      gtAddr = a[i];
      uint8_t id[4] = {0,0,0,0};
      gtRead(0x8140, id, 4);
      Serial.printf("GT911 at 0x%02X, id %c%c%c%c\n",
                    gtAddr, id[0], id[1], id[2], id[3]);
      return true;
    }
  }
  Serial.println("GT911 not found at 0x5D or 0x14");
  return false;
}

// The config table. Written in 16-byte chunks because one 186-byte
// transaction is silently truncated by the Wire buffer and then
// reports success - that is how TEST 008 fooled itself.
void gtFixConfig() {
  uint8_t cfg[GT_CFG_LEN];
  if (gtRead(GT_CFG_START, cfg, GT_CFG_LEN) != GT_CFG_LEN) {
    Serial.println("GT911 config: could not be read");
    return;
  }
  cfgW = cfg[2] << 8 | cfg[1];
  cfgH = cfg[4] << 8 | cfg[3];
  cfgPoints = cfg[5] & 0x0F;
  Serial.printf("GT911 config BEFORE: ver 0x%02X, %d x %d, %d points\n",
                cfg[0], cfgW, cfgH, cfgPoints);

  bool bad = (cfgPoints < 1 || cfgPoints > 5) ||
             (cfgW != SCREEN_W) || (cfgH != SCREEN_H);
  if (!bad) { Serial.println("GT911 config: already sane"); return; }

  cfg[1] = SCREEN_W & 0xFF;  cfg[2] = SCREEN_W >> 8;
  cfg[3] = SCREEN_H & 0xFF;  cfg[4] = SCREEN_H >> 8;
  cfg[5] = (cfg[5] & 0xF0) | 5;          // five points, the legal maximum

  uint8_t sum = 0;
  for (int i = 0; i < GT_CFG_LEN; i++) sum += cfg[i];
  uint8_t checksum = (uint8_t)((~sum) + 1);

  bool ok = true;
  for (int off = 0; off < GT_CFG_LEN && ok; off += 16) {
    int n = min(16, GT_CFG_LEN - off);
    uint16_t reg = GT_CFG_START + off;
    i2cLock();
    Wire.beginTransmission(gtAddr);
    Wire.write(reg >> 8); Wire.write(reg & 0xFF);
    for (int i = 0; i < n; i++) Wire.write(cfg[off + i]);
    int rc = Wire.endTransmission();
    i2cUnlock();
    if (rc != 0) {
      Serial.printf("GT911 config: chunk at 0x%04X FAILED\n", reg);
      ok = false;
    }
    delay(3);
  }
  if (ok) { gtWrite(0x80FF, checksum); delay(3); gtWrite(0x8100, 1); }
  delay(250);

  // Read it back. A write that cannot be seen did not happen.
  uint8_t v[8];
  if (gtRead(GT_CFG_START, v, 8) == 8) {
    cfgW = v[2] << 8 | v[1];
    cfgH = v[4] << 8 | v[3];
    cfgPoints = v[5] & 0x0F;
    Serial.printf("GT911 config AFTER : %d x %d, %d points  -  %s\n",
                  cfgW, cfgH, cfgPoints,
                  (cfgPoints >= 1 && cfgPoints <= 5) ? "ACCEPTED" : "REJECTED");
  }
  gtWrite(0x814E, 0);
}

bool touchBegin() {
  touchReady = gtProbe();
  if (touchReady) gtFixConfig();
  haveTouch = touchReady;
  return touchReady;
}

bool touchGet(int &x, int &y, int &count) {
  if (!touchReady) return false;
  uint8_t st = 0;
  if (gtRead(0x814E, &st, 1) != 1) return false;
  bool got = false;
  if (st & 0x80) {
    int pts = st & 0x0F;
    if (pts > 0 && pts <= 5) {
      uint8_t d[4];
      if (gtRead(0x8150, d, 4) == 4) {
        x = d[0] | (d[1] << 8);
        y = d[2] | (d[3] << 8);
        count = pts;
        got = (x <= SCREEN_W && y <= SCREEN_H);
      }
    }
    gtWrite(0x814E, 0);      // mandatory, or it never reports again
  }
  return got;
}

// ============================================================
//  Boot photo - straight from flash to the glass
// ============================================================
int jpegDrawCb(JPEGDRAW *p) {
  // TEST 003: LITTLE endian out of JPEGDEC, so the NON-Be draw call.
  // These two lines and the setPixelType call below are a matched
  // pair. Change one and you must change the other.
  gfx->draw16bitRGBBitmap(p->x, p->y, p->pPixels, p->iWidth, p->iHeight);
  return 1;
}

// ============================================================
//  TEST 003: colour bars, drawn with no JPEG involved.
//  This exists to answer one question that serial output cannot:
//  is the panel showing the colours it is given?
// ============================================================
void colourBarTest() {
  const int w = SCREEN_W / 4;
  struct { uint16_t c; const char *name; } bars[4] = {
    { RED,   "RED"   },
    { GREEN, "GREEN" },
    { BLUE,  "BLUE"  },
    { WHITE, "WHITE" },
  };
  gfx->fillScreen(BLACK);
  for (int i = 0; i < 4; i++) {
    gfx->fillRect(i * w, 0, w, SCREEN_H - 90, bars[i].c);
    gfx->setTextColor(WHITE);
    gfx->setTextSize(3);
    gfx->setCursor(i * w + 20, SCREEN_H - 60);
    gfx->print(bars[i].name);
  }
  Serial.println("colour bars: left to right should read RED GREEN BLUE WHITE");
  Serial.println("  if they do not, the panel is wired BGR and the JPEG is innocent");
}

void bootReport();     // TEST 002

bool showBootPhoto() {
#if PANEL_ID == 2
  const uint8_t *src = BOOT_IRA_JPG;   const uint32_t len = BOOT_IRA_JPG_LEN;
#else
  const uint8_t *src = BOOT_SHEMI_JPG; const uint32_t len = BOOT_SHEMI_JPG_LEN;
#endif
  uint8_t *buf = (uint8_t *)malloc(len);
  if (!buf) { Serial.println("boot photo: no heap"); return false; }
  memcpy_P(buf, src, len);
  bool ok = false;
  if (jpeg.openRAM(buf, len, jpegDrawCb)) {
    // TEST 003: stated explicitly, not left to a library default.
    // Matched with draw16bitRGBBitmap() in jpegDrawCb above.
    jpeg.setPixelType(RGB565_LITTLE_ENDIAN);
    ok = jpeg.decode(0, 0, 0);
    jpeg.close();
  }
  free(buf);
  Serial.printf("boot photo: %s\n", ok ? "shown" : "DECODE FAILED");
  return ok;
}

// ============================================================
//  LVGL glue
// ============================================================
static lv_disp_draw_buf_t drawBuf;
static lv_color_t *buf1 = NULL;

void lvFlush(lv_disp_drv_t *d, const lv_area_t *a, lv_color_t *px) {
  gfx->draw16bitRGBBitmap(a->x1, a->y1, (uint16_t *)px,
                          a->x2 - a->x1 + 1, a->y2 - a->y1 + 1);
  lv_disp_flush_ready(d);
}

void lvTouch(lv_indev_drv_t *d, lv_indev_data_t *data) {
  // TEST 072: while the raw-gfx saver is active, LVGL must not read the
  // touch - the loop owns it. Report RELEASED and read nothing.
  if (saverActive) {
    data->state = LV_INDEV_STATE_REL;
    return;
  }
  int x = 0, y = 0, n = 0;
  if (touchGet(x, y, n)) {
    data->state   = LV_INDEV_STATE_PR;
    data->point.x = x;
    data->point.y = y;
    if (lblTouch && lv_scr_act() == scrTest)
      lv_label_set_text_fmt(lblTouch, "touch  %d , %d   (%d finger%s)",
                            x, y, n, n == 1 ? "" : "s");
  } else {
    data->state = LV_INDEV_STATE_REL;
  }
}

// ============================================================
//  The software dimmer. This board's backlight is a switch, so
//  night dimming is a black sheet laid over everything on the
//  top layer, which sits above every screen we will ever build.
// ============================================================
// TEST 005: the sheet is created on first use, not at start-up, so
// during bring-up there is nothing at all covering the screen.
// ============================================================
//  TEST 030: temperature from the SHT31 (0x44), for the screensaver.
//  Single-shot, high repeatability. Wrapped in the I2C mutex because
//  the GT911 touch shares the bus. Same maths as panel_shemi.
// ============================================================
float gTemp = -100.0f;
bool  gTempOK = false;

static bool sht31Read(float &t) {
  if (!haveSHT31) return false;
  bool ok = false;
  i2cLock();
  Wire.beginTransmission(ADDR_SHT31);
  Wire.write(0x24); Wire.write(0x00);
  if (Wire.endTransmission() == 0) {
    delay(20);                                  // ~15 ms conversion
    int n = Wire.requestFrom((int)ADDR_SHT31, 6);
    if (n == 6) {
      uint8_t d[6];
      for (int i = 0; i < 6; i++) d[i] = Wire.read();
      uint16_t rawT = ((uint16_t)d[0] << 8) | d[1];
      if (rawT != 0xFFFF) { t = -45.0f + 175.0f * ((float)rawT / 65535.0f); ok = true; }
    }
  }
  i2cUnlock();
  return ok;
}

void setDim(int pct) {
  dimPct = constrain(pct, 0, 95);

  if (!dimLayer && dimPct > 0) {
    dimLayer = lv_obj_create(lv_layer_top());
    lv_obj_set_size(dimLayer, SCREEN_W, SCREEN_H);
    lv_obj_set_pos(dimLayer, 0, 0);
    lv_obj_set_style_bg_color(dimLayer, lv_color_black(), 0);
    lv_obj_set_style_border_width(dimLayer, 0, 0);
    lv_obj_set_style_radius(dimLayer, 0, 0);
    lv_obj_clear_flag(dimLayer, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(dimLayer, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(dimLayer, LV_OBJ_FLAG_IGNORE_LAYOUT);
  }
  if (dimLayer)
    lv_obj_set_style_bg_opa(dimLayer, (lv_opa_t)(dimPct * 255 / 100), 0);
}

// ============================================================
//  TEST 024: the home screen
//
//  Four 200x200 tiles in a 2x2 block, centred. Same artwork and the
//  same order as Shemi's panel, deliberately: you two swap sides, and
//  an interface that depends on which side of the bed you woke up on
//  would be worse than none.
// ============================================================
// TEST 028: identical to panel_shemi. Do not "improve" one without
// the other - the whole point is that they are the same screen.
#define TILE_SZ      175
#define TILE_GAPX     20
#define TILE_GAPY     16
#define TILE_X0       20
#define TILE_Y0       74
#define TILE_ZOOM    224      // artwork is 200 px; 256 = full size
#define TILE_DIM_OPA 110      // how far the artwork is pulled toward black

lv_obj_t *scrHome = NULL, *scrTest = NULL, *scrRadio = NULL, *scrMassage = NULL, *scrSettings = NULL, *scrBlank = NULL;   // TEST 062
static void evDim(lv_event_t *e);      // defined with the test page below
void buildTestPage();
void buildSoon();
void buildDreams();                    // TEST 029: Dreamsaver UI shell
void buildRadio();                     // TEST 049: radio screen (ported from panel_shemi)
void buildMassage();                   // TEST 050: massage screen (ported from panel_shemi)
void buildSettings();                  // TEST 051: settings screen (ported from panel_shemi)
void setDim(int pct);

lv_obj_t *scrSoon = NULL, *lblSoonWhat = NULL;

static void evBackHome(lv_event_t *e) { if (scrHome) lv_scr_load(scrHome); }

// TEST 026: one screen, reused for every tile. It names what you
// pressed and says plainly that it is not built, which is far more
// use at 2 am than a line on a serial port nobody is reading.
void buildSoon() {
  scrSoon = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(scrSoon, lv_color_hex(0x0B0E14), 0);

  lblSoonWhat = lv_label_create(scrSoon);
  lv_label_set_text(lblSoonWhat, "");
  lv_obj_set_style_text_font(lblSoonWhat, &lv_font_montserrat_28, 0);
  lv_obj_set_style_text_color(lblSoonWhat, lv_color_hex(0xE8F0FF), 0);
  lv_obj_align(lblSoonWhat, LV_ALIGN_CENTER, 0, -60);

  lv_obj_t *t = lv_label_create(scrSoon);
  lv_label_set_text(t, "not yet");
  lv_obj_set_style_text_font(t, &lv_font_montserrat_28, 0);
  lv_obj_set_style_text_color(t, lv_color_hex(0xFFC24B), 0);
  lv_obj_align(t, LV_ALIGN_CENTER, 0, -8);

  lv_obj_t *sub = lv_label_create(scrSoon);
  lv_label_set_text(sub, "this screen has not been built");
  lv_obj_set_style_text_font(sub, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(sub, lv_color_hex(0x6C7A94), 0);
  lv_obj_align(sub, LV_ALIGN_CENTER, 0, 36);

  lv_obj_t *b = lv_btn_create(scrSoon);
  lv_obj_set_size(b, 160, 60);
  lv_obj_align(b, LV_ALIGN_BOTTOM_MID, 0, -26);
  lv_obj_add_event_cb(b, evBackHome, LV_EVENT_CLICKED, NULL);
  lv_obj_t *bl3 = lv_label_create(b);
  lv_label_set_text(bl3, "back");
  lv_obj_set_style_text_font(bl3, &lv_font_montserrat_24, 0);
  lv_obj_center(bl3);
}

static void evTile(lv_event_t *e) {
  const char *name = (const char *)lv_event_get_user_data(e);
  Serial.printf("tile: %s\n", name);
  // TEST 049: real screens now exist for some tiles; the rest still
  // fall through to the "coming soon" placeholder.
  if (!strcmp(name, "settings") && scrSettings) { lv_scr_load(scrSettings); return; }
  if (!strcmp(name, "massage") && scrMassage) { lv_scr_load(scrMassage); return; }
  if (!strcmp(name, "radio") && scrRadio) { lv_scr_load(scrRadio); return; }
  if (scrSoon && lblSoonWhat) {
    lv_label_set_text(lblSoonWhat, name);
    lv_scr_load(scrSoon);
  }
}

// TEST 028: kept, unused, and marked as such. If a way back to the
// diagnostics page is ever wanted again - a long press somewhere, a
// serial command - the handler is already here.
static void evGoTest(lv_event_t *e) { if (scrTest) lv_scr_load(scrTest); }

static int tileX(int c) { return TILE_X0 + c * (TILE_SZ + TILE_GAPX); }
static int tileY(int r) { return TILE_Y0 + r * (TILE_SZ + TILE_GAPY); }

static lv_obj_t *makeTile(lv_obj_t *parent, int col, int row, uint32_t bg,
                          const char *name) {
  lv_obj_t *b = lv_btn_create(parent);
  lv_obj_set_size(b, TILE_SZ, TILE_SZ);
  lv_obj_set_pos(b, tileX(col), tileY(row));
  lv_obj_set_style_bg_color(b, lv_color_hex(bg), 0);
  lv_obj_set_style_radius(b, 16, 0);
  lv_obj_set_style_pad_all(b, 0, 0);
  lv_obj_set_style_shadow_width(b, 0, 0);
  lv_obj_clear_flag(b, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_event_cb(b, evTile, LV_EVENT_CLICKED, (void *)name);
  return b;
}

static void addImg(lv_obj_t *parent, const lv_img_dsc_t *src) {
  lv_obj_t *im = lv_img_create(parent);
  lv_img_set_src(im, src);
  // The artwork is untouched. LVGL pulls it toward black at draw time
  // so a white bed is not a lamp at three in the morning.
  lv_obj_set_style_img_recolor(im, lv_color_black(), 0);
  lv_obj_set_style_img_recolor_opa(im, TILE_DIM_OPA, 0);
  lv_img_set_zoom(im, TILE_ZOOM);          // 200 px artwork, 175 px tile
  lv_obj_center(im);
  lv_obj_clear_flag(im, LV_OBJ_FLAG_CLICKABLE);
}

// ============================================================
//  TEST 029 - DREAMSAVER (UI shell), ported from panel_shemi
//  A dreamcatcher tile in the (3,1) slot opens a "Dreams" screen
//  with two tiles: "Record Dream" (press-and-hold) and "Play
//  Dreams". Interface only - the real audio lives on the listener
//  node (mics + microSD) and the audionode (speakers) over the
//  RS-485 bus, none of which exist yet. panel_ira has no clock of
//  its own, so the "after 07:00" rule on Play is inactive here; it
//  turns on once the time arrives over the bus.
// ============================================================
lv_obj_t *scrDreams = NULL;
lv_obj_t *dreamRecTile = NULL, *dreamPlayTile = NULL;
lv_obj_t *dreamRecDot = NULL, *dreamStatus = NULL;
static bool dreamRecording = false;

static void evDreamRecPressed(lv_event_t *e) {
  dreamRecording = true;
  if (dreamRecDot)  lv_obj_clear_flag(dreamRecDot, LV_OBJ_FLAG_HIDDEN);
  if (dreamRecTile) lv_obj_set_style_bg_color(dreamRecTile, lv_color_hex(0x5A1A1A), 0);
  if (dreamStatus)  lv_label_set_text(dreamStatus, "recording... (hold)");
  Serial.println("[dream] record START (hold) -> would tell listener over RS-485");
}

static void evDreamRecReleased(lv_event_t *e) {
  if (!dreamRecording) return;
  dreamRecording = false;
  if (dreamRecDot)  lv_obj_add_flag(dreamRecDot, LV_OBJ_FLAG_HIDDEN);
  if (dreamRecTile) lv_obj_set_style_bg_color(dreamRecTile, lv_color_hex(0x181C22), 0);
  if (dreamStatus)  lv_label_set_text(dreamStatus, "saved (stub - nothing stored yet)");
  Serial.println("[dream] record STOP (release) -> would save to microSD with timestamp");
}

static void evDreamPlay(lv_event_t *e) {
  if (dreamStatus) lv_label_set_text(dreamStatus, "no dreams yet (07:00 gate needs bus time)");
  Serial.println("[dream] play/review -> after 07:00 (once time arrives over the bus)");
}

static void evGoDreams(lv_event_t *e) { if (scrDreams) lv_scr_load(scrDreams); }

void buildDreams() {
  scrDreams = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(scrDreams, lv_color_hex(0x0B0E14), 0);

  lv_obj_t *t = lv_label_create(scrDreams);
  lv_label_set_text(t, "Dreams");
  lv_obj_set_style_text_font(t, &lv_font_montserrat_28, 0);
  lv_obj_set_style_text_color(t, lv_color_white(), 0);
  lv_obj_align(t, LV_ALIGN_TOP_MID, 0, 16);

  lv_obj_t *bk = lv_btn_create(scrDreams);
  lv_obj_set_size(bk, 64, 48);
  lv_obj_align(bk, LV_ALIGN_TOP_LEFT, 16, 12);
  lv_obj_set_style_bg_color(bk, lv_color_hex(0x232A33), 0);
  lv_obj_set_style_shadow_width(bk, 0, 0);
  lv_obj_add_event_cb(bk, evBackHome, LV_EVENT_CLICKED, NULL);
  lv_obj_t *bl = lv_label_create(bk);
  lv_label_set_text(bl, LV_SYMBOL_LEFT);
  lv_obj_set_style_text_font(bl, &lv_font_montserrat_28, 0);
  lv_obj_center(bl);

  const int SZ = TILE_SZ;                        // 175
  const int GAP = 40;
  const int y = 150;
  const int x0 = (800 - (2 * SZ + GAP)) / 2;

  dreamRecTile = lv_btn_create(scrDreams);
  lv_obj_set_size(dreamRecTile, SZ, SZ);
  lv_obj_set_pos(dreamRecTile, x0, y);
  lv_obj_set_style_bg_color(dreamRecTile, lv_color_hex(0x181C22), 0);
  lv_obj_set_style_radius(dreamRecTile, 16, 0);
  lv_obj_set_style_shadow_width(dreamRecTile, 0, 0);
  lv_obj_clear_flag(dreamRecTile, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_event_cb(dreamRecTile, evDreamRecPressed,  LV_EVENT_PRESSED,    NULL);
  lv_obj_add_event_cb(dreamRecTile, evDreamRecReleased, LV_EVENT_RELEASED,   NULL);
  lv_obj_add_event_cb(dreamRecTile, evDreamRecReleased, LV_EVENT_PRESS_LOST, NULL);

  lv_obj_t *ri = lv_label_create(dreamRecTile);
  lv_label_set_text(ri, LV_SYMBOL_AUDIO);
  lv_obj_set_style_text_font(ri, &lv_font_montserrat_40, 0);
  lv_obj_set_style_text_color(ri, lv_color_hex(0xC0C8D0), 0);
  lv_obj_align(ri, LV_ALIGN_CENTER, 0, -14);
  lv_obj_t *rt = lv_label_create(dreamRecTile);
  lv_label_set_text(rt, "hold to record");
  lv_obj_set_style_text_font(rt, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(rt, lv_color_hex(0x8A94A0), 0);
  lv_obj_align(rt, LV_ALIGN_CENTER, 0, 30);

  dreamRecDot = lv_obj_create(dreamRecTile);
  lv_obj_set_size(dreamRecDot, 22, 22);
  lv_obj_set_style_radius(dreamRecDot, 11, 0);
  lv_obj_set_style_bg_color(dreamRecDot, lv_color_hex(0xE03030), 0);
  lv_obj_set_style_border_width(dreamRecDot, 0, 0);
  lv_obj_align(dreamRecDot, LV_ALIGN_TOP_RIGHT, -12, 12);
  lv_obj_add_flag(dreamRecDot, LV_OBJ_FLAG_HIDDEN);

  dreamPlayTile = lv_btn_create(scrDreams);
  lv_obj_set_size(dreamPlayTile, SZ, SZ);
  lv_obj_set_pos(dreamPlayTile, x0 + SZ + GAP, y);
  lv_obj_set_style_bg_color(dreamPlayTile, lv_color_hex(0x181C22), 0);
  lv_obj_set_style_radius(dreamPlayTile, 16, 0);
  lv_obj_set_style_shadow_width(dreamPlayTile, 0, 0);
  lv_obj_clear_flag(dreamPlayTile, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_event_cb(dreamPlayTile, evDreamPlay, LV_EVENT_CLICKED, NULL);

  lv_obj_t *pi = lv_label_create(dreamPlayTile);
  lv_label_set_text(pi, LV_SYMBOL_PLAY);
  lv_obj_set_style_text_font(pi, &lv_font_montserrat_40, 0);
  lv_obj_set_style_text_color(pi, lv_color_hex(0xC0C8D0), 0);
  lv_obj_align(pi, LV_ALIGN_CENTER, 0, -14);
  lv_obj_t *pt = lv_label_create(dreamPlayTile);
  lv_label_set_text(pt, "hear dreams");
  lv_obj_set_style_text_font(pt, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(pt, lv_color_hex(0x8A94A0), 0);
  lv_obj_align(pt, LV_ALIGN_CENTER, 0, 30);

  dreamStatus = lv_label_create(scrDreams);
  lv_label_set_text(dreamStatus, "UI shell - recording & playback not wired yet");
  lv_obj_set_style_text_font(dreamStatus, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(dreamStatus, lv_color_hex(0x556070), 0);
  lv_obj_align(dreamStatus, LV_ALIGN_BOTTOM_MID, 0, -24);
}


// ============================================================
//  TEST 030 - THE SCREENSAVER, with the Dreamsaver on it
//  Idle for a while and the home fades to this dark screen. Temperature
//  is real (SHT31); the clock is a placeholder until time arrives over
//  the bus. HOLD the dreamcatcher to record a dream from right here -
//  no menus. Tap anywhere else to wake back to the home screen.
// ============================================================
// ============================================================
//  TEST 072: SCREENSAVER - rewritten from scratch, raw gfx, no LVGL.
//
//  Every earlier version made the saver a full LVGL screen with a
//  full-screen button and image objects. That caused two things that
//  cost days: the previous screen bled through under it (overlap), and
//  a touch on the LVGL button hung the panel. Both are gone now because
//  the saver is NOT an LVGL screen at all.
//
//  How it works:
//    - saverActive is a plain flag. When idle passes SAVER_IDLE_MS we
//      paint the saver DIRECTLY with gfx-> over a black screen and set
//      the flag. LVGL is left completely alone (its widgets are simply
//      not on screen because we painted over them).
//    - While active, the loop reads the touch itself (touchGet). A touch
//      on the RIGHT third is the dreamcatcher: hold to record a dream,
//      release to stop (serial stub until the listener node exists).
//      A touch anywhere else exits: we repaint home via LVGL.
//    - No LVGL button, no second screen, no competing touch reader.
// ============================================================
#define SAVER_IDLE_MS 30000     // idle time before the screensaver appears

bool     saverActive   = false;   // true while the saver is painted
// dreamRecording is declared earlier (the Dreams tile uses it too); reuse it.
uint32_t dreamHoldStart = 0;      // when the current hold began
int      saverLastTempInt = -999; // so we only repaint the temp when it changes

// the dreamcatcher hit area: the right third of the screen
#define DREAM_X0   ((SCREEN_W * 2) / 3)

void saverPaintTemp(bool force);   // TEST 072: fwd decl, defined below
#define SAVER_TEMP_X 520      // TEST 077: 60 px (~7 mm) right of the old 460

// paint the whole saver from scratch (black + clock + temp + hint)
void saverPaintAll() {
  gfx->fillScreen(0x0000);                 // black

  // ---- temperature, top-right, big ----
  saverLastTempInt = -999;                 // force the temp to draw
  // (drawn by saverPaintTemp below)

  // ---- the real dial image on the left (340x340, TRUE_COLOR) ----
  // Draw it with raw gfx so LVGL stays out of the saver. Centre it near
  // the left-middle of the screen.
  const int DIAL_W = 340, DIAL_H = 340;
  const int DIAL_X = 40;                     // left margin
  const int DIAL_Y = (SCREEN_H - DIAL_H) / 2;
  gfx->draw16bitRGBBitmap(DIAL_X, DIAL_Y,
                          (uint16_t *)img_dial_map, DIAL_W, DIAL_H);

  // clock centre = middle of the dial image
  const int CX = DIAL_X + DIAL_W / 2;
  const int CY = DIAL_Y + DIAL_H / 2;

  // placeholder hands at 10:10 until real time arrives over the bus.
  // Drawn as clean lines over the dial (rotating the hand IMAGES would
  // need alpha + rotation in raw gfx; lines look tidy on the real face).
  // hour hand -> 305 deg, minute hand -> 60 deg
  {
    float ah = 305.0f * 0.01745329f;
    float am = 60.0f  * 0.01745329f;
    // draw each hand 3 px wide by drawing three parallel lines
    for (int o = -1; o <= 1; o++) {
      gfx->drawLine(CX + o, CY, CX + o + (int)(78  * sinf(ah)),
                                CY     - (int)(78  * cosf(ah)), 0xFFFF); // hour
      gfx->drawLine(CX, CY + o, CX + (int)(120 * sinf(am)),
                                CY + o - (int)(120 * cosf(am)), 0xFFFF); // minute
    }
    gfx->fillCircle(CX, CY, 7, 0xE800);                                  // red cap
  }

  // ---- temperature first (top-right), then the dreamcatcher below it ----
  saverPaintTemp(true);   // paints at the top-right; sets saverDreamX/Y anchor

  // ---- the real dreamcatcher image, 1.2x bigger, BELOW the temperature,
  //      aligned on the same right-side vertical line (TEST 077) ----
  // draw16bitRGBBitmap cannot scale, so we upscale 175->210 (x1.2) by
  // nearest-neighbour into a small static line buffer, row by row.
  const int DREAM_W = 175, DREAM_H = 175;
  const int DREAM_BW = 210, DREAM_BH = 210;          // 1.2x
  const int DREAM_IMG_X = SAVER_TEMP_X + 40;         // same right vertical as temp
  const int DREAM_IMG_Y = 210;                        // below the temperature
  {
    static uint16_t rowbuf[DREAM_BW];
    const uint16_t *src = (const uint16_t *)img_dream_map;
    for (int dy = 0; dy < DREAM_BH; dy++) {
      int sy = (dy * DREAM_H) / DREAM_BH;
      for (int dx = 0; dx < DREAM_BW; dx++) {
        int sx = (dx * DREAM_W) / DREAM_BW;
        rowbuf[dx] = src[sy * DREAM_W + sx];
      }
      gfx->draw16bitRGBBitmap(DREAM_IMG_X, DREAM_IMG_Y + dy, rowbuf, DREAM_BW, 1);
    }
  }
  gfx->setTextColor(0x8410);
  gfx->setTextSize(1);
  gfx->setCursor(DREAM_IMG_X + 20, DREAM_IMG_Y + DREAM_BH + 10);
  gfx->print("hold to record a dream");
}

// paint just the temperature; only redraws when the value changes so the
// panel does not flicker.
void saverPaintTemp(bool force) {
  int ti = gTempOK ? (int)lroundf(gTemp) : -999;
  if (!force && ti == saverLastTempInt) return;
  saverLastTempInt = ti;
  const int TX = SAVER_TEMP_X;
  const int TY = 70;
  // clear a generous area (top-right) before redrawing
  gfx->fillRect(TX - 20, TY - 8, 300, 130, 0x0000);
  // TEST 078: the default GFX font at size 7, big and clean. (The rounded
  // FreeSansBold font is not present in this GFX install, so we stay with
  // the built-in font which is always available.)
  gfx->setTextColor(0x8CFF);               // soft light blue
  gfx->setTextSize(7);
  gfx->setCursor(TX, TY);
  if (gTempOK) {
    gfx->print(ti);
    int cx = gfx->getCursorX();
    int cy = TY;
    gfx->drawCircle(cx + 16, cy + 8, 8, 0x8CFF);      // clean degree ring
    gfx->drawCircle(cx + 16, cy + 8, 7, 0x8CFF);
    gfx->setCursor(cx + 36, TY);
    gfx->print("C");
  } else {
    gfx->print("--");
  }
}

// enter the saver: paint it and raise the flag. LVGL is untouched.
void enterSaverNew() {
  saverActive = true;
  dreamRecording = false;
  saverPaintAll();
  Serial.println("[saver] entered (raw gfx)");
}

// exit the saver: drop the flag and let LVGL repaint the home screen.
void exitSaverNew() {
  saverActive = false;
  dreamRecording = false;
  if (scrHome) { lv_scr_load(scrHome); lv_refr_now(NULL); }
  lv_disp_trig_activity(NULL);
  Serial.println("[saver] exited -> home");
}

void buildSaver() {
  // TEST 072: nothing to build - the saver is painted directly with gfx,
  // it is not an LVGL screen. Kept as an empty function so the call in
  // setup() still compiles.
}

// ============================================================
//  TEST 037 - AC: tile + control screen, COPIED from panel_shemi
//  buildAC / refreshAC / the handlers / mkBtn are ported verbatim
//  from Shemi's panel. Only sendMsg is stubbed to a serial print
//  (panel_ira has no bus yet), evGoHome -> evBackHome, and the room
//  reading comes from panel_ira's own SHT31 (gTemp / gTempOK).
// ============================================================
#define NODE_AUDIO      9
#define CMD_AC_POWER   10
#define CMD_AC_TEMP    11
#define CMD_AC_MODE    12
#define CMD_AC_FAN     13
#define CMD_AC_SWING   14
#define CMD_AC_LIGHT   17   // AC display light (was 16 - clashed with CMD_AC_TIMER)
#define CMD_AC_TIMER   16          // TEST 038: value = hours, 0 off / 1 / 2 / 3
#define AC_TEMP_MIN    16
#define AC_TEMP_MAX    30

// no bus yet - this stub prints the 4-byte command that WOULD be sent
static void sendMsg(uint8_t bedId, uint8_t cmd, uint8_t target, uint8_t value) {
  Serial.printf("[bus] bedId %u cmd %u target %u value %u\n", bedId, cmd, target, value);
}

lv_obj_t *acImg = NULL;             // the home tile image
bool acPower = false;
int  acTemp  = 24;
int  acMode  = 0;                   // cool
int  acFan   = 3;                   // auto
int  acTimer = 0;                   // TEST 038: 0 off, 1/2/3 hours
bool acSwing = false;
bool acLight = false;
uint32_t acPressMs = 0;
bool acLongFired = false;
lv_obj_t *scrAC = NULL;
lv_obj_t *acLblTemp = NULL, *acLblRoom = NULL, *acLblPower = NULL;
lv_obj_t *acModeBtn[5] = {NULL}, *acFanBtn[4] = {NULL}, *acSwingBtn = NULL, *acLightBtn = NULL;
lv_obj_t *acTimerBtn[4] = {NULL};
static const char *AC_MODE_NAME[5] = { "Cool", "Heat", "Dry", "Fan", "Auto" };
static const char *AC_FAN_NAME[4]  = { "Low", "Med", "High", "Auto" };
static const char *AC_TIMER_NAME[4] = { "Off", "1h", "2h", "3h" };

void refreshACTile() {
  if (!acImg) return;
  lv_img_set_src(acImg, acPower ? &img_actile_on : &img_actile_off);
}

void refreshAC() {
  if (acLblTemp)
    lv_label_set_text_fmt(acLblTemp, "%d", acTemp);
  if (acLblPower) {
    lv_label_set_text(acLblPower, acPower ? "ON" : "OFF");
    lv_obj_set_style_text_color(acLblPower,
      lv_color_hex(acPower ? 0x40E080 : 0x808890), 0);
  }
  if (acLblRoom) {
    if (gTempOK) lv_label_set_text_fmt(acLblRoom, "%d", (int)lroundf(gTemp));
    else         lv_label_set_text(acLblRoom, "--");
  }
  for (int i = 0; i < 5; i++) if (acModeBtn[i])
    lv_obj_set_style_bg_color(acModeBtn[i],
      lv_color_hex(i == acMode ? 0x2080FF : 0x2A3346), 0);
  for (int i = 0; i < 4; i++) if (acFanBtn[i])
    lv_obj_set_style_bg_color(acFanBtn[i],
      lv_color_hex(i == acFan ? 0x2080FF : 0x2A3346), 0);
  if (acSwingBtn)
    lv_obj_set_style_bg_color(acSwingBtn,
      lv_color_hex(acSwing ? 0x2E6F4E : 0x2A3346), 0);
  if (acLightBtn)
    lv_obj_set_style_bg_color(acLightBtn,
      lv_color_hex(acLight ? 0xB0842E : 0x2A3346), 0);
  for (int i = 0; i < 4; i++) if (acTimerBtn[i])
    lv_obj_set_style_bg_color(acTimerBtn[i],
      lv_color_hex(i == acTimer ? 0x2080FF : 0x2A3346), 0);
}

static void evAcPower(lv_event_t *e) {
  acPower = !acPower;
  sendMsg(NODE_AUDIO, CMD_AC_POWER, 0, acPower ? 1 : 0);
  refreshAC();
}
static void evAcTemp(lv_event_t *e) {
  int delta = (int)(intptr_t)lv_event_get_user_data(e);
  acTemp += delta;
  if (acTemp < AC_TEMP_MIN) acTemp = AC_TEMP_MIN;
  if (acTemp > AC_TEMP_MAX) acTemp = AC_TEMP_MAX;
  sendMsg(NODE_AUDIO, CMD_AC_TEMP, 0, (uint8_t)acTemp);
  refreshAC();
}
static void evAcMode(lv_event_t *e) {
  acMode = (int)(intptr_t)lv_event_get_user_data(e);
  sendMsg(NODE_AUDIO, CMD_AC_MODE, 0, (uint8_t)acMode);
  refreshAC();
}
static void evAcFan(lv_event_t *e) {
  acFan = (int)(intptr_t)lv_event_get_user_data(e);
  sendMsg(NODE_AUDIO, CMD_AC_FAN, 0, (uint8_t)acFan);
  refreshAC();
}
static void evAcSwing(lv_event_t *e) {
  acSwing = !acSwing;
  sendMsg(NODE_AUDIO, CMD_AC_SWING, 0, acSwing ? 1 : 0);
  refreshAC();
}

static void evAcLight(lv_event_t *e) {
  acLight = !acLight;
  sendMsg(NODE_AUDIO, CMD_AC_LIGHT, 0, acLight ? 1 : 0);
  refreshAC();
}
static void evAcTimer(lv_event_t *e) {
  acTimer = (int)(intptr_t)lv_event_get_user_data(e);
  sendMsg(NODE_AUDIO, CMD_AC_TIMER, 0, (uint8_t)acTimer);
  refreshAC();
}

static lv_obj_t *mkBtn(lv_obj_t *par, int x, int y, int w, int h,
                       const char *text, lv_event_cb_t cb, int userData,
                       const lv_font_t *font) {
  lv_obj_t *b = lv_btn_create(par);
  lv_obj_set_size(b, w, h);
  lv_obj_set_pos(b, x, y);
  lv_obj_set_style_bg_color(b, lv_color_hex(0x2A3346), 0);
  lv_obj_set_style_radius(b, 10, 0);
  lv_obj_add_event_cb(b, cb, LV_EVENT_CLICKED, (void *)(intptr_t)userData);
  lv_obj_t *l = lv_label_create(b);
  lv_label_set_text(l, text);
  lv_obj_set_style_text_font(l, font, 0);
  lv_obj_set_style_text_color(l, lv_color_white(), 0);
  lv_obj_center(l);
  return b;
}

void buildAC() {
  scrAC = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(scrAC, lv_color_hex(0x101418), 0);
  lv_obj_clear_flag(scrAC, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t *back = lv_btn_create(scrAC);
  lv_obj_set_size(back, 90, 50);
  lv_obj_align(back, LV_ALIGN_TOP_LEFT, 10, 10);
  lv_obj_set_style_bg_color(back, lv_color_hex(0x303840), 0);
  lv_obj_add_event_cb(back, evBackHome, LV_EVENT_CLICKED, NULL);
  lv_obj_t *bl = lv_label_create(back);
  lv_label_set_text(bl, "<");
  lv_obj_set_style_text_font(bl, &lv_font_montserrat_28, 0);
  lv_obj_center(bl);

  lv_obj_t *ttl = lv_label_create(scrAC);
  lv_label_set_text(ttl, "Air Conditioner");
  lv_obj_set_style_text_font(ttl, &lv_font_montserrat_28, 0);
  lv_obj_set_style_text_color(ttl, lv_color_white(), 0);
  lv_obj_align(ttl, LV_ALIGN_TOP_MID, 0, 20);

  lv_obj_t *pw = lv_btn_create(scrAC);
  lv_obj_set_size(pw, 130, 50);
  lv_obj_align(pw, LV_ALIGN_TOP_RIGHT, -14, 10);
  lv_obj_set_style_bg_color(pw, lv_color_hex(0x303840), 0);
  lv_obj_add_event_cb(pw, evAcPower, LV_EVENT_CLICKED, NULL);
  acLblPower = lv_label_create(pw);
  lv_label_set_text(acLblPower, "OFF");
  lv_obj_set_style_text_font(acLblPower, &lv_font_montserrat_28, 0);
  lv_obj_center(acLblPower);

  mkBtn(scrAC, 120, 90, 90, 90, "-", evAcTemp, -1, &lv_font_montserrat_40);
  mkBtn(scrAC, 400, 90, 90, 90, "+", evAcTemp, +1, &lv_font_montserrat_40);

  acLblTemp = lv_label_create(scrAC);
  lv_label_set_text_fmt(acLblTemp, "%d", acTemp);
  lv_obj_set_style_text_font(acLblTemp, &lv_font_montserrat_40, 0);
  lv_obj_set_style_text_color(acLblTemp, lv_color_hex(0x60D0FF), 0);
  lv_obj_set_pos(acLblTemp, 275, 110);

  lv_obj_t *cu = lv_label_create(scrAC);
  lv_label_set_text(cu, "C");
  lv_obj_set_style_text_font(cu, &lv_font_montserrat_28, 0);
  lv_obj_set_style_text_color(cu, lv_color_hex(0x60D0FF), 0);
  lv_obj_set_pos(cu, 340, 120);

  lv_obj_t *roomCap = lv_label_create(scrAC);
  lv_label_set_text(roomCap, "room");
  lv_obj_set_style_text_font(roomCap, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(roomCap, lv_color_hex(0x7B90A0), 0);
  lv_obj_set_pos(roomCap, 546, 86);

  acLblRoom = lv_label_create(scrAC);
  lv_label_set_text(acLblRoom, "--");
  lv_obj_set_style_text_font(acLblRoom, &lv_font_montserrat_40, 0);
  lv_obj_set_style_text_color(acLblRoom, lv_color_hex(0x8FE0A0), 0);
  lv_obj_set_pos(acLblRoom, 546, 112);

  lv_obj_t *roomC = lv_label_create(scrAC);
  lv_label_set_text(roomC, "c");
  lv_obj_set_style_text_font(roomC, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(roomC, lv_color_hex(0x8FE0A0), 0);
  lv_obj_set_pos(roomC, 630, 140);

  lv_obj_t *roomNote = lv_label_create(scrAC);
  lv_label_set_text(roomNote, "measured here");
  lv_obj_set_style_text_font(roomNote, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(roomNote, lv_color_hex(0x556070), 0);
  lv_obj_set_pos(roomNote, 546, 168);

  lv_obj_t *ml = lv_label_create(scrAC);
  lv_label_set_text(ml, "Mode");
  lv_obj_set_style_text_font(ml, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(ml, lv_color_hex(0x7B90A0), 0);
  lv_obj_set_pos(ml, 20, 205);
  for (int i = 0; i < 5; i++)
    acModeBtn[i] = mkBtn(scrAC, 100 + i * 138, 195, 128, 58,
                         AC_MODE_NAME[i], evAcMode, i, &lv_font_montserrat_20);

  lv_obj_t *fl = lv_label_create(scrAC);
  lv_label_set_text(fl, "Fan");
  lv_obj_set_style_text_font(fl, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(fl, lv_color_hex(0x7B90A0), 0);
  lv_obj_set_pos(fl, 20, 285);
  for (int i = 0; i < 4; i++)
    acFanBtn[i] = mkBtn(scrAC, 100 + i * 138, 275, 128, 58,
                        AC_FAN_NAME[i], evAcFan, i, &lv_font_montserrat_20);

  lv_obj_t *sl = lv_label_create(scrAC);
  lv_label_set_text(sl, "Swing");
  lv_obj_set_style_text_font(sl, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(sl, lv_color_hex(0x7B90A0), 0);
  lv_obj_set_pos(sl, 20, 365);
  acSwingBtn = mkBtn(scrAC, 100, 355, 128, 58, "Swing",
                     evAcSwing, 0, &lv_font_montserrat_20);

  // AC display light on/off - the LIGHT button on the remote
  lv_obj_t *ll = lv_label_create(scrAC);
  lv_label_set_text(ll, "Light");
  lv_obj_set_style_text_font(ll, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(ll, lv_color_hex(0x7B90A0), 0);
  lv_obj_set_pos(ll, 250, 365);
  acLightBtn = mkBtn(scrAC, 330, 355, 128, 58, "Light",
                     evAcLight, 0, &lv_font_montserrat_20);

  // TEST 038: the off-timer row, right of the swing button
  lv_obj_t *tl = lv_label_create(scrAC);
  lv_label_set_text(tl, "Timer");
  lv_obj_set_style_text_font(tl, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(tl, lv_color_hex(0x7B90A0), 0);
  lv_obj_set_pos(tl, 245, 374);
  for (int i = 0; i < 4; i++)
    acTimerBtn[i] = mkBtn(scrAC, 320 + i * 115, 355, 108, 58,
                          AC_TIMER_NAME[i], evAcTimer, i, &lv_font_montserrat_20);

  lv_obj_t *note = lv_label_create(scrAC);
  lv_label_set_text(note, "shows what was last commanded - this AC does not report back");
  lv_obj_set_style_text_font(note, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(note, lv_color_hex(0x556070), 0);
  lv_obj_align(note, LV_ALIGN_BOTTOM_MID, 0, -8);

  refreshAC();
}

// ---- AC tile: short press toggles, long press opens the screen ----
static void evTileACPressed(lv_event_t *e) {
  acPressMs = millis();
  acLongFired = false;
}
static void evTileACLong(lv_event_t *e) {
  acLongFired = true;
  // TEST 039: opening the AC screen turns the AC on by default
  if (!acPower) {
    acPower = true;
    sendMsg(NODE_AUDIO, CMD_AC_POWER, 0, 1);
    refreshACTile();
  }
  refreshAC();                 // also pulls the fresh room temperature
  lv_scr_load(scrAC);
}
static void evTileACReleased(lv_event_t *e) {
  if (acLongFired) return;
  acPower = !acPower;
  sendMsg(NODE_AUDIO, CMD_AC_POWER, 0, acPower ? 1 : 0);
  refreshACTile();
  refreshAC();
}

// ============================================================
//  TEST 035 - the light tile: img_lamp recoloured off / on
//  Grey (unlit) when off, warm amber when on, toggled on tap.
//  Same idea as panel_shemi. Real light control needs the bus.
// ============================================================
lv_obj_t *lampImg = NULL;
static bool lightOn = false;
#define LAMP_COL_OFF 0x9098A0
#define LAMP_COL_ON  0xE8A030
static void refreshLamp() {
  if (!lampImg) return;
  if (lightOn) { lv_obj_set_style_img_recolor(lampImg, lv_color_hex(LAMP_COL_ON), 0);
                 lv_obj_set_style_img_recolor_opa(lampImg, 100, 0); }
  else { lv_obj_set_style_img_recolor(lampImg, lv_color_hex(LAMP_COL_OFF), 0);
         lv_obj_set_style_img_recolor_opa(lampImg, 205, 0); }
}
static void evLampToggle(lv_event_t *e) {
  lightOn = !lightOn; refreshLamp();
  Serial.printf("[light] %s -> would send CMD_RF light %d\n", lightOn ? "ON" : "OFF", lightOn ? 1 : 0);
}

// ============================================================
//  TEST 040 - the shade tile, COPIED from panel_shemi
//  The picture flips open<->closed the moment it is pressed; the
//  real shutter still takes 20 s and a second press sends STOP.
//  sendMsg is the serial stub; refreshRoom() is dropped (panel_ira
//  has no Room screen).
// ============================================================
#define CMD_RF           15      // TEST 040: target 0 shade, 1 light
#define SHADE_TIMEOUT_MS 45000UL
#define SHADE_TRAVEL_MS  20000UL
int      shadePos = 0;           // 0 open, 1000 closed
int      shadeMoving = 0;        // 0 stopped, 1 going up, 2 going down
uint32_t shadeStartMs = 0;
uint32_t shadeMoveStart = 0;
lv_obj_t *tileShade = NULL, *tileShadeImg = NULL;

void refreshShadeTile() {
  if (!tileShadeImg) return;
  lv_img_set_src(tileShadeImg,
                 shadePos > 500 ? &img_shade_closed : &img_shade_open);
}

static void evTileShade(lv_event_t *e) {
  if (shadeMoving != 0) {
    sendMsg(NODE_AUDIO, CMD_RF, 0, 0);          // STOP
    shadeMoving = 0;
    Serial.printf("shade: stopped, picture stays as it is\n");
  } else {
    bool goDown = (shadePos < 500);
    shadePos    = goDown ? 1000 : 0;
    shadeMoving = goDown ? 2 : 1;
    shadeMoveStart = millis();
    shadeStartMs   = millis();
    sendMsg(NODE_AUDIO, CMD_RF, 0, goDown ? 2 : 1);
  }
  refreshShadeTile();
}

// ============================================================
//  TEST 041 - FAN, modelled on the AC tile + screen
//  Fan tile: short press toggles on/off; 3 s hold opens the fan
//  control screen (power, 6 speeds, off-timer, light) - the same
//  shape as the AC. The tile swaps two photos of the real fan -
//  off = light dark, on = light glowing - just like the AC tile.
//  All sends are the serial stub until the bus exists.
// ============================================================
#define CMD_FAN_POWER  18
#define CMD_FAN_SPEED  19
#define CMD_FAN_TIMER  20
#define CMD_FAN_LIGHT  21
#define FAN_SPEEDS      6          // change to 4 for a 4-speed fan

lv_obj_t *fanImg = NULL, *fanPowerBtn = NULL, *fanLblPower = NULL;
lv_obj_t *fanSpeedBtn[FAN_SPEEDS] = {NULL};
lv_obj_t *fanTimerBtn[4] = {NULL};
lv_obj_t *fanLightBtn = NULL;
lv_obj_t *scrFan = NULL;
bool fanPower = false;
int  fanSpeed = 1;                 // 1..FAN_SPEEDS
int  fanTimer = 0;                 // 0 off, 1/2/3 hours
bool fanLight = false;
uint32_t fanPressMs = 0;
bool fanLongFired = false;
static const char *FAN_TIMER_NAME[4] = { "Off", "1h", "2h", "3h" };

// ============================================================
//  TEST 049: the massage screen - copied verbatim from panel_shemi,
//  adapted for panel_ira. The bed command enum (0..5) is free here
//  (panel_ira's own commands start at 9). sendMsg is the serial stub,
//  and the header shows a placeholder time (panel_ira has no real
//  clock until the RS-485 bus is built) plus the live SHT31 temp.
// ============================================================
enum Cmd { CMD_OFF = 0, CMD_ALL = 1, CMD_ZONE = 2,
           CMD_MOTOR = 3, CMD_PRESET = 4, CMD_TIMER = 5 };

uint8_t curBedId = 2;        // panel_ira defaults to Ira's bed (id 2)
bool randomActive = false;
int  activePattern = -1;     // -1 = none lit

// The twelve massage mode names, stored pre-reversed (visual order)
// exactly as in panel_shemi's font_hebrew build.
#define P_MAPAL      "לפמ"   // מפל
#define P_ALIYA      "היילע"   // עלייה
#define P_NADNEDA    "הדנדנ"   // נדנדה
#define P_MALE       "אלמ"   // מלא
#define P_ALACHSON   "ןוסכלא"   // אלכסון
#define P_TZAD       "דצ"   // צד
#define P_SICHRUR    "רורחס"   // סחרור
#define P_LISHA      "השיל"   // לישה
#define P_DOFEK      "קפוד"   // דופק
#define P_NESHIMA    "המישנ"   // נשימה
#define P_GESHEM     "םשג"   // גשם
#define P_AKRAI      "יארקא"   // אקראי
#define M_OFF        "יוביכ"   // כיבוי

const uint8_t PAT_MAP[12] = { 0, 1, 2, 255, 3, 4, 5, 6, 7, 8, 9, 10 };
const uint32_t PAT_BG_ON[3]  = { 0x0F6E56, 0x534AB7, 0x993C1D };
const uint32_t PAT_BG_OFF[3] = { 0x0B3D31, 0x2E2870, 0x6B2A14 };
const uint32_t PAT_TX_ON[3]  = { 0x9FE1CB, 0xCECBF6, 0xF5C4B3 };
const uint32_t PAT_TX_OFF[3] = { 0x5DCAA5, 0xAFA9EC, 0xF0997B };

lv_obj_t *sliderZone[4], *lblZoneVal[4];
lv_obj_t *patTile[12] = { NULL };
lv_obj_t *lblMassTime = NULL, *lblMassDate = NULL, *lblMassTemp = NULL;
lv_obj_t *lblHomeTitle = NULL, *lblHomeClock = NULL, *lblHomeTemp = NULL;  // TEST 076: home header

// panel_ira has no real clock yet, so the massage header shows a
// fixed placeholder time. When the RS-485 bus is built, this will be
// replaced by the time the bus delivers from panel_shemi.
long nowSecOfDay() { return -1; }

void refreshPatTiles() {
  for (int i = 0; i < 12; i++) {
    if (!patTile[i]) continue;
    int row = i / 4;
    bool on = (i == activePattern);
    lv_obj_set_style_bg_color(patTile[i],
      lv_color_hex(on ? PAT_BG_ON[row] : PAT_BG_OFF[row]), 0);
    lv_obj_set_style_border_width(patTile[i], on ? 3 : 0, 0);
    lv_obj_set_style_border_color(patTile[i], lv_color_hex(PAT_TX_ON[row]), 0);
    lv_obj_t *l = lv_obj_get_child(patTile[i], 0);
    if (l) lv_obj_set_style_text_color(l,
             lv_color_hex(on ? PAT_TX_ON[row] : PAT_TX_OFF[row]), 0);
  }
}

static void evZoneSlider(lv_event_t *e) {
  lv_obj_t *s = lv_event_get_target(e);
  int zone = (int)(intptr_t)lv_event_get_user_data(e);
  int v = lv_slider_get_value(s);
  lv_label_set_text_fmt(lblZoneVal[zone], "%d%%", v);
  if (lv_event_get_code(e) == LV_EVENT_RELEASED)
    sendMsg(curBedId, CMD_ZONE, zone, v);
}

static void evPreset(lv_event_t *e) {
  int i = (int)(intptr_t)lv_event_get_user_data(e);
  randomActive = false;
  activePattern = i;
  refreshPatTiles();
  if (PAT_MAP[i] == 255) sendMsg(curBedId, CMD_ALL, 0, 100);
  else                   sendMsg(curBedId, CMD_PRESET, PAT_MAP[i], 60);
  Serial.printf("mode tile %d -> %s\n", i,
                PAT_MAP[i] == 255 ? "ALL 100" : "preset");
}

static void evTimer(lv_event_t *e) {
  int mins = (int)(intptr_t)lv_event_get_user_data(e);
  sendMsg(curBedId, CMD_TIMER, 0, mins);
}

static void evOff(lv_event_t *e) {
  randomActive = false;
  activePattern = -1;
  refreshPatTiles();
  sendMsg(curBedId, CMD_OFF, 0, 0);
  for (int z = 0; z < 4; z++) {
    lv_slider_set_value(sliderZone[z], 0, LV_ANIM_ON);
    lv_label_set_text(lblZoneVal[z], "0%");
  }
}

void massHeaderTick(lv_timer_t *t) {
  if (!lblMassTime) return;
  // TEST 053: only update while the massage screen is on show. The timer
  // fires every second regardless of screen; touching these hidden
  // labels was invalidating LVGL and flickering the panel.
  if (lv_scr_act() != scrMassage) return;
  // panel_ira: placeholder time until the bus delivers the real one.
  lv_label_set_text(lblMassTime, "10:10");
  lv_label_set_text(lblMassDate, "");
  if (gTempOK) lv_label_set_text_fmt(lblMassTemp, "%d", (int)lroundf(gTemp));
  else         lv_label_set_text(lblMassTemp, "--");
}

// ============================================================
//  TEST 049: the radio screen - copied verbatim from panel_shemi,
//  adapted for panel_ira. sendMsg is the serial stub (no bus yet),
//  and the command numbers were moved to a free range so they do
//  not collide with panel_ira's AC / fan / RF numbers.
//  Radio audio itself needs the audio node, which is not built.
// ============================================================
#define CMD_RADIO_PLAY  22      // target = station index
#define CMD_RADIO_STOP  23
#define CMD_RADIO_VOL   24      // value 0..21
#define CMD_RADIO_SLEEP 25      // value = minutes

bool radioPlaying = false;
int  radioStation = -1;         // which tile is lit
int  radioVolume  = 12;         // 0..21, the audio node's own scale
lv_obj_t *radioNowLbl = NULL, *radioVolLbl = NULL;
lv_obj_t *radioBtn[9] = { NULL };

struct RadioStation { const char *name; uint32_t colour; };
const RadioStation RADIO[8] = {
  { "Kan 88",      0x8C24FF },
  { "Kan Gimel",   0xFF931E },
  { "Galgalatz",   0x2C58A8 },
  { "Eco 99",      0x2E8A5C },
  { "Reshet Bet",  0x5F6673 },
  { "Kol HaMusica",0x7A5BB5 },
  { "Groove Salad",0x1D7A94 },
  { "Drone Zone",  0xA04F7E },
};

// panel_ira's home radio tile is a plain image with no on/off variant
// wired up, so this is a stub - it keeps refreshRadio identical to
// panel_shemi without needing a second tile image.
void refreshRadioTile() { }

void refreshRadio() {
  if (radioNowLbl) {
    if (radioPlaying && radioStation >= 0 && radioStation < 8)
      lv_label_set_text_fmt(radioNowLbl, "playing  %s", RADIO[radioStation].name);
    else
      lv_label_set_text(radioNowLbl, "stopped");
  }
  if (radioVolLbl) lv_label_set_text_fmt(radioVolLbl, "Volume  %d", radioVolume);
  for (int i = 0; i < 8; i++) {
    if (!radioBtn[i]) continue;
    bool on = (radioPlaying && radioStation == i);
    lv_obj_set_style_border_width(radioBtn[i], on ? 4 : 0, 0);
    lv_obj_set_style_border_color(radioBtn[i], lv_color_white(), 0);
    lv_obj_set_style_bg_opa(radioBtn[i], on ? LV_OPA_COVER : 200, 0);
  }
  refreshRadioTile();
}

static void evRadioPlay(lv_event_t *e) {
  int n = (int)(intptr_t)lv_event_get_user_data(e);
  radioStation = n;
  radioPlaying = true;
  sendMsg(NODE_AUDIO, CMD_RADIO_PLAY, (uint8_t)n, 0);
  refreshRadio();
}
static void evRadioStop(lv_event_t *e) {
  radioPlaying = false;
  radioStation = -1;
  sendMsg(NODE_AUDIO, CMD_RADIO_STOP, 0, 0);
  refreshRadio();
}
static void evRadioVol(lv_event_t *e) {
  lv_obj_t *s = lv_event_get_target(e);
  radioVolume = lv_slider_get_value(s);
  sendMsg(NODE_AUDIO, CMD_RADIO_VOL, 0, (uint8_t)radioVolume);
  refreshRadio();
}
static void evRadioSleep(lv_event_t *e) {
  sendMsg(NODE_AUDIO, CMD_RADIO_SLEEP, 0, 30);
  Serial.println("radio: sleep timer 30 minutes");
}

void buildRadio() {
  scrRadio = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(scrRadio, lv_color_hex(0x101418), 0);
  lv_obj_clear_flag(scrRadio, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t *back = lv_btn_create(scrRadio);
  lv_obj_set_size(back, 90, 50);
  lv_obj_align(back, LV_ALIGN_TOP_LEFT, 10, 10);
  lv_obj_set_style_bg_color(back, lv_color_hex(0x303840), 0);
  lv_obj_add_event_cb(back, evBackHome, LV_EVENT_CLICKED, NULL);
  lv_obj_t *bl = lv_label_create(back);
  lv_label_set_text(bl, "<");
  lv_obj_set_style_text_font(bl, &lv_font_montserrat_28, 0);
  lv_obj_center(bl);

  radioNowLbl = lv_label_create(scrRadio);
  lv_label_set_text(radioNowLbl, "stopped");
  lv_obj_set_style_text_font(radioNowLbl, &lv_font_montserrat_28, 0);
  lv_obj_set_style_text_color(radioNowLbl, lv_color_hex(0x8FE0A0), 0);
  lv_obj_align(radioNowLbl, LV_ALIGN_TOP_MID, 30, 22);

  // eight station tiles, 4 across and 2 down
  const int SW = 178, SH = 84, SX0 = 20, SGX = 14, SY0 = 92, SGY = 12;
  for (int i = 0; i < 8; i++) {
    int c = i % 4, r = i / 4;
    lv_obj_t *b = lv_btn_create(scrRadio);
    radioBtn[i] = b;
    lv_obj_set_size(b, SW, SH);
    lv_obj_set_pos(b, SX0 + c * (SW + SGX), SY0 + r * (SH + SGY));
    lv_obj_set_style_bg_color(b, lv_color_hex(RADIO[i].colour), 0);
    lv_obj_set_style_radius(b, 12, 0);
    lv_obj_add_event_cb(b, evRadioPlay, LV_EVENT_CLICKED, (void *)(intptr_t)i);
    lv_obj_t *l = lv_label_create(b);
    lv_label_set_text(l, RADIO[i].name);
    lv_obj_set_style_text_font(l, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(l, lv_color_white(), 0);
    lv_obj_center(l);
  }

  // volume
  radioVolLbl = lv_label_create(scrRadio);
  lv_label_set_text_fmt(radioVolLbl, "Volume  %d", radioVolume);
  lv_obj_set_style_text_font(radioVolLbl, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(radioVolLbl, lv_color_hex(0x90A0B0), 0);
  lv_obj_set_pos(radioVolLbl, 22, 296);

  lv_obj_t *sl = lv_slider_create(scrRadio);
  lv_obj_set_size(sl, 470, 22);
  lv_obj_set_pos(sl, 22, 326);
  lv_slider_set_range(sl, 0, 21);
  lv_slider_set_value(sl, radioVolume, LV_ANIM_OFF);
  lv_obj_add_event_cb(sl, evRadioVol, LV_EVENT_VALUE_CHANGED, NULL);

  // sleep and stop
  lv_obj_t *sleepB = lv_btn_create(scrRadio);
  lv_obj_set_size(sleepB, 150, 60);
  lv_obj_set_pos(sleepB, 520, 306);
  lv_obj_set_style_bg_color(sleepB, lv_color_hex(0x2A3346), 0);
  lv_obj_add_event_cb(sleepB, evRadioSleep, LV_EVENT_CLICKED, NULL);
  lv_obj_t *sll2 = lv_label_create(sleepB);
  lv_label_set_text(sll2, "Sleep 30");
  lv_obj_set_style_text_font(sll2, &lv_font_montserrat_20, 0);
  lv_obj_center(sll2);

  lv_obj_t *stopB = lv_btn_create(scrRadio);
  lv_obj_set_size(stopB, 100, 60);
  lv_obj_set_pos(stopB, 682, 306);
  lv_obj_set_style_bg_color(stopB, lv_color_hex(0x8A3A3A), 0);
  lv_obj_add_event_cb(stopB, evRadioStop, LV_EVENT_CLICKED, NULL);
  lv_obj_t *stl = lv_label_create(stopB);
  lv_label_set_text(stl, "Stop");
  lv_obj_set_style_text_font(stl, &lv_font_montserrat_20, 0);
  lv_obj_center(stl);

  // Spotify - honest about not being connected
  lv_obj_t *sp = lv_label_create(scrRadio);
  lv_label_set_text(sp, "Spotify: not connected - needs cspot on the audio node");
  lv_obj_set_style_text_font(sp, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(sp, lv_color_hex(0x556070), 0);
  lv_obj_align(sp, LV_ALIGN_BOTTOM_MID, 0, -34);

  lv_obj_t *nb = lv_label_create(scrRadio);
  lv_label_set_text(nb, "nothing plays yet - the audio node is not built");
  lv_obj_set_style_text_font(nb, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(nb, lv_color_hex(0x556070), 0);
  lv_obj_align(nb, LV_ALIGN_BOTTOM_MID, 0, -10);

  refreshRadio();
}

void buildMassage() {
  scrMassage = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(scrMassage, lv_color_hex(0x0D0F16), 0);
  lv_obj_clear_flag(scrMassage, LV_OBJ_FLAG_SCROLLABLE);

  // ---------------- header ----------------
  lv_obj_t *back = lv_btn_create(scrMassage);
  lv_obj_set_size(back, 80, 44);
  lv_obj_align(back, LV_ALIGN_TOP_LEFT, 8, 8);
  lv_obj_set_style_bg_color(back, lv_color_hex(0x1A1E2A), 0);
  lv_obj_add_event_cb(back, evBackHome, LV_EVENT_CLICKED, NULL);
  lv_obj_t *bl = lv_label_create(back);
  lv_label_set_text(bl, "<");
  lv_obj_set_style_text_font(bl, &lv_font_montserrat_28, 0);
  lv_obj_center(bl);

  lblMassTime = lv_label_create(scrMassage);
  lv_label_set_text(lblMassTime, "--:--");
  lv_obj_set_style_text_font(lblMassTime, &lv_font_montserrat_40, 0);
  lv_obj_set_style_text_color(lblMassTime, lv_color_hex(0xE8EAF0), 0);
  lv_obj_align(lblMassTime, LV_ALIGN_TOP_RIGHT, -280, 8);

  lblMassDate = lv_label_create(scrMassage);
  lv_label_set_text(lblMassDate, "");
  lv_obj_set_style_text_font(lblMassDate, &lv_font_montserrat_28, 0);
  lv_obj_set_style_text_color(lblMassDate, lv_color_hex(0x8992A6), 0);
  lv_obj_align(lblMassDate, LV_ALIGN_TOP_RIGHT, -150, 16);

  lblMassTemp = lv_label_create(scrMassage);
  lv_label_set_text(lblMassTemp, "--");
  lv_obj_set_style_text_font(lblMassTemp, &lv_font_montserrat_28, 0);
  lv_obj_set_style_text_color(lblMassTemp, lv_color_hex(0xE8EAF0), 0);
  lv_obj_align(lblMassTemp, LV_ALIGN_TOP_RIGHT, -46, 16);

  lv_obj_t *cSmall = lv_label_create(scrMassage);
  lv_label_set_text(cSmall, "c");
  lv_obj_set_style_text_font(cSmall, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(cSmall, lv_color_hex(0x8992A6), 0);
  lv_obj_align(cSmall, LV_ALIGN_TOP_RIGHT, -24, 26);

  lv_obj_t *rule = lv_obj_create(scrMassage);
  lv_obj_set_size(rule, 780, 1);
  lv_obj_align(rule, LV_ALIGN_TOP_MID, 0, 60);
  lv_obj_set_style_bg_color(rule, lv_color_hex(0x232735), 0);
  lv_obj_set_style_border_width(rule, 0, 0);

  const char *PN[12] = {
    P_MAPAL,     P_ALIYA,   P_NADNEDA, P_MALE,
    P_ALACHSON,  P_TZAD,    P_SICHRUR, P_LISHA,
    P_DOFEK,     P_NESHIMA, P_GESHEM,  P_AKRAI
  };
  const int TW = 139, TH = 106, TX = 16, TY = 74, TGX = 8, TGY = 8;
  for (int i = 0; i < 12; i++) {
    int col = i % 4, row = i / 4;
    lv_obj_t *b = lv_btn_create(scrMassage);
    lv_obj_set_size(b, TW, TH);
    lv_obj_align(b, LV_ALIGN_TOP_LEFT, TX + col * (TW + TGX), TY + row * (TH + TGY));
    lv_obj_set_style_radius(b, 10, 0);
    lv_obj_set_style_shadow_width(b, 0, 0);
    lv_obj_add_event_cb(b, evPreset, LV_EVENT_CLICKED, (void *)(intptr_t)i);
    lv_obj_t *l = lv_label_create(b);
    lv_label_set_text(l, PN[i]);
    lv_obj_set_style_text_font(l, &font_hebrew_28, 0);
    lv_obj_center(l);
    patTile[i] = b;
  }
  refreshPatTiles();

  const int SW = 30, SH = 250, SX = 618, SY = 86, SGX = 42;
  for (int z = 0; z < 4; z++) {
    int x = SX + z * SGX;

    lblZoneVal[z] = lv_label_create(scrMassage);
    lv_label_set_text(lblZoneVal[z], "0");
    lv_obj_set_style_text_font(lblZoneVal[z], &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(lblZoneVal[z], lv_color_hex(0x8992A6), 0);
    lv_obj_align(lblZoneVal[z], LV_ALIGN_TOP_LEFT, x, SY - 24);

    sliderZone[z] = lv_slider_create(scrMassage);
    lv_obj_set_size(sliderZone[z], SW, SH);
    lv_obj_align(sliderZone[z], LV_ALIGN_TOP_LEFT, x, SY);
    lv_slider_set_range(sliderZone[z], 0, 100);
    lv_obj_set_style_bg_color(sliderZone[z], lv_color_hex(0x1A1E2A), LV_PART_MAIN);
    lv_obj_set_style_bg_color(sliderZone[z], lv_color_hex(0x12B886), LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(sliderZone[z], lv_color_hex(0x2BD4A0), LV_PART_KNOB);
    lv_obj_add_event_cb(sliderZone[z], evZoneSlider, LV_EVENT_VALUE_CHANGED, (void *)(intptr_t)z);
    lv_obj_add_event_cb(sliderZone[z], evZoneSlider, LV_EVENT_RELEASED, (void *)(intptr_t)z);

    lv_obj_t *nm = lv_label_create(scrMassage);
    lv_label_set_text_fmt(nm, "%d", z + 1);
    lv_obj_set_style_text_font(nm, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(nm, lv_color_hex(0xC8CDDB), 0);
    lv_obj_align(nm, LV_ALIGN_TOP_LEFT, x + 8, SY + SH + 6);
  }

  const int tm[3] = { 15, 30, 60 };
  for (int i = 0; i < 3; i++) {
    lv_obj_t *b = lv_btn_create(scrMassage);
    lv_obj_set_size(b, 150, 52);
    lv_obj_align(b, LV_ALIGN_BOTTOM_LEFT, 16 + i * 162, -10);
    lv_obj_set_style_radius(b, 10, 0);
    lv_obj_set_style_shadow_width(b, 0, 0);
    lv_obj_set_style_bg_color(b, lv_color_hex(0x1F5A8A), 0);
    lv_obj_add_event_cb(b, evTimer, LV_EVENT_CLICKED, (void *)(intptr_t)tm[i]);
    lv_obj_t *l = lv_label_create(b);
    lv_label_set_text_fmt(l, "%d", tm[i]);
    lv_obj_set_style_text_font(l, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(l, lv_color_hex(0xB5D4F4), 0);
    lv_obj_center(l);
  }

  lv_obj_t *off = lv_btn_create(scrMassage);
  lv_obj_set_size(off, 280, 52);
  lv_obj_align(off, LV_ALIGN_BOTTOM_RIGHT, -16, -10);
  lv_obj_set_style_radius(off, 10, 0);
  lv_obj_set_style_shadow_width(off, 0, 0);
  lv_obj_set_style_bg_color(off, lv_color_hex(0x8F1F1F), 0);
  lv_obj_add_event_cb(off, evOff, LV_EVENT_CLICKED, NULL);
  lv_obj_t *ol = lv_label_create(off);
  lv_label_set_text(ol, M_OFF);
  lv_obj_set_style_text_font(ol, &font_hebrew_28, 0);
  lv_obj_set_style_text_color(ol, lv_color_hex(0xF7C1C1), 0);
  lv_obj_center(ol);

  lv_timer_create(massHeaderTick, 1000, NULL);
  massHeaderTick(NULL);
}

// ============================================================
//  TEST 051: the settings screen - ported from panel_shemi (option
//  B: same structure and look, values are display-only for now).
//  Differences from panel_shemi: NO Clock tab (panel_ira gets time
//  over the bus), and the Files tab lists dream recordings split into
//  Shemi / Ira sections by filename prefix. saveSettings is a serial
//  stub (panel_ira has no NVS wiring yet), and About shows panel_ira's
//  own sensors. The list is empty until the listener node is built.
// ============================================================
#define SET_PAGE_BG  0x2A3038      // the tab page behind everything
#define SET_BAR_BG   0x1E242B      // the tab bar across the top
#define SET_TEXT_COL 0xE8ECF2      // primary text on that grey
#define SET_DIM_COL  0x9AA6B2      // secondary text, still readable

// display-only setting values (mirrors panel_shemi; not yet wired to
// panel_ira's own SAVER_DIM / SAVER_IDLE_MS - that is a later test)
int brightFloor = 40;
int saverFloor  = 4;
uint32_t saverTimeoutMs = 300000UL;
int setMinSmall = 60, setMinBig = 165;
int randomChar  = 1;                 // 0 gentle 1 lively 2 wild

lv_obj_t *lblAbout = NULL;
lv_obj_t *fileList = NULL;
lv_obj_t *lblFloorVal = NULL, *ddSaver = NULL, *sldSmall = NULL, *sldBig = NULL,
         *lblSmall = NULL, *lblBig = NULL, *ddRandom = NULL, *lblNightVal = NULL;

// panel_ira has no NVS wiring yet; this is where panel_shemi persists
// settings. Kept as a serial stub so the handlers stay identical.
void saveSettings() {
  Serial.printf("[settings] floor %d night %d saver %lu small %d big %d rand %d\n",
                brightFloor, saverFloor, (unsigned long)saverTimeoutMs,
                setMinSmall, setMinBig, randomChar);
}

void refreshAbout() {
  if (!lblAbout || !lv_obj_is_valid(lblAbout)) return;
  char info[320];
  char tempTxt[32];
  if (gTempOK) snprintf(tempTxt, sizeof(tempTxt), "%.1f C", gTemp);
  else         snprintf(tempTxt, sizeof(tempTxt), "-- no reading --");
  snprintf(info, sizeof(info),
    "BG BEDROOM - IRA PANEL\nTEST %03d\n\n"
    "Temp     : %s\n"
    "SHT31    : %s\n"
    "BH1750   : %s\n"
    "Touch    : %s\n"
    "Expander : %s\n\n"
    "Time comes from the main panel over the bus.\n"
    "Dream recordings are stored on the microSD.",
    TEST_NUMBER,
    tempTxt,
    haveSHT31  ? "ok" : "absent",
    haveBH1750 ? "ok" : "absent",
    haveTouch  ? "GT911 ok" : "absent",
    haveCH422  ? "CH422G ok" : "absent");
  lv_label_set_text(lblAbout, info);
}

static void evSettingsTab(lv_event_t *e) {
  // panel_ira: nothing to populate on demand yet (no SD card reading
  // until the listener node exists). Kept so the tabview has its cb.
  (void)e;
}

static void evFloor(lv_event_t *e) {
  brightFloor = lv_slider_get_value(lv_event_get_target(e));
  lv_label_set_text_fmt(lblFloorVal, "%d%%", brightFloor);
  saveSettings();
}
static void evNightFloor(lv_event_t *e) {
  saverFloor = lv_slider_get_value(lv_event_get_target(e));
  lv_label_set_text_fmt(lblNightVal, "%d%%", saverFloor);
  saveSettings();
}
static void evSaverTimeout(lv_event_t *e) {
  const uint32_t opts[4] = { 30000UL, 60000UL, 300000UL, 600000UL };
  saverTimeoutMs = opts[lv_dropdown_get_selected(ddSaver)];
  saveSettings();
}
static void evSmall(lv_event_t *e) {
  setMinSmall = lv_slider_get_value(lv_event_get_target(e));
  lv_label_set_text_fmt(lblSmall, "%d", setMinSmall); saveSettings();
}
static void evBig(lv_event_t *e) {
  setMinBig = lv_slider_get_value(lv_event_get_target(e));
  lv_label_set_text_fmt(lblBig, "%d", setMinBig); saveSettings();
}
static void evRandom(lv_event_t *e) {
  randomChar = lv_dropdown_get_selected(ddRandom); saveSettings();
}

void buildSettings() {
  scrSettings = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(scrSettings, lv_color_hex(0x101418), 0);

  lv_obj_t *back = lv_btn_create(scrSettings);
  lv_obj_set_size(back, 90, 46);
  lv_obj_align(back, LV_ALIGN_TOP_LEFT, 8, 6);
  lv_obj_add_event_cb(back, evBackHome, LV_EVENT_CLICKED, NULL);
  lv_obj_t *bl = lv_label_create(back); lv_label_set_text(bl, "<");
  lv_obj_set_style_text_font(bl, &lv_font_montserrat_28, 0); lv_obj_center(bl);

  lv_obj_t *tv = lv_tabview_create(scrSettings, LV_DIR_TOP, 48);
  lv_obj_set_size(tv, 800, 424);
  lv_obj_align(tv, LV_ALIGN_BOTTOM_MID, 0, 0);

  lv_obj_set_style_bg_color(tv, lv_color_hex(SET_PAGE_BG), 0);
  lv_obj_set_style_bg_opa(tv, LV_OPA_COVER, 0);

  lv_obj_t *tabbar = lv_tabview_get_tab_btns(tv);
  lv_obj_set_style_bg_color(tabbar, lv_color_hex(SET_BAR_BG), 0);
  lv_obj_set_style_bg_opa(tabbar, LV_OPA_COVER, 0);
  lv_obj_set_style_text_color(tabbar, lv_color_hex(SET_DIM_COL), 0);
  lv_obj_set_style_text_color(tabbar, lv_color_hex(SET_TEXT_COL),
                              LV_PART_ITEMS | LV_STATE_CHECKED);
  lv_obj_set_style_bg_color(tabbar, lv_color_hex(SET_PAGE_BG),
                            LV_PART_ITEMS | LV_STATE_CHECKED);
  lv_obj_set_style_bg_opa(tabbar, LV_OPA_COVER,
                          LV_PART_ITEMS | LV_STATE_CHECKED);

  // TEST 051: NO Clock tab - panel_ira gets time from the main panel.
  lv_obj_t *tDisp  = lv_tabview_add_tab(tv, "Display");
  lv_obj_t *tMass  = lv_tabview_add_tab(tv, "Massage");
  lv_obj_t *tFiles = lv_tabview_add_tab(tv, "Files");
  lv_obj_t *tAbout = lv_tabview_add_tab(tv, "About");

  lv_obj_t *pages[4] = { tDisp, tMass, tFiles, tAbout };
  for (int i = 0; i < 4; i++) {
    lv_obj_set_style_bg_color(pages[i], lv_color_hex(SET_PAGE_BG), 0);
    lv_obj_set_style_bg_opa(pages[i], LV_OPA_COVER, 0);
    lv_obj_set_style_text_color(pages[i], lv_color_hex(SET_TEXT_COL), 0);
  }
  lv_obj_t *cont = lv_tabview_get_content(tv);
  lv_obj_set_style_bg_color(cont, lv_color_hex(SET_PAGE_BG), 0);
  lv_obj_set_style_bg_opa(cont, LV_OPA_COVER, 0);
  lv_obj_add_event_cb(tv, evSettingsTab, LV_EVENT_VALUE_CHANGED, NULL);

  // ---- Display ----
  lv_obj_t *fl=lv_label_create(tDisp); lv_label_set_text(fl,"Min brightness (day)");
  lv_obj_set_style_text_font(fl,&lv_font_montserrat_20,0); lv_obj_align(fl,LV_ALIGN_TOP_LEFT,20,20);
  lv_obj_t *sf=lv_slider_create(tDisp); lv_obj_set_size(sf,480,20);
  lv_slider_set_range(sf,2,100); lv_slider_set_value(sf,brightFloor,LV_ANIM_OFF);
  lv_obj_align(sf,LV_ALIGN_TOP_LEFT,20,60);
  lv_obj_add_event_cb(sf,evFloor,LV_EVENT_VALUE_CHANGED,NULL);
  lblFloorVal=lv_label_create(tDisp); lv_label_set_text_fmt(lblFloorVal,"%d%%",brightFloor);
  lv_obj_set_style_text_font(lblFloorVal,&lv_font_montserrat_20,0); lv_obj_align(lblFloorVal,LV_ALIGN_TOP_LEFT,520,55);

  lv_obj_t *nl=lv_label_create(tDisp); lv_label_set_text(nl,"Night backlight (screensaver)");
  lv_obj_set_style_text_font(nl,&lv_font_montserrat_20,0); lv_obj_align(nl,LV_ALIGN_TOP_LEFT,20,110);
  lv_obj_t *sn=lv_slider_create(tDisp); lv_obj_set_size(sn,480,20);
  lv_slider_set_range(sn,1,60); lv_slider_set_value(sn,saverFloor,LV_ANIM_OFF);
  lv_obj_align(sn,LV_ALIGN_TOP_LEFT,20,150);
  lv_obj_add_event_cb(sn,evNightFloor,LV_EVENT_VALUE_CHANGED,NULL);
  lblNightVal=lv_label_create(tDisp); lv_label_set_text_fmt(lblNightVal,"%d%%",saverFloor);
  lv_obj_set_style_text_font(lblNightVal,&lv_font_montserrat_20,0); lv_obj_align(lblNightVal,LV_ALIGN_TOP_LEFT,520,145);

  lv_obj_t *tl=lv_label_create(tDisp); lv_label_set_text(tl,"Screensaver after");
  lv_obj_set_style_text_font(tl,&lv_font_montserrat_20,0); lv_obj_align(tl,LV_ALIGN_TOP_LEFT,20,220);
  ddSaver=lv_dropdown_create(tDisp); lv_dropdown_set_options(ddSaver,"30 sec\n1 min\n5 min\n10 min");
  lv_obj_set_width(ddSaver,200); lv_obj_align(ddSaver,LV_ALIGN_TOP_LEFT,20,255);
  { int sel=2; if(saverTimeoutMs==30000UL)sel=0; else if(saverTimeoutMs==60000UL)sel=1; else if(saverTimeoutMs==600000UL)sel=3; lv_dropdown_set_selected(ddSaver,sel);}
  lv_obj_add_event_cb(ddSaver,evSaverTimeout,LV_EVENT_VALUE_CHANGED,NULL);

  // ---- Massage ----
  lv_obj_t *m1=lv_label_create(tMass); lv_label_set_text(m1,"Small motor threshold");
  lv_obj_set_style_text_font(m1,&lv_font_montserrat_20,0); lv_obj_align(m1,LV_ALIGN_TOP_LEFT,20,15);
  sldSmall=lv_slider_create(tMass); lv_obj_set_size(sldSmall,430,18);
  lv_slider_set_range(sldSmall,30,140); lv_slider_set_value(sldSmall,setMinSmall,LV_ANIM_OFF);
  lv_obj_align(sldSmall,LV_ALIGN_TOP_LEFT,20,50); lv_obj_add_event_cb(sldSmall,evSmall,LV_EVENT_VALUE_CHANGED,NULL);
  lblSmall=lv_label_create(tMass); lv_label_set_text_fmt(lblSmall,"%d",setMinSmall);
  lv_obj_set_style_text_font(lblSmall,&lv_font_montserrat_20,0); lv_obj_align(lblSmall,LV_ALIGN_TOP_LEFT,470,46);
  lv_obj_t *m2=lv_label_create(tMass); lv_label_set_text(m2,"Big motor threshold");
  lv_obj_set_style_text_font(m2,&lv_font_montserrat_20,0); lv_obj_align(m2,LV_ALIGN_TOP_LEFT,20,95);
  sldBig=lv_slider_create(tMass); lv_obj_set_size(sldBig,430,18);
  lv_slider_set_range(sldBig,100,230); lv_slider_set_value(sldBig,setMinBig,LV_ANIM_OFF);
  lv_obj_align(sldBig,LV_ALIGN_TOP_LEFT,20,130); lv_obj_add_event_cb(sldBig,evBig,LV_EVENT_VALUE_CHANGED,NULL);
  lblBig=lv_label_create(tMass); lv_label_set_text_fmt(lblBig,"%d",setMinBig);
  lv_obj_set_style_text_font(lblBig,&lv_font_montserrat_20,0); lv_obj_align(lblBig,LV_ALIGN_TOP_LEFT,470,126);
  lv_obj_t *m3=lv_label_create(tMass); lv_label_set_text(m3,"Random character");
  lv_obj_set_style_text_font(m3,&lv_font_montserrat_20,0); lv_obj_align(m3,LV_ALIGN_TOP_LEFT,20,180);
  ddRandom=lv_dropdown_create(tMass); lv_dropdown_set_options(ddRandom,"Gentle\nLively\nWild");
  lv_obj_set_width(ddRandom,200); lv_obj_align(ddRandom,LV_ALIGN_TOP_LEFT,20,215);
  lv_dropdown_set_selected(ddRandom,randomChar); lv_obj_add_event_cb(ddRandom,evRandom,LV_EVENT_VALUE_CHANGED,NULL);
  lv_obj_t *mn=lv_label_create(tMass); lv_label_set_text(mn,"(applies to bed box when linked)");
  lv_obj_set_style_text_font(mn,&lv_font_montserrat_20,0);
  lv_obj_set_style_text_color(mn,lv_color_hex(SET_DIM_COL),0); lv_obj_align(mn,LV_ALIGN_TOP_LEFT,240,220);

  // ---- Files: dream recordings, split Shemi / Ira by filename prefix ----
  fileList = lv_list_create(tFiles);
  lv_obj_set_size(fileList, 760, 330);
  lv_obj_align(fileList, LV_ALIGN_TOP_MID, 0, 0);
  lv_list_add_text(fileList, "Shemi's dreams");
  lv_list_add_text(fileList, "Ira's dreams");
  lv_obj_t *note = lv_label_create(tFiles);
  lv_label_set_text(note, "recordings appear here once the listener node is built");
  lv_obj_set_style_text_font(note, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(note, lv_color_hex(SET_DIM_COL), 0);
  lv_obj_align(note, LV_ALIGN_BOTTOM_MID, 0, -8);

  // ---- About ----
  lblAbout=lv_label_create(tAbout);
  lv_obj_set_style_text_font(lblAbout,&lv_font_montserrat_20,0);
  lv_obj_set_style_text_color(lblAbout,lv_color_hex(SET_TEXT_COL),0);
  lv_obj_align(lblAbout,LV_ALIGN_TOP_LEFT,30,20);
  refreshAbout();
}

void refreshFanTile() {
  if (!fanImg) return;
  // TEST 042: two-photo toggle, made from the real fan photo -
  // off = light dark, on = centre light glowing. Same as the AC tile.
  lv_img_set_src(fanImg, fanPower ? &img_fantile_on : &img_fantile_off);
}

void refreshFan() {
  if (fanLblPower) {
    lv_label_set_text(fanLblPower, fanPower ? "ON" : "OFF");
    lv_obj_set_style_text_color(fanLblPower,
      lv_color_hex(fanPower ? 0x40E080 : 0x808890), 0);
  }
  for (int i = 0; i < FAN_SPEEDS; i++) if (fanSpeedBtn[i])
    lv_obj_set_style_bg_color(fanSpeedBtn[i],
      lv_color_hex(((i + 1) == fanSpeed && fanPower) ? 0x2080FF : 0x2A3346), 0);
  for (int i = 0; i < 4; i++) if (fanTimerBtn[i])
    lv_obj_set_style_bg_color(fanTimerBtn[i],
      lv_color_hex(i == fanTimer ? 0x2080FF : 0x2A3346), 0);
  if (fanLightBtn)
    lv_obj_set_style_bg_color(fanLightBtn,
      lv_color_hex(fanLight ? 0xB0842E : 0x2A3346), 0);
}

static void evFanPower(lv_event_t *e) {
  fanPower = !fanPower;
  sendMsg(NODE_AUDIO, CMD_FAN_POWER, 0, fanPower ? 1 : 0);
  refreshFanTile();
  refreshFan();
}
static void evFanSpeed(lv_event_t *e) {
  fanSpeed = (int)(intptr_t)lv_event_get_user_data(e);   // 1..FAN_SPEEDS
  if (!fanPower) {
    fanPower = true;
    sendMsg(NODE_AUDIO, CMD_FAN_POWER, 0, 1);
    refreshFanTile();
  }
  sendMsg(NODE_AUDIO, CMD_FAN_SPEED, 0, (uint8_t)fanSpeed);
  refreshFan();
}
static void evFanTimer(lv_event_t *e) {
  fanTimer = (int)(intptr_t)lv_event_get_user_data(e);   // 0..3
  sendMsg(NODE_AUDIO, CMD_FAN_TIMER, 0, (uint8_t)fanTimer);
  refreshFan();
}
static void evFanLight(lv_event_t *e) {
  fanLight = !fanLight;
  sendMsg(NODE_AUDIO, CMD_FAN_LIGHT, 0, fanLight ? 1 : 0);
  refreshFan();
}

// fan tile: short press toggles, 3 s hold opens the screen
static void evTileFanPressed(lv_event_t *e) {
  fanPressMs = millis();
  fanLongFired = false;
}
static void evTileFanLong(lv_event_t *e) {
  fanLongFired = true;
  if (!fanPower) {
    fanPower = true;
    sendMsg(NODE_AUDIO, CMD_FAN_POWER, 0, 1);
    refreshFanTile();
  }
  refreshFan();
  lv_scr_load(scrFan);
}
static void evTileFanReleased(lv_event_t *e) {
  if (fanLongFired) return;
  fanPower = !fanPower;
  sendMsg(NODE_AUDIO, CMD_FAN_POWER, 0, fanPower ? 1 : 0);
  refreshFanTile();
  refreshFan();
}

void buildFan() {
  scrFan = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(scrFan, lv_color_hex(0x101418), 0);
  lv_obj_clear_flag(scrFan, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t *back = lv_btn_create(scrFan);
  lv_obj_set_size(back, 90, 50);
  lv_obj_set_pos(back, 10, 10);
  lv_obj_set_style_bg_color(back, lv_color_hex(0x2A3346), 0);
  lv_obj_add_event_cb(back, evBackHome, LV_EVENT_CLICKED, NULL);
  lv_obj_t *bl = lv_label_create(back);
  lv_label_set_text(bl, LV_SYMBOL_LEFT);
  lv_obj_set_style_text_font(bl, &lv_font_montserrat_28, 0);
  lv_obj_center(bl);

  lv_obj_t *ttl = lv_label_create(scrFan);
  lv_label_set_text(ttl, "Fan");
  lv_obj_set_style_text_font(ttl, &lv_font_montserrat_28, 0);
  lv_obj_set_style_text_color(ttl, lv_color_white(), 0);
  lv_obj_align(ttl, LV_ALIGN_TOP_MID, 0, 22);

  fanPowerBtn = lv_btn_create(scrFan);
  lv_obj_set_size(fanPowerBtn, 130, 50);
  lv_obj_align(fanPowerBtn, LV_ALIGN_TOP_RIGHT, -14, 10);
  lv_obj_set_style_bg_color(fanPowerBtn, lv_color_hex(0x303840), 0);
  lv_obj_add_event_cb(fanPowerBtn, evFanPower, LV_EVENT_CLICKED, NULL);
  fanLblPower = lv_label_create(fanPowerBtn);
  lv_label_set_text(fanLblPower, "OFF");
  lv_obj_set_style_text_font(fanLblPower, &lv_font_montserrat_28, 0);
  lv_obj_center(fanLblPower);

  // speed - the main control, big buttons 1..FAN_SPEEDS
  lv_obj_t *sl = lv_label_create(scrFan);
  lv_label_set_text(sl, "Speed");
  lv_obj_set_style_text_font(sl, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(sl, lv_color_hex(0x7B90A0), 0);
  lv_obj_set_pos(sl, 24, 96);
  for (int i = 0; i < FAN_SPEEDS; i++) {
    char n[4]; snprintf(n, sizeof(n), "%d", i + 1);
    fanSpeedBtn[i] = mkBtn(scrFan, 20 + i * 128, 126, 116, 86,
                           n, evFanSpeed, i + 1, &lv_font_montserrat_40);
  }

  // off-timer, same idea as the AC
  lv_obj_t *tl = lv_label_create(scrFan);
  lv_label_set_text(tl, "Timer");
  lv_obj_set_style_text_font(tl, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(tl, lv_color_hex(0x7B90A0), 0);
  lv_obj_set_pos(tl, 24, 286);
  for (int i = 0; i < 4; i++)
    fanTimerBtn[i] = mkBtn(scrFan, 150 + i * 130, 272, 120, 58,
                           FAN_TIMER_NAME[i], evFanTimer, i,
                           &lv_font_montserrat_20);

  // light within the fan
  lv_obj_t *ll = lv_label_create(scrFan);
  lv_label_set_text(ll, "Light");
  lv_obj_set_style_text_font(ll, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(ll, lv_color_hex(0x7B90A0), 0);
  lv_obj_set_pos(ll, 24, 360);
  fanLightBtn = mkBtn(scrFan, 150, 350, 140, 58, "Light",
                      evFanLight, 0, &lv_font_montserrat_20);

  lv_obj_t *note = lv_label_create(scrFan);
  lv_label_set_text(note,
    "shows what was last commanded - the fan does not report back");
  lv_obj_set_style_text_font(note, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(note, lv_color_hex(0x55606E), 0);
  lv_obj_align(note, LV_ALIGN_BOTTOM_MID, 0, -14);

  refreshFan();
}

void buildHome() {
  scrHome = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(scrHome, lv_color_hex(0x0B0E14), 0);

  // TEST 076: top header - BG + TEST number (left), date + time (centre),
  // temperature (right). The clock is a placeholder until the time comes
  // over the bus. There is room above the tiles (TILE_Y0 = 74).
  // TEST 077: header rearranged.
  //   LEFT   - "BG" + bedroom in Hebrew (stored pre-reversed: no bidi)
  //   CENTRE - TEST number
  //   RIGHT  - temperature (top) and date + time (below)
  lblHomeTitle = lv_label_create(scrHome);
  lv_label_set_text(lblHomeTitle, "BG  \u05d4\u05e0\u05d9\u05e9 \u05e8\u05d3\u05d7");  // "BG  חדר שינה" pre-reversed
  lv_obj_set_style_text_font(lblHomeTitle, &font_hebrew_28, 0);
  lv_obj_set_style_text_color(lblHomeTitle, lv_color_hex(0xE8EAF0), 0);
  lv_obj_align(lblHomeTitle, LV_ALIGN_TOP_LEFT, 20, 20);

  lv_obj_t *lblHomeTest = lv_label_create(scrHome);
  lv_label_set_text_fmt(lblHomeTest, "TEST %03d", TEST_NUMBER);
  lv_obj_set_style_text_font(lblHomeTest, &lv_font_montserrat_24, 0);
  lv_obj_set_style_text_color(lblHomeTest, lv_color_hex(0xB8C0CC), 0);
  lv_obj_align(lblHomeTest, LV_ALIGN_TOP_MID, 0, 24);

  lblHomeTemp = lv_label_create(scrHome);
  lv_label_set_text(lblHomeTemp, "--\u00b0");
  lv_obj_set_style_text_font(lblHomeTemp, &lv_font_montserrat_28, 0);
  lv_obj_set_style_text_color(lblHomeTemp, lv_color_hex(0x8CC8FF), 0);
  lv_obj_align(lblHomeTemp, LV_ALIGN_TOP_RIGHT, -20, 14);

  lblHomeClock = lv_label_create(scrHome);
  lv_label_set_text(lblHomeClock, "--:--   --/--");
  lv_obj_set_style_text_font(lblHomeClock, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(lblHomeClock, lv_color_hex(0x9AA6B2), 0);
  lv_obj_align(lblHomeClock, LV_ALIGN_TOP_RIGHT, -20, 46);

  // --- row 0: massage, radio, air conditioning, fan ---
  addImg(makeTile(scrHome, 0, 0, 0x181C22, "massage"),          &img_bed_off);
  addImg(makeTile(scrHome, 1, 0, 0x181C22, "radio"),            &img_radio_off);
  // TEST 032: AC tile - custom, so it can toggle between the two photos.
  lv_obj_t *tileAC = lv_btn_create(scrHome);
  lv_obj_set_size(tileAC, TILE_SZ, TILE_SZ);
  lv_obj_set_pos(tileAC, tileX(2), tileY(0));
  lv_obj_set_style_bg_color(tileAC, lv_color_hex(0x181C22), 0);
  lv_obj_set_style_radius(tileAC, 16, 0);
  lv_obj_set_style_pad_all(tileAC, 0, 0);
  lv_obj_set_style_shadow_width(tileAC, 0, 0);
  lv_obj_clear_flag(tileAC, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_event_cb(tileAC, evTileACPressed,  LV_EVENT_PRESSED,      NULL);
  lv_obj_add_event_cb(tileAC, evTileACLong,     LV_EVENT_LONG_PRESSED, NULL);
  lv_obj_add_event_cb(tileAC, evTileACReleased, LV_EVENT_RELEASED,     NULL);
  acImg = lv_img_create(tileAC);
  lv_img_set_src(acImg, &img_actile_off);
  lv_obj_center(acImg);
  // TEST 041: fan tile - short press toggles, 3 s hold opens the screen.
  lv_obj_t *tileFan = lv_btn_create(scrHome);
  lv_obj_set_size(tileFan, TILE_SZ, TILE_SZ);
  lv_obj_set_pos(tileFan, tileX(3), tileY(0));
  lv_obj_set_style_bg_color(tileFan, lv_color_hex(0x181C22), 0);
  lv_obj_set_style_radius(tileFan, 16, 0);
  lv_obj_set_style_pad_all(tileFan, 0, 0);
  lv_obj_set_style_shadow_width(tileFan, 0, 0);
  lv_obj_clear_flag(tileFan, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_event_cb(tileFan, evTileFanPressed,  LV_EVENT_PRESSED,      NULL);
  lv_obj_add_event_cb(tileFan, evTileFanLong,     LV_EVENT_LONG_PRESSED, NULL);
  lv_obj_add_event_cb(tileFan, evTileFanReleased, LV_EVENT_RELEASED,     NULL);
  fanImg = lv_img_create(tileFan);
  lv_img_set_src(fanImg, &img_fantile_off);
  lv_img_set_zoom(fanImg, TILE_ZOOM);
  lv_obj_clear_flag(fanImg, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_center(fanImg);
  refreshFanTile();

  // --- row 1: light, shades, settings ---
  // TEST 035: light tile - img_lamp recoloured grey (off) / amber (on).
  lv_obj_t *tileLight = lv_btn_create(scrHome);
  lv_obj_set_size(tileLight, TILE_SZ, TILE_SZ);
  lv_obj_set_pos(tileLight, tileX(0), tileY(1));
  lv_obj_set_style_bg_color(tileLight, lv_color_hex(0x14262C), 0);
  lv_obj_set_style_radius(tileLight, 16, 0);
  lv_obj_set_style_pad_all(tileLight, 0, 0);
  lv_obj_set_style_shadow_width(tileLight, 0, 0);
  lv_obj_clear_flag(tileLight, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_event_cb(tileLight, evLampToggle, LV_EVENT_CLICKED, NULL);
  lampImg = lv_img_create(tileLight);
  lv_img_set_src(lampImg, &img_lamp);
  lv_img_set_zoom(lampImg, TILE_ZOOM);
  lv_obj_center(lampImg);
  refreshLamp();
  // TEST 040: shade tile - flips open/closed on tap (copied behaviour).
  tileShade = lv_btn_create(scrHome);
  lv_obj_set_size(tileShade, TILE_SZ, TILE_SZ);
  lv_obj_set_pos(tileShade, tileX(1), tileY(1));
  lv_obj_set_style_bg_color(tileShade, lv_color_hex(0x181C22), 0);
  lv_obj_set_style_radius(tileShade, 16, 0);
  lv_obj_set_style_pad_all(tileShade, 0, 0);
  lv_obj_set_style_shadow_width(tileShade, 0, 0);
  lv_obj_clear_flag(tileShade, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_event_cb(tileShade, evTileShade, LV_EVENT_CLICKED, NULL);
  tileShadeImg = lv_img_create(tileShade);
  lv_img_set_src(tileShadeImg, &img_shade_open);
  lv_obj_set_style_img_recolor(tileShadeImg, lv_color_black(), 0);
  lv_obj_set_style_img_recolor_opa(tileShadeImg, TILE_DIM_OPA, 0);
  lv_img_set_zoom(tileShadeImg, TILE_ZOOM);
  lv_obj_clear_flag(tileShadeImg, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_center(tileShadeImg);

  lv_obj_t *tileSet = makeTile(scrHome, 2, 1, 0x232A33, "settings");
  lv_obj_t *si = lv_label_create(tileSet);
  lv_label_set_text(si, LV_SYMBOL_SETTINGS);
  lv_obj_set_style_text_font(si, &lv_font_montserrat_40, 0);
  lv_obj_set_style_text_color(si, lv_color_hex(0x8A94A0), 0);
  lv_obj_center(si);

  // TEST 029: the eighth slot at (3,1) now holds the Dreamsaver -
  // a dreamcatcher tile that opens the Dreams screen.
  lv_obj_t *tileDream = lv_btn_create(scrHome);
  lv_obj_set_size(tileDream, TILE_SZ, TILE_SZ);
  lv_obj_set_pos(tileDream, tileX(3), tileY(1));
  lv_obj_set_style_bg_color(tileDream, lv_color_hex(0x181C22), 0);
  lv_obj_set_style_radius(tileDream, 16, 0);
  lv_obj_set_style_pad_all(tileDream, 0, 0);
  lv_obj_set_style_shadow_width(tileDream, 0, 0);
  lv_obj_clear_flag(tileDream, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_event_cb(tileDream, evGoDreams, LV_EVENT_CLICKED, NULL);
  lv_obj_t *tileDreamImg = lv_img_create(tileDream);
  lv_img_set_src(tileDreamImg, &img_dream);
  lv_obj_center(tileDreamImg);
}

// ============================================================
//  The test page
// ============================================================
// TEST 026: back to the overlay. Steady, and it never fights the
// touch controller for the I2C bus.
static void evDim(lv_event_t *e) {
  setDim(lv_slider_get_value(lv_event_get_target(e)));
}

void buildTestPage() {
  if (!scrTest) scrTest = lv_obj_create(NULL);
  lv_obj_t *scr = scrTest;
  lv_obj_set_style_bg_color(scr, lv_color_hex(0x101820), 0);

  lv_obj_t *t = lv_label_create(scr);
  lv_label_set_text_fmt(t, "IRA PANEL  -  TEST %03d", TEST_NUMBER);
  lv_obj_set_style_text_font(t, &lv_font_montserrat_28, 0);
  lv_obj_set_style_text_color(t, lv_color_hex(0xE8F0FF), 0);
  lv_obj_align(t, LV_ALIGN_TOP_MID, 0, 18);

  char s[220];
  snprintf(s, sizeof(s),
           "CH422G expander : %s\n"
           "GT911 touch     : %s\n"
           "SHT31 temp/hum  : %s\n"
           "BH1750 light    : %s\n\n"
           "I2C found: %s",
           haveCH422  ? "ok" : "-- MISSING --",
           haveTouch  ? "ok" : "-- MISSING --",
           haveSHT31  ? "ok" : "not fitted yet",
           haveBH1750 ? "ok" : "not fitted yet (expect 0x5C)",
           i2cLine);

  lv_obj_t *l = lv_label_create(scr);
  lv_label_set_text(l, s);
  lv_obj_set_style_text_font(l, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(l, lv_color_hex(0xB8C4D8), 0);
  lv_obj_align(l, LV_ALIGN_TOP_LEFT, 40, 80);

  lblTouch = lv_label_create(scr);
  lv_label_set_text(lblTouch, "touch  -  press the screen");
  lv_obj_set_style_text_font(lblTouch, &lv_font_montserrat_24, 0);
  lv_obj_set_style_text_color(lblTouch, lv_color_hex(0x66D9A0), 0);
  lv_obj_align(lblTouch, LV_ALIGN_TOP_LEFT, 40, 262);

  lv_obj_t *dl = lv_label_create(scr);
  lv_label_set_text(dl, "dim  -  black overlay. True dimming needs a wire to MP3302 EN.");
  lv_obj_set_style_text_font(dl, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(dl, lv_color_hex(0xB8C4D8), 0);
  lv_obj_align(dl, LV_ALIGN_TOP_LEFT, 40, 318);

  lv_obj_t *sl = lv_slider_create(scr);
  lv_obj_set_size(sl, 640, 26);
  lv_slider_set_range(sl, 0, 90);
  lv_slider_set_value(sl, dimPct, LV_ANIM_OFF);
  lv_obj_align(sl, LV_ALIGN_TOP_LEFT, 40, 356);
  lv_obj_add_event_cb(sl, evDim, LV_EVENT_VALUE_CHANGED, NULL);

  // TEST 028: no button back to home, because there is no longer a
  // button to get here. The diagnostics page is still built and still
  // updated; it is simply not on the way to anywhere. Everything it
  // shows also arrives on serial every twenty seconds.
}

// ============================================================
//  setup
// ============================================================
void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println();
  Serial.printf("=== IRA PANEL - TEST %03d - Waveshare ESP32-S3-Touch-LCD-4.3 ===\n",
                TEST_NUMBER);
  // TEST 069: why did we (re)boot? This is the key fact for the "reboots
  // every ~3 touches" symptom. brownout = power sag; a panic = a code
  // crash; a watchdog = the loop was stuck too long.
  {
    esp_reset_reason_t rr = esp_reset_reason();
    const char *name = "?";
    switch (rr) {
      case ESP_RST_POWERON:   name = "POWERON (fresh power / RST button)"; break;
      case ESP_RST_SW:        name = "SW (software restart)";              break;
      case ESP_RST_PANIC:     name = "PANIC (code crash)";                 break;
      case ESP_RST_INT_WDT:   name = "INT_WDT (interrupt watchdog)";       break;
      case ESP_RST_TASK_WDT:  name = "TASK_WDT (task watchdog)";           break;
      case ESP_RST_WDT:       name = "WDT (other watchdog)";               break;
      case ESP_RST_BROWNOUT:  name = "BROWNOUT (voltage sag - power!)";    break;
      case ESP_RST_DEEPSLEEP: name = "DEEPSLEEP";                          break;
      default:                name = "other";                             break;
    }
    Serial.printf(">>> RESET REASON: %s  (code %d)\n", name, (int)rr);
  }
  Serial.printf("PSRAM: %u bytes free\n", (unsigned)ESP.getFreePsram());
  if (ESP.getFreePsram() < 1000000)
    Serial.println("PSRAM LOOKS WRONG - check board_build.arduino.memory_type = opi_opi");

  step("Wire.begin");
  i2cMutex = xSemaphoreCreateMutex();
  Wire.begin(I2C_SDA, I2C_SCL, I2C_HZ);
  // TEST 009: the default 128-byte buffer is too small for a GT911
  // config table and it truncates without complaining.
  Wire.setBufferSize(512);

  step("CH422G expander");
  if (!ch422Begin())
    Serial.println("CH422G did NOT answer - backlight and resets will not work");

  i2cScan();

  step("gfx->begin  (RGB bus starts here)");
  if (!gfx->begin()) Serial.println("gfx->begin FAILED");
  gfx->fillScreen(BLACK);

  // Screen is alive and black. Only now is it polite to switch the lamps on.
  step("backlight on");
  backlight(true);

  step("boot photo");
  if (showBootPhoto()) {
    delay(3500);
  } else {
    // TEST 002: never leave a black screen with no explanation.
    gfx->fillScreen(BLACK);
    gfx->setTextColor(WHITE);
    gfx->setTextSize(3);
    gfx->setCursor(40, 200);
    gfx->print("boot photo failed to decode");
    delay(2500);
  }
  gfx->fillScreen(BLACK);

  // TEST 012: the touch controller is reset and probed HERE, with the
  // RGB bus already running and settled. Before this line it was being
  // set up into the teeth of the panel starting.
  // ---- LVGL ----
  step("lv_init");
  lv_init();

  // Two buffers of 40 lines each, out of PSRAM. Whole-frame buffers
  // would be 768 KB and this panel does not need them.
  step("allocate the LVGL draw buffer");
  size_t px = SCREEN_W * 40;
  buf1 = (lv_color_t *)heap_caps_malloc(px * sizeof(lv_color_t) * 2,
                                        MALLOC_CAP_SPIRAM);
  if (!buf1) {
    Serial.println("no PSRAM for the LVGL buffer - falling back to internal RAM");
    buf1 = (lv_color_t *)malloc(px * sizeof(lv_color_t) * 2);
  }
  lv_disp_draw_buf_init(&drawBuf, buf1, buf1 + px, px);

  step("register the display driver");
  static lv_disp_drv_t dd;
  lv_disp_drv_init(&dd);
  dd.hor_res  = SCREEN_W;
  dd.ver_res  = SCREEN_H;
  dd.flush_cb = lvFlush;
  dd.draw_buf = &drawBuf;
  lv_disp_drv_register(&dd);

  step("register the input driver");
  static lv_indev_drv_t id;
  lv_indev_drv_init(&id);
  id.type    = LV_INDEV_TYPE_POINTER;
  id.read_cb = lvTouch;
  lv_indev_drv_register(&id);

  // TEST 016: the page is built BEFORE touch is initialised, so the
  // screen is readable no matter what the touch driver does.
  step("build the pages");
  buildTestPage();
  buildSoon();
  buildHome();
  buildDreams();
  buildSaver();
  buildAC();
  buildFan();
  buildRadio();          // TEST 049
  buildMassage();        // TEST 050
  buildSettings();       // TEST 051
  // TEST 062: empty black screen, loaded before the screensaver to wipe
  // the framebuffer clean - same as panel_shemi's scrBlank. Needed
  // because the settings tabview leaves residue behind otherwise.
  scrBlank = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(scrBlank, lv_color_black(), 0);
  lv_scr_load(scrHome);
  lv_timer_handler();

  // ---- touch, last, because it is the only thing that can hang ----
  step("release TP_RST on the CH422G");
  touchResetPulse();

  step("bb_captouch init  <-- if the board stops here, this is why");
  touchBegin();

  step("rescan I2C");
  i2cScan();

  step("refresh the diagnostics page with what was found");
  lv_obj_clean(scrTest);
  lblTouch = NULL;
  buildTestPage();

  bootReport();
}

// ============================================================
//  TEST 002: the whole boot story, printed on demand.
//  monitor_rts/dtr are 0, so opening the monitor does not reset the
//  board and everything setup() printed is long gone. This is called
//  once at the end of setup() and again every 20 seconds, so there is
//  always a fresh copy arriving no matter when the monitor is opened.
// ============================================================
void bootReport() {
  Serial.println();
  Serial.println("------------------------------------------------");
  Serial.printf("IRA PANEL  -  TEST %03d  -  panel id %d\n", TEST_NUMBER, PANEL_ID);
  Serial.printf("PSRAM free      : %u bytes\n", (unsigned)ESP.getFreePsram());
  Serial.printf("Heap free       : %u bytes\n", (unsigned)ESP.getFreeHeap());
  Serial.printf("CH422G expander : %s\n", haveCH422  ? "ok" : "MISSING");
  Serial.printf("GT911 touch     : %s\n", haveTouch  ? "ok" : "MISSING");
  Serial.printf("SHT31 0x44      : %s\n", haveSHT31  ? "ok" : "not fitted");
  Serial.printf("BH1750 0x5C     : %s\n", haveBH1750 ? "ok" : "not fitted");
  Serial.printf("I2C devices     : %s\n", i2cLine);
  Serial.printf("dim overlay     : %d %%   (backlight is on/off only)\n", dimPct);
  Serial.printf("touch           : addr 0x%02X  %s\n",
                gtAddr, touchReady ? "ready" : "NOT READY");
  Serial.printf("touch config    : %d x %d, %d points%s\n",
                cfgW, cfgH, cfgPoints,
                (cfgPoints >= 1 && cfgPoints <= 5) ? "  (legal)" : "  <-- ILLEGAL");
  Serial.println("Touch the screen - the coordinates should follow your finger.");
  Serial.println("If points stays 0 while ready stays 1, the touch FLEX is the suspect,");
  Serial.println("not the code: the chip is scanning and finding no sensor.");
  Serial.println("------------------------------------------------");
}

// ============================================================
//  loop
// ============================================================
void loop() {
  // TEST 074: only let LVGL draw when the raw-gfx saver is NOT up. While
  // the saver is showing we paint it ourselves with gfx-> and LVGL must
  // stay quiet, or a periodic redraw (the About tab) paints over it.
  if (!saverActive) lv_timer_handler();
  delay(5);


  // TEST 072: screensaver - raw gfx, no LVGL screen.
  if (!saverActive) {
    // not in the saver: enter it once the screen has been idle long enough.
    if (lv_disp_get_inactive_time(NULL) > SAVER_IDLE_MS) enterSaverNew();
  } else {
    // in the saver: read the touch ourselves. LVGL is not drawing now.
    int tx = 0, ty = 0, tn = 0;
    bool touched = touchGet(tx, ty, tn);
    if (touched && tx >= DREAM_X0) {
      // RIGHT third = the dreamcatcher. Hold to record a dream.
      if (!dreamRecording) {
        dreamRecording = true;
        dreamHoldStart = millis();
        gfx->fillCircle(SCREEN_W - 40, 40, 14, 0xE800);   // red REC dot
        Serial.println("[dream] record START (hold) -> would tell listener over RS-485");
      }
    } else if (touched) {
      // any other touch = wake up and go home.
      exitSaverNew();
    } else {
      // no touch this cycle. If we were recording, the finger lifted -> stop.
      if (dreamRecording) {
        dreamRecording = false;
        gfx->fillCircle(SCREEN_W - 40, 40, 14, 0x0000);   // clear the REC dot
        Serial.println("[dream] record STOP (release) -> would save to microSD with timestamp");
      }
    }
    // keep the temperature fresh while the saver is up.
    saverPaintTemp(false);
  }
  // TEST 030: refresh the temperature (SHT31) every ~5 s.
  static unsigned long lastTempMs = 0;
  if (millis() - lastTempMs > 5000) {
    lastTempMs = millis();
    float t;
    if (sht31Read(t)) { gTemp = t; gTempOK = true; }
    // TEST 072: the saver temperature is painted with raw gfx in the
    // saver branch of the loop, not here.
    // TEST 053: only refresh About while Settings is the screen on show.
    // Restyling a hidden label every 5 s was invalidating LVGL and made
    // the whole panel flicker (and overlap the screensaver).
    if (lv_scr_act() == scrSettings) refreshAbout();
    // TEST 076: keep the home header's temperature live, only while home
    // is the screen on show (same guard rule as the others).
    if (lblHomeTemp && lv_scr_act() == scrHome) {
      if (gTempOK) lv_label_set_text_fmt(lblHomeTemp, "%d\u00b0", (int)lroundf(gTemp));
      else         lv_label_set_text(lblHomeTemp, "--\u00b0");
    }
  }

  // TEST 040: the shutter takes 20 s; clear the moving flag when up.
  if (shadeMoving != 0 && (millis() - shadeMoveStart) >= SHADE_TRAVEL_MS) {
    shadeMoving = 0;
    Serial.println("shade: 20 s elapsed, assuming it has arrived");
  }

  static unsigned long last = 0, lastReport = 0;
  if (millis() - last > 5000) {
    last = millis();
    Serial.printf("alive  heap %u  psram %u  dim %d%%\n",
                  (unsigned)ESP.getFreeHeap(),
                  (unsigned)ESP.getFreePsram(), dimPct);
  }
  // TEST 002: repeat the full report so it can never be missed again.
  if (millis() - lastReport > 20000) {
    lastReport = millis();
    bootReport();
  }

  // TEST 017: report real touches, and count the impossible ones.
  static unsigned long lastDump = 0;
  if (millis() < 60000 && millis() - lastDump > 500) {
    lastDump = millis();
    int x, y, n;
    if (touchGet(x, y, n))
      Serial.printf("touch: %d point(s), first at %d , %d\n", n, x, y);
  }

}

// ============================================================
//  IRA PANEL - TEST 078 - end of file
//  TEST 078: fix the 077 build - Fonts/FreeSansBold24pt7b.h is not in
//  this GFX install, so the temperature is back to the built-in font at
//  size 7 (always available). Everything else from 077 stays: header
//  layout, temp 60 px right, dreamcatcher 1.2x below the temperature.
//  TEST 077: home header rearranged (BG + Hebrew bedroom LEFT, TEST
//  number CENTRE, temperature + date/time RIGHT). Saver: rounded
//  FreeSansBold font for the temperature (was blocky), temp moved 60 px
//  (~7 mm) RIGHT, degree mark a clean ring, and the dreamcatcher is 1.2x
//  bigger and sits BELOW the temperature on the same right vertical.
//  TEST 076: two things. Saver: temperature and dreamcatcher moved 60 px
//  (~7 mm) left, temp font one size bigger, and the degree mark is now a
//  drawn ring (the old (char)247 glyph rendered as noise). Home: added a
//  top header - BG + TEST number (left), date + time placeholder (centre,
//  fills once the bus clock arrives), and live temperature (right).
//  TEST 075: prettier saver. Now that the raw-gfx saver is stable, draw
//  the REAL dial image (img_dial, 340x340) and the REAL dreamcatcher
//  image (img_dream, 175x175) with draw16bitRGBBitmap instead of plain
//  circles. Hands stay as clean lines (rotating the hand images would
//  need alpha+rotation in raw gfx). Same safe mechanism, nicer looks.
//  TEST 074: the About tab overlapped the new saver because LVGL kept
//  running (lv_timer_handler every loop) and About redraws every few
//  seconds, painting over the raw-gfx saver. Only About did this - it is
//  the only screen that refreshes on a timer. Fix: pause LVGL (skip
//  lv_timer_handler) while saverActive. The saver owns the screen alone.
//  TEST 073: fix the 072 build - saverActive needed a forward decl for
//  lvTouch, and the new dreamRecording clashed with the existing one
//  from the Dreams tile (reused it instead). No behaviour change.
//  TEST 072: SCREENSAVER REWRITTEN FROM SCRATCH. It is no longer an
//  LVGL screen - it is painted directly with gfx-> over a black screen,
//  and a plain flag (saverActive) tracks it. This removes BOTH long-
//  standing bugs at their root: the previous-screen overlap (there is no
//  second LVGL screen to bleed through) and the touch freeze (there is
//  no LVGL button to process). Touch is read directly in the loop: the
//  right third is the dreamcatcher (hold to record a dream, serial stub
//  until the listener node exists); any other touch wakes to home.
//  TEST 071: combine the two fixes that each worked alone. Clean ENTRY
//  via scrBlank + full repaint (062 made it 'come up nicely'), and the
//  direct-touch EXIT with LVGL blocked on the saver (070 stopped the
//  touch freeze). Entry fixes the settings-screen overlap; exit handles
//  the touch without LVGL's button path.
//  TEST 070: THE FIX. The repeating banner was NEVER a reboot - it is
//  bootReport() printing every 20 s on purpose (heap was steady, no
//  ESP-ROM line, no RESET REASON). The only real bug was the touch on
//  the saver: two readers of the GT911 competed and LVGL's full-screen
//  button press hung the panel. Now while the saver shows, lvTouch reads
//  nothing and reports RELEASED; the loop's direct-exit owns the touch,
//  loads home with a full repaint, and resets the idle timer.
//  TEST 069: DIAGNOSTIC. The panel reboots about every 3 touches.
//  Print esp_reset_reason() at boot so the NEXT reboot tells us why:
//  BROWNOUT (power sag) vs PANIC (code crash) vs WDT (stuck loop). This
//  is the fact we need before trying any fix.
//  TEST 068: THE FIX (from facts). The freeze is in LVGL processing
//  the press event of the saver's full-screen button - lvTouch read the
//  touch and flushed (284,295), but evSaverExit never ran. panel_shemi
//  never uses an LVGL button for this: it reads the touch directly in
//  the loop and calls exitSaver. Copied that: on a touch while the saver
//  is showing, read it here and load home, bypassing the button path.
//  TEST 067: fix build - forward-declare scrSaver before lvTouch's
//  T66 diagnostic print uses it. No behaviour change from 066.
//  TEST 066: DIAGNOSTIC. On touch the panel freezes with NOTHING on
//  serial (no crash, no reboot - a true hang). Print the touch coords
//  AND which screen is active AND the saver pointer, with flush(), the
//  moment a touch is read. The last line before the hang tells us the
//  exact coordinates and whether the saver is really the active screen.
//  TEST 065: DIAGNOSTIC. The freeze happens on TOUCH of the saver,
//  not on entry (064 entered clean, alive kept running). Added a serial
//  print at the start of each saver touch handler (evSaverExit,
//  evSaverRecPressed, evSaverRecReleased). The last line printed before
//  the freeze tells us exactly which handler is responsible.
//  TEST 064: DIAGNOSTIC. Removing the dim layer (063) did not stop
//  the freeze, so the dim is not the cause. Strip the saver entry to
//  the bare minimum - just lv_scr_load(scrSaver) - to find out whether
//  the freeze is in the entry code or in the saver screen itself. It
//  may overlap (that is expected here); we only care whether it freezes.
//  TEST 063: DIAGNOSTIC. The saver now comes up clean (062 fixed the
//  overlap) but you cannot exit it. Enter with setDim(0) instead of
//  SAVER_DIM to test whether the dim layer on lv_layer_top was blocking
//  the exit touch. If exit works now, that was the cause.
//  TEST 062: FACT - the overlap is ALWAYS with the About/settings
//  screen, never radio or massage. The difference: settings has an
//  lv_tabview, which leaves residue when the saver loads over it. Fix,
//  copied from panel_shemi's enterSaver: load an empty black scrBlank
//  and force a full repaint BEFORE loading the saver. This is the ONLY
//  change on top of the known-good TEST 061.
//  TEST 061: FACT-based. The radio (49) and massage (50) screenshots
//  showed a clean screensaver, so the original TEST 030 entry logic was
//  good. Reverted the screensaver back to it: setDim(SAVER_DIM) then
//  lv_scr_load(scrSaver), and setDim restored to just setting opacity.
//  Removed the TEST 054 additions that introduced the overlap.
//  TEST 060: reverted the guesswork of 056-059 (scrBlank, full_refresh,
//  60-line buffer, ghost-touch filter) back to the TEST 054 state. The
//  draw buffer is the original 40-line double buffer again. Kept from
//  054: setDim(0) deletes the dim layer, the saver is entered via home,
//  and the diagnostics label is written only on its own page. i2cLock
//  waits forever (portMAX_DELAY). From here we work only from the serial
//  monitor - no more guessing.
//  TEST 055: fix build - forward-declare scrTest before lvTouch
//  uses it (the scrTest guard added in TEST 054 came before its
//  definition). No behaviour change from 054.
//  TEST 054: chase the screensaver freeze. Four changes: setDim(0)
//  now DELETES the top-layer dim object (was left transparent and could
//  swallow touch); the saver is entered via the home screen so a live
//  tabview is torn down first; lblTouch is written only on the diag page;
//  and i2cLock uses a 100 ms timeout instead of waiting forever (a stuck
//  i2c holder would freeze the touch read and wedge the whole panel).
//  TEST 053: stop the flicker. About, the saver temperature and the
//  massage header were all being updated every cycle from every screen,
//  invalidating LVGL constantly. Each now updates only while its own
//  screen is the one on show (same rule panel_shemi already follows).
//  TEST 052: two fixes - the About tab temperature now refreshes
//  live (was stuck at 'no reading'); the screensaver clears the dim
//  layer before loading so it no longer overlaps the previous screen.
//  TEST 051: settings screen added (Display / Massage / Files /
//  About tabs; no Clock tab; Files split Shemi/Ira by name prefix,
//  empty until the listener node exists; values display-only).
//  TEST 050: massage screen added (12 Hebrew mode tiles, 4 zone
//  sliders, timer row; placeholder header time until the bus exists).
//  TEST 048: lowercase c; dial+hands dimmed; wider arrow + green dot.
//  TEST 047: screensaver temperature bigger (100 px) and 3 mm lower.
//  TEST 046: dream icon on the screensaver dimmed to ~45% (render time).
//  TEST 045: barbed arrow tips on the hour + minute hands (rotated images).
//  TEST 044: new dream icon (sleeping figure) in dream_img.h.
//  TEST 043: screensaver - clock 5 mm right, temp + dreamcatcher 5 mm left.
//  TEST 042: fan tile off/on photos from the real fan (like the AC).
//  TEST 041: fan tile (short=toggle, 3 s hold=screen) + fan control
//  screen (power, 6 speeds, off-timer, light).
//  TEST 040: shade tile flips open/closed (copied from Shemi); AC Light
//  command number fixed (17, was clashing with the off-timer at 16).
//  TEST 039: opening the AC screen (3 s hold) defaults the AC to ON and
//  refreshes the room temperature from the SHT31.
//  TEST 038: AC off-timer (Off / 1h / 2h / 3h).
//  TEST 037: AC screen copied verbatim from panel_shemi.
//  TEST 049: radio screen ported from panel_shemi (serial-stub sends,
//  command numbers 22-25 to avoid collisions; audio node not built).
//                        TEST  078
// ============================================================