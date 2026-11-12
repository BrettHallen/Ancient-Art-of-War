#!/bin/bash
TOOLS=$(cd "$(dirname "$0")" && pwd); DOS=${DOS:-/root/dos}
# dosgo.sh: start DOSBox with $CONF (default dosbox.conf) and answer the manual question
CONF=${CONF:-dosbox.conf} bash $TOOLS/dosrun2.sh
bash $TOOLS/dosanswer.sh
