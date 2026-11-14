/*
 * The Ancient Art of War at Sea (1987) - remake
 * Nov 2026 using Claude Opus 5.5
 * "Guided" by Brett Hallen
 * Original game by Dave Murry & Barry Murry
 * Published by Brøderbund Software Inc.
 *
 * boarding.c: the fight on deck when you Zoom in on a grappled ship
 * (WAS.EXE's overlay 7, ovl7_c9b0-ff14). Forty men, the two crews, fight
 * across the decks drawn from the picture B; you send yours from one part of
 * the deck to another (FROM Flag/Quarterdeck/Poopdeck/Railing TO ...) and win
 * by clearing the enemy's deck or reaching his flag.
 *
 * As in WAS.EXE everything lives in the data segment copy (b->m): the
 * picture B and the men's arrays go in a 32000-byte buffer where SO8 was
 * (SO8 is freed while you board and loaded again afterwards), the scripts
 * and the men's pictures are WAS.EXE's own tables. The deck is unpacked into
 * the buffer's two fields (the even and odd screen lines) every frame, the
 * men are drawn into it and the result copied to the screen.
 */
#include "battle_int.h"
#include <stdlib.h>

/* ------------------------------------------------------------------ where things are */

#define BUF     0x8000            /* [3F37]: alloc(7D00h), B is loaded at its start */
#define FLD     (BUF + 0x1F40)    /* [3F39]: field 0 of the deck (8000 bytes); field 1 at +3E80h */
#define FLD1    0x3E80
#define A_KIND  (BUF + 0x1770)    /* [6CF4] byte: 1 an officer */
#define A_SIDE  (BUF + 0x1798)    /* [6CF6] word: -1 your men, 0 the enemy's */
#define A_ADDR  (BUF + 0x17E8)    /* [6CF8] word: the man's place in the buffer */
#define A_X     (BUF + 0x1838)    /* [6CFA] word: x in bytes (0: gone) */
#define A_Y     (BUF + 0x1888)    /* [6CFC] word: y in lines of a field */
#define A_FACE  (BUF + 0x18D8)    /* [6CFE] byte: 101 right, 102 left, 103 down, 104 up */
#define A_STATE (BUF + 0x1928)    /* [6D00] byte: from the script; 2 striking, 5 crossing, 9 dead */
#define A_AREA  (BUF + 0x19C8)    /* [6D02] byte: 1 flag, 2 quarterdeck, 3 poopdeck, 4 railing, 5 across */
#define A_TGT   (BUF + 0x1A18)    /* [6D04] word: the man he fights (an officer's: his shot's place) */
#define A_ORD   (BUF + 0x1A68)    /* [6D06] byte: the drawing order */
#define A_MOV   (BUF + 0x1AB8)    /* [6D08] byte: on his way to another part of the deck */

#define V_SCRIPT 0x6D0A           /* 40 words: each man's place in his script */
#define V_SPR    0x6D5A           /* 48 words: the men's pictures */
#define V_SP     0x6DBC           /* word: &script of the man being moved */
#define V_CNT    0x6DC7           /* 11 words: men in each part (enemy +5) */
#define V_TARGET 0x64D7           /* word: the man's opponent (-1 none, -2 too far) */
#define V_COUNT  0x64DD           /* word: 20 waiting; Attack sets 3; 0 fighting */
#define V_CMD    0x6DBE           /* word: 1 FROM given, 2 send a man */
#define V_MAN    0x6DC0           /* byte: the man sent */
#define V_RESULT 0x6DC1           /* word: 1 you won, 2 you lost */
#define V_POOP   0x6DC3           /* word: the enemy has sent his poopdeck across */
#define V_FRAME  0x6DC5           /* word: frames to the enemy's next orders */
#define V_FROM   0x6DDD           /* byte: FROM (9 none) */
#define V_TO     0x6DDE           /* byte: TO (0 asked for, 9 none) */
#define V_COL    0x6DDF           /* byte: the column for " TO " */

/* The time WAS.EXE spends working: 8086 instructions at 300 a millisecond
 * (see battle_gfx.c). ASM() counts the assembly routines (fe6b, fec9, ff14)
 * exactly; WORK() is a rough count for the compiled C, scaled by LOGIC_K so
 * that a frame takes as long as in DOSBox at 300 cycles: about 0.5 s while
 * the crews wait for the Attack, about 0.6 s as the fight starts. */
#define LOGIC_K 2.5
#define ASM(n)  (b->clock_us += (n) * (1000.0 / 300.0))
#define WORK(n) ASM((n) * LOGIC_K)

static int  uw(const Battle *b, int a) { return b->m[a & 0xFFFF] | (b->m[(a + 1) & 0xFFFF] << 8); }
static void sw(Battle *b, int a, int v) { b->m[a & 0xFFFF] = (uint8_t)v; b->m[(a + 1) & 0xFFFF] = (uint8_t)(v >> 8); }

#define KIND(i)   mb(b, A_KIND + (i))
#define SIDE(i)   mw(b, A_SIDE + 2 * (i))
#define ADDR(i)   uw(b, A_ADDR + 2 * (i))
#define MX(i)     mw(b, A_X + 2 * (i))
#define MY(i)     mw(b, A_Y + 2 * (i))
#define FACE(i)   mb(b, A_FACE + (i))
#define STATE(i)  mb(b, A_STATE + (i))
#define AREA(i)   mb(b, A_AREA + (i))
#define TGT(i)    mw(b, A_TGT + 2 * (i))
#define ORD(i)    mb(b, A_ORD + (i))
#define MOV(i)    mb(b, A_MOV + (i))
#define SCR(i)    uw(b, V_SCRIPT + 2 * (i))
#define CNT(k)    mw(b, V_CNT + 2 * (k))
#define SET_KIND(i, v)  setmb(b, A_KIND + (i), v)
#define SET_SIDE(i, v)  sw(b, A_SIDE + 2 * (i), v)
#define SET_ADDR(i, v)  sw(b, A_ADDR + 2 * (i), v)
#define SET_X(i, v)     sw(b, A_X + 2 * (i), v)
#define SET_Y(i, v)     sw(b, A_Y + 2 * (i), v)
#define SET_FACE(i, v)  setmb(b, A_FACE + (i), v)
#define SET_STATE(i, v) setmb(b, A_STATE + (i), v)
#define SET_AREA(i, v)  setmb(b, A_AREA + (i), v)
#define SET_TGT(i, v)   sw(b, A_TGT + 2 * (i), v)
#define SET_ORD(i, v)   setmb(b, A_ORD + (i), v)
#define SET_MOV(i, v)   setmb(b, A_MOV + (i), v)
#define SET_SCR(i, v)   sw(b, V_SCRIPT + 2 * (i), v)
#define SET_CNT(k, v)   sw(b, V_CNT + 2 * (k), v)
#define TARGET          mw(b, V_TARGET)
#define SET_TARGET(v)   sw(b, V_TARGET, v)
#define COUNT           mw(b, V_COUNT)
#define RESULT          mw(b, V_RESULT)
#define FROM            mb(b, V_FROM)
#define TO              mb(b, V_TO)
#define LAST_RANDOM     (b->p->rnd)           /* [0808]: random()'s last value */
#define OPP(addr)       (b->p->opp[((addr) - 0x604D) / 2])

/* *[6DBC] = script: the script of the man def1 is moving */
static void set_sp(Battle *b, int script) { sw(b, uw(b, V_SP), script); }
static void x_step(Battle *b, int i, int d) { SET_X(i, MX(i) + d); SET_ADDR(i, ADDR(i) + d); }
static void place(Battle *b, int i) { SET_ADDR(i, FLD + MY(i) * 80 + MX(i)); }

