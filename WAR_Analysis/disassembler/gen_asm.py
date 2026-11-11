"""Generate the annotated WAR.EXE.asm from the wardis model.

Usage:  python3 gen_asm.py [output]     (default: ../WAR.EXE.asm)
Needs capstone (pip install capstone).
"""
import os
import sys
import re
import bisect
from collections import defaultdict, Counter
HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import wardis as W
from capstone import x86

OUT = sys.argv[1] if len(sys.argv) > 1 else os.path.join(HERE, '..', 'WAR.EXE.asm')

# ------------------------------------------------------------------ names
# (region, offset): (name, description)
N = {
    # --- resident library (segment 0000)
    ('rt', 0x0000): ('main', 'Game main loop: startup, menus, campaign play and battles, all through the overlay thunks'),
    ('rt', 0x0148): ('check_war_disk', 'Look for file M on drive arg (B:M / A:M / M) to find the WAR disk'),
    ('rt', 0x011E): ('require_war_disk', 'Loop until the WAR disk is found (prompts via disk_prompt)'),
    ('rt', 0x0400): ('disk_prompt', 'Status-line prompt for disk errors / "Not found", <R>etry or <Q>uit'),
    ('rt', 0x0468): ('open_m_file', 'Open file M (map tiles) on the WAR disk, prompting until found'),
    ('rt', 0x05A9): ('beep_low', 'PC-speaker beep (500 Hz-ish, short)'),
    ('rt', 0x05CC): ('beep_high', 'PC-speaker beep (higher pitch)'),
    ('rt', 0x06C1): ('show_one_moment', 'Clear status row 24 and print "One moment, please"'),
    ('rt', 0x176B): ('print_number', 'Print a decimal number (recursive /10)'),
    ('rt', 0x17A9): ('blit_buffer_to_cga', 'Copy the off-screen picture buffer (even lines at +0, odd at +3E80h) to CGA memory via the row table at DS:096A'),
    ('rt', 0x17ED): ('load_file', 'load_file(name, buffer, size): open (retrying via disk_prompt), read, close. Used for CNF, O-files, WarW, raw P-files'),
    ('rt', 0x1D8B): ('clear_box', 'Clear a box in CGA memory (8 scan lines x 25 words per text row)'),
    ('rt', 0x1DD1): ('draw_vline', 'Draw a vertical line (byte 0Fh) in CGA memory'),
    ('rt', 0x1DF3): ('draw_hline', 'Draw a horizontal run of FFh bytes in CGA memory'),
    ('rt', 0x1E12): ('draw_icon_8x6', 'Copy a 1-word-wide, 6-line icon (8x6 pixels) to CGA memory'),
    ('rt', 0x1E9F): ('scroll_text_area', 'Copy/clear 8 scan lines of the text area'),
    ('rt', 0x1EDE): ('clear_text_row', 'clear_text_row(row): blank one 8-pixel text row, then goto_xy(0,row)'),
    ('rt', 0x1F15): ('memset', 'memset(value, dest, count) in DS'),
    ('rt', 0x1F2C): ('memsetw', 'memsetw(value, dest, count) in DS'),
    ('rt', 0x1F43): ('memcpy', 'memcpy (rep movsb)'),
    ('rt', 0x1F5A): ('draw_bitmap', 'draw_bitmap(src, x, y): src starts with width/height words'),
    ('rt', 0x1F87): ('set_text_pos_px', 'Set pixel position for the next character (col*2, row*16)'),
    ('rt', 0x2017): ('invert_bar', 'XOR-invert an 8-line bar (menu highlight)'),
    ('rt', 0x2055): ('cga_addr', 'DI = row_table[BX] + DX  (CGA address helper)'),
    ('rt', 0x205C): ('goto_xy', 'goto_xy(col, row): remember text position, BIOS INT 10h AH=02h'),
    ('rt', 0x207D): ('print_spaces', 'Print N spaces'),
    ('rt', 0x20AA): ('random', 'Table-driven pseudo-random number 0-127'),
    ('rt', 0x20DC): ('speaker_freq', 'Program PIT channel 2 divisor (ports 43h/42h)'),
    ('rt', 0x20EF): ('speaker_on', 'Enable PC speaker (port 61h) if sound is on'),
    ('rt', 0x20FD): ('speaker_off', 'Disable PC speaker (port 61h)'),
    ('rt', 0x2135): ('tandy_sound', 'Write to the Tandy 1000 / PCjr SN76489 sound chip (port C0h)'),
    ('rt', 0x2188): ('delay', 'delay(n): n x m calls of random(), m = 1/2, 1, 1 1/2, 2 ... from [0948]; a delay of k x [0948] grows on faster PCs'),
    ('rt', 0x22C8): ('set_cga_palette', 'BIOS INT 10h AH=0Bh BH=1: choose CGA palette 0 or 1'),
    ('rt', 0x22E2): ('set_palette_reg', 'BIOS INT 10h AH=10h (PCjr/Tandy/EGA palette register)'),
    ('rt', 0x24E7): ('put_char', 'Draw one character with the 8x8 font, handles CR/LF/BS'),
    ('rt', 0x2565): ('print_string', 'print_string(s): put_char for each byte until NUL'),
    ('rt', 0x260A): ('dos_open', 'dos_open(name, mode): install INT 24h handler, INT 21h AH=3Dh (AH=3Ch create if mode==5). Returns handle or 0'),
    ('rt', 0x264E): ('dos_read', 'dos_read(handle, buf, count): INT 21h AH=3Fh'),
    ('rt', 0x2667): ('dos_write', 'dos_write(handle, buf, count): INT 21h AH=40h'),
    ('rt', 0x267E): ('dos_delete', 'dos_delete(name): INT 21h AH=41h'),
    ('rt', 0x268A): ('dos_close', 'dos_close(handle): INT 21h AH=3Eh'),
    # campaign play support (used by overlay 6)
    ('rt', 0x055B): ('text_rows_add', 'text_rows_add(n): grow the text area under the map by n rows ([A5C0]); the map then shows 198-8n lines'),
    ('rt', 0x0B27): ('update_squads', 'Recount men; squads of spies only get the small figure ([A090]=2); squads with no men are removed'),
    ('rt', 0x0BD5): ('count_men', 'men = archers + knights + barbarians + spies, for every squad'),
    ('rt', 0x1E35): ('draw_view_frame', 'View: the frame of the small map (48 x 85 pixels at line 106)'),
    ('rt', 0x1E67): ('draw_view_cell', 'View: one map cell, 4 pixels by 4 lines, from two CGA bit patterns'),
    ('rt', 0x0859): ('squad_strength', 'squad_strength(i, j): battle_edge/2 + 4 x (men_i - men_j); spies only: -67 / +67. Beyond +-50 the weaker squad is captured'),
    ('rt', 0x08EF): ('battle_edge', 'battle_edge(i, j): condition difference, +-15 on a hill, -30/+30 in water or spies only; -30..30'),
    ('rt', 0x09D3): ('place_formation', 'place_formation(first, squad, side): soldiers of a squad take the places of its formation (F file: types, x, y)'),
    ('rt', 0x0C44): ('clear_squad_path', 'clear_squad_path(i): zero squad i\'s steps in the path memory (DS:9782), remove its mark, clear its enemy route (DS:9B6A) and stop it'),
    ('rt', 0x0EEF): ('erase_map_sprites', 'Restore the map under the cursor, squads, flags and destination marks'),
    ('rt', 0x0F2C): ('draw_map_sprites', 'Draw destination marks, squads (rt_19d9), flags (rt_1ae0) and the cursor'),
    ('rt', 0x0F80): ('text_area_reset', 'Back to two text rows (drops messages) and clear row 23'),
    ('rt', 0x0FA2): ('is_scroll_up_key', 'Scan code is -, keypad - or Tab'),
    ('rt', 0x0FF5): ('is_scroll_down_key', 'Scan code is =, keypad + or Ctrl'),
    ('rt', 0x1048): ('tile_is_mountain', 'Map tile 37-54, 65, 66, 71 or 72'),
    ('rt', 0x1112): ('tile_is_forest', 'Map tile 55-57, 61-63 or 67-69'),
    ('rt', 0x11F8): ('tile_is_hill', 'Map tile 5 or 6 (hills)'),
    ('rt', 0x1228): ('tile_is_bridge', 'Map tile 9 or 10 (bridges)'),
    ('rt', 0x1258): ('tile_is_water', 'Map tile 7, 8, 13-15 or 19-36'),
    ('rt', 0x1330): ('squad_map_cell', 'squad_map_cell(i): ((y+5)/10)*10 + ((x+1)>>3), the 10x20 map cell under squad i'),
    ('rt', 0x1369): ('path_next_slot', 'path_next_slot(i): i+1, wrapping at 1000 (path memory DS:9782)'),
    ('rt', 0x1523): ('find_join_partner', 'find_join_partner(i): a squad of the same side within 3 columns and 7 line pairs (index+1, or 0)'),
    ('rt', 0x1836): ('save_cursor_background', 'Save the map bytes under the cursor to DS:9E54'),
    ('rt', 0x18A9): ('draw_cursor', 'Draw the cursor at [9E4E]: a cross, or an open box while tracing a path ([A92F]=2)'),
    ('rt', 0x190C): ('restore_cursor_background', 'Put back the map bytes saved by save_cursor_background'),
    ('rt', 0x1966): ('save_squad_backgrounds', 'Save the map under every squad'),
    ('rt', 0x19D9): ('draw_squads', 'Draw every visible squad with its pose: fighting, standing, weak (condition <= 10) or marching'),
    ('rt', 0x1A66): ('save_flag_backgrounds', 'Save the map under every flag'),
    ('rt', 0x1AE0): ('draw_flags', 'Draw the flags'),
    ('rt', 0x1B17): ('draw_map_sprite', 'draw_map_sprite(sprite, address, side): 8x12 figure, white with a black outline or the reverse'),
    ('rt', 0x1C93): ('restore_mark_background', 'restore_mark_background(i): remove squad i\'s destination mark'),
    ('rt', 0x1CBC): ('draw_destination_mark', 'draw_destination_mark(i): small white mark at [A51E+2i]'),
    ('rt', 0x1CE8): ('draw_text_area_border', 'White line under the map, then clear the text rows'),
    ('rt', 0x1D2E): ('pixel_pairs_match', 'pixel_pairs_match(address, code): does one of the four 2-pixel nibbles of the word match code? (terrain test)'),
    ('rt', 0x2096): ('seed_random', 'Seed random() from the BIOS tick count; table pointer 0BB8h'),
    # --- root (segment 028D)
    ('root', 0x02FA): ('start', 'Program entry point (MZ CS:IP = 028D:02FA): set up stack, preload DGROUP, run'),
    ('root', 0x0287): ('overlay_load', 'Make an overlay resident: open WAR.EXE, seek to its file position, read it'),
    ('root', 0x02D0): ('overlay_call', 'Target of every thunk: pops overlay number, calls overlay_load, jumps on'),
    # --- overlay 1
    ('ovl1', 0x3567): ('load_o3', 'Load O3 (scroll font, pointer, icons, title texts) to [A6D0]'),
    ('ovl1', 0x3594): ('load_o2', 'Load O2 (opponents, quotes, battle report and Options Shoppe texts)'),
    ('ovl1', 0x37E5): ('load_saved_game', 'Read the saved game S from the data disk (0x217E bytes of DS)'),
    ('ovl1', 0x3FA0): ('clear_all_text_rows', 'Clear text rows 24 down to 0 (a black screen)'),
    ('ovl1', 0x4545): ('menu_poll_key', 'Menus: read a key (or the joystick)'),
    ('ovl1', 0x4D08): ('help_change_select', 'Row 24: [+] or [-] to change, [Enter] ...'),
    ('ovl1', 0x5030): ('rules_text', 'The RULES scroll: one sentence per rule'),
    ('ovl1', 0x5706): ('report_number', 'Battle report: a number right-aligned in three cells of the scroll digits'),
    ('ovl1', 0x5731): ('report_digits', 'Battle report: the digits of a number'),
    ('ovl1', 0x5A6E): ('shoppe_display', 'Options Shoppe: "display is black/white / tv/composite / rgb" ([A6DF])'),
    ('ovl1', 0x5AFA): ('shoppe_sounds', 'Options Shoppe: "all sounds are off / do not play music / all sounds are on" ([A6E1])'),
    ('ovl1', 0x5B98): ('shoppe_fight_delay', 'Options Shoppe: "fight delay is short / medium / long" ([A6E3] 200/400/800)'),
    ('ovl1', 0x5BFE): ('shoppe_data_disk', 'Options Shoppe: "data disk uses a: / b:" ([A6F5])'),
    ('ovl1', 0x5C4D): ('shoppe_joystick', 'Options Shoppe: "(do not) use joystick" ([A6E7])'),
    ('ovl1', 0x5CA3): ('shoppe_help', 'Options Shoppe: the key help on rows 23 and 24'),
    ('ovl1', 0x6018): ('campaign_menu_choose', 'Campaign menu (PG): +/- move the frame between the six scenes, Enter chooses'),
    ('ovl1', 0x6238): ('story_view', 'The story scroll: +/- scroll 4 lines, Enter done, Esc back'),
    ('ovl1', 0x62FA): ('choose_title', 'The TITLES scroll: +/- move the pointer two rows, Enter chooses ([A694])'),
    ('ovl1', 0x655A): ('titles_help', 'Rows 23 and 24 for the TITLES scroll'),
    ('ovl1', 0x6663): ('rules_view', 'The RULES scroll: +/- move the pointer, Space changes a rule'),
    ('ovl1', 0x6A78): ('scroll_editor', 'Scroll text editor (campaign editor: story, title)'),
    ('ovl1', 0x6D28): ('scroll_goto', 'Scroll text position: column, row'),
    ('ovl1', 0x6D7E): ('scroll_print', 'Write a string on the scroll; a capital takes two cells'),
    ('ovl1', 0x6DE4): ('small_caps', 'Write a lowercase string in the small capitals of O3 on a picture (title box, Options Shoppe)'),
    ('ovl1', 0x6E80): ('scroll_clear', 'Clear the scroll text (all cells to the parchment glyph)'),
    ('ovl1', 0x6EDE): ('scroll_row_up', 'Scroll pointer up a row; the window follows'),
    ('ovl1', 0x6F6D): ('scroll_row_down', 'Scroll pointer down a row; the window follows'),
    ('ovl1', 0x6FE0): ('scroll_advance', 'Next scroll cell (skipping the 0x01 after a capital)'),
    ('ovl1', 0x70C7): ('scroll_char', 'One scroll cell: clear it, draw the glyph (capital 16x10, small capital 8x8 two lines down, punctuation)'),
    ('ovl1', 0x7499): ('scroll_open', 'CGA palette 0, then the top and the bottom curl of the scroll'),
    ('ovl1', 0x74D1): ('scroll_write_text', 'Write the 26 x 32 text at DS:9782 onto the scroll, count the titles ([A8CD])'),
    ('ovl1', 0x75C5): ('scroll_unroll', 'Unroll the scroll down the screen (with clicks)'),
    ('ovl1', 0x7659): ('scroll_rollup', 'Roll the scroll back up'),
    ('ovl1', 0x76DA): ('help_plus_minus', 'Row 24: [+] or [-]'),
    ('ovl1', 0x7870): ('load_campaign_file', 'Read a T file: story, map, armies, opponent, rules'),
    ('ovl1', 0x79B7): ('squad_addresses', 'Map buffer addresses of the squads and flags'),
    ('ovl1', 0x7B3D): ('campaign_source', 'TITLES from the WAR disk or the data disk; "Saved game / New campaigns" when the data disk has S'),
    ('ovl1', 0x7FBD): ('scroll_copy_lines', 'Copy line pairs of the scroll buffer to the screen'),
    ('ovl1', 0x7FFC): ('copy_glyph', 'Copy an O3 glyph (width words, height line pairs) into the buffer'),
    ('ovl1', 0x8037): ('and_pointer', 'AND the sword pointer onto the screen'),
    ('ovl1', 0x8070): ('xor_box', 'XOR a box with rounded corners (the blinking title box, the sweep across the top rows)'),
    ('ovl1', 0x810B): ('xor_frame', 'XOR a frame 4 pixels wide around a picture (opponents, campaign menu)'),
    ('ovl6', 0x4207): ('clear_keys', 'Forget the last key'),
    ('ovl6', 0x462B): ('formation_choose', 'Formation: +/- choose 1-9, the men shown as A, B and K in a box; E ends'),
    ('ovl6', 0x62A7): ('detach_join', 'Detach or Join: the table of the two squads; K, B, A, S move a man; J/D change direction; E ends'),
    ('ovl6', 0x69BF): ('group_full', 'Detach/Join: a squad is full (14): beep, change direction'),
    ('rt', 0x0606): ('set_display_palette', 'CGA palette registers for the display setting (black/white, composite, rgb) when the hardware has them'),
    ('rt', 0x0D3F): ('formation_preview', 'Formation: place the men (place_formation) and draw them in the box at the bottom left'),
    ('rt', 0x1D68): ('formation_box_clear', 'Formation: clear the box (bytes 0-16, lines 86-189)'),
    ('ovl1', 0x30F0): ('ovl1_startup', 'thunk 0. Load CNF (options) and PM (title screen), show "presented by"'),
    ('ovl1', 0x325F): ('game_init', 'thunk 1. Set up buffers and variables, load O3, show the title screen with the barbarian (PI); rule enemy seen >= 80: spies become barbarians; [A4E8] at most half the enemy squads'),
    ('ovl1', 0x35C4): ('save_game', 'thunk 4. Save the game to file S on the data disk; on failure "Game not saved!" / "<C>ontinue with game or <Q>uit?"'),
    ('ovl1', 0x389B): ('ensure_war_disk', 'thunk 15. Make sure the WAR disk is in a drive (check_war_disk)'),
    ('ovl1', 0x39AA): ('show_scroll', 'thunk 14. Story / text scroll: loads PE or PF'),
    ('ovl1', 0x3D1E): ('campaign_menu', 'thunk 6. Loads PG (scene sheet 1) and O4 (menu text)'),
    ('ovl1', 0x3E08): ('campaign_intro', 'thunk 10. Story scrolls (show_scroll) and choose_opponent'),
    ('ovl1', 0x3EB6): ('editor_menu', 'thunk 16. Loads PH (scene sheet 2: rulebook, storybook...) and O4'),
    ('ovl1', 0x3FC3): ('ovl1_entry_3fc3', 'thunk 17'),
    ('ovl1', 0x4000): ('select_campaign', 'thunk 11. Copy the campaign file name (table of T0-TA at DS:08C1, 3 bytes each, index [A694]) to DS:0B3E and load it'),
    ('ovl1', 0x3505): ('place_flag', 'A side with no flag gets one at a random place: x 8-71, y 31-158'),
    ('ovl1', 0x4161): ('show_title_barbarian', 'Loads PI (title screen with barbarian); 189 turns with no key start the demo: one of T3-TA, opponent 0, all seen'),
    ('ovl1', 0x4808): ('choose_opponent', 'thunk 18. Loads PJ (the 8 opponent portraits)'),
    ('ovl1', 0x4EB2): ('data_disk', 'thunk 7. Data disk handling (ovl1_61fd prints "Unable to use data disk.")'),
    ('ovl1', 0x53AE): ('battle_report', 'thunk 5. Loads PN (victory) or PO (defeat): captured / casualties / flags taken'),
    ('ovl1', 0x5782): ('options_shoppe', 'thunk 13. Ye Olde Options Shoppe: loads PK'),
    ('ovl1', 0x5D5D): ('save_options', 'Write CNF (create, mode 5)'),
    ('ovl1', 0x7816): ('make_data_path', 'Copy file name to DS:0B3E, prefixed with "A:" or "B:" for the data disk'),
    ('ovl1', 0x7EDA): ('init_hardware', 'thunk 19. Hook INT 05h (Print Screen) and INT 1Bh (Ctrl-Break), read equipment list (INT 11h), check video, calibrate delay'),
    ('ovl1', 0x84B9): ('check_video', 'Run detect_video unless already done'),
    ('ovl1', 0x84E5): ('calibrate_delay', 'Time a loop against PIT channel 2 and store the delay count in [0948]'),
    ('ovl1', 0x8163): ('load_picture_rle8', 'load_picture_rle8(name, dest): CGA pictures PE-PO. Pairs of [count, byte]; runs alternate between the even-line field (dest+0..1F3Fh) and the odd-line field (dest+3E80h..5DBFh)'),
    ('ovl1', 0x8210): ('rle8_read_chunk', 'Read the next 512 bytes of the picture file into the buffer at [A6CE]'),
    ('ovl1', 0x822D): ('bios_disk_rw', 'bios_disk_rw(head, track, sector, buf, write): INT 13h AH=02h/03h, 4 retries'),
    ('ovl1', 0x827B): ('bios_disk_reset', 'INT 13h AH=00h'),
    ('ovl1', 0x8314): ('detect_joystick', 'Probe the game port (201h); returns 1 if a joystick responds'),
    ('ovl1', 0x8341): ('key_disk_check', 'COPY PROTECTION (disk): read track 10 sectors 6 and 7, then sector 241 '
                                         '(must fail with CRC error 10h), and compare 16 words. Returns 1 if the original disk is present'),
    ('ovl1', 0x83C0): ('wait_port_change', 'Wait until (port DX & BL) changes; CF=1 on timeout'),
    ('ovl1', 0x83D5): ('detect_video', 'Probe for Hercules (3BFh) and CGA RAM, force colour mode, set mode 4 (320x200)'),
    # --- overlay 2 (resident next to overlay 1)
    ('ovl2', 0x8520): ('copy_protection', 'thunk 20. COPY PROTECTION (manual): loads WarW (encrypted word list) and asks for a word from the manual'),
    ('ovl2', 0x8606): ('show_welcome', '"Welcome to The Ancient Art of War."'),
    ('ovl2', 0x895C): ('show_thank_you', '"Thank You" (correct answer)'),
    ('ovl2', 0x8981): ('show_wrong_answer', '"Sorry, that answer is wrong."'),
    ('ovl2', 0x89A9): ('ask_manual_word', '"On page N, what is word N in paragraph N"'),
    # --- other overlays
    ('ovl3', 0x30F0): ('squads_editor', 'thunk 9. Campaign editor step 2: place squads and flags (loads O1)'),
    ('ovl4', 0x30F0): ('map_maker', 'thunk 8. Campaign editor step 1: map maker (loads O0)'),
    ('ovl5', 0x30F0): ('formation_editor', 'thunk 12. "Teach squads new formations": loads O8 (text) and PL (background)'),
    ('ovl5', 0x470F): ('decode_picture_rle16', 'Decode word-RLE picture from DS:0B68 to [bp+4]: two fields (even lines, then odd lines at +3E80h); each = word N, then N x [count byte, word]'),
    ('ovl6', 0x30F0): ('campaign_play', 'thunk 2. The strategic map game (loads O5 messages). Main loop: a tick every [A51C] iterations (Time: 64, 16, 4, 1)'),
    ('ovl6', 0x3413): ('init_cell_costs', 'Cost of each map cell for the enemy (DS:8B8C) from water, mountains, forest and forts'),
    ('ovl6', 0x3589): ('check_victory', 'Count men on each side; win, loss, or the opponent offers to surrender'),
    ('ovl6', 0x380F): ('game_over', 'Show every squad, "You Won" / "You Lost ... [Enter] to continue", wait in mode 99'),
    ('ovl6', 0x38F6): ('play_end_sound', 'Two tones on the PC speaker'),
    ('ovl6', 0x392B): ('check_flags', 'Each tick: a squad within 1 column and -1..2 line pairs of the other side\'s flag takes it'),
    ('ovl6', 0x3ADE): ('count_flags', 'Returns 1 if you have no flags left, 2 if the enemy has none (also picks the enemy\'s target flag)'),
    ('ovl6', 0x4E26): ('map_battles', 'Every iteration: encounters count up to the fight delay ([A6E3], Zoom possible), then the squads blink and kill each other\'s men until one is gone'),
    ('ovl6', 0x5A6E): ('msg_encounter', '"WARNING - ENCOUNTER IN THE ..."'),
    ('ovl6', 0x5AC7): ('msg_fighting', '"A L E R T  - FIGHTING IN THE ..." (your squads)'),
    ('ovl6', 0x5B1E): ('msg_battle_result', '"NOTICE  - BATTLE WON / LOST IN THE ..."'),
    ('ovl6', 0x5B6E): ('msg_captured', '"ENEMY CAPTURED" / "YOUR SQUAD CAPTURED IN THE ..."; counts the men captured'),
    ('ovl6', 0x52F8): ('food_and_condition', 'Every 16 ticks: food from the supply lines, condition from marching, resting and hunger'),
    ('ovl6', 0x5526): ('forts_train_men', 'Every 16 ticks: a fort may add a man to a squad in it (rule: forts train men)'),
    ('ovl6', 0x5613): ('enemy_split_in_fort', 'A full enemy squad resting in a fort splits off a man into a free slot'),
    ('ovl6', 0x5BE7): ('msg_flag_taken', '"URGENT - FLAG TAKEN IN THE ..."'),
    ('ovl6', 0x5C56): ('msg_flag_captured', '"ENEMY FLAG CAPTURED IN THE ..."'),
    ('ovl6', 0x570B): ('ai_join', 'Enemy: move men into a nearby squad of the same side with room (find_join_partner)'),
    ('ovl6', 0x6B03): ('ai_food_cell', 'Enemy: the village or fort to go to for food (near, and held by the enemy)'),
    ('ovl6', 0x760E): ('cell_distance', 'Steps between two map cells, moving diagonally'),
    ('ovl6', 0x7698): ('ai_tick', 'Enemy: each tick the next enemy squad standing still gets its orders ([A8CD] round robin, [A50A] the squad)'),
    ('ovl6', 0x7742): ('ai_orders', 'Enemy: follow the route ([A1BC]) or decide ([A0B8] clear)'),
    ('ovl6', 0x7778): ('ai_decide', 'Enemy: hold a fort, village, bridge, hill or flag cell, rest, or plan a move (by the general\'s values)'),
    ('ovl6', 0x7886): ('ai_plan', 'Enemy: destination: food, guard its own flag ([A512]), or the target ([A514]: your flag [A516] or the best cell)'),
    ('ovl6', 0x7A94): ('ai_follow_route', 'Enemy: at the end of a cell: grab a flag, attack a squad nearby, march the next route cell, or arrive'),
    ('ovl6', 0x7D6E): ('ai_count_heading', 'Enemy squads in a cell or heading for it'),
    ('ovl6', 0x7DDE): ('ai_count_stopped', 'Squads of one side standing still in a cell'),
    ('ovl6', 0x7E2F): ('ai_choose_attack_flag', 'Enemy: which of your flags to go for ([A516]): weakly held, near the last one, a little random'),
    ('ovl6', 0x7F0D): ('ai_nearest_squad', 'Nearest squad of one side by cells (mode 2: ones the enemy squad is strong enough for and can reach); [A8C7] = distance + 1'),
    ('ovl6', 0x7FFC): ('ai_nearest_flag', 'Nearest flag of one side within reach, not across water; [A8C7] = distance'),
    ('ovl6', 0x80C1): ('ai_blocked', 'Is the straight line of cells blocked by water (or mountains if the general\'s mountain factor is over 7)?'),
    ('ovl6', 0x8176): ('ai_make_path', 'Enemy: write a path of at most N steps toward a point into the path memory and set off'),
    ('ovl6', 0x825E): ('ai_route', 'Enemy: plan a route of map cells to [A8CF] (cheapest first, costs DS:8B8C + cells you hold), keep half in the route memory DS:9B6A'),
    ('ovl6', 0x8B7E): ('ai_cell_values', 'Worth of each map cell for the enemy (DS:9294): forts, villages, bridges, hills, and your flags (+130)'),
    ('rt', 0x1383): ('flag_cell', 'Map cell of a flag'),
    ('rt', 0x13BC): ('path_steps_toward', 'Steps from the squad toward a point into the path memory (across four steps in five)'),
    ('rt', 0x14A0): ('route_free_run', 'Start of the longest free run in the route memory (DS:9B6A, 200 bytes)'),
    ('rt', 0x161B): ('new_squad_from', 'A new squad in a free slot, standing with another (Detach, enemy_split_in_fort)'),
    ('ovl6', 0x873A): ('supply_map', 'Strength of each side over each map cell (DS:8F74, minus = yours), spread twice to neighbouring cells'),
    ('ovl6', 0x89A9): ('show_view', 'View: small map of who holds what, "Enemy has N men", "you have N men"'),
    ('ovl6', 0x3B87): ('campaign_input', 'Handle one key: commands for the current mode, then cursor movement and scrolling'),
    ('ovl6', 0x421C): ('poll_input', 'Read the key ([AAED], scan code); cursor keys repeat while held; the demo holds its key, 1 in 32 a new - or +'),
    ('ovl6', 0x4605): ('command_pause', 'Short delay after a command'),
    ('ovl6', 0x477D): ('start_move_order', 'Move: stop the squad, find room in the path memory (find_free_path_run)'),
    ('ovl6', 0x47C5): ('stop_squad', 'Stop the selected squad and clear its path and mark'),
    ('ovl6', 0x47F8): ('find_free_path_run', 'Start of the longest free run in the path memory, or -1 if under 10 steps'),
    ('ovl6', 0x48AA): ('confirm_move', 'Mark: end the path (99), show the destination mark, start the squad moving'),
    ('ovl6', 0x49B6): ('encounters_and_sighting', 'Each tick: friendly v enemy distance; touching starts a battle, close enough is "ENEMY SIGHTED"'),
    ('ovl6', 0x50D9): ('lose_man', 'lose_man(attacker, defender, count): archers beat knights, knights beat barbarians, barbarians beat archers'),
    ('ovl6', 0x57EA): ('update_move_delays', 'Ticks per step for each squad: speed x terrain rule (+speed for knights); men lost in deep water / high mountains'),
    ('ovl6', 0x59A5): ('msg_squad_lost', '"NOTICE - SQUAD LOST IN THE MOUNTAINS" / "DROWNED IN THE ..."'),
    ('ovl6', 0x59FC): ('msg_enemy_sighted', '"ALERT - ENEMY SIGHTED IN THE ..." (once per map region)'),
    ('ovl6', 0x5CC5): ('new_message_row', 'Add a message row above the command line (two at most, then scroll)'),
    ('ovl6', 0x5D00): ('map_region_code', 'North/south/west/east/interior code for a position'),
    ('ovl6', 0x5D42): ('print_map_region', '" IN THE NORTH", "SOUTHEAST", "INTERIOR"...'),
    ('ovl6', 0x5DF2): ('move_squads', 'Each tick: count down every moving squad and take its next path step'),
    ('ovl6', 0x603E): ('squad_terrain', 'Ground under a squad: 55h forest, 66h water, 99h mountain, 44h/11h open (from the map tile and CGA pixels)'),
    ('ovl6', 0x6171): ('check_cursor_over_squad', 'Is the cursor on a squad? Show its commands, or the normal command line'),
    ('ovl6', 0x6A21): ('supply_distance', 'Steps to the nearest friendly village or fort, divided by the supply rule'),
    ('ovl6', 0x6BAC): ('show_command_line', '"View  Time (slow)  Clear  Surrender"'),
    ('ovl6', 0x6C0B): ('show_time_setting', '"faster", "fast", "medium" or "slow" from [A51C]'),
    ('ovl6', 0x6C63): ('show_clear_option', '"Clear" when there are messages'),
    ('ovl6', 0x6CA3): ('select_squad', 'Cursor on a squad: find a free slot, show its commands'),
    ('ovl6', 0x6CDD): ('show_squad_commands', '"Move Info Size Formation Detach Join Zoom"'),
    ('ovl6', 0x6E1A): ('start_tracing', '"Trace path to destination and press [M]", cursor becomes a box'),
    ('ovl6', 0x6EA0): ('show_squad_size', 'Size: "KNIGHT:n BARBARIAN:n ARCHER:n SPY:n"'),
    ('ovl6', 0x6FAB): ('show_squad_info', 'Info: orders, condition and food bars; S changes the speed, E ends'),
    ('ovl6', 0x7127): ('draw_info_scale', 'Info: lines and the 0-25-50-75-100 scale'),
    ('ovl6', 0x7245): ('info_orders', '"Marching in a northwest direction" / "STOPPED - when moving: MARCH"'),
    ('ovl6', 0x741F): ('info_condition', '"Condition ----" bar with + or -'),
    ('ovl6', 0x750F): ('info_food', '"Food ---------" bar with + or -'),
    ('ovl6', 0x7588): ('cell_centre_x', 'Byte column of the middle of a map cell'),
    ('ovl6', 0x75A4): ('cell_top_y', 'Line pair of the top of a map cell'),
    ('ovl6', 0x75BB): ('step_distance', 'Steps between two positions moving diagonally'),
    ('ovl7', 0x30F0): ('battle', 'thunk 3. Battle screen (loads O6, O7, and the terrain background P0-PD)'),
    ('ovl7', 0x4738): ('battle_setup', 'Pick the two squads, the enemy formation (by opponent), the background P0-PD from the terrain, place the soldiers, set each side\'s skill (45 +- 2 x battle_edge)'),
    ('ovl7', 0x5258): ('battle_soldiers', 'Each frame: choose every soldier\'s next animation script from orders, terrain and the nearest enemy'),
    ('ovl7', 0x60B0): ('tile_is_battle_p7', 'Map tile 11, 12, 16-18, 58-60, 64 or 70 (battle background P7)'),
    ('ovl7', 0x652F): ('battle_input', 'Battle command line: choose Archer / Barbarian / Knight / Squad, then Attack, Backoff, Forward, Stop; Retreat'),
    ('ovl7', 0x46A9): ('battle_losses', '"You lost N  They lost N"'),
    ('ovl7', 0x438A): ('battle_retreat', 'One side retreats: its squad moves away on the map; the battle ends 75 frames later at most'),
    ('ovl7', 0x5057): ('battle_alive_types', 'Which of your types (knight, barbarian, archer) still have men on the field'),
    ('ovl7', 0x5103): ('battle_ask_attack', 'Row 24: "Attack ... Retreat"'),
    ('ovl7', 0x5148): ('battle_show_groups', 'Row 24: "Archer Barbarian Knight Squad ... Retreat"'),
    ('ovl7', 0x5204): ('battle_show_orders', 'Row 24: "Attack Forward Backoff Stop"'),
    ('ovl7', 0x5CBF): ('battle_step', 'Keep a soldier on the field, take one step of his script (move, picture, state) and draw him'),
    ('ovl7', 0x61CE): ('battle_kill', 'A soldier falls: counts, death script, and perhaps a rout or a lost fort'),
    ('ovl7', 0x6C4E): ('battle_check_target', 'Is the soldier\'s target still alive and within reach (-2 if not)?'),
    ('ovl7', 0x6D01): ('battle_choose_target', 'Nearest enemy (his own attacker first; barbarians favour archers, knights barbarians); none left ends the battle'),
    ('ovl7', 0x6F48): ('battle_toward', 'Direction to the target: -160/0/160 for up/down, +-1 for across'),
    ('ovl7', 0x700C): ('battle_close_in', 'Close enough to fight, or step round a friend in the way'),
    ('ovl7', 0x7173): ('battle_sort', 'One bubble pass ordering the soldiers by y (drawing order)'),
    ('ovl7', 0x745B): ('battle_archer_aim', 'An archer chooses a target and one of 20 directions; the arrow is loosed when the shooting picture comes'),
    ('ovl7', 0x7A52): ('battle_loose_arrow', 'The arrow leaves: position and power (skill x 2 + 40)'),
    ('ovl7', 0x7AFB): ('battle_arrows_hit', 'Arrows that reach a soldier: power, armour (knights), dodging (barbarians), friendly fire'),
    ('ovl7', 0x7D0C): ('battle_arrows_fly', 'Arrows move (O7 tables: dx, dy bytes, address words) and are drawn'),
    ('ovl7', 0x7E97): ('decode_picture_rle16_b', 'Same decoder as decode_picture_rle16 (copy in overlay 7)'),
    ('ovl7', 0x7ED2): ('blit_rows_to_cga', 'Copy rows of the off-screen buffer to CGA memory (B800h)'),
}

