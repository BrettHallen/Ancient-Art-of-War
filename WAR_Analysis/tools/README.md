# Tools

The helpers used to study WAR.EXE and to check the remake against the original, saved from the cloud workspace where they were written (Nov 2026). They ran on Linux; most will work on macOS with the same programs installed.

## disassembly/

Work on `../WAR.ASM`, which `../disassembler/gen_asm.py` writes (its name table gives each routine and data address a name; add names there as routines are understood, then run it again).

| File | Use |
|---|---|
| `fn NAME` | one routine on a single line, by name or by address (`fn ovl6_4E26`, `fn rt_0859`), even after renaming |
| `fc NAME` | the same, shorter (no `word ptr`, comments) |
| `dec.py NAME` | C-like pseudocode with gotos for one routine, using the DS names in its table. It sometimes shows `x[i++] = v` as a pre-increment: check the asm for array writes and string loops |
| `ovl FILE START END` | a compact listing of lines START..END |
| `ds.py OFF...` | the C strings at those DS offsets of WAR.EXE |
| `analyze.py` | helpers on top of `wardis.py` (string starts, the routine an address belongs to) |

Paths: `fn` and `fc` look in `WAR_Analysis/` (or `$ASM_DIR`), `dec.py` reads `WAR_Analysis/WAR.ASM` (or `$WAR_ASM`), `ds.py` reads `WAR_Original_Files_3.5/WAR.EXE` at the top of the repo (or `$WAR_EXE`). `dec.py` and `analyze.py` need Python 3; `wardis.py` needs `capstone`.

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
| `screentext.py SHOT.png` | the 25 text rows of a screenshot, read with the game's font (unknown cells are `#`); a 720x348 Hercules screenshot is read from its CGA picture |
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
- `dosbox_herc.conf` / `dosbox_ed_herc.conf`: `machine=hercules` and `WAR.EXE HERC` (the second with the data disk as B:, for `dosloads.sh`). Screenshots are then 720x348; `dosanswer.sh` and `doswait.sh` work as before. Checked this way (Nov 13): the title page, the titles, story and rules scrolls, the campaign menu and the Options Shoppe matched the remake's Hercules screens (`WAR_Remake/test/test_herc`) pixel for pixel; the opponents, the map and a battle apart from what moves or blinks.

### WARET.EXE in DOSBox-X (the 16-colour version)

DOSBox 0.74 shows WARET.EXE's EGA, Tandy and VGA screens too, but DOSBox-X is the more faithful (the At Sea work found that 0.74 ignores a Tandy's palette registers), so these use `dosbox-x` (Ubuntu: `apt install dosbox-x`).