static void d444(Battle *b);
static int  f86f(Battle *b, int shot);
static void fa94(Battle *b);
static void fb07(Battle *b);
static void def1(Battle *b);
static int  f3a9(Battle *b, int a);
static int  f484(Battle *b, int a);

/* ------------------------------------------------------------------ the deck picture */

static void t_unpack(Battle *b);
static void t_to_screen(Battle *b, int off, int bx, int end);
static void t_man(Battle *b, int spr, int addr, int side);

/* ovl7_fe6b: unpack B (at the buffer's start) into the two fields at dst and
 * dst+3E80h. Each field: a word count of runs, then runs: a byte n <= 200 and
 * the byte to repeat n times, or n > 200 and n-200 bytes to copy. */
static void fe6b(Battle *b, int dst)
{
    t_unpack(b);
    int si = BUF;
    long work = 20;
    for (int f = 0; f < 2; f++) {
        int di = dst + f * FLD1;
        int n = (int16_t)uw(b, si);
        si += 2;
        do {
            int c = b->m[si++ & 0xFFFF];
            if (c <= 200) {
                int v = b->m[si++ & 0xFFFF];
                for (int k = 0; k < c; k++) b->m[di++ & 0xFFFF] = (uint8_t)v;
            } else {
                for (int k = 0; k < c - 200; k++) b->m[di++ & 0xFFFF] = b->m[si++ & 0xFFFF];
                c -= 200;
            }
            work += 9 + c;
        } while (--n > 0);
    }
    ASM(work);
}

/* ------------------------------------------------------------------ the Tandy version's deck (mode 9) */

/* expand_2to4 (WAST rt:01DAB) through the nibble table DS:2E86: a 2-bit byte to
 * two 4-bit bytes, colours 0, 1, 2, 3 to 0, 3, 12, 15 (a mask's 11b to 1111b) */
static uint16_t x24(int v)
{
    static const uint8_t X4[4] = { 0, 3, 12, 15 };
    int hi = (X4[(v >> 6) & 3] << 4) | X4[(v >> 4) & 3], lo = (X4[(v >> 2) & 3] << 4) | X4[v & 3];
    return (uint16_t)(hi | (lo << 8));     /* the bytes as stosw leaves them: hi first */
}

/* boarding_load_b (WAST ovl4:72A2): mode 9, WAST.DAT's deck into its two fields
 * (put_block16: even lines, odd lines) */
static void t_load(Battle *b)
{
    b->t9 = true;
    memset(b->scr16, 0, sizeof b->scr16);
    for (int y = 0; y < SCREEN_H; y++) b->row16[y] = true;
    memset(b->vram_t, 0, sizeof b->vram_t);
    memset(b->deck16, 0, sizeof b->deck16);
    const Image *img = &b->ga->set[MODE_16].pictures[PIC_DECK];
    if (!b->ga->set[MODE_16].loaded || !img->px) return;
    for (int y = 0; y < SCREEN_H && y < img->h; y++)
        for (int x = 0; x + 1 < SCREEN_W && x + 1 < img->w; x += 2)
            b->deck16[y & 1][(y >> 1) * 160 + x / 2] =
                (uint8_t)(((img->px[y * img->w + x] & 15) << 4) | (img->px[y * img->w + x + 1] & 15));
}

/* unpack_deck (WAST ovl4:9EF6) in mode 9: 91 clean lines of each field into the working copy */
static void t_unpack(Battle *b)
{
    for (int f = 0; f < 2; f++) memcpy(b->work16[f], b->deck16[f], 0x38E0);
}

/* deck_to_screen (WAST ovl4:9F90) in mode 9: as fec9, 160 bytes a line */
static void t_line(Battle *b, int f, int line, int row)
{
    if (row < 0 || row >= SCREEN_H || line < 0 || line >= 100) return;
    const uint8_t *src = &b->work16[f][line * 160];
    for (int i = 0; i < 160; i++) {
        b->scr16[row * SCREEN_W + 2 * i] = src[i] >> 4;
        b->scr16[row * SCREEN_W + 2 * i + 1] = src[i] & 15;
    }
    b->row16[row] = true;
}

static void t_to_screen(Battle *b, int off, int bx, int end)
{
    int line[2] = { off / 80, off / 80 }, f = 0;
    for (int ax = 0; ax < 200; ax += 2, bx += 2) { t_line(b, f, line[f]++, bx / 2); f ^= 1; }
    for (; bx < end; bx += 2) { t_line(b, f, line[f]++, bx / 2); f ^= 1; }
}

/* draw_man (WAST ovl4:A006) in mode 9: the man's 2-bit picture expanded, at the
 * CGA address's place in the working copy */
static void t_man(Battle *b, int spr, int addr, int side)
{
    int si = spr;
    int w = b->m[si++ & 0xFFFF], h = b->m[si++ & 0xFFFF];
    if (w == 0 || h == 0) return;
    for (int y = 0; y < h; y++, addr += 80)
        for (int f = 0; f < 2; f++) {
            int di = addr - FLD;
            for (int k = 0; k < w; k++, di++) {
                uint16_t a = x24(b->m[si++ & 0xFFFF]), c = x24(b->m[si++ & 0xFFFF]);
                for (int j = 0; j < 2; j++) {
                    long o = 2L * di + j;
                    if (o < 0 || o >= (long)sizeof b->work16[0]) continue;
                    uint8_t am = (uint8_t)(a >> (8 * j)), cv = (uint8_t)(c >> (8 * j));
                    uint8_t *d = &b->work16[f][o];
                    if (side != 0) *d = (uint8_t)((*d & ~am) | cv);
                    else *d = (uint8_t)((*d | am) & ~cv);
                }
            }
        }
}

/* ovl7_fec9: copy the fields to the screen, a line of 80 bytes from each in
 * turn, from offset off; rows from bx/2: 100 of them, then on until row end/2 */
static void fec9(Battle *b, int off, int bx, int end)
{
    t_to_screen(b, off, bx, end);
    int si = FLD + off, dx = si + FLD1, rows = 0;
    for (int ax = 0; ax < 200; ax += 2, bx += 2, rows++) {
        int di = row_addr(bx / 2);
        for (int k = 0; k < 80; k++) b->vram[(di + k) & 0x3FFF] = b->m[(si + k) & 0xFFFF];
        si += 80;
        int t = si; si = dx; dx = t;
    }
    for (; bx < end; bx += 2, rows++) {
        int di = row_addr(bx / 2);
        for (int k = 0; k < 80; k++) b->vram[(di + k) & 0x3FFF] = b->m[(si + k) & 0xFFFF];
        si += 80;
        int t = si; si = dx; dx = t;
    }
    ASM(15 + rows * 49);
}

/* ovl7_ff14: a man's picture into both fields at addr: w bytes by h lines,
 * each line w pairs for field 0 then w pairs for field 1. Your men (side -1):
 * AND NOT the first byte, OR the second; the enemy's (0): OR the first, AND
 * NOT the second. */
static void ff14(Battle *b, int spr, int addr, int side)
{
    t_man(b, spr, addr, side);
    int si = spr;
    int w = b->m[si++ & 0xFFFF], h = b->m[si++ & 0xFFFF];
    if (w == 0 || h == 0) return;      /* a 0 would loop 65536 times in WAS.EXE: none in its tables */
    for (int y = 0; y < h; y++, addr += 80) {
        for (int f = 0; f < 2; f++) {
            int di = addr + f * FLD1;
            for (int k = 0; k < w; k++, di++) {
                uint8_t a = b->m[si++ & 0xFFFF], c = b->m[si++ & 0xFFFF];
                uint8_t *d = &b->m[di & 0xFFFF];
                if (side != 0) *d = (uint8_t)((*d & ~a) | c);
                else *d = (uint8_t)((*d | a) & ~c);
            }
        }
    }
    ASM(16 + h * (10 + 14 * w));
}

