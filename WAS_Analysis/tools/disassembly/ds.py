#!/usr/bin/env python3
# ds.py [-t] OFF... : what is at DS offsets of WAS.EXE (or, with -t, WAST.EXE)
#   OFF       the C string there (ds.py 0x1234)
#   OFF:N     N bytes there in hex (ds.py 0x2e2d:6)
#   OFF:Nw    N words there in hex (ds.py 0x2e2d:3w)
#   -s TEXT   the offsets where the string TEXT starts
# WAS.EXE's data segment is the overlay table's entry flagged 4000h (as in
# ../../disassembler/wasdis.py); WAST.EXE's is segment 207A of the image
# unpacked by unexepack.py (as in ../../disassembler/wastdis.py).
# WAS_EXE / WAST_EXE in the environment choose other copies.
import os, struct, sys

HERE = os.path.dirname(os.path.abspath(__file__))
GAME = os.path.join(HERE, '..', '..', '..', 'WAS_Original_Files_3.5')


def was_ds():
    d = open(os.environ.get('WAS_EXE', os.path.join(GAME, 'WAS.EXE')), 'rb').read()
    hdr = struct.unpack('<H', d[8:10])[0] * 16
    img = d[hdr:]
    root_seg = struct.unpack('<H', d[22:24])[0]            # the header's CS
    tbl = root_seg * 16 + 0x051E + 0x172                   # 72E0h, as wasdis.py
    k = 0
    while True:
        f = struct.unpack('<8H', img[tbl + k * 16:tbl + k * 16 + 16])
        if f[2] == 0xFFFF:
            sys.exit('ds.py: no data segment in the overlay table')
        if f[2] & 0x4000:
            size = struct.unpack('<8H', img[tbl + (k + 1) * 16:tbl + (k + 2) * 16])[1] * 16
            return d[f[7] * 16:f[7] * 16 + size]
        k += 1


def wast_ds():
    sys.path.insert(0, HERE)
    from unexepack import unpack
    exe, _ovl, _info = unpack(open(os.environ.get('WAST_EXE', os.path.join(GAME, 'WAST.EXE')), 'rb').read())
    img = exe[struct.unpack('<H', exe[8:10])[0] * 16:]
    return img[0x207A * 16:]


def main(args):
    if not args:
        sys.exit('usage: ds.py [-t] OFF[:N[w]]... | ds.py [-t] -s TEXT')
    if args[0] == '-t':
        ds = wast_ds(); args = args[1:]
    else:
        ds = was_ds()
    if args and args[0] == '-s':
        t = ' '.join(args[1:]).encode('latin1')
        i = ds.find(t)
        while i >= 0:
            print(hex(i)); i = ds.find(t, i + 1)
        return
    for a in args:
        o = int(a.split(':')[0], 0)
        if o >= len(ds):
            print(hex(o), '(past the initialised data, %Xh bytes: set at run time)' % len(ds)); continue
        if ':' in a:
            off, n = a.split(':')
            o = int(off, 0)
            if n.endswith('w'):
                n = int(n[:-1], 0)
                ws = struct.unpack('<%dH' % n, ds[o:o + 2 * n])
                print(hex(o), ' '.join('%04X' % w for w in ws))
            else:
                n = int(n, 0)
                print(hex(o), ds[o:o + n].hex(' ').upper())
        else:
            o = int(a, 0)
            e = ds.index(b'\0', o)
            print(hex(o), repr(ds[o:e].decode('latin1')))


if __name__ == '__main__':
    main(sys.argv[1:])
