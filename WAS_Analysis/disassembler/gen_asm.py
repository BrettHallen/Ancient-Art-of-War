"""Generate the annotated WAS.EXE.asm from the wasdis model.

Usage:  python3 gen_asm.py [output]     (default: ../WAS.EXE.asm)
Needs capstone (pip install capstone).
Adapted from WAR_Analysis/disassembler/gen_asm.py (Nov 2026). Add names to N as routines
are understood, then run it again.
"""
import os
import sys
import re
import bisect
from collections import defaultdict, Counter
HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import wasdis as W
from capstone import x86

OUT = sys.argv[1] if len(sys.argv) > 1 else os.path.join(HERE, '..', 'WAS.EXE.asm')

# ------------------------------------------------------------------ names
# (region, offset): (name, description)
N = {
    # --- named from WAS.EXE itself (Nov 2026)
    ('rt', 0x0532): ('load_sm_tiles', 'Load SM (94 map tiles)'),
    ('rt', 0x30AE): ('set_cga_palette_if', 'set_cga_palette(n) unless [0ECB] = 1: palette 0 for the scrolls (scroll_palette) and flashes in the sea battle (ovl5), 1 otherwise'),
    ('rt', 0x336D): ('read_joystick', 'Read the game port (201h) with PIT timing'),
    ('ovl1', 0x7650): ('ovl1_startup', 'thunk 0. Startup: SCNF options (29 bytes to DS:684C), title picture SPM'),
    ('ovl1', 0x7AF8): ('load_so2', 'Load SO2 (opponents, quotes, tunes) to [3F37]+5DC0h'),
    ('ovl1', 0x7B29): ('save_game', 'thunk 4. Alt+S: write SS (the saved game) to the data disk'),
    ('ovl1', 0x7D39): ('load_saved_game', 'Read SS from the data disk: 28CCh bytes of DS from 3F39h'),
    ('ovl1', 0x86D0): ('show_title_ship', 'Title page with ship (SPI)'),
    ('ovl1', 0x8BED): ('choose_opponent', 'thunk 19. The opponents (SPJ, SO2 bios)'),
    ('ovl1', 0x9704): ('battle_report', 'thunk 5. Battle report: SPN, SPO, SPP or SPQ'),
    ('ovl1', 0x9BCB): ('options_shoppe', 'thunk 13. Ye Olde Options Shoppe (SPK)'),
    ('ovl1', 0xA7DA): ('opening', 'thunk 12. Opening: AD (the face), the opening tune (SO2 447h/48Bh) and text (SO2 13FBh)'),
    ('ovl1', 0xC388): ('load_campaign', 'load_campaign(n): read S<n>: story 348h -> DS:4E53, map C8h -> 5BFF, ships 1E0h -> 5CDD, opponent 2 -> 646F, rules 12h -> 64AD, names 258h -> 609F, 28h more -> 6025 (only in 2220-byte files)'),
    ('ovl1', 0xC4D8): ('ship_map_addresses', 'For the 40 ships: map buffer address (y+1)*80 + x'),
    ('ovl1', 0xCBEA): ('load_picture', 'load_picture(name, dest): two fields of word N + N codes (<=200 run, >200 literal), even lines at dest, odd at dest+3E80h'),
    ('ovl7', 0xCBC8): ('boarding_load_b', 'Boarding: load the deck picture B into [3F37], set up the 40 men (crew sizes from the ships\' damage and types), the men\'s pictures DS:6D5A'),
    # --- the sea battle and boarding (step 4 of the remake, 13 Nov 2026)
    ('ovl5', 0x7650): ('sea_battle', "thunk 3. The sea battle (Zoom or Practice): ovl6_c9b0 setup, then the loop: crew animation, battle_input, particles; every [10AA+time] iterations the orders and aim (rt_34bf), the enemy's broadsides, the end check, sailing, speeds; battle_end unless Practice"),
    ('ovl5', 0x7AC6): ('battle_input', "The battle's keys: G C S O A F V T P L, Space, + / -, the view's scrolling; returns 1 to end"),
    ('ovl5', 0x87E5): ('battle_read_key', 'A key (or the joystick, ovl5_886f)'),
    ('ovl5', 0x8D89): ('build_shore', "The shore [686B] for the battle's map: run-length rows at the shore buffer"),
    ('ovl5', 0x936D): ('fire_guns', 'A broadside: the shots, the hits (abb6), the explosions (a7ad)'),
    ('ovl5', 0xA7AD): ('explosion', 'An explosion at a hit ship'),
    ('ovl5', 0xAB6E): ('tone_bursts', "The guns' sound"),
    ('ovl5', 0xABB6): ('shot_hits', 'Which ship a shot hits: sails or hull damage, struck colours, sinking'),
    ('ovl5', 0xAFC6): ('draw_ships', 'Draw the six ships in the overview or the close view'),
    ('ovl5', 0xB4C5): ('sail_close', 'Sailing in the close view'),
    ('ovl5', 0xB841): ('sail_overview', 'Sailing in the overview'),
    ('ovl5', 0xBC12): ('collisions', 'Ships touching: aground, grappling ("X grapples Y. Zoom Pass": Z boards, ovl7 boarding), aid / capture, collisions'),
    ('ovl6', 0xC9B0): ('battle_setup', 'Load SO6 (DS:4AF5), SO7 (DS:4257), SO8; the pointer tables; the ships from the two squadrons'),
    ('ovl6', 0xD5AA): ('battle_end', 'The results back to the map: ships captured and lost, "Leaving the battle.", "No losses on either side."'),
    ('ovl6', 0xDDC7): ('battle_panel', "The top panel: Speed, Gun, Fire, the ship's type and name, Orders, the compass"),
    ('ovl6', 0xDF4B): ('speed_icon', 'The mast and sails icon with the wind'),
    ('ovl6', 0xDFE2): ('compass', 'The compass: heading and course'),
    ('ovl6', 0xE15D): ('battle_commands', 'Row 24: Time (...)  View  Pause  Leave'),
    ('ovl6', 0xE204): ('ship_name', "The selected ship's name, centred"),
    ('ovl6', 0xE31E): ('time_text', 'Time (faster / fast / medium / slow / slower)'),
    ('ovl6', 0xE3E3): ('order_text', "Orders- and the order's text (SO6, tables DS:18ED, 18F9)"),
    ('ovl6', 0xE4AD): ('erase_ship', 'Erase a ship in the view'),
    ('ovl6', 0xE651): ('show_ship', 'Another of your ships in the panel'),
    ('ovl6', 0xE7F1): ('crew_anim', "The gun crew's animation: 10-byte script entries"),
    ('ovl6', 0xED60): ('crew_panel', "The gun crew's panel"),
    ('ovl6', 0xF0D2): ('ship_speeds', "The ships' speeds from sails, damage and wind"),
    ('ovl6', 0xF4B5): ('particles', 'The shot and splash particles'),
    ('ovl6', 0xF898): ('battle_message', 'A message on row 23: text, name, text, name'),
    ('ovl6', 0xF9CE): ('battle_sprites', "The ships' pictures for the view: overview or close (SO8)"),
    ('ovl6', 0xFB95): ('battle_background', 'The sea and shore of the view'),
    ('ovl7', 0xC9B0): ('boarding', 'Boarding (a0 the ship grappled, a1 the other): the loop unpack_deck, boarding_frame, deck_to_screen, every sixth frame boarding_enemy_orders, boarding_keys; returns the winner (-1: Leave or B not found)'),
    ('ovl7', 0xD444): ('boarding_commands', 'Row 24: Attack Leave, or the parts of the deck to send men from or to, Cross, Again'),
    ('ovl7', 0xD579): ('boarding_keys', 'A L before the fight; F Q P R C A Esc Backspace in it; the opponent 2 or 4 may attack first'),
    ('ovl7', 0xD8BF): ('boarding_key', 'A key (the joystick: ovl7_d8eb); none in the demo'),
    ('ovl7', 0xD9A2): ('boarding_from', '"FROM "'),
    ('ovl7', 0xD9CC): ('boarding_to', '" TO "'),
    ('ovl7', 0xD9F0): ('boarding_send', 'Send one of your men FROM -> TO'),
    ('ovl7', 0xDA69): ('boarding_demo', "The demo's orders"),
    ('ovl7', 0xDAE0): ('boarding_pick', 'One of your men in the FROM part, one without an opponent first'),
    ('ovl7', 0xDBAC): ('boarding_enemy_orders', "The enemy's orders"),
    ('ovl7', 0xDCD7): ('boarding_enemy_pick', "One of the enemy's men to send to a part"),
    ('ovl7', 0xDE29): ('boarding_enemy_cross', "All the enemy's men in a part go across"),
    ('ovl7', 0xDEF1): ('boarding_frame', 'Move and draw every man (scripts, opponents, walking, blows), then the flags'),
    ('ovl7', 0xE651): ('man_step', "The next entry of a man's script [step, picture, state]: move, draw; an officer's shot"),
    ('ovl7', 0xE926): ('officer', 'An officer: stand, wave, or fire at the nearest man'),
    ('ovl7', 0xEC05): ('man_walk', "To his part of the deck: ec31 yours, ee50 the enemy's"),
    ('ovl7', 0xEC31): ('man_walk_yours', "Your man to his part (boxes DS:193B); at the enemy's flag: you win"),
    ('ovl7', 0xEE50): ('man_walk_enemy', "The enemy's man to his part (boxes DS:1963); at your flag: you lose"),
    ('ovl7', 0xF06F): ('walk_script', 'The walking script for a step dx, dy; 0 when arrived (the counts updated)'),
    ('ovl7', 0xF17A): ('step_aside', 'Step off another man'),
    ('ovl7', 0xF221): ('keep_distance', 'Keep a few bytes from the opponent'),
    ('ovl7', 0xF327): ('man_overlaps', 'Another man stands at the same place'),
    ('ovl7', 0xF3A9): ('man_opponent', '[64D7]: the opponent if within 5 bytes and 2 lines, -2 if further, else nearest_opponent'),
    ('ovl7', 0xF484): ('nearest_opponent', 'The nearest man of the other side he may fight'),
    ('ovl7', 0xF6E5): ('toward_opponent', 'The step toward the opponent: +-160, +-1'),
    ('ovl7', 0xF86F): ('man_struck', '[64D7] is struck down; the end if a side has nobody left'),
    ('ovl7', 0xFA94): ('boarding_lost', 'You lost: [6DC1] = 2, the last frame, the tune'),
    ('ovl7', 0xFB07): ('boarding_won', 'You won: [6DC1] = 1, the last frame, the tune'),
    ('ovl7', 0xFB5A): ('boarding_last_frame', 'All men stop; 30 frames to go; a frame'),
    ('ovl7', 0xFBB3): ('count_men', 'Count the men in each part of the deck (DS:6DC7, the enemy +5)'),
    ('ovl7', 0xFC5E): ('sort_men', 'One pass sorting the drawing order by y, the dead first'),
    ('ovl7', 0xFD25): ('forget_man', 'Nobody fights man n any more'),
    ('ovl7', 0xFD62): ('grappled_to', 'Is a ship grappled to ship n'),
    ('ovl7', 0xFD94): ('slide_tune', 'A tune: notes slid to from 3000, and their lengths'),
    ('ovl7', 0xFE6B): ('unpack_deck', "Unpack B (at [3F37]) into the deck's two fields: load_picture's format"),
    ('ovl7', 0xFEC9): ('deck_to_screen', 'Copy the fields to screen rows, a line from each in turn'),
    ('ovl7', 0xFF14): ('draw_man', "A man's picture into both fields: your men AND NOT / OR, the enemy's OR / AND NOT"),
    ('rt', 0x0688): ('mark_flagship_squadrons', 'For each squadron: DS:5EBD = 1 if one of its ships (DS:6456 list) is type 3 (flag), 0 otherwise; merchants (2) are left alone'),
    ('rt', 0x074B): ('squadron_info', 'Info: each ship of the squadron: name (DS:609F + 15n), LINE / FRIGATE / FLAG from DS:5E95 (1, 2, else), sails bar 4 x (5 - DS:5E6D), hull bar 4 x (5 - DS:6025); the supplies bar DS:5E1D'),
    ('rt', 0x10D2): ('draw_squadrons', 'Draw the 40 squadrons: friend always, foe if seen or rule [64B7] = 80; crossed swords if in an encounter [56FF], anchor if stopped, else the heading sprite DS:607F[DS:5FD5] (DS:075A: one line pair lower for some headings); pennant (DS:5EBD = 1) or cargo (= 2) one line pair above'),
    ('rt', 0x2941): ('draw_map_sprite', 'draw_map_sprite(sprite, address, side): byte width, byte height in line pairs, then even/odd lines; friend: white figure, black outline (within each byte); foe: the reverse'),
    ('rt', 0x2B2A): ('draw_bar', 'Info bars: draw_bar(x, y, length, pattern)'),
    ('ovl4', 0xB35E): ('init_heading_sprites', 'DS:607F[16] = the 16 heading sprites at DS:04BF-059D'),
    ('ovl1', 0x7F1A): ('show_scroll', 'thunk 15. The scrolls SPE / SPF, shown in CGA palette 0 (scroll_palette)'),
    ('ovl1', 0xBC67): ('scroll_palette', 'If [684D] (an SCNF option, 1 in the shipped SCNF): CGA palette 0 for the scrolls'),
    ('ovl1', 0xA9A9): ('key_disk', 'Copy protection: reads the original floppy with key_disk_check and looks for the copyright string DS:0BD9; 0 makes startup fail (ovl1:76B3). The DOSBox harness patches 76B3 jne -> jmp'),
    ('rt', 0x6B04): ('switch_helper', 'The C switch: the caller is followed by word lowest case, word count, word default, then the targets'),
    ('rt', 0x0630): ('list_ships', 'list_ships(sq): the (up to 3) ships whose squadron is sq into DS:6456, -1 after the last'),
    ('rt', 0x0AC7): ('show_speed', 'Info: "SPEED: " Anchored / Half Sail / Full Sail at row 16, for a squadron the player commands'),
    ('rt', 0x0B3D): ('commandable', 'commandable(sq): DS:5EE5 = -1, or side -1 and DS:5EE5 != 0'),
    ('rt', 0x0C72): ('buzz', 'The buzz as the marker comes onto a squadron: divisor 1F40h for 600 delay units'),
    ('rt', 0x1217): ('is_scroll_up_key', 'Scan code is -, keypad - or Tab'),
    ('rt', 0x126A): ('is_scroll_down_key', 'Scan code is =, keypad + or Ctrl'),
    ('rt', 0x1985): ('squadron_cell', 'squadron_cell(sq): ((y+2)/10)*10 + (x>>3), the 10x20 map cell'),
    ('rt', 0x280E): ('draw_cursor', 'The marker at [5CCB]: a cross, or a box while tracing ([65AA] = 2)'),
    ('rt', 0x2B71): ('draw_vline', 'draw_vline(line, xbyte, n): 2n lines of byte 0Fh'),
    ('rt', 0x2E04): ('clear_info_box', 'Clear lines 62-135, bytes 5-74 (the Info panel)'),
    ('ovl1', 0x7753): ('game_init', 'thunk 1. The title page, then a new game: view at the bottom (3F3B = 109), marker 38,145, squadrons reset (5EE5 = -2), time slow (6312 = 64), the wind (6314), flagships'),
    ('ovl4', 0x7650): ('campaign_play', 'thunk 2. Campaign play (as WAR.EXE ovl6:3197): load SM and SO5, then the loop: blit, ticks every [6312] iterations (8BCD, 96A0, 9345 for two squadrons), campaign_input'),
    ('ovl4', 0x7B65): ('campaign_input', 'Keys in campaign play (as WAR.EXE ovl6:3B87): S Surrender, T Time, F Fleet, C Clear, P Pause; over a squadron I Info (S speed, E End), D Detach, J Join, Z Zoom, P Practice, M Move; the marker and scrolling'),
    ('ovl4', 0x85F8): ('key_waiting', 'Is a key waiting (any key)?'),
    ('ovl4', 0x8645): ('poll_input', '[6896] = the latched key, then [804] = the key held down (a held key repeats every iteration)'),
    ('ovl4', 0xA202): ('check_cursor_over_squadron', 'Is the marker over a squadron (its byte column, 2 line pairs above to 2 below (y+1))? buzz and select_squad, or the normal command line'),
    ('ovl4', 0xAA1C): ('show_command_line', 'Time (slow)  Fleet  Pause  Surrender'),
    ('ovl4', 0xAB00): ('show_clear_or_pause', '"Clear" if there are messages, else "Pause"'),
    ('ovl4', 0xAB82): ('squadron_command_line', 'Move Info Detach Join Zoom Practice'),
    ('ovl4', 0xAD63): ('show_fleet', 'F: Friend: n Flag, n Line, n Frigate. / Foe: ... (rows 21-22), End'),
    ('ovl4', 0xB252): ('join_partner', 'A squadron of the same side within 3 bytes and 7 line pairs (index+1, or 0); not for a squadron of three ships ([6458] = -1)'),
    ('ovl4', 0xC268): ('port_of_cell', 'The port next to a map cell (-1 none): "In Port" in Info'),
    ('ovl4', 0xEF92): ('count_fleet', 'Count flagships, ships of the line and frigates on each side (DS:64BF-64C9); sets the pennants'),
    # --- the rules (step 3, Nov 2026)
    ('rt', 0x0B90): ('debug_hook', 'Does something only with the debug flags DS:081C-0820 set'),
    ('rt', 0x0F55): ('clear_paths', 'clear_paths(sq): zero its path in the ring DS:4E53 from [579F] (and route at 542F from [57EF]); destination 6316 = 9999; speed 0'),
    ('rt', 0x104A): ('erase_squadrons', 'Put back what was under the marker, the squadrons and the destination marks'),
    ('rt', 0x1084): ('redraw_squadrons', 'Destination marks (2A63/2AB8), then draw_squadrons and the marker'),
    ('rt', 0x12BD): ('tile_is_sea', 'Tiles 0-2, 7, 12, 13, 19, 37-39, 42-45, 48-50, 78-81, 83 and up'),
    ('rt', 0x15C4): ('tile_is_rough', 'Tiles 10-12, 16-18 (dark water: frigates may be lost, rule 64B3)'),
    ('rt', 0x1656): ('tile_is_shallow', 'Tiles 7-9, 13-15 (light water: ships of the line may be lost, rule 64B9)'),
    ('rt', 0x16E8): ('coast_a', 'Tiles 8, 10, 31-35: shore type 371h'),
    ('rt', 0x176C): ('coast_b', 'Tiles 9, 11, 25-28: shore type 372h'),
    ('rt', 0x17F0): ('coast_c', 'Tiles 14, 16, 23, 24, 29, 30: shore type 373h'),
    ('rt', 0x18AC): ('coast_d', 'Tiles 15, 17, 19-22, 36: shore type 374h'),
    ('rt', 0x1953): ('map_has_tile', 'Is the tile anywhere on the map?'),
    ('rt', 0x19D6): ('supply_port_beside', 'supply_port_beside(cell, most): tile 75 in the four directions (on the map, rt_1c0c clear), with the least (most=0) or most supplies'),
    ('rt', 0x1AA8): ('repair_port_beside', 'repair_port_beside(cell, most): the same for tile 74'),
    ('rt', 0x1B7D): ('crown_beside', 'crown_beside(cell, side): tile 76 (the enemy crown) for side -1, 77 for side 0, in the four directions'),
    ('rt', 0x1C0C): ('coast_blocks', 'coast_blocks(cell, dir): the cell\'s shore (425D 371h-374h) does not face dir all along (table DS:0796)'),
    ('rt', 0x1C52): ('off_map', 'off_map(cell, col, dir): the neighbour in dir (1 E, 2 SE ... 8 NE; DS:0784) is off the 10x20 map'),
    ('rt', 0x1E10): ('way_blocked', 'way_blocked(dir, cell, col): no way by sea from cell in dir (edges, shores)'),
    ('rt', 0x1EF4): ('coast_pass', 'coast_pass(dir, cell): DS:0796 row of the cell\'s shore type: nonzero if it cannot be entered from dir'),
    ('rt', 0x1F2D): ('place_detached', 'A squadron made by Detach: beside the old one, a byte east or west, or two line pairs north or south, where there is sea'),
    ('rt', 0x2523): ('coast_init', 'The shore type of each cell into DS:425D (371h-379h, 0 sea); 65B2/65B4 (160 halved per repair/supply port)'),
    ('rt', 0x28C8): ('save_squadron_bgs', 'Save what is under each squadron (10 line pairs of 2 bytes, DS:58DF)'),
    ('rt', 0x28FE): ('restore_squadron_bgs', 'Put it back'),
    ('rt', 0x2A2E): ('draw_tile', 'draw_tile(src, dst): a 32x20 tile into the map buffer (the wind\'s face)'),
    ('rt', 0x2A63): ('save_dest_bg', 'Save what is under a squadron\'s destination mark'),
    ('rt', 0x2A8F): ('restore_dest_bg', 'Put it back'),
    ('rt', 0x2AB8): ('draw_dest_mark', 'A little cross at [6316]: OR 3Ch, FFh, OR 3Ch on three line pairs'),
    ('rt', 0x2CC4): ('draw_icon', 'draw_icon(src, line, xbyte): byte width, byte height/2, rows (the Enter, M, + and - icons)'),
    ('rt', 0x2DC8): ('invert_block', 'invert_block(addr, width, height/2): XOR FFh on the screen (Detach\'s mark)'),
    ('rt', 0x2E26): ('clear_detach_panel', 'Clear lines 60-107, bytes 11-70'),
    ('rt', 0x2E48): ('buf_byte_is', 'buf_byte_is(addr, value): the map buffer byte (44h, 11h: land)'),
    ('rt', 0x2E61): ('invert_tile_buf', 'XOR a 32x20 tile in the map buffer (a port with nothing left)'),
    ('rt', 0x30E2): ('beep_message', 'A 1000 divisor beep for 546h delay units'),
    ('rt', 0x6AD0): ('sparse_switch', 'The C switch on scattered values: word count, the values, then count+1 targets (the default first)'),
    ('ovl1', 0x7A3F): ('promote_flagship', 'promote_flagship(side): a side with no flagship and no crown: one of its ships becomes one (frigates only after a while)'),
    ('ovl4', 0x8A2C): ('cancel_path', 'The selected squadron stops; its path is cleared'),
    ('ovl4', 0x8A75): ('end_trace', 'M while tracing: 99 ends the path, full sail, the destination mark'),
    ('ovl4', 0x8BCD): ('encounters', 'Every tick: your squadrons and merchants against the enemy\'s and merchants: encounters (C5B9), merchants met (BB29) or taken (BACD), sighting (rule 64B7)'),
    ('ovl4', 0x8EDE): ('map_battles', 'Every iteration: encounters count up to the fight delay [6851], then are fought out on the map (9154)'),
    ('ovl4', 0x910F): ('enemy_count', 'The enemy\'s squadrons'),
    ('ovl4', 0x9154): ('battle_hit', 'battle_hit(sq, way): a ship of sq may be damaged; 1 when its last ship is lost'),
    ('ovl4', 0x92B7): ('count_loss', 'count_loss(ship, sq, way): 64A5/64A9 yours, 64A7/64AB theirs (way 0: by 5EE5 = -1)'),
    ('ovl4', 0x9345): ('eat', 'eat(sq): supplies used (rule 64B5 or opponent 6059), hunger damage, and an anchored squadron\'s supplies and repairs from a port or crown'),
    ('ovl4', 0x96A0): ('move_squadrons', 'Every tick: squadrons on paths step when their countdown runs out; the marker follows the selected one'),
    ('ovl4', 0x9A3C): ('step_heading', 'The heading (0-15) of a path step: -80 0, -79 3, 1 4, 81 5, 80 8, 79 11, -1 12, -81 13'),
    ('ovl4', 0x9AB2): ('set_move_delays', 'Ticks per step: 3 - speed, doubled for ships of the line, torn sails, the wind (DS:0ECD); losses in rough and shallow water'),
    ('ovl4', 0x9D7E): ('lost_at_sea', 'A ship lost in rough or shallow water'),
    ('ovl4', 0x9E4F): ('end_check', 'After a loss: the counts, the opponent\'s values, crown, last ship, last flagship, the offer to surrender'),
    ('ovl4', 0xA129): ('game_over', 'You Lost (1) / You Won blinks until a key; all squadrons shown'),
    ('ovl4', 0xA347): ('detach_join', 'D or J: the two squadrons\' ships side by side; + and - mark one, S switches it, E ends'),
    ('ovl4', 0xA952): ('detach_names', 'The two columns of ship names'),
    ('ovl4', 0xACDA): ('trace_prompt', 'Trace path to destination and press [M]; the marker becomes a box'),
    ('ovl4', 0xAEAE): ('cell_mid_x', 'The middle byte of a cell, with the shore\'s slant'),
    ('ovl4', 0xAF08): ('cell_mid_y', 'The middle line pair of a cell, with the shore\'s slant'),
    ('ovl4', 0xB3E5): ('trace_blocked', 'May the tracing box move this way? 77 if not (land tiles, land bytes in the buffer, the shore\'s slant)'),
    ('ovl4', 0xB590): ('msg_sunk', 'name / ENEMY FRIGATE / ENEMY SHIP SUNK IN ROUGH WATERS / LOST IN SHALLOW WATER'),
    ('ovl4', 0xB612): ('msg_sighted', 'ALERT - ENEMY SIGHTED IN THE ...'),
    ('ovl4', 0xB6E5): ('msg_merchant_taken', 'NOTICE - MERCHANT LOST / ENEMY MERCHANT CAPTURED'),
    ('ovl4', 0xB7B0): ('msg_resupplied', 'MERCHANT SUPPLIES SQUAD'),
    ('ovl4', 0xB88B): ('msg_flag_taken', 'URGENT - FLAG TAKEN'),
    ('ovl4', 0xB8F6): ('msg_enemy_flag', 'ENEMY FLAG CAPTURED'),
    ('ovl4', 0xB961): ('msg_crown', 'WHITE / BLACK CROWN CAPTURED; the crown flashes to a tune with the sound on'),
    ('ovl4', 0xBACD): ('take_merchant', 'take_merchant(m, w): w takes merchant m: +10 supplies'),
    ('ovl4', 0xBB29): ('meet_merchant', 'A merchant meets a warship of its side: supplies 20; used up if it had less than 10'),
    ('ovl4', 0xBBBC): ('message_row', 'A message row: up to two, then they scroll'),
    ('ovl4', 0xBCD2): ('restock_ports', 'Tick 10: ports with 2 or less left call for a merchant'),
    ('ovl4', 0xBD3C): ('port_side', 'The side of a port\'s merchant, from the strength map DS:4645'),
    ('ovl4', 0xBD86): ('port_call', 'A port was used: restocked at once beside a crown or with no sea way; else a merchant sets out (BEE3)'),
    ('ovl4', 0xC07C): ('port_sources', 'The ports merchants can reach (DS:689C) and where they come from (6932)'),
    ('ovl4', 0xC102): ('port_source_of', 'Where a port\'s merchants come from'),
    ('ovl4', 0xC141): ('merchant_bound', 'Is a merchant sailing for this port?'),
    ('ovl4', 0xC197): ('port_supplies', 'At the start: repair ports rule 64AF, supply ports and crowns twice that'),
    ('ovl4', 0xC21C): ('invert_empty_ports', 'Ports with nothing are drawn inverted'),
    ('ovl4', 0xC302): ('promote_at_sea', 'After a loss at sea the squadron\'s first other ship of the line becomes a flagship'),
    ('ovl4', 0xC35C): ('invert_cell', 'invert_tile_buf for a map cell'),
    ('ovl4', 0xC3C9): ('flags_check', '1: you have no flagship left, 2: they have none (unless 65BA / 65BB: no flagships at the start but a crown)'),
    ('ovl4', 0xC44E): ('crown_check', 'A squadron beside the other side\'s crown takes it unless a squadron of that side is in its cell: the game ends'),
    ('ovl4', 0xC51A): ('crowns_check', 'Tick 20: crown_check for every squadron'),
    ('ovl4', 0xC57C): ('port_empty_sound', 'Three notes: a port has nothing left'),
    ('ovl4', 0xC5B9): ('close_to', 'close_to(a, b): |dx| < 3 and |dy| < 5; distance 2|dx| + |dy| into 689A'),
    ('ovl4', 0xC658): ('wind_change', 'The wind (option 6865): a new direction and its message, or (1) just the face over tile 73'),
    ('ovl4', 0xD549): ('count_at_cell', 'count_at_cell(cell, side, any): squadrons (anchored, or any) of a side in a cell'),
    ('ovl4', 0xE5BF): ('sea_way', 'A way by sea from a port\'s cell to the edge of the map (the cell on the edge, or -1)'),
    ('ovl4', 0xE805): ('strength_map', 'DS:4645: the strength of each side in each cell'),
    ('ovl4', 0xE8C8): ('squadron_strength', 'Ship strengths less half their damage'),
    ('ovl4', 0xEA05): ('has_flagship', 'Has the squadron a flagship?'),
    ('ovl4', 0xEA51): ('port_value_map', 'DS:47D5: the value of the cells beside ports and crowns'),
    # --- the enemy commander (step 5 of the remake, 13 Nov 2026)
    ('ovl4', 0xB03E): ('path_steps', 'The steps from the squadron [62FF] to x, y into the path ring after pos, at most n'),
    ('ovl4', 0xB1B5): ('abs_value', 'abs()'),
    ('ovl4', 0xBEE3): ('new_merchant', 'A merchant squadron at the edge of the map for a port, with a route (db21)'),
    ('ovl4', 0xC79A): ('enemy_orders', 'four times a tick, the next anchored enemy squadron (or merchant) is given orders'),
    ('ovl4', 0xC862): ('enemy_orders_one', 'A squadron on a route follows it (cfef), else new orders (c88a)'),
    ('ovl4', 0xC88A): ('new_orders', 'Stay to resupply, repair or guard a crown, or choose a target (caea)'),
    ('ovl4', 0xCAEA): ('choose_target', 'Off the land; a port for supplies or repairs; or the best cell: your crown [6075], a flagship, a weak spot'),
    ('ovl4', 0xCFEF): ('follow_route', 'To the middle of its cell; attack a squadron near enough (d5c2); on along the route; or arrived: a merchant unloads'),
    ('ovl4', 0xD5C2): ('pick_victim', 'The nearest of your squadrons (or a merchant) worth attacking; [64D7] its distance + 1'),
    ('ovl4', 0xD799): ('way_barred', 'Land, or water the squadron should keep out of, between two cells (5 at most)'),
    ('ovl4', 0xD99E): ('sail_to', 'A path to x, y (at most n steps) at full sail'),
    ('ovl4', 0xDB21): ('find_route', 'A route for [62FF] to the cell [64DF] avoiding your strength (DS:3F3D-457D); the first half into the route ring DS:542F'),
    ('ovl4', 0xE103): ('port_search', 'The nearest cell beside a port of a kind (74, 75) or a crown that can serve'),
    ('ovl4', 0xE554): ('port_ok', 'A port that can serve (more than 1 left, or the last of its kind)'),
    ('ovl4', 0xE96B): ('frigate_aboard', 'A frigate in the squadron [62FF]'),
    ('ovl4', 0xE9B8): ('liner_aboard', 'A ship of the line or flagship in the squadron [62FF]'),
    ('ovl4', 0xED83): ('enemy_join', 'A squadron of fewer than three ships takes the ships of one close by'),
    ('ovl4', 0xEEBF): ('bound_for', 'The enemy squadrons bound for or in a cell'),
    ('ovl4', 0xEF2E): ('flash_squadron', 'Flash a squadron ten times (not called)'),
    ('ovl4', 0xEE4E): ('worst_damage', 'The worst sail or hull damage among a squadron\'s ships'),
    # --- matched to WAR.EXE
    ('rt', 0x01DF): ('require_war_disk', 'Same code as WAR.EXE rt:011E (exact match of the instruction sequence); see WAR_Analysis'),
    ('rt', 0x04CA): ('disk_prompt', 'Same code as WAR.EXE rt:0400 (exact match of the instruction sequence); see WAR_Analysis'),
    ('rt', 0x0BC2): ('text_rows_add', 'Same code as WAR.EXE rt:055B (exact match of the instruction sequence); see WAR_Analysis'),
    ('rt', 0x0C10): ('text_area_reset', 'Same code as WAR.EXE rt:0F80 (exact match of the instruction sequence); see WAR_Analysis'),
    ('rt', 0x0C32): ('beep_low', 'Same code as WAR.EXE rt:05A9 (exact match of the instruction sequence); see WAR_Analysis'),
    ('rt', 0x0D48): ('show_one_moment', 'Same code as WAR.EXE rt:06C1 (exact match of the instruction sequence); see WAR_Analysis'),
    ('rt', 0x14A6): ('tile_is_land', 'Tiles 3-6, 40, 41, 46, 47, 51-77 (the ports and crowns too). Same code as WAR.EXE ovl7:60B0 (there: battle tiles)'),
    ('rt', 0x19BC): ('path_next_slot', 'Same code as WAR.EXE rt:1369 (exact match of the instruction sequence); see WAR_Analysis'),
    ('rt', 0x26D7): ('print_number', 'Same code as WAR.EXE rt:176B (exact match of the instruction sequence); see WAR_Analysis'),
    ('rt', 0x2759): ('load_file', 'Same code as WAR.EXE rt:17ED (exact match of the instruction sequence); see WAR_Analysis'),
    ('rt', 0x27A2): ('save_cursor_background', 'Same code as WAR.EXE rt:1836 (exact match of the instruction sequence); see WAR_Analysis'),
    ('rt', 0x2870): ('restore_cursor_background', 'Same code as WAR.EXE rt:190C (exact match of the instruction sequence); see WAR_Analysis'),
    ('rt', 0x2B95): ('draw_hline', 'Same code as WAR.EXE rt:1DF3 (exact match of the instruction sequence); see WAR_Analysis'),
    ('rt', 0x2BB4): ('draw_icon_8x6', 'Same code as WAR.EXE rt:1E12 (exact match of the instruction sequence); see WAR_Analysis'),
    ('rt', 0x2C09): ('scroll_text_area', 'Same code as WAR.EXE rt:1E9F (exact match of the instruction sequence); see WAR_Analysis'),
    ('rt', 0x2C48): ('clear_text_row', 'Same code as WAR.EXE rt:1EDE (exact match of the instruction sequence); see WAR_Analysis'),
    ('rt', 0x2C7F): ('memset', 'Same code as WAR.EXE rt:1F15 (exact match of the instruction sequence); see WAR_Analysis'),
    ('rt', 0x2C96): ('memsetw', 'Same code as WAR.EXE rt:1F2C (exact match of the instruction sequence); see WAR_Analysis'),
    ('rt', 0x2CAD): ('memcpy', 'Same code as WAR.EXE rt:1F43 (exact match of the instruction sequence); see WAR_Analysis'),
    ('rt', 0x2CF3): ('set_text_pos_px', 'Same code as WAR.EXE rt:1F87 (exact match of the instruction sequence); see WAR_Analysis'),
    ('rt', 0x2D83): ('invert_bar', 'Same code as WAR.EXE rt:2017 (exact match of the instruction sequence); see WAR_Analysis'),
    ('rt', 0x2EBF): ('goto_xy', 'Same code as WAR.EXE rt:205C (exact match of the instruction sequence); see WAR_Analysis'),
    ('rt', 0x2EE0): ('print_spaces', 'Same code as WAR.EXE rt:207D (exact match of the instruction sequence); see WAR_Analysis'),
    ('rt', 0x2EF9): ('seed_random', 'Same code as WAR.EXE rt:2096 (exact match of the instruction sequence); see WAR_Analysis'),
    ('rt', 0x2F0D): ('random', 'Same code as WAR.EXE rt:20AA (exact match of the instruction sequence); see WAR_Analysis'),
    ('rt', 0x2F31): ('speaker_click', 'Same code as WAR.EXE rt:20CE (exact match of the instruction sequence); see WAR_Analysis'),
    ('rt', 0x2F46): ('speaker_freq', 'Same code as WAR.EXE rt:20DC (exact match of the instruction sequence); see WAR_Analysis'),
    ('rt', 0x2F59): ('speaker_on', 'Same code as WAR.EXE rt:20EF (exact match of the instruction sequence); see WAR_Analysis'),
    ('rt', 0x2F6E): ('delay', 'Same code as WAR.EXE rt:2188 (exact match of the instruction sequence); see WAR_Analysis'),
    ('rt', 0x30B5): ('set_cga_palette', 'Same code as WAR.EXE rt:22C8 (exact match of the instruction sequence); see WAR_Analysis'),
    ('rt', 0x30CF): ('set_palette_reg', 'Same code as WAR.EXE rt:22E2 (exact match of the instruction sequence); see WAR_Analysis'),
    ('rt', 0x3355): ('print_string', 'Same code as WAR.EXE rt:2565 (exact match of the instruction sequence); see WAR_Analysis'),
    ('rt', 0x33F3): ('dos_open', 'Same code as WAR.EXE rt:260A (exact match of the instruction sequence); see WAR_Analysis'),
    ('rt', 0x3437): ('dos_read', 'Same code as WAR.EXE rt:264E (exact match of the instruction sequence); see WAR_Analysis'),
    ('rt', 0x3450): ('dos_write', 'Same code as WAR.EXE rt:2667 (exact match of the instruction sequence); see WAR_Analysis'),
    ('rt', 0x3467): ('dos_delete', 'Same code as WAR.EXE rt:267E (exact match of the instruction sequence); see WAR_Analysis'),
    ('rt', 0x3473): ('dos_close', 'Same code as WAR.EXE rt:268A (exact match of the instruction sequence); see WAR_Analysis'),
    ('root', 0x0287): ('overlay_load', 'Same code as WAR.EXE root:0287 (exact match of the instruction sequence); see WAR_Analysis'),
    ('root', 0x02D0): ('overlay_call', 'Same code as WAR.EXE root:02D0 (exact match of the instruction sequence); see WAR_Analysis'),
    ('root', 0x02FA): ('start', 'Same code as WAR.EXE root:02FA (exact match of the instruction sequence); see WAR_Analysis'),
    ('ovl1', 0x7ACB): ('load_o3', 'Same code as WAR.EXE ovl1:3567 (exact match of the instruction sequence); see WAR_Analysis'),
    ('ovl1', 0x7E0B): ('ensure_war_disk', 'Same code as WAR.EXE ovl1:389B (exact match of the instruction sequence); see WAR_Analysis'),
    ('ovl1', 0x7EE7): ('disk_sound', 'Same code as WAR.EXE ovl1:3977 (exact match of the instruction sequence); see WAR_Analysis'),
    ('ovl1', 0x851B): ('clear_all_text_rows', 'Same code as WAR.EXE ovl1:3FA0 (exact match of the instruction sequence); see WAR_Analysis'),
    ('ovl1', 0x853E): ('save_or_reload', 'Same code as WAR.EXE ovl1:3FC3 (exact match of the instruction sequence); see WAR_Analysis'),
    ('ovl1', 0x8694): ('scroll_erase_line', 'Same code as WAR.EXE ovl1:4125 (exact match of the instruction sequence); see WAR_Analysis'),
    ('ovl1', 0x8A93): ('menu_poll_key', 'Same code as WAR.EXE ovl1:4545 (exact match of the instruction sequence); see WAR_Analysis'),
    ('ovl1', 0x8B7A): ('play_tune', 'Same code as WAR.EXE ovl1:462C (exact match of the instruction sequence); see WAR_Analysis'),
    ('ovl1', 0x9124): ('help_change_select', 'Same code as WAR.EXE ovl1:4D08 (exact match of the instruction sequence); see WAR_Analysis'),
    ('ovl1', 0x916E): ('story_step_words', 'Same code as WAR.EXE ovl1:4D52 (exact match of the instruction sequence); see WAR_Analysis'),
    ('ovl1', 0xA33A): ('save_options', 'Same code as WAR.EXE ovl1:5D5D (exact match of the instruction sequence); see WAR_Analysis'),
    ('ovl1', 0xAA0D): ('disk_error', 'Same code as WAR.EXE ovl1:61FD (exact match of the instruction sequence); see WAR_Analysis'),
    ('ovl1', 0xAA48): ('story_view', 'Same code as WAR.EXE ovl1:6238 (exact match of the instruction sequence); see WAR_Analysis'),
    ('ovl1', 0xAD69): ('titles_help', 'Same code as WAR.EXE ovl1:655A (exact match of the instruction sequence); see WAR_Analysis'),
    ('ovl1', 0xB54A): ('scroll_print', 'Same code as WAR.EXE ovl1:6D7E (exact match of the instruction sequence); see WAR_Analysis'),
    ('ovl1', 0xB5B0): ('small_caps', 'Same code as WAR.EXE ovl1:6DE4 (exact match of the instruction sequence); see WAR_Analysis'),
    ('ovl1', 0xB6AC): ('scroll_row_up', 'Same code as WAR.EXE ovl1:6EDE (exact match of the instruction sequence); see WAR_Analysis'),
    ('ovl1', 0xB73B): ('scroll_row_down', 'Same code as WAR.EXE ovl1:6F6D (exact match of the instruction sequence); see WAR_Analysis'),
    ('ovl1', 0xB7AE): ('scroll_advance', 'Same code as WAR.EXE ovl1:6FE0 (exact match of the instruction sequence); see WAR_Analysis'),
    ('ovl1', 0xB827): ('scroll_back', 'Same code as WAR.EXE ovl1:7059 (exact match of the instruction sequence); see WAR_Analysis'),
    ('ovl1', 0xB895): ('scroll_char', 'Same code as WAR.EXE ovl1:70C7 (exact match of the instruction sequence); see WAR_Analysis'),
    ('ovl1', 0xBB9F): ('is_capital', 'Same code as WAR.EXE ovl1:73D1 (exact match of the instruction sequence); see WAR_Analysis'),
    ('ovl1', 0xBBDD): ('scroll_key_ok', 'Same code as WAR.EXE ovl1:740F (exact match of the instruction sequence); see WAR_Analysis'),
    ('ovl1', 0xC307): ('war_disk_prompt', 'Same code as WAR.EXE ovl1:77D9 (exact match of the instruction sequence); see WAR_Analysis'),
    ('ovl1', 0xC32E): ('make_data_path', 'Same code as WAR.EXE ovl1:7816 (exact match of the instruction sequence); see WAR_Analysis'),
    ('ovl1', 0xC527): ('write_campaign_file', 'Same code as WAR.EXE ovl1:7A71 (prefix match of the instruction sequence); see WAR_Analysis'),
    ('ovl1', 0xC8D0): ('write_titles', 'Same code as WAR.EXE ovl1:7E09 (exact match of the instruction sequence); see WAR_Analysis'),
    ('ovl1', 0xC961): ('init_hardware', 'Same code as WAR.EXE ovl1:7EDA (exact match of the instruction sequence); see WAR_Analysis'),
    ('ovl1', 0xCA44): ('scroll_copy_lines', 'Same code as WAR.EXE ovl1:7FBD (exact match of the instruction sequence); see WAR_Analysis'),
    ('ovl1', 0xCA83): ('copy_glyph', 'Same code as WAR.EXE ovl1:7FFC (exact match of the instruction sequence); see WAR_Analysis'),
    ('ovl1', 0xCABE): ('and_pointer', 'Same code as WAR.EXE ovl1:8037 (exact match of the instruction sequence); see WAR_Analysis'),
    ('ovl1', 0xCAF7): ('xor_box', 'Same code as WAR.EXE ovl1:8070 (exact match of the instruction sequence); see WAR_Analysis'),
    ('ovl1', 0xCB92): ('xor_frame', 'Same code as WAR.EXE ovl1:810B (exact match of the instruction sequence); see WAR_Analysis'),
    ('ovl1', 0xCCEA): ('bios_disk_rw', 'Same code as WAR.EXE ovl1:822D (exact match of the instruction sequence); see WAR_Analysis'),
    ('ovl1', 0xCDCA): ('detect_joystick', 'Same code as WAR.EXE ovl1:8314 (exact match of the instruction sequence); see WAR_Analysis'),
    ('ovl1', 0xCDF7): ('wait_port_change', 'Same code as WAR.EXE ovl1:83C0 (exact match of the instruction sequence); see WAR_Analysis'),
    ('ovl1', 0xCE0C): ('detect_video', 'Same code as WAR.EXE ovl1:83D5 (exact match of the instruction sequence); see WAR_Analysis'),
    ('ovl1', 0xCEF0): ('check_video', 'Same code as WAR.EXE ovl1:84B9 (exact match of the instruction sequence); see WAR_Analysis'),
    ('ovl1', 0xCF1C): ('calibrate_delay', 'Same code as WAR.EXE ovl1:84E5 (exact match of the instruction sequence); see WAR_Analysis'),
    ('ovl1', 0xCF96): ('key_disk_check', 'Same code as WAR.EXE ovl1:8341 (exact match of the instruction sequence); see WAR_Analysis'),
    ('ovl2', 0x7B69): ('sqed_get', 'Same code as WAR.EXE ovl3:35AF (exact match of the instruction sequence); see WAR_Analysis'),
    ('ovl2', 0x7FBF): ('sqed_read_key', 'Same code as WAR.EXE ovl3:3E69 (exact match of the instruction sequence); see WAR_Analysis'),
    ('ovl3', 0x79C3): ('mapmk_panel', 'Same code as WAR.EXE ovl4:33FA (exact match of the instruction sequence); see WAR_Analysis'),
    ('ovl3', 0x7B4C): ('mapmk_page', 'Same code as WAR.EXE ovl4:3558 (exact match of the instruction sequence); see WAR_Analysis'),
    ('ovl3', 0x7E44): ('mapmk_read_key', 'Same code as WAR.EXE ovl4:37F6 (exact match of the instruction sequence); see WAR_Analysis'),
    ('ovl3', 0x7EC3): ('mapmk_joystick', 'Same code as WAR.EXE ovl4:3875 (exact match of the instruction sequence); see WAR_Analysis'),
    ('ovl3', 0x811C): ('mapmk_plan_cursor', 'Same code as WAR.EXE ovl4:3ACD (exact match of the instruction sequence); see WAR_Analysis'),
    ('ovl3', 0x8537): ('mapmk_tile', 'Same code as WAR.EXE ovl4:3AF7 (exact match of the instruction sequence); see WAR_Analysis'),
    ('ovl3', 0x8561): ('mapmk_invert', 'Same code as WAR.EXE ovl4:3B21 (exact match of the instruction sequence); see WAR_Analysis'),
    ('ovl3', 0x8591): ('mapmk_plan_mark', 'Same code as WAR.EXE ovl4:3B51 (exact match of the instruction sequence); see WAR_Analysis'),
    ('ovl3', 0x85B9): ('mapmk_plan', 'Same code as WAR.EXE ovl4:3B79 (exact match of the instruction sequence); see WAR_Analysis'),
    ('ovl4', 0x85D2): ('command_pause', 'Same code as WAR.EXE ovl6:4605 (exact match of the instruction sequence); see WAR_Analysis'),
    ('ovl4', 0xAAA8): ('show_time_setting', 'Same code as WAR.EXE ovl6:6C0B (exact match of the instruction sequence); see WAR_Analysis'),
    ('ovl4', 0xAB48): ('select_squad', 'Same code as WAR.EXE ovl6:6CA3 (exact match of the instruction sequence); see WAR_Analysis'),
    ('ovl4', 0xAF61): ('step_distance', 'Same code as WAR.EXE ovl6:75BB (exact match of the instruction sequence); see WAR_Analysis'),
    ('ovl4', 0xAFB4): ('cell_distance', 'Same code as WAR.EXE ovl6:760E (exact match of the instruction sequence); see WAR_Analysis'),
    ('ovl4', 0x89E4): ('start_move_order', 'Same code as WAR.EXE ovl6:477D (exact match of the instruction sequence); see WAR_Analysis'),
    ('ovl4', 0xB68C): ('msg_encounter', 'Same code as WAR.EXE ovl6:5A6E (exact match of the instruction sequence); see WAR_Analysis'),
    ('ovl4', 0xB7E4): ('msg_fighting', 'Same code as WAR.EXE ovl6:5AC7 (exact match of the instruction sequence); see WAR_Analysis'),
    ('ovl4', 0xB83B): ('msg_battle_result', 'Same code as WAR.EXE ovl6:5B1E (exact match of the instruction sequence); see WAR_Analysis'),
    ('ovl4', 0xBBEB): ('map_region_code', 'Same code as WAR.EXE ovl6:5D00 (exact match of the instruction sequence); see WAR_Analysis'),
    ('ovl4', 0xBC2D): ('print_map_region', 'Same code as WAR.EXE ovl6:5D42 (prefix match of the instruction sequence); see WAR_Analysis'),
    ('ovl4', 0xC394): ('play_end_sound', 'Same code as WAR.EXE ovl6:38F6 (exact match of the instruction sequence); see WAR_Analysis'),
    ('ovl4', 0xB1CE): ('route_free_run', 'Same code as WAR.EXE rt:14A0 (prefix match of the instruction sequence); see WAR_Analysis'),
    ('ovl4', 0xECD1): ('find_free_path_run', 'Same code as WAR.EXE ovl6:47F8 (exact match of the instruction sequence); see WAR_Analysis'),
    # step 6: the menus (WAS_Remake/src/menu.c)
    ('ovl1', 0x82AF): ('campaign_menu', 'thunk 6. Make changes: SPG and SO4 (scenes 19Bh/1A7h/1B3h, names 1E3h), then campaign_menu_choose'),
    ('ovl1', 0xA5F5): ('campaign_menu_choose', 'The scene under the frame: -/+ sweep a box over rows 0-1 and print its name; the frame round it blinks (delay 1000 / 800)'),
    ('ovl1', 0x843D): ('editor_menu', 'thunk 17. The editor menu: SPH and SO4 (1BFh/1CBh/1D7h, names 1EFh) (step 7)'),
    ('ovl1', 0x838D): ('new_campaign', 'thunk 10. The last steps of a new campaign: the opponent (6485 = 9), the story (11, 0), the title (3) (step 7)'),
    ('ovl1', 0x857B): ('remove_campaign', 'thunk 11. Remove a campaign from a data disk (step 7)'),
    ('ovl1', 0x92CC): ('change_campaign', 'thunk 7. Change a campaign: blank armies, default rules, the data disk (step 7)'),
    ('ovl1', 0x9372): ('blank_armies', 'Every squadron and ship slot emptied (step 7)'),
    ('ovl1', 0x93FA): ('default_rules', 'A new campaign\'s rules: 40, 10, -, 1, 2, 80, 1, 1 (step 7)'),
    ('ovl1', 0x9428): ('rules_write', 'The rules as sentences on the scroll (SO3 A09h RULES, the seven rules at rows 3-15), then unroll'),
    ('ovl1', 0x951E): ('rule_repairs', 'Rule 64AD: slowly (20), steadily (40), briskly'),
    ('ovl1', 0xAB0A): ('titles_choose', 'The pointer on the titles; Enter: a title, ( war disk ) (0) or ( data disk ) (11) -> 270Ah; Esc -> 270Bh'),
    ('ovl1', 0xAE72): ('rules_choose', 'The pointer on the rules, Space changes one (columns at SO3+8F8h)'),
    ('ovl1', 0xB21E): ('scroll_editor', 'Typing on the scroll: the story (6485 0/10) or a title (3) (step 7)'),
    ('ovl1', 0xB4F3): ('scroll_at', 'scroll_at(col, row): the writing position [64D3] = 2 col, [64D5] = row, [64D7] = BUF+826h+row*1E0h, [6870]'),
    ('ovl1', 0xB64C): ('scroll_clear', 'The scroll\'s 840 cells written with spaces (0.75 s on an XT)'),
    ('ovl1', 0xBCA0): ('scroll_write', 'The text (DS:4E53) written on the scroll, counting the titles ([64DD]), then scroll_unroll'),
    ('ovl1', 0xBDAC): ('scroll_unroll', 'The scroll unrolls from line 36 to 180, the curl (BUF+39D0h) below, the speaker crackling'),
    ('ovl1', 0xBE42): ('scroll_roll_up', 'The curl copied up from line 180 to 34, then the top of the scroll'),
    ('ovl1', 0xBF06): ('make_data_disk', 'Shall I make a new data disk? (Y or N): formats the floppy, writes SITLES from SDITLES'),
    ('ovl1', 0xC2E6): ('data_disk_back', 'After making a data disk: the WAS disk again'),
    ('ovl1', 0xC617): ('titles_load', 'The disk\'s SITLES into 0BB5h and the scroll text; on a data disk with SS: Saved game / New campaigns (S -> 2)'),
    ('ovl1', 0x7E51): ('data_disk_prompt', 'Put DATA disk in drive A/B, press any key; [6497] 1, or 2 if the WAS disk (the file B) is still there'),
    ('ovl1', 0x9AAB): ('tune_victory', 'The battle report\'s tune when won (SO2 122Fh/1295h)'),
    ('ovl1', 0x9B45): ('tune_defeat', 'The battle report\'s tune when lost (SO2 110Dh/11E5h)'),
    ('ovl1', 0xA282): ('shoppe_help', 'The Options Shoppe\'s rows 23-24'),
    ('ovl1', 0xA384): ('center_screen', 'Options Shoppe: -/+ move the picture (CRTC register 2, [684C] 40-46)'),
    ('ovl1', 0xA489): ('joystick_calibrate', 'Options Shoppe: the joystick\'s corner and centre'),
    ('ovl1', 0xA5A2): ('shoppe_sweep', 'A box sweeps across row 24'),
    ('ovl1', 0xA935): ('xor_text', 'xor_text(s, col, row): text XORed onto a picture (the report, the opening)'),
    ('ovl1', 0xA966): ('xor_number', 'xor_number(n, col, row): up to two digits, XORed'),
    ('ovl1', 0xCF52): ('xor_char', 'A character of the font XORed onto the screen at a text position'),
    ('ovl1', 0x91BA): ('scroll_done_help', 'Row 24: [Esc] when done (step 7)'),
    ('ovl1', 0x91F4): ('title_step', 'A new campaign\'s title: the quill below the titles (step 7)'),
}

