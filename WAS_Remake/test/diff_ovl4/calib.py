# The Ancient Art of War at Sea (1987) - remake
# Nov 2026 using Claude Opus 5.5
# "Guided" by Brett Hallen
# Original game by Dave Murry & Barry Murry
# Published by Brøderbund Software Inc.
#
# calib.py: instruction counts for the timing (play.c EV_WEIGHT)
"""Instruction counts of WAS.EXE's tick routines (under unicorn) against the
remake's event counters, for the timing model. Writes calib.json."""
import ctypes, json, os, sys
import diff, was_emu
from unicorn import UC_HOOK_CODE
lib = diff.lib
NEV = lib.shim_ev_count()
emu = was_emu.Emu()
cnt = [0]; skipped = [0]
def h(mu, a, s, u): cnt[0] += 1
emu.mu.hook_add(UC_HOOK_CODE, h)
for a in emu.skip:
    emu.mu.hook_add(UC_HOOK_CODE, lambda mu, ad, s, u: skipped.__setitem__(0, skipped[0] + 1), None, was_emu.CS*16+a, was_emu.CS*16+a)
FN = ['turn', 'encounters', 'move', 'eat', 'battles', 'restock', 'strength']
out = []
M = None
camps = range(11); seeds = (1, 2, 3); samples = int(sys.argv[1]) if len(sys.argv) > 1 else 12
for c in camps:
    for seed in seeds:
        lib.shim_start(c, seed); lib.shim_set_time(1)
        for s in range(samples):
            lib.shim_frames(25, 0)
            if lib.shim_result() != 0: break
            if lib.shim_mode() not in (0, 1): lib.shim_key(0x1C); lib.shim_frames(5, 0); continue
            A = diff.image()
            for f in FN:
                addr, remake, args = diff.FUNCS[f]
                emu.set_ds(A); cnt[0] = 0; skipped[0] = 0
                emu.call(addr, *args)
                n = cnt[0]
                diff.load(bytearray(A))
                lib.shim_ev_reset(); remake()
                ev = (ctypes.c_uint32 * NEV)(); lib.shim_ev(ev)
                out.append({'f': f, 'n': n, 'skip': skipped[0], 'ev': list(ev)})
            diff.load(bytearray(A))
json.dump(out, open(os.path.join(diff.HERE, 'calib.json'), 'w'))
print(len(out), 'samples')
