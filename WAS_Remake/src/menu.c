/*
 * The Ancient Art of War at Sea (1987) - remake
 * Nov 2026 using Claude Opus 5.5
 * "Guided" by Brett Hallen
 * Original game by Dave Murry & Barry Murry
 * Published by Brøderbund Software Inc.
 *
 * menu.c: step 6, overlay 1 of WAS.EXE: "presented by", the title page, the
 * scrolls (titles, story, rules), the opponents and the quote, the campaign
 * menu, Ye Olde Options Shoppe, the opening, the battle report, saving and
 * loading the game, and the demo.
 *
 * Overlay 1 draws into a 32000-byte buffer (BUF: two fields of 200 line
 * pairs, the scroll is 400 lines tall) and into video memory, and it waits
 * in delay() as it goes. The remake runs the ported routines as they were
 * written, on a thread of their own (fiber.c, as the sea battle does), with
 * the same memories: BUF and the CGA screen as WAS.EXE has them, and beside
 * each a 16-colour copy that every drawing routine writes too (the Tandy
 * pictures from WAST.DAT; letters, icons and the pointer in the nearest of
 * the 16 colours), so Tab shows either at any moment. The clock counts
 * delay() at 50 us a unit and the drawing in 8086 instructions at 300 a
 * millisecond, as an IBM PC/XT runs it.
 *
 * Each function names the WAS.EXE routine it ports.
 */
#include "menu_int.h"
#include "dsmap.h"

#include <SDL3/SDL.h>

static void ed_from_s(Menu *m, const uint8_t *d, size_t size);
static void scroll_store(Menu *m, const char *s);

/* ------------------------------------------------------------------ the clock, random() and the keys */


void maybe_yield(Menu *m)
{
    if (m->clock_us >= m->target_us) fiber_yield(&m->fiber);
}

/* random() (rt:2F0D): adds successive words of WAS.EXE's own code, 0-127 */
static int rnd(Menu *m)
{
    if (++m->rnd_ptr == 0x2328) m->rnd_ptr = 0xBB8;
    unsigned w = m->ga->code[m->rnd_ptr] | (m->ga->code[m->rnd_ptr + 1] << 8);
    m->rnd = (uint16_t)((w + m->rnd) & 0x7F);
    return m->rnd;
}

/* delay() (rt:2F6E): random() units / 2 times, 50 us a unit on an XT */
void delay(Menu *m, int units)
{
    unsigned n = (unsigned)(units & 0xFFFF) >> 1;
    if (n == 0) n = 1;
    m->clock_us += (double)(units & 0xFFFF) * DELAY_UNIT_US;
    while (n--) rnd(m);
    maybe_yield(m);
}

void beep_low(Menu *m) { m->beeps++; delay(m, 0x3E8); }   /* rt:05A9 (the sound is step 8) */

/* a tune (ovl1: play_tune 8B7A, 9AAB, 9B45, the opening): the notes are step 8;
 * WAS.EXE plays them before it goes on, so the time is kept */
static void tune_wait(Menu *m, int units) { m->beeps++; delay(m, units); }

static int so2w(const Menu *m, unsigned off) { return off + 1 < sizeof m->ga->so2 ? (int16_t)(m->ga->so2[off] | (m->ga->so2[off + 1] << 8)) : 0; }
static int so4w(const Menu *m, unsigned off) { return off + 1 < sizeof m->ga->so4 ? (int16_t)(m->ga->so4[off] | (m->ga->so4[off + 1] << 8)) : 0; }
static int dsw(const Menu *m, unsigned off) { return off + 1 < DS_SIZE ? (int16_t)(m->ga->ds[off] | (m->ga->ds[off + 1] << 8)) : 0; }
static const char *SO2(const Menu *m, unsigned off) { return assets_buf_string(m->ga->so2, sizeof m->ga->so2, off); }
static const char *SO3(const Menu *m, unsigned off) { return assets_buf_string(m->ga->so3, sizeof m->ga->so3, off); }
static const char *SO4(const Menu *m, unsigned off) { return assets_buf_string(m->ga->so4, sizeof m->ga->so4, off); }
const char *DS(const Menu *m, unsigned off) { return assets_ds_string(m->ga, off); }

/* menu_poll_key (ovl1:8AA9): the last key pressed, once */
int poll_key(Menu *m)
{
    WORK(m, 5);
    m->a6469 = 100;
    if (m->n804 == 0) { maybe_yield(m); return 0; }
    int k = m->q804[0];
    m->n804--;
    SDL_memmove(m->q804, m->q804 + 1, (size_t)m->n804 * sizeof m->q804[0]);
    return k;
}

/* [0802]: the key held down (a key let go is still seen once) */
int key802(Menu *m)
{
    WORK(m, 10);
    int k = m->k802;
    if (m->up802) { m->k802 = 0; m->up802 = false; }
    maybe_yield(m);                                     /* a loop waiting for one key while another is held goes on */
    return k;
}

void keys_clear(Menu *m) { m->n804 = 0; m->k802 = 0; m->up802 = false; }   /* [0804] = [0802] = 0 */

/* rt_313c and rt_3110: the BIOS's keyboard handler in and out. While it is
 * in, keys go to its buffer as DOS reads them (letters as typed) and
 * [0802] and [0804] are left alone. */
void text_begin(Menu *m) { m->text_on = true; m->ndos = 0; }
void text_end(Menu *m) { m->text_on = false; m->ndos = 0; }

static void dos_push(Menu *m, int c) { if (m->ndos < (int)SDL_arraysize(m->dosq)) m->dosq[m->ndos++] = c; }

/* rt_2e8e: DOS's next character (int 21h 0Bh, 08h): 0 if none; a cursor
 * key is 0 then its scan code, which WAS.EXE reads as 27 (Esc) then the scan code */
int dos_key(Menu *m)
{
    WORK(m, 120);
    if (m->ndos == 0) { maybe_yield(m); return 0; }
    int c = m->dosq[0];
    m->ndos--;
    SDL_memmove(m->dosq, m->dosq + 1, (size_t)m->ndos * sizeof m->dosq[0]);
    return c;
}

bool key_down(int k) { return k == PK_EQUALS || k == PK_KP_PLUS || k == PK_CTRL; }   /* is_scroll_down_key rt_126A */
bool key_up(int k)   { return k == PK_MINUS || k == PK_KP_MINUS || k == PK_TAB; }    /* is_scroll_up_key rt_1217 */

/* ------------------------------------------------------------------ the screen and BUF */

static uint8_t *line16(Menu *m, int line) { return &m->v16[line * SCREEN_W]; }

/* a CGA byte written to the screen (4 pixels) */
void scr_byte(Menu *m, int line, int xb, uint8_t b)
{
    if (line < 0 || line >= SCREEN_H || xb < 0 || xb >= 80) return;
    m->vram[line * 80 + xb] = b;
    uint8_t *d = line16(m, line) + xb * 4;
    for (int k = 0; k < 4; k++) d[k] = m->c16[m->pal0][(b >> (6 - 2 * k)) & 3];
}

/* XOR a byte of the screen: each pixel flips (0 <-> 3, 1 <-> 2; in 16 colours its complement) */
void scr_xor(Menu *m, int line, int xb, uint8_t mask)
{
    if (line < 0 || line >= SCREEN_H || xb < 0 || xb >= 80) return;
    m->vram[line * 80 + xb] ^= mask;
    uint8_t *d = line16(m, line) + xb * 4;
    for (int k = 0; k < 4; k++)
        if ((mask >> (6 - 2 * k)) & 3) d[k] ^= 15;
}

/* AND a byte of the screen (the pointer): where the pointer has colour 3 the screen shows */
static void scr_and(Menu *m, int line, int xb, uint8_t b)
{
    if (line < 0 || line >= SCREEN_H || xb < 0 || xb >= 80) return;
    m->vram[line * 80 + xb] &= b;
    uint8_t *d = line16(m, line) + xb * 4;
    for (int k = 0; k < 4; k++) {
        int v = (b >> (6 - 2 * k)) & 3;
        if (v != 3) d[k] = m->c16[m->pal0][v];
    }
}

/* a byte written into BUF at offset a (BUF-relative); the 16-colour copy gets
 * the same pixels in the nearest colours */
void buf_byte(Menu *m, int a, uint8_t b)
{
    if (a < 0 || a >= BUF_SIZE) return;
    m->buf[a] = b;
    int field = a >= FIELD, rel = a - field * FIELD;
    int line = (rel / 80) * 2 + field, xb = rel % 80;
    if (line >= 400) return;
    uint8_t *d = &m->b16[line * SCREEN_W + xb * 4];
    for (int k = 0; k < 4; k++) d[k] = m->c16[m->pal0][(b >> (6 - 2 * k)) & 3];
}

static const Image *picture_img(const Menu *m, PictureId id, GfxMode mode)
{
    const AssetSet *set = &m->ga->set[mode];
    if (!set->loaded || !set->pictures[id].px) return NULL;
    return &set->pictures[id];
}

/* load_picture (ovl1:CBEA) into BUF at off (0, or 1F40h for the scroll's second half) */
static bool load_picture(Menu *m, PictureId id, int off)
{
    const Image *c = picture_img(m, id, MODE_CGA);
    if (!c || c->w < SCREEN_W || c->h < SCREEN_H) return false;
    for (int y = 0; y < SCREEN_H; y++) {
        int a = off + (y & 1) * FIELD + (y >> 1) * 80;
        for (int xb = 0; xb < 80; xb++) {
            const uint8_t *p = &c->px[y * c->w + xb * 4];
            m->buf[a + xb] = (uint8_t)(((p[0] & 3) << 6) | ((p[1] & 3) << 4) | ((p[2] & 3) << 2) | (p[3] & 3));
        }
    }
    const Image *t = picture_img(m, id, MODE_16);
    bool scroll = id == PIC_SCROLL_1 || id == PIC_SCROLL_2;
    int top = (off / 80) * 2;
    for (int y = 0; y < SCREEN_H && top + y < 400; y++) {
        uint8_t *d = &m->b16[(top + y) * SCREEN_W];
        for (int x = 0; x < SCREEN_W; x++) {
            if (t && x < t->w && y < t->h) d[x] = t->px[y * t->w + x] & 15;
            else d[x] = m->c16[scroll][c->px[y * c->w + x] & 3];
        }
    }
    WORK(m, 16000 * 3);           /* loading and decoding: SPJ shows 0.2 s after the scroll rolls up (DOSBox, 300 cycles) */
    maybe_yield(m);
    return true;
}

/* rt_2715: BUF from [3F39] to the screen, a field each line */
void blit(Menu *m, int lines)
{
    for (int y = 0; y < lines && y < SCREEN_H; y++) {
        int lp = m->view + (y >> 1), field = y & 1;
        int a = lp * 80 + field * FIELD;
        if (a < 0 || a + 80 > BUF_SIZE) continue;
        SDL_memcpy(&m->vram[y * 80], &m->buf[a], 80);
        int l16 = lp * 2 + field;
        if (l16 < 400) SDL_memcpy(line16(m, y), &m->b16[l16 * SCREEN_W], SCREEN_W);
    }
    if (m->sq_on || m->mk_on) ed_overlay(m, SDL_min(lines, SCREEN_H));   /* the ships editor */
    WORK(m, 30 + lines * 50);
    maybe_yield(m);
}

void blit_menu(Menu *m) { blit(m, m->blit_lines); }   /* rt_2715([6459]) */

/* scroll_copy_lines (ovl1:CA44): n line pairs of BUF from offset a to the screen at line y */
static void copy_lines(Menu *m, int a, int y, int n)
{
    for (int k = 0; k < n; k++, a += 80, y += 2)
        for (int f = 0; f < 2; f++) {
            int src = a + f * FIELD, line = y + f;
            if (src < 0 || src + 80 > BUF_SIZE || line < 0 || line >= SCREEN_H) continue;
            SDL_memcpy(&m->vram[line * 80], &m->buf[src], 80);
            int l16 = (a / 80) * 2 + f;
            if (l16 < 400) SDL_memcpy(line16(m, line), &m->b16[l16 * SCREEN_W], SCREEN_W);
        }
    WORK(m, 20 + n * 2 * 50);
}

/* copy_glyph (ovl1:CA83): an SO3 bitmap (width in words, height in line pairs) into BUF */
static void copy_glyph(Menu *m, unsigned off, int dst)
{
    const uint8_t *g = m->ga->so3;
    if (off + 4 > sizeof m->ga->so3) return;
    int w = (g[off] | (g[off + 1] << 8)) * 2, h = g[off + 2] | (g[off + 3] << 8);
    unsigned s = off + 4;
    for (int lp = 0; lp < h; lp++, dst += 80)
        for (int f = 0; f < 2; f++)
            for (int i = 0; i < w; i++, s++)
                buf_byte(m, dst + f * FIELD + i, s < sizeof m->ga->so3 ? g[s] : 0);
    WORK(m, 25 + h * 2 * (w / 2 + 6));
}

