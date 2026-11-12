#!/usr/bin/env python3
"""Mini decompiler for WAS.EXE.asm functions (copied from WAR_Analysis/tools/disassembly) (compiled C, simple idioms).
Usage: dec.py NAME|region_ADDR  -> C-like pseudocode with gotos."""
import re, sys

import os
ASM = os.environ.get('WAS_ASM', os.path.join(os.path.dirname(os.path.abspath(__file__)), '../../WAS.EXE.asm'))
lines = open(ASM).read().split('\n')

NAMES = {
    # squadrons (40) and ships (40): WAS_file_reference.md
    0x5cdd: 'sq_side', 0x5d2d: 'sq_speed', 0x5d7d: 'sq_x', 0x5dcd: 'sq_y', 0x5e1d: 'sq_supplies',
    0x5e45: 'ship_sq', 0x5e6d: 'ship_sails', 0x5e95: 'ship_type', 0x6025: 'ship_hull', 0x609f: 'ship_names',
    0x5ebd: 'sq_mark', 0x5f85: 'sq_addr', 0x5fd5: 'sq_heading', 0x56ff: 'sq_wreck', 0x574f: 'sq_seen',
    0x607f: 'heading_spr', 0x5bff: 'map', 0x4e53: 'story', 0x646f: 'OPPONENT', 0x64ad: 'rules',
    0x3f37: 'BUF', 0x684c: 'CNF',
}
def nm(v):
    v &= 0xFFFF
    if v in NAMES: return NAMES[v]
    return '0x%X' % v

def find(name):
    for i, l in enumerate(lines):
        if l == name + ':': return i
    reg, addr = name.split('_')
    pat = '  %s:%s ' % (reg, addr.upper())
    for i, l in enumerate(lines):
        if l.startswith(pat):
            j = i
            while not re.match(r'^[A-Za-z_0-9]+:$', lines[j]): j -= 1
            return j
    raise SystemExit('not found')

def body(start):
    out = []
    for l in lines[start + 1:]:
        if l.startswith('; ----'): break
        m = re.match(r'\s+\S+:([0-9A-F]{4})\s+[0-9A-F]{5}\s+\S+\s+(\S+)\s*(.*?)\s*(;.*)?$', l)
        if m: out.append((int(m.group(1), 16), m.group(2), m.group(3)))
    return out

