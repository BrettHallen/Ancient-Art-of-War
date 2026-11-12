# Tools for WAS: disassembly

Copies of `../../../WAR_Analysis/tools/disassembly` that work on `../../WAS.EXE.asm`:

| File | Use |
|---|---|
| `fn NAME` | one routine on a single line, by name or by address (`fn ovl4_7650`, `fn rt_10d2`) |
| `fc NAME` | the same, shorter |
| `dec.py NAME` | C-like pseudocode with gotos, with WAS's DS names (squadrons, ships, the map) in its table. As in WAR it sometimes shows `x[i++] = v` as a pre-increment: check the asm for array writes |

Run `../../disassembler/gen_asm.py` after adding names to its table, then these see the new names.
