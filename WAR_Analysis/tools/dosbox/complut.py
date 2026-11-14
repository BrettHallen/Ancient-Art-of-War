"""complut.py PATTERN.json SHOTS-DIR [OUT.c]: the composite colour table from DOSBox-X screenshots of CGAPAT.COM
(cgapat.py): for each CGA (cga_composite, cga_composite2), palette, phase of 4 hdots and pixel triple (left, pixel,
right), the colour of the hdot. The shots are SHOTS-DIR/pat_MACHINE_p1.png (palette 1) and pat_MACHINE_p0.png (after
a key: palette 0), 640x400 (DOSBox-X's normal2x). Reports any entry seen in two colours, writes composite_lut.json
and, given OUT.c, the table as WAR_Remake/src/composite.c has it."""
import sys, json
from PIL import Image
from collections import defaultdict, Counter
scr = json.load(open(sys.argv[1])); shots = sys.argv[2]
out = {}; luts = {}
for m in ('cga_composite', 'cga_composite2'):
    tab = defaultdict(Counter)
    for pal, shot in ((1, 'p1'), (0, 'p0')):
        p = Image.open('%s/pat_%s_%s.png' % (shots, m, shot)).convert('RGB').load()
        for y in range(200):
            line = scr[y]
            if not any(line): continue
            for x in range(640):
                i = x // 2
                key = (pal, x % 4) + tuple(line[k] if 0 <= k < 320 else 0 for k in (i - 1, i, i + 1))
                tab[key][p[x, 2 * y]] += 1
    incons = sum(1 for c in tab.values() if len(c) > 1)
    lut = {k: c.most_common(1)[0][0] for k, c in tab.items()}
    print(m, 'entries', len(lut), 'of 512, seen in more than one colour', incons)
    luts[m] = lut
    out[m] = {'%d,%d,%d,%d,%d' % k: '%02X%02X%02X' % c for k, c in lut.items()}
json.dump(out, open('composite_lut.json', 'w'))
if len(sys.argv) > 3:
    w = ['static const uint32_t COMPOSITE[2][2][4][64] = {']
    for n, m in enumerate(('cga_composite', 'cga_composite2')):
        w.append('    { /* the %s CGA */' % ('old', 'new')[n])
        for pal in (0, 1):
            w.append('        { /* palette %d */' % pal)
            for ph in range(4):
                v = []
                for l in range(4):
                    for c in range(4):
                        for r in range(4):
                            rgb = luts[m].get((pal, ph, l, c, r), (0, 0, 0))
                            v.append('0x%02X%02X%02X' % rgb)
                rows = [', '.join(v[i:i + 8]) for i in range(0, 64, 8)]
                w.append('            { ' + (',\n              ').join(rows) + ' },')
            w.append('        },')
        w.append('    },')
    w.append('};')
    open(sys.argv[3], 'w').write('\n'.join(w) + '\n')