/* ------------------------------------------------------------------ sounds */

/* ovl7_fd94: a tune: notes (dividers, 0 ends) slid to from 3000, and their lengths */
static void fd94(Battle *b, int notes, int lengths)
{
    int l6 = 0xBB8;
    for (int i = 0;; i++) {
        int f = mw(b, notes + 2 * i);
        if (f == 0) break;
        if (f > l6) {
            while (f - 25 > l6) { b_sound(b, l6); b_delay(b, 25); l6 += 25; }
        } else {
            if (f == l6) l6 += 0x12C;
            while (f + 25 < l6) { b_sound(b, l6); b_delay(b, 25); l6 -= 25; }
        }
        l6 = f;
        b_sound(b, l6);
        b_delay(b, mw(b, lengths + 2 * i));
    }
    b_sound_off(b);
}

/* ------------------------------------------------------------------ the command line */

/* ovl7_d444: row 24: "Attack Leave" before the fight, then the parts of the
 * deck you can send men from (FROM) or to (TO), Cross and Again */
static void d444(Battle *b)
{
    g_clear_row(b, 24);
    if (COUNT > 5) { g_print(b, 0x403); WORK(30); return; }   /* Attack Leave */
    if (b->demo) return;
    static const int names[5] = { 0, 0x410, 0x416, 0x423, 0x42D };   /* Flag Quarterdeck Poopdeck Railing */
    for (int k = 1; k <= 4; k++) {
        if ((FROM != k && TO == 0) || (TO != 0 && CNT(k) != 0)) g_print(b, names[k]);
    }
    if (TO == 0 && FROM != 1 && FROM != 2) g_print(b, 0x436);  /* Cross */
    else if (TO != 0 && TO != 9) g_print(b, 0x43C);             /* Again */
    WORK(80);
}

/* ovl7_d9a2: FROM */
static void d9a2(Battle *b)
{
    sw(b, V_CMD, 1);
    g_clear_row(b, 23);
    g_print(b, 0x46F);                                   /* FROM */
    setmb(b, V_COL, 5);
    setmb(b, V_TO, 0);
}

/* ovl7_d9cc: TO */
static void d9cc(Battle *b)
{
    sw(b, V_CMD, 2);
    g_goto(b, mb(b, V_COL), 23);
    g_print(b, 0x475);                                   /* TO */
}

/* ovl7_dae0: one of your men in the FROM part, not on his way: one without
 * an opponent first, else any (his opponent forgotten) */
static void dae0(Battle *b)
{
    for (int pass = 0; pass < 2; pass++)
        for (int i = 0; i < 40; i++) {
            WORK(25);
            if (STATE(i) == 9 || SIDE(i) == 0 || KIND(i) == 1 || MOV(i) != 0) continue;
            if (AREA(i) != FROM) continue;
            if (TGT(i) == -1 || pass == 1) {
                setmb(b, V_MAN, i);
                if (pass == 1) SET_TGT(i, -1);
                return;
            }
        }
}

/* ovl7_d9f0: send a man FROM -> TO */
static void d9f0(Battle *b)
{
    int none = 0;
    dae0(b);
    SET_CNT(FROM, CNT(FROM) - 1);
    if (CNT(FROM) <= 0) { none = 1; SET_CNT(FROM, 0); }
    SET_MOV(mb(b, V_MAN), 1);
    SET_AREA(mb(b, V_MAN), TO);
    if (none) setmb(b, V_TO, 9);
}

/* ovl7_da69: the demo's orders */
static void da69(Battle *b)
{
    setmb(b, V_TO, 4);
    if (CNT(2) != 0) { setmb(b, V_FROM, 2); sw(b, V_CMD, 2); return; }
    if (CNT(1) != 0) { setmb(b, V_FROM, 1); sw(b, V_CMD, 2); return; }
    setmb(b, V_TO, 5);
    if (CNT(3) != 0) { setmb(b, V_FROM, 3); sw(b, V_CMD, 2); return; }
    if (CNT(4) != 0) { setmb(b, V_FROM, 4); sw(b, V_CMD, 2); }
}

/* ovl7_d8bf: a key (ovl7_d8eb, the joystick, is not ported) */
static int d8bf(Battle *b)
{
    if (b->demo) return 0;
    return b_getkey(b);
}

/* ovl7_d579: the keys. Returns -1 to Leave. */
static int d579(Battle *b)
{
    int k = d8bf(b);
    if (k >= 'a' && k <= 'z') k -= 32;
    if (RESULT != 0) return 0;
    if (COUNT == 0) while (b_getkey(b)) ;
    sw(b, V_CMD, 0);
    WORK(30);
    if (COUNT > 5) {
        int auto_attack = 0;
        if (b->demo) k = 'A';
        int opponent = b->p->c->opponent;
        if ((opponent == 2 && b_random(b) < 20) || (opponent == 4 && b_random(b) == 2)) {
            k = 'A';
            auto_attack = 1;
            BLOGN("boarding: the enemy attacks first (%s)", opponent == 2 ? "Blackbeard: chance 20/128 a frame" : "John Paul Jones: chance 1/128 a frame");
            fd94(b, 0x1CC3, 0x1CCD);                    /* the enemy attacks */
        }
        if (k == 'A') {
            if (!auto_attack) BLOGN("boarding: you Attack");
            sw(b, V_COUNT, 3);
            d444(b);
            if (!auto_attack) fd94(b, 0x1CA5, 0x1CB5);  /* the charge */
        }
        if (k == 'L') { BLOGN("boarding: you Leave"); return -1; }
    } else {
        static const int names[5] = { 0, 0x442, 0x447, 0x453, 0x45C };   /* FLAG QUARTERDECK POOPDECK RAILING */
        static const int width[5] = { 0, 4, 11, 8, 7 };
        int area = k == 'F' ? 1 : k == 'Q' ? 2 : k == 'P' ? 3 : k == 'R' ? 4 : 0;
        if (area) {
            if ((FROM != area && TO == 0) || (TO != 0 && CNT(area) != 0)) {
                static const char *const AREA_NAME[5] = { "", "the flag", "the quarterdeck", "the poopdeck", "the railing" };
                BLOGN("boarding: you %s %s (%d of your men there)", TO != 0 ? "send men FROM" : "choose TO", AREA_NAME[area], CNT(area));
                if (TO != 0) { setmb(b, V_FROM, area); d9a2(b); }
                else { setmb(b, V_TO, area); d9cc(b); }
                g_print(b, names[area]);
                setmb(b, V_COL, mb(b, V_COL) + width[area]);
                b_beep(b);
            }
        } else if (k == 'C') {
            if (TO == 0 && FROM != 1 && FROM != 2) {
                BLOGN("boarding: you Cross to the enemy's flag");
                setmb(b, V_TO, 5);
                d9cc(b);
                g_print(b, 0x464);                       /* ENEMY FLAG */
                b_beep(b);
            }
        } else if (k == 'A') {
            if (TO != 0 && TO != 9) { sw(b, V_CMD, 2); b_beep(b); BLOGN("boarding: you send Again"); }
        } else if (k == 27 || k == 8) {
            g_clear_row(b, 23);
            setmb(b, V_TO, 9);
            setmb(b, V_FROM, 9);
            d444(b);
            b_beep(b);
        }
    }
    if (b->demo) da69(b);
    if (mw(b, V_CMD) == 2 && TO != 0) d9f0(b);
    if (mw(b, V_CMD) != 0) d444(b);
    return 0;
}