OVERLAY_ROLE = {
    1: 'Main program: startup, title and menu screens, options (CNF), save/load, data disk, hardware detection, disk copy-protection check, byte-RLE picture loader',
    2: 'Manual copy protection (WarW word list) and welcome message. Loads at 0852h, after overlay 1, so both can be resident together',
    3: 'Campaign editor: placing squads and flags (O1)',
    4: 'Campaign editor: map maker (O0)',
    5: 'Formation editor, "Teach squads new formations" (O8 text, PL background)',
    6: 'Campaign play: the strategic map, movement, spies, messages (O5)',
    7: 'Battle screen: the fight between two squads (O6, O7, P0-PD backgrounds)',
}

KNOWN_VARS = {
    0x0959: 'video segment (B800h)', 0x096A: 'CGA row address table', 0x0944: 'picture decode destination',
    0x0B3E: 'path buffer for make_data_path', 0x0B68: 'raw file / picture buffer',
    0x8868: 'off-screen picture buffer pointer', 0xA6CE: 'file read buffer pointer',
    0xA6DC: 'data drive mode', 0xA6F5: 'data disk in drive B flag', 0xA9AA: 'current file handle',
    0xA6E1: 'sound on flag', 0x0948: 'delay calibration', 0xA6D8: 'demo (1 running, 2 zoom to an encounter)',
    0x052F: 'key held (scan code, 0 none)', 0x0531: 'key pressed (scan code, latched)', 0xA694: 'current campaign number',
    0x0535: 'random value', 0x0537: 'random table pointer',
    0x886A: 'view top (line pairs)', 0x9E4A: 'cursor x (bytes)', 0x9E4C: 'cursor y (line pairs)',
    0x9E4E: 'cursor address', 0xA502: 'path memory write position', 0xA506: 'selected squad',
    0xA508: 'free squad slot', 0xA51A: 'tick counter', 0xA51C: 'time setting (64/16/4/1)',
    0xA5C0: 'text rows under the map', 0xA5C2: 'message rows', 0xA92F: 'play mode (0, 1 squad, 2 trace, 3 size, 4 info, 99)',
    0xAAED: 'key scan code', 0xAAE3: 'ticks (food every 16)', 0xAAE5: 'cursor on a squad',
    0xAAE7: 'last sighting region', 0xA8C1: 'trace half-speed toggle',
    0xA705: 'rule: villages supply food', 0xA707: 'rule: forts supply food', 0xA709: 'rule: supply line range',
    0xA70B: 'rule: forts train men', 0xA70D: 'rule: men shape', 0xA70F: 'rule: enemy seen',
    0xA711: 'rule: water', 0xA713: 'rule: mountain', 0xA715: 'rule: forest',
    0xA518: 'sighting message timer', 0xA701: 'enemy flags captured', 0xA703: 'flags taken by the enemy',
    0xA6FD: 'your men lost', 0xA6FF: 'enemy men lost', 0xA6F7: 'campaign result code',
    0xA4EA: 'opponent: surrender ratio', 0xA4FA: 'opponent: mountain factor', 0xA4FC: 'opponent: forest factor',
}

