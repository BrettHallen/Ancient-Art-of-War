# Tools for WAS: DOSBox

Runs the original WAS.EXE in DOSBox with the WAR harness (`../../../WAR_Analysis/tools/dosbox`: `dosrun2.sh`, `doskey.sh`, `dosshot.sh` and `screentext.py` work unchanged with `CONF=dosbox_was.conf`).

| File | Use |
|---|---|
| `setup_was.sh` | copies the At Sea disk to `$DOS/was` (default `/root/dos`) and patches that copy of WAS.EXE at file offset 7AB3h (`75 07` -> `EB 07`) so it starts without the original floppy (the key-disk check, ovl1_a9a9); installs the confs; writes `$DOS/font8.txt` (copy it next to `screentext.py`; it is the game's font, so it is not kept in the repo) |
| `dosbox_was.conf` | machine=cga, 1500 cycles, mounts `$DOS/was` as A: and runs WAS.EXE |
| `dosbox_was_xt.conf` | the same at 300 cycles (about an IBM PC/XT): use it for timings; keys must then be held about 0.6 s (`HOLD=0.6`) |
| `dosbox_was_herc.conf` | `machine=hercules`, runs `WAS.EXE HERC` (screenshots are 720x348) |
| `wasplay.sh` | starts WAS.EXE and presses Enter five times (go to war, the first title, its story, the rules, the opponent): the map of The Bane of Oldain |
| `findcur.py SHOT` | where the marker is on a screenshot or video frame |
| `wasgoto.py X CY` | moves the marker to byte X, line pair CY by tapping the keypad and reading the screen |
| `make_font8.py` | the game's font for `screentext.py` (the same as WAR's) |

At 1500 cycles a key held for WAR's default 0.12 s is read more than once (T moves the time on two steps): use `HOLD=0.02` for single presses.

## Checked this way (12 Nov 2026)
- The scrolls are shown in CGA palette 0, everything else in palette 1.
- The first campaign's start, the command line over a squadron, Info, Fleet and the Surrender question match the remake pixel for pixel (`WAS_Remake/test/test_play.c` makes the same screens).
- The main loop at 300 cycles: with a key held the marker moves 27 bytes a second (video at 30 fps), so the remake runs campaign play at 27 iterations a second.
- Step 3 (the rules), with `wasplay.sh`, `wasgoto.py` and the WAR harness's `doskey.sh` / `dosshot.sh` / `screentext.py`: tracing from F.D.Roosevelt (3,185) right and up until the land stops it, M, sailing to the end; Join with Herbert Hoover and Detach again; Running the Gauntlet sailing north (sighting, encounter, fighting, battle lost messages); "In Port" (The Last Crown, 51,182); the wind's face (SCNF byte 19h set to 1 in the DOSBox copy); Race for the Crown with the squadron moved beside the enemy crown (48,8) in the DOSBox copy of S1: taken on tick 20, "You Won" blinking, a key goes straight to the battle report; The Bane of Oldain with all but two enemy squadrons removed: the offer to surrender, shown over the opponent's page. All matched the remake (`WAS_Remake/test/test_rules.c`) pixel for pixel apart from the enemy's moves. The DOSBox copies of S0, S1 and SCNF were put back afterwards.
- `screentext.py` needs `font8.txt` next to it: `python3 make_font8.py ../../../WAS_Original_Files_3.5/WAS.EXE > font8.txt`, copied to `WAR_Analysis/tools/dosbox` (not kept in either repo folder).
- Hercules (`CONF=dosbox_was_herc.conf`): the first campaign's start matches the remake's Hercules screen (`WAS_Remake/test/test_play.c`, p1_start_herc) pixel for pixel. DOSBox shows the lit pixels as grey AAAAAAh.
