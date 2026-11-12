import sys, os; sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '../../disassembler'))
import wardis as W, re, bisect
from capstone import x86
# string starts in DS
STR={}
for m in re.finditer(rb'[\x20-\x7e]{3,}',W.DSEG):
    STR[m.start()]=m.group().decode()
def funcs(r):
    fs=sorted(r.func_starts); return fs
def func_of(r,off):
    fs=funcs(r); i=bisect.bisect_right(fs,off)-1
    return fs[i] if i>=0 else None
INFO={}  # (region,func)->dict
def info(r,f): return INFO.setdefault((r.name,f),{'strings':set(),'ints':set(),'calls':set(),'thunks':set(),'imm':set()})
lastah={}
for r in W.REGIONS:
    if r.kind!='code': continue
    prev=None
    for off in sorted(r.insns):
        ins=r.insns[off]; f=func_of(r,off)
        if f is None: continue
        I=info(r,f)
        ops=ins.operands
        if ins.mnemonic in('mov','push') and ops and ops[-1].type==x86.X86_OP_IMM:
            v=ops[-1].imm&0xffff
            if v in STR and v>=0x10: I['strings'].add(v)
        if ins.mnemonic=='int':
            n=ops[0].imm; ah=None
            # find preceding mov ah/ax
            for back in range(1,6):
                p=r.insns.get(prev_off(r,off,back)) if False else None
            I['ints'].add(n)
        if ins.mnemonic=='call' and ops and ops[0].type==x86.X86_OP_IMM:
            t=ops[0].imm&0xffff
            if 0x2fee<=t<0x30ea and (t-0x2fee)%12==0:
                I['thunks'].add((t-0x2fee)//12)
            else: I['calls'].add(t)
        if ins.mnemonic=='lcall' and len(ops)==2:
            I['calls'].add(('far',ops[0].imm&0xffff,ops[1].imm&0xffff))
