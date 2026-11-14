"""cgapat.py: writes CGAPAT.COM (and pattern.json, the 2-bit value of each of its pixels): a DOS program that shows
every triple of the four colours at both pixel parities in CGA mode 4 (palette 1 high intensity, then after a key
palette 0 high intensity), for measuring composite colours in DOSBox-X (wcgapat.sh, complut.py). Needs nasm."""
import json, subprocess
def de_bruijn(k, n):
    a=[0]*k*n; seq=[]
    def db(t,p):
        if t>n:
            if n%p==0: seq.extend(a[1:p+1])
        else:
            a[t]=a[t-p]; db(t+1,p)
            for j in range(a[t-p]+1,k): a[t]=j; db(t+1,t)
    db(1,1); return seq
s=de_bruijn(4,3)            # 64 values: every triple once, cyclically
screen=[[0]*320 for _ in range(200)]
for band,(off) in enumerate((0,1,0,1)):
    for yy in range(12):
        y=20+band*40+yy
        for k in range(-2,66+64):     # the sequence twice, cyclic
            x=40+off+k
            if 0<=x<320: screen[y][x]=s[k%64]
mem=bytearray(16384)
for y in range(200):
    base=(y&1)*0x2000+(y>>1)*80
    for x in range(320):
        mem[base+x//4] |= screen[y][x] << (6-2*(x%4))
json.dump(screen,open('pattern.json','w'))
asm='''org 100h
    mov ax,4
    int 10h
    mov dx,3D9h
    mov al,30h
    out dx,al
    push es
    mov ax,0B800h
    mov es,ax
    xor di,di
    mov si,data
    mov cx,16384
    cld
    rep movsb
    pop es
    mov ah,0
    int 16h
    mov dx,3D9h
    mov al,10h
    out dx,al
    mov ah,0
    int 16h
    mov ax,3
    int 10h
    mov ax,4C00h
    int 21h
data:
    incbin "screen.bin"
'''
open('screen.bin','wb').write(mem)
open('cgapat.asm','w').write(asm)
subprocess.run(['nasm','-f','bin','-o','CGAPAT.COM','cgapat.asm'],check=True)
print('ok', len(open('CGAPAT.COM','rb').read()))
