# BG Bed firmware

This repository contains the firmware source for six installed modules and several auxiliary experiments. Open each module's directory as its own PlatformIO project. The table below is a **source inventory**, not permission to flash every module.

| Module | Directory | Source baseline | Status on 2026-10-04 |
|---|---|---:|---|
| Audio Node | `audionode/` | TEST055 | Candidate paired with Listener TEST030; clear Dream playback reported, but waits for full-file transfer. |
| Listener | `listener/` | TEST030 | Candidate paired with Audio Node TEST055; microphones and SD stay installed here. |
| Shemi panel | `panel_shemi/` | TEST105 | Wait animation candidate; flashing and build status must be confirmed on the actual panel. |
| Ira panel | `panel_ira/` | TEST119 | Wait animation candidate; flashing and build status must be confirmed on the actual panel. |
| Shemi BedBox | `bedbox_shemi/` | TEST024 in earlier cloud tree | Its source says **not yet run on hardware**; identify the firmware actually installed before replacing it. |
| Ira BedBox | `bedbox_ira/` | TEST001 in earlier cloud tree | **Motor diagnostic, runs on boot. Do not flash as production firmware.** Obtain the deployed firmware. |

The first four projects were reconciled from the latest small update packages prepared for the BG Bed system. Their `TEST_NUMBER` values were corrected to match their source headers and terminal banners. Neither a local PlatformIO build nor a six-board hardware regression was available in the preparation workspace. Do not treat repository presence as hardware validation.

The panel code depends on the board description and graphics/Waveshare libraries in `panel_shemi/`. `panel_ira/platformio.ini` uses that library directory on PlatformIO Core 6.x instead of duplicating a large vendor tree. Keep `panel_shemi/lib/` until builds of both panels pass in a clean checkout. The upstream platform in the project files currently tracks a `stable` release URL; pin a tested platform release after reproducing all builds.

## Release discipline

1. Record the **installed test number** and backup from each module before any flash. The source inventory above may differ from what is installed.
2. Build and flash matching protocol pairs, especially Audio Node TEST055 and Listener TEST030. Verify both panels, then exercise radio, Dream record/play, RS485, alarm and each side's massage independently.
3. Keep Ira BedBox out of every bulk-flash target until its actual production firmware is recovered and tested. The same caution applies to Shemi BedBox TEST024.
4. Next work: implement buffered Dream streaming without moving microphones/SD, then Edge Impulse voice recognition on the Listener. These features are **planned**, not present in this baseline.

The parent of the repository's September 7 `main` commit was `f267bd438197469405629a1d09cf4d9c6c69ba74`, the historical “KNOWN GOOD” marker for earlier Shemi/Ira panels. Preserve it as a rollback reference. Repository cleanup removes logs/WAV and credential-like files from the current tracked tree, but old Git commits still contain them. Rotate any active credentials and decide separately whether a coordinated history cleanup is required.
