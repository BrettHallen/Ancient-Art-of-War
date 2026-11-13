"""egacmp.py REMAKE.idx SHOT.png [--rows N]: compare the remake's palette indexes (WAR_Remake/test/dump_cards)
with WARET.EXE's screen in DOSBox-X on an EGA or a Tandy 1000 (the standard 16 colours, so each colour is one index).
Prints the pixels that differ grouped by (remake index -> original index) with their box."""
import sys
from PIL import Image
from collections import defaultdict
STD=[(0,0,0),(0,0,0xAA),(0,0xAA,0),(0,0xAA,0xAA),(0xAA,0,0),(0xAA,0,0xAA),(0xAA,0x55,0),(0xAA,0xAA,0xAA),
     (0x55,0x55,0x55),(0x55,0x55,0xFF),(0x55,0xFF,0x55),(0x55,0xFF,0xFF),(0xFF,0x55,0x55),(0xFF,0x55,0xFF),(0xFF,0xFF,0x55),(0xFF,0xFF,0xFF)]
IDX={c:i for i,c in enumerate(STD)}
def shot_indexes(path):
    im=Image.open(path).convert('RGB')
    if im.size!=(320,200): im=im.resize((320,200),Image.NEAREST)
    p=im.load(); out=[]
    for y in range(200):
        for x in range(320):
            out.append(IDX.get(p[x,y],-1))
    return out
def remake_indexes(path):
    d=open(path,'rb').read(); return list(d[:64000])
if __name__=='__main__':
    a=remake_indexes(sys.argv[1]); b=shot_indexes(sys.argv[2])
    rows=int(sys.argv[sys.argv.index('--rows')+1]) if '--rows' in sys.argv else 200
    g=defaultdict(list)
    for k in range(320*rows):
        if a[k]!=b[k]: g[(a[k],b[k])].append(k)
    tot=sum(len(v) for v in g.values())
    print('%s vs %s: %d pixels differ'%(sys.argv[1],sys.argv[2],tot))
    for (ra,rb),ks in sorted(g.items(),key=lambda t:-len(t[1]))[:25]:
        xs=[k%320 for k in ks]; ys=[k//320 for k in ks]
        print('  remake %2d -> original %2d: %6d px, x %d-%d y %d-%d'%(ra,rb,len(ks),min(xs),max(xs),min(ys),max(ys)))
