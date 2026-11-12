# The Ancient Art of War at Sea: analysis

Analysis of the files on the At Sea 3.5" disk (`WAS_Original_Files_3.5`), started 12 Nov 2026 after the WAR remake was finished, following the same method (see `Handoff_At_Sea.md`).

| File | What it is |
|---|---|
| `WAS_file_reference.md` | Every file on the disk: what it holds and how it is laid out |
| `WAS.EXE.asm` | Annotated disassembly of WAS.EXE (the CGA version), 59,000 lines |
| `disassembler/wasdis.py`, `disassembler/gen_asm.py` | The disassembler that writes `WAS.EXE.asm` (`python3 gen_asm.py`, needs capstone). Adapted from `WAR_Analysis/disassembler`: the layout (resident size, root segment, overlay table, thunks) is read from WAS.EXE itself. Add names to the table `N` in `gen_asm.py` as routines are understood, then run it again |
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
- **Campaigns S0-SA** (2180 or 2220 bytes): story, map, 40 ships (WAR: 20 squads and 10 flags), opponent, 9 rule words, 40 ship names of 15 bytes. Saved game **SS** (10,444 bytes of DS).
- **6 opponents** (Thor Foote, Medina Sidonia, Blackbeard, Tromp, John Paul Jones, Nelson).
- **No manual word question**; only the key-disk check.
- The CGA palette stays at the BIOS default (palette 1, bright cyan / magenta / white); palette 0 is used only for flashes in the sea battle and under one option.

### Map with the files (checked)
S0, S5 and S9 were drawn from their map bytes with both tile sets and show the campaign maps (Oldain; the English Channel for the Armada; the North Atlantic for the Bismarck).

## Open questions
- The four byte fields of the ship block and the meaning of each rule word (the editors in overlay 2 and ovl1 will tell).
- The 29-byte SCNF layout (Options Shoppe ovl1:9BCB).
- The SS layout (as WAR's, a block of DS: map once the play variables are named).
- Sprite formats in SO0, SO3, SO8 and the map sprites in the data segment.
- The extra 40 bytes in six of the campaign files (DS:6025; all zero).
- The tile classes (port, crown, rough water, shallow water, land) and the wind.
- WAST.EXE (EXEPACK + 4 MS-LINK overlays) is not disassembled; as with WARET.EXE, the remake would assume it plays like WAS.EXE.

## Next steps
1. Name routines and DS variables as they are understood, starting with overlay 4 (campaign play) where WAR's names already help.
2. DOSBox harness for WAS (new `.conf` mounts, a font dump for `screentext.py`; no manual-question helper is needed).
3. The asset viewer (the remake's step 1), then the rest of the build order in `../WAS_Remake/WAS_Remake_plan.md`.
