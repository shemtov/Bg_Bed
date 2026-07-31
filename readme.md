# BedSystem — clean start

Rev 4 architecture. One central panel, two bed boxes, one audio node, all wired.

---

## The one rule that fixes the confusion

**The code file is always `src/main.cpp`. Always. In every project.**

The test number lives *inside* the file, on the `#define TEST_NUMBER` line and in
the banner at the top and bottom. It is never in the filename.

That means:

- There is never a question of "which .cpp is the current one" — there is only one.
- You never get "multiple definition of setup()" from two files sitting in `src/`.
- To move to a new test, you **overwrite** `src/main.cpp`. You never add a file.

The old names — `panel_test040.cpp`, `bedbox_test0009.cpp` — are what caused the
mess. They stop here.

---

## Structure

```
BedSystem/
├── panel/                  ESP32-8048S043 touchscreen at the headboard
│   ├── platformio.ini
│   ├── include/lv_conf.h   <-- YOU MUST SUPPLY THIS
│   └── src/main.cpp        TEST 041  (ready)
│
├── bedbox/                 QuinLED-ESP32 + L298N under each bed
│   ├── platformio.ini      (your real TEST 0009 config)
│   └── src/main.cpp        <-- YOU MUST SUPPLY THIS
│
└── audionode/              WT32-ETH01 + PCM5102A, by the soundbar
    ├── platformio.ini      (ready)
    └── src/main.cpp        (not written yet — TEST 001)
```

Each of the three folders is a **separate PlatformIO project**. Open them one at a
time in VS Code with **File → Open Folder**, pointing at `panel`, `bedbox` or
`audionode` — never at `BedSystem` itself.

You can rename the `BedSystem` folder to anything you like. PlatformIO only cares
about the folder that directly contains `platformio.ini`.

---

## Three files you must bring from your PC

I cannot reconstruct these reliably, and guessing would waste your time:

**1. `panel/include/lv_conf.h`** — LVGL's configuration. Yours works; a guessed one
probably won't. Find it in your old panel project under `include/`. There is a
placeholder file next to it from the old Test 1 project — that one is for LVGL 8.4
and is only a reference, not a drop-in. Delete it once you've copied the real one.

**2. `panel/platformio.ini`** — I rebuilt one from the `#include` lines in your
code, and it should be close. But if you can find the file that actually built
TEST 040, **use that instead** and overwrite mine. Known-good beats deduced.

**3. `bedbox/src/main.cpp`** — your TEST 0009 bed box code. Never uploaded here.
Its `platformio.ini` is already in place and is the real one.

---

## First build

1. VS Code → File → Open Folder → `BedSystem/panel`
2. Copy your `lv_conf.h` into `include/`
3. Click ✓ in the bottom bar to build
4. Green SUCCESS, or paste the first red error

At boot the serial monitor should print:

```
=== PANEL — TEST 041 — LVGL UI + Settings ===
```

---

## Never lose the files again

Once this builds, do one of these — five minutes now saves an afternoon later:

- Add `panel/src/main.cpp`, `panel/platformio.ini` and the bed box equivalents to
  the **project knowledge** of this Claude project, so every future chat opens
  with the real code already loaded, or
- `git init` in `BedSystem`, push it to GitHub, and paste the repo URL into any
  new chat.

A `.gitignore` is included so the build folder never gets committed.
