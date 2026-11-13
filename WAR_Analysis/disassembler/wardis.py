"""Disassembly engine for WAR.EXE.

Needs capstone (pip install capstone). Reads ../../WAR_Original_Files_3.5/WAR.EXE.

WAR.EXE (The Ancient Art of War, CGA version).

Builds a model of the program: regions with their real load addresses,
recursive-descent disassembly, call graph, relocations and data-segment
string references. gen_asm.py turns the model into WAR.ASM.
"""
import os
import re
import struct
from capstone import Cs, CS_ARCH_X86, CS_MODE_16, CS_GRP_JUMP, CS_GRP_CALL, CS_GRP_RET, CS_GRP_IRET
from capstone import x86

EXE = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', 'WAR_Original_Files_3.5', 'WAR.EXE')
D = open(EXE, 'rb').read()
HDR = struct.unpack('<14H', D[:28])
HEADER_SIZE = HDR[4] * 16
IMG = D[HEADER_SIZE:]
RELOCS = [struct.unpack('<HH', D[HDR[12] + 4 * i:HDR[12] + 4 * i + 4]) for i in range(HDR[3])]
RELOC_IMG = {seg * 16 + off for off, seg in RELOCS}          # image offsets of fixed-up words

md = Cs(CS_ARCH_X86, CS_MODE_16)
md.detail = True

# ------------------------------------------------------------------ layout
# Overlay descriptor table (frame 0x2F6, image 0x2F60): 16-byte entries.
OVL_TABLE_IMG = 0x2F60


def overlay_table():
    rows = []
    base = OVL_TABLE_IMG
    k = 0
    while True:
        e = IMG[base + k * 16:base + k * 16 + 16]
        f = struct.unpack('<8H', e)
        if f[2] == 0xFFFF:
            break
        nxt = struct.unpack('<8H', IMG[base + (k + 1) * 16:base + (k + 2) * 16])
        rows.append(dict(index=k + 1, flags=f[2], load_seg=f[4], load_end=f[5], name_ptr=f[6],
                         file_para=f[7], size_para=nxt[1]))
        k += 1
    return rows


OVERLAYS = overlay_table()
# DGROUP (initialised data) is the last table entry, flagged 0x4000 = load at startup
DATA_ENTRY = [o for o in OVERLAYS if o['flags'] & 0x4000][0]
DS_SEG = DATA_ENTRY['load_seg']                     # 0x8C6
DS_FILE = DATA_ENTRY['file_para'] * 16              # 0x172E0
DS_SIZE = DATA_ENTRY['size_para'] * 16              # 0xB40
DSEG = D[DS_FILE:DS_FILE + DS_SIZE]

THUNK_IMG = 0x2FEE
THUNKS = []                                          # (image offset, overlay#, target offset)
for k in range(21):
    a = THUNK_IMG + 12 * k
    t = IMG[a:a + 12]
    assert t[0] == 0x9A and t[7] == 0xEA
    THUNKS.append((a, struct.unpack('<H', t[5:7])[0], struct.unpack('<H', t[8:10])[0]))


class Region:
    def __init__(self, name, kind, file_start, size, cs, origin, title, note=''):
        self.name, self.kind, self.file_start, self.size = name, kind, file_start, size
        self.cs, self.origin, self.title, self.note = cs, origin, title, note
        self.data = D[file_start:file_start + size]
        self.insns = {}          # offset -> capstone insn
        self.code = set()        # offsets of code bytes
        self.labels = {}         # offset -> name
        self.comments = {}       # offset -> list of comments
        self.func_starts = set()

    def contains(self, off):
        return self.origin <= off < self.origin + self.size

    def file_of(self, off):
        return self.file_start + (off - self.origin)


def build_regions():
    R = []
    R.append(Region('rt', 'code', 0x200, 0x28D0, 0x0000, 0x0000,
                    'C RUN-TIME LIBRARY + GAME CORE (segment 0000, CS = load base)',
                    'Resident. Compiled C code (BP-frame prologues) and library helpers. '
                    'Overlays run in this same segment, so their near calls into 0000-28CF land here.'))
    R.append(Region('root', 'code', 0x2AD0, 0x051E, 0x028D, 0x0000,
                    'ROOT: STARTUP AND PHOENIX OVERLAY LOADER (segment 028D)',
                    'Program entry 028D:02FA. Overlay paging, DOS file I/O for overlays.'))
    R.append(Region('ldat', 'data', 0x2FEE, 0x172, 0x02DE, 0x000E,
                    'LOADER DATA (segment 02DE)', 'Overlay loader variables and $-terminated messages.'))
    for o in OVERLAYS:
        if o['flags'] & 0x4000:
            continue
        n = o['index']
        R.append(Region(f'ovl{n}', 'code', o['file_para'] * 16, o['size_para'] * 16, 0x0000,
                        o['load_seg'] * 16,
                        f'OVERLAY {n} (loads at 0000:{o["load_seg"] * 16:04X}-{o["load_end"] * 16 - 1:04X})'))
    R.append(Region('dseg', 'data', DS_FILE, DS_SIZE, DS_SEG, 0x0000,
                    f'DATA SEGMENT DGROUP (segment {DS_SEG:04X}, preloaded at startup)',
                    'Initialised data: strings, tables, 8x8 font. BSS and stack follow in memory '
                    f'up to segment {DATA_ENTRY["load_end"]:04X}.'))
    return R


