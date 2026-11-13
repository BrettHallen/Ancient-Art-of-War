# The Ancient Art of War at Sea (1987) - remake
# Nov 2026 using Claude Opus 5.5
# "Guided" by Brett Hallen
# Original game by Dave Murry & Barry Murry
# Published by Brøderbund Software Inc.
#
# diff.py: the remake against WAS.EXE, routine by routine
"""Differential test: the remake's ovl4 routines against WAS.EXE's under unicorn.
Each sample: the remake plays a campaign on for some frames, its state is
written out in WAS.EXE's data-segment layout (dsmap.c), the routine runs in
both from that state, and the two data segments are compared byte by byte.
usage: diff.py FUNC CAMPAIGNS SEEDS SAMPLES [FRAMES]
  FUNC: turn strength pvalue encounters move eat battles restock ports
  e.g. diff.py turn 0,1,2,3,4,5,6,7,8,9,10 1,2 10"""
import ctypes, os, sys, struct
import was_emu
HERE = os.path.dirname(os.path.abspath(__file__))
lib = ctypes.CDLL(os.path.join(HERE, 'libwas.so'))
GAME = was_emu.GAME.encode()
assert lib.shim_init(GAME)
NAMES = [(0x5CDD,80,'side'),(0x5D2D,80,'speed'),(0x5D7D,80,'x'),(0x5DCD,80,'y'),(0x5E1D,40,'supplies'),(0x5E45,40,'ship_sq'),
 (0x5E6D,40,'sails'),(0x5E95,40,'type'),(0x5EBD,40,'mark'),(0x5EE5,80,'ctrl'),(0x5F35,80,'mdest'),(0x5F85,80,'addr'),(0x5FD5,80,'heading'),
 (0x6025,40,'hull'),(0x56FF,80,'partner'),(0x574F,80,'seen'),(0x579F,80,'path_pos'),(0x57EF,80,'route_pos'),(0x583F,80,'has_path'),
 (0x588F,80,'has_route'),(0x56AF,80,'countdown'),(0x5687,40,'routed'),(0x64E1,80,'move_delay'),(0x6316,80,'dest'),(0x6581,40,'port_speed'),
 (0x3F3D,400,'came'),(0x40CD,400,'cost'),(0x425D,400,'coast'),(0x43ED,400,'danger'),(0x457D,200,'queue'),(0x4645,400,'strength'),
 (0x47D5,400,'pvalue'),(0x4965,400,'port_sup'),(0x4E53,1500,'path'),(0x542F,600,'route'),(0x5BFF,200,'map'),(0x689C,150,'port_cell'),(0x6932,150,'port_src'),
 (0x0808,4,'rnd'),(0x6456,3,'list')]
def name(a):
    for b,n,s in NAMES:
        if b <= a < b+n: return '%s[%d]' % (s, (a-b)//(1 if n in (40,200,600,1500,3,4) else 2))
    return 'DS:%04X' % a
def dump(base=None):
    buf = (ctypes.c_uint8 * 0x10000)()
    if base is not None: ctypes.memmove(buf, bytes(base), 0x10000)
    lib.shim_dump(buf); return bytearray(buf)
def mask():
    a = bytearray([0xAA])*0x10000; b = bytearray([0x55])*0x10000
    ba = (ctypes.c_uint8*0x10000).from_buffer(a); bb = (ctypes.c_uint8*0x10000).from_buffer(b)
    lib.shim_dump(ba); lib.shim_dump(bb)
    return [i for i in range(0x10000) if a[i] == b[i]]
BASE = was_emu.base_ds()
def image():
    img = dump(BASE)
    img[0:0x3F10] = BASE[0:0x3F10]
    img[0x808:0x80C] = dump()[0x808:0x80C]
    img[0x4AF5:0x4E53] = BASE[0x4AF5:0x4E53]
    struct.pack_into('<H', img, 0x6886, 0x4AF5 + 0x12)
    return img
def load(img):
    buf = (ctypes.c_uint8*0x10000).from_buffer(img); lib.shim_load(buf)
FUNCS = {'turn': (0xC79A, lambda: lib.shim_enemy_turn(), ()),
         'strength': (0xE805, lambda: lib.shim_strength_map(), ()),
         'pvalue': (0xEA51, lambda: lib.shim_port_values(), ()),
         'encounters': (0x8BCD, lambda: lib.shim_call(0, 0), ()),
         'move': (0x96A0, lambda: lib.shim_call(1, 0), ()),
         'eat': (0x9345, lambda: lib.shim_call(2, 7), (7,)),
         'battles': (0x8EDE, lambda: lib.shim_call(3, 0), ()),
         'restock': (0xBCD2, lambda: lib.shim_call(4, 0), ()),
         'ports': (0xC07C, lambda: lib.shim_call(6, 0), ())}
# the whole tick (campaign_play ovl4:7959..7A80) is not a routine of its own
def run(fname, camps, seeds, samples, frames=60, verbose=3):
    emu = was_emu.Emu(); M = None; bad = 0; n = 0; changed = [0]
    addr, remake, args = FUNCS[fname]
    for c in camps:
        for seed in seeds:
            lib.shim_start(c, seed); lib.shim_set_time(1)
            if M is None: M = mask()
            for s in range(samples):
                lib.shim_frames(frames, 0)
                if lib.shim_result() != 0: break
                if lib.shim_mode() not in (0, 1): lib.shim_key(0x1C); lib.shim_frames(5, 0); continue
                A = image()
                emu.set_ds(A); emu.call(addr, *args); Bo = emu.get_ds()
                remake(); Br = image()
                diffs = [i for i in M if Bo[i] != Br[i]]
                n += 1
                if any(Bo[i] != A[i] for i in M if not 0x808 <= i < 0x80C): changed[0] += 1
                if diffs:
                    bad += 1
                    if bad <= verbose:
                        print('S%d seed %d sample %d: %d bytes differ' % (c, seed, s, len(diffs)))
                        for i in diffs[:12]: print('   %s orig %02X remake %02X (before %02X)' % (name(i), Bo[i], Br[i], A[i]))
                    load(bytearray(Bo))   # carry on from the original's state
    print('%s: %d samples (%d changed the state), %d differ' % (fname, n, changed[0], bad))
if __name__ == '__main__':
    f = sys.argv[1]; camps = [int(x) for x in sys.argv[2].split(',')]; seeds = [int(x) for x in sys.argv[3].split(',')]
    run(f, camps, seeds, int(sys.argv[4]), frames=int(sys.argv[5]) if len(sys.argv) > 5 else 60)