PLAY_OVERLAY = 'ovl4'

OVERLAY_ROLE = {
    1: 'Main program (as WAR.EXE overlay 1): startup (SCNF, SPM), title and menu screens, scrolls, opponents (SO2, SPJ, AD), Options Shoppe (SPK), battle report (SPN-SPQ), save/load (SS), data disk, the campaign editor flows, hardware detection, key-disk check, campaign loader (S0-SA)',
    2: 'Campaign editor: placing ships, fleets and flag ships (SO1); as WAR.EXE overlay 3',
    3: 'Campaign editor: map maker (SO0); as WAR.EXE overlay 4',
    4: 'Campaign play: the sea map, fleets, ports, wind, messages (SO5); as WAR.EXE overlay 6',
    5: 'Sea battle (Zoom): ships manoeuvre and fire; calls overlay 6 (and overlay 7 for boarding) through thunks',
    6: 'Sea battle support, loads at C9B0h beside overlay 5: loads SO6, SO7, SO8; the battle results ("They lost N ships")',
    7: 'Boarding: the fight on deck (picture B; Flag, Quarterdeck, Poopdeck, Railing), loads at C9B0h beside overlay 5',
}

KNOWN_VARS = {
}

KNOWN_ARRAYS = {
}


# ------------------------------------------------------------------ helpers
STR = {}
for m in re.finditer(rb'[\x20-\x7e\r\n]{1,}', W.DSEG):
    if m.start() == 0 or W.DSEG[m.start() - 1] == 0:
        STR[m.start()] = m.group().decode()
