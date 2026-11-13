# The Ancient Art of War at Sea (1987) - remake
# Nov 2026 using Claude Opus 5.5
# "Guided" by Brett Hallen
# Original game by Dave Murry & Barry Murry
# Published by Brøderbund Software Inc.
#
# fit.py: the timing weights from calib.json
"""Fit play.c's EV_WEIGHT: each event's weight in 8086 instructions and each
routine's own fixed part (EV_BASE), non-negative least squares over calib.json."""
import json, os, numpy as np, sys
from scipy.optimize import nnls
d=json.load(open(os.path.join(os.path.dirname(os.path.abspath(__file__)), 'calib.json')))
FN=['turn','encounters','move','eat','battles','restock','strength']
names=['RND','LIST','SMALL','HELP','PORT','S40','C200','QUEUE','NEIGH','STEP','RING','PRING','TILE']   # EV_BASE (the last) is left out: it is what is fitted here
X=np.array([list(r['ev'])[:len(names)]+[1 if r['f']==f else 0 for f in FN] for r in d],float); y=np.array([r['n'] for r in d],float)
k,res=nnls(X,y)
for n,v in zip(names+['b_'+f for f in FN],k): print('%-12s %8.2f'%(n,v))
pred=X@k
for f in FN:
    idx=[i for i,r in enumerate(d) if r['f']==f]
    yy=y[idx]; pp=pred[idx]; err=np.abs(pp-yy)
    print('%-10s n=%d mean %7.0f pred %7.0f  mean|err| %6.0f  rel %.2f  max %6.0f' % (f,len(idx),yy.mean(),pp.mean(),err.mean(),err.sum()/yy.sum(),yy.max()))