/* and_pointer (ovl1:CABE): an SO3 bitmap ANDed onto the screen */
static void and_pointer(Menu *m, unsigned off, int xb, int y)
{
    const uint8_t *g = m->ga->so3;
    if (off + 4 > sizeof m->ga->so3) return;
    int w = (g[off] | (g[off + 1] << 8)) * 2, h = (g[off + 2] | (g[off + 3] << 8)) * 2;
    unsigned s = off + 4;
    for (int l = 0; l < h; l++)
        for (int i = 0; i < w; i++, s++)
            scr_and(m, y + l, xb + i, s < sizeof m->ga->so3 ? g[s] : 0xFF);
    WORK(m, 20 + h * (w + 6));
}

/* draw_icon (rt_2cc4): a bitmap (width in bytes, height in line pairs) onto the screen */
void draw_icon_from(Menu *m, const uint8_t *src, size_t size, unsigned off, int y, int xb)
{
    if (off + 2 > size) return;
    int w = src[off], h = src[off + 1] * 2;
    unsigned s = off + 2;
    for (int l = 0; l < h; l++)
        for (int i = 0; i < w; i++, s++)
            if (s < size) scr_byte(m, y + l, xb + i, src[s]);
    WORK(m, 20 + h * (w + 6));
}
void icon_ds(Menu *m, unsigned off, int y, int xb) { draw_icon_from(m, m->ga->ds, DS_SIZE, off, y, xb); }
static void icon_so3(Menu *m, unsigned off, int y, int xb) { draw_icon_from(m, m->ga->so3, sizeof m->ga->so3, off, y, xb); }

/* xor_box (ovl1:CAF7): a box with rounded corners (x, w in bytes; h in line pairs) */
static void xor_box(Menu *m, int x, int y, int w, int h)
{
    int line = y;
    for (int k = 0; k < 2; k++, line++) {
        scr_xor(m, line, x, k ? 0x3F : 0x0F);
        for (int i = 1; i <= w; i++) scr_xor(m, line, x + i, 0xFF);
        scr_xor(m, line, x + w + 1, k ? 0xFC : 0xF0);
    }
    for (int l = 0; l < 2 * h; l++, line++)
        for (int i = 0; i < w + 2; i++) scr_xor(m, line, x + i, 0xFF);
    for (int k = 0; k < 2; k++, line++) {
        scr_xor(m, line, x, k ? 0x0F : 0x3F);
        for (int i = 1; i <= w; i++) scr_xor(m, line, x + i, 0xFF);
        scr_xor(m, line, x + w + 1, k ? 0xF0 : 0xFC);
    }
    WORK(m, 40 + (2 * h + 4) * (w + 8));
}

/* xor_frame (ovl1:CB92): a frame a byte wide round a picture */
static void xor_frame(Menu *m, int x, int y, int w, int h)
{
    int line = y;
    for (int k = 0; k < 4; k++, line++) for (int i = 0; i < w; i++) scr_xor(m, line, x + i, 0xFF);
    for (int l = 0; l < 2 * h; l++, line++) { scr_xor(m, line, x, 0xFF); scr_xor(m, line, x + w - 1, 0xFF); }
    for (int k = 0; k < 4; k++, line++) for (int i = 0; i < w; i++) scr_xor(m, line, x + i, 0xFF);
    WORK(m, 40 + 8 * (w + 6) + 2 * h * 12);
}

/* draw_hline (rt:2B95): a white line */
void hline(Menu *m, int line, int xb, int len)
{
    for (int i = 0; i < len; i++) scr_byte(m, line, xb + i, 0xFF);
    WORK(m, 20 + len);
}

/* goto_xy, clear_text_row (rt:2C48, which also goes to its start), print_string (rt_32d4) */
void gotoxy(Menu *m, int col, int row) { m->ccol = col; m->crow = row; }

void clear_row(Menu *m, int row)
{
    for (int l = 0; l < 8; l++) for (int xb = 0; xb < 80; xb++) scr_byte(m, row * 8 + l, xb, 0);
    gotoxy(m, 0, row);
    WORK(m, 8 * 45);
}

void clear_all_rows(Menu *m) { for (int r = 24; r >= 0; r--) clear_row(m, r); }   /* ovl1:8519 */

/* the font, a byte (8 pixels) as a CGA word of colour 3 (DS:088C) */
static void char_line(Menu *m, int line, int col, uint8_t bits, bool x)
{
    uint8_t hi = 0, lo = 0;
    for (int k = 0; k < 4; k++) {
        if (bits & (0x80 >> k)) hi |= (uint8_t)(3 << (6 - 2 * k));
        if (bits & (0x08 >> k)) lo |= (uint8_t)(3 << (6 - 2 * k));
    }
    if (x) { scr_xor(m, line, col * 2, hi); scr_xor(m, line, col * 2 + 1, lo); }
    else { scr_byte(m, line, col * 2, hi); scr_byte(m, line, col * 2 + 1, lo); }
}

void put_char(Menu *m, int c)
{
    if (c == '\n') { m->crow++; return; }
    if (c == '\r') { m->ccol = 0; return; }
    if (c == 8) { if (m->ccol > 0) m->ccol--; return; }
    if (c < FONT_FIRST || c >= FONT_FIRST + FONT_COUNT) c = ' ';
    if (m->crow >= 0 && m->crow < 25)
        for (int l = 0; l < 8; l++) char_line(m, m->crow * 8 + l, m->ccol, m->ga->font[c - FONT_FIRST][l], false);
    WORK(m, 110);
    if (++m->ccol >= 40) { m->ccol = 0; m->crow++; }
}

void print(Menu *m, const char *s) { while (*s) put_char(m, (unsigned char)*s++); maybe_yield(m); }
void print_at(Menu *m, int col, int row, const char *s) { gotoxy(m, col, row); print(m, s); }

/* ovl1_cf52 and ovl1_a935: text XORed onto a picture at a text position */
static void xor_text(Menu *m, const char *s, int col, int row)
{
    for (; *s; s++, col++) {
        int c = (unsigned char)*s;
        if (c < FONT_FIRST || c >= FONT_FIRST + FONT_COUNT) c = ' ';
        if (row >= 0 && row < 25 && col >= 0 && col < 40)
            for (int l = 0; l < 8; l++) char_line(m, row * 8 + l, col, m->ga->font[c - FONT_FIRST][l], true);
        WORK(m, 110);
    }
    maybe_yield(m);
}

/* ovl1_a966: a number of up to two digits */
static void xor_number(Menu *m, int v, int col, int row)
{
    char s[2] = { 0, 0 };
    if (v > 9) { s[0] = (char)('0' + (v / 10) % 10); xor_text(m, s, col, row); }
    s[0] = (char)('0' + (v % 10 + 10) % 10);
    xor_text(m, s, col + 1, row);
}

/* text_rows_add (rt:0BC2) with rt_2ae4: the white line and the rows below the picture */
void text_rows_add(Menu *m, int n)
{
    m->rows += n;
    int a6459 = 200 - m->rows * 16 - 4;
    m->blit_lines = 100 + a6459 / 2;
    int line = (a6459 + 200) / 2;
    for (int xb = 0; xb < 80; xb++) { scr_byte(m, line, xb, 0xFF); scr_byte(m, line + 1, xb, 0); }
    for (int l = 0; l < (n > 0 ? n * 8 : 0); l++)
        for (int xb = 0; xb < 80; xb++) scr_byte(m, line + 2 + l, xb, 0);
}

/* set_cga_palette: palette 1 for the pictures, 0 for the scrolls */
void palette(Menu *m, bool pal0) { m->pal0 = pal0; }

void show_one_moment(Menu *m) { clear_row(m, 24); print_at(m, 10, 24, DS(m, 0x6A)); }   /* ovl1:8A2F */

/* ovl1_bec5: [+] or [-] on row 24 */
void plus_or_minus(Menu *m)
{
    clear_row(m, 24);
    icon_ds(m, I_PLUS, 192, 0);
    print_at(m, 3, 24, DS(m, 0x16E));
    icon_ds(m, I_MINUS, 192, 12);
}

/* help_change_select (ovl1:76CD) */
static void help_change_select(Menu *m)
{
    plus_or_minus(m);
    print_at(m, 9, 24, SO3(m, 0x991));      /* to change, */
    icon_ds(m, I_ENTER, 192, 40);
    print_at(m, 23, 24, SO3(m, 0x939));     /* when done. */
}

/* small_caps (ovl1:B5B0): lowercase letters on a picture, from [64D1] */
static void small_caps(Menu *m, const char *s)
{
    for (; *s; s++) {
        int c = (unsigned char)*s;
        copy_glyph(m, F_SPACE, m->a64D1);
        if (c == '/') copy_glyph(m, F_SLASH, m->a64D1);
        else if (c == ':') copy_glyph(m, F_COLON, m->a64D1);
        else if (c != ' ') {
            if (c >= 'A' && c <= 'Z') c += 'a' - 'A';   /* the remake: a disk's name in capitals */
            if (c >= 'a' && c <= 'z') copy_glyph(m, F_LOWER + (unsigned)(c - 'a') * 20, m->a64D1);
            else if (c >= '0' && c <= '9') copy_glyph(m, F_DIGITS + (unsigned)(c - '0') * 24, m->a64D1);
        }
        m->a64D1 += 2;
    }
}

/* ------------------------------------------------------------------ the scroll's text */

static bool is_capital(int c) { return c >= 'A' && c <= 'Z'; }

static void text_cell(Menu *m) { m->a6870 = m->a64D5 * 32 + (m->a64D3 >> 1); }

/* ovl1_b4f3: the writing position at a column and row */
static void at_cell(Menu *m, int col, int row)
{
    m->a64D3 = col << 1;
    m->a64D5 = row;
    m->a64D7 = 0x826 + row * 0x1E0;
    m->a64D1 = m->a64D3 + m->a64D7;
    text_cell(m);
}

static void row_down(Menu *m)                /* scroll_row_down (ovl1:B739) */
{
    if (++m->a64D5 > 25) m->a64D5 = 25;
    else {
        m->a64D7 += 0x1E0;
        m->a64D1 = m->a64D3 + m->a64D7;
        if (m->view * 80 + 0x1630 < m->a64D7 && m->a6485 != 12) m->view += 6;
    }
    text_cell(m);
}

static void row_up(Menu *m)                  /* scroll_row_up (ovl1:B6A9) */
{
    int top = (m->a6485 == 0 || m->a6485 == 10) ? 0x826 : 0xDC6;
    if (--m->a64D5 < 0) m->a64D5 = 0;
    else {
        m->a64D7 -= 0x1E0;
        m->a64D1 = m->a64D3 + m->a64D7;
        if (m->view * 80 + top > m->a64D7 && m->a6485 != 12) m->view -= 6;
    }
    text_cell(m);
}

static uint8_t cell(const Menu *m, int i) { return i >= 0 && i < 0x348 ? m->story[i] : ' '; }

static void advance(Menu *m)                 /* scroll_advance (ovl1:B7AA) */
{
    WORK(m, 40);
    for (int guard = 0; guard < 64; guard++) {
        m->a64D3 += 2;
        if (m->a64D3 > 60) {
            if (m->a64D5 < 25) { m->a64D3 = 0; row_down(m); }
            else m->a64D3 -= 2;
            break;
        }
        m->a64D1 += 2;
        m->a6870++;
        if (cell(m, m->a6870) != 1 || m->a6485 == 2 || m->a6485 == 6) break;
    }
    text_cell(m);
}

/* scroll_char (ovl1:B89D): a letter in the cell at [64D1]; a capital takes two */
static void scroll_char(Menu *m, int c)
{
    int d = m->a64D1;
    bool typing = m->a6485 == 3 || m->a6485 == 0 || m->a6485 == 10;   /* the scroll editor keeps the text's marks */
    copy_glyph(m, F_SPACE, d);
    if (is_capital(cell(m, m->a6870))) {
        copy_glyph(m, F_SPACE, d + 2);
        if (typing && m->a6870 + 1 < 0x348) m->story[m->a6870 + 1] = ' ';
    }
    if (typing && is_capital(c) && m->a6870 + 1 < 0x348) {   /* a capital takes the next cell too: 01h */
        m->story[m->a6870 + 1] = 1;
        if (m->a6870 + 2 < 0x348 && m->story[m->a6870 + 2] == 1) {
            m->story[m->a6870 + 2] = ' ';
            copy_glyph(m, F_SPACE, d + 4);
        }
    }
    WORK(m, 30);
    unsigned off = F_SPACE;
    int dy = 0;
    if (is_capital(c))             off = F_UPPER + (unsigned)(c - 'A') * 44;
    else if (c >= 'a' && c <= 'z') { off = F_LOWER + (unsigned)(c - 'a') * 20; dy = 80; }
    else if (c >= '0' && c <= '9') off = F_DIGITS + (unsigned)(c - '0') * 24;
    else if (c == '|')             off = F_BAR;
    else if (c == '_')             off = F_UNDER;
    else if (c == '?')             { off = F_QUEST; dy = 80; }
    else if (c == '!')             { off = F_EXCL; dy = 80; }
    else if (c == '.')             { off = F_PERIOD; dy = 0x140; }
    else if (c == ',')             { off = F_COMMA; dy = 240; }
    else if (c == '\'')            off = F_COMMA;
    else if (c == '"')             off = F_QUOTES;
    else if (c == ':' || c == ';') { off = F_COLON; dy = 80; }
    else if (c == '(' || c == '[' || c == '{') { off = F_LPAREN; dy = 80; }
    else if (c == ')' || c == ']' || c == '}') { off = F_RPAREN; dy = 80; }
    else if (c == '-')             { off = F_DASH; dy = 80; }
    else if (c == '/')             { off = F_SLASH; dy = 80; }
    copy_glyph(m, off, d + dy);
}

