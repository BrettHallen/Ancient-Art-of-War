/*
 * The Ancient Art of War at Sea (1987) - remake
 * Nov 2026 using Claude Opus 5.5
 * "Guided" by Brett Hallen
 * Original game by Dave Murry & Barry Murry
 * Published by Brøderbund Software Inc.
 *
 * battle_gfx.c: WAS.EXE's drawing routines for the sea battle, on the CGA
 * screen as WAS.EXE has it (B800:0000, 80 bytes a line, even lines first,
 * odd lines at 2000h; the row table DS:0D3B). Each routine names the one it
 * ports; the arithmetic, the clipping and the byte masks are WAS.EXE's.
 */
#include "battle_int.h"

/* DS:0D3B, the row table WAS.EXE fills for CGA (ovl1:CE78 fills it for Hercules
 * the same way: the remake shows Hercules from the CGA screen) */
int row_addr(int row) { return (row & 1) * 0x2000 + (row >> 1) * 80; }

static inline uint8_t *V(Battle *b, int addr) { return &b->vram[addr & 0x3FFF]; }

/* The time these routines take on an XT: about their 8086 instructions (counted
 * from the asm: per byte and per line), at the 300 instructions a millisecond of
 * DOSBox's "cycles=300" (see battle.c) */
#define WORK(b, n) ((b)->clock_us += (n) * (1000.0 / 300.0))

/* ------------------------------------------------------------------ text */

void g_goto(Battle *b, int col, int row) { b->tcol = col; b->trow = row; }   /* goto_xy rt:2EBF */

void g_clear_row(Battle *b, int row)   /* clear_text_row rt:2C48 */
{
    WORK(b, 400);
    for (int i = 0; i < 8; i++) {
        int a = row_addr(row * 8 + i);
        for (int k = 0; k < 80; k++) *V(b, a + k) = 0;
    }
    g_goto(b, 0, row);
}

void g_putc(Battle *b, int c)   /* rt_32d4 */
{
    if (c == 10) { b->trow++; return; }
    if (c == 13) { b->tcol = 0; return; }
    if (c == 8) { if (b->tcol > 0) b->tcol--; return; }
    WORK(b, 120);
    int src = 0x89C + (c - 0x20) * 8;
    for (int i = 0; i < 8; i++) {
        int a = row_addr(b->trow * 8 + i) + b->tcol * 2;
        int g = mb(b, src + i);
        *V(b, a) = (uint8_t)mb(b, 0x88C + (g >> 4));
        *V(b, a + 1) = (uint8_t)mb(b, 0x88C + (g & 15));
    }
    if ((unsigned)++b->tcol >= 40) { b->tcol = 0; b->trow++; }
}

void g_print(Battle *b, int ds)   /* print_string rt:3355 */
{
    for (int a = ds; mb(b, a) != 0; a++) g_putc(b, mb(b, a));
}

void g_prints(Battle *b, const char *s) { while (*s) g_putc(b, (uint8_t)*s++); }

void g_number(Battle *b, int n)   /* print_number rt:26D7 */
{
    if (n >= 10) { g_number(b, n / 10); g_putc(b, n % 10 + '0'); }
    else g_putc(b, n + '0');
}

void g_spaces(Battle *b, int n) { while (n-- > 0) g_putc(b, ' '); }   /* print_spaces rt:2EE0 */

/* ------------------------------------------------------------------ lines and icons */

void g_hline(Battle *b, int line, int x, int len)   /* draw_hline rt:2B95 */
{
    WORK(b, 15 + len);
    int a = row_addr(line) + x;
    for (int i = 0; i < len; i++) *V(b, a + i) = 0xFF;
}

void g_vline(Battle *b, int line, int x, int n)   /* draw_vline rt:2B71: 2n lines of 0Fh */
{
    WORK(b, 15 + 12 * n);
    for (int i = 0; i < 2 * n; i++) *V(b, row_addr(line + i) + x) = 0x0F;
}