FILENAMES = {k: v for k, v in STR.items()
             if re.fullmatch(r'S[0-9AB]|SO[0-8]|SP[A-Z]|SM|SS|AD|[AB]:B|B|SCNF|SITLES|SDITLES|TITLES', v)}
LOADERS = {0x2759, 0xC32E, 0x33F3, 0x3467}      # load_file, make_data_path, dos_open, dos_delete

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
                and (v >= 0x100 or nxt_call == 0x3355):
            s = STR[v].replace('\r', '\\r').replace('\n', '\\n')
            c.append(f'DS:{v:04X} "{s[:48]}"')
    # per-squad arrays
    if r.name == PLAY_OVERLAY and ins.mnemonic == 'mov' and len(ops) == 2 and ops[0].type == x86.X86_OP_REG \
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
                if seg == W.ROOT_SEG:
                    text = f'lcall  {W.ROOT_SEG:04X}:{t:04X}'
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
        elif (r.name, off) in W.SWITCHES:
            lo, n, dflt, tg = W.SWITCHES[(r.name, off)]
            P(f'  {r.name}:{off:04X}  {r.file_of(off):05X}  dw {lo}, {n}, {dflt:04X}h'
              f'{"":<54} ; switch table: lowest case {lo}, {n} cases, default {dflt:04X}')
            for k, t in enumerate(tg):
                a = off + 6 + 2 * k
                P(f'  {r.name}:{a:04X}  {r.file_of(a):05X}  dw {t:04X}h{"":<64} ; case {lo + k}')
            off += 6 + 2 * n
        elif (r.name, off) in W.SPARSE:
            vals, tg = W.SPARSE[(r.name, off)]
            n = len(vals)
            P(f'  {r.name}:{off:04X}  {r.file_of(off):05X}  dw {n}{"":<67} ; sparse switch table: {n} values, then {n + 1} targets (the default first)')
            for k, v in enumerate(vals):
                a = off + 2 + 2 * k
                P(f'  {r.name}:{a:04X}  {r.file_of(a):05X}  dw {v}{"":<{68 - len(str(v))}} ; value {k}')
            for k, t in enumerate(tg):
                a = off + 2 + 2 * n + 2 * k
                P(f'  {r.name}:{a:04X}  {r.file_of(a):05X}  dw {t:04X}h{"":<64} ; ' + ('default' if k == 0 else f'case {vals[k - 1]}'))
            off += 4 + 4 * n
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
                note = '  ; "COPYRIGHT 1983 DAVE MURRY" with every character +1 (then "Insert disk in drive")'
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
TBL_FILE = W.HEADER_SIZE + W.OVL_TABLE_IMG
THUNK_END = W.THUNK_IMG + 12 * len(W.THUNKS)
P(';' + '#' * 78)
P('; WAS.EXE - The Ancient Art of War at Sea, CGA version (Broderbund; game (c) 1987 Dave and Barry Murry)')
P('; From the 3.5" disk release (files dated 11 July 1988).')
P(';')
P('; Annotated disassembly, generated by recursive descent from the program entry point,')
P('; the overlay thunk table and every call target, so code and data are separated.')
P('; Every code region is shown at the address it really runs at.')
P(';')
P('; Line format:   region:offset  file-offset  bytes  instruction   ; comment')
P(f';   region  rt = resident code (segment 0000), root = startup/loader (segment {W.ROOT_SEG:04X}),')
P(';           ovl1..ovl7 = overlays (segment 0000, at their load offset), DS = data segment')
P(';   Names in the form ovl3_4a2f / rt_1330 are unnamed functions; named ones are listed below.')
P(';   Names marked "Same code as WAR.EXE" were matched to WAR.EXE.asm by their instruction sequence.')
P(';' + '#' * 78)
P('')
section('MZ HEADER', [
    f'file size          {len(W.D)} bytes',
    f'header             {W.HEADER_SIZE:#x} bytes, {mz[3]} relocations',
    f'load image         {img_size:#x} bytes declared (file {W.HEADER_SIZE:#x}-{W.HEADER_SIZE + img_size:#x})',
    f'                   the overlays and data segment are stored after it, inside WAS.EXE',
    f'entry CS:IP        {mz[11]:04X}:{mz[10]:04X}  (file {W.HEADER_SIZE + mz[11] * 16 + mz[10]:#x})',
    f'initial SS:SP      {mz[7]:04X}:{mz[8]:04X}',
    'linker             Phoenix Software PLINK86 (overlay loader "LINK86 Overlay Loader"),',
    '                   the same loader as WAR.EXE (1290 of its 1310 bytes are identical)',
])
section('MEMORY MAP (segments relative to the load address)', [
    f'0000:0000-{W.RT_SIZE - 1:04X}  rt     resident C code and library: file I/O, text, graphics, sound, game core',
    f'{W.ROOT_SEG:04X}:0000-051D  root   startup and Phoenix overlay loader (entry {W.ROOT_SEG:04X}:02FA)',
    f'0000:{W.OVL_TABLE_IMG:04X}        table  overlay descriptor table (16-byte entries, see below)',
    f'0000:{W.THUNK_IMG:04X}-{THUNK_END - 1:04X}  thunks {len(W.THUNKS)} overlay call thunks, called as near calls 0000:{W.THUNK_IMG:04X} + 12*n',
    '0000:7650-....  ovl1-5 overlay area 1: one of overlays 1-5',
    '0000:C9B0-....  ovl6-7 overlay area 2: overlay 6 or 7 (flag 5), resident beside overlay 2, 3 or 5',
    f'{W.DS_SEG:04X}:0000-{W.DS_SIZE - 1:04X}  DS     data segment DGROUP (initialised data, preloaded at startup)',
    f'{W.DS_SEG:04X}:{W.DS_SIZE:04X}-....  DS     BSS, buffers, stack (SS={mz[7]:04X}h); DGROUP ends at segment {W.DATA_ENTRY["load_end"]:04X}h',
    '',
    'All game code (rt + overlays) runs with CS = load segment, so near calls between them work.',
    f'Data references [xxxx] in this code are offsets in DS (segment {W.DS_SEG:04X}h).',
])
rows = ['#  file offset  size    loads at       flags  role']
for o in W.OVERLAYS:
    role = OVERLAY_ROLE.get(o['index'], 'DGROUP initialised data (flag 4000h = load at startup)')
    if o['flags'] & 0x4000:
        role = 'DGROUP initialised data (flag 4000h = load at startup)'
    rows.append(f'{o["index"]}  {o["file_para"] * 16:#07x}     {o["size_para"] * 16:#06x}  '
                f'{o["load_seg"]:04X}-{o["load_end"]:04X}      {o["flags"]:04X}   {role}')
