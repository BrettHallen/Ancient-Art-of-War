"""cursor.py SHOT.png [white]: the map cursor's position (x, top line) on an EGA/Tandy/VGA shot of WARET.EXE, from its shape."""
import sys; sys.path.insert(0,'/root/bin')
from egacmp import shot_indexes
def find(path, white=None):
    a=shot_indexes(path)
    best=None
    for w in ([white] if white is not None else [15,14]):
        arm=[0,0,w,w]; bar=[0,0,w,w,w,None,None,0,0,w,w,w]
        for l in range(0,172):
            for x in range(4,308):
                ok=True
                for k in (0,1,2,7,8,9):
                    r=(l+k)*320+x
                    if a[r:r+4]!=arm: ok=False; break
                if not ok: continue
                for k in (4,5):
                    for i,b in enumerate(bar):
                        if b is not None and a[(l+k)*320+x-4+i]!=b: ok=False; break
                    if not ok: break
                if ok: return (x,l,w)
    return None
if __name__=='__main__':
    print(find(sys.argv[1]))
