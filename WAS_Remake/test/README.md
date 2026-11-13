# Tests

Programs that drive the remake's code without a window: scripted checks that write screenshots, and fuzzers (damaged files, under AddressSanitizer). They take the game folder (the player's original files) and an output or work folder.

Build one with `./build.sh NAME` (or `bash build.sh NAME`): it compiles `NAME.c` with the remake's sources, ASan and UBSan, leaving out `main.c` and any source the test `#include`s. SDL3 comes from `pkg-config`, or set `SDL3_DIR`. Run headless with `SDL_VIDEODRIVER=dummy`.

| Test | What it checks |
|---|---|
| `test_viewer GAME OUT` | the asset loader and the viewer (step 1): both colour sets, every picture and tile, Tab's three modes and the Hercules conversion, the font, the map sprites, the campaigns (titles, opponents, squadrons, ships, the Armada's ships on their right sides), the viewer's keys; writes every screen as BMPs |
| `compare_extract.py OUT` | the remake's pictures and tiles (from `test_viewer`'s BMPs) against `../../WAS_Graphics/extract_graphics.py`: two decoders written separately must agree pixel for pixel |
| `fuzz_assets GAME WORK [ROUNDS]` | damaged files (bytes changed, cut short, emptied) must load or be refused cleanly, and every viewer screen must draw |
| `test_play GAME OUT` | campaign play, step 2, scripted as it was done in the original in DOSBox: the start, the marker to a squadron, Info, Fleet, Time, Pause, Surrender, scrolling and the marker's limits; checks the text rows against the original's and writes the screens as BMPs (they matched the DOSBox screenshots pixel for pixel) |
| `test_rules GAME OUT` | campaign play, step 3, scripted as it was done in the original in DOSBox: tracing a path (the box, the land stopping it), the mark, sailing to the end; Join and Detach; sighting, an encounter and its fight; a crown taken ("You Won"); the opponent's offer to surrender; the wind; a frigate lost in rough water. Writes the screens as BMPs (they matched the DOSBox screenshots pixel for pixel, apart from the enemy's moves, which are step 5) |
| `fuzz_play GAME [FRAMES]` | random keys in every campaign: the marker and the view stay in their limits, every frame draws; then, with the clock at its fastest, paths traced from random squadrons: every sailing squadron's path still ends with 99, supplies stay 0-20 |
| `compare_shots.py DOSBOX.png REMAKE.bmp` | pixel differences between a DOSBox screenshot of the original and a remake screen (640x400 against 320x200, or two 720x348 Hercules screens) |
| `test_battle GAME OUT` | the sea battle and the boarding, step 4, scripted as it was done in the original in DOSBox: Practice (the overview and the close view), Zoom on an encounter (with the squadrons moved together), Time faster until a ship grapples, Zoom: the boarding, Attack, the fight to its end and the battle after it. Writes the screens as BMPs: the battle's first screens matched the DOSBox screenshots pixel for pixel, the boarding's deck too (the men stand where the random numbers put them) |
| `fuzz_battle GAME [ROUNDS]` | random keys in Zoomed battles (Zoom at every grapple, so most rounds board) and in Practice: every battle ends and draws, every boarding finishes |
| `test_enemy GAME OUT` | the enemy commander, step 5: every campaign left to the enemy at the fastest time (three seeds), the map every minute as a BMP and how each game ends; the enemy must move |
| `diff_ovl4/` | Python (unicorn): the remake's campaign-play and enemy routines against WAS.EXE's own code run in an emulator from the same state, byte for byte (since step 6 also `ports`, port_sources ovl4:C07C); and the instruction counts behind the timing (see its README) |
| `test_menu GAME OUT` | the menus, step 6, walked through as in the original in DOSBox: *presented by*, the title page, go to war (titles, story, rules with a rule changed, the opponents, the quote), make changes (the campaign menu, the Options Shoppe with the wind changed and saved, the opening), the battle report won and lost against each opponent, saving (the warning the second time), *( data disk )* and *Saved game*, and the demo. Writes each screen in CGA and 16 colours as BMPs (title, titles, story, rules, opponents, opening, the save pages and the report matched the DOSBox screenshots pixel for pixel) and checks the timings measured there |
| `test_saved GAME OUT [SS]` | saved games: an `SS` written by the original (in DOSBox) is read and played on (its map matched the original's pixel for pixel); the remake's own SS written, read back and written again, byte for byte, for every campaign; 3000 damaged files load without harm. `OUT/SS_remake` is one of the remake's for the original to load (it does) |
| `test_app GAME OUT` | the whole app (`main.c`) with SDL's offscreen driver and scripted key events: *presented by*, the title page, F12 to the viewer, Enter to play, the marker, Fleet, Time, Tab, Esc and Y to the title page, quit from it. Run with `SDL_VIDEODRIVER=offscreen` (and `ASAN_OPTIONS=detect_leaks=0`: the GL driver keeps a little memory) |
| `test_gamelog GAME LOG-FOLDER` | the game log changes nothing: every campaign played twice side by side with the same keys (paths, Info and Speed, Detach and Join, Time, Fleet, Pause, the questions, Zoom and Practice, the battle's and boarding's keys), one logged (normal, then verbose) and one not, compared byte for byte after each iteration and each slice of a battle (131 battles, 20 with a boarding); then the logs' headers and lines, and that the demo and logging off write nothing |
| `test_editor GAME OUT` | the editors, step 7, walked through as in the original in DOSBox: a new campaign's six steps (a port on the map, two squadrons, ships named, a frigate's sails torn, friend and foe, the fleet counts, Shift and + to the map's bottom, the enemy leader, a rule, the story, the title), the files it wrote (`OUT/new_S1` and `OUT/new_SITLES`: the story and `SITLES` matched the original's byte for byte), the campaign played from *( data disk )*, each scene of the change menu saved and checked in the file, and the campaign removed (its title gone, its file name to the end of the list). Writes each screen in CGA and 16 colours as BMPs (they matched the DOSBox screenshots pixel for pixel, but for the blinking frames and cursor and what was put on the map differently) |
| `fuzz_editor GAME WORK [KEYS] [SEED]` | random keys and typing at random moments through the campaign menu and the editors (campaigns made, changed, removed; the map maker, the ships editor, the scroll editor), every page drawn in both modes; then every `S` file `SITLES` names must be whole and load |
| `test_app_log GAME LOG-FOLDER` | `--log` as the app does it: a campaign opens a new dated log, Esc and Y close it with the reason, a second campaign gets its own file, the demo closes it and writes none (it includes `main.c`) |