/* ------------------------------------------------------------------ the enemy's orders */

/* ovl7_de29: all the enemy's men in a part go across */
static void de29(Battle *b, int area)
{
    for (int i = 0; i < 40; i++) {
        WORK(25);
        if (SIDE(i) != 0 || STATE(i) == 9 || KIND(i) == 1) continue;
        if (AREA(i) != area || MOV(i) != 0) continue;
        int k = AREA(i) + 5;
        SET_CNT(k, CNT(k) - 1);
        if (CNT(k) < 0) SET_CNT(k, 0);
        SET_AREA(i, 5);
        SET_MOV(i, 1);
    }
}

/* ovl7_dcd7: one of the enemy's men to send to the part area, or -1 */
static int dcd7(Battle *b, int area)
{
    int crossing = 0;
    for (int i = 0; i < 40; i++) {
        WORK(25);
        if (SIDE(i) == -1 && STATE(i) != 9 && KIND(i) != 1 && AREA(i) == 5) { crossing++; break; }
    }
    for (int i = 0; i < 40; i++) {
        WORK(30);
        int a = AREA(i);
        if (SIDE(i) != 0 || STATE(i) == 9) continue;
        if (a == 5 || a == area) continue;
        if (TGT(i) != -1 || MOV(i) != 0 || KIND(i) == 1) continue;
        if (a == 4 && CNT(4) >= CNT(9)) continue;
        if (a == 3 && CNT(3) >= CNT(8)) continue;
        if (area == 5 && a != 4 && a != 3) continue;
        if (a == 1 && CNT(6) < 4 && crossing != 0) continue;
        return i;
    }
    return -1;
}

/* ovl7_dbac: the enemy's orders, every sixth frame of the fight */
static void dbac(Battle *b)
{
    int to = -1, man = -1;
    if (CNT(9) != 0) de29(b, 4);
    if (CNT(3) <= CNT(8) || (CNT(8) != 0 && mw(b, V_POOP) != 0)) {
        de29(b, 3);
        sw(b, V_POOP, 1);
    }
    if (CNT(7) != 0) {
        if (b_random(b) < 64) { man = dcd7(b, 4); to = 4; }
        else { man = dcd7(b, 3); to = 3; }
    } else if (CNT(4) > CNT(9) && b_random(b) < 32) {
        man = dcd7(b, 4); to = 4;
    } else if (CNT(3) > CNT(8) && b_random(b) < 32) {
        man = dcd7(b, 3); to = 3;
    }
    if (man != -1) {
        int k = AREA(man) + 5;
        SET_CNT(k, CNT(k) - 1);
        if (CNT(k) < 0) SET_CNT(k, 0);
        SET_AREA(man, to);
        SET_MOV(man, 1);
    }
    WORK(40);
}

/* ------------------------------------------------------------------ the men */

/* ovl7_fbb3: count the men in each part */
static void fbb3(Battle *b)
{
    for (int k = 0; k < 11; k++) SET_CNT(k, 0);
    for (int i = 0; i < 40; i++) {
        if (STATE(i) == 9 || KIND(i) == 1) continue;
        int k = AREA(i);
        if (SIDE(i) == 0) k += 5;
        SET_CNT(k, CNT(k) + 1);
    }
    if (CNT(FROM) == 0) g_clear_row(b, 23);
}

/* ovl7_fc5e: one pass sorting the drawing order by y (the dead first) */
static void fc5e(Battle *b)
{
    for (int i = 0, j = 1; j < 40; i++, j++) {
        int a = ORD(i), c = ORD(j);
        int ya = MY(a), yc = MY(c);
        if (STATE(a) == 9) ya -= 100;
        if (STATE(c) == 9) yc -= 100;
        if (ya > yc) { SET_ORD(i, c); SET_ORD(j, a); }
    }
    WORK(39 * 45);
}

/* ovl7_fd25: nobody fights man t any more */
static void fd25(Battle *b, int t)
{
    for (int i = 0; i < 40; i++) if (TGT(i) == t) SET_TGT(i, -1);
    WORK(40 * 12);
}

/* ovl7_fd62: is a ship grappled to ship s? */
static bool fd62(Battle *b, int s)
{
    for (int i = 0; i < BS; i++) if (b->grappled[i] == s) return true;
    return false;
}

/* ovl7_f327: does another man (alive) stand where man a is? */
static int f327(Battle *b, int a)
{
    int x = MX(a), y = MY(a);
    for (int i = 0; i < 40; i++) {
        if (i == a || STATE(i) == 9) continue;
        if (MX(i) == x && MY(i) == y) { WORK(20 * i); return 1; }
    }
    WORK(800);
    return 0;
}

/* ovl7_f17a: step off another man */
static void f17a(Battle *b, int a)
{
    if (!f327(b, a)) return;
    b_random(b);
    if (LAST_RANDOM < 32) SET_X(a, MX(a) + 1);
    else if (LAST_RANDOM < 64) SET_X(a, MX(a) - 1);
    b_random(b);
    if (LAST_RANDOM < 32) SET_Y(a, MY(a) + 1);
    else if (LAST_RANDOM < 64) SET_Y(a, MY(a) - 1);
    place(b, a);
}

/* ovl7_f221: keep a few bytes from the opponent */
static void f221(Battle *b, int a)
{
    if (STATE(a) == 2) return;
    int hi = 77, lo = 1, moved = 0;
    if (MY(a) < 27) { hi = 59; lo = 20; }
    int d = MX(a) - MX(TARGET);
    if (d >= 0 && d < 4 && MX(a) < hi) { SET_X(a, MX(a) + 1); moved++; }
    else if (d <= 0 && d > -4 && MX(a) > lo) { SET_X(a, MX(a) - 1); moved++; }
    if (moved) place(b, a);
    WORK(40);
}

/* ovl7_f484: the nearest man of the other side that man a may fight */
static int f484(Battle *b, int a)
{
    int best = 500;
    SET_TARGET(-1);
    int side = SIDE(a), area = AREA(a), x = MX(a), y = MY(a);
    long work = 30;
    for (int i = 0; i < 40; i++) {
        work += 12;
        if (STATE(i) == 9 || SIDE(i) == side) continue;
        int ix = MX(i);
        work += 20;
        if (MOV(a) == 0 && KIND(a) != 1) {
            if (area == 4) {
                if (MY(i) < 40) continue;
            } else if (area == 3) {
                if (MY(i) > 23) continue;
                if (side == -1 && ix > 33) continue;
                if (side == 0 && ix < 42) continue;
            } else if (area != 5 && AREA(i) != 5) continue;
        }
        if (y < 27 && ((x < 35 && ix > 38) || (x > 38 && ix < 35))) continue;
        int d = abs(x - ix) + (abs(y - MY(i)) >> 1);
        if (MOV(i) != 0) d += 5;
        if (TGT(i) == a && best > 5) { best = 5; SET_TARGET(i); }
        if (best > d) { best = d; SET_TARGET(i); }
        work += 40;
    }
    WORK(work);
    if (KIND(a) == 1) return 1;
    if (MOV(a) != 0) best <<= 2;
    if (area == 1) best <<= 1;
    if (best > 20) SET_TARGET(-1);
    if (TARGET >= 0) {
        SET_TGT(a, TARGET);
        f3a9(b, a);
    }
    return 0;
}

