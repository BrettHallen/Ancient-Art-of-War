#!/usr/bin/env python3
"""wastdis.py: a matched disassembly of WAST.EXE (the Tandy 1000 version).

WAST.EXE's main program is EXEPACKed; it is unpacked here with
../tools/disassembly/unexepack.py (in memory). Layout of the unpacked image:
  image 00000-06C8F  resident code: SEG:OFF is image SEG x 16 + OFF
  image 06C90-...    the overlay area (zeros in the file): overlays 1-4 load
                     at segment 06C9
  image 207A0-       the data segment DGROUP (segment 207A)
The overlay modules (MS-LINK, one MZ header each) follow the unpacked program.

WAST.EXE was compiled by a different compiler than WAS.EXE (far calls, sub
ax,ax for 0), so most C routines do not match instruction for instruction.
Routines are matched to WAS.EXE's (../WAS.ASM) in three passes:
  1. exact: the same instructions, operands that are addresses ignored
     (the hand-written assembly routines);
  2. same shape: the same instructions with some constants changed;
  3. similar: the strings they print (by their text) and their constants
     (Jaccard similarity, best mutual pairs above a threshold).
Writes ../WAST.ASM (every routine with its WAS.EXE name and how it
matched) and prints a summary.
"""
import os
import re
import struct
import sys
from collections import Counter, defaultdict

from capstone import Cs, CS_ARCH_X86, CS_MODE_16
from capstone import x86

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.join(HERE, '..', '..')
sys.path.insert(0, os.path.join(HERE, '..', 'tools', 'disassembly'))
from unexepack import unpack  # noqa: E402

WAST = open(os.path.join(ROOT, 'WAS_Original_Files_3.5', 'WAST.EXE'), 'rb').read()
WAS = open(os.path.join(ROOT, 'WAS_Original_Files_3.5', 'WAS.EXE'), 'rb').read()
WAS_ASM = os.path.join(HERE, '..', 'WAS.ASM')
OUT = os.path.join(HERE, '..', 'WAST.ASM')

exe, OVL_BYTES, INFO = unpack(WAST)
HDR_LEN = struct.unpack('<H', exe[8:10])[0] * 16
IMG = exe[HDR_LEN:]
FRAME = 0                         # image offset of segment 0000
CODE_END = 0x6C90                 # resident code ends where the overlay area starts
OVL_SEG = 0x6C9
DS_SEG = 0x207A
DS_IMG = DS_SEG * 16 + FRAME      # 207A0h
DSEG = IMG[DS_IMG:]

md = Cs(CS_ARCH_X86, CS_MODE_16)
md.detail = True
md.skipdata = True


# ------------------------------------------------------------------ regions
class Region:
    def __init__(self, name, data, base_lin, file_off, title):
        self.name, self.data, self.base, self.file_off, self.title = name, data, base_lin, file_off, title
        self.funcs = {}           # start offset in data -> Func


def overlay_modules():
    mods, o = [], 0
    while o + 28 <= len(OVL_BYTES) and OVL_BYTES[o:o + 2] == b'MZ':
        h = struct.unpack('<14H', OVL_BYTES[o:o + 28])
        size = (h[2] - 1) * 512 + h[1] if h[1] else h[2] * 512
        mods.append((h[13], OVL_BYTES[o + h[4] * 16:o + size], o + h[4] * 16))
        o = (o + size + 511) // 512 * 512
    return mods


PAD = (len(exe) + 511) // 512 * 512
REGIONS = [Region('rt', IMG[FRAME:CODE_END], 0, HDR_LEN + FRAME, 'RESIDENT CODE (segments 0000-06C8)')]
for n, code, o in overlay_modules():
    REGIONS.append(Region(f'ovl{n}', code, OVL_SEG * 16, PAD + o, f'OVERLAY {n} (MS-LINK module, loads at segment 06C9)'))