void g_icon(Battle *b, int src, int line, int x)   /* draw_icon rt:2CC4 */
{
    int w = mb(b, src), h = mb(b, src + 1) * 2;
    src += 2;
    WORK(b, 20 + h * (w + 8));
    for (int r = 0; r < h; r++) {
        int a = row_addr(line + r) + x;
        for (int i = 0; i < w; i++) *V(b, a + i) = (uint8_t)mb(b, src++);
    }
}

void g_invert_block(Battle *b, int addr, int w, int h)   /* invert_block rt:2DC8 */
{
    int row = (addr / 80) * 2, x = addr % 80;
    WORK(b, 20 + 2 * h * (3 * w + 6));
    for (int r = 0; r < 2 * h; r++) {
        int a = row_addr(row + r) + x;
        for (int i = 0; i < w; i++) *V(b, a + i) ^= 0xFF;
    }
}

void g_palette(Battle *b, int n) { b->palette = n; }   /* set_cga_palette_if rt:30AE */

/* ------------------------------------------------------------------ the gun crew's panel */

void g_panel_copy(Battle *b, int off)   /* rt_6697: 24 bytes of each of 18 buffer lines to lines 2-19 at byte 12 */
{
    int src = PANEL + off;
    WORK(b, 351);
    for (int r = 2; r < 20; r++) {
        int a = row_addr(r) + 12;
        for (int i = 0; i < 24; i++) *V(b, a + i) = (uint8_t)mb(b, src + i);
        src += 0x2C;
    }
}

void g_panel_sprite(Battle *b, int spr, int off, int half, int over)   /* rt_656f */
{
    int w = mb(b, spr), rows = mb(b, spr + 1) * 2;
    int si = spr + 2, row = PANEL + off;
    WORK(b, 20 + rows * (8 + w * (half ? 30 : 22)));
    while (rows-- > 0) {
        int di = row, carry = 0;
        for (int i = 0; i <= w; i++) {
            int al;
            if (i < w) {
                al = mb(b, si++);
                if (half) {
                    int ah = (al >> 4) | carry;
                    carry = (al << 4) & 0xFF;
                    al = ah;
                }
            } else {
                if (!carry) break;
                al = carry; carry = 0;
            }
            int bl = over ? 0 : mb(b, di);
            bl &= ~(al >> 2) & 0xFF;
            bl &= ~(al << 2) & 0xFF;
            setmb(b, di, al | bl);
            di++;
        }
        row += 0x2C;
    }
}

/* ------------------------------------------------------------------ the battle view (lines 34-181) */

/* rt_617a(spr, y, colour, x, half): the sprite as a mask, filled with the colour byte */
void g_mask(Battle *b, int spr, int y, int colour, int x, int half)
{
    if (y < -24 || y > 179) return;
    int row = 2 * y + 34;
    int w = mb(b, spr), rows = mb(b, spr + 1) * 2, si = spr + 2;
    if (x > 80 || x < -14) return;
    WORK(b, 20 + rows * (12 + w * (half ? 26 : 18)));
    while (rows-- > 0) {
        if (row < 34) { si += w; row++; continue; }
        if (row >= 182) return;
        int base = row_addr(row), dx = x, bh = 0;
        for (int i = 0; ; i++) {
            int al;
            if (i < w) {
                al = mb(b, si++);
                if (half) { int ah = (al >> 4) | bh; bh = (al << 4) & 0xFF; al = ah; }
            } else {
                if (bh == 0) break;
                al = bh; bh = 0;
            }
            if (dx >= 0 && dx <= 79) {
                uint8_t *p = V(b, base + dx);
                *p = (uint8_t)((al & colour) | (*p & ~al));
            }
            dx++;
        }
        row++;
    }
}

/* rt_6242(spr, y, x, hull): the sea (66h) over the sprite's box, a line pair higher and
 * one byte and two line pairs bigger (fewer lines as the ship sinks) */
