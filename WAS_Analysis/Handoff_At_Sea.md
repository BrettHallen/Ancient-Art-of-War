# Handoff: from the WAR remake to The Ancient Art of War at Sea

Written Nov 12 2026 at the end of the WAR remake sessions, as the starting point for a new session (and project) that does the same for **The Ancient Art of War at Sea** (WAS, 1987). Copy this doc into the new project. `Remake_plan.md` and `WAR_file_reference.md` from the WAR project are worth copying in too: they record how each WAR format and system worked, and At Sea is by the same authors.

## The goal (as for WAR)
A modern version that runs on the user's Mac and plays like the original, with the original CGA and 16-colour graphics loaded at run time from the player's own copy of the game. Logic is ported from a disassembly of the original program and checked side by side against the original running in DOSBox.

## Where things are
- **Repo:** `~/Documents/GitHub/Ancient-Art-of-War` on the user's Mac (reached as `$HOME/mnt/Ancient-Art-of-War` from the device shell).
  - `WAS_Original_Files_3.5/`: the At Sea disk. WAS.EXE, WAST.EXE + WAST.DAT, overlays SO0–SO8, pictures SPE–SPQ, SM, S0–SA, SITLES/SDITLES, SCNF/SCNFT, AD, B.
  - `WAS_Documentation/`: the manual as TIFF scans (it needs OCR), the quick reference card and the hard-disk installation sheet (PDFs).
  - `WAR_*`: the land game. `WAR_Remake/` is the finished remake; `WAR_Analysis/` has `WAR.EXE.asm`, the disassembler and the README of findings.
  - `tools/`: the disassembly helpers and the DOSBox harness (see `tools/README.md`). `WAR_Remake/test/`: the remake's tests and `build.sh`.
- **The WAS names look like WAR's with an S in front.** WAR had WAR.EXE, WARET.EXE + WARET.DAT (the 16-colour version), overlays O0–O8, pictures P*, the map tiles M, the campaigns T*, TITLES/DTITLES and CNF. Check this before relying on it: it is a guess from the file names.