# ------------------------------------------------------------------ strings
def strings_of(ds):
    """offset -> text of NUL-terminated printable strings (3+ chars)."""
    out = {}
    for m in re.finditer(rb'[\x20-\x7e]{3,}(?=\x00)', ds):
        s = m.group().decode()
        for k in range(0, len(s) - 2):        # a pointer may point inside
            out.setdefault(m.start() + k, s[k:])
    return out


WAST_STR = strings_of(DSEG)


# ------------------------------------------------------------------ functions
class Func:
    def __init__(self, region, start):
        self.region, self.start = region, start
        self.insns = []
        self.name = None
        self.match = None         # (kind, was name, score)

    def lin(self, off=None):
        return self.region.base + (self.start if off is None else off)

    def addr(self, off=None):
        lin = self.lin(off)
        if self.region.name == 'rt':
            return '%04X:%04X' % (lin >> 4, lin & 15) if False else 'rt:%05X' % lin
        return '%s:%04X' % (self.region.name, (off if off is not None else self.start))


def find_starts(r):
    d = r.data
    starts = set(m.start() for m in re.finditer(rb'\x55\x8b\xec', d))
    # far calls whose target is in this region
    for reg in REGIONS:
        for m in re.finditer(rb'\x9a(..)(..)', reg.data, re.S):
            o, s = struct.unpack('<HH', m.group(1) + m.group(2))
            lin = s * 16 + o
            if r.name == 'rt' and s < OVL_SEG and lin < len(d):
                starts.add(lin)
            elif r.name != 'rt' and reg is r and s >= OVL_SEG:
                p = lin - r.base
                if 0 <= p < len(d):
                    starts.add(p)
    # push cs; call near (a far routine called from its own segment)
    for m in re.finditer(rb'\x0e\xe8(..)', d, re.S):
        t = m.start() + 4 + struct.unpack('<h', m.group(1))[0]
        if 0 <= t < len(d):
            starts.add(t)
    starts.add(0)
    return sorted(s for s in starts if d[s:s + 2] != b'\x00\x00' or d[s:s + 3] == b'\x55\x8b\xec')


class Int3F:
    """INT 3Fh with its three bytes: the overlay manager's call (MS-LINK)."""
    id = -1
    mnemonic = 'int3f'
    operands = ()

    def __init__(self, addr, b):
        self.address, self.bytes, self.size = addr, b, 5
        self.op_str = 'segment entry %d, offset %04Xh' % (b[2], struct.unpack('<H', b[3:5])[0])


def decode(data, s, e):
    out, p = [], s
    while p < e:
        if data[p:p + 2] == b'\xcd\x3f' and p + 5 <= e:
            out.append(Int3F(p, data[p:p + 5]))
            p += 5
            continue
        nxt = p + 1
        for i in md.disasm(data[p:e], p):
            if i.id != 0 and i.bytes[:2] == b'\xcd\x3f':
                break
            out.append(i)
            nxt = i.address + i.size
            if i.mnemonic == 'int' and i.op_str == '0x3f':
                break
        p = max(nxt, p + 1) if nxt > p else p + 1
    return out


for r in REGIONS:
    st = find_starts(r)
    for i, s in enumerate(st):
        e = st[i + 1] if i + 1 < len(st) else len(r.data)
        fn = Func(r, s)
        fn.insns = decode(r.data, s, e)
        r.funcs[s] = fn
ALL = [f for r in REGIONS for f in r.funcs.values()]


# ------------------------------------------------------------------ WAS.EXE's routines from its listing
def was_functions():
    funcs, cur, lines = {}, None, []
    pat = re.compile(r'^  (\w+):([0-9A-F]{4})  ([0-9A-F]{5})  ([0-9a-f]+)\s')
    for line in open(WAS_ASM, encoding='utf-8-sig'):
        if line.startswith('; ---- '):
            cur = line[7:].split()[0]
            funcs[cur] = []
            continue
        if line.startswith(';') and 'OVERLAY' in line and cur:
            cur = None
        m = pat.match(line)
        if m and cur:
            funcs[cur].append((m.group(1), int(m.group(2), 16), int(m.group(3), 16), bytes.fromhex(m.group(4))))
    return {k: v for k, v in funcs.items() if v}


