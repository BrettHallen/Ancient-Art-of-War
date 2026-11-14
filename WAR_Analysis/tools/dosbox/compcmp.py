"""compcmp.py REMAKE.rgb SHOT.png: the remake's composite picture (640x200 XRGB) against DOSBox-X's (640x400)."""
import sys, struct
from PIL import Image
d=open(sys.argv[1],'rb').read()
im=Image.open(sys.argv[2]).convert('RGB'); p=im.load()
bad=[]
for y in range(200):
    for x in range(640):
        v=struct.unpack_from('<I',d,(y*640+x)*4)[0]
        c=((v>>16)&255,(v>>8)&255,v&255)
        if c!=p[x,2*y]: bad.append((x,y))
if bad:
    xs=[b[0] for b in bad]; ys=[b[1] for b in bad]
    print(sys.argv[2],len(bad),'hdots differ: x %d-%d y %d-%d'%(min(xs),max(xs),min(ys),max(ys)))
else: print(sys.argv[2],'identical')