static void scroll_print(Menu *m, const char *s)   /* ovl1:B54A */
{
    for (; *s; s++) {
        scroll_char(m, (unsigned char)*s);
        advance(m);
        if (is_capital((unsigned char)*s)) advance(m);
    }
}

/* ovl1_b64c: the scroll emptied */
static void scroll_clear(Menu *m)
{
    m->view = 0;
    at_cell(m, 0, 0);
    for (int i = 0; i < 0x348; i++) { scroll_char(m, ' '); advance(m); }
    m->view = 0;
}

/* ovl1_bdac: the scroll unrolls, the speaker crackling */
static void unroll(Menu *m)
{
    m->beeps++;
    for (int a = 0x5A0, y = 36; a < 0x1C20; a += 80, y += 2) {
        if (rnd(m) < 95) WORK(m, 25);       /* speaker_click */
        int n = SDL_min((182 - y) >> 1, 14);
        copy_lines(m, a, y, 1);
        copy_lines(m, 0x39D0, y + 2, n);
        maybe_yield(m);
    }
    copy_lines(m, 0, 0, 18);
}

/* ovl1_be42: the scroll rolls up */
static void roll_up(Menu *m)
{
    m->beeps++;
    for (int y = 180; y > 32; y -= 2) {
        if (rnd(m) < 80) WORK(m, 25);
        int n = SDL_min((182 - y) >> 1, 14);
        copy_lines(m, 0x39D0, y + 2, n);
        maybe_yield(m);
    }
    m->view = 0;
    copy_lines(m, 0, 0, 18);
}

/* scroll_palette (ovl1:BC63): palette 0, the top of the scroll and its lower curl */
static void scroll_palette(Menu *m)
{
    palette(m, true);
    copy_lines(m, 0, 0, 18);
    copy_lines(m, 0x39D0, 36, 14);
}

/* ovl1_bca0: the text written on the scroll, then it unrolls */
static void scroll_write(Menu *m)
{
    m->view = 0;
    m->a64DD = 0;
    for (int a = 0x864; a < 0x3924; a += 0x1E0) copy_glyph(m, F_SPACE, a);
    at_cell(m, 0, 0);
    for (int i = 0; i < 0x348; i++) {
        int c = cell(m, m->a6870);
        if (m->a64D3 == 4 && m->a64D5 > 1 && (c != ' ' || cell(m, m->a6870 + 1) != ' ')) m->a64DD++;
        scroll_char(m, c);
        advance(m);
    }
    m->a64DD--;
    if (m->a6485 == 3) { at_cell(m, 9, 0); scroll_store(m, DS(m, 0x167)); }   /* TITLES */
    m->view = 0;
    if (m->a64DD == 0 && (m->a6485 == 5 || m->a6485 == 7)) return;
    if (m->a6485 != 9) unroll(m);
}

/* ------------------------------------------------------------------ data disks (folders) and files
 * The original kept campaigns and saved games on data disks; here each data
 * disk is a folder in Data Disks/ (as in the WAR remake), kept in the
 * original's formats: SITLES, the S files, SS. */

static void root_path(const Menu *m, const char *name, char *out, size_t n) { SDL_snprintf(out, n, "%s%s", m->data_root, name); }

static SDL_EnumerationResult SDLCALL disk_found(void *ud, const char *dir, const char *name)
{
    Menu *m = ud;
    char path[1200];
    SDL_snprintf(path, sizeof path, "%s%s/%s", m->data_root, DISKS_DIR, name);
    SDL_PathInfo info;
    if (name[0] != '.' && SDL_GetPathInfo(path, &info) && info.type == SDL_PATHTYPE_DIRECTORY && m->ndisks < MAX_DISKS)
        SDL_strlcpy(m->disks[m->ndisks++], name, sizeof m->disks[0]);
    (void)dir;
    return SDL_ENUM_CONTINUE;
}

static int disk_cmp(const void *a, const void *b) { return SDL_strcasecmp((const char *)a, (const char *)b); }

static void disk_select(Menu *m, int i)
{
    if (m->ndisks == 0) return;
    m->disk = SDL_clamp(i, 0, m->ndisks - 1);
    SDL_snprintf(m->data_dir, sizeof m->data_dir, "%s%s/%s/", m->data_root, DISKS_DIR, m->disks[m->disk]);
    char path[1200];
    root_path(m, "DRIVE", path, sizeof path);          /* remembered for next time */
    SDL_SaveFile(path, m->disks[m->disk], SDL_strlen(m->disks[m->disk]));
}

static void disks_scan(Menu *m)
{
    char dir[1200], path[1200];
    SDL_snprintf(dir, sizeof dir, "%s%s", m->data_root, DISKS_DIR);
    m->ndisks = 0;
    SDL_EnumerateDirectory(dir, disk_found, m);
    if (m->ndisks == 0) {
        SDL_snprintf(path, sizeof path, "%s/Data Disk 1", dir);
        SDL_CreateDirectory(path);
        SDL_strlcpy(m->disks[m->ndisks++], "Data Disk 1", sizeof m->disks[0]);
    }
    SDL_qsort(m->disks, m->ndisks, sizeof m->disks[0], disk_cmp);
    int pick = 0;
    root_path(m, "DRIVE", path, sizeof path);
    char *name = SDL_LoadFile(path, NULL);
    for (int i = 0; name && i < m->ndisks; i++)
        if (!SDL_strcmp(m->disks[i], name)) pick = i;
    SDL_free(name);
    disk_select(m, pick);
}

/* a blank data disk: the original wrote SITLES (from SDITLES) on a formatted floppy (ovl1_bf06) */
static void disk_new(Menu *m)
{
    char name[64], path[1200];
    for (int n = 1; n < 100; n++) {
        SDL_snprintf(name, sizeof name, "Data Disk %d", n);
        SDL_snprintf(path, sizeof path, "%s%s/%s", m->data_root, DISKS_DIR, name);
        if (!SDL_GetPathInfo(path, NULL)) break;
    }
    if (!SDL_CreateDirectory(path)) return;
    disks_scan(m);
    for (int i = 0; i < m->ndisks; i++)
        if (!SDL_strcmp(m->disks[i], name)) disk_select(m, i);
}

/* the folder of the disk in the drive */
static const char *drive_dir(const Menu *m) { return m->a6497 == 1 ? m->data_dir : m->ga->dir; }

/* SITLES of the disk in the drive: the WAS disk's, or the data disk's (made
 * from SDITLES when the disk has none) */
static bool titles_read(Menu *m, uint8_t out[876])
{
    if (m->a6497 != 1) { SDL_memcpy(out, m->ga->sitles, 876); return true; }
    size_t size = 0;
    uint8_t *d = assets_load_file(m->data_dir, "SITLES", &size);
    if (!d || size < 876) {
        SDL_free(d);
        char path[1200];
        SDL_snprintf(path, sizeof path, "%sSITLES", m->data_dir);
        SDL_SaveFile(path, m->ga->sditles, 876);
        SDL_memcpy(out, m->ga->sditles, 876);
        return true;
    }
    SDL_memcpy(out, d, 876);
    SDL_free(d);
    WORK(m, 4000);
    return true;
}

static bool ss_exists(const Menu *m)
{
    int n = 0;
    char **found = SDL_GlobDirectory(m->data_dir, "SS", SDL_GLOB_CASEINSENSITIVE, &n);
    SDL_free(found);
    return n > 0;
}

/* SCNF: the options; the remake keeps its copy in the data folder (save_options ovl1:A33C) */
static void options_load(Menu *m)
{
    char path[1200];
    root_path(m, "SCNF", path, sizeof path);
    size_t size = 0;
    uint8_t *d = SDL_LoadFile(path, &size);
    if (d && size >= SCNF_SIZE) SDL_memcpy(m->ga->scnf, d, SCNF_SIZE);
    SDL_free(d);
}

static void options_save(Menu *m)
{
    char path[1200];
    root_path(m, "SCNF", path, sizeof path);
    SDL_SaveFile(path, m->ga->scnf, SCNF_SIZE);
}

static int opt(const Menu *m, unsigned ds) { return assets_scnf_word(m->ga, ds); }
static int optb(const Menu *m, unsigned ds) { return m->ga->scnf[ds - 0x684C]; }
static void set_opt(Menu *m, unsigned ds, int v)
{
    m->ga->scnf[ds - 0x684C] = (uint8_t)v;
    if (ds != 0x6867 && ds != 0x6868 && ds + 1 < 0x684C + SCNF_SIZE) m->ga->scnf[ds + 1 - 0x684C] = (uint8_t)(v >> 8);
}

/* load_campaign (ovl1:C388): the campaign chosen from the disk in the drive, and
 * its title from the scroll's text (row 3 + 2k) */
static bool load_campaign(Menu *m, int k)
{
    char name[4] = { (char)m->names[k * 3], (char)m->names[k * 3 + 1], 0, 0 };
    size_t size = 0;
    uint8_t *d = assets_load_file(drive_dir(m), name, &size);
    bool ok = campaign_parse(&m->game, name, d, size);
    if (ok) ed_from_s(m, d, size);
    SDL_free(d);
    if (!ok) return false;
    if (m->a6485 == 1) {                                /* going to war: the title from the scroll */
        char title[33];
        int n = 0;
        for (int c = 2; c < 31; c++) {
            uint8_t ch = cell(m, (3 + 2 * k) * 32 + c);
            if (ch >= 0x20 && ch < 0x7F) title[n++] = (char)ch;
        }
        while (n > 0 && title[n - 1] == ' ') n--;
        title[n] = 0;
        if (n) SDL_strlcpy(m->game.title, title, sizeof m->game.title);
    }
    m->opponent = m->game.opponent;
    SDL_memcpy(m->story, m->game.story_raw, 0x348);
    WORK(m, 6000);
    return true;
}

/* ------------------------------------------------------------------ step 7: the campaign being made or changed */

static int16_t rd16s(const uint8_t *p) { return (int16_t)(p[0] | (p[1] << 8)); }
static void wr16(uint8_t *p, int v) { p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8); }

/* load_campaign (ovl1:C388): an S file's map, squadrons and ships into DS */
static void ed_from_s(Menu *m, const uint8_t *d, size_t size)
{
    Fleet *f = &m->ed;
    if (!d || size < 0x85C) return;
    SDL_memcpy(f->map, d + 0x348, 200);
    const uint8_t *a = d + 0x410;
    for (int i = 0; i < 40; i++) {
        f->side[i] = rd16s(a + 2 * i);
        f->speed[i] = rd16s(a + 80 + 2 * i);
        f->x[i] = rd16s(a + 160 + 2 * i);
        f->y[i] = rd16s(a + 240 + 2 * i);
        f->supplies[i] = (int8_t)a[320 + i];
        f->ship_sq[i] = (int8_t)a[360 + i];
        f->sails[i] = (int8_t)a[400 + i];
        f->type[i] = (int8_t)a[440 + i];
    }
    SDL_memcpy(f->names, d + 0x604, 600);
    if (size >= S_SIZE) SDL_memcpy(f->hull, d + 0x85C, 40);
}

/* write_campaign_file (ovl1:C527): the story, the map, the squadrons and
 * ships, the opponent, the rules, the names, the hulls */
static void ed_to_s(const Menu *m, uint8_t out[S_SIZE])
{
    const Fleet *f = &m->ed;
    SDL_memcpy(out, m->story, 0x348);
    SDL_memcpy(out + 0x348, f->map, 200);
    uint8_t *a = out + 0x410;
    for (int i = 0; i < 40; i++) {
        wr16(a + 2 * i, f->side[i]);
        wr16(a + 80 + 2 * i, f->speed[i]);
        wr16(a + 160 + 2 * i, f->x[i]);
        wr16(a + 240 + 2 * i, f->y[i]);
        a[320 + i] = (uint8_t)f->supplies[i];
        a[360 + i] = (uint8_t)f->ship_sq[i];
        a[400 + i] = (uint8_t)f->sails[i];
        a[440 + i] = (uint8_t)f->type[i];
    }
    wr16(out + 0x5F0, m->opponent);
    for (int r = 0; r < NUM_RULES; r++) wr16(out + 0x5F2 + 2 * r, m->game.rules[r]);
    SDL_memcpy(out + 0x604, f->names, 600);
    SDL_memcpy(out + 0x85C, f->hull, 40);
}