## How the WAR work went
1. Decode the formats and write a viewer (pictures, map tiles, sprites, fonts, CGA and the 16-colour set).
2. Disassemble the program, then name routines and DS variables as they are understood (`gen_asm.py`'s name table). Write findings into the analysis README.
3. Build the game in steps, each checked against DOSBox:
   1. the asset viewer
   2. the campaign map
   3. the strategic rules
   4. battles
   5. the enemy AI
   6. menus, options, saved games, the battle report and the demo
   7. the editors
   8. sound

   The user chose the order and the scope of each step.
4. After each step: tests (scripted walkthroughs with screenshots, fuzzers under ASan), the README and the plan doc updated, files copied to the Mac.

## Decisions and conventions to carry over (unless the user says otherwise)
- **Rewrite, not emulate.** C + SDL3, CMake, a macOS app bundle (it also builds on Linux and Windows). The original was compiled C, so it is ported function by function and each C function names the routine it ports (e.g. `/* ovl6:4E26 */`).
- **Keep the original's units and state** (screen-buffer addresses, line pairs, DS layouts), so the code can be checked against the asm. Play fields are named after their DS addresses.
- **No original artwork, campaigns or fonts in the remake's code.** Assets are read at run time; the app icon is drawn at build time from the player's files. (The original files are in the repo, but the remake must not depend on that.)
- **CGA and 16-colour modes** draw the same 320x200 screen and can be switched at any time (Tab). F2 changes the aspect ratio (4:3, pixels 1.2x taller, or square).
- **Speed:** the IBM PC/XT is the reference. WAR's main loop ran at about 20 Hz. The user judged timings by memory and by ear (the XT tempo for sounds); measure with DOSBox at 300 cycles.
- **Data disks are folders** in `~/Library/Application Support/<App>/`, with the original's file formats kept, so the original can read what the remake writes. Original saved games can be imported.
- **Deliberate differences** from the original are listed in the plan doc. Ask before adding one. When the user reports something odd, check the original first: the archers in a fort missing turned out to be the original's own behaviour.
- **Source files** start with the user's header and are **UTF-8 with a BOM** (UltraEdit needs it for the ø). For At Sea, the first line would presumably become `The Ancient Art of War at Sea (1987) - remake`; confirm with the user.
  ```
  /*
   * The Ancient Art of War (1984) - remake
   * Nov 2026 using Claude Opus 5.5
   * "Guided" by Brett Hallen
   * Original game by Dave Murry & Barry Murry
   * Published by Brøderbund Software Inc.
   *
   * file.c: what it does
   */
  ```
- **The user's preferences:**
  - Commit or push only when asked. Commit messages end with the attribution lines the session gives.
  - The user's email is private.
  - Short, plain progress replies. Questions only when a choice is really theirs.

## Tooling (in the repo now)
- `tools/disassembly/`: `fn`, `fc` (one routine on a line), `dec.py` (C-like pseudocode), `ovl`, `ds.py` (DS strings), `analyze.py`. They point at WAR's files. For WAS, `wardis.py`/`gen_asm.py` (in `WAR_Analysis/disassembler/`) need a WAS variant: the overlay layout, segment table and name table are WAR.EXE's. `dec.py`'s DS name table is WAR's too.
- `tools/dosbox/`: start DOSBox on a virtual display (`Xvfb :99`), send keys (`xdotool`), take screenshots, read the screen's text with the game's own font (`screentext.py` + `font8.txt`, made by `WAR_Remake/test/fontdump`), answer the manual question (`warword.py` decodes WARW), wait for text, load a saved game, record video with ffmpeg. For WAS: a new font dump, a decoder for At Sea's manual question (if it has one), and new `.conf` mounts.
- `WAR_Remake/test/build.sh NAME`: builds a test with ASan/UBSan against the remake's sources.

**Setting up a new cloud workspace:**
- `apt-get install dosbox xvfb xdotool imagemagick ffmpeg tesseract-ocr`
- `pip install pillow capstone --break-system-packages`
- SDL3 built from source with CMake (3.4.x) into a prefix (`/root/sdl3`), then `-DCMAKE_PREFIX_PATH`.

DOSBox working folder layout: see `tools/README.md` (`$DOS` with the game copies, a patched copy for the B: data disk, `data_b/`, the confs).

## Lessons that saved or cost time
- **Measure in DOSBox at 300 cycles** (about an XT) for anything timed. Most captures were at 1500 cycles, which is several times faster. DOSBox plays sounds about 2.6x faster than a real XT at any cycle setting (its delay calibration loop runs relatively fast).
- **Record video for timings** (ffmpeg x11grab at 30 fps). Screenshots every 0.4 s alias fast blinking and gave a wrong answer once.
- **Keys at 300 cycles** must be held about 0.6 s (`HOLD=0.6`). xdotool autorepeat makes held cursor movement fiddly: tap and check the screen text instead.
- **Read the screen with the game's own font**, not OCR: tesseract misread the manual question (5? as 37).
- **Patch a saved game to set up a test case** (e.g. put a squad in a fort beside an enemy, put the cursor on it). The same file can be loaded by the original and imported by the remake, so both run the same scenario.
- **Patching the EXE helps:** WAR.EXE at 336Fh `EB` lets the data disk be B:, and at 8132h `90 90` forces Tandy sound.
- **`dec.py` sometimes renders `x[i++] = v` as a pre-increment:** check the asm for array writes and string loops.
- **Copying files to the Mac:**
  - Stage the file under `/mnt/user-data/outputs/<new folder>/`, then use `device_commit_files` with `force: true`, then compare md5s.
  - Committing the same staged path twice once delivered the old content: use a fresh staging folder each time and always verify.
  - Exec bits don't survive: run `chmod +x` on the Mac, or call scripts through `bash`/`sh`.
  - Deleting on the Mac isn't permitted: move unwanted files to `_to_delete/` in the repo and tell the user.
- **Writing project docs:** `project_write` with a local file needs that file read in full (after the last edit) in the same session.

## Parts of the WAR remake that may be reusable
These may carry over if At Sea is built like WAR; check each against the WAS formats first:
- `gfx.c`: the 320x200 screen, palettes, scaling and aspect.
- `assets.c`: the RLE picture decoder, WARET.DAT records, 2bpp bitmaps, DS strings.
- `sound.c`: the PC speaker and the Tandy/PCjr SN76496 chip via an SDL3 audio callback, the delay unit and tune formats.
- `menu.c`: the patterns for overlay-1-style title pages, scrolls, the Options Shoppe, CNF and data-disk folders.
- `main.c`: the fixed-rate loop, keys as scan codes, Tab/F2.
- `icon.c`, `tools/make_icon.c`: the app icon.
- The editors' approach (`mapedit.c`, `sqedit.c`, `formed.c`) and the saved-game import (`savegame.c`).

Whether At Sea gets its own `WAS_Remake/` with copies, or shares code with `WAR_Remake/`, is the user's call. Copies are simpler; sharing avoids fixing bugs twice.

## Suggested first steps in the new session
1. Inventory the WAS files and compare them with WAR's formats (sizes, headers, RLE, overlay layout); OCR the manual.
2. Set up the workspace (above) and make a WAS variant of the disassembler; get WAS.EXE and WAST.EXE disassembled.
3. Write `WAS_file_reference.md` and a `WAS_Remake_plan.md` with the build order. Agree the scope and order with the user before coding.
4. Start with the asset viewer, as for WAR.