WAS_FUNCS = was_functions()
# WAS.EXE's data segment (from the listing's header numbers, as wasdis.py finds them)
WAS_DS_FILE, WAS_DS_SIZE = 0x23AB0, 0x3F10
WAS_DSEG = WAS[WAS_DS_FILE:WAS_DS_FILE + WAS_DS_SIZE]
WAS_STR = strings_of(WAS_DSEG)


def was_insns(name):
    out = []
    for reg, a, fo, b in WAS_FUNCS[name]:
        out.extend(md.disasm(b, a))
    return out


# ------------------------------------------------------------------ features
CALLS = {'call', 'lcall'}
JUMPS_PREFIX = 'j'


def norm(insns):
    """mnemonics with operands: registers kept, constants kept as numbers,
    memory displacements and call / jump targets as '*'."""
    out = []
    for i in insns:
        if i.id == 0:          # skipdata
            out.append(('db',))
            continue
        if i.id == -1:
            out.append(('call', '*'))
            continue
        mn = i.mnemonic
        if mn in CALLS or mn.startswith(JUMPS_PREFIX) or mn == 'loop' or mn == 'ljmp':
            out.append((mn, '*'))
            continue
        ops = []
        for op in i.operands:
            if op.type == x86.X86_OP_REG:
                ops.append(i.reg_name(op.reg))
            elif op.type == x86.X86_OP_IMM:
                ops.append(op.imm & 0xFFFF)
            elif op.type == x86.X86_OP_MEM:
                m = op.mem
                ops.append(('m', i.reg_name(m.base) if m.base else '', i.reg_name(m.index) if m.index else '',
                            '*' if not m.base and not m.index else (m.disp if abs(m.disp) < 0x80 else '*'), op.size))
        out.append((mn,) + tuple(ops))
    return out


def shape(n):
    """norm with every number replaced: for 'same shape' matching."""
    return tuple(tuple('#' if isinstance(x, int) else x for x in t) for t in n)


def feats(insns, strtab):
    imms, strs = Counter(), set()
    for i in insns:
        if i.id <= 0 or i.mnemonic in CALLS or i.mnemonic.startswith('j'):
            continue
        for op in i.operands:
            v = None
            if op.type == x86.X86_OP_IMM:
                v = op.imm & 0xFFFF
            elif op.type == x86.X86_OP_MEM and not op.mem.base and not op.mem.index:
                v = op.mem.disp & 0xFFFF
                if v in strtab and len(strtab[v]) >= 4:
                    strs.add(strtab[v])
                continue
            if v is None:
                continue
            if v in strtab and len(strtab[v]) >= 4 and op.type == x86.X86_OP_IMM and i.mnemonic in ('mov', 'push', 'lea'):
                strs.add(strtab[v])
            elif v > 2 and v not in (0xFFFF, 0xFFFE):
                imms[v] += 1
    return imms, strs


def calls_of(insns):
    return sum(1 for i in insns if i.mnemonic in CALLS or i.id == -1)


WAS_INFO = {}
for name in WAS_FUNCS:
    ins = was_insns(name)
    n = norm(ins)
    WAS_INFO[name] = dict(norm=n, shape=shape(n), feats=feats(ins, WAS_STR), size=sum(len(b) for *_, b in WAS_FUNCS[name]),
                          calls=calls_of(ins), addr='%s:%04X' % (WAS_FUNCS[name][0][0], WAS_FUNCS[name][0][1]))
for f in ALL:
    n = norm(f.insns)
    f.norm, f.shape = n, shape(n)
    f.feats = feats(f.insns, WAST_STR)
    f.size = sum(i.size for i in f.insns)
    f.calls = calls_of(f.insns)


# Microsoft C's stack check: the routine most routines call first (mov ax, n / call far)
def _first_far_call(f):
    for i in f.insns[:6]:
        if i.id > 0 and i.mnemonic == 'lcall' and len(i.operands) == 2:
            return (i.operands[0].imm & 0xFFFF) * 16 + (i.operands[1].imm & 0xFFFF)
    return None


