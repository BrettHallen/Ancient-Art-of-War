#!/bin/bash
# setup_wast.sh: make the DOSBox-X working folder for the Tandy version ($DOS/wast).
# Copies the At Sea disk there, unpacks WAST.EXE (../disassembly/unexepack.py)
# to WASTU.EXE and patches the key-disk check in it (after key_disk_check:
# jz D8 at file offset 2C1E6h and, after the string compare, jnz BD at 2C201h,
# both to nop nop), so it starts without the original floppy. Installs
# dosboxx_wastu.conf (DOSBox-X, machine=tandy; DOSBox 0.74 does not show the
# battle's palette registers). The repo's files are not changed.
TOOLS=$(cd "$(dirname "$0")" && pwd); DOS=${DOS:-/root/dos}
REPO=$(cd "$TOOLS/../../.." && pwd)
mkdir -p "$DOS/wast" "$DOS/shots"
cp "$REPO"/WAS_Original_Files_3.5/* "$DOS/wast/"
python3 "$TOOLS/../disassembly/unexepack.py" "$DOS/wast/WAST.EXE" "$DOS/wast/WASTU.EXE"
python3 - "$DOS/wast/WASTU.EXE" <<'PY'
import sys
p = sys.argv[1]; d = bytearray(open(p, 'rb').read())
for off, old in ((0x2C1E6, b'\x74\xd8'), (0x2C201, b'\x75\xbd')):
    assert d[off:off + 2] == old, hex(off)
    d[off:off + 2] = b'\x90\x90'
open(p, 'wb').write(d)
PY
sed "s#/root/dos#$DOS#g" "$TOOLS/dosboxx_wastu.conf" > "$DOS/dosboxx_wastu.conf"
