# The Ancient Art of War at Sea (1987) - remake
# Nov 2026 using Claude Opus 5.5
# "Guided" by Brett Hallen
# Original game by Dave Murry & Barry Murry
# Published by Brøderbund Software Inc.
#
# was_emu.py: WAS.EXE under the unicorn emulator
"""Run WAS.EXE's resident code and overlay 4 under unicorn on a given data
segment image: Emu().call(addr, *args) -> ax. Interrupts are ignored, port
reads return 0. The DS image is 64 KB; rt is at CS:0000, overlay 4 at CS:7650.
The game folder is $WAS_GAME (default: WAS_Original_Files_3.5 beside WAS_Remake)."""
import os, struct
from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_INTR, UC_HOOK_INSN, UC_HOOK_CODE
from unicorn.x86_const import *
GAME = os.environ.get('WAS_GAME', os.path.join(os.path.dirname(os.path.abspath(__file__)), '../../../WAS_Original_Files_3.5'))
EXE = os.path.join(GAME, 'WAS.EXE')
D = open(EXE, 'rb').read()
CS = 0x1000
DSEG = CS + 0x236B
SS = 0x8000
STUB = 0xFFF0

class Emu:
    def __init__(self, ovl=4):
        mu = self.mu = Uc(UC_ARCH_X86, UC_MODE_16)
        mu.mem_map(0, 0x100000)
        mu.mem_write(CS * 16, D[0x400:0x400 + 0x7650])
        ovls = {4: (0x10380, 31360)}
        off, n = ovls[ovl]
        mu.mem_write(CS * 16 + 0x7650, D[off:off + n])
        mu.mem_write(CS * 16 + STUB, b'\xf4')
        # drawing only (erase/redraw squadrons, invert a tile, the marker): return at once.
        # Not patched in memory: random() reads the code as its numbers.
        self.skip = (0x104A, 0x1084, 0x2E61, 0x10D2, 0x280E)
        for a in self.skip:
            mu.hook_add(UC_HOOK_CODE, self._ret, None, CS * 16 + a, CS * 16 + a)
        mu.hook_add(UC_HOOK_INTR, self._intr)
        mu.hook_add(UC_HOOK_INSN, self._in, None, 1, 0, UC_X86_INS_IN)
        mu.hook_add(UC_HOOK_INSN, self._out, None, 1, 0, UC_X86_INS_OUT)
        self.ints = []
    def _intr(self, mu, intno, ud):
        self.ints.append(intno)
        ah = (mu.reg_read(UC_X86_REG_AX) >> 8) & 0xFF
        if intno == 0x16 and ah in (1, 0x11):      # key waiting? no: ZF set
            fl = mu.reg_read(UC_X86_REG_EFLAGS); mu.reg_write(UC_X86_REG_EFLAGS, fl | 0x40)
        if intno == 0x21 and ah == 0x0B: mu.reg_write(UC_X86_REG_AX, 0x0B00)
    def _ret(self, mu, addr, size, ud):
        sp = mu.reg_read(UC_X86_REG_SP)
        ip = struct.unpack('<H', mu.mem_read(SS * 16 + sp, 2))[0]
        mu.reg_write(UC_X86_REG_SP, sp + 2)
        mu.reg_write(UC_X86_REG_IP, ip)
    def _in(self, mu, port, size, ud): return 0
    def _out(self, mu, port, size, value, ud): pass
    def set_ds(self, img):
        assert len(img) == 0x10000
        self.mu.mem_write(DSEG * 16, bytes(img))
    def get_ds(self): return bytes(self.mu.mem_read(DSEG * 16, 0x10000))
    def call(self, addr, *args, count=50_000_000):
        mu = self.mu
        mu.reg_write(UC_X86_REG_SS, SS); sp = 0xFFF0
        for a in reversed(args):
            sp -= 2; mu.mem_write(SS * 16 + sp, struct.pack('<H', a & 0xFFFF))
        sp -= 2; mu.mem_write(SS * 16 + sp, struct.pack('<H', STUB))
        mu.reg_write(UC_X86_REG_SP, sp); mu.reg_write(UC_X86_REG_BP, 0)
        for r in (UC_X86_REG_DS, UC_X86_REG_ES): mu.reg_write(r, DSEG)
        mu.reg_write(UC_X86_REG_CS, CS)
        mu.emu_start(CS * 16 + addr, CS * 16 + STUB, count=count)
        ip = mu.reg_read(UC_X86_REG_IP)
        if ip != STUB: raise RuntimeError('stopped at %04X (count?)' % ip)
        v = mu.reg_read(UC_X86_REG_AX)
        return v - 0x10000 if v & 0x8000 else v

def base_ds():
    """WAS.EXE's initialised data, SO5 at 4AF5, [0D19] as on an XT, [6886] = SO5+12h."""
    img = bytearray(0x10000)
    img[:0x3F10] = D[0x23AB0:0x23AB0 + 0x3F10]
    so5 = open(os.path.join(GAME, 'SO5'), 'rb').read()
    img[0x4AF5:0x4AF5 + len(so5)] = so5
    struct.pack_into('<H', img, 0x0D19, 0x200)
    struct.pack_into('<H', img, 0x6886, 0x4AF5 + 0x12)
    return img