CHKSTK = Counter(x for x in map(_first_far_call, ALL) if x is not None).most_common(1)[0][0]
LIBNAMES = {('rt', CHKSTK): '_chkstk'}

# ------------------------------------------------------------------ matching
def trim(n):
    """drop trailing padding (nop / int3 / db) and a final ret form difference."""
    n = list(n)
    while n and n[-1][0] in ('nop', 'db', 'int3'):
        n.pop()
    return tuple(n)


for f in ALL:
    if (f.region.name, f.start) in LIBNAMES:
        f.match = ('lib', LIBNAMES[(f.region.name, f.start)], 0)
used_was = set()
by_norm = defaultdict(list)
by_shape = defaultdict(list)
for name, w in WAS_INFO.items():
    if len(w['norm']) >= 4:
        by_norm[trim(w['norm'])].append(name)
        by_shape[trim(w['shape'])].append(name)

for f in ALL:
    if len(f.norm) < 4:
        continue
    cands = [c for c in by_norm.get(trim(f.norm), []) if c not in used_was]
    if cands:
        f.match = ('exact', cands[0], 1.0)
        used_was.add(cands[0])
for f in ALL:
    if f.match or len(f.norm) < 6:
        continue
    cands = [c for c in by_shape.get(trim(f.shape), []) if c not in used_was]
    if cands:
        f.match = ('constants', cands[0], 1.0)
        used_was.add(cands[0])


def ret_far_to_near(n):
    """WAST's routines return with retf and are called far; WAS.EXE's are near."""
    return tuple(('ret',) if t[0] == 'retf' else t for t in n)


# near / far: compare again with retf as ret, push cs dropped
for f in ALL:
    if f.match or len(f.norm) < 6:
        continue
    n = trim(ret_far_to_near(f.norm))
    for kind, table, key in (('exact', by_norm, n), ('constants', by_shape, shape(n))):
        cands = [c for c in table.get(key, []) if c not in used_was]
        if cands:
            f.match = (kind, cands[0], 1.0)
            used_was.add(cands[0])
            break


def sim(a, b):
    ia, sa = a
    ib, sb = b
    ka = set(ia) | {('s', s) for s in sa}
    kb = set(ib) | {('s', s) for s in sb}
    if not ka or not kb:
        return 0.0
    inter = ka & kb
    # strings weigh three times a constant
    w = lambda k: 3.0 if isinstance(k, tuple) else 1.0
    return sum(map(w, inter)) / sum(map(w, ka | kb))


REGION_OK = {'rt': {'rt', 'root'}, 'ovl1': {'ovl1', 'rt'}, 'ovl2': {'ovl2', 'ovl3', 'rt'},
             'ovl3': {'ovl4', 'rt'}, 'ovl4': {'ovl5', 'ovl6', 'ovl7', 'rt'}}


def was_region(name):
    return WAS_INFO[name]['addr'].split(':')[0]


def similar_pass(threshold, min_size):
    scores = []
    for f in ALL:
        if f.match or f.size < min_size:
            continue
        for name, w in WAS_INFO.items():
            if name in used_was or w['size'] < min_size or was_region(name) not in REGION_OK[f.region.name]:
                continue
            s = sim(f.feats, w['feats'])
            if s >= threshold:
                ratio = min(f.size, w['size']) / max(f.size, w['size'])
                scores.append((s * (0.6 + 0.4 * ratio), id(f), f, name))
    scores.sort(key=lambda t: -t[0])
    n = 0
    for s2, _, f, name in scores:
        if f.match or name in used_was or s2 < threshold:
            continue
        f.match = ('similar', name, round(s2, 2))
        used_was.add(name)
        n += 1
    return n


