"""vgacmp.py REMAKE.idx SHOT.png: the remake's VGA screen (indexes + palette) against WARET.EXE on a VGA in
DOSBox (colours within 10 a channel: the DAC has 6 bits). Groups differing pixels by (remake index -> original colour;
which index of the remake's palette that colour is). REMAKE.idx comes from WAR_Remake/test/dump_cards."""
import sys
from PIL import Image
from collections import defaultdict
d=open(sys.argv[1],'rb').read(); a=d[:64000]; pal=[tuple(d[64000+3*i:64003+3*i]) for i in range(16)]
near=lambda a,b: all(abs(x-y)<=10 for x,y in zip(a,b))
im=Image.open(sys.argv[2]).convert('RGB').resize((320,200),Image.NEAREST); p=im.load()
g=defaultdict(list)
for k in range(64000):
    c=p[k%320,k//320]
    if not near(pal[a[k]],c): g[(a[k],c)].append(k)
print('%s: %d pixels differ'%(sys.argv[2],sum(len(v) for v in g.values())))
for (ra,c),ks in sorted(g.items(),key=lambda t_:-len(t_[1]))[:14]:
    xs=[k%320 for k in ks]; ys=[k//320 for k in ks]
    cand=[i for i in range(16) if near(pal[i],c)]
    print('  remake %2d -> %02X%02X%02X (index %s): %5d px, x %d-%d y %d-%d'%(ra,*c,cand,len(ks),min(xs),max(xs),min(ys),max(ys)))
