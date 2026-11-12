# Tools

The helpers used to study WAR.EXE and to check the remake against the original, saved from the cloud workspace where they were written (Nov 2026). They ran on Linux; most will work on macOS with the same programs installed.

## disassembly/

Work on `../WAR.EXE.asm`, which `../disassembler/gen_asm.py` writes (its name table gives each routine and data address a name; add names there as routines are understood, then run it again).

| File | Use |
|---|---|
| `fn NAME` | one routine on a single line, by name or by address (`fn ovl6_4E26`, `fn rt_0859`), even after renaming |
| `fc NAME` | the same, shorter (no `word ptr`, comments) |
| `dec.py NAME` | C-like pseudocode with gotos for one routine, using the DS names in its table. It sometimes shows `x[i++] = v` as a pre-increment: check the asm for array writes and string loops |
| `ovl FILE START END` | a compact listing of lines START..END |
| `ds.py OFF...` | the C strings at those DS offsets of WAR.EXE |
| `analyze.py` | helpers on top of `wardis.py` (string starts, the routine an address belongs to) |

Paths: `fn` and `fc` look in `WAR_Analysis/` (or `$ASM_DIR`), `dec.py` reads `WAR_Analysis/WAR.EXE.asm` (or `$WAR_ASM`), `ds.py` reads `WAR_Original_Files_3.5/WAR.EXE` at the top of the repo (or `$WAR_EXE`). `dec.py` and `analyze.py` need Python 3; `wardis.py` needs `capstone`.

## dosbox/

Runs the original in DOSBox on a virtual display and drives it with key presses, for side-by-side checks. Needs `dosbox`, `Xvfb`, `xdotool`, ImageMagick (`import`), `ffmpeg` and Python 3 with Pillow.

Set-up: a working folder `$DOS` (default `/root/dos`) holding
- `war/`: a copy of the game disk (WAR_Original_Files_3.5)
- `war_ed/`: another copy with WAR.EXE patched at file offset 336Fh to `EB` (the data disk can then be B:)
- `data_b/`: the data disk, mounted as B: by `dosbox_ed.conf`
- the `.conf` files from here (their `mount` lines use `/root/dos`: change them if `$DOS` is elsewhere)
- `font8.txt`, made by `WAR_Remake/test/fontdump GAME-FOLDER > font8.txt` (in the repo's `WAR_Remake/test/`) and kept next to `screentext.py` (it is the game's own font, so it isn't kept in the repo)

| Script | Use |
|---|---|
| `dosrun2.sh` | start DOSBox with `$CONF` (default `dosbox.conf`) on display :99 |
| `dosgo.sh` | start, then answer the manual question |
| `dosanswer.sh` | read the manual question off the screen and type the answer (`warword.py` decodes WARW) |
| `doskey.sh KEY...` | key presses (xdotool names, `a+b` chords, `sleep:N`); `HOLD` sets how long each key is held (0.15 s at 1500 cycles, 0.6 s at 300) |
| `dosshot.sh NAME` | a screenshot to `$DOS/shots/NAME.png` |
| `screentext.py SHOT.png` | the 25 text rows of a screenshot, read with the game's font (unknown cells are `#`) |
| `doswait.sh TEXT [SECONDS]` | wait until TEXT appears in the text rows |
| `dosplay.sh` | from the title page, start the first campaign and wait for the map |
| `dostime.sh WORD` | press T until the command line shows Time (WORD) |
| `dosloads.sh` | start `dosbox_ed.conf` and load the saved game `data_b/S` |
| `dosfort.sh N` | load, Zoom, record the battle (`$DOS/fortcase/run_N`) and print "They lost N men" |
| `dosed.sh` | start `dosbox_ed.conf`, answer the question, open "make changes" |
| `dossnd.sh` | start with sound (`dosbox_snd.conf`); Ctrl+F6 records to `$DOS/cap` |
| `demowatch.sh SECONDS` | screenshots every 0.4 s into `$DOS/demo` |

Notes:
- `dosbox_xt.conf` runs at 300 cycles, about an IBM PC/XT; the others at 1500. Time things at 300.
- For timings record video, not screenshots: `ffmpeg -f x11grab -framerate 30 -video_size 640x400 -i :99.0+192,184 out.mp4`.
- Alt+S saves in WAR.EXE. A saved game `S` is DS:8868-A9E5 (see `../README.md`, Saved games); patching one is the quickest way to set up a test case.
- `dosbox_tandy.conf` / `dosbox_pcjr.conf`: for Tandy sound WAR.EXE was patched at 8132h to `90 90`.

## The remake's tests

They are in `WAR_Remake/test/` (`build.sh NAME` builds one; see the README there).