# per-squad arrays (20 entries) and other tables, annotated where their address is loaded
KNOWN_ARRAYS = {
    0x9782: 'path memory (1000 steps)', 0x9D82: 'campaign map (200 cells)', 0x9E60: 'squad side (-1 friend)',
    0x9E88: 'squad speed order', 0x9EB0: 'squad moving speed (0 stopped, 9999 no squad)', 0x9ED8: 'squad x',
    0x9F00: 'squad y', 0x9F28: 'squad formation', 0x9F3C: 'squad condition', 0x9F50: 'squad food',
    0x9FA0: 'squad archers', 0x9FB4: 'squad knights', 0x9FC8: 'squad barbarians', 0x9FDC: 'squad spies',
    0x9FF0: 'squad men', 0xA068: 'squad map address', 0xA0CC: 'squad step countdown', 0xA0F4: 'squad fighting',
    0xA11C: 'squad seen', 0xA144: 'squad path position', 0xA194: 'squad has a path', 0xA51E: 'squad mark address',
    0xA8D9: 'squad ticks per step', 0x8F74: 'supply map (200 cells)', 0x9104: 'supply map work area',
    0x9F64: 'flag x', 0x9F78: 'flag y', 0x9F8C: 'flag owner (-1 yours)', 0xA054: 'flag map address',
    0xA090: 'squad figure (1 normal, 2 spies only)',
}