Set-up: `$DOS/war_et/` a copy of the game disk with `CNFET`; `$DOS/data_bx/` a data disk mounted as B: (CNFET's last word set to 1 so the data disk is B:). WARET.EXE's saved game is `SET`.

| Script | Use |
|---|---|
| `dosrunx.sh` | start DOSBox-X with `$CONF` on display :99. Its log goes to /dev/null: DOSBox-X writes gigabytes of it |
| `dosboxx_et_ega.conf`, `dosboxx_et_tandy.conf`, `dosboxx_et_vgaonly.conf` | WARET.EXE on an EGA, a Tandy 1000 and a VGA |
| `wseq.sh MACHINE` | the opening, title page, the three scrolls, the opponents and the map (`x_MACHINE_sNN.png`) |
| `wmenus.sh MACHINE` | the title page, the campaign menu and the Options Shoppe (`x_MACHINE_mNN.png`) |
| `wdemo.sh MACHINE` | record the demo (2 frames a second) into `$DOS/xdemo_MACHINE/` |
| `wquote.sh MACHINE` | the quote and PREPARE FOR WAR! (`x_MACHINE_qNN.png`, every half second) |
| `wtomap.sh MACHINE` | to the first campaign's map |
| `wtosquad.sh MACHINE` | the map, then the cursor onto your squad 16 with `steer.py X LINE`, which taps the arrow keys until `cursor.py` finds the cursor there (from its shape) and the command line shows a squad's commands: one tap moves WARET.EXE's cursor further than the remake's single key, so the scripts steer |
| `wboxes.sh MACHINE` | from your squad 16: Info, Size, Formation, Detach, a traced path, View, Time, Surrender and the battle report (`x_MACHINE_kNN.png`; `dump_cards` sends the same keys) |
| `wtrace.sh MACHINE` | from your squad 16: M, the cursor up and left, M: the trace box and the path's mark (`x_MACHINE_uNN.png`) |
| `wedit.sh MACHINE` | Make a new campaign: the map maker and the squads, Size, Info, Formation (`x_MACHINE_eNN.png`) |
| `wchange.sh MACHINE` | Change a campaign: the scroll of campaigns and the editor's menu (`x_MACHINE_cNN.png`) |
| `wsaveset.sh start\|play\|mark` | saved games for `WAR_Remake/test/test_set`, copied to `$DOS/sets/`: `start` saves the first campaign as it reaches the map with WARET.EXE (`SET_start`) and with WAR.EXE (`S_start`, through `dosboxx_et_cga.conf`); `play` traces squad 16 north-west, sets the time fast and saves 25 s later; `mark` saves with squad 16 on a long traced path |
| `wloadset.sh MACHINE` | WARET.EXE loads the data disk's `SET` and plays on: a shot every 4 s (`x_MACHINE_lNN.png`) |
| `wformed.sh MACHINE` | Teach squads new formations: formations 1, 3 and 6, the pointer (`x_MACHINE_fNN.png`) |
| `dosboxx_et_cga.conf` | WAR.EXE (CGA) with the data disk as B:, to run the same scripts on the CGA version (`$DOS/war_cgab/`: a copy of the game disk with CNF's last word set to 1) |
| `egacmp.py REMAKE.idx SHOT.png` | an EGA or Tandy screen (standard colours, so each is one index) against the remake's indexes from `WAR_Remake/test/dump_cards`: the differing pixels grouped by (remake index -> original index) |
| `vgacmp.py REMAKE.idx SHOT.png` | the same for a VGA screen, by colour |

Checked this way (Nov 14): the opening and title pages, the scrolls, the opponents, the quote, the campaign menu, the Options Shoppe, the map and its boxes, the trace box and path marks, the battle report and the editors matched pixel for pixel on all three cards, apart from what moves or blinks (DOSBox reads a key held 0.15 s twice in some places, so a script's screen can be one step on); the battle screen's soldiers and their outlines by counting each outline's place in its 4 or 8 pixels.

DOSBox-X ignores the signal to quit: `dosrunx.sh` kills the last one with `kill -9`, as stray copies slow the next run and change its timing.

### The CGA on a TV (composite)

DOSBox-X draws a CGA's composite output with `machine=cga_composite` (the old IBM CGA) and `machine=cga_composite2` (the new one), at its default TV settings. The remake's tables (`WAR_Remake/src/composite.c`) were measured with a test pattern:

| File | Use |
|---|---|
| `cgapat.py` | writes `CGAPAT.COM` (put it in `$DOS/comptest/`) and `pattern.json`: mode 4 with every triple of 2-bit pixels at both pixel parities, palette 1 then (after a key) palette 0. Needs `nasm` |
| `wcgapat.sh` | CGAPAT.COM on the old and the new CGA: shots `pat_MACHINE_p1.png`, `pat_MACHINE_p0.png` |
| `complut.py pattern.json SHOTS-DIR [OUT.c]` | the colour of each hdot by (CGA, palette, phase of 4 hdots, left, pixel, right) from those shots, reporting any seen in two colours; `OUT.c` gets the table as `composite.c` has it |
| `compcmp.py REMAKE.rgb SHOT.png` | a composite screen from `dump_cards` (`NAME_comp.rgb`, `NAME_comp2.rgb`) against DOSBox-X's |
| `dosboxx_pat_cga_composite.conf`, `dosboxx_pat_cga_composite2.conf`, `dosboxx_war_cga_composite.conf`, `dosboxx_war_cga_composite2.conf` | the pattern, and WAR.EXE, on the old and the new CGA |

Checked (Nov 14): the remake's composite screens are DOSBox-X's pixel for pixel, apart from the title page's blinking box.

## The remake's tests

They are in `WAR_Remake/test/` (`build.sh NAME` builds one; see the README there).
