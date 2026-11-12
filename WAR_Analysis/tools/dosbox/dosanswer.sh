#!/bin/bash
TOOLS=$(cd "$(dirname "$0")" && pwd); DOS=${DOS:-/root/dos}
# dosanswer.sh: wait for the manual question, read it with the game's font (screentext.py) and type the answer
for try in $(seq 40); do
  sleep 2
  bash $TOOLS/dosshot.sh q
  T=$(python3 $TOOLS/screentext.py $DOS/shots/q.png | tr '\n' ' ')
  P=$(echo "$T" | grep -oE "page [0-9]+" | grep -oE "[0-9]+"); W=$(echo "$T" | grep -oE "word [0-9]+" | grep -oE "[0-9]+"); A=$(echo "$T" | grep -oE "paragraph [0-9]+" | grep -oE "[0-9]+")
  [ -n "$P" ] && [ -n "$W" ] && [ -n "$A" ] && break
done
ANS=$(python3 $TOOLS/warword.py $P $A $W)
echo "Q: $P $A $W A: $ANS"
case "$ANS" in *[!a-zA-Z]*|"") echo "no answer"; exit 1;; esac
bash $TOOLS/doskey.sh $(echo "$ANS" | sed 's/./& /g') Return
