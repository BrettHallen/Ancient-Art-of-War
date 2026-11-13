# Tests

Programs that drive the remake's code without a window: scripted walkthroughs that write screenshots, fuzzers (random keys or damaged files, under AddressSanitizer), and a few debugging aids. Most take the game folder (the player's original files) and sometimes an output folder and an empty data folder.

Build one with `./build.sh NAME` (or `bash build.sh NAME`): it compiles `NAME.c` with the remake's sources, ASan and UBSan, leaving out the sources the test `#include`s. SDL3 comes from `pkg-config`, or set `SDL3_DIR`. Run headless with `SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy`.

| Test | What it checks |
|---|---|
| `test_viewer`, `pics`, `vgapics`, `pl` | the asset viewer and every picture, CGA and VGA |
| `test_play`, `test_rules`, `t_mov`, `t_newgame`, `fuzz_play`, `fuzz_smart` | the campaign map and its rules |
| `test_battle`, `test_zoom`, `test_bgs`, `fuzz_battle` | battles on the map and the battle screen (every background) |
| `watch_ai`, `route_dbg`, `fuzz_ai` | the enemy commander |
| `test_menu`, `test_menu2`, `test_cmenu`, `test_blink`, `t_presented`, `t_tab`, `test_tune_once` | title pages, scrolls, menus, saving, the battle report |
| `test_groups` | Formation, Detach and Join |
| `test_demo`, `test_app_demo`, `dbg_demo`, `dbg_demo2`, `maps_demo` | the demo |
| `test_app` | the whole app, scripted (it includes `main.c`) |
| `test_datadisk`, `test_disks` | data disk folders |
| `test_editor`, `fuzz_editor` | the campaign editor |
| `test_formed`, `fuzz_formed` | the formation editor |
| `test_import`, `fuzz_import` | importing the original's saved games |
| `test_diskerr` | "Unable to use data disk." when a file can't be written |
| `test_tab_rows` | the help rows on the scroll pages after switching CGA/VGA and back (`test_tab_rows GAME DATA OUT`) |
| `test_herc` | Hercules (`WAR HERC`): the conversion to 720x348, Tab's three modes, every CGA screen using colours 0-3 only; writes the title page, scrolls, menus, map and a battle as Hercules screens to compare with DOSBox (`test_herc GAME DATA OUT [S-FILE]`; it includes `main.c`) |
| `test_gamelog` | the game log changes nothing: every campaign played twice side by side with the same keys (orders, Time, Zoom, the battle screen), one logged (normal, then verbose) and one not, compared byte for byte after each iteration and battle frame; then the logs' headers and lines, and that the demo and logging off write nothing (`test_gamelog GAME LOG-FOLDER`) |
| `test_app_log` | `--log` as the app does it: a campaign opens a new dated log, Esc, Y closes it with the reason, a second campaign gets its own file, the demo none (`test_app_log GAME LOG-FOLDER`; it includes `main.c`) |
| `test_sound`, `tempo` | sounds, written as WAV files |
| `fort_arrows`, `fort_trace`, `fort_import` | archers in a fort, compared with the original (`WAR_Analysis/tools/dosbox/dosfort.sh`) |
| `fontdump` | the game's 8x8 font for `WAR_Analysis/tools/dosbox/screentext.py` |
| `dbg` | a scratch debugging aid |
