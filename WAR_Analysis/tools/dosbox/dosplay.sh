#!/bin/bash
TOOLS=$(cd "$(dirname "$0")" && pwd); DOS=${DOS:-/root/dos}
# dosplay.sh: from the title page (after dosgo.sh), start the first campaign and wait for the map
export HOLD=0.15
bash $TOOLS/doskey.sh sleep:8 Return
bash $TOOLS/doswait.sh "Select a campaign" 30 && bash $TOOLS/doskey.sh ${CAMPAIGN_KEYS} Return
bash $TOOLS/doswait.sh "story of this campaign" 30 && bash $TOOLS/doskey.sh Return
bash $TOOLS/doswait.sh "look at the rules" 30 && bash $TOOLS/doskey.sh Return
bash $TOOLS/doswait.sh "continue" 30 && bash $TOOLS/doskey.sh Return
bash $TOOLS/doswait.sh "Surrender" 40
