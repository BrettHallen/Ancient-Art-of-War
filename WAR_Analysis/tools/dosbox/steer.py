"""steer.py X LINE: tap the arrow keys until WARET's map cursor is at screen x X and top line LINE (+-4) and the
command line shows a squad's commands."""
import sys, subprocess, time
sys.path.insert(0,'/root/bin')
from cursor import find
tx, tl = int(sys.argv[1]), int(sys.argv[2])
W=subprocess.run('DISPLAY=:99 xdotool search --class dosbox | head -1',shell=True,capture_output=True,text=True).stdout.strip()
def tap(k):
    subprocess.run(['xdotool','keydown','--window',W,k],env={'DISPLAY':':99'}); time.sleep(0.03)
    subprocess.run(['xdotool','keyup','--window',W,k],env={'DISPLAY':':99'}); time.sleep(0.35)
def shot():
    subprocess.run(['bash','/root/bin/dosshot.sh','steer'])
    return '/root/dos/shots/steer.png'
for n in range(60):
    p=shot()
    text=subprocess.run(['python3','/root/bin/screentext.py',p],capture_output=True,text=True).stdout
    c=find(p)
    if c is None: print('no cursor'); tap('Up'); continue
    x,l,_=c
    if 'Info' in text and abs(l-tl)<=4 and x==tx: print('on the squad', c); sys.exit(0)
    if x<tx: tap('Right')
    elif x>tx: tap('Left')
    elif l<tl-4: tap('Down')
    elif l>tl+4: tap('Up')
    else: tap('Down' if l<tl else 'Up')
print('gave up', c); sys.exit(1)
