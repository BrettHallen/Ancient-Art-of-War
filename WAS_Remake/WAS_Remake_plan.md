# The Ancient Art of War at Sea remake: plan

Drafted 12 Nov 2026 after the first analysis of the At Sea disk (`WAS_Analysis/README.md`, `WAS_Analysis/WAS_file_reference.md`). The decisions below are carried over from the WAR remake (`WAR_Remake`, `Remake_plan.md` in the project) unless marked **to agree**.

## Decisions carried over
- **Rewrite, not emulate.** C + SDL3, CMake, a macOS app bundle (also builds on Linux and Windows). Logic ported function by function from `WAS_Analysis/WAS.EXE.asm`; each C function names the routine it ports (e.g. `/* ovl4:85D2 */`), and keeps WAS.EXE's units and state so it can be checked against the asm and against the original in DOSBox.
- **Assets are read at run time from the player's own copy** (WAS.EXE, SM, SP*, B, AD, SO0-SO8, S0-SA, SITLES/SDITLES, SCNF, WAST.DAT). No original artwork, campaigns or fonts in the remake's code or repo; the app icon is drawn at build time from the player's files.
- **CGA and 16-colour modes** draw the same 320x200 screen, switched at any time with Tab; F2 changes the aspect (4:3 or square pixels). The 16-colour set is the Tandy 1000 one from WAST.DAT, in the standard 16 colours.
- **Speed:** the IBM PC/XT is the reference; measure in DOSBox at 300 cycles, timings from video.
- **Data disks are folders** in `~/Library/Application Support/AncientArtOfWarAtSea/Remake/` (SDL_GetPrefPath("AncientArtOfWarAtSea", "Remake"), as WAR's AncientArtOfWar/Remake), keeping the original formats (S files, SITLES, SS, SCNF) so the original can read what the remake writes; original saved games imported.
- **Deliberate differences** are listed here and agreed with the user first.
- **Source files** start with the user's header (agreed 12 Nov), UTF-8 with a BOM:
  ```
  /*
   * The Ancient Art of War at Sea (1987) - remake
   * Nov 2026 using Claude Opus 5.5
   * "Guided" by Brett Hallen
   * Original game by Dave Murry & Barry Murry
   * Published by Brøderbund Software Inc.
   *
   * file.c: what it does
   */
  ```
- **App name:** AncientArtOfWarAtSea (agreed 12 Nov): the CMake project and executable, and the Application Support folder (above).
- **Build order:** the same as WAR's (agreed 12 Nov), below.

- **Own copies, no shared library** (agreed 12 Nov): `WAS_Remake/` has its own `src/`, `test/` and `tools/`, laid out like `WAR_Remake/`; the parts that carry over from WAR (gfx.c, icon.c, tools/make_icon.c, test/build.sh, later sound.c and the main loop) are copied and changed as At Sea needs. `WAR_Remake/` is left as it is.

## To agree
- The scope of each step, as it comes up.

## Build order (as for WAR)
1. Asset loader + viewer - done (12 Nov)
   - assets.c: WAS.EXE's data segment (found through the overlay table), the font (DS:089C), the 20 map sprites (DS:049B-05CA) and the heading offsets (DS:075A); the 14 CGA pictures (load_picture ovl1:CBEA; the scrolls in palette 0, show_scroll ovl1:7F1A) and the 94 SM tiles; WAST.DAT's 12 pictures and 94 tiles in the standard 16 colours
   - campaign.c: S0-SA (load_campaign ovl1:C388): story, map, 40 squadrons (side, speed, position, supplies), 40 ships (squadron, sails and hull damage, type, name), opponent, rules; titles from SITLES
   - gfx.c: draw_map_sprite (rt:2941), the outline within each byte; campaign_draw_map draws the squadrons as draw_squadrons (rt:10D2) does at the start: the anchor, the pennant for a flagship's squadron (rt:0688)
   - viewer.c: pictures, tiles, font, map sprites, the 11 maps (scrolling; Enter: the story, then the squadrons and ships); Tab CGA / 16 colours, F2 aspect, H caption; `--screenshots`
   - main.c, icon.c, tools/make_icon.c: the window, the game folder (asked for once, remembered), the app icon from SPM
   - checked: the first campaign's map matches the original in DOSBox pixel for pixel (182 lines; WAS_Analysis/tools/dosbox); the scrolls' palette 0 seen in DOSBox
   - tests: test_viewer.c, compare_extract.py (the C and Python decoders agree on every picture and tile), fuzz_assets.c (damaged files under ASan)
2. Campaign map - done (12 Nov)
   - play.c: campaign_play (ovl4:7650) as far as the map goes: play_start from game_init (ovl1:7753: the bottom of the map, marker 38,145, time slow, 5EE5 = -2) and mark_flagship_squadrons (rt:0688); campaign_input (ovl4:7B65): the marker and scrolling (keypad steps DS:0739/0744, is_scroll_up_key rt:1217 / is_scroll_down_key rt:126A, Shift jumps, the view follows the marker, limits 1-78 and 0-195), check_cursor_over_squadron (ovl4:A202), show_command_line (ovl4:AA1C), squadron_command_line (ovl4:AB82, with join_partner ovl4:B252), Info (ovl4:7CAC, squadron_info rt:074B, show_speed rt:0AC7; S changes the speed only on a traced path), Fleet (ovl4:AD63, count_fleet ovl4:EF92), Time (show_time_setting ovl4:AAA8), Pause, Clear, Surrender (ovl4:7A83); draw_squadrons (rt:10D2), the marker (draw_cursor rt:280E), text_rows_add (rt:0BC2), the line under the map
   - main.c: Enter on a viewer map plays it (I now shows the story and ships); 27 iterations a second; keys as scan codes, a held marker key repeats every iteration (poll_input ovl4:8645)
   - checked in DOSBox (WAS_Analysis/tools/dosbox): the start, the command line over a squadron, Info, Fleet and the Surrender question are pixel-identical; the marker moves 27 bytes a second at 300 cycles (video at 30 fps), hence PLAY_HZ 27
   - tests: test_play.c, fuzz_play.c, test_app.c
   - left for step 3: Move (tracing a path), "In Port" in Info (port_of_cell ovl4:C268), the ticks' work (sailing, wind, sighting, ports, supplies); Detach, Join, Practice and Zoom with steps 3-4
3. Strategic rules: movement and speed (anchored / half sail / full sail), wind, ports (repair, supplies), rough and shallow water, merchants, sighting, crowns.
4. Battles: the sea battle (overlays 5 and 6: Zoom, courses, firing, orders) and boarding (overlay 7).
5. The enemy commander.
6. Menus, options, saved games, battle report, the opening and the demo.
7. The editors (map maker, ships, rules, story, title).
8. Sound.

Each step: tests (scripted walkthroughs with screenshots, fuzzers under ASan), the analysis README and this plan updated, files copied to the Mac.

## Deliberate differences
- Tab switches CGA / 16 colours (in WAS.EXE Tab scrolls up, as - does); F2 changes the aspect.
- Esc in play asks "Quit to the title page? (Y or N)" (no Esc in WAS.EXE); until step 6 it goes back to the viewer, as does Enter after a surrender.
- Info, Fleet and Pause are screens of the main loop rather than loops of their own (WAS.EXE waits in them); time is stopped in them as in the original.
- Keys other than the marker's act once per press (WAS.EXE reads a held key again every iteration, so a held T steps the time on more than once).
- (The viewer itself is the remake's own, as in WAR.)

## Tooling still to make
- The DOSBox harness (`WAS_Analysis/tools/dosbox`): starts the original at 1500 or 300 cycles, reads the screen, moves the marker. Still to add: the Tandy version (WAST.EXE is EXEPACKed, so the key-disk patch needs unpacking first).
- `WAS_Analysis/tools/disassembly`: `fn`, `fc` and `dec.py` work on WAS.EXE.asm (done 12 Nov); `ds.py` still to copy.
