#!/bin/bash
TOOLS=$(cd "$(dirname "$0")" && pwd); DOS=${DOS:-/root/dos}
# dosed.sh: the editor DOSBox (war_ed + data_b on B:), answer the manual question, open "make changes"
CONF=dosbox_ed.conf bash $TOOLS/dosrun2.sh
bash $TOOLS/dosanswer.sh || exit 1
bash $TOOLS/doskey.sh sleep:9 KP_Add sleep:1 Return sleep:5
