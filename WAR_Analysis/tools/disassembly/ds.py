#!/usr/bin/env python3
# ds.py OFF... : print the C strings at DS offsets of WAR.EXE
import sys, os, struct
d=open(os.environ.get('WAR_EXE', os.path.join(os.path.dirname(os.path.abspath(__file__)), '../../../WAR_Original_Files_3.5/WAR.EXE')),'rb').read()
img=struct.unpack('<H',d[8:10])[0]*16
tbl=img+0x2F60
ds=None
for k in range(32):
    e=tbl+k*16
    fl=struct.unpack('<H',d[e+4:e+6])[0]
    if fl==0xFFFF: break
    if fl&0x4000: ds=struct.unpack('<H',d[e+14:e+16])[0]*16; break
def s(off):
    e=d.index(b'\0',ds+off); return d[ds+off:e].decode('latin1')
for a in sys.argv[1:]:
    o=int(a,0); print(hex(o), repr(s(o)))