/* blank_armies (ovl1:9370): no squadrons, no ships, no names */
static void blank_armies(Menu *m)
{
    for (int i = 0; i < 40; i++) {
        m->ed.speed[i] = 9999;
        m->ed.side[i] = -1;
        m->ed.type[i] = -1;
        m->ed.ship_sq[i] = -1;
    }
    SDL_memset(m->ed.names, 0, sizeof m->ed.names);
    WORK(m, 40 * 700);
}

static void default_rules(Menu *m)                  /* ovl1:93FA */
{
    uint16_t *r = m->game.rules;                        /* DS:64AD */
    r[0] = 40; r[1] = 10; r[7] = 1; r[3] = 1; r[6] = 1; r[4] = 2; r[5] = 80;
}

/* disk_error (ovl1:AA0A): a file could not be written */
static void disk_error(Menu *m)
{
    clear_row(m, 23);
    print(m, DS(m, 0x121));                             /* Unable to use data disk. */
    clear_row(m, 24);
    print(m, DS(m, 0x13A));                             /* Press any key to continue. */
    keys_clear(m);
    while (!key802(m)) WORK(m, 20);
}

/* write_campaign_file (ovl1:C527): S file k of the data disk */
static bool write_campaign_file(Menu *m, int k)
{
    char name[4] = { (char)m->names[k * 3], (char)m->names[k * 3 + 1], 0, 0 }, path[1200];
    uint8_t s[S_SIZE];
    ed_to_s(m, s);
    SDL_snprintf(path, sizeof path, "%s%s", m->data_dir, name);
    bool ok = SDL_SaveFile(path, s, S_SIZE);
    WORK(m, 8000);
    if (!ok) { disk_error(m); return false; }
    char title[sizeof m->game.title];
    SDL_strlcpy(title, m->game.title, sizeof title);
    campaign_parse(&m->game, name, s, S_SIZE);          /* what is played from it */
    SDL_strlcpy(m->game.title, title, sizeof m->game.title);
    return true;
}

/* write_titles (ovl1:C8D2): the file names and the scroll's text, SITLES */
static bool write_titles(Menu *m)
{
    char path[1200];
    uint8_t t[876];
    SDL_memcpy(t, m->names, 36);
    SDL_memcpy(t + 36, m->story, 0x348);
    SDL_snprintf(path, sizeof path, "%sSITLES", m->data_dir);
    bool ok = SDL_SaveFile(path, t, sizeof t);
    WORK(m, 4000);
    if (!ok) disk_error(m);
    return ok;
}

/* save_or_reload (thunk 18, ovl1:8538): 100 writes the changes, 101 reads
 * the campaign again */
static void save_or_reload(Menu *m, int a)
{
    if (a == 100) write_campaign_file(m, m->a6487);
    else if (a == 101) load_campaign(m, m->a6487);
}

/* ------------------------------------------------------------------ step 7: the scroll editor (the story, a title) */

#define F_QUILL 0x7D0                                   /* the pen the scroll editor writes with */

static bool scroll_key_ok(int k)                    /* ovl1:BBDE: the characters the scroll has no letter for */
{
    return !(k == '\\' || k == '>' || k == '<' || k == '~' || k == '@' || k == '#' || k == '$' || k == '%' ||
             k == '^' || k == '&' || k == '*' || k == '=' || k == '+' || k == '`');
}

static void scroll_back(Menu *m)                    /* ovl1:B81F: a cell back, over a capital's second half */
{
    for (int guard = 0; guard < 64; guard++) {
        m->a6870--;
        m->a64D3 -= 2;
        if (m->a64D3 < 0) {
            if (m->a64D5 > 0) { m->a64D3 = 60; row_up(m); }
            else { m->a6870++; m->a64D3 += 2; }
        } else m->a64D1 -= 2;
        if (cell(m, m->a6870) != 1) break;
    }
    text_cell(m);
}

/* ovl1_b580: a string onto the scroll and into its text */
static void scroll_store(Menu *m, const char *s)
{
    for (; *s; s++) {
        scroll_char(m, (unsigned char)*s);
        if (m->a6870 >= 0 && m->a6870 < 0x348) m->story[m->a6870] = (uint8_t)*s;
        advance(m);
    }
}

static void story_step_words(Menu *m)               /* ovl1:9171 */
{
    clear_row(m, 23);
    print(m, SO3(m, 0x99C));                            /* The fifth step is to write a story. */
    clear_row(m, 24);
    print(m, SO3(m, 0x9E5));                            /* Press */
    print(m, SO3(m, 0xEC6));                            /*  any key. */
    while (!dos_key(m)) {}
    beep_low(m);
    clear_row(m, 23);
}

static void scroll_done_help(Menu *m)               /* ovl1:9138 */
{
    clear_row(m, 24);
    icon_so3(m, I_ESC, 192, 0);
    print_at(m, 4, 24, SO3(m, 0x939));                  /* when done. */
}

/* title_step (ovl1:9200): the pen goes down to the new title's row */
static void title_step(Menu *m)
{
    m->a64DD++;
    clear_row(m, 24);
    at_cell(m, 2, 3);
    for (int k = 0; k < m->a64DD; k++) {
        blit_menu(m);
        and_pointer(m, F_QUILL, m->a64D3 + 5, m->a64D5 * 12 + 58 - m->view * 2);
        row_down(m);
        row_down(m);
    }
    clear_row(m, 23);
    print(m, SO3(m, 0x9C2));                            /* The last step is to write a title. */
    clear_row(m, 24);
    print(m, SO3(m, 0x9E5));                            /* Press */
    icon_ds(m, I_ENTER, 192, 12);
    print_at(m, 9, 24, SO3(m, 0x939));                  /* when done. */
    m->a6487 = m->a64DD;
}

/* scroll_editor (ovl1:B21E): typing on the scroll, the story (Esc when
 * done) or a title on its row (Enter when done) */
static void scroll_editor(Menu *m)
{
    text_begin(m);
    int letters = 0;                                    /* a title's letters: Enter ends it only when there are some */
    if (m->a6485 == 0) story_step_words(m);
    scroll_done_help(m);
    m->view = 0;
    m->k802 = 0;
    at_cell(m, 0, 0);
    dos_key(m);
    bool title = m->a6485 == 3;
    if (title) title_step(m);
    for (;;) {
        blit_menu(m);
        and_pointer(m, F_QUILL, m->a64D3 + 7, m->a64D5 * 12 + 58 - m->view * 2);
        int k;
        do {
            while (!(k = dos_key(m))) {}
        } while (!scroll_key_ok(k));
        if (k == 27) {                                  /* a cursor key, or Esc */
            k = dos_key(m);
            if (title) {
                if (k == PK_LEFT && m->a64D3 > 4) scroll_back(m);
                if (k == PK_RIGHT && m->a64D3 < 58 && (m->a64D3 > 4 || cell(m, m->a6870) != ' ')) advance(m);
                continue;
            }
            if (k == PK_UP) row_up(m);
            else if (k == PK_DOWN) row_down(m);
            else if (k == PK_LEFT) scroll_back(m);
            else if (k == PK_RIGHT) advance(m);
            else if (k == 0) { beep_low(m); break; }
            continue;
        }
        if (k == 13) {
            if (title) { if (letters) break; continue; }
            if (m->a64D5 < 25) { m->a64D3 = 0; row_down(m); }
            continue;
        }
        if (k == 8 && (m->a64D3 > 0 || m->a64D5 > 0)) {
            if (title) {
                if (m->a64D3 >= 58) { scroll_char(m, ' '); m->story[m->a6870] = ' '; }
                if (m->a64D3 <= 4) continue;
            }
            scroll_back(m);
            if (letters && cell(m, m->a6870) > ' ') letters--;
            scroll_char(m, ' ');
            m->story[m->a6870] = ' ';
            continue;
        }
        if (title && m->a64D3 == 4 && k == ' ') continue;
        if (cell(m, m->a6870) == 1) { m->a6870--; m->a64D3 -= 2; m->a64D1 -= 2; }   /* a capital's second half: its first */
        if (k > ' ') letters++;
        else if (k == ' ' && cell(m, m->a6870) > ' ' && letters) letters--;
        scroll_char(m, k);
        if (m->a6870 >= 0 && m->a6870 < 0x348) m->story[m->a6870] = (uint8_t)k;
        if (title && m->a64D3 >= 58) continue;
        advance(m);
    }
    m->view = 0;
    clear_row(m, 23);
    clear_row(m, 24);
    dos_key(m);
    text_end(m);
    keys_clear(m);
}

/* ------------------------------------------------------------------ the pages */

static void scroll_pictures(Menu *m)        /* show_scroll ovl1:7F31: SPE and SPF */
{
    m->view = 0;
    if (!m->a6467) {
        load_picture(m, PIC_SCROLL_1, 0);
        load_picture(m, PIC_SCROLL_2, 0x1F40);
        m->a6467 = true;
    }
    m->rows = 2;
}

/* titles_help (ovl1:AD76) for the titles (mode 1) */
static void titles_help(Menu *m)
{
    int mode = m->a6485;
    clear_row(m, 23);
    if (mode == 5) {
        print(m, SO3(m, 0xB9B));                        /* Changed your mind?  Press */
        icon_so3(m, I_ESC, 184, 52);
    } else if (mode == 7) print(m, SO3(m, 0xBB5));      /* Select the campaign you want to change. */
    else if (mode == 1) {
        icon_so3(m, I_ESC, 184, 52);
        print_at(m, 30, 23, DS(m, 0x155));              /* to back up */
        gotoxy(m, 0, 23);
        print(m, m->disk_list ? "Select a data disk.     " : SO3(m, 0xBDD));   /* Select a campaign title. */
    } else print_at(m, 7, 23, SO3(m, 0xBDD));
    help_change_select(m);
    if (mode == 7) print_at(m, 12, 24, SO3(m, 0xEDE));  /* point, */
    gotoxy(m, 23, 24);
    if (mode == 5) {
        print(m, SO3(m, 0xCB4));                        /* to erase. */
        put_char(m, ' ');
        gotoxy(m, 9, 24);
    }
    print(m, SO3(m, 0xBF6));                            /* to select. */
}

/* the remake: the data disks listed on the scroll, laid out as SITLES */
static void disks_text(Menu *m)
{
    SDL_memset(m->story, ' ', 0x348);
    const char *head = "DATA DISKS";
    for (int c = 7, i = 0; head[i]; i++) {
        m->story[c++] = (uint8_t)head[i];
        if (is_capital((unsigned char)head[i])) m->story[c++] = 1;
    }
    for (int k = 0; k < m->ndisks; k++) {
        int c = 2;
        for (const char *s = m->disks[k]; *s && c < 31; s++) {
            m->story[(3 + 2 * k) * 32 + c++] = (uint8_t)*s;
            if (is_capital((unsigned char)*s) && c < 31) m->story[(3 + 2 * k) * 32 + c++] = 1;
        }
    }
}

/* ovl1_ab0a: choose a title with the pointer */
static int titles_choose(Menu *m)
{
    int mode = m->a6485;
    at_cell(m, 0, 3);
    m->view = 0;
    titles_help(m);
    bool moved = false;
    m->a6487 = 0;
    keys_clear(m);
    if (mode == 5 || mode == 7) {                       /* the first campaign, not ( war disk ) */
        row_down(m);
        row_down(m);
        m->a6487 = 1;
    }
    if (m->disk_list) {                       /* the remake: start at the disk in the drive */
        for (int k = 0; k < m->disk; k++) { row_down(m); row_down(m); }
        m->a6487 = m->disk;
    }
    for (;;) {
        blit_menu(m);
        and_pointer(m, F_POINTER, 3, (m->a64D5 + 4) * 12 + 4 - m->view * 2);
        if (moved) { delay(m, m->a6469); moved = false; }
        int k;
        while (!(k = poll_key(m))) delay(m, 0x1F4);
        if (k == PK_ENTER) {
            beep_low(m);
            if (m->disk_list) return 0;
            if (m->a6487 == 0 && m->a6497 < 2) { m->a6497 = 2; return R_AGAIN; }      /* ( war disk ) */
            if (m->a6487 == 11 && m->a6497 == 2) {                                   /* ( data disk ) */
                if (m->ndisks > 1) { m->disk_list = true; return R_AGAIN; }
                m->a6497 = 1;
                return R_AGAIN;
            }
            if (mode != 5) break;
            for (;;) {                                  /* Erase this title? (Y or N) */
                clear_row(m, 23);
                clear_row(m, 24);
                print(m, SO3(m, 0xB80));
                gotoxy(m, 0, 24);
                while (!(k = poll_key(m))) delay(m, 0x1F4);
                if (k == PK_N || k == PK_Y) break;
            }
            if (k == PK_Y) break;
            beep_low(m);
            titles_help(m);
            continue;
        }
        if (k == PK_ESC && m->disk_list) { m->disk_list = false; return R_AGAIN; }   /* the remake: no disk after all */
        if (k == PK_ESC && mode == 1) return R_BACK;
        if (k == PK_ESC && mode == 7) return R_BACK;    /* the remake: WAS.EXE has no way back here */
        if (k == PK_ESC && mode == 5) { m->a6487 = -1; beep_low(m); break; }
        if (key_up(k)) {
            int first = (mode == 5 || mode == 7) ? 1 : 0;
            if (--m->a6487 < first) m->a6487 = first;
            else { beep_low(m); row_up(m); row_up(m); moved = true; }
        } else if (key_down(k)) {
            int last = m->disk_list ? m->ndisks - 1 : m->a64DD;
            if (++m->a6487 > last) m->a6487 = last;
            else { beep_low(m); row_down(m); row_down(m); moved = true; }
        }
    }
    clear_row(m, 23);
    clear_row(m, 24);
    return 0;
}