/* ovl7_f3a9: man a's opponent, if near enough ([64D7]) */
static int f3a9(Battle *b, int a)
{
    WORK(30);
    if (KIND(a) == 1) { SET_TARGET(-1); return -1; }
    SET_TARGET(TGT(a));
    if (STATE(TARGET) == 9) { SET_TGT(a, -1); SET_TARGET(-1); }
    if (TARGET > -1) {
        int dx = abs(MX(a) - MX(TARGET)), dy = abs(MY(a) - MY(TARGET));
        if (dx > 5 || dy > 2) SET_TARGET(-2);
        return 0;
    }
    return f484(b, a);
}

/* ovl7_f6e5: which way to go to the opponent: +-160 a line pair down/up,
 * +-1 a byte right/left (0: stay) */
static int f6e5(Battle *b, int a)
{
    SET_TARGET(TGT(a));
    if (STATE(TARGET) == 5) return 0;
    int area = AREA(a);
    if (KIND(a) == 1) area = 2;
    int hi = 74, lo = 4;
    if (SIDE(a) == 0) {
        if (area == 3) lo = 54;
        else if (area == 1) lo = 40;
        else if (area != 5) lo = 30;
    } else {
        if (area == 3) hi = 26;
        else if (area == 1) hi = 40;
        else if (area != 5) hi = 50;
    }
    WORK(40);
    if (TARGET < 0) return 0;
    int r = 0, dy = MY(a) - MY(TARGET);
    if (dy < -1) r = 160;
    else if (dy > 1) r = -160;
    int dx = MX(a) - MX(TARGET);
    if (dx < -5 && MX(a) < hi) r++;
    else if (dx > 5 && MX(a) > lo) r--;
    return r;
}

/* ovl7_f06f: the walking script for the step (dx, dy); 0 when he has arrived */
static int f06f(Battle *b, int dx, int dy, int a)
{
    if (dy < 0) set_sp(b, dx > 0 ? 0x19BD : dx < 0 ? 0x1B8D : 0x19E5);
    else if (dy > 0) set_sp(b, dx > 0 ? 0x19D1 : dx < 0 ? 0x1BA1 : 0x19F3);
    else if (dx > 0) set_sp(b, 0x19A1);
    else if (dx < 0) set_sp(b, 0x1B71);
    WORK(30);
    if (dx != 0 || dy != 0) return 1;
    if (MOV(a) != 0) {
        SET_MOV(a, 0);
        int k = AREA(a);
        if (SIDE(a) == 0) k += 5;
        SET_CNT(k, CNT(k) + 1);
        if (SIDE(a) == -1) d444(b);
    }
    return 0;
}

/* ovl7_ec31: one of your men on his way to his part of the deck (the
 * tables at DS:193B give each part's box). Reaching the enemy's flag wins. */
static int ec31(Battle *b, int a)
{
    if (TGT(a) != -1) return 0;
    int dy = 0, dx = 0, x = MX(a), y = MY(a), k = AREA(a) - 1;
    if (mw(b, 0x193B + 2 * k) > x) dx++;
    if (mw(b, 0x1945 + 2 * k) < x) dx--;
    if (mw(b, 0x194F + 2 * k) > y) dy++;
    if (mw(b, 0x1959 + 2 * k) < y) dy--;
    WORK(60);
    if (y < 40 && k != 2) {
        if (k == 4) {
            if (x < 57) { dx = 1; dy = 0; }
            else {
                dx = 0; dy = 1;
                if (x > 59) x_step(b, a, -1);
                if (x < 59) x_step(b, a, 1);
            }
        } else if (x > 16) { dx = -1; dy = 0; }
        else { dx = 0; dy = 1; goto centre; }
    }
    if (k == 2 && y > 25) {
        if (y < 40) {
            dy = -1; dx = 0;
        centre:
            if (x > 15) x_step(b, a, -1);
            if (x < 15) x_step(b, a, 1);
        } else {
            dy = -1; dx = 0;
            if (y < 42 && x > 16) dy = 0;
            if (x > 16) dx = -1;
            if (x < 14) dx = 1;
        }
    }
    if (dx == 0 && dy == 0 && k == 4 && TGT(a) == -1) fb07(b);   /* at the enemy's flag */
    return f06f(b, dx, dy, a);
}

/* ovl7_ee50: the same for the enemy's men (tables at DS:1963); reaching your flag wins for him */
static int ee50(Battle *b, int a)
{
    if (TGT(a) != -1) return 0;
    int dy = 0, dx = 0, x = MX(a), y = MY(a), k = AREA(a) - 1;
    if (mw(b, 0x1963 + 2 * k) > x) dx++;
    if (mw(b, 0x196D + 2 * k) < x) dx--;
    if (mw(b, 0x1977 + 2 * k) > y) dy++;
    if (mw(b, 0x1981 + 2 * k) < y) dy--;
    WORK(60);
    if (y < 40 && k != 2) {
        if (k == 4) {
            if (x > 16) { dx = -1; dy = 0; }
            else {
                dx = 0; dy = 1;
                if (x > 15) x_step(b, a, -1);
                if (x < 15) x_step(b, a, 1);
            }
        } else if (x < 56) { dx = 1; dy = 0; }
        else { dx = 0; dy = 1; goto centre; }
    }
    if (k == 2 && y > 23) {
        if (y < 40) {
            dy = -1; dx = 0;
        centre:
            if (x > 59) x_step(b, a, -1);
            if (x < 59) x_step(b, a, 1);
        } else {
            dy = -1; dx = 0;
            if (y < 42 && x < 56) dy = 0;
            if (x > 60) dx = -1;
            if (x < 57) dx = 1;
        }
    }
    if (dx == 0 && dy == 0 && k == 4 && TGT(a) == -1) fa94(b);   /* at your flag */
    return f06f(b, dx, dy, a);
}

/* ovl7_ec05 */
static int ec05(Battle *b, int a) { return SIDE(a) == 0 ? ee50(b, a) : ec31(b, a); }

/* ovl7_e651: the next step of man a's script ([delta, picture, state]): move
 * him, draw him, and an officer's pistol shot */
static void e651(Battle *b, int a)
{
    if (MX(a) == 0) return;
    int clash = STATE(a) == 1 && STATE(TARGET) == 1;
    if (clash) b_sound(b, 0x3E8);
    int p = SCR(a);
    int d = mw(b, p);
    SET_ADDR(a, ADDR(a) + d);
    while (d > 10) { SET_Y(a, MY(a) + 1); d -= 80; }
    while (d < -10) { SET_Y(a, MY(a) - 1); d += 80; }
    SET_X(a, MX(a) + d);
    if (MY(a) < 20 && STATE(a) != 5) { SET_Y(a, MY(a) + 1); SET_ADDR(a, ADDR(a) + 80); }
    if (MY(a) < 36 || MX(a) > 74 || MX(a) < 1) {
        if (MX(a) > 59) x_step(b, a, -1);
        if (MX(a) < 14) x_step(b, a, 1);
    }
    ff14(b, uw(b, V_SPR + 2 * mw(b, p + 2)), ADDR(a), SIDE(a));
    SET_STATE(a, mw(b, p + 4));
    SET_SCR(a, p + 6);
    if (clash) b_sound_off(b);
    WORK(90);
    int step = AREA(a);
    if (KIND(a) != 1 || step == 0 || TGT(a) == -1) return;
    int spr = 0;
    if (step == 1) spr = 0x1905;
    if (step == 2) spr = 0x191F;
    if (step >= 3) spr = 0x1931;
    if (step > 3) SET_AREA(a, 0);
    SET_TGT(a, TGT(a) - 161);
    if (b_random(b) < 84 && AREA(a) != 0) SET_AREA(a, AREA(a) + 1);
    ff14(b, spr, TGT(a), -1);
}

