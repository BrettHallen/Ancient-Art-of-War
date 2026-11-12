#!/bin/bash
TOOLS=$(cd "$(dirname "$0")" && pwd); DOS=${DOS:-/root/dos}
# doswait.sh TEXT [SECONDS]: wait until the screen's text rows contain TEXT (read with the game's font)
for i in $(seq ${2:-60}); do
  bash $TOOLS/dosshot.sh w >/dev/null 2>&1
  python3 $TOOLS/screentext.py $DOS/shots/w.png | grep -q "$1" && exit 0
  sleep 1
done
echo "timeout waiting for: $1"; exit 1
