#!/usr/bin/env python3
# warword.py PAGE PARA WORD (as asked: "what is word W in paragraph P"): the answer to WAR.EXE's manual question
import sys, os
d=bytearray(open(os.path.join(os.environ.get('DOS', '/root/dos'), 'war', 'WARW'),'rb').read())
l=0
while l < len(d)-2 and not (d[l]==0 and d[l+1]==0 and d[l+2]==0): l+=1
k=117; out=bytearray()
for i in range(l): out.append(d[i]^(k&0xFF)); k+=23
p,a,w=map(int,sys.argv[1:4])
for e in out.split(b';'):
    if len(e)>2 and e[0]==p and e[1]==(w<<4|a): print(e[2:].decode()); break
else:
    print('?', [ (e[0],hex(e[1]),e[2:]) for e in out.split(b';') if len(e)>2 and e[0]==p])