# the calls each routine makes, in order
def was_call_names():
    out, cur = defaultdict(list), None
    thunk = {}
    for line in open(WAS_ASM, encoding='utf-8-sig'):
        m = re.match(r'^; (thunk_\w+)\s+0000:([0-9A-F]{4})\s+-> overlay \d+ at [0-9A-F]+\s+(\S+)', line)
        if m:
            thunk[m.group(1)] = m.group(3)
            thunk['0x%x' % int(m.group(2), 16)] = m.group(3)
        if line.startswith('; ---- '):
            cur = line[7:].split()[0]
            continue
        m = re.match(r'^  \w+:[0-9A-F]{4}  [0-9A-F]{5}  [0-9a-f]+\s+call\s+(\S+)', line)
        if m and cur:
            t = m.group(1)
            out[cur].append(thunk.get(t, t))
    return out


WAS_CALLS = was_call_names()
FUNC_AT = {(f.region.name, f.start): f for f in ALL}


def wast_calls(f):
    out = []
    for i in f.insns:
        if i.id == -1:
            out.append(None)
        elif i.id > 0 and i.mnemonic == 'lcall' and len(i.operands) == 2:
            sg, o = i.operands[0].imm & 0xFFFF, i.operands[1].imm & 0xFFFF
            lin = sg * 16 + o
            if sg < OVL_SEG and lin == CHKSTK:
                continue
            out.append(FUNC_AT.get(('rt', lin)) if sg < OVL_SEG else FUNC_AT.get((f.region.name, lin - OVL_SEG * 16)))
        elif i.id > 0 and i.mnemonic == 'call' and i.operands and i.operands[0].type == x86.X86_OP_IMM:
            out.append(FUNC_AT.get((f.region.name, i.operands[0].imm & 0xFFFF)))
    return out


for f in ALL:
    f.callees = wast_calls(f)


def propagate():
    import difflib
    was_of = {}
    n = 0
    for f in ALL:
        if not f.match:
            continue
        if f.match[0] == 'lib':
            continue
        wl = WAS_CALLS.get(f.match[1], [])
        fl = f.callees
        ta = [(c.match[1] if c is not None and c.match else ('?', id(c))) for c in fl]
        sm = difflib.SequenceMatcher(None, ta, wl, autojunk=False)
        for op, a1, a2, b1, b2 in sm.get_opcodes():
            if op != 'replace' or a2 - a1 != b2 - b1:
                continue
            for k in range(a2 - a1):
                c, name = fl[a1 + k], wl[b1 + k]
                if c is None or c.match or name not in WAS_INFO or name in used_was:
                    continue
                if was_region(name) not in REGION_OK[c.region.name] | {'rt'}:
                    continue
                ratio = min(c.size, WAS_INFO[name]['size']) / max(c.size, WAS_INFO[name]['size'], 1)
                if ratio < 0.2:
                    continue
                c.match = ('calls', name, round(ratio, 2))
                used_was.add(name)
                n += 1
    return n


similar_pass(0.45, 24)
for rnd in range(6):
    a = propagate()
    b = similar_pass(0.30, 24)
    if not a and not b:
        break
similar_pass(0.22, 16)
propagate()