REGIONS = build_regions()
RG = {r.name: r for r in REGIONS}

# ------------------------------------------------------------------ descent
FLOW_END = {'ret', 'retf', 'iret', 'jmp', 'ljmp', 'hlt'}


def imm_target(insn):
    if insn.operands and insn.operands[0].type == x86.X86_OP_IMM:
        return insn.operands[0].imm & 0xFFFF
    return None


def descend(region, seeds):
    work = [s for s in seeds if region.contains(s)]
    data = region.data
    while work:
        off = work.pop()
        while region.contains(off) and off not in region.insns:
            i = off - region.origin
            insn = next(md.disasm(data[i:i + 16], off), None)
            if insn is None or i + insn.size > len(data):
                break
            # refuse to decode into bytes already claimed by another instruction
            if any((off + k) in region.code for k in range(insn.size)):
                break
            region.insns[off] = insn
            for k in range(insn.size):
                region.code.add(off + k)
            m = insn.mnemonic
            if insn.group(CS_GRP_JUMP) or insn.group(CS_GRP_CALL):
                t = imm_target(insn)
                if t is not None and m not in ('ljmp', 'lcall'):
                    if region.contains(t):
                        work.append(t)
                    if insn.group(CS_GRP_CALL):
                        CALLS.append((region.name, off, t))
            if m in FLOW_END or insn.group(CS_GRP_RET) or insn.group(CS_GRP_IRET):
                break
            off += insn.size


CALLS = []


def resolve(src, t):
    """Region a near call target belongs to, seen from region src."""
    if RG[src].contains(t):
        return src
    if RG['rt'].contains(t):
        return 'rt'
    if src in ('ovl1', 'ovl2', 'rt') and RG['ovl2'].contains(t):
        return 'ovl2'
    if src in ('ovl2',) and RG['ovl1'].contains(t):
        return 'ovl1'
    return None


def prologue_seeds(region):
    s = set()
    for m in re.finditer(rb'\x55\x8b\xec', region.data):
        s.add(region.origin + m.start())
    return s


def run():
    seeds = {r.name: set() for r in REGIONS}
    seeds['root'] |= {HDR[10]}                           # entry IP 02FA
    seeds['root'] |= {0x0287, 0x02D0}                   # overlay loader
    for a, ovl, tgt in THUNKS:
        seeds[f'ovl{ovl}'].add(tgt)
    for r in REGIONS:
        if r.kind == 'code':
            seeds[r.name] |= prologue_seeds(r)
            if r.name.startswith('ovl'):
                seeds[r.name].add(r.origin)
    seeds['rt'].add(0)
    # iterate: calls from any region into rt / ovl2 (always-addressable code)
    for _ in range(6):
        for r in REGIONS:
            if r.kind == 'code':
                descend(r, seeds[r.name])
        new = False
        for src, off, t in CALLS:
            tgt = resolve(src, t)
            if tgt and t not in seeds[tgt]:
                seeds[tgt].add(t)
                new = True
        # far calls
        for r in REGIONS:
            for off, insn in r.insns.items():
                if insn.mnemonic == 'lcall' and len(insn.operands) == 2:
                    seg, t = insn.operands[0].imm & 0xFFFF, insn.operands[1].imm & 0xFFFF
                    if seg == 0x028D and t not in seeds['root']:
                        seeds['root'].add(t); new = True
                    if seg == 0x0000 and t not in seeds['rt']:
                        seeds['rt'].add(t); new = True
        if not new:
            break
    for r in REGIONS:
        if r.kind == 'code':
            r.func_starts = {t for src, _, t in CALLS if resolve(src, t) == r.name} | prologue_seeds(r)
            if r.name.startswith('ovl'):
                r.func_starts |= {tg for a, ov, tg in THUNKS if f'ovl{ov}' == r.name}
            if r.name == 'root':
                r.func_starts |= {HDR[10], 0x0287, 0x02D0}
                for rr in REGIONS:
                    for o, ins in rr.insns.items():
                        if ins.mnemonic == 'lcall' and len(ins.operands) == 2 and ins.operands[0].imm & 0xFFFF == 0x028D:
                            r.func_starts.add(ins.operands[1].imm & 0xFFFF)


run()