# ------------------------------------------------------------------ helpers
STR = {}
for m in re.finditer(rb'[\x20-\x7e\r\n]{1,}', W.DSEG):
    if m.start() == 0 or W.DSEG[m.start() - 1] == 0:
        STR[m.start()] = m.group().decode()
FILENAMES = {k: v for k, v in STR.items()
             if re.fullmatch(r'[OPT][0-9A-Z]|[A-Z]?:?M|CNF|S|TE|WarW|B:M|A:M', v)}
LOADERS = {0x17ED, 0x8163, 0x7816, 0x260A, 0x267E}

THUNK_AT = {a: (k, ovl, tgt) for k, (a, ovl, tgt) in enumerate(W.THUNKS)}

calls_to = defaultdict(int)
for src, off, t in W.CALLS:
    calls_to[(W.resolve(src, t), t)] += 1


def name_of(region, off):
    if (region, off) in N:
        return N[(region, off)][0]
    return None


def callee_name(src_region, t):
    """Name for a near call target seen from src_region (all in segment 0000 unless root)."""
    if t in THUNK_AT:
        k, ovl, tgt = THUNK_AT[t]
        nm = name_of(f'ovl{ovl}', tgt) or f'ovl{ovl}_{tgt:04x}'
        return f'thunk_{k:02d}', f'-> overlay {ovl}: {nm}'
    if src_region == 'root':
        return (name_of('root', t) or f'root_{t:04x}'), None
    tgt = W.resolve(src_region, t)
    if tgt:
        return (name_of(tgt, t) or f'{tgt}_{t:04x}'), None
    return None, None