# The Tandy version's own routines, named by hand (14 Nov)
MANUAL = {
    ('rt', 0x1C0A): ('memset_far', 'rep stosb to a far pointer'),
    ('rt', 0x1C1D): ('memcpy_far', 'rep movsb between far pointers'),
    ('rt', 0x1C34): ('set_video_mode', 'BIOS mode n ([2DD8]); below 7 the CGA row table (2 fields), 9 the Tandy one (4 banks of 160 bytes); then the CRTC position'),
    ('rt', 0x1C6C): ('rows_cga', 'DS:3308: the CGA screen rows (B800, even and odd fields at 0 and 2000h, 80 bytes)'),
    ('rt', 0x1C8B): ('rows_tandy', 'DS:3308: the Tandy 320x200x16 rows (4 banks at 0, 2000h, 4000h, 6000h, 160 bytes)'),
    ('rt', 0x1CC8): ('set_all_palette_regs', 'INT 10h AX=1002h (not called)'),
    ('rt', 0x1CD7): ('video_on_off', 'the mode control register 3D8h after the vertical retrace: 0Bh on, 03h off (bit 0 cleared outside mode 9)'),
    ('rt', 0x1CFC): ('set_palette_or_border', 'INT 10h AX=1000h (or 1001h for register 10h, the border)'),
    ('rt', 0x1D14): ('put_block16', 'copies a picture (word height, word bytes a line) to both fields of a 160-byte-a-line buffer (+7D00h)'),
    ('rt', 0x1D42): ('rle_unpack', "WAST.DAT's run-length code (signed byte n: n >= 0 copy n+1, else repeat -n)"),
    ('rt', 0x1D73): ('shore_to_tandy', "the battle's shore lines: fill bytes 66h -> 55h, 77h -> AAh, 67h -> 5Ah, 76h -> A5h (called by build_shore)"),
    ('rt', 0x1DAB): ('expand_2to4', 'a 2-bit picture (bytes w, h, then data) to 4 bits a pixel through the nibble table DS:2E86: colours 0, 1, 2, 3 -> 0, 3, 12, 15 (the boarding men)'),
    ('rt', 0x1DF1): ('dos_disk_reset', 'INT 21h AH=0Dh'),
    ('rt', 0x1DF6): ('wastdat_open', "opens WAST.DAT and reads its directory (startup)"),
    ('rt', 0x1EE8): ('wastdat_find', 'looks a record up in the directory and reads it'),
    ('rt', 0x1FC0): ('wastdat_read', 'reads a record, asking for the disk while it fails'),
    ('rt', 0x200A): ('wastdat_unpack', 'reads a record and unpacks it (rle_unpack)'),
    ('rt', 0x2070): ('wastdat_campaign', 'S0-SA from records 40-50 (load_campaign)'),
    ('rt', 0x2148): ('wastdat_picture', 'a picture record into the buffer (the title pages, scrolls, menus)'),
    ('rt', 0x2188): ('wastdat_raw', 'a record read as it is (SO files, campaign menu)'),
    ('rt', 0x2504): ('draw_map_sprite', "WAS.EXE's draw_map_sprite (rt:2941) in 16 colours: each 2-bit byte through expand (rt_3495, the table DS:2E86), the outline one 4-bit pixel to each side; arg 0 your side (white with black outline), else the enemy's (black with white outline)"),
    ('rt', 0x25CF): ('draw_tile16', 'a 32x20 16-colour map tile (8 words a line) into both fields of the map buffer'),
    ('ovl1', 0x0000): ('ovl1_startup', "the Tandy startup: WAST.DAT's directory, SCNFT, the Tandy check, the title (WAS.EXE's ovl1_startup)"),
    ('ovl1', 0x15BE): ('version_screen', '"3.0 TANDY 1000"'),
    ('ovl1', 0x2D0C): ('save_options', 'writes SCNFT (WAS.EXE: save_options, SCNF)'),
    ('ovl1', 0x504C): ('tandy_check', '"This version for Tandy 1000 series only."'),
    ('ovl4', 0x72A2): ('boarding_load_b', 'mode 9, the 16-colour deck (WAST.DAT record 29) and the men expanded to 4 bits (expand_2to4) (WAS.EXE: boarding_load_b ovl7:CBC8)'),
}
for f in ALL:
    k = (f.region.name, f.start)
    if k in MANUAL and not (f.match and f.match[0] in ('exact', 'constants')):
        f.match = None
        f.manual = MANUAL[k]
    else:
        f.manual = None


# ------------------------------------------------------------------ names and the DS map
for f in ALL:
    if f.manual:
        f.name = f.manual[0]
    elif f.match:
        f.name = f.match[1] if f.match[1] not in ('', None) else None
    if not f.name:
        f.name = '%s_%04x' % (f.region.name, f.start) if f.region.name != 'rt' else 'rt_%05x' % f.start
NAME_AT = {}
for f in ALL:
    NAME_AT[(f.region.name, f.start)] = f.name