Run them all:
```sh
./build.sh test_viewer && ./build.sh fuzz_assets && ./build.sh test_play && ./build.sh test_rules && ./build.sh fuzz_play && ./build.sh test_battle && ./build.sh fuzz_battle && ./build.sh test_enemy && ./build.sh test_menu && ./build.sh test_editor && ./build.sh fuzz_editor && ./build.sh test_saved && ./build.sh test_app && ./build.sh test_gamelog && ./build.sh test_app_log
mkdir -p /tmp/was_shots
SDL_VIDEODRIVER=dummy ./test_viewer ../../WAS_Original_Files_3.5 /tmp/was_shots
python3 compare_extract.py /tmp/was_shots
./fuzz_assets ../../WAS_Original_Files_3.5 /tmp/was_fuzz 2000
mkdir -p /tmp/was_play && ./test_play ../../WAS_Original_Files_3.5 /tmp/was_play
./test_rules ../../WAS_Original_Files_3.5 /tmp/was_play
./fuzz_play ../../WAS_Original_Files_3.5 30000
mkdir -p /tmp/was_b && ./test_battle ../../WAS_Original_Files_3.5 /tmp/was_b
./fuzz_battle ../../WAS_Original_Files_3.5 30
mkdir -p /tmp/was_e && ./test_enemy ../../WAS_Original_Files_3.5 /tmp/was_e
(cd diff_ovl4 && ./build.sh && python3 diff.py turn 0,1,2,3,4,5,6,7,8,9,10 1,2 10 && python3 diff.py ports 0,1,2,3,4,5,6,7,8,9,10 1,2 6)
mkdir -p /tmp/was_m && SDL_VIDEODRIVER=dummy ./test_menu ../../WAS_Original_Files_3.5 /tmp/was_m
mkdir -p /tmp/was_ed && SDL_VIDEODRIVER=dummy ./test_editor ../../WAS_Original_Files_3.5 /tmp/was_ed
./fuzz_editor ../../WAS_Original_Files_3.5 /tmp/was_fuzz_ed 100000
./test_saved ../../WAS_Original_Files_3.5 /tmp/was_m [an SS saved by the original]
SDL_VIDEODRIVER=offscreen ASAN_OPTIONS=detect_leaks=0 ./test_app ../../WAS_Original_Files_3.5 /tmp/was_play
mkdir -p /tmp/was_log && ASAN_OPTIONS=detect_leaks=0 ./test_gamelog ../../WAS_Original_Files_3.5 /tmp/was_log
mkdir -p /tmp/was_alog && SDL_VIDEODRIVER=dummy ASAN_OPTIONS=detect_leaks=0 ./test_app_log ../../WAS_Original_Files_3.5 /tmp/was_alog
```