/* story_view (ovl1:AA48): - and + scroll the story */
static int story_view(Menu *m)
{
    keys_clear(m);
    for (;;) {
        blit_menu(m);
        int k;
        while (!(k = poll_key(m)) && !(k = key802(m))) delay(m, 0x1F4);
        if (k == PK_ENTER) { beep_low(m); break; }
        if (k == PK_ESC) return R_BACK;
        if (key_up(k)) { if (m->view > 1) m->view -= 2; }
        else if (key_down(k)) { if (m->view < 108) m->view += 2; }
    }
    clear_row(m, 23);
    clear_row(m, 24);
    return 0;
}

/* ovl1_951e .. ovl1_96c5: each rule's words */
static const char *rule_word(Menu *m, int r)
{
    uint16_t *rules = m->game.rules;
    switch (r) {
    case 0: return SO3(m, rules[0] == 20 ? 0xA6D : rules[0] == 40 ? 0xA64 : 0xA76);    /* slowly steadily briskly */
    case 1: return SO3(m, rules[1] == 20 ? 0xAA7 : rules[1] == 10 ? 0xAA0 : 0xA99);    /* never rarely often */
    case 2: return SO3(m, rules[3] == 1 ? 0xAE4 : rules[3] == 2 ? 0xAD9 : 0xACE);      /* mild risky very risky */
    case 3: return SO3(m, rules[6] == 1 ? 0xB58 : rules[6] == 2 ? 0xAD9 : 0xACE);      /* safe risky very risky */
    case 5: return SO3(m, rules[5] == 20 ? 0xB0E : rules[5] == 40 ? 0xB18 : 0xB22);    /* poor good unlimited */
    case 6: return SO3(m, rules[7] == 0 ? 0xAA7 : rules[7] == 1 ? 0xAA0 : 0xA99);      /* never rarely often */
    default: return "";
    }
}

static void rule_print(Menu *m, int r)
{
    if (r != 4) { scroll_print(m, rule_word(m, r)); return; }
    uint16_t *rules = m->game.rules;                    /* ovl1_9622: a green / an average / a crack crew */
    scroll_print(m, SO3(m, rules[4] == 4 ? 0xAEF : rules[4] == 2 ? 0xAF6 : 0xB00));
    scroll_print(m, SO3(m, 0xB08));
    if (rules[4] == 1) scroll_print(m, DS(m, 0xF8));
}

/* ovl1_9428: the rules written on the scroll */
static void rules_write(Menu *m)
{
    static const unsigned HEAD[7] = { 0xA0F, 0xA23, 0xA46, 0xB46, 0xA56, 0xB37, 0xB63 };
    scroll_clear(m);
    at_cell(m, 10, 0);
    scroll_print(m, SO3(m, 0xA09));                     /* RULES */
    for (int r = 0; r < 7; r++) {
        at_cell(m, 2, 3 + 2 * r);
        scroll_print(m, SO3(m, HEAD[r]));
        rule_print(m, r);
    }
    m->view = 0;
    unroll(m);
}

/* ovl1_ae72: the pointer on a rule, Space changes it */
static int rules_choose(Menu *m)
{
    at_cell(m, 0, 3);
    m->view = 0;
    int r = 0;
    bool moved = false;
    keys_clear(m);
    if (m->a6485 == 11) {                               /* a new campaign's fourth step */
        clear_row(m, 23);
        print(m, SO3(m, 0xD96));                        /* The fourth step is to s */
        print(m, SO3(m, 0xED0));                        /* et the rules. */
        clear_row(m, 24);
        print(m, SO3(m, 0x9E5));                        /* Press */
        print(m, SO3(m, 0xEC6));                        /*  any key. */
        while (!poll_key(m)) WORK(m, 20);
        beep_low(m);
    }
    clear_row(m, 23);
    icon_ds(m, I_PLUS, 184, 0);
    print_at(m, 3, 23, DS(m, 0x160));                   /* or */
    icon_ds(m, I_MINUS, 184, 12);
    print_at(m, 9, 23, DS(m, 0x163));                   /* to */
    print(m, SO3(m, m->a6485 == 2 ? 0xC01 : 0xC14));    /* look at the rules. / select the rule to change. */
    clear_row(m, 24);
    icon_so3(m, I_SPACE, 192, 0);
    print_at(m, 7, 24, SO3(m, 0x991));                  /* to change, */
    icon_ds(m, I_ENTER, 192, 37);
    print_at(m, 22, 24, SO3(m, 0x939));                 /* when done. */
    for (;;) {
        blit_menu(m);
        and_pointer(m, F_POINTER, 3, (m->a64D5 + 4) * 12 + 4 - m->view * 2);
        if (moved) { delay(m, m->a6469); moved = false; }
        int k;
        while (!(k = poll_key(m))) delay(m, 0x1F4);
        if (k == PK_ENTER) { beep_low(m); clear_row(m, 23); clear_row(m, 24); return 0; }
        if (k == PK_ESC && m->a6485 == 2) return R_BACK;
        if (key_up(k)) {
            if (--r < 0) r = 0;
            else { beep_low(m); row_up(m); row_up(m); moved = true; }
        } else if (key_down(k)) {
            if (++r > 6) r = 6;
            else { beep_low(m); row_down(m); row_down(m); moved = true; }
        } else if (k == PK_SPACE) {
            beep_low(m);
            uint16_t *v = m->game.rules;
            int row = m->a64D5;
            at_cell(m, m->ga->so3[0x8F8 + 2 * r], row);
            switch (r) {
            case 0: v[0] = (uint16_t)(v[0] << 1); if (v[0] > 80) v[0] = 20; break;
            case 1: v[1] = (uint16_t)((int16_t)v[1] / 2); if ((int16_t)v[1] < 5) v[1] = 20; break;
            case 2: if (++v[3] > 2) v[3] = 0; break;
            case 3: if (++v[6] > 2) v[6] = 0; break;
            case 4: if ((int16_t)v[4] > 4 || (v[4] = (uint16_t)((int16_t)v[4] >> 1)) == 0) v[4] = 4; break;
            case 5: if (v[5] == 0 || (v[5] = (uint16_t)(v[5] << 1)) > 80) v[5] = 20; break;
            default: if (++v[7] > 2) v[7] = 0; break;
            }
            rule_print(m, r);
            at_cell(m, 0, row);
            delay(m, m->a6469);
        }
    }
}

/* ovl1_c617 for the titles: the disk's SITLES into the scroll's text; on a
 * data disk with a saved game, "Saved game / New campaigns" first */
static int titles_load(Menu *m)
{
    int mode = m->a6485;
    if (mode == 3 || mode == 5 || mode == 7 || mode == 9) m->a6497 = 1;   /* data_disk_prompt: the data disk (a folder) is in */
    if (mode == 1 && m->a6497 == 1 && !m->disk_list && ss_exists(m)) {
        clear_row(m, 23);
        clear_row(m, 24);
        print(m, SO3(m, 0xE23));                        /* Saved game             New campaigns */
        m->beeps++;                                     /* rt_0c55 */
        keys_clear(m);
        for (;;) {
            int k = key802(m);
            if (k == PK_S) { beep_low(m); return 2; }
            if (k == PK_N) { beep_low(m); break; }
            if (!k) delay(m, 100);
        }
    }
    m->a6487 = 0;
    if (mode != 3) blank_armies(m);
    SDL_memset(m->story, ' ', 0x348);
    scroll_clear(m);
    show_one_moment(m);
    if (m->disk_list) { disks_text(m); return 1; }
    uint8_t t[876];
    if (!titles_read(m, t)) return 0;
    SDL_memcpy(m->names, t, 36);
    SDL_memcpy(m->story, t + 36, 0x348);
    return 1;
}

/* load_saved_game (ovl1:7D39): SS from the data disk */
static bool load_saved_game(Menu *m)
{
    size_t size = 0;
    uint8_t *d = assets_load_file(m->data_dir, "SS", &size);
    bool ok = d && size >= SS_SIZE;
    if (ok) SDL_memcpy(m->ss, d, SS_SIZE);
    SDL_free(d);
    m->have_ss = ok;
    WORK(m, 12000);
    return ok;
}

/* show_scroll (ovl1:7F1A) for the titles (1), the story (4) and the rules (2) */
static int show_scroll(Menu *m)
{
    keys_clear(m);
    for (;;) {
        scroll_pictures(m);
        int mode = m->a6485;
        if (mode == 0 || mode == 10) {                  /* the story to write */
            if (mode == 10) clear_all_rows(m);
            scroll_palette(m);
            scroll_write(m);
            text_rows_add(m, 0);
            scroll_editor(m);
            if (mode == 10) {
                clear_row(m, 23);
                for (;;) {
                    clear_row(m, 24);
                    print(m, SO3(m, 0xDB3));            /* Save the changes? (Y or N) */
                    int k;
                    while (!(k = key802(m))) WORK(m, 20);
                    if (k == PK_Y) { print(m, SO3(m, 0xE5A)); save_or_reload(m, 100); break; }
                    if (k == PK_N) { print(m, DS(m, 0xD9)); save_or_reload(m, 101); break; }
                }
            }
        } else if (mode == 1 || mode == 5 || mode == 7) {   /* the titles */
            int l2 = titles_load(m);
            if (l2 == 0) return R_FAIL;
            if (l2 == 2) return load_saved_game(m) ? 100 : 0x270E;
            for (int row = 24; row > 7; row--) clear_row(m, row);
            scroll_palette(m);
            scroll_write(m);
            if (m->a64DD == 0 && mode != 1) {           /* the remake: nothing to change or remove */
                clear_row(m, 24);
                print(m, "No campaigns on this data disk.");
                delay(m, 40000);
                return 6;
            }
            text_rows_add(m, 0);
            int r = titles_choose(m);
            if (r == R_BACK) return R_BACK;
            if (r == R_AGAIN) continue;
            if (m->disk_list) {                          /* the remake: that disk goes in the drive */
                disk_select(m, m->a6487);
                m->disk_list = false;
                m->a6497 = 1;
                continue;
            }
            if (mode != 5) {
                scroll_clear(m);
                if (!load_campaign(m, m->a6487)) return R_FAIL;
            }
        } else if (mode == 4) {                         /* the story to read */
            scroll_palette(m);
            scroll_write(m);
            text_rows_add(m, 0);
            clear_row(m, 23);
            print(m, SO3(m, 0x90A));                    /* This is the story of this campaign. */
            plus_or_minus(m);
            print_at(m, 9, 24, SO3(m, 0x92E));          /* to scroll, */
            icon_ds(m, I_ENTER, 192, 39);
            print_at(m, 23, 24, SO3(m, 0x939));         /* when done. */
            if (story_view(m) == R_BACK) return R_BACK;
            scroll_clear(m);
        } else if (mode == 2 || mode == 6 || mode == 11) {   /* the rules */
            for (int row = 24; row > 7; row--) clear_row(m, row);
            scroll_palette(m);
            rules_write(m);
            text_rows_add(m, 0);
            if (rules_choose(m) == R_BACK) return R_BACK;
            if (mode == 6) save_or_reload(m, 100);
        } else if (mode == 3) {                         /* the new campaign's title */
            titles_load(m);
            scroll_palette(m);
            scroll_write(m);
            text_rows_add(m, 0);
            scroll_editor(m);
            if (!write_titles(m)) return 0x270E;
        }
        clear_row(m, 22);
        if (mode != 5 || m->a6487 == -1) roll_up(m);
        if (mode == 7) clear_all_rows(m);
        clear_row(m, 23);
        clear_row(m, 24);
        keys_clear(m);
        return 0;
    }
}

/* choose_opponent (ovl1:8BF7) when going to war: SPJ, a frame blinks round the opponent */
static void opp_frame(Menu *m, int o)
{
    int pos = so2w(m, 0x247 + 2 * o), y = (pos / 80) * 2, x = pos % 80;
    delay(m, 0x3E8);
    xor_frame(m, x, y, so2w(m, 0x257 + 2 * o), so2w(m, 0x267 + 2 * o));
    delay(m, 0x320);
    xor_frame(m, x, y, so2w(m, 0x257 + 2 * o), so2w(m, 0x267 + 2 * o));
}

