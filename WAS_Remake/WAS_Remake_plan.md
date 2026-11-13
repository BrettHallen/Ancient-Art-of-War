# The Ancient Art of War at Sea remake: plan

Drafted 12 Nov 2026 after the first analysis of the At Sea disk (`WAS_Analysis/README.md`, `WAS_Analysis/WAS_file_reference.md`). The decisions below are carried over from the WAR remake (`WAR_Remake`, `Remake_plan.md` in the project) unless marked **to agree**.

## Decisions carried over
- **Rewrite, not emulate.** C + SDL3, CMake, a macOS app bundle (also builds on Linux and Windows). Logic ported function by function from `WAS_Analysis/WAS.EXE.asm`; each C function names the routine it ports (e.g. `/* ovl4:85D2 */`), and keeps WAS.EXE's units and state so it can be checked against the asm and against the original in DOSBox.
- **Assets are read at run time from the player's own copy** (WAS.EXE, SM, SP*, B, AD, SO0-SO8, S0-SA, SITLES/SDITLES, SCNF, WAST.DAT). No original artwork, campaigns or fonts in the remake's code or repo; the app icon is drawn at build time from the player's files.
- **CGA and 16-colour modes** draw the same 320x200 screen, switched at any time with Tab; F2 changes the aspect (4:3 or square pixels). The 16-colour set is the Tandy 1000 one from WAST.DAT, in the standard 16 colours.
- **Hercules** (asked for 12 Nov, after step 3): Tab's third mode, or `HERC` on the command line as `WAS HERC`. The CGA screen is shown as WAS.EXE shows it on a Hercules card (ovl1:CE78): each 2-bit pixel as two mono pixels, 640x200 at 32,72 of a 720x348 screen (gfx.c screen_to_herc); the whole 720x348 is shown, lit pixels grey AAAAAAh as in DOSBox. Checked against DOSBox's machine=hercules.
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
- The battle and the boarding in 16 colours: WAST.EXE (the Tandy version) may draw them in its own colours (WAST.DAT has a 16-colour deck for boarding); how is not yet known (WAST.EXE is EXEPACKed). Until then the remake shows the CGA battle in the 16-colour mode as well (Hercules shows it as WAS HERC does).

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
3. Strategic rules - done (12 Nov)
   - play.c, the clock (campaign_play ovl4:7959): every [6312] iterations a tick: encounters (ovl4:8BCD), the enemy (4 x ovl4:C79A, step 5), move_squadrons (ovl4:96A0, with set_move_delays ovl4:9AB2), eat (ovl4:9345) for two squadrons; tick 10 restock_ports (ovl4:BCD2), tick 20 crowns_check (ovl4:C51A), tick 25 the wind (ovl4:C658, rule 64BB), tick 31 the messages' timer. Every iteration map_battles (ovl4:8EDE)
   - Move: start_move_order (ovl4:89E4, find_free_path_run ovl4:ECD1), trace_prompt (ovl4:ACDA), the marker as a box moving every other iteration (campaign_input ovl4:81A8-85CC, trace_blocked ovl4:B3E5: land tiles, land bytes in the map buffer, the shore's slant), the steps into the path ring DS:4E53 (at most 80 a step), end_trace (ovl4:8A75: 99, full sail, the destination mark rt:2AB8), cancel_path (ovl4:8A2C), clear_paths (rt:0F55); step_heading (ovl4:9A3C)
   - sailing: 3 - speed ticks a step, doubled for ships of the line, +1/+2 for torn sails, shifted by DS:0ECD into the wind; losses in rough and shallow water (lost_at_sea ovl4:9D7E, rules 64B3 and 64B9, promote_at_sea ovl4:C302)
   - ports and crowns: coast_init (rt:2523, the shore types DS:425D and the table DS:0796), repair/supply_port_beside (rt:1AA8, 19D6), crown_beside (rt:1B7D), port_of_cell (ovl4:C268), port_supplies (ovl4:C197), invert_empty_ports (ovl4:C21C), port_call (ovl4:BD86), port_sources and sea_way (ovl4:C07C, E5BF, WAS.EXE's search kept as it is); "In Port" in Info
   - encounters, merchants met and taken (ovl4:BB29, BACD), sighting (rule 64B7); map battles (battle_hit ovl4:9154, count_loss ovl4:92B7); end_check (ovl4:9E4F: the opponent's values, the last ship, the last flagship (flags_check ovl4:C3C9 with 65BA/65BB), the offer to surrender), crown_check (ovl4:C44E), game_over (ovl4:A129)
   - Detach and Join (detach_join ovl4:A347, place_detached rt:1F2D); join_partner fixed: a squadron of up to two ships can join ([6458], not [6457])
   - the messages (message_row ovl4:BBBC, print_map_region ovl4:BC2D) and WAS.EXE's pauses (delay rt:2F6E: 50 us a unit on an XT) as iterations skipped
   - assets: SO2's six opponents (50 bytes from SO2+277h, copied to DS:604D), SCNF (the fight delay 6851, the wind 6865), the code words random() reads; game_init's new-game part (ovl1:77AD: flagships promoted where a side has none and no crown, ovl1_7A3F)
   - checked in DOSBox: tracing (the box, the land stopping it), the mark, sailing to the end, Join and Detach (all four screens each), the sighting/encounter/fighting/battle messages, "In Port", the wind's face (North and South), a crown taken on tick 20 with "You Won" (a key then ends the game at once, as WAS.EXE does after ovl4:C51A), the offer to surrender: pixel-identical, apart from the enemy's moves
   - tests: test_rules.c; fuzz_play.c also traces paths at the fastest time and checks the path ring
   - left for step 5: the merchants (created at ovl4:BEE3 with the enemy's routes, db21), port_side (ovl4:BD3C, the strength map ovl4:E805); for step 4: Zoom, Practice; for step 8: the crown's flashing tune
4. Battles - done (13 Nov)
   - the way in: Zoom on an encounter's countdown, Practice on a lone squadron (battle_ask, the shore [686B] from the map tile, a port beside or the land around); afterwards campaign_play goes on (play_after_battle: the flagships, the coast, end_check unless you Left, the ports' inversion)
   - fiber.c: WAS.EXE's battle draws straight to the screen and waits in delay() inside its loops, so the ported code runs as it was written, on a thread of its own taking turns with the main loop (a semaphore each way; only one runs at a time). The battle keeps its own clock: delay() units (50 us), each look at an empty keyboard, and the drawing and the rest of the work counted in 8086 instructions at 300 a millisecond; it hands back to the main loop when its clock passes real time, so it runs at an XT's speed on any machine
   - battle.c (overlays 5 and 6): the setup (ovl6_c9b0: SO6, SO7, SO8 into the data-segment copy at the offsets WAS.EXE's pointers use; the ships placed from the squadrons), the main loop (ovl5_7650), the keys (ovl5_7ac6: Gun, Course, Speed, Orders, Aim, Fire, View, Time, Pause, Leave, the ship keys, the view's scrolling), the panel (ovl6: the masts, speed, wind, compass, the gun crews and their animations, the orders, the messages), the end (ovl6_d5aa: ships captured, lost and the squadrons' changes back on the map)
   - battle_ships.c: drawing the ships (ovl5_afc6, rt_5ed1 with the sail holes and the sinking), sailing and turning (ovl5_b4c5, b841, rt_59ec), collisions, running aground and grappling (ovl5_bc12: "X grapples Y. Zoom Pass"), the guns (ovl5_936d, the hits abb6, the explosion a7ad), the speeds (ovl6_f0d2) and the orders and aim of every ship, yours and the enemy's (rt_34bf-4b35); where WAS.EXE reads a ship index of -1 or 6 the neighbouring arrays are rebuilt in WAS.EXE's layout
   - battle_gfx.c: the hand-written drawing routines (rt_5dc5-6765: the sea, the shore, masks, the box, the text) with their instruction counts
   - boarding.c (overlay 7): the deck B unpacked every frame into two fields where SO8 was (SO8 is loaded again afterwards), 40 men with their scripts and pictures from WAS.EXE's tables, Attack / Leave, FROM Flag / Quarterdeck / Poopdeck / Railing TO ... / Cross / Again, the enemy's orders every sixth frame, the officers' pistols, the end (the winners cheer, the loser's flag struck) and the ship taken
   - checked in DOSBox (WAS_Analysis/tools/dosbox, with GEO.WASHINGTON's squadron moved beside MACABRE's in the DOSBox copy of S0): Practice's overview and close view and Zoom's first screens pixel-identical; the boarding's deck pixel-identical (the men stand where the random numbers put them). Timing at 300 cycles: Practice at faster time 83 ms an iteration in the overview, 160-167 ms close (the remake 88 and 161 ms); a boarding frame about 0.50 s while the crews wait and 0.6 s as the fight starts (the remake 0.48 and 0.64 s)
   - tests: test_battle.c, fuzz_battle.c
   - not ported: the joystick (as in WAR); WAS.EXE's extra wait on fast PCs ([0D19] > 1500), as the remake runs at an XT's speed
   - left for step 5: the enemy's moves on the map; step 6: the demo's battles (the code is there, b->demo); step 8: the battle's sounds (counted, not played)
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
- Detach / Join, the opponent's offer and "You Won" / "You Lost" are screens of the main loop too; WAS.EXE's delays (after a message, a command) skip iterations rather than stop the program, and a key pressed meanwhile waits, as in the keyboard buffer.
- Until step 6 the opponent's offer to surrender at the start of a game is shown over the map (WAS.EXE shows it over the opponent's page, before the map).
- (The viewer itself is the remake's own, as in WAR.)

## Tooling still to make
- The DOSBox harness (`WAS_Analysis/tools/dosbox`): starts the original at 1500 or 300 cycles, reads the screen, moves the marker. Still to add: the Tandy version (WAST.EXE is EXEPACKed, so the key-disk patch needs unpacking first).
- `WAS_Analysis/tools/disassembly`: `fn`, `fc` and `dec.py` work on WAS.EXE.asm (done 12 Nov); `ds.py` still to copy.