/* ovl7_e926: an officer: stand, wave, or pick a man and shoot at him.
 * Returns 1 when the shot ends the fight. */
static int e926(Battle *b, int a)
{
    int shoot = 0, chance;
    b_random(b);
    int r = LAST_RANDOM;
    if (SIDE(a) == -1) {
        set_sp(b, 0x1A49);
        if (STATE(a) == 1) set_sp(b, 0x1AB3);
        else if (STATE(a) == 5) { if (r < 95) set_sp(b, 0x1AB3); }
        else if (r < 20) set_sp(b, 0x1A49);
        else if (r < 110) set_sp(b, 0x1A5D);
        else shoot = 1;
        chance = 20;
    } else {
        set_sp(b, 0x1BFD);
        if (STATE(a) == 1) set_sp(b, 0x1C67);
        else if (STATE(a) == 5) { if (r < 95) set_sp(b, 0x1C67); }
        else if (r < 20) set_sp(b, 0x1BFD);
        else if (r < 110) set_sp(b, 0x1C11);
        else shoot = 1;
        chance = OPP(0x6079);
    }
    WORK(40);
    if (!shoot) return 0;
    SET_MOV(a, 1);
    f484(b, a);
    if (TARGET == -1) return 0;
    int dx = MX(a) - MX(TARGET), dy = MY(a) - MY(TARGET), sy = 1, sx = 1, q;
    if (dx < 0) { dx = -dx; sx = -1; }
    if (dy < 0) { dy = -dy; sy = -1; }
    q = dy != 0 ? dx / dy : 100;
    if (q == 0) {
        q = dx != 0 ? dy / dx : 100;
        if (q >= 10) {
            if (sy < 0) { set_sp(b, 0x1B41); SET_FACE(a, 104); }
            else { set_sp(b, 0x1B27); SET_FACE(a, 103); }
            goto fire;
        }
        goto level;
    }
    if (q < 2) {
    level:
        if (sx > 0) { SET_FACE(a, 101); set_sp(b, sy < 0 ? 0x1C4D : 0x1C33); }
        else { SET_FACE(a, 102); set_sp(b, sy < 0 ? 0x1A99 : 0x1A7F); }
    } else if (sx < 0) { set_sp(b, 0x1A65); SET_FACE(a, 102); }
    else { set_sp(b, 0x1C19); SET_FACE(a, 101); }
fire:
    if (b_random(b) < chance && f86f(b, 1)) return 1;
    return 0;
}

/* ------------------------------------------------------------------ the end */

static int depth;   /* f86f -> fa94/fb07 -> fb5a -> def1 -> f86f: WAS.EXE could nest without end */

/* ovl7_fb5a: the last picture of the fight */
static void fb5a(Battle *b)
{
    for (int i = 0; i < 40; i++) SET_MOV(i, 1);
    sw(b, V_FRAME, 30);
    fe6b(b, FLD);
    if (depth < 8) { depth++; def1(b); depth--; }
    fec9(b, 0x550, 68, 0x132);
}

/* ovl7_fb9a */
static void fb9a(Battle *b)
{
    b_sound_off(b);
    g_clear_row(b, 23);
    g_clear_row(b, 24);
}

/* ovl7_fa94: you lost */
static void fa94(Battle *b)
{
    sw(b, V_RESULT, 2);
    fb5a(b);
    b_sound(b, 0x1C2C); b_delay(b, 0x1770);
    b_sound(b, 0x1C2C); b_delay(b, 0x1770);
    b_sound(b, 0x1FA0); b_delay(b, 0xBB8);
    b_sound(b, 0x237C); b_delay(b, 0x1770);
    b_sound(b, 0);      b_delay(b, 0x3E8);
    b_sound(b, 0x1C2C); b_delay(b, 0x2EE0);
    fb9a(b);
}

/* ovl7_fb07: you won */
static void fb07(Battle *b)
{
    sw(b, V_RESULT, 1);
    fb5a(b);
    for (int k = 0; k < 3; k++)
        for (int f = 0xBB8; f >= 0x3E8; f -= 0x1F4) { b_sound(b, f); b_delay(b, 0x5DC); }
    b_delay(b, 0x2710);
    fb9a(b);
}

/* ovl7_f86f: man [64D7] is struck down (strike: by an officer's shot).
 * Returns 1 when one side has nobody left. */
static int f86f(Battle *b, int shot)
{
    int t = TARGET, x = MX(t);
    if (FACE(t) == 102) {
        SET_SCR(t, shot ? 0x1AC1 : 0x1ACD);
        if (x < 10 || (MY(t) < 36 && x < 22) || (x > 38 && x < 41)) SET_SCR(t, 0x1AE3);
    } else {
        SET_SCR(t, shot ? 0x1C75 : 0x1C81);
        if (x > 70 || (MY(t) < 36 && x > 56) || (x > 32 && x < 36)) SET_SCR(t, 0x1C97);
    }
    fd25(b, t);
    SET_TGT(t, -1);
    int k = AREA(t);
    if (SIDE(t) == 0) k += 5;
    if (KIND(t) == 0 && MOV(t) == 0) {
        SET_CNT(k, CNT(k) - 1);
        if (CNT(k) < 0) SET_CNT(k, 0);
        if (CNT(k) == 0 && FROM == k) setmb(b, V_TO, 9);
    }
    if (SIDE(t) == -1) d444(b);
    int theirs = 0, yours = 0;
    for (int i = 0; i < 40; i++) {
        if (STATE(i) == 9 || i == t) continue;
        if (SIDE(i) == -1) yours++; else theirs++;
    }
    WORK(40 * 15 + 60);
    if (yours == 0) { fa94(b); return 1; }
    if (theirs == 0) { fb07(b); return 1; }
    return 0;
}

/* ------------------------------------------------------------------ a frame */