void g_erase(Battle *b, int spr, int y, int x, int hull)
{
    int lim = (35 - hull) & 0xFF;
    if (y < -24) return;
    int by = y - 1;
    if (by > 179) return;
    int row = 2 * by + 34;
    int w = mb(b, spr) + 1, h = mb(b, spr + 1);
    if (h >= lim) h = lim;
    int rows = ((h + 2) & 0xFF) * 2;
    int bp = x, dx = w;
    if (dx > 80 - bp) {
        dx = 80 - bp;
        if (dx <= 0) return;
    } else if (bp < 0) {
        if (row > 182) return;
        int cx = dx + bp;
        if (cx <= 0) return;
        bp = 0;
        dx = cx;
    }
    WORK(b, 30 + rows * (10 + dx));
    while (rows-- > 0) {
        if (row >= 34) {
            if (row >= 182) return;
            int a = row_addr(row) + bp;
            for (int i = 0; i < dx; i++) *V(b, a + i) = 0x66;
        }
        row++;
    }
}

void g_box(Battle *b, int x, int y, int on)   /* rt_62dc: the overview's box, 16 bytes by 28 lines */
{
    uint8_t v = on ? 0xFF : 0x66;
    int r = 2 * y;
    WORK(b, 460);
    for (int i = 0; i < 16; i++) {
        *V(b, row_addr(r) + x + i) = v;
        *V(b, row_addr(r + 28) + x + i) = v;
        *V(b, row_addr(r + 1) + x + i) = v;
        *V(b, row_addr(r + 29) + x + i) = v;
    }
    for (int i = 0; i < 28; i++) {
        int a = row_addr(r + i) + x;
        *V(b, a) = v;
        *V(b, a + 15) = v;
    }
}

void g_sea(Battle *b)   /* rt_5dc5 */
{
    WORK(b, 148 * 47);
    for (int r = 34; r <= 181; r++) {
        int a = row_addr(r);
        for (int i = 0; i < 80; i++) *V(b, a + i) = 0x66;
    }
}

void g_shore_overview(Battle *b, int rle)   /* rt_5de5: one coded line a screen line */
{
    int si = rle;
    for (int r = 34; r < 34 + 0x94; r++) {
        int di = row_addr(r);
        for (;;) {
            int n = mb(b, si++);
            WORK(b, 6);
            if (n == 0x63) break;
            WORK(b, n);
            int v = mb(b, si++);
            for (int i = 0; i < n; i++) *V(b, di++) = (uint8_t)v;
        }
    }
}

/* rt_5e16(rle, xoff): the close view: each coded line from xoff on, four times as wide,
 * on four screen lines; the shore's edge bytes (76h, 67h) become sea */
void g_shore_close(Battle *b, int rle, int xoff)
{
    uint8_t line[256];
    int src = rle, r = 34, left = 0x94;
    for (;;) {
        int n = 0;
        int si = src;
        for (;;) {
            int al = mb(b, si++);
            if (al == 0x76 || al == 0x67) al = 0x66;
            line[n++ & 255] = (uint8_t)al;
            if (al == 0x63) break;
        }
        src = si;
        /* skip xoff bytes (8-bit, signed compare) */
        int k = 0, d = 0;
        int8_t cl = (int8_t)xoff;
        for (;;) {
            int8_t al = (int8_t)line[k++ & 255];
            uint8_t ch = line[k++ & 255];
            if (cl >= al) { cl = (int8_t)(cl - al); continue; }
            line[d++ & 255] = (uint8_t)(al - cl);
            line[d++ & 255] = ch;
            break;
        }
        for (;;) {
            uint8_t al = line[k++ & 255];
            line[d++ & 255] = al;
            if (al == 0x63) break;
        }
        /* four times as wide, at most 80 bytes */
        int8_t ah = 0x50;
        k = 0; d = 0;
        while (ah > 0) {
            int8_t al = (int8_t)line[k++ & 255];
            if (al > 0x15) al = ah;
            else {
                al = (int8_t)(al << 2);
                if (ah < al) al = ah;
            }
            ah = (int8_t)(ah - al);
            line[d++ & 255] = (uint8_t)al;
            line[d++ & 255] = line[k++ & 255];
        }
        line[d & 255] = 0x63;
        WORK(b, 260);
        for (int rep = 0; rep < 4; rep++) {
            WORK(b, 100);
            int di = row_addr(r);
            for (k = 0; ; ) {
                int c = line[k++ & 255];
                if (c == 0x63) break;
                int v = line[k++ & 255];
                for (int i = 0; i < c; i++) *V(b, di++) = (uint8_t)v;
            }
            r++;
            if (--left == 0) return;
        }
    }
}

