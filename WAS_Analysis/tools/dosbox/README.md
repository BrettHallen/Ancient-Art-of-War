# Tools for WAS

## dosbox/

Runs the original WAS.EXE in DOSBox with the WAR harness (`../../../WAR_Analysis/tools/dosbox`: `dosrun2.sh`, `doskey.sh`, `dosshot.sh` work unchanged with `CONF=dosbox_was.conf`).

| File | Use |
|---|---|
| `setup_was.sh` | copies the At Sea disk to `$DOS/was` (default `/root/dos`) and patches that copy of WAS.EXE at file offset 7AB3h (`75 07` -> `EB 07`) so it starts without the original floppy (the key-disk check, ovl1_a9a9); installs `dosbox_was.conf` |
| `dosbox_was.conf` | machine=cga, 1500 cycles, mounts `$DOS/was` as A: and runs WAS.EXE |

From the title page, Enter five times (go to war, the first title, its story, the rules, the opponent) reaches the map of The Bane of Oldain.

Checked this way (12 Nov 2026): the scrolls are shown in CGA palette 0, everything else in palette 1; the first campaign's map with its squadrons matches the remake's viewer pixel for pixel (182 lines of map; the only difference is the original's cursor).
