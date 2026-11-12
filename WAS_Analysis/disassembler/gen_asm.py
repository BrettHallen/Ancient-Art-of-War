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
    ('rt', 0x30AE): ('set_cga_palette_if', 'set_cga_palette(n) unless [0ECB] = 1; palette 0 is used for flashes in the sea battle (ovl5) and by ovl1_bc67 when [684D] is set'),
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
    ('ovl7', 0xCBC8): ('boarding_load_b', 'Boarding: load the deck picture B'),
    # --- matched to WAR.EXE
    ('rt', 0x01DF): ('require_war_disk', 'Same code as WAR.EXE rt:011E (exact match of the instruction sequence); see WAR_Analysis'),
    ('rt', 0x04CA): ('disk_prompt', 'Same code as WAR.EXE rt:0400 (exact match of the instruction sequence); see WAR_Analysis'),
    ('rt', 0x0BC2): ('text_rows_add', 'Same code as WAR.EXE rt:055B (exact match of the instruction sequence); see WAR_Analysis'),
    ('rt', 0x0C10): ('text_area_reset', 'Same code as WAR.EXE rt:0F80 (exact match of the instruction sequence); see WAR_Analysis'),
    ('rt', 0x0C32): ('beep_low', 'Same code as WAR.EXE rt:05A9 (exact match of the instruction sequence); see WAR_Analysis'),
    ('rt', 0x0D48): ('show_one_moment', 'Same code as WAR.EXE rt:06C1 (exact match of the instruction sequence); see WAR_Analysis'),
    ('rt', 0x14A6): ('tile_is_battle_p7', 'Same code as WAR.EXE ovl7:60B0 (exact match of the instruction sequence); see WAR_Analysis'),
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
    ('ovl4', 0x89E4): ('start_move_order', 'Same code as WAR.EXE ovl6:477D (exact match of the instruction sequence); see WAR_Analysis'),
    ('ovl4', 0xAAA8): ('show_time_setting', 'Same code as WAR.EXE ovl6:6C0B (exact match of the instruction sequence); see WAR_Analysis'),
    ('ovl4', 0xAB48): ('select_squad', 'Same code as WAR.EXE ovl6:6CA3 (exact match of the instruction sequence); see WAR_Analysis'),
    ('ovl4', 0xAF61): ('step_distance', 'Same code as WAR.EXE ovl6:75BB (exact match of the instruction sequence); see WAR_Analysis'),
    ('ovl4', 0xAFB4): ('cell_distance', 'Same code as WAR.EXE ovl6:760E (exact match of the instruction sequence); see WAR_Analysis'),
    ('ovl4', 0xB1CE): ('route_free_run', 'Same code as WAR.EXE rt:14A0 (prefix match of the instruction sequence); see WAR_Analysis'),
    ('ovl4', 0xB68C): ('msg_encounter', 'Same code as WAR.EXE ovl6:5A6E (exact match of the instruction sequence); see WAR_Analysis'),
    ('ovl4', 0xB7E4): ('msg_fighting', 'Same code as WAR.EXE ovl6:5AC7 (exact match of the instruction sequence); see WAR_Analysis'),
    ('ovl4', 0xB83B): ('msg_battle_result', 'Same code as WAR.EXE ovl6:5B1E (exact match of the instruction sequence); see WAR_Analysis'),
    ('ovl4', 0xBBEB): ('map_region_code', 'Same code as WAR.EXE ovl6:5D00 (exact match of the instruction sequence); see WAR_Analysis'),
    ('ovl4', 0xBC2D): ('print_map_region', 'Same code as WAR.EXE ovl6:5D42 (prefix match of the instruction sequence); see WAR_Analysis'),
    ('ovl4', 0xC394): ('play_end_sound', 'Same code as WAR.EXE ovl6:38F6 (exact match of the instruction sequence); see WAR_Analysis'),
    ('ovl4', 0xECD1): ('find_free_path_run', 'Same code as WAR.EXE ovl6:47F8 (exact match of the instruction sequence); see WAR_Analysis'),
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
    'key_disk_check (overlay 1, same code as WAR.EXE): reads a track of the original floppy',
    'with INT 13h. There is no manual word question in WAS.EXE.',
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