ds_votes = defaultdict(Counter)       # WAST DS offset -> WAS DS offset votes
for f in ALL:
    if not f.match or f.match[0] in ('similar', 'lib'):
        continue
    w = was_insns(f.match[1])
    if len(w) != len(f.insns):
        continue
    for a, b in zip(f.insns, w):
        for oa, ob in zip(a.operands, b.operands):
            if oa.type == x86.X86_OP_MEM and ob.type == x86.X86_OP_MEM and not oa.mem.base and not oa.mem.index \
                    and not ob.mem.base and not ob.mem.index:
                ds_votes[oa.mem.disp & 0xFFFF][ob.mem.disp & 0xFFFF] += 1
DS_MAP = {k: v.most_common(1)[0][0] for k, v in ds_votes.items()}


# ------------------------------------------------------------------ output
def target_name(f, i):
    """the name of a call's target, if known."""
    if i.id <= 0:
        return None
    if i.mnemonic == 'lcall' and len(i.operands) == 2:
        s, o = i.operands[0].imm & 0xFFFF, i.operands[1].imm & 0xFFFF
        lin = s * 16 + o
        if s < OVL_SEG:
            return NAME_AT.get(('rt', lin), 'rt:%05X' % lin)
        p = lin - OVL_SEG * 16
        return NAME_AT.get((f.region.name, p), '%s:%04X' % (f.region.name, p))
    if i.mnemonic == 'call' and i.operands and i.operands[0].type == x86.X86_OP_IMM:
        return NAME_AT.get((f.region.name, i.operands[0].imm & 0xFFFF))
    return None


def comment(f, i):
    c = []
    if i.id <= 0:
        return ''
    if i.mnemonic in CALLS:
        t = target_name(f, i)
        if t:
            c.append(t)
    for op in i.operands:
        if op.type == x86.X86_OP_MEM and not op.mem.base and not op.mem.index:
            v = op.mem.disp & 0xFFFF
            if v in DS_MAP:
                c.append('WAS DS:%04X' % DS_MAP[v])
            if v in WAST_STR and len(WAST_STR[v]) >= 4:
                c.append('"%s"' % WAST_STR[v][:40])
        elif op.type == x86.X86_OP_IMM and i.mnemonic in ('mov', 'push') and (op.imm & 0xFFFF) in WAST_STR \
                and len(WAST_STR[op.imm & 0xFFFF]) >= 4 and (op.imm & 0xFFFF) > 0x40:
            c.append('DS:%04X "%s"' % (op.imm & 0xFFFF, WAST_STR[op.imm & 0xFFFF][:40]))
    return ('   ; ' + '; '.join(c)) if c else ''


