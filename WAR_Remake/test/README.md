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
| `test_sound`, `tempo` | sounds, written as WAV files |
| `fort_arrows`, `fort_trace`, `fort_import` | archers in a fort, compared with the original (`WAR_Analysis/tools/dosbox/dosfort.sh`) |
| `fontdump` | the game's 8x8 font for `WAR_Analysis/tools/dosbox/screentext.py` |
| `dbg` | a scratch debugging aid |
