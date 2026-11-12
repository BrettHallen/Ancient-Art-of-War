# The Ancient Art of War at Sea: analysis

Analysis of the files on the At Sea 3.5" disk (`WAS_Original_Files_3.5`), started 12 Nov 2026 after the WAR remake was finished, following the same method (see `Handoff_At_Sea.md`).

| File | What it is |
|---|---|
| `WAS_file_reference.md` | Every file on the disk: what it holds and how it is laid out |
| `WAS.EXE.asm` | Annotated disassembly of WAS.EXE (the CGA version), 59,000 lines |
| `disassembler/wasdis.py`, `disassembler/gen_asm.py` | The disassembler that writes `WAS.EXE.asm` (`python3 gen_asm.py`, needs capstone). Adapted from `WAR_Analysis/disassembler`: the layout (resident size, root segment, overlay table, thunks) is read from WAS.EXE itself. Add names to the table `N` in `gen_asm.py` as routines are understood, then run it again |
| `tools/dosbox/` | Runs the original WAS.EXE in DOSBox with the WAR harness, reads the screen, moves the marker (see its README) |
| `tools/disassembly/` | `fn`, `fc` and `dec.py` (C-like pseudocode) for WAS.EXE.asm |
| `manual_ocr/` | Text of the quick reference card, the hard-disk installation sheet and the manual pages scanned so far (raw tesseract output) |
| `../WAS_Graphics/extract_graphics.py` | Extracts the CGA and 16-colour pictures and map tiles into `../WAS_Graphics/CGA` and `../WAS_Graphics/VGA` |

## Findings so far

### At Sea is built like WAR
- WAS.EXE is linked with Phoenix PLINK86 and uses the same overlay loader as WAR.EXE; the same compiler and much of the same C code. 105 of WAR.EXE's named routines are found unchanged in WAS.EXE (matched by instruction sequence): the C library, file I/O, text output, the 8x8 font (byte-identical), sound, delay and random(), the scroll and menu helpers, the hardware detection and key-disk check, the editors' key and cursor handling, some campaign-play routines (select_squad, show_time_setting, msg_encounter, print_map_region, play_end_sound ...). Where a routine is the same, the WAR analysis and the WAR remake's port of it apply.
- The overlays map onto WAR's: overlay 1 = WAR's overlay 1 (main program), 2 = WAR's 3 (squads editor, now ships), 3 = WAR's 4 (map maker), 4 = WAR's 6 (campaign play). The battle is new: overlay 5 (sea battle, with overlay 6 beside it) and overlay 7 (boarding).
- The resident code is much bigger (6C50h bytes against 28D0h) and there are 61 overlay thunks (WAR: 21).
- The units look the same as WAR's: a 10 x 20 tile map of 32x20 tiles (320x400 pixels), ship x in byte columns and y in line pairs, 9999 for an empty slot, -1/0 for the two sides, 2 bits per pixel in an off-screen buffer with odd lines at +3E80h.

### What is different
- **New picture format** (load_picture ovl1:CBEA): word count per field, codes <= 200 are runs, > 200 literals. All 14 CGA pictures decode exactly to 2 x 8000 bytes.
- **WAST.DAT has no palettes**: the 16-colour pictures use the standard 16 colours. The 16-colour version is for the Tandy 1000 (WAST.EXE); it has 12 pictures (no 16-colour SPP or SPQ battle reports).
- **94 map tiles** (WAR: 72), with place names, ports, crowns, Neptune and sea serpents.
- **Campaigns S0-SA** (2180 or 2220 bytes): story, map, 40 squadrons (side, speed, position, supplies) and 40 ships (squadron, sails damage, type, name, hull damage; at most 3 ships to a squadron), opponent, 9 rule words. WAR had 20 squads and 10 flags. Saved game **SS** (10,444 bytes of DS).
- **Map sprites** in the data segment: anchor, crossed swords (an encounter), a ship in 16 headings, the flagship's pennant and the merchant's cargo. They are drawn as WAR's squads were (white with a black outline for the player, the reverse for the enemy), but the outline stays within each byte.
- **6 opponents** (Thor Foote, Medina Sidonia, Blackbeard, Tromp, John Paul Jones, Nelson).
- **No manual word question**; only the key-disk check.
- CGA palette 1 (bright cyan / magenta / white) for everything but the scrolls, which are in palette 0 as in WAR (checked in DOSBox).