static int choose_opponent(Menu *m)
{
    static const unsigned DESC[6] = { 0, 66, 143, 215, 0x117, 0x155 };
    int mode = m->a6485;
    m->a6467 = false;
    m->view = 0;
    if (!load_picture(m, PIC_OPPONENTS, 0)) return R_FAIL;
    m->rows = 1;
    blit(m, 200);
    palette(m, false);
    clear_row(m, 0);
    int o = SDL_clamp(m->opponent, 0, 5), k = 0;
    if (mode == 9) {                                    /* a new campaign's third step */
        print(m, SO2(m, 0x3A7));                        /* The third step is to select the enemy */
        clear_row(m, 1);
        print(m, SO2(m, 0x3CD));                        /* leader. */
        put_char(m, ' ');
        print(m, SO3(m, 0x9E5));                        /* Press */
        print(m, SO2(m, 0x3D5));                        /*  any key. */
        draw_icon_from(m, m->ga->so2, sizeof m->ga->so2, 0x14D3, 8, 46);   /* Esc */
        print_at(m, 27, 1, SO2(m, 0x10EE));             /* to quit. */
        clear_row(m, 24);
        keys_clear(m);
        while (!(k = poll_key(m))) WORK(m, 20);
        if (k == PK_ESC) { beep_low(m); show_one_moment(m); return 5; }
        o = 0;
        goto select;
    }
    if (mode == 10) goto select;                        /* a campaign changed: the opponent at once */
    print(m, SO2(m, DESC[o]));
    clear_row(m, 24);
    print(m, SO2(m, 0x3DF));                            /*   to change the opponent,   to continue */
    icon_ds(m, I_PLUS, 192, 0);
    icon_ds(m, I_ENTER, 192, 51);
    keys_clear(m);
    for (;;) {                                          /* ovl1:8E09 */
        k = poll_key(m);
        if (!k) { opp_frame(m, o); continue; }
        if (key_down(k)) { if (++o > 5) o = 0; break; }
        if (key_up(k)) { if (--o < 0) o = 5; break; }
        if (k == PK_ENTER) goto chosen;
        if (k == PK_ESC && mode == 2) return R_BACK;
    }
select:
    beep_low(m);                                        /* ovl1:8F2D */
    help_change_select(m);
    print_at(m, 23, 24, SO3(m, 0xBF6));                 /* to select. */
    k = PK_EQUALS;
    for (;;) {
        if (key_down(k) || key_up(k)) {
            beep_low(m);
            clear_row(m, 1);
            clear_row(m, 0);
            for (int x = 0; x < 58; x += 10) {          /* a box sweeps across the top */
                delay(m, 100);
                xor_box(m, x, 0, 18, 6);
                delay(m, 200);
                xor_box(m, x, 0, 18, 6);
            }
            print(m, SO2(m, DESC[o]));
        }
        k = poll_key(m);
        if (key_down(k)) { if (++o > 5) o = 0; }
        else if (key_up(k)) { if (--o < 0) o = 5; }
        else if (k == PK_ENTER) break;
        if (k == PK_ESC && mode == 2) return R_BACK;
        opp_frame(m, o);
    }
chosen:
    beep_low(m);
    m->opponent = o;
    if (mode == 10) save_or_reload(m, 100);
    return 0;
}

/* show_title_ship (ovl1:86D6): SPI, go to war / make changes / quit; left
 * alone for 5000 looks at the keyboard it plays a demo */
static int title_ship(Menu *m)
{
    for (;;) {
        int item = 0, idle = 0;
        m->a6467 = false;
        m->view = 0;
        m->a6497 = 2;
        m->a6471 = 0;
        m->demo = false;
        keys_clear(m);
        if (!load_picture(m, PIC_TITLE_SHIP, 0)) return R_FAIL;
        m->rows = 1;
        m->a6469 = 1;
        palette(m, false);
        text_rows_add(m, 0);
        blit_menu(m);
        help_change_select(m);
        print_at(m, 23, 24, SO3(m, 0xBF6));             /* to select. */
        int r = -1;
        for (;;) {
            static const unsigned ITEM[3] = { 0x951, 0x95E, 0x96B };   /* go to war, make changes, quit */
            m->a64D1 = 0x24E;
            small_caps(m, SO3(m, ITEM[item]));
            blit_menu(m);
            delay(m, D19 + 0x1F4 + m->a6469);
            int k = 0;
            m->on_title = true;
            while (!(k = poll_key(m))) {
                delay(m, D19 / 10);
                if (++idle > 5000) break;
            }
            m->on_title = false;
            if (!k) {                                   /* ovl1:87FF: the demo */
                m->demo = true;
                m->a6485 = 2;
                m->a64D1 = 0x24E;
                small_caps(m, SO3(m, 0x96B));
                m->a64D1 = 0x256;
                small_caps(m, SO3(m, 0xE54));          /* demo */
                blit_menu(m);
                clear_row(m, 24);
                SDL_memset(m->story, ' ', 0x348);       /* ovl1_c617 */
                scroll_clear(m);
                show_one_moment(m);
                int n = 3 + (rnd(m) & 7);
                char name[3] = { 'S', (char)(n < 10 ? '0' + n : 'A' + n - 10), 0 };
                int idx = -1;
                for (int c = 0; c < NUM_CAMPAIGNS; c++)
                    if (m->camps[c].loaded && !SDL_strcasecmp(m->camps[c].file, name)) idx = c;
                if (idx < 0) return 101;
                m->game = m->camps[idx];
                m->game.opponent = 2;                   /* SO2+2DBh to DS:604D; 604F 3, 6057 128 (play_start_demo) */
                m->game.rules[5] = 80;                  /* [64B7]: every squadron is seen */
                WORK(m, 6000);
                return 0;
            }
            idle = 0;
            if (key_down(k)) { beep_low(m); if (++item > 2) item = 0; continue; }
            if (key_up(k)) { beep_low(m); if (--item < 0) item = 2; continue; }
            if (k != PK_ENTER) continue;
            beep_low(m);
            if (item == 1) { show_one_moment(m); return 2; }
            if (item == 2) { clear_all_rows(m); return 1; }
            show_one_moment(m);
            break;
        }
        /* go to war: the titles, the story, the rules, the opponent */
        int step = 1;
        while (r < 0) {
            if (step == 1) {
                m->a6485 = 1;
                int s = show_scroll(m);
                if (s == R_BACK || s == R_FAIL) { m->disk_list = false; m->a6497 = 2; clear_all_rows(m); break; }
                if (s == 100) return 100;
                if (s != 0) return s;
                step = 4;
            } else if (step == 4) {
                m->a6485 = 4;
                int s = show_scroll(m);
                if (s == R_BACK) { step = 1; continue; }
                if (s != 0) return s;
                step = 2;
            } else if (step == 2) {
                if (opt(m, 0x6865) == 0) m->game.rules[7] = 0;   /* no wind: it never changes */
                m->a6485 = 2;
                int s = show_scroll(m);
                if (s == R_BACK) { clear_all_rows(m); step = 4; continue; }
                if (s != 0) return s;
                step = 3;
            } else {
                int s = choose_opponent(m);
                if (s == R_BACK) { step = 2; continue; }
                if (s != 0) return s;
                clear_row(m, 24);                       /* ovl1:89D3: PREPARE FOR WAR! and a quote */
                print_at(m, 11, 24, SO3(m, 0x978));
                int q;
                do q = rnd(m); while (q > 34);
                clear_row(m, 12);
                clear_row(m, 11);
                print(m, SO2(m, (unsigned)so2w(m, 0x4CD + 2 * q)));
                clear_row(m, 10);
                clear_row(m, 13);
                hline(m, 80, 0, 80);
                hline(m, 112, 0, 80);
                m->game.opponent = m->opponent;
                return 0;
            }
        }
    }
}

/* the campaign menu (ovl1_82af) and the change menu (editor_menu ovl1:843C):
 * SPG or SPH with SO4's words, campaign_menu_choose (ovl1_a5f5): the frame
 * round a scene blinks. tab: SO4's names, then the frames' places, widths
 * and heights */
typedef struct { PictureId pic; unsigned names, pos, w, h; } Scenes;

static const Scenes CAMPAIGN_MENU = { PIC_SCENES_1, 0x1E3, 0x19B, 0x1A7, 0x1B3 };
static const Scenes CHANGE_MENU = { PIC_SCENES_2, 0x1EF, 0x1BF, 0x1CB, 0x1D7 };

static void scene_frame(Menu *m, const Scenes *t, int i)
{
    int pos = so4w(m, t->pos + 2 * i), y = (pos / 80) * 2, x = pos % 80;
    delay(m, 0x3E8);
    xor_frame(m, x, y, so4w(m, t->w + 2 * i), so4w(m, t->h + 2 * i));
    delay(m, 0x320);
    xor_frame(m, x, y, so4w(m, t->w + 2 * i), so4w(m, t->h + 2 * i));
}

static int scenes_menu(Menu *m, const Scenes *t, int start)
{
    m->a6467 = false;
    m->view = 0;
    if (!load_picture(m, t->pic, 0)) return R_FAIL;
    m->rows = 1;
    blit(m, 200);
    palette(m, false);
    int i = SDL_clamp(start, 0, 5), k = PK_EQUALS;      /* ovl1_a5f5 */
    plus_or_minus(m);
    print_at(m, 9, 24, SO3(m, 0xD86));                  /* to move marker, */
    icon_ds(m, I_ENTER, 192, 50);
    print_at(m, 28, 24, SO3(m, 0xBF6));                 /* to select. */
    keys_clear(m);
    for (;;) {
        if (key_down(k) || key_up(k)) {
            beep_low(m);
            clear_row(m, 1);
            clear_row(m, 0);
            for (int x = 0; x < 58; x += 10) {
                delay(m, 200);
                xor_box(m, x, 0, 18, 6);
                delay(m, 0x190);
                xor_box(m, x, 0, 18, 6);
            }
            print_at(m, 15, 0, SO4(m, (unsigned)so4w(m, t->names + 2 * i)));
        }
        k = poll_key(m);
        if (key_down(k)) { if (++i > 5) i = 0; delay(m, 0x3E8); }
        else if (key_up(k)) { if (--i < 0) i = 5; delay(m, 0x3E8); }
        else if (k == PK_ENTER) break;
        scene_frame(m, t, i);
    }
    show_one_moment(m);
    return i;
}

static int campaign_menu(Menu *m) { return scenes_menu(m, &CAMPAIGN_MENU, m->a6471); }

/* options_shoppe (ovl1:9BCB): SPK, nine lines in small capitals */
static void shoppe_line(Menu *m, int k)
{
    static const int AT[9] = { 172, 0x28C, 0x46C, 0x64C, 0x82C, 0xA0C, 0xBEC, 0xDCC, 0xFAC };
    int keep = m->a64D1;
    m->a64D1 = AT[k];
    switch (k) {
    case 0:                                             /* ovl1_9f34: fight delay is short / medium / long */
        small_caps(m, SO2(m, 0x13DE));
        small_caps(m, SO3(m, opt(m, 0x6851) == 250 ? 0xA7F : opt(m, 0x6851) == 500 ? 0xAAE : 0xAB5));
        break;
    case 1:                                             /* ovl1_9f9b: wind is (not) realistic */
        small_caps(m, SO2(m, 0x13D3));
        if (opt(m, 0x6865) == 0) small_caps(m, SO2(m, 0xF69));
        small_caps(m, SO2(m, 0xF6E));
        if (opt(m, 0x6865) != 0) small_caps(m, SO2(m, 0xEB7));
        break;
    case 2:                                             /* ovl1_9ffe: realistic / medium / short reload time */
        small_caps(m, optb(m, 0x6867) == 0 ? SO2(m, 0xF6E) : optb(m, 0x6867) == 3 ? SO3(m, 0xAAE) : SO3(m, 0xB2C));
        small_caps(m, SO2(m, 0xF78));
        if (optb(m, 0x6867) == 10) small_caps(m, SO2(m, 0xEB7));
        break;
    case 3:                                             /* ovl1_a07e: manual / automatic cannon aim */
        small_caps(m, SO3(m, optb(m, 0x6868) == 0 ? 0xA92 : 0xAC3));
        small_caps(m, SO3(m, 0xA86));
        if (optb(m, 0x6868) == 0) small_caps(m, SO2(m, 0xEB8));
        break;
    case 4:                                             /* ovl1_a0e5: display is black/white / tv/composite / rgb */
        small_caps(m, SO2(m, 0x1375));
        small_caps(m, SO2(m, opt(m, 0x684D) == 0 ? 0x1381 : opt(m, 0x684D) == 1 ? 0x138E : 0x139B));
        break;
    case 5:                                             /* ovl1_a168: the remake: which data disk is in the drive */
        if (m->shoppe_disk >= m->ndisks) small_caps(m, "a new data disk           ");
        else {
            char s[40];
            SDL_snprintf(s, sizeof s, "disk %-21.21s", m->disks[m->shoppe_disk]);
            small_caps(m, s);
        }
        break;
    case 6:                                             /* ovl1_a1b8: all sounds are off / do not play music / all sounds are on */
        if (opt(m, 0x684F) == 0) { small_caps(m, SO2(m, 0x13A8)); small_caps(m, SO2(m, 0x13B8)); }
        else if (opt(m, 0x684F) == 1) small_caps(m, SO2(m, 0x13C0));
        else { small_caps(m, SO2(m, 0x13A8)); small_caps(m, SO2(m, 0x13BC)); }
        break;
    case 7: small_caps(m, SO2(m, 0x1363)); break;      /* center the screen */
    default:                                            /* ovl1_a22b: (do not) use joystick */
        if (opt(m, 0x6855) == 0) small_caps(m, SO3(m, 0xE5E));
        small_caps(m, SO2(m, 0x13EE));
        if (opt(m, 0x6855) != 0) small_caps(m, SO2(m, 0xEB4));
        break;
    }
    m->a64D1 = keep;
}