class D:
    def __init__(s): s.r = {}; s.stack = []; s.cmp = None
    def reset(s): s.r = {}; s.cmp = None
    def g(s, x):
        x = x.strip()
        if x in ('ax','bx','cx','dx','si','di','al','cl','dl'):
            k = {'al':'ax','cl':'cx','dl':'dx'}.get(x, x)
            return s.r.get(k, k)
        m = re.match(r'^(?:word ptr |byte ptr )?\[(.*)\]$', x)
        if m: return s.mem(m.group(1), 'byte' in x)
        m = re.match(r'^-?(0x[0-9a-f]+|\d+)$', x)
        if m:
            v = int(x, 0)
            if v > 0x7fff: v -= 0x10000
            return str(v) if abs(v) < 0x100 else nm(v)
        return x
    def mem(s, a, byte=False):
        a = a.replace(' ', '')
        m = re.match(r'^bp([+-])(0x[0-9a-f]+|\d+)$', a)
        if m:
            off = int(m.group(2), 0)
            return ('a%d' % ((off - 4) // 2)) if m.group(1) == '+' else ('l%d' % off)
        m = re.match(r'^(0x[0-9a-f]+)$', a)
        if m: return nm(int(a, 16))
        m = re.match(r'^(si|di|bx)$', a)
        if m:
            e = s.r.get(a, a)
            mm = re.match(r'^(\w+)\+\((.*)\)\*2$', e)
            if mm: return '%s[%s]' % (mm.group(1), mm.group(2))
            mm = re.match(r'^(\w+)\+(.*)$', e)
            if mm: return '%s{%s}' % (mm.group(1), mm.group(2))
            return '*(%s)' % e
        m = re.match(r'^(si|di|bx)\+(0x[0-9a-f]+|\d+)$', a)
        if m: return '*(%s+%s)' % (s.r.get(m.group(1), m.group(1)), m.group(2))
        return '[%s]' % a
    def st(s, dst, val, emit):
        dst = dst.strip()
        if dst in ('ax','bx','cx','dx','si','di','al','cl','dl'):
            k = {'al':'ax','cl':'cx','dl':'dx'}.get(dst, dst)
            s.r[k] = val
        else:
            emit('%s = %s;' % (s.g(dst), val))

def binop(a, op, b):
    return '(%s %s %s)' % (a, op, b)

CJ = {'je':'==','jne':'!=','jl':'<','jle':'<=','jg':'>','jge':'>=','jb':'<u','jae':'>=u','ja':'>u','jbe':'<=u'}

def decompile(name):
    st = find(name)
    ins = body(st)
    targets = set()
    for a, mn, ops in ins:
        if mn.startswith('j') and re.match(r'0x[0-9a-f]+', ops):
            targets.add(int(ops, 16))
    d = D(); out = []
    emit = out.append
    incoming = {}
    prev_uncond = False
    for a, mn, ops in ins:
        if a in targets:
            emit('L%04X:' % a)
            states = list(incoming.get(a, []))
            if not prev_uncond: states.append(dict(d.r))
            d.reset()
            if states:
                for k in ('ax','si','dx'):
                    vals = []
                    for st_ in states:
                        v = st_.get(k, k)
                        if v not in vals: vals.append(v)
                    if len(vals) == 1 and vals[0] != k: d.r[k] = vals[0]
                    elif len(vals) > 1 and all(v != k for v in vals) and len(' | '.join(vals)) < 200:
                        d.r[k] = '{' + ' | '.join(vals) + '}'
        prev_uncond = mn in ('jmp', 'ret')
        o = [x.strip() for x in re.split(r',(?![^\[]*\])', ops)] if ops else []
        if mn in ('push',) and o[0] == 'bp': continue
        if mn == 'mov' and o == ['bp', 'sp']: continue
        if mn == 'sub' and o[0] == 'sp': continue
        if mn == 'pop' and o[0] == 'bp': continue
        if mn == 'mov':
            if o[1] == 'sp' or o[0] == 'sp': continue
            v = d.g(o[1])
            # si = ARR ; add si, (x<<1)
            d.st(o[0], v, emit)
        elif mn == 'push':
            d.stack.append(d.g(o[0]))
        elif mn == 'pop':
            if d.stack:
                v = d.stack.pop()
                if o[0] not in ('di',): d.st(o[0], v, emit)
            elif o[0] != 'di':
                d.r[o[0]] = o[0]
        elif mn == 'call':
            args = list(reversed(d.stack)); d.stack = []
            f = o[0]
            d.r['ax'] = '%s(%s)' % (f, ', '.join(args))
            # emit call as statement too; keep ax as call result
            emit('ax = %s;' % d.r['ax']); d.r['ax'] = 'ax'; d.r.pop('dx', None)
        elif mn == 'add' and o[0] == 'sp':
            continue
        elif mn in ('add','sub','and','or','xor','imul','shl','sar','shr'):
            if mn == 'imul' and len(o) == 1:
                d.r['ax'] = binop(d.g('ax'), '*', d.g(o[0])); continue
            if mn in ('or','and','xor') and len(o) == 2 and o[0] == o[1]:
                if mn == 'xor': d.st(o[0], '0', emit)
                else: d.cmp = (d.g(o[0]), '0')
                continue
            sym = {'add':'+','sub':'-','and':'&','or':'|','xor':'^','imul':'*','shl':'<<','sar':'>>','shr':'>>>'}[mn]
            src = d.g(o[1]) if len(o) > 1 else '1'
            if src == 'cx' or src == d.r.get('cx'): src = d.g('cx')
            cur = d.g(o[0])
            if mn == 'add' and o[0] in ('si','di','bx'):
                mm = re.match(r'^\(?(.*?) << 1\)?$', src)
                if mm: d.r[o[0]] = '%s+(%s)*2' % (cur, mm.group(1)); continue
                d.r[o[0]] = '%s+%s' % (cur, src); continue
            val = binop(cur, sym, src)
            if mn == 'shl' and src == '1': val = '(%s << 1)' % cur
            d.st(o[0], val, emit)
        elif mn in ('inc','dec'):
            if o[0] in ('ax','bx','cx','dx','si','di'):
                d.r[o[0]] = binop(d.g(o[0]), '+' if mn == 'inc' else '-', '1')
            else:
                emit('%s%s;' % (d.g(o[0]), '++' if mn == 'inc' else '--'))
        elif mn == 'neg':
            d.st(o[0], '-%s' % d.g(o[0]), emit)
        elif mn in ('cwde', 'cdq', 'cbw', 'cwd', 'cld'):
            continue
        elif mn == 'idiv':
            num, den = d.g('ax'), d.g(o[0])
            d.r['ax'] = '(%s / %s)' % (num, den); d.r['dx'] = '(%s %% %s)' % (num, den)
        elif mn == 'cmp':
            d.cmp = (d.g(o[0]), d.g(o[1]))
        elif mn in CJ:
            incoming.setdefault(int(o[0], 16), []).append(dict(d.r))
            if d.cmp: emit('if (%s %s %s) goto L%s;' % (d.cmp[0], CJ[mn], d.cmp[1], o[0][2:].upper()))
            else: emit('%s L%s;' % (mn, o[0][2:].upper()))
        elif mn == 'jmp':
            incoming.setdefault(int(o[0], 16), []).append(dict(d.r))
            emit('goto L%s;' % o[0][2:].upper())
        elif mn == 'ret':
            emit('return %s;' % d.g('ax'))
        else:
            emit('/* %s %s */' % (mn, ops))
    return out

if __name__ == '__main__':
    for l in decompile(sys.argv[1]):
        print(l if l.endswith(':') else '  ' + l)
