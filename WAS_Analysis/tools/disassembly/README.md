# Tools for WAS: disassembly

Copies of `../../../WAR_Analysis/tools/disassembly` that work on `../../WAS.EXE.asm`:

| File | Use |
|---|---|
| `fn NAME` | one routine on a single line, by name or by address (`fn ovl4_7650`, `fn rt_10d2`) |
| `fc NAME` | the same, shorter |
| `dec.py NAME` | C-like pseudocode with gotos, with WAS's DS names (squadrons, ships, the map) in its table. As in WAR it sometimes shows `x[i++] = v` as a pre-increment: check the asm for array writes. The compiler's booleans (`mov ax,1 / jcc / dec ax`) show as `(a == b)`, and a test after `and`/`or`/`add` uses that result (12 Nov; before, a byte flag read with `mov al / and ax,0FFh` was shown as the previous comparison). `&&` / `||` chains are still shown as gotos with `{0 | 1}` merges: read those in the asm |

Run `../../disassembler/gen_asm.py` after adding names to its table, then these see the new names.
