# Tools for WAS: disassembly

Copies of `../../../WAR_Analysis/tools/disassembly` that work on `../../WAS.ASM`, and `ds.py` on the data segment of WAS.EXE or WAST.EXE:

| File | Use |
|---|---|
| `fn NAME` | one routine on a single line, by name or by address (`fn ovl4_7650`, `fn rt_10d2`) |
| `fc NAME` | the same, shorter |
| `ds.py OFF...` | what is at DS offsets of WAS.EXE: `ds.py 0x6a` the C string (`'One moment, please'`), `ds.py 0x37:9` bytes, `ds.py 0x41:4w` words; `ds.py -s TEXT` finds a string's offsets; `-t` first for WAST.EXE (unpacked with `unexepack.py`; `ds.py -t 0x6bba`). Offsets past the initialised data (WAS.EXE: 3F10h) are set at run time |
| `unexepack.py IN OUT` | undoes WAST.EXE's EXEPACK (used by `ds.py -t` and `../../disassembler/wastdis.py`) |
| `dec.py NAME` | C-like pseudocode with gotos, with WAS's DS names (squadrons, ships, the map) in its table. As in WAR it sometimes shows `x[i++] = v` as a pre-increment: check the asm for array writes. The compiler's booleans (`mov ax,1 / jcc / dec ax`) show as `(a == b)`, and a test after `and`/`or`/`add` uses that result (12 Nov; before, a byte flag read with `mov al / and ax,0FFh` was shown as the previous comparison). `&&` / `||` chains are still shown as gotos with `{0 | 1}` merges: read those in the asm |

Run `../../disassembler/gen_asm.py` after adding names to its table, then these see the new names.