def func_label(r, off):
    return name_of(r.name, off) or f'{r.name}_{off:04x}'


def int_comment(r, offs, idx, ins):
    n = ins.operands[0].imm
    ah = None
    for b in range(1, 8):
        if idx - b < 0:
            break
        p = r.insns[offs[idx - b]]
        if p.mnemonic == 'mov' and p.op_str.startswith('ah,'):
            ah = p.operands[1].imm & 0xFF if p.operands[1].type == x86.X86_OP_IMM else None
            break
        if p.mnemonic == 'mov' and p.op_str.startswith('ax,') and p.operands[1].type == x86.X86_OP_IMM:
            ah = (p.operands[1].imm >> 8) & 0xFF
            break
    DOS = {0x02: 'display char', 0x09: 'print $-string', 0x0A: 'buffered input', 0x0F: 'FCB open',
           0x1A: 'set DTA', 0x25: 'set interrupt vector', 0x27: 'FCB random block read', 0x29: 'parse file name',
           0x30: 'get DOS version', 0x35: 'get interrupt vector', 0x3C: 'create file', 0x3D: 'open file',
           0x3E: 'close file', 0x3F: 'read file', 0x40: 'write file', 0x41: 'delete file', 0x42: 'seek',
           0x4A: 'resize memory', 0x4C: 'exit', 0xEF: 'vendor-specific call (loader checks BX bit 0)'}
    VID = {0x00: 'set video mode', 0x02: 'set cursor position', 0x0B: 'set CGA palette/border',
           0x0F: 'get video mode', 0x10: 'set palette register'}
    DSK = {0x00: 'reset disk', 0x02: 'read sectors', 0x03: 'write sectors'}
    KBD = {0x00: 'read key', 0x01: 'key available?'}
    table = {0x21: ('DOS', DOS), 0x10: ('BIOS video', VID), 0x13: ('BIOS disk', DSK), 0x16: ('BIOS keyboard', KBD)}
    if n == 0x11:
        return 'BIOS equipment list'
    if n in table:
        nm, t = table[n]
        if ah is None:
            return f'{nm}'
        return f'{nm} AH={ah:02X}h: {t.get(ah, "?")}'
    return None