def write():
    o = []
    P = o.append
    P('; WAST.EXE - The Ancient Art of War at Sea, Tandy 1000 16-colour version (1987)')
    P('; A matched disassembly made by disassembler/wastdis.py from the unpacked program')
    P('; (tools/disassembly/unexepack.py) and its four MS-LINK overlay modules.')
    P(';')
    P('; Addresses: rt:XXXXX = linear address in the resident code (SEG:OFF with SEG x 16 + OFF),')
    P(';   the same as the offset in the unpacked image; the unpacked file offset is that + %Xh.' % HDR_LEN)
    P(';   ovlN:XXXX = offset in overlay N\'s code (it loads at segment 06C9: linear 6C90h + XXXX).')
    P(';   DS = segment 207A (unpacked image 207A0h). WAS DS:XXXX = the same variable in WAS.EXE, where')
    P(';   matched routines use it in the same place.')
    P('; Each routine: its WAS.EXE name when matched (../WAS.ASM), and how:')
    P(';   exact      the same instructions (addresses ignored)')
    P(';   constants  the same instructions, some constants different (listed)')
    P(';   similar    the same strings and constants, compiled differently (score 0-1)')
    P(';   calls      called in the same place by routines already matched (size ratio)')
    P(';   Tandy:     the Tandy version\'s own, named by hand')
    P(';   (none)     not matched')
    P(';')
    kinds = Counter(f.match[0] if f.match else ('named' if f.manual else 'none') for f in ALL)
    P('; Routines: %d (%s)' % (len(ALL), ', '.join('%s %d' % kv for kv in sorted(kinds.items()))))
    P('; WAS.EXE routines matched: %d of %d' % (len(used_was), len(WAS_FUNCS)))
    P('')
    for r in REGIONS:
        P(';' + '=' * 78)
        P('; %s   code at file offset %X of the unpacked WAST.EXE' % (r.title, r.file_off))
        P(';' + '=' * 78)
        for s in sorted(r.funcs):
            f = r.funcs[s]
            m = f.match
            head = '; ---- %s   %s' % (f.name, f.addr())
            if m and m[0] == 'lib':
                head += '   [Microsoft C library]'
            elif m:
                head += '   [%s %s%s]' % (m[0], WAS_INFO[m[1]]['addr'], '' if m[0] not in ('similar', 'calls') else ' %.2f' % m[2])
            if f.manual:
                head += '   [Tandy: %s]' % f.manual[1]
            P('')
            P(head)
            if m and m[0] in ('similar', 'calls') and m[1] in WAS_INFO:
                a, b = set(f.feats[0]), set(WAS_INFO[m[1]]['feats'][0])
                # (addresses in either data segment left out: below 1000h, or a byte twice as 5555h)
                keep = lambda v: v > 0x10 and (v < 0x1000 or (v >> 8) == (v & 0xFF))
                only_t = sorted(v for v in a - b if keep(v))[:10]
                only_w = sorted(v for v in b - a if keep(v))[:10]
                if only_t or only_w:
                    P(';      constants only here: %s; only in WAS.EXE: %s' % (' '.join('%Xh' % v for v in only_t) or '-',
                                                                           ' '.join('%Xh' % v for v in only_w) or '-'))
            if m and m[0] == 'constants':
                diffs = []
                for a, b in zip(trim(f.norm), trim(ret_far_to_near(WAS_INFO[m[1]]['norm']))):
                    if a != b:
                        diffs.append('%s -> %s' % (' '.join(map(fmt, b)), ' '.join(map(fmt, a))))
                for d in diffs[:12]:
                    P(';      WAS.EXE %s' % d)
            for i in f.insns:
                lin = f.region.base + i.address
                a = 'rt:%05X' % lin if r.name == 'rt' else '%s:%04X' % (r.name, i.address)
                fo = r.file_off + i.address
                P('  %-10s %05X  %-14s %-6s %s%s' % (a, fo, i.bytes.hex(), i.mnemonic, i.op_str, comment(f, i)))
    open(OUT, 'w', encoding='utf-8').write('\n'.join(o) + '\n')


def fmt(x):
    if isinstance(x, int):
        return '%Xh' % x
    if isinstance(x, tuple):
        return '[%s]' % '+'.join(str(v) for v in x[1:4] if v not in ('', None))
    return str(x)


if __name__ == '__main__':
    write()
    kinds = Counter(f.match[0] if f.match else ('named' if f.manual else 'none') for f in ALL)
    print('routines', len(ALL), dict(kinds), 'WAS matched', len(used_was), 'of', len(WAS_FUNCS))
    for r in REGIONS:
        k = Counter(f.match[0] if f.match else ('named' if f.manual else 'none') for f in r.funcs.values())
        print(' ', r.name, len(r.funcs), dict(k))
    # check: does a caller of each matched routine match a caller of WAS.EXE's?
    was_callers = defaultdict(set)
    for k, v in WAS_CALLS.items():
        for t in v:
            was_callers[t].add(k)
    callers = defaultdict(set)
    for g in ALL:
        for c in g.callees:
            if c is not None:
                callers[id(c)].add(g)
    odd = []
    for f in ALL:
        if f.match and f.match[0] == 'similar':
            cs = [g.match[1] for g in callers[id(f)] if g.match]
            if cs and not any(c in was_callers[f.match[1]] for c in cs):
                odd.append('%s %s' % (f.addr(), f.name))
    print('matched by strings and constants, no caller in common:', len(odd))
    for o in odd:
        print('  ', o)
