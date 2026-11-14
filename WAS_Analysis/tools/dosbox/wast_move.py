#!/usr/bin/env python3
"""wast_move.py WAST.DAT N S X Y [S X Y ...]: in campaign file SN inside a
(DOSBox copy of) WAST.DAT, move squadron S to byte X, line pair Y. WAST.EXE
reads the campaigns from WAST.DAT (records 40-50), not the loose S files. The
record is re-coded in place, so the file keeps its size (WAST.EXE says
"WAST.DAT Not found" for a longer file)."""
import struct
import sys


def rle_dec(p, size):
    out, i = bytearray(), 0
    while len(out) < size:
        n = p[i] - 256 if p[i] > 127 else p[i]; i += 1
        if n >= 0: out += p[i:i + n + 1]; i += n + 1
        else: out += bytes([p[i]]) * -n; i += 1
    return bytes(out[:size])


def rle_enc(d):
    out, lit, i = bytearray(), bytearray(), 0
    def flush():
        nonlocal lit
        while lit:
            out.append(len(lit[:128]) - 1); out.extend(lit[:128]); lit = lit[128:]
    while i < len(d):
        j = i
        while j < len(d) and d[j] == d[i] and j - i < 128: j += 1
        if j - i >= 3: flush(); out += bytes([(256 - (j - i)) & 255, d[i]]); i = j
        else: lit.append(d[i]); i += 1
    flush()
    return bytes(out)


path, n = sys.argv[1], int(sys.argv[2])
d = bytearray(open(path, 'rb').read())
do = struct.unpack('<I', d[:4])[0]
dr = dict(struct.unpack('<HI', d[do + 5 + 6 * i:do + 11 + 6 * i]) for i in range(struct.unpack('<H', d[do + 3:do + 5])[0]))
off = dr[40 + n]
ln = struct.unpack('<H', d[off:off + 2])[0]
p = d[off + 3:off + 3 + ln]
size = struct.unpack('<H', p[:2])[0]
s = bytearray(rle_dec(p[2:], size))
a = [int(v) for v in sys.argv[3:]]
for k in range(0, len(a), 3):
    sq, x, y = a[k:k + 3]
    s[0x410 + 160 + 2 * sq:0x410 + 162 + 2 * sq] = struct.pack('<h', x)
    s[0x410 + 240 + 2 * sq:0x410 + 242 + 2 * sq] = struct.pack('<h', y)
np = struct.pack('<H', size) + rle_enc(bytes(s))
assert len(np) <= ln, 'the record got longer'
d[off:off + 3] = struct.pack('<HB', len(np), sum(np) & 255)
d[off + 3:off + 3 + ln] = np + bytes(ln - len(np))
open(path, 'wb').write(d)