/* rt_5ed1(spr, y, set, x, half, sails, type, hull): a ship. Yours (set != 0): the
 * picture with the pixels beside it within each byte cleared; theirs (set 0): the
 * picture cleared out of the sea with those pixels set. Torn sails leave holes,
 * a damaged hull single pixels, and a sinking ship shows fewer lines. */
void g_ship(Battle *b, int spr, int y, int set, int x, int half, int sails, int type, int hull)
{
    if (y < -24 || y > 179) return;
    int row = 2 * y + 34;
    int8_t ba7 = (int8_t)hull;
    int8_t ba5 = type == 2 ? 0x14 : 0x1C;
    int w = mb(b, spr), rows = mb(b, spr + 1) * 2, si = spr + 2;
    int ax = 9 - sails;
    if (ax <= 0) ax = 0;
    if (ax <= 1) ax = 2;
    if (ax == 9) ax = 0x78;
    int8_t ba3 = (int8_t)ax, ba4 = (int8_t)ax;
    int8_t ba6 = (int8_t)((0x1D - ba7) * 2);
    if (x > 80 || x < -14) return;
    WORK(b, 40 + rows * (18 + w * (half ? 38 : 30)));
    while (rows-- > 0) {
        if (row < 34) {
            si += w;
        } else {
            if (row >= 182) return;
            if (ba6 <= 0) return;
            int base = row_addr(row), dx = x, bh = 0;
            for (int i = 0; ; i++) {
                int al;
                if (i < w) {
                    al = mb(b, si++);
                    if (al & 3) ba4--;
                    if (ba5 == 0) {
                        int8_t ah = (int8_t)(9 - ba7);
                        if (ah <= 0) ah = 0;
                        if (ah <= 1) ah = 2;
                        ba3 = ba4 = ah;
                    }
                    if (ba3 != 9) {
                        ba4--;
                        if (ba4 <= 0) {
                            ba4 = ba3;
                            if (ba5 > 0) al = 0;
                            else {
                                int sel = (ba5 ^ dx) & 3;
                                al &= sel == 0 ? 0xC0 : sel == 1 ? 0x30 : 0x0C;
                            }
                        }
                    }
                    if (half) { int ah2 = (al >> 4) | bh; bh = (al << 4) & 0xFF; al = ah2; }
                } else {
                    if (bh == 0) break;
                    al = bh; bh = 0;
                }
                if (dx >= 0 && dx <= 0x4F) {
                    uint8_t *p = V(b, base + dx);
                    int bl = *p;
                    if (set) {
                        bl &= ~(al >> 2) & 0xFF;
                        bl &= ~(al << 2) & 0xFF;
                        *p = (uint8_t)(al | bl);
                    } else {
                        bl |= al >> 2;
                        bl |= (al << 2) & 0xFF;
                        *p = (uint8_t)(bl & ~al);
                    }
                }
                dx++;
            }
        }
        row++;
        ba5--;
        ba6--;
    }
}

/* ------------------------------------------------------------------ the panel at the top */