/* ovl7_def1: move and draw every man, then the two flags */
static void def1(Battle *b)
{
    if (COUNT != 0 && COUNT < 11) sw(b, V_COUNT, COUNT - 1);
    for (int n = 0; n < 40; n++) {
        int a = ORD(n);
        sw(b, 0x6DBA, A_FACE + a);
        sw(b, V_SP, V_SCRIPT + 2 * a);
        int x = MX(a);
        WORK(40);
        if (MX(a) == 0) continue;
        f3a9(b, a);
        if (KIND(a) == 1 && STATE(a) == 2) {
            /* an officer fires */
            for (int f = 0x2BC; f < 0xDAC; f += 0x190) { b_sound(b, f); b_delay(b, 100); }
            b_sound_off(b);
            SET_AREA(a, 1);
            SET_TGT(a, ADDR(a) + 2);
            int d = 0;
            if (FACE(a) == 102) d = 2;
            if (FACE(a) == 101) d = -4;
            if (FACE(a) == 103) d = -160;
            SET_TGT(a, TGT(a) + d);
        }
        int w = mw(b, SCR(a));
        if (w == 102 || w == 101) {
            /* his script is done: the next */
            SET_FACE(a, w);
            if (COUNT != 0) {
                /* waiting for the attack */
                if (SIDE(a) == -1) {
                    set_sp(b, 0x198B);
                    if (b_random(b) < 20) set_sp(b, 0x1993);
                    if (KIND(a) == 1) set_sp(b, 0x1A49);
                } else {
                    set_sp(b, 0x1B5B);
                    if (b_random(b) < 20) set_sp(b, 0x1B63);
                    if (KIND(a) == 1) set_sp(b, 0x1BFD);
                }
                e651(b, a);
                continue;
            }
            if (STATE(a) == 9) {
                /* dead: over the side, or lying on the deck */
                set_sp(b, FACE(a) == 102 ? 0x1ADB : 0x1C8F);
                int d = b_random(b) & 3;
                if (x > 32 && x < 40) { SET_X(a, 0); continue; }
                if (MY(a) < 40) {
                    if (x > 56) d = -d;
                    else if (x >= 16 || x == 0) d = 0;
                } else {
                    if (x > 30 && x < 33) d = -d;
                    else if (x <= 39 || x >= 42) d = 0;
                }
                x_step(b, a, d);
                goto strike;
            }
            if (RESULT != 0) {
                /* the fight is over: the winners cheer, the others give up */
                b_random(b);
                bool cheer = RESULT == 1 ? SIDE(a) == -1 : SIDE(a) == 0;
                if (cheer) {
                    if (MOV(a) != 0) set_sp(b, 0x1AF1);
                    else if (LAST_RANDOM < 42) set_sp(b, 0x1B07);
                    else if (LAST_RANDOM < 84) set_sp(b, 0x1AF7);
                    else set_sp(b, 0x1AFF);
                    if (KIND(a) == 1) set_sp(b, 0x1B0F);
                } else {
                    set_sp(b, 0x1B17);
                    if (KIND(a) == 1) set_sp(b, 0x1B1F);
                }
                SET_MOV(a, 0);
                goto strike;
            }
            if (x >= 31 && x <= 34 && MY(a) < 27) {
                /* at your side of the gap: across, or back */
                if (SIDE(a) == -1) set_sp(b, AREA(a) == 5 ? 0x1A2F : 0x1B85);
                else set_sp(b, 0x1B71);
                SET_TARGET(-1);
                SET_TGT(a, -1);
                goto strike;
            }
            if (x >= 39 && x <= 42 && MY(a) < 27) {
                /* at the enemy's side of it */
                if (SIDE(a) == 0 && AREA(a) == 5) set_sp(b, 0x1BE3);
                else set_sp(b, 0x19A1);
                SET_TARGET(-1);
                SET_TGT(a, -1);
                goto strike;
            }
            if (KIND(a) == 1) {
                if (e926(b, a)) goto done;
                goto strike;
            }
            if (TARGET >= 0) {
                /* fight him */
                f221(b, a);
                b_random(b);
                if (MX(TARGET) >= x) set_sp(b, LAST_RANDOM < 90 ? 0x1A01 : 0x1A1B);
                else set_sp(b, LAST_RANDOM < 90 ? 0x1BB5 : 0x1BCF);
                goto strike;
            }
            if (ec05(b, a)) goto strike;
            switch (f6e5(b, a)) {
            case 0:
                set_sp(b, SIDE(a) != -1 ? 0x1B5B : 0x198B);
                f17a(b, a);
                break;
            case -161: set_sp(b, 0x1B8D); break;
            case -160:                                   /* WAS.EXE falls through to -159's */
            case -159: set_sp(b, 0x19BD); break;
            case -1:   set_sp(b, 0x1B71); break;
            case 1:    set_sp(b, 0x19A1); break;
            case 161:  set_sp(b, 0x19D1); break;
            case 160:  set_sp(b, 0x19F3); break;
            default:   set_sp(b, 0x1BA1); break;
            }
        }
    strike:
        if (STATE(a) == 2 && STATE(TARGET) != 9 && TARGET >= 0) {
            /* his blow */
            int chance = SIDE(a) != 0 ? 20 : OPP(0x607B);
            if (b_random(b) < chance || KIND(TARGET) == 1) {
                if (b_random(b) < 45) {
                    b_sound(b, 0x32C8);
                    if (f86f(b, 0)) goto done;
                    b_delay(b, 0x190);
                    b_sound_off(b);
                }
            }
        }
        e651(b, a);
    }
done:
    fc5e(b);
    /* the flags: struck by the loser */
    b_random(b);
    int left = -1, right = 0;
    if (RESULT == 1) right = -1;
    if (RESULT == 2) left = 0;
    int spr = 0x2E0B;
    if (LAST_RANDOM < 53) spr = 0x28F5;
    else if (LAST_RANDOM < 75) spr = 0x2D79;
    ff14(b, spr, FLD + 0x12C0, left);
    b_random(b);
    spr = 0x3785;
    if (LAST_RANDOM < 53) spr = 0x2B4D;
    else if (LAST_RANDOM < 75) spr = 0x3563;
    ff14(b, spr, FLD + 0x130B, right);
}

/* ------------------------------------------------------------------ the start */

/* where a man starts: the part of the deck, picked by a random number */
static void start_at(Battle *b, int i, int x, int y) { SET_X(i, x); SET_Y(i, y); }

/* boarding_load_b (ovl7:CBC8): load the deck and line up the crews. a0 and
 * a1 are the two ships; your crew (side -1) is the larger the less damaged
 * your ship is compared with the other, ships of the line carry more than
 * frigates, and a third ship alongside helps. */
