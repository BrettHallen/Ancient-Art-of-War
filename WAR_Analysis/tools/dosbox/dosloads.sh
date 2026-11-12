#!/bin/bash
TOOLS=$(cd "$(dirname "$0")" && pwd); DOS=${DOS:-/root/dos}
# dosloads.sh: start the original (CONF=dosbox_ed.conf) and load the saved game S from the data disk (B:)
export HOLD=0.15
CONF=${CONF:-dosbox_ed.conf} bash $TOOLS/dosgo.sh
sleep 6; bash $TOOLS/doskey.sh Return
bash $TOOLS/doswait.sh "Select a campaign" 30 || exit 1
for i in $(seq 13); do bash $TOOLS/doskey.sh equal; done
bash $TOOLS/doskey.sh Return
bash $TOOLS/doswait.sh "Saved game" 20 || exit 1
bash $TOOLS/doskey.sh s