def operand_comment(r, offs, idx, ins):
    c = []
    ops = ins.operands
    # immediates that point at data-segment strings / file names
    if r.name == 'root':
        return c
    if ins.mnemonic in ('mov', 'push') and ops and ops[-1].type == x86.X86_OP_IMM \
            and (ins.mnemonic == 'push' or (ops[0].type == x86.X86_OP_REG and ops[0].size == 2)):
        v = ops[-1].imm & 0xFFFF
        nxt_call = None
        for k in range(1, 4):
            if idx + k < len(offs):
                n = r.insns[offs[idx + k]]
                if n.mnemonic == 'call' and n.operands and n.operands[0].type == x86.X86_OP_IMM:
                    nxt_call = n.operands[0].imm & 0xFFFF
                    break
        if v in FILENAMES and (v >= 0x40 or nxt_call in LOADERS):
            c.append(f'file "{FILENAMES[v]}"')
        elif v in STR and v >= 0x40 and len(STR[v]) >= 3 and v not in FILENAMES \
                and (v >= 0x100 or nxt_call == 0x2565):
            s = STR[v].replace('\r', '\\r').replace('\n', '\\n')
            c.append(f'DS:{v:04X} "{s[:48]}"')
    # per-squad arrays
    if r.name.startswith('ovl6') and ins.mnemonic == 'mov' and len(ops) == 2 and ops[0].type == x86.X86_OP_REG \
            and ops[0].size == 2 and ops[1].type == x86.X86_OP_IMM and (ops[1].imm & 0xFFFF) in KNOWN_ARRAYS:
        c.append(KNOWN_ARRAYS[ops[1].imm & 0xFFFF])
    # known variables
    for o in ops:
        if o.type == x86.X86_OP_MEM and o.mem.base == 0 and o.mem.index == 0 and o.mem.segment == 0:
            v = o.mem.disp & 0xFFFF
            if v in KNOWN_VARS:
                c.append(f'[{v:04X}] {KNOWN_VARS[v]}')
    return c