section(f'OVERLAY TABLE (file {TBL_FILE:#x}, segment {W.OVL_TABLE_IMG >> 4:04X})', rows + [
    '',
    'Entry layout (16 bytes, fields shared with the next entry): +4 flags (FFFFh ends the table,',
    '4000h preload), +8 load segment, +A end segment, +C file-name pointer (WAS.EXE),',
    '+E file position in paragraphs; the size in paragraphs is at +2 of the next entry.',
])
trows = []
for k, (a, ovl, tgt) in enumerate(W.THUNKS):
    nm = name_of(f'ovl{ovl}', tgt) or f'ovl{ovl}_{tgt:04x}'
    trows.append(f'thunk_{k:02d}  0000:{a:04X}  -> overlay {ovl} at {tgt:04X}  {nm}')
section(f'OVERLAY THUNKS (each: lcall {W.ROOT_SEG:04X}:02D0 / dw overlay# / jmp far target)', trows)
section('DATA FILES USED BY WAS.EXE (names are strings in DS)', [
    'B         the deck-view battle picture (word-count RLE); also the file looked for to find the',
    '          WAR AT SEA disk ("B:B", "A:B", "B")',
    'SM        94 map tiles (32x20, CGA 2 bits/pixel)',
    'SCNF      saved options',
    'SPM       title. SPE/SPF scrolls. SPG/SPH scene sheets. SPI title with ship. SPJ opponents.',
    '          SPK Options Shoppe. SPN-SPQ battle reports.',
    'AD        the opponent\'s face (the "eyes" picture), plus extra data after the picture',
    'SO0-SO8   sprites and text for each module (see WAS_file_reference.md)',
    'S0-SA     campaigns; SITLES/SDITLES campaign lists (names "S0".."SB" at DS:0BB5)',
    'SS        saved game (data disk)',
])
section('PICTURE FORMAT (B, AD, SP*)', [
    'Two fields (even lines, then odd lines); each = word N, then N codes:',
    '  code c <= 200: repeat the next byte c times; c > 200: copy the next c - 200 bytes.',
    'Each field unpacks to 8000 bytes (80 bytes x 100 lines).',
])
section('COPY PROTECTION', [
    'key_disk (ovl1:A9A9) calls key_disk_check (same code as WAR.EXE: INT 13h reads of the',
    'original floppy) and looks for the copyright string; no manual word question in WAS.EXE.',
    'The string "DPQZSJHIU 2:94 EBWF NVSSZ" is "COPYRIGHT 1983 DAVE MURRY" with each character +1.',
])
named = sorted(N.items(), key=lambda kv: (['rt', 'root', 'ovl1', 'ovl2', 'ovl3', 'ovl4', 'ovl5', 'ovl6', 'ovl7'].index(kv[0][0]), kv[0][1]))
section('NAMED FUNCTIONS', [f'{rn}:{off:04X}  {nm:<26} {desc[:80]}' for (rn, off), (nm, desc) in named])
section('KNOWN VARIABLES (DS offsets)', [f'[{k:04X}]  {v}' for k, v in sorted(KNOWN_VARS.items())])

# ------------------------------------------------------------------ body
for r in W.REGIONS:
    if r.name == 'ldat':
        section(r.title + f'  - file {r.file_start:#x}-{r.file_start + r.size:#x}', [r.note])
        emit_data(r, f'{r.cs:04X}')
        P('')
        # overlay table + thunks as data
        section(f'OVERLAY TABLE AND THUNKS - file {TBL_FILE:#x}-{W.HEADER_SIZE + THUNK_END:#x}', [
            'See the decoded tables in the header.'])
        tbl = W.Region('tbl', 'data', TBL_FILE, THUNK_END - W.OVL_TABLE_IMG, W.OVL_TABLE_IMG >> 4, 0x0000, '')
        emit_data(tbl, f'{W.OVL_TABLE_IMG >> 4:04X}')
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
