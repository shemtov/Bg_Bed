// ============================================================
//                        TEST  027
// ============================================================
//  IRA PANEL - Waveshare ESP32-S3-Touch-LCD-4.3
//  ESP32-S3-WROOM-1 N16R8, 800x480 RGB, GT911 touch, CH422G expander
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

#define TEST_NUMBER 27

#include <Arduino.h>
#include <Wire.h>
#include <Arduino_GFX_Library.h>
#include <lvgl.h>
#include <JPEGDEC.h>
#include <esp_timer.h>

#include "boot_photo.h"      // the same header Shemi's panel uses
#include "tile_img.h"        // TEST 024: bed, radio, AC, shades - from panel/
#include "lamp_img.h"        // TEST 025: the lamp, also from panel/
#include "fan_img.h"         // TEST 027: the ceiling fan, from its photograph

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
inline void i2cLock()   { if (i2cMutex) xSemaphoreTake(i2cMutex, portMAX_DELAY); }
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
  int x = 0, y = 0, n = 0;
  if (touchGet(x, y, n)) {
    data->state   = LV_INDEV_STATE_PR;
    data->point.x = x;
    data->point.y = y;
    if (lblTouch)
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
#define TILE_PX      180
#define TILE_GAP     22
#define TILE_COLS    3
#define TILE_ROWS    2
#define TILE_DIM_OPA 110      // how far the artwork is pulled toward black

lv_obj_t *scrHome = NULL, *scrTest = NULL;
static void evDim(lv_event_t *e);      // defined with the test page below
void buildTestPage();
void buildSoon();
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
  if (scrSoon && lblSoonWhat) {
    lv_label_set_text(lblSoonWhat, name);
    lv_scr_load(scrSoon);
  }
}

static void evGoTest(lv_event_t *e) { if (scrTest) lv_scr_load(scrTest); }
static void evGoHome(lv_event_t *e) { if (scrHome) lv_scr_load(scrHome); }

// Where a tile sits in the 3 x 2 grid.
static void tilePos(int col, int row, int &x0, int &y0) {
  int totalW = TILE_PX * TILE_COLS + TILE_GAP * (TILE_COLS - 1);
  int totalH = TILE_PX * TILE_ROWS + TILE_GAP * (TILE_ROWS - 1);
  x0 = (SCREEN_W - totalW) / 2 + col * (TILE_PX + TILE_GAP);
  y0 = (SCREEN_H - totalH) / 2 + row * (TILE_PX + TILE_GAP) - 14;
}

static void addTile(lv_obj_t *parent, const lv_img_dsc_t *src,
                    int col, int row, const char *name) {
  int x0, y0;
  tilePos(col, row, x0, y0);

  lv_obj_t *btn = lv_btn_create(parent);
  lv_obj_set_size(btn, TILE_PX, TILE_PX);
  lv_obj_set_pos(btn, x0, y0);
  lv_obj_set_style_bg_opa(btn, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(btn, 0, 0);
  lv_obj_set_style_shadow_width(btn, 0, 0);
  lv_obj_add_event_cb(btn, evTile, LV_EVENT_CLICKED, (void *)name);

  lv_obj_t *im = lv_img_create(btn);
  lv_img_set_src(im, src);
  lv_obj_center(im);
  // Same treatment as Shemi's panel: the artwork is untouched, LVGL
  // pulls it toward black at draw time so it is not a lamp at night.
  lv_obj_set_style_img_recolor(im, lv_color_black(), 0);
  lv_obj_set_style_img_recolor_opa(im, TILE_DIM_OPA, 0);
  lv_obj_clear_flag(im, LV_OBJ_FLAG_CLICKABLE);
}

void buildHome() {
  scrHome = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(scrHome, lv_color_hex(0x0B0E14), 0);

  // Top row:    massage   radio   air conditioning
  // Bottom row:  shades    light   fan
  addTile(scrHome, &img_bed_off,    0, 0, "massage");
  addTile(scrHome, &img_radio_off,  1, 0, "radio");
  addTile(scrHome, &img_ac_off,     2, 0, "air conditioning");
  addTile(scrHome, &img_shade_open, 0, 1, "shades");
  addTile(scrHome, &img_lamp,       1, 1, "light");
  addTile(scrHome, &img_fan,        2, 1, "fan");

  // brightness, along the bottom
  lv_obj_t *sl = lv_slider_create(scrHome);
  lv_obj_set_size(sl, 620, 18);
  lv_slider_set_range(sl, 0, 90);
  lv_slider_set_value(sl, dimPct, LV_ANIM_OFF);
  lv_obj_align(sl, LV_ALIGN_BOTTOM_MID, 0, -18);
  lv_obj_add_event_cb(sl, evDim, LV_EVENT_VALUE_CHANGED, NULL);

  // a small way back to the diagnostics page
  lv_obj_t *b = lv_btn_create(scrHome);
  lv_obj_set_size(b, 76, 42);
  lv_obj_align(b, LV_ALIGN_TOP_RIGHT, -12, 12);
  lv_obj_add_event_cb(b, evGoTest, LV_EVENT_CLICKED, NULL);
  lv_obj_t *bl2 = lv_label_create(b);
  lv_label_set_text(bl2, "info");
  lv_obj_center(bl2);
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

  lv_obj_t *hb = lv_btn_create(scr);
  lv_obj_set_size(hb, 76, 42);
  lv_obj_align(hb, LV_ALIGN_TOP_RIGHT, -12, 12);
  lv_obj_add_event_cb(hb, evGoHome, LV_EVENT_CLICKED, NULL);
  lv_obj_t *hl = lv_label_create(hb);
  lv_label_set_text(hl, "home");
  lv_obj_center(hl);
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
  lv_timer_handler();
  delay(5);


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
//  IRA PANEL - TEST 027 - end of file
//  Hardware layer only: RGB screen, GT911 touch, CH422G backlight,
//  I2C scan, boot photo, software dimmer.
//                        TEST  027
// ============================================================