def fmt_bytes(b):
    h = b.hex()
    return h if len(h) <= 14 else h[:12] + '..'


def ascii_of(b):
    return ''.join(chr(x) if 32 <= x < 127 else '.' for x in b)


# ------------------------------------------------------------------ output
out = []
P = out.append


def section(title, lines):
    P(';' + '=' * 78)
    P(f'; {title}')
    for l in lines:
        P(f'; {l}' if l else ';')
    P(';' + '=' * 78)
    P('')


def emit_code(r):
    offs = sorted(r.insns)
    reloc = {r.file_of(o) - W.HEADER_SIZE for o in offs}
    fstarts = sorted(r.func_starts)
    off = r.origin
    end = r.origin + r.size
    idx_of = {o: i for i, o in enumerate(offs)}
    while off < end:
        if off in r.insns:
            ins = r.insns[off]
            idx = idx_of[off]
            if off in r.func_starts:
                P('')
                lab = func_label(r, off)
                desc = N.get((r.name, off), (None, ''))[1]
                ncall = calls_to.get((r.name, off), 0)
                hdr = f'; ---- {lab}'
                if ncall:
                    hdr += f'   (called from {ncall} place{"s" if ncall > 1 else ""})'
                P(hdr)
                if desc:
                    P(f';      {desc}')
                P(f'{lab}:')
            if (r.name, off) in N and off not in r.func_starts:
                nm, desc = N[(r.name, off)]
                P(f'  ; >>> {nm}: {desc}')
            text = f'{ins.mnemonic:<6} {ins.op_str}'.rstrip()
            comments = []
            if ins.mnemonic == 'call' and ins.operands and ins.operands[0].type == x86.X86_OP_IMM:
                t = ins.operands[0].imm & 0xFFFF
                nm, extra = callee_name(r.name, t)
                if nm:
                    text = f'call   {nm}'
                    if extra:
                        comments.append(extra)
            if ins.mnemonic == 'lcall' and len(ins.operands) == 2:
                seg, t = ins.operands[0].imm & 0xFFFF, ins.operands[1].imm & 0xFFFF
                if seg == 0x028D:
                    text = f'lcall  028D:{t:04X}'
                    comments.append(name_of('root', t) or f'root_{t:04x}')
                elif seg == 0x0000:
                    text = f'lcall  0000:{t:04X}'
                    comments.append(name_of('rt', t) or f'rt_{t:04x}')
            if ins.mnemonic == 'int':
                c = int_comment(r, offs, idx, ins)
                if c:
                    comments.append(c)
            comments += operand_comment(r, offs, idx, ins)
            img = r.file_of(off) - W.HEADER_SIZE
            if any((img + k) in W.RELOC_IMG for k in range(ins.size)):
                comments.append('segment fixup')
            line = f'  {r.name}:{off:04X}  {r.file_of(off):05X}  {fmt_bytes(ins.bytes):<14} {text}'
            if comments:
                line = f'{line:<70} ; ' + '; '.join(comments)
            P(line)
            off += ins.size
        else:
            # data gap: up to next instruction
            nxt = min([o for o in offs if o > off] + [end])
            P(f'  ; ---- data / unreached bytes ({nxt - off} bytes)')
            while off < nxt:
                n = min(16, nxt - off)
                b = r.data[off - r.origin:off - r.origin + n]
                P(f'  {r.name}:{off:04X}  {r.file_of(off):05X}  db {", ".join(f"{x:02X}h" for x in b):<95} ; {ascii_of(b)}')
                off += n


def emit_data(r, seg_label):
    data = r.data
    off = 0
    while off < len(data):
        a = r.origin + off
        if a in STR and len(STR[a]) >= 3 and (a == 0 or data[off - 1] == 0 or r.name == 'ldat'):
            s = STR[a] if r.name == 'dseg' else None
        else:
            s = None
        if r.name == 'dseg' and a in STR and (len(STR[a]) >= 3 or a in FILENAMES):
            s = STR[a]
            raw = data[off:off + len(s.encode()) + 1]
            esc = s.replace('\r', '",0Dh,"').replace('\n', '",0Ah,"')
            note = ''
            if a in FILENAMES:
                note = '  ; file name'
            if s.startswith('DPQZSJHIU'):
                note = '  ; "COPYRIGHT 1983 DAVE MURRY" with every character +1'
            P(f'  {seg_label}:{a:04X}  {r.file_of(a):05X}  db "{esc}", 0{note}')
            off += len(raw)
            continue
        n = 16
        # stop the hex row at the next string start
        for k in range(1, 16):
            if (a + k) in STR and r.name == 'dseg' and (len(STR[a + k]) >= 3 or (a + k) in FILENAMES):
                n = k
                break
        b = data[off:off + n]
        P(f'  {seg_label}:{a:04X}  {r.file_of(a):05X}  db {", ".join(f"{x:02X}h" for x in b):<95} ; {ascii_of(b)}')
        off += n