static void shoppe_help(Menu *m)                    /* ovl1_a282 */
{
    clear_row(m, 23);
    icon_ds(m, I_PLUS, 184, 0);
    print_at(m, 3, 23, DS(m, 0x116));                   /* or */
    icon_ds(m, I_MINUS, 184, 12);
    print_at(m, 9, 23, SO2(m, 0x407));                  /* to select an option to change. */
    clear_row(m, 24);
    icon_so3(m, I_SPACE, 192, 0);
    print_at(m, 7, 24, SO2(m, 0x426));                  /* to change, */
    icon_ds(m, I_ENTER, 192, 37);
    print_at(m, 22, 24, SO2(m, 0x431));                 /* when done. */
}

static void options_shoppe(Menu *m)
{
    m->a6485 = 12;
    m->a6467 = false;
    m->view = 0;
    if (!load_picture(m, PIC_OPTIONS_SHOPPE, 0)) return;
    palette(m, false);
    m->shoppe_disk = m->disk;
    for (int k = 0; k < 9; k++) shoppe_line(m, k);
    m->rows = 2;
    m->a6469 = 2;
    text_rows_add(m, 0);
    shoppe_help(m);
    m->a64D5 = 3;
    m->a64D3 = 0;
    int item = 0;
    bool changed = false;
    keys_clear(m);
    for (;;) {
        blit_menu(m);
        and_pointer(m, F_POINTER, 3, m->a64D5 * 12 - 32 - m->view * 2);
        delay(m, m->a6469);
        int k;
        while (!(k = poll_key(m))) delay(m, 0x1F4);
        if (k == PK_ENTER) { beep_low(m); break; }
        if (key_up(k)) {
            do {
                if (--item < 0) item = 0;
                else { beep_low(m); m->a64D5--; }
            } while (item == 7);                        /* "center the screen" (the remake has no CRTC to move) */
        } else if (key_down(k)) {
            do {
                if (++item > 8) item = 8;
                else { beep_low(m); m->a64D5++; }
            } while (item == 7);
        } else if (k == PK_SPACE) {
            beep_low(m);
            switch (item) {
            case 0: { int v = opt(m, 0x6851) << 1; set_opt(m, 0x6851, v > 1000 ? 250 : v); break; }
            case 1: set_opt(m, 0x6865, opt(m, 0x6865) ? 0 : 1); break;
            case 2: set_opt(m, 0x6867, optb(m, 0x6867) == 10 ? 3 : optb(m, 0x6867) == 3 ? 0 : 10); break;
            case 3: set_opt(m, 0x6868, optb(m, 0x6868) ? 0 : 1); break;
            case 4: set_opt(m, 0x684D, opt(m, 0x684D) >= 2 ? 0 : opt(m, 0x684D) + 1); break;
            case 5: m->shoppe_disk = (m->shoppe_disk + 1) % (m->ndisks + 1); break;   /* the next disk, or a new one */
            case 6: set_opt(m, 0x684F, opt(m, 0x684F) >= 2 ? 0 : opt(m, 0x684F) + 1); break;
            default: set_opt(m, 0x6855, opt(m, 0x6855) ? 0 : 1); break;   /* stored (the joystick is not read) */
            }
            shoppe_line(m, item);
            changed = true;
        }
    }
    clear_row(m, 23);
    show_one_moment(m);
    if (changed) options_save(m);
    if (m->shoppe_disk >= m->ndisks) disk_new(m);
    else if (m->shoppe_disk != m->disk) disk_select(m, m->shoppe_disk);
}

/* opening (ovl1:A7DA): AD, a tune, Sun Tzu's lines */
static void opening(Menu *m)
{
    m->a6467 = false;
    m->view = 0;
    if (!load_picture(m, PIC_FACE, 0)) return;
    blit(m, 200);
    bool music = opt(m, 0x684F) == 2;
    unsigned text = 0x13FB;
    int row = 3;
    if (music) m->beeps++;
    for (int i = 0; i < 64; i++) {
        int note = so2w(m, 0x447 + 2 * i), d = so2w(m, 0x48B + 2 * i);
        if (note == 0) break;
        if (note == 0xBDC) {
            xor_text(m, SO2(m, text), 7, row);
            text += 27;
            if (++row == 7) row = 13;
            if (music) delay(m, (d - 145) * 3);
        } else if (music) delay(m, d * 4);
    }
    xor_text(m, SO2(m, text), 7, row);
    keys_clear(m);
    for (int i = 0; i < 0xE10 && !key802(m) && !m->n804; i++) delay(m, 100);
    keys_clear(m);
}

/* ------------------------------------------------------------------ step 7: making, changing and removing campaigns */

/* change_campaign (ovl1:92C9): a new campaign takes the data disk's next title */
static int change_campaign(Menu *m)
{
    blank_armies(m);
    default_rules(m);
    m->opponent = 0;
    m->a6485 = 9;
    if (!titles_load(m)) return 5;
    scroll_write(m);
    if (++m->a64DD > 11) {
        m->beeps++;                                     /* rt_30f9 */
        delay(m, 0xBB8);
        clear_row(m, 24);
        print(m, SO3(m, 0x9EB));                        /* Can't add any more campaigns! */
        delay(m, 30000);
        return 5;
    }
    m->a6487 = m->a64DD;
    return 0;
}

/* new_campaign (ovl1:8395): the opponent, the rules, the story, then the
 * file is written and the title typed */
static int new_campaign(Menu *m)
{
    m->a6485 = 9;
    int r = choose_opponent(m);
    if (r) return r;
    SDL_memset(m->story, ' ', 0x348);
    m->a6485 = 11;
    if ((r = show_scroll(m)) != 0) return r;
    m->a6485 = 0;
    if ((r = show_scroll(m)) != 0) return r;
    if (!write_campaign_file(m, m->a6487)) return 0;
    m->a6485 = 3;
    if (show_scroll(m) == 0) {
        print_at(m, 6, 12, SO3(m, 0xC9A));              /* THIS CAMPAIGN IS COMPLETE */
        m->complete = true;
        delay(m, 50000);                                /* the remake: 2 1/2 s before the title page */
    }
    return 0;
}

/* rt:037F: change a campaign: choose it, then the change menu until EXIT */
static int change_menu(Menu *m)
{
    int keep = m->a6497;
    m->a6485 = 7;
    int r = show_scroll(m);
    if (r) { m->a6497 = keep; return r; }
    m->a6473 = 0;
    for (;;) {
        m->a6473 = scenes_menu(m, &CHANGE_MENU, m->a6473);
        switch (m->a6473) {
        case R_FAIL: return R_FAIL;
        case 0:                                         /* RULEBOOK */
            m->a6485 = 6;
            if ((r = show_scroll(m)) != 0) return r;
            break;
        case 1:                                         /* STORYBOOK */
            m->a6485 = 10;
            if ((r = show_scroll(m)) != 0) return r;
            break;
        case 2:                                         /* MAP MAKER */
            m->a6485 = 0;
            r = map_maker(m);
            if (r > 9000) return r;
            save_or_reload(m, r);
            break;
        case 3:                                         /* EXIT */
            m->a6497 = keep;
            return 0;
        case 4:                                         /* ENEMY */
            m->a6485 = 10;
            if (choose_opponent(m)) return R_FAIL;
            break;
        default:                                        /* FLEET */
            m->a6485 = 0;
            r = ships_editor(m);
            if (r > 9000) return r;
            save_or_reload(m, r);
            break;
        }
    }
}

/* scroll_erase_line (ovl1:8693): the title rubbed out a letter at a time */
static void scroll_erase_line(Menu *m)
{
    for (int i = 0; i < 33; i++) {
        scroll_char(m, ' ');
        m->a64D1 += 2;
        blit_menu(m);
        WORK(m, 25);                                    /* speaker_click */
        delay(m, 0x1F4);
    }
    m->beeps++;
}

/* remove_campaign (ovl1:8579): its title goes, the titles below move up and
 * its file name goes to the end of the list (the S file stays) */
static int remove_campaign(Menu *m)
{
    int keep = m->a6497;
    m->a6485 = 5;
    int r = show_scroll(m);
    if (r) { m->a6497 = keep; return r; }
    if (m->a64DD >= 1 && m->a6487 >= 1) {
        int k = m->a6487;
        if (k < 11) {
            uint8_t t[3];
            SDL_memcpy(t, m->names + k * 3, 3);
            SDL_memmove(m->names + k * 3, m->names + k * 3 + 3, (size_t)(11 - k) * 3);
            SDL_memcpy(m->names + 33, t, 3);
        }
        int at = m->a6870, n = 0x2FF - at;
        if (at >= 0 && n > 0 && n < 0x3E7) SDL_memmove(m->story + at, m->story + at + 64, (size_t)n);
        int last = m->a64DD * 64 + 95;
        if (last >= 0 && last + 32 <= 0x348) SDL_memset(m->story + last, ' ', 32);
        scroll_erase_line(m);
        roll_up(m);
        scroll_clear(m);
        m->a64DD--;
        write_titles(m);
    }
    m->a6497 = keep;
    return 0;
}

/* the editors' menu loop (rt_0270): the campaign menu and what it leads to */
static int changes(Menu *m)
{
    for (;;) {
        int i = campaign_menu(m);
        if (i == R_FAIL) return R_FAIL;
        m->a6471 = i;
        if (i == 0) {                                  /* Make a new campaign: six steps */
            if (change_campaign(m)) return 0;
            m->a6485 = 3;
            if (map_maker(m)) return 0;
            m->a6485 = 9;
            if (ships_editor(m)) return 0;
            new_campaign(m);
            return 0;
        }
        if (i == 1) {                                  /* Change a campaign */
            int r = change_menu(m);
            if (r == R_BACK || r == 6) continue;       /* the remake: Esc, or nothing to change */
            return r == R_FAIL ? R_FAIL : 0;
        }
        if (i == 2) {                                  /* Remove a campaign */
            int r = remove_campaign(m);
            if (r == 0 || r == 5 || r == 6) continue;
            return r == R_FAIL ? R_FAIL : 0;
        }
        if (i == 3) return 0;                          /* Return to title page */
        if (i == 4) opening(m);                        /* The Ancient Art of War. */
        else if (i == 5) options_shoppe(m);            /* Ye Olde Options Shoppe. */
    }
}

/* ovl1_startup (ovl1:7662): SPM and the tune */
static void startup(Menu *m)
{
    m->view = 0;
    if (!load_picture(m, PIC_TITLE, 0)) return;
    blit(m, 200);
    palette(m, false);
    keys_clear(m);
    int units = 60000;
    if (opt(m, 0x684F) == 2) {                          /* play_tune (ovl1:8B7A) DS:0C1A, 0C86 */
        units = 0;
        for (int i = 0; i < 64 && dsw(m, 0xC1A + 2 * i); i++) units += dsw(m, 0xC86 + 2 * i) * 4;
        m->beeps++;
    }
    /* the remake: a key skips it */
    for (int u = 0; u < units && !m->n804 && !key802(m); u += 1000) delay(m, SDL_min(1000, units - u));
    keys_clear(m);
}