### Campaign play (overlay 4), as far as the map goes
- campaign_play (ovl4:7650) has WAR's shape (WAR.EXE ovl6:3197): load SM and SO5 (the messages, to DS:4AF5), then the loop: blit the map, every [6312] iterations a tick (time slow 64, medium 16, fast 4, faster 1; no ticks while a key is down) with ovl4_8BCD, ovl4_96A0 (sighting and encounters), ovl4_9345 for two squadrons, every 10, 20, 25 and 30 ticks more work (at 25, with SCNF option [6865], a chance set by rule [64BB] of calling ovl4_C658, which also runs at the start: probably the wind), then campaign_input.
- The view: DS:3F3B is the top line pair (0-109), 182 lines of map with two text rows; the marker DS:5CC7 (byte 1-78) and DS:5CC9 (line pair 0-195), its buffer address DS:5CCB; the keypad's steps at DS:0739/0744. The marker stays within 86 line pairs of the top; - and + scroll by 2 line pairs; Shift jumps to the top (marker 38,45) or bottom (38,145).
- The marker is over a squadron when it is in the squadron's byte column and 2 line pairs above to 2 below the squadron's address line (y+1). The command line then shows Move (yours, not in an encounter), Info, Detach (2 or more ships and a free slot), Join (a one-ship squadron within 3 bytes and 7 line pairs of another), Zoom (in an encounter), Practice.
- Per squadron at run time: DS:56FF in an encounter (drawn as crossed swords: the sprite at DS:04AD is not a wreck), 574F seen, 583F on a traced path (path memory: the story's buffer DS:4E53, 1500 entries), 588F on a route (route memory DS:542F, 600 bytes), 5EE5 control (-2 at the start; commandable = 5EE5 -1, or side -1 and 5EE5 != 0), 5EBD pennant / merchant, 5FD5 heading, 6581 speed before putting into port.
- Info (squadron_info rt:074B) draws a panel over the map (lines 62-136): each ship's name, type and sails and hull bars, the supplies bar, a 0-100 scale, "SPEED: Anchored / Half Sail / Full Sail" (S changes it only for a squadron on a traced path), "Encounter" or "In Port" over the speed.
- Fleet (ovl4:AD63) counts the flagships, ships of the line and frigates on each side (counted by who commands the squadron).
- **Speed:** at 300 cycles (about an IBM PC/XT) the marker moves 27 bytes a second with a key held (video at 30 fps): the main loop runs about 27 times a second when nothing else is happening. WAS.EXE adds a delay only on faster PCs ([0D19] > 514h).
- Checked in DOSBox: the start, the command line over a squadron, Info, Fleet and the Surrender question are pixel-identical in the remake.

### Map with the files (checked)
S0, S5 and S9 were drawn from their map bytes with both tile sets and show the campaign maps (Oldain; the English Channel for the Armada; the North Atlantic for the Bismarck). The original in DOSBox (`tools/dosbox`) shows the first campaign's map and squadrons exactly as the remake's viewer draws them: 182 lines of map, squadrons at anchor, pennants over the ones with a flagship.

## Open questions
- The meaning of each rule word (the rules scroll in overlay 1 will tell).
- The 29-byte SCNF layout (Options Shoppe ovl1:9BCB).
- The SS layout (as WAR's, a block of DS: map once the play variables are named).
- Sprite formats in SO0, SO3 and SO8.
- How WAST.EXE colours the map sprites (the remake draws them white and black).
- The tile classes (port, crown, rough water, shallow water, land) and the wind.
- WAST.EXE (EXEPACK + 4 MS-LINK overlays) is not disassembled; as with WARET.EXE, the remake would assume it plays like WAS.EXE.

## Next steps
1. Name routines and DS variables as they are understood: next, the ticks of overlay 4 (sailing, the wind, sighting, ports, supplies) for the remake's step 3.
2. The Tandy version in DOSBox (WAST.EXE needs unpacking before its key-disk check can be patched), to see how it colours the map sprites and the Info panel.
3. The remake's step 3, the strategic rules, then the rest of the build order in `../WAS_Remake/WAS_Remake_plan.md`.