# ------------------------------------------------------------------ header
mz = W.HDR
img_size = (mz[2] - 1) * 512 + mz[1] - W.HEADER_SIZE if mz[1] else mz[2] * 512 - W.HEADER_SIZE
P(';' + '#' * 78)
P('; WAR.EXE - The Ancient Art of War, CGA version (Broderbund; game (c) 1984 Dave and Barry Murry)')
P('; From the 3.5" disk release (files dated 22 June 1989). Program copyright string: 1987.')
P(';')
P('; Annotated disassembly, generated by recursive descent from the program entry point,')
P('; the overlay thunk table and every call target, so code and data are separated.')
P('; Every code region is shown at the address it really runs at.')
P(';')
P('; Line format:   region:offset  file-offset  bytes  instruction   ; comment')
P(';   region  rt = resident code (segment 0000), root = startup/loader (segment 028D),')
P(';           ovl1..ovl7 = overlays (segment 0000, at their load offset), DS = data segment')
P(';   Names in the form ovl3_4a2f / rt_1330 are unnamed functions; named ones are listed below.')
P(';' + '#' * 78)
P('')
section('MZ HEADER', [
    f'file size          {len(W.D)} bytes',
    f'header             {W.HEADER_SIZE:#x} bytes, {mz[3]} relocations',
    f'load image         {img_size:#x} bytes declared (file 0x200-{W.HEADER_SIZE + img_size:#x})',
    f'                   the overlays and data segment are stored after it, inside WAR.EXE',
    f'entry CS:IP        {mz[11]:04X}:{mz[10]:04X}  (file {W.HEADER_SIZE + mz[11] * 16 + mz[10]:#x})',
    f'initial SS:SP      {mz[7]:04X}:{mz[8]:04X}',
    'linker             Phoenix Software PLINK86 (overlay loader "LINK86 Overlay Loader")',
    'file 0x17E20-end   zero padding',
])
section('MEMORY MAP (segments relative to the load address)', [
    '0000:0000-28CF  rt     resident C code and library: file I/O, text, graphics, sound, game core',
    '028D:0000-051D  root   startup and Phoenix overlay loader (entry 028D:02FA)',
    '02DE:000E-017F  ldat   loader variables, PATH buffer, $-terminated loader messages',
    '02F6:0000-008D  table  overlay descriptor table (16-byte entries, see below)',
    '02F6:008E-0189  thunks 21 overlay call thunks, called as near calls 0000:2FEE + 12*n',
    '0000:30F0-851F  ovl1   overlay area: overlay 1, or one of overlays 3-7 (all load at 30F0h)',
    '0000:8520-8AFF  ovl2   overlay 2 (loads after overlay 1, so both can be resident)',
    f'{W.DS_SEG:04X}:0000-0B3F  DS     data segment DGROUP (initialised data, preloaded at startup)',
    f'{W.DS_SEG:04X}:0B40-....  DS     BSS, buffers, stack (SS=1381h); DGROUP ends at segment 1391h',
    '',
    'All game code (rt + overlays) runs with CS = load segment, so near calls between them work.',
    'Data references [xxxx] in this code are offsets in DS (segment 08C6h).',
])
rows = ['#  file offset  size    loads at       flags  role']
for o in W.OVERLAYS:
    role = OVERLAY_ROLE.get(o['index'], 'DGROUP initialised data (flag 4000h = load at startup)')
    rows.append(f'{o["index"]}  {o["file_para"] * 16:#07x}     {o["size_para"] * 16:#06x}  '
                f'{o["load_seg"]:04X}-{o["load_end"]:04X}      {o["flags"]:04X}   {role}')
section('OVERLAY TABLE (file 0x3160, segment 02F6)', rows + [
    '',
    'Entry layout (16 bytes, fields shared with the next entry): +4 flags (FFFFh ends the table,',
    '4000h preload), +8 load segment, +A end segment, +C file-name pointer (WAR.EXE),',
    '+E file position in paragraphs; the size in paragraphs is at +2 of the next entry.',
])
trows = []
for k, (a, ovl, tgt) in enumerate(W.THUNKS):
    nm = name_of(f'ovl{ovl}', tgt) or f'ovl{ovl}_{tgt:04x}'
    trows.append(f'thunk_{k:02d}  0000:{a:04X}  -> overlay {ovl} at {tgt:04X}  {nm}')
section('OVERLAY THUNKS (each: lcall 028D:02D0 / dw overlay# / jmp far target)', trows)
section('DATA FILES USED BY WAR.EXE (names are strings in DS)', [
    'M         72 map tiles (32x20, CGA 2 bits/pixel); also used to check the WAR disk is present',
    '          (check_war_disk tries "B:M", "A:M", "M")',
    'CNF       saved options (load_file in ovl1_startup, save_options writes it)',
    'PM        title screen. PI title with barbarian. PJ opponents. PK Options Shoppe.',
    'PE, PF    parchment scrolls. PG, PH scene sheets for the menus. PN/PO battle report win/lose.',
    '          All loaded with load_picture_rle8 (overlay 1).',
    'P0-PD     battle backgrounds, chosen from the terrain (overlay 7, choose_battle_background).',
    'PL        formation editor background (overlay 5). Same word-RLE format as P0-PD.',
    'O0-O8     sprites and text for each module: O0 map maker (ovl4), O1 squads editor (ovl3),',
    '          O2 opponent bios, O3 story/rules/title steps, O4 menus (ovl1), O5 campaign',
    '          messages (ovl6), O6/O7 battle (ovl7), O8 formation editor (ovl5)',
    'T0-TA     campaigns, TITLES/DTITLES campaign lists (names built at run time)',
    'S         saved game (data disk, via make_data_path)',
    'TE        file used when preparing a data disk',
    'WarW      encrypted word list for the manual copy protection (overlay 2)',
])
section('PICTURE FORMATS (decoded here)', [
    'Word RLE (P0-PD, PL), decode_picture_rle16: two fields (even lines, then odd lines);',
    '  each field = word N, then N x [count byte, data word]; each field = 8000 bytes.',
    'Byte RLE (PE-PO), load_picture_rle8: pairs of [count, byte]; the pairs alternate',
    '  between the even-line field and the odd-line field until each has 8000 bytes.',
    'Both decode into an off-screen buffer (even lines at +0, odd lines at +3E80h),',
    '  then blit_buffer_to_cga copies it to video memory at B800h.',
])
section('COPY PROTECTION', [
    '1. key_disk_check (overlay 1): reads track 10 of the original floppy with INT 13h,',
    '   including sector 241, which must fail with a CRC error, and compares 16 words.',
    '2. copy_protection (overlay 2): loads WarW and asks "On page N, what is word N in',
    '   paragraph N" from the manual.',
    'The string "DPQZSJHIU 2:94 EBWF NVSSZ" is "COPYRIGHT 1983 DAVE MURRY" with each',
    'character +1.',
])
section('HARDWARE', [
    'Video: CGA 320x200 mode 4 (detect_video also checks for Hercules). Palette set with',
    '       INT 10h AH=0Bh; AH=10h palette registers for PCjr/Tandy.',
    'Sound: PC speaker via PIT channel 2 (speaker_freq/on/off) and the Tandy/PCjr SN76489',
    '       chip at port C0h (tandy_sound).',
    'Input: keyboard (INT 16h), joystick (game port 201h, detect_joystick).',
])
named = sorted(N.items(), key=lambda kv: (['rt', 'root', 'ovl1', 'ovl2', 'ovl3', 'ovl4', 'ovl5', 'ovl6', 'ovl7'].index(kv[0][0]), kv[0][1]))
section('NAMED FUNCTIONS', [f'{rn}:{off:04X}  {nm:<26} {desc[:80]}' for (rn, off), (nm, desc) in named])
section('KNOWN VARIABLES (DS offsets)', [f'[{k:04X}]  {v}' for k, v in sorted(KNOWN_VARS.items())])

# ------------------------------------------------------------------ body
for r in W.REGIONS:
    if r.name == 'ldat':
        section(r.title + f'  - file {r.file_start:#x}-{r.file_start + r.size:#x}', [r.note])
        emit_data(r, '02DE')
        P('')
        # overlay table + thunks as data
        section('OVERLAY TABLE AND THUNKS (segment 02F6)  - file 0x3160-0x32F0', [
            'See the decoded tables in the header.'])
        tbl = W.Region('tbl', 'data', 0x3160, 0x190, 0x02F6, 0x0000, '')
        emit_data(tbl, '02F6')
        P('')
        continue
    if r.kind == 'code':
        lines = [f'file {r.file_start:#x}-{r.file_start + r.size:#x} ({r.size} bytes)']
        if r.name.startswith('ovl'):
            lines.append(OVERLAY_ROLE[int(r.name[3:])])
        if r.note:
            lines.append(r.note)
        section(r.title, lines)
        emit_code(r)
        P('')
    else:
        section(r.title + f'  - file {r.file_start:#x}-{r.file_start + r.size:#x}', [r.note])
        emit_data(r, 'DS')
        P('')

with open(OUT, 'w') as f:
    f.write('\n'.join(out) + '\n')
print('wrote', OUT, len(out), 'lines')