static int mast_mask(int col, int dmg, int al)   /* rt_64a9: the sails torn, by column */
{
    static const int need[7] = { 0, 1, 3, 5, 99, 4, 2 };
    if (col >= 1 && col <= 6 && col != 4 && dmg >= need[col]) return al & 0x55;
    return al;
}

/* rt_6344(set, dmg): the Speed picture: the sails set (0-6) on the mast, torn by damage */
void g_mast(Battle *b, int set, int dmg)
{
    WORK(b, 600);
    int si = 0x1080, dx = set;
    for (int bp = 1; bp <= 6; bp++) {
        int al = dx > 0 ? mb(b, si) : 0;
        al = mast_mask(bp, dmg, al); *V(b, row_addr(14) + bp) = (uint8_t)al;
        al = mast_mask(bp, dmg, al); *V(b, row_addr(15) + bp) = (uint8_t)al;
        si++;
    }
    for (int bp = 1; bp <= 6; bp++) {
        int al = dx > 0 ? mb(b, si) : 0;
        al = mast_mask(bp, dmg, al); *V(b, row_addr(16) + bp) = (uint8_t)al;
        al = mast_mask(bp, dmg, al); *V(b, row_addr(17) + bp) = (uint8_t)al;
        si++;
    }
    dx--;
    for (int bp = 1; bp <= 6; bp++) {
        int al = 0;
        if (dx > 0) al = mb(b, si++);
        for (int r = 18; r <= 21; r++) { al = mast_mask(bp, dmg, al); *V(b, row_addr(r) + bp) = (uint8_t)al; }
    }
    dx--;
    for (int ah = 4; ah > 0; ah--) {
        int row = ah == 4 ? 8 : ah == 3 ? 10 : ah == 2 ? 2 : 4;
        for (int bp = 1; bp <= 6; bp++) {
            int al = 0;
            if (dx > 0) al = mb(b, si++);
            al = mast_mask(bp, dmg, al); *V(b, row_addr(row) + bp) = (uint8_t)al;
            al = mast_mask(bp, dmg, al); *V(b, row_addr(row + 1) + bp) = (uint8_t)al;
        }
        dx--;
    }
    for (int r = 0; r < 24; r++) {
        uint8_t *p = V(b, row_addr(r) + 4);
        *p = (uint8_t)((*p & 0x3F) | 0x80);
    }
    *V(b, row_addr(1) + 3) = 0x2A; *V(b, row_addr(1) + 4) = 0xAA;
    *V(b, row_addr(7) + 1) = 0x00; *V(b, row_addr(7) + 2) = 0x02;
    *V(b, row_addr(7) + 3) = 0xAA; *V(b, row_addr(7) + 4) = 0xAA;
    *V(b, row_addr(13) + 3) = 0xAA; *V(b, row_addr(13) + 4) = 0xAA;
    *V(b, row_addr(7) + 5) = 0xA0; *V(b, row_addr(7) + 6) = 0x00;
    *V(b, row_addr(13) + 5) = 0xAA; *V(b, row_addr(13) + 6) = 0x00;
    *V(b, row_addr(13) + 1) = 0x00; *V(b, row_addr(13) + 2) = 0x2A;
}

/* rt_64f0(n): the speed (0-9) as a bar at byte 8 */
void g_speed_bar(Battle *b, int n)
{
    int bx = 0x28, dx = n;
    while (dx > 0) {
        *V(b, row_addr(bx / 2 + 1) + 8) = 0x55;
        if (bx >= 0x24) { if (--dx == 0) break; }
        *V(b, row_addr(bx / 2) + 8) = 0x55;
        bx -= 4;
        if (bx == 8 || bx == 0x14 || bx == 0x1C) continue;
        dx--;
    }
    dx = n;
    while (bx > 0) {
        *V(b, row_addr(bx / 2) + 8) = 0;
        if (dx == 3 || dx == 1) dx = 9;
        else *V(b, row_addr(bx / 2 + 1) + 8) = 0;
        bx -= 4;
    }
}

