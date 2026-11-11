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
    ('rt', 0x2188): ('delay', 'Busy-wait delay, calibrated by DS:0948'),
    ('rt', 0x22C8): ('set_cga_palette', 'BIOS INT 10h AH=0Bh BH=1: choose CGA palette 0 or 1'),
    ('rt', 0x22E2): ('set_palette_reg', 'BIOS INT 10h AH=10h (PCjr/Tandy/EGA palette register)'),
    ('rt', 0x24E7): ('put_char', 'Draw one character with the 8x8 font, handles CR/LF/BS'),
    ('rt', 0x2565): ('print_string', 'print_string(s): put_char for each byte until NUL'),
    ('rt', 0x260A): ('dos_open', 'dos_open(name, mode): install INT 24h handler, INT 21h AH=3Dh (AH=3Ch create if mode==5). Returns handle or 0'),
    ('rt', 0x264E): ('dos_read', 'dos_read(handle, buf, count): INT 21h AH=3Fh'),
    ('rt', 0x2667): ('dos_write', 'dos_write(handle, buf, count): INT 21h AH=40h'),
    ('rt', 0x267E): ('dos_delete', 'dos_delete(name): INT 21h AH=41h'),
    ('rt', 0x268A): ('dos_close', 'dos_close(handle): INT 21h AH=3Eh'),
    # --- root (segment 028D)
    ('root', 0x02FA): ('start', 'Program entry point (MZ CS:IP = 028D:02FA): set up stack, preload DGROUP, run'),
    ('root', 0x0287): ('overlay_load', 'Make an overlay resident: open WAR.EXE, seek to its file position, read it'),
    ('root', 0x02D0): ('overlay_call', 'Target of every thunk: pops overlay number, calls overlay_load, jumps on'),
    # --- overlay 1
    ('ovl1', 0x30F0): ('ovl1_startup', 'thunk 0. Load CNF (options) and PM (title screen), show "presented by"'),
    ('ovl1', 0x325F): ('game_init', 'thunk 1. Set up buffers and variables, load O3, show the title screen with the barbarian (PI)'),
    ('ovl1', 0x35C4): ('save_game', 'thunk 4. Save the game to file S on the data disk; on failure "Game not saved!" / "<C>ontinue with game or <Q>uit?"'),
    ('ovl1', 0x389B): ('ensure_war_disk', 'thunk 15. Make sure the WAR disk is in a drive (check_war_disk)'),
    ('ovl1', 0x39AA): ('show_scroll', 'thunk 14. Story / text scroll: loads PE or PF'),
    ('ovl1', 0x3D1E): ('campaign_menu', 'thunk 6. Loads PG (scene sheet 1) and O4 (menu text)'),
    ('ovl1', 0x3E08): ('campaign_intro', 'thunk 10. Story scrolls (show_scroll) and choose_opponent'),
    ('ovl1', 0x3EB6): ('editor_menu', 'thunk 16. Loads PH (scene sheet 2: rulebook, storybook...) and O4'),
    ('ovl1', 0x3FC3): ('ovl1_entry_3fc3', 'thunk 17'),
    ('ovl1', 0x4000): ('select_campaign', 'thunk 11. Copy the campaign file name (table of T0-TA at DS:08C1, 3 bytes each, index [A694]) to DS:0B3E and load it'),
    ('ovl1', 0x4161): ('show_title_barbarian', 'Loads PI (title screen with barbarian)'),
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
    ('ovl6', 0x30F0): ('campaign_play', 'thunk 2. The strategic map game (loads O5 messages)'),
    ('ovl7', 0x30F0): ('battle', 'thunk 3. Battle screen (loads O6, O7, and the terrain background P0-PD)'),
    ('ovl7', 0x48AA): ('choose_battle_background', 'Pick P0-PD from the terrain where the fight happens'),
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
    0xA6E1: 'sound on flag', 0x0948: 'delay calibration', 0xA694: 'current campaign number',
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
