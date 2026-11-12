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
| `test_app GAME OUT` | the whole app (`main.c`) with SDL's offscreen driver and scripted key events: the viewer, Enter to play, the marker, Fleet, Time, Tab, Esc and Y, quit. Run with `SDL_VIDEODRIVER=offscreen` (and `ASAN_OPTIONS=detect_leaks=0`: the GL driver keeps a little memory) |

Run them all:
```sh
./build.sh test_viewer && ./build.sh fuzz_assets && ./build.sh test_play && ./build.sh test_rules && ./build.sh fuzz_play && ./build.sh test_app
mkdir -p /tmp/was_shots
SDL_VIDEODRIVER=dummy ./test_viewer ../../WAS_Original_Files_3.5 /tmp/was_shots
python3 compare_extract.py /tmp/was_shots
./fuzz_assets ../../WAS_Original_Files_3.5 /tmp/was_fuzz 2000
mkdir -p /tmp/was_play && ./test_play ../../WAS_Original_Files_3.5 /tmp/was_play
./test_rules ../../WAS_Original_Files_3.5 /tmp/was_play
./fuzz_play ../../WAS_Original_Files_3.5 30000
SDL_VIDEODRIVER=offscreen ASAN_OPTIONS=detect_leaks=0 ./test_app ../../WAS_Original_Files_3.5 /tmp/was_play
```