void g_needle(Battle *b, int spr, int x, int row)   /* rt_660c: half a byte to the right, overwriting */
{
    int w = mb(b, spr), h = mb(b, spr + 1), si = spr + 2;
    WORK(b, 20 + 2 * h * (10 + 12 * w));
    for (int k = 0; k < 2 * h; k++) {
        int a = row_addr(row + k) + x, carry = 0;
        for (int i = 0; i < w; i++) {
            int al = mb(b, si++);
            *V(b, a++) = (uint8_t)((al >> 4) | carry);
            carry = (al << 4) & 0xFF;
        }
        *V(b, a) = (uint8_t)carry;
    }
}

void g_fill(Battle *b, int x, int line, int w, int h, int v)   /* rt_66e2 (AAh), rt_6712 (FFh) */
{
    for (int r = 0; r < 2 * h; r++) {
        int a = row_addr(line + r) + x;
        for (int i = 0; i < w; i++) *V(b, a + i) = (uint8_t)v;
    }
}

void g_bar3f(Battle *b, int x, int line, int n)   /* rt_673e */
{
    for (int r = 0; r < 2 * n; r++) *V(b, row_addr(line + r) + x) = 0x3F;
}

/* ------------------------------------------------------------------ the shore (ovl5_8d89) */

/* ovl5_8d89 builds the shore for the battle into [6CD5] as run-length lines
 * (count, byte ... 63h), 154 lines: [686B] 1-8 from a template in DS
 * (13B9-161E) whose first line is copied and whose nibbles then move the
 * shore's edge line by line; 9-12 (a port beside) straight shores, which set
 * [686B] to 1-4. The locals are kept in a copy of its stack frame, as the
 * two little arrays at bp-28h and bp-1Eh can run into each other. */