/* battle_report (ovl1:970A) */
static void battle_report(Menu *m)
{
    int code = m->rep_code;
    PictureId pic = code < 10 ? PIC_REPORT_1 : m->rep_opp == 0 ? PIC_REPORT_4 : m->rep_opp == 2 ? PIC_REPORT_3 : PIC_REPORT_2;
    m->view = 0;
    if (!load_picture(m, pic, 0)) return;
    m->rows = 1;
    text_rows_add(m, 0);
    palette(m, false);
    clear_row(m, 24);
    blit_menu(m);
    xor_text(m, SO3(m, 0xB32), 5, 5);                   /* Sunk */
    xor_number(m, m->rep[0], 17, 5);
    xor_number(m, m->rep[1], 21, 5);
    xor_text(m, SO3(m, 0xB71), 5, 6);                   /* Captured */
    xor_number(m, m->rep[2], 17, 6);
    xor_number(m, m->rep[3], 21, 6);
    xor_text(m, SO3(m, 0xB7A), 5, 7);                   /* Flags */
    xor_number(m, m->rep[4], 17, 7);
    xor_number(m, m->rep[5], 21, 7);
    bool music = opt(m, 0x684F) == 2;
    if (code < 10) {
        xor_text(m, SO2(m, 0xF86), 1, 1);               /* Victory was yours */
        static const unsigned WHY[5] = { 0xF99, 0xFB1, 0x1064, 0x10A7, 0x10D9 };
        if (code >= 0 && code <= 4) xor_text(m, SO2(m, WHY[code]), 1, 2);
        if (music) {                                    /* ovl1_9aab: SO2+122Fh, 1295h */
            int units = 0;
            for (int i = 0; i < 64 && so2w(m, 0x122F + 2 * i); i++) {
                int d = so2w(m, 0x1295 + 2 * i);
                units += d * 4 + ((d == 100 || d == 0x578 || d == 0x191) ? 0 : 0x4B0);
            }
            tune_wait(m, 0);
            for (; units > 0; units -= 20000) delay(m, SDL_min(units, 20000));
        }
    } else {
        xor_text(m, SO2(m, (unsigned)so2w(m, 0x10FD + 2 * SDL_clamp(m->rep_opp, 0, 5))), 1, 1);
        static const unsigned WHY[5] = { 0x107B, 0x1092, 0x104D, 0x231, 0x10D9 };
        if (code >= 10 && code <= 14) xor_text(m, SO2(m, WHY[code - 10]), 1, 2);
        if (music) {                                    /* ovl1_9b45: SO2+110Dh, 11E5h */
            int units = 0;
            for (int i = 0; i < 64 && so2w(m, 0x110D + 2 * i); i++) {
                int d = so2w(m, 0x11E5 + 2 * i);
                units += d / 2 + d * 4;
            }
            tune_wait(m, 0);
            for (; units > 0; units -= 20000) delay(m, SDL_min(units, 20000));
        }
    }
    clear_row(m, 24);
    print_at(m, 6, 24, SO2(m, 0x10BE));                 /* Press any key to continue. */
    keys_clear(m);
    while (!poll_key(m)) delay(m, 100);
    beep_low(m);
    show_one_moment(m);
}

/* save_game (ovl1:7B29): SS, 28CCh bytes of the data segment from 3F39h */
static int save_game(Menu *m)
{
    m->rows = 2;
    text_rows_add(m, 0);
    clear_all_rows(m);
    show_one_moment(m);
    keys_clear(m);
    if (ss_exists(m)) {
        print_at(m, 0, 21, SO2(m, 0xF07));              /* WARNING:  Any previously saved game on */
        print(m, SO2(m, 0xF30));                        /* your data disk will be erased. */
        clear_row(m, 24);
        print(m, SO2(m, 0xF59));                        /* Continue   Quit */
        for (;;) {
            int k = key802(m);
            if (k == PK_C) { for (int r = 21; r < 25; r++) clear_row(m, r); break; }
            if (k == 0x10) return 0;                    /* Q: back to the game, not saved */
            if (!k) delay(m, 100);
        }
    }
    print_at(m, 14, 10, SO3(m, 0xE48));                 /* Saving Game */
    uint8_t *ds = SDL_calloc(1, 0x10000);
    bool ok = false;
    if (ds && m->save_from) {
        play_save_ds(m->save_from, ds);
        char path[1200];
        SDL_snprintf(path, sizeof path, "%sSS", m->data_dir);
        ok = SDL_SaveFile(path, ds + SS_START, SS_SIZE);
    }
    SDL_free(ds);
    delay(m, 10000);                                    /* the remake: the floppy's half a second */
    clear_all_rows(m);
    if (!ok) {
        print_at(m, 0, 23, DS(m, 0x121));               /* Unable to use data disk. */
        print_at(m, 0, 24, DS(m, 0x13A));               /* Press any key to continue. */
        keys_clear(m);
        while (!poll_key(m)) delay(m, 100);
        return 0;
    }
    print_at(m, 3, 10, DS(m, 0xAE));                    /* <C>ontinue with game or <Q>uit? */
    keys_clear(m);
    for (;;) {
        int k = key802(m);
        if (k == PK_C) { clear_row(m, 10); show_one_moment(m); return 0; }
        if (k == 0x10) return 1;
        if (!k) delay(m, 100);
    }
}

/* ------------------------------------------------------------------ WAS.EXE's main loop (rt:0000) */

static void menu_main(void *arg)
{
    Menu *m = arg;
    switch (m->entry) {
    case E_START: startup(m); break;
    case E_REPORT: battle_report(m); break;
    case E_SAVE:
        if (save_game(m) == 0) { m->action = MENU_CONTINUE; return; }
        break;                                          /* the remake: Q goes to the title page */
    default: break;
    }
    for (;;) {
        int r = title_ship(m);
        if (r == 2) { if (changes(m) == R_FAIL) { m->action = MENU_QUIT; return; } continue; }
        if (r == 6 || r == 101) continue;
        if (r == 100) { m->action = MENU_RESUME_GAME; return; }
        if (r == 0) {                                   /* the page stays while campaign_play loads (play.c) */
            m->action = m->demo ? MENU_DEMO : MENU_START_GAME;
            return;
        }
        m->action = MENU_QUIT;
        return;
    }
}

/* ------------------------------------------------------------------ public */

static void begin(Menu *m, int entry)
{
    if (m->fiber_on) fiber_stop(&m->fiber);
    m->fiber_on = false;
    m->entry = entry;
    m->action = MENU_NONE;
    m->on_title = false;
    m->clock_us = m->target_us = 0;
    keys_clear(m);
    m->fiber_on = fiber_start(&m->fiber, menu_main, m);
}

Menu *menu_create(GameAssets *ga, Campaign *camps, const char *data_dir)
{
    Menu *m = SDL_calloc(1, sizeof *m);
    if (!m) return NULL;
    m->ga = ga;
    m->camps = camps;
    SDL_strlcpy(m->data_root, data_dir ? data_dir : "", sizeof m->data_root);
    Screen tmp;
    screen_set_palette(&tmp, STD16_PALETTE, 16);
    for (int i = 0; i < 4; i++) {
        m->c16[0][i] = screen_nearest(&tmp, 16, CGA_PALETTE[i].r, CGA_PALETTE[i].g, CGA_PALETTE[i].b);
        m->c16[1][i] = screen_nearest(&tmp, 16, CGA_PALETTE_0[i].r, CGA_PALETTE_0[i].g, CGA_PALETTE_0[i].b);
    }
    m->rnd = (uint16_t)(SDL_GetTicks() & 0xFF);          /* seed_random (rt:2EF9): the BIOS clock */
    m->rnd_ptr = 0xBB8;
    m->a6497 = 2;
    options_load(m);
    disks_scan(m);
    return m;
}

void menu_destroy(Menu *m)
{
    if (!m) return;
    if (m->fiber_on) fiber_stop(&m->fiber);
    SDL_free(m);
}

void menu_start(Menu *m) { begin(m, E_START); }
void menu_title(Menu *m) { begin(m, E_TITLE); }

void menu_report(Menu *m, const Play *p)
{
    m->rep_code = p->result_code;
    m->rep[0] = p->losses[0]; m->rep[1] = p->losses[1];       /* 64A5, 64A7: sunk */
    m->rep[2] = p->captured[0]; m->rep[3] = p->captured[1];   /* 64A1, 64A3: captured */
    m->rep[4] = p->losses[2]; m->rep[5] = p->losses[3];       /* 64A9, 64AB: flags */
    m->rep_opp = p->c ? p->c->opponent : 0;
    begin(m, E_REPORT);
}

void menu_save(Menu *m, const Play *p)
{
    m->save_from = p;
    begin(m, E_SAVE);
}

void menu_key(Menu *m, int key)
{
    if (!key) return;
    if (m->text_on) {                    /* the BIOS: what DOS reads for the keys that type nothing */
        if (key == PK_ENTER) dos_push(m, 13);
        else if (key == PK_ESC) dos_push(m, 27);
        else if (key == PK_BACKSPACE) dos_push(m, 8);
        else if ((key >= PK_HOME && key <= PK_PGDN && key != PK_KP_MINUS && key != PK_KP_PLUS) || key == PK_DELETE) {
            dos_push(m, 27);
            dos_push(m, key);
        }
        return;
    }
    m->q804[0] = key;                    /* the keyboard interrupt keeps the last key (rt:3160) */
    m->n804 = 1;
    m->k802 = key;
    m->up802 = false;
}

void menu_key_up(Menu *m) { if (m->k802) m->up802 = true; }
void menu_shift(Menu *m, bool down) { m->shift = down; }

bool menu_wants_text(const Menu *m) { return m->text_on; }

void menu_text(Menu *m, const char *utf8)
{
    if (!m->text_on) return;
    for (const unsigned char *c = (const unsigned char *)utf8; *c; c++)
        if (*c >= 32 && *c < 127) dos_push(m, *c);
}

void menu_run(Menu *m, double us)
{
    if (!m->fiber_on || m->action != MENU_NONE) return;
    if (us > 100000) us = 100000;
    m->target_us += us;
    if (m->clock_us < m->target_us && !fiber_resume(&m->fiber)) {   /* overlay 1 has returned */
        fiber_stop(&m->fiber);
        m->fiber_on = false;
        if (m->action == MENU_NONE) m->action = MENU_QUIT;
    }
}

void menu_draw(const Menu *m, Screen *s, GfxMode mode)
{
    if (mode == MODE_16) {
        screen_set_palette(s, STD16_PALETTE, 16);
        SDL_memcpy(s->px, m->v16, sizeof m->v16);
        return;
    }
    screen_set_palette(s, m->pal0 ? CGA_PALETTE_0 : CGA_PALETTE, 4);
    for (int y = 0; y < SCREEN_H; y++)
        for (int xb = 0; xb < 80; xb++) {
            uint8_t v = m->vram[y * 80 + xb];
            uint8_t *o = &s->px[y * SCREEN_W + xb * 4];
            o[0] = (v >> 6) & 3; o[1] = (v >> 4) & 3; o[2] = (v >> 2) & 3; o[3] = v & 3;
        }
}

MenuAction menu_action(Menu *m) { MenuAction a = m->action; m->action = MENU_NONE; return a; }
bool menu_on_title(const Menu *m) { return m->on_title; }
const Campaign *menu_game(const Menu *m) { return &m->game; }
void menu_random(const Menu *m, uint16_t *r, uint16_t *ptr) { *r = m->rnd; *ptr = m->rnd_ptr; }
int  menu_beeps(const Menu *m) { return m->beeps; }
const char *menu_data_dir(const Menu *m) { return m->data_dir; }

/* A game saved by the original or the remake: its campaign's title and story,
 * found by its ship names and opponent among the WAS disk's campaigns and the
 * data disk's (SS does not keep them: DS:4E53 holds the paths in play) */
static void saved_campaign(Menu *m, Campaign *c)
{
    for (int i = 0; i < NUM_CAMPAIGNS; i++) {
        const Campaign *k = &m->camps[i];
        if (!k->loaded) continue;
        bool same = true;
        for (int s = 0; s < MAX_SHIPS && same; s++) same = !SDL_strcmp(k->ships[s].name, c->ships[s].name);
        if (!same) continue;
        SDL_strlcpy(c->title, k->title, sizeof c->title);
        SDL_strlcpy(c->file, k->file, sizeof c->file);
        SDL_memcpy(c->story_raw, k->story_raw, sizeof c->story_raw);
        SDL_strlcpy(c->story, k->story, sizeof c->story);
        return;
    }
    size_t size = 0;
    uint8_t *t = assets_load_file(m->data_dir, "SITLES", &size);
    for (int k = 1; t && size >= 876 && k < 12; k++) {
        char name[4] = { (char)t[k * 3], (char)t[k * 3 + 1], 0, 0 };
        Campaign d;
        if (!campaign_load_file(&d, m->data_dir, name)) continue;
        bool same = true;
        for (int s = 0; s < MAX_SHIPS && same; s++) same = !SDL_strcmp(d.ships[s].name, c->ships[s].name);
        if (!same) continue;
        char title[33];
        int n = 0;
        for (int col = 2; col < 31; col++) {
            uint8_t ch = t[36 + (3 + 2 * k) * 32 + col];
            if (ch >= 0x20 && ch < 0x7F) title[n++] = (char)ch;
        }
        while (n > 0 && title[n - 1] == ' ') n--;
        title[n] = 0;
        if (n) SDL_strlcpy(c->title, title, sizeof c->title);
        SDL_memcpy(c->story_raw, d.story_raw, sizeof c->story_raw);
        SDL_strlcpy(c->story, d.story, sizeof c->story);
        break;
    }
    SDL_free(t);
}

bool menu_resume(Menu *m, Play *p, Campaign *c)
{
    if (!m->have_ss) return false;
    uint8_t *ds = SDL_calloc(1, 0x10000);
    if (!ds) return false;
    SDL_memcpy(ds + SS_START, m->ss, SS_SIZE);
    bool ok = play_load_ds(p, c, m->ga, ds, m->rnd, m->rnd_ptr);
    SDL_free(ds);
    if (!ok) return false;
    saved_campaign(m, c);
    play_resume(p);
    return true;
}
