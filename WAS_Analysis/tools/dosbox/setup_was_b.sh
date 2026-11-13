#!/bin/bash
# setup_was_b.sh: a second WAS copy ($DOS/was6) with a data disk as B: ($DOS/was6b),
# for saving and loading games in DOSBox (WAS.EXE's SS). As setup_was.sh, plus:
# WAS.EXE uses B: only on a PC with two floppy drives ([0818], INT 11h), which
# DOSBox does not report, so the copy is patched at file offsets 7AC2h
# (ovl1:76C2) and A2A3h (ovl1:9EA3): jne (75) becomes jmp (EB); and SCNF's
# drive word [6863] is set to 1. The data disk must not hold the file B (the
# deck): rt_0209 takes a disk with B on it for the WAS disk.
# Run setup_was.sh first. Use CONF=dosbox_was_b.conf.
TOOLS=$(cd "$(dirname "$0")" && pwd); DOS=${DOS:-/root/dos}
rm -rf "$DOS/was6"; cp -r "$DOS/was" "$DOS/was6"; mkdir -p "$DOS/was6b"
python3 - "$DOS/was6" <<'PY'
import sys
d6 = sys.argv[1]
p = d6 + '/WAS.EXE'
d = bytearray(open(p, 'rb').read())
for a in (0x7AC2, 0xA2A3):
    assert d[a] in (0x75, 0xEB), 'not the expected WAS.EXE'
    d[a] = 0xEB
open(p, 'wb').write(d)
c = bytearray(open(d6 + '/SCNF', 'rb').read())
c[0x6863 - 0x684C] = 1
open(d6 + '/SCNF', 'wb').write(c)
PY
sed "s#mount a .*#mount a $DOS/was6\nmount b $DOS/was6b -t floppy#" "$TOOLS/dosbox_was.conf" > "$DOS/dosbox_was_b.conf"
echo "ready: CONF=dosbox_was_b.conf ../../../WAR_Analysis/tools/dosbox/dosrun2.sh (the data disk: $DOS/was6b)"