void g_build_shore(Battle *b)
{
    /* bp-28h: a28[10], bp-1Eh: a1e[10] (the remake makes the frame bigger, so a
       runaway index stays inside it; none of the eight templates runs past 9) */
    uint8_t fr[0x200];
    memset(fr, 0, sizeof fr);
    int prev = BMEM_SHORE, dst = BMEM_SHORE, tpl = 0;
    int t = b->shore;
    if (t == 0) return;
    if (t >= 9) {
        for (int i = 0; i < 0x9A; i++) {
            int n1, v1, n2, v2;
            if (t == 9) { n1 = 0x47; v1 = 0x66; n2 = 9; v2 = 0x77; }
            else if (t == 10) { n1 = 0x0B; v1 = 0x77; n2 = 0x45; v2 = 0x66; }
            else if (t == 11) { n1 = 0x50; v1 = i < 0x1A ? 0x77 : 0x66; n2 = -1; v2 = 0; }
            else { n1 = 0x50; v1 = i < 0x82 ? 0x66 : 0x77; n2 = -1; v2 = 0; }
            setmb(b, dst++, n1); setmb(b, dst++, v1);
            if (n2 >= 0) { setmb(b, dst++, n2); setmb(b, dst++, v2); }
            setmb(b, dst++, 0x63);
        }
        b->shore = t - 8;
        return;
    }
    static const int TPL[9] = { 0, 0x13B9, 0x140B, 0x145D, 0x14C0, 0x151F, 0x1574, 0x15C9, 0x161E };
    tpl = TPL[t];
    /* the locals: 2 = index into 1E[], 4 = index into 28[], 6, 8 = the edge byte, A = x, C, E,
       10 = the run, 12 = the nibble, 14 = the line */
    int l2 = 0, l4 = 0, l6 = 0, l8 = 0, lA = 0, lC = 0, lE = 0, l10 = 0, l12 = 0;
    (void)lE;
    uint8_t *a28 = &fr[0], *a1e = &fr[10];
    b->nibble = 0;
    int al;
    do { al = msb(b, tpl++); setmb(b, dst++, al); } while (al != 0x63);
    for (int i = 0; i < 10; i++) a28[i] = 0;
    for (int line = 0; line < 0x9A; line++) {
        l2 = l4 = l8 = lA = lC = 0;
        for (int i = 0; i < 10; i++) { a1e[i] = a28[i]; a28[i] = 0; }
        for (;;) {
            /* 9014 */
            l12 = (b->nibble ? (msb(b, tpl) & 15) : ((msb(b, tpl) >> 4) & 15));   /* ovl6_fb2a */
            b->nibble ^= 1;
            if (!b->nibble) tpl++;
            for (;;) {   /* 903B */
                l10 = msb(b, prev);
                if (l10 == 0x63) l12 = 0xF; else prev++;
                if (lC > l10) { lC -= l10; prev++; continue; }
                break;
            }
            l10 -= lC;
            int jump913f = 0;
            if (lA + l10 + lC + (int8_t)a1e[l2 & 0xFF] >= 0x50) jump913f = 1;
            else if (msb(b, prev + 1) == 0x63) jump913f = 1;
            if (!jump913f) {
                lC = 0;
                if (l12 > 0 && l12 < 7) {
                    int e = 7 - l12;
                    if (e > l10) e = l10;
                    l10 -= e;
                    lC = -e;
                    l6 = 0;
                    l4++;
                    a28[l4 & 0xFF]++;
                } else if (l12 > 6 && l12 < 0xE) {
                    l10 += l12 - 6;
                    lC = l12 - 6;
                    l6 = 1;
                    a28[l4 & 0xFF]++;
                    l4++;
                }
                if (lC != 0) l8 = msb(b, prev) == 0x66 ? 0x67 : 0x76;
            } else {
                /* 913F */
                if (!(l12 == 0xF && !(msb(b, prev + 1) == 0x63 && lA != 0))) {
                    if (l12 != 0xE) {
                        b->nibble ^= 1;
                        if (b->nibble) tpl--;
                        l12 = 0xF;
                    }
                }
            }
            /* 917F */
            if (l10 <= 0) { prev++; continue; }
            if (l10 + lA > 0x50) {
                l10 = 0x50 - lA;
                if (l10 <= 0) {
                    while (msb(b, prev++) != 0x63) ;
                    b->nibble ^= 1;
                    if (!b->nibble) tpl++;
                }
            }
            l10 += (int8_t)a1e[l2++ & 0xFF];
            setmb(b, dst++, l10);
            setmb(b, dst++, msb(b, prev++));
            if (msb(b, prev + 1) == 0x67 || msb(b, prev + 1) == 0x76) prev += 2;
            if (l8 != 0) {
                if (l6 == 0) { l10++; lC++; }
                else setmb(b, dst - 2, mb(b, dst - 2) - 1);
                setmb(b, dst++, 1);
                setmb(b, dst++, l8);
                l8 = 0;
            }
            if (l12 == 0xE) {
                if (b->nibble) { tpl++; b->nibble = 0; }
                if (lA != 0) {
                    for (;;) {
                        al = msb(b, prev++);
                        setmb(b, dst, al);
                        if (msb(b, dst) == 0x63) { dst++; break; }
                        int add = (int8_t)a1e[l2++ & 0xFF];
                        setmb(b, dst, msb(b, dst) + add);
                        dst++;
                    }
                } else dst -= 2;
                prev = dst;
                do { al = msb(b, tpl++); setmb(b, dst++, al); } while (al != 0x63);
                for (int i = 0; i < 10; i++) a28[i] = 0;
                break;
            }
            if (l12 == 0xF) {
                for (;;) {
                    al = msb(b, prev++);
                    setmb(b, dst, al);
                    if (msb(b, dst) == 0x63) { dst++; break; }
                    int add = (int8_t)a1e[l2++ & 0xFF];
                    setmb(b, dst, msb(b, dst) + add);
                    dst++;
                }
                break;
            }
            lA += l10;
        }
    }
}