static int load_b(Battle *b, int a0, int a1)
{
    sw(b, V_RESULT, 0);
    sw(b, V_POOP, 0);
    setmb(b, V_TO, 9);
    setmb(b, V_FROM, 9);
    sw(b, V_COUNT, 20);
    size_t n = 0;
    void *d = assets_load_file(b->ga->dir, "B", &n);
    if (!d) return 9999;
    memcpy(&b->m[BUF], d, n < 0x1770 ? n : 0x1770);
    SDL_free(d);
    /* the pointers to the men's arrays (DS:6CF4-6D08), as WAS.EXE sets them */
    static const int ptrs[11] = { A_KIND, A_SIDE, A_ADDR, A_X, A_Y, A_FACE, A_STATE, A_AREA, A_TGT, A_ORD, A_MOV };
    for (int k = 0; k < 11; k++) sw(b, 0x6CF4 + 2 * k, ptrs[k]);
    sw(b, 0x3F37, BUF);
    sw(b, 0x3F39, FLD);

    if (b->owner[a0] == 0) { int t = a0; a0 = a1; a1 = t; }
    int yours = 20 - 2 * b->hull[a0] + 2 * b->hull[a1];
    if (b->type[a0] == 2) yours -= 5;
    if (b->type[a1] == 2) yours += 5;
    if (fd62(b, a0)) yours += 5;
    if (fd62(b, a1)) yours -= 5;
    if (yours > 25) yours = 25;
    if (yours < 15) yours = 15;

    for (int i = 0; i < 40; i++) {
        SET_ORD(i, i);
        SET_TGT(i, -1);
        SET_AREA(i, 0);
        SET_MOV(i, 0);
        SET_STATE(i, 0);
        int r8 = b_random(b) & 3, r4 = b_random(b) & 7, r6 = b_random(b) & 3, r2 = b_random(b) & 7;
        WORK(60);
        if (yours != 0) {
            yours--;
            SET_FACE(i, 102);
            SET_SIDE(i, -1);
            if (b_random(b) > 100) {
                /* an officer: on the quarterdeck or poopdeck, anywhere free */
                SET_KIND(i, 1);
                SET_SCR(i, 0x1A49);
                for (;;) {
                    int r = b_random(b);
                    if (r < 30) start_at(b, i, r4 + 24, r2 + 58);
                    else if (r < 60) start_at(b, i, r4 + 25, r8 + 34);
                    else if (r < 90) start_at(b, i, r8 + 17, r6 + 20);
                    else start_at(b, i, r8 + 4, r4 + 46);
                    if (!f327(b, i) || LAST_RANDOM < 3) break;
                }
            } else {
                SET_KIND(i, 0);
                SET_SCR(i, 0x198B);
                int r = b_random(b);
                if (r < 27) { SET_AREA(i, 1); start_at(b, i, r8 + 3, r4 + 60); }
                else if (r < 74) { SET_AREA(i, 4); start_at(b, i, r8 + 28, r4 + 41); }
                else if (r < 120) { SET_AREA(i, 3); start_at(b, i, r4 + 21, r8 + 20); }
                else { SET_AREA(i, 2); start_at(b, i, r8 + 4, r4 + 46); }
            }
        } else {
            SET_FACE(i, 101);
            SET_SIDE(i, 0);
            if (b_random(b) > OPP(0x6077)) {
                SET_KIND(i, 1);
                SET_SCR(i, 0x1BFD);
                for (;;) {
                    int r = b_random(b);
                    if (r < 30) start_at(b, i, r4 + 46, r2 + 58);
                    else if (r < 60) start_at(b, i, r4 + 40, r8 + 34);
                    else if (r < 90) start_at(b, i, r8 + 55, r6 + 20);
                    else start_at(b, i, r8 + 70, r4 + 46);
                    if (!f327(b, i) || LAST_RANDOM < 3) break;
                }
            } else {
                SET_KIND(i, 0);
                SET_SCR(i, 0x1B5B);
                int r = b_random(b);
                if (r < 32) { SET_AREA(i, 1); start_at(b, i, r8 + 70, r4 + 60); }
                else if (r < 64) { SET_AREA(i, 4); start_at(b, i, r8 + 40, r4 + 41); }
                else if (r < 96) { SET_AREA(i, 3); start_at(b, i, r4 + 50, r8 + 20); }
                else { SET_AREA(i, 2); start_at(b, i, r8 + 70, r4 + 46); }
            }
        }
        place(b, i);
    }
    fbb3(b);
    for (int i = 0; i < 40; i++) fc5e(b);
    /* the men's pictures (DS:6D5A) */
    static const uint16_t SPR[48] = {
        0x3341, 0x329F, 0x1CD5, 0x1D67, 0x38C9, 0x3827, 0x2233, 0x22C5, 0x1EF7, 0x1F71, 0x1FEB, 0x3D2D,
        0x2455, 0x3E19, 0x33F7, 0x397F, 0x1E1D, 0x237B, 0x2F4F, 0x2EAD, 0x204D, 0x20EF, 0x2191, 0x3D77,
        0x27F1, 0x273F, 0x3133, 0x2893, 0x31E9, 0x3041, 0x3B71, 0x2C91, 0x24B7, 0x2559, 0x25FB, 0x3E63,
        0x2AAB, 0x29F9, 0x3619, 0x2997, 0x36CF, 0x269D, 0x2BEF, 0x2D0B, 0x34F5, 0x3A7D, 0x3AF7, 0x3C63,
    };
    for (int k = 0; k < 48; k++) sw(b, V_SPR + 2 * k, SPR[k]);
    fe6b(b, FLD);
    def1(b);
    fec9(b, 0, 0, 0x16C);
    /* the Tandy version's mode 9 cleared the screen: it draws the line under the
       deck again (text_rows_add, WAST ovl4:7A5F) */
    for (int xb = 0; xb < 80; xb++) { b->vram_t[row_addr(182) + xb] = 0xFF; b->vram_t[row_addr(183) + xb] = 0; }
    b->row16[182] = b->row16[183] = false;
    d444(b);
    g_clear_row(b, 23);
    while (b_getkey(b)) ;
    return 0;
}

/* the debug log: the men standing on a side (-1 yours, 0 the enemy's), and their officers */
static int men_standing(const Battle *b, int side, int *officers)
{
    int n = 0;
    *officers = 0;
    for (int i = 0; i < 40; i++) {
        if (SIDE(i) != side || STATE(i) == 9 || MX(i) == 0) continue;
        n++;
        if (KIND(i) == 1) (*officers)++;
    }
    return n;
}

/* ovl7_c9b0: board. o is the ship grappled, s the one that grappled her.
 * Returns the ship that won the fight, or -1 if you Leave. Afterwards the
 * two ships lie side by side, the loser turned to the winner's heading. */
static int c9b0(Battle *b, int a0, int a1)
{
    int r;
    sw(b, V_FRAME, 0);
    if (load_b(b, a0, a1) == 9999) { r = -1; goto out; }
    int log_yours = 0, log_theirs = 0, off_y = 0, off_t = 0;
    if (blog_on(b, 1)) {
        log_yours = men_standing(b, -1, &off_y);
        log_theirs = men_standing(b, 0, &off_t);
        BLOGN("the boarding begins: %s and %s; your crew %d men (%d officers), theirs %d (%d officers)", blog_ship(b, a0),
              blog_ship(b, a1), log_yours, off_y, log_theirs, off_t);
    }
    for (;;) {
        fe6b(b, FLD);
        def1(b);
        if (blog_on(b, 1)) {
            int y = men_standing(b, -1, &off_y), t = men_standing(b, 0, &off_t);
            if (y != log_yours || t != log_theirs)
                BLOGN("boarding: %s falls; you have %d men standing, they %d", y < log_yours && t < log_theirs ? "a man on each side" :
                      y < log_yours ? "one of your men" : "one of their men", y, t);
            log_yours = y;
            log_theirs = t;
        }
        fec9(b, 0x550, 68, 0x132);
        sw(b, V_FRAME, mw(b, V_FRAME) - 1);
        if (mw(b, V_FRAME) < 0) {
            sw(b, V_FRAME, 5);
            if (COUNT == 0) dbac(b);
        }
        if (d579(b) == -1) { r = -1; goto out; }
        if (RESULT != 0 && mw(b, V_FRAME) < 10) break;
        /* [0D19] > 1500 (a fast PC) would wait here: an XT doesn't */
    }
    {
        int g = b->heading[a0] - b->heading[a1];
        if (g < 0) g = -g;
        if (g > 8) g = g - (16 - g);       /* sic */
        /* the enemy's ship turns to lie alongside yours */
        int t = b->owner[a0] == 0 ? a0 : a1, k = t == a0 ? a1 : a0;
        b->heading[t] = b->heading[k];
        if (g > 4) b->heading[t] += 8;
        if (b->heading[t] > 15) b->heading[t] -= 16;
        b->turning[t] = 0;
    }
    r = a0;
    if ((RESULT == 1 && b->owner[a0] == 0) || (RESULT == 2 && b->owner[a0] == -1)) r = a1;
out:
    g_clear_row(b, 23);
    g_clear_row(b, 24);
    return r;
}

/* the call from ovl5_bc12: SO8 is freed for the boarding and loaded again after it */
int ovl7_boarding(Battle *b, int o, int s)
{
    depth = 0;
    t_load(b);
    int r = c9b0(b, o, s);
    /* the Tandy version: mode 4 and its colours again (collisions ovl4:4184), the screen cleared */
    b->t9 = false;
    memset(b->vram_t, 0, sizeof b->vram_t);
    g_tandy_colours(b);
    size_t n = 0;
    void *d = assets_load_file(b->ga->dir, "SO8", &n);
    if (d) {
        memcpy(&b->m[BMEM_SO8], d, n < 0x483C ? n : 0x483C);
        SDL_free(d);
    }
    return r;
}
