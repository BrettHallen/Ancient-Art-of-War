#!/bin/bash
# setup_was.sh: make the DOSBox working folder for WAS.EXE ($DOS, default /root/dos).
# Copies the At Sea disk to $DOS/was and patches that copy of WAS.EXE so it
# starts without the original floppy: at file offset 7AB3h (ovl1:76B3, after
# the key-disk check ovl1_a9a9) jne 07 (75 07) becomes jmp 07 (EB 07).
# The repo's WAS.EXE is not changed.
TOOLS=$(cd "$(dirname "$0")" && pwd); DOS=${DOS:-/root/dos}
REPO=$(cd "$TOOLS/../../.." && pwd)
mkdir -p "$DOS/was" "$DOS/shots" "$DOS/cap"
cp "$REPO"/WAS_Original_Files_3.5/* "$DOS/was/"
python3 - "$DOS/was/WAS.EXE" <<'PY'
import sys
p = sys.argv[1]
d = bytearray(open(p, 'rb').read())
assert d[0x7AB3:0x7AB5] in (b'\x75\x07', b'\xeb\x07'), 'not the expected WAS.EXE'
d[0x7AB3] = 0xEB
open(p, 'wb').write(d)
PY
cp "$TOOLS/dosbox_was.conf" "$TOOLS/dosbox_was_xt.conf" "$DOS/"
python3 "$TOOLS/make_font8.py" > "$DOS/font8.txt"   # copy next to screentext.py (the game's font: not kept in the repo)
echo "ready: CONF=dosbox_was.conf ../../../WAR_Analysis/tools/dosbox/dosrun2.sh"
