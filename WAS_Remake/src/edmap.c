/*
 * The Ancient Art of War at Sea (1987) - remake
 * Nov 2026 using Claude Opus 5.5
 * "Guided" by Brett Hallen
 * Original game by Dave Murry & Barry Murry
 * Published by Brøderbund Software Inc.
 *
 * edmap.c: step 7, the map maker: overlay 3 of WAS.EXE (ovl3_7650), run on
 * overlay 1's thread (menu.c) as WAS.EXE runs it.
 *
 * The top of the screen shows four rows of the map with the cursor's tile
 * inverted; below are a plan of the whole map with the cursor's cell, and a
 * page of 18 tiles with the letters that put them down. + and - turn the
 * five pages (90 tiles), the arrows move the cursor, Enter is done. The
 * tiles are SM's (in 16 colours WAST.DAT's), drawn straight onto the screen.
 */
#include "menu_int.h"

#define PAGE_TILES 18

static const char *SO0(const Menu *m, unsigned off) { return assets_buf_string(m->ga->so0, sizeof m->ga->so0, off); }

/* mapmk_tile (ovl3:8537): a 32 x 20 tile onto the screen at a byte and a line */
static void mapmk_tile(Menu *m, int t, int xb, int line)
{
    const Image *c = &m->ga->set[MODE_CGA].tiles[t];
    const Image *v = m->ga->set[MODE_16].loaded ? &m->ga->set[MODE_16].tiles[t] : NULL;
    if (t < 0 || t >= NUM_TILES || !c->px) return;
    for (int y = 0; y < TILE_H; y++) {
        int l = line + y;
        if (l < 0 || l >= SCREEN_H) continue;
        for (int i = 0; i < TILE_W / 4; i++) {
            const uint8_t *p = &c->px[y * c->w + i * 4];
            if (xb + i < 0 || xb + i >= 80) continue;
            m->vram[l * 80 + xb + i] = (uint8_t)(((p[0] & 3) << 6) | ((p[1] & 3) << 4) | ((p[2] & 3) << 2) | (p[3] & 3));
            uint8_t *d = &m->v16[l * SCREEN_W + (xb + i) * 4];
            for (int k = 0; k < 4; k++)
                d[k] = v && v->px ? (v->px[y * v->w + i * 4 + k] & 15) : m->c16[m->pal0][p[k] & 3];
        }
    }
    WORK(m, 30 + TILE_H * 14);
}

/* mapmk_invert (ovl3:8561): the cursor's tile in the view, XORed */
static void mapmk_invert(Menu *m)
{
    int line = (m->a64D7 / 80) * 2, xb = m->a64D7 % 80;
    for (int l = 0; l < 20; l++)
        for (int i = 0; i < 8; i++) scr_xor(m, line + l, xb + i, 0xFF);
    WORK(m, 20 + 20 * 30);
}

/* mapmk_plan (ovl3:85B9): the plan of the whole map, a cell 4 x 4 */
static void mapmk_plan(Menu *m)
{
    for (int l = 90; l < 90 + 81; l++) {
        scr_byte(m, l, 2, 0x01);
        for (int i = 3; i < 13; i++) scr_byte(m, l, i, 0x01);
    }
    for (int l = 90; l <= 170; l += 4)
        for (int i = 3; i < 13; i++) scr_byte(m, l, i, 0x55);
    WORK(m, 40 + 81 * 25 + 21 * 20);
}

/* mapmk_plan_mark (ovl3:8591) at plan address a (0E13h + column + row x A0h) */
static void mapmk_plan_mark(Menu *m, int a)
{
    int line = (a / 80) * 2, xb = a % 80;
    for (int l = 0; l < 4; l++) scr_byte(m, line + l, xb, 0xFF);
    WORK(m, 40);
}

static void mapmk_plan_cursor(Menu *m)       /* ovl3:8128 */
{
    mapmk_plan(m);
    mapmk_plan_mark(m, m->a64D3 + 0xE13 + m->a64D5 * 160);
}

/* mapmk_page (ovl3:7B57): the page's 18 tiles in three rows of six */
static void mapmk_page(Menu *m)
{
    int xb = 21, line = 90;
    for (int k = 0; k < PAGE_TILES; k++) {
        mapmk_tile(m, m->a64D9 * PAGE_TILES + k, xb, line);
        if (k == 5 || k == 11) { xb = 21; line += 32; }
        else xb += 10;
    }
}

/* mapmk_panel (ovl3:79CE): the line beside the plan, a row of '_' under the
 * view, the tiles' letters */
static void mapmk_panel(Menu *m)
{
    for (int l = 88; l < 88 + 102; l++) scr_byte(m, l, 16, 0x0F);   /* draw_vline(88, 16, 51) */
    WORK(m, 102 * 12);
    gotoxy(m, 0, 10);
    for (int i = 0; i < 40; i++) put_char(m, '_');
    int c = 'A';
    gotoxy(m, 12, 14);
    for (; c < 'G'; c++) { put_char(m, c); for (int s = 0; s < 4; s++) put_char(m, ' '); }
    gotoxy(m, 12, 18);
    for (; c < 'M'; c++) { put_char(m, c); for (int s = 0; s < 4; s++) put_char(m, ' '); }
    gotoxy(m, 12, 22);
    for (; c < 'P'; c++) { put_char(m, c); for (int s = 0; s < 4; s++) put_char(m, ' '); }
    for (c = 'Q'; c < 'T'; c++) { put_char(m, c); for (int s = 0; s < 4; s++) put_char(m, ' '); }
    mapmk_page(m);
    mapmk_plan(m);
}

/* ovl3_807d: the four rows of the map from the top row, the cursor, the plan */
static void mapmk_view(Menu *m)
{
    int xb = 0, line = 0, first = m->a64D1 * 10;
    for (int k = 0; k < 40; k++) {
        int t = m->ed.map[(k + first) % 200] - 1;
        if (t >= 0) mapmk_tile(m, t, xb, line);
        if (k == 9 || k == 19 || k == 29) { xb = 0; line += 20; }
        else xb += 8;
    }
    mapmk_invert(m);
    mapmk_plan_cursor(m);
}

/* ovl3_7aa4: the keys on row 24 */
static void mapmk_keys(Menu *m)
{
    clear_row(m, 24);
    icon_ds(m, 0x64D, 192, 0);                          /* + */
    icon_ds(m, 0x66F, 192, 6);                          /* - */
    gotoxy(m, 6, 22);
    char n[4];
    SDL_snprintf(n, sizeof n, "%d", m->a64D9 + 1);
    print(m, n);
    draw_icon_from(m, m->ga->so0, sizeof m->ga->so0, 0, 192, 14);       /* A */
    gotoxy(m, 9, 24);
    put_char(m, '-');
    draw_icon_from(m, m->ga->so0, sizeof m->ga->so0, 34, 192, 20);      /* S */
    icon_ds(m, 0x62B, 192, 53);                         /* Enter */
    print_at(m, 30, 24, SO0(m, 0xEC));                  /* when done */
}

/* mapmk_read_key (ovl3:7E5B): [0804], once (the joystick is not read) */
static int mapmk_read_key(Menu *m)
{
    m->a6469 = 100;
    return poll_key(m);
}

/* ovl3_7bb3: one key; 1 when Enter ends the editing */
static int mapmk_key(Menu *m)
{
    bool redraw = false;
    int k;
    while (!(k = mapmk_read_key(m))) delay(m, 0x1F4);
    if (k == PK_ENTER) { beep_low(m); return 1; }
    if (k == PK_UP) {
        if (m->a64D5 > 0) { m->a64D5--; m->a64D7 -= 0x320; m->a64DD -= 10; }
        if (m->a64D5 < m->a64D1) { m->a64D1--; m->a64D7 += 0x320; }
        redraw = true;
    } else if (k == PK_LEFT) {
        if (m->a64D3 > 0) { m->a64D3--; m->a64D7 -= 8; m->a64DD--; }
        else {
            if (m->a64D5 > 0) { m->a64D5--; m->a64D3 = 9; m->a64D7 -= 0x2D8; m->a64DD--; }
            if (m->a64D5 < m->a64D1) { m->a64D1--; m->a64D7 += 0x320; }
        }
        redraw = true;
    } else if (k == PK_RIGHT) {
        if (m->a64D3 < 9) { m->a64D3++; m->a64D7 += 8; m->a64DD++; }
        else {
            if (m->a64D5 < 19) { m->a64D5++; m->a64D3 = 0; m->a64D7 += 0x2D8; m->a64DD++; }
            if (m->a64D1 + 3 < m->a64D5) { m->a64D1++; m->a64D7 -= 0x320; }
        }
        redraw = true;
    } else if (k == PK_DOWN) {
        if (m->a64D5 < 19) { m->a64D5++; m->a64D7 += 0x320; m->a64DD += 10; }
        if (m->a64D1 + 3 < m->a64D5) { m->a64D1++; m->a64D7 -= 0x320; }
        redraw = true;
    } else if (key_down(k) || key_up(k)) {             /* the next or the last page */
        beep_low(m);
        if (key_down(k)) { if (++m->a64D9 > 4) m->a64D9 = 0; }
        else if (--m->a64D9 < 0) m->a64D9 = 4;
        mapmk_page(m);
        gotoxy(m, 6, 22);
        char n[4];
        SDL_snprintf(n, sizeof n, "%d", m->a64D9 + 1);
        print(m, n);
    } else if ((k >= 16 && k <= 24) || (k >= 30 && k <= 38) || (k >= 44 && k <= 50)) {
        int c = m->ga->so0[0x14B + k - 16];             /* "Q0ER000IO00000ASDFGHJKL0000000C0BNM" */
        if (c != '0') {
            beep_low(m);
            if (c > 'P') c--;
            m->ed.map[SDL_clamp(m->a64DD, 0, 199)] = (uint8_t)(c - 64 + m->a64D9 * PAGE_TILES);
            redraw = true;
        }
    }
    if (redraw) mapmk_view(m);
    delay(m, m->a6469);
    return 0;
}

/* ovl3_7650: the map maker. 5 when Esc leaves the first page of a new
 * campaign, 0 to go on to the next step, 100 / 101 to save or forget the
 * changes (a campaign changed, [6485] = 0) */
int map_maker(Menu *m)
{
    m->a64D1 = m->a64DD = m->a64D5 = m->a64D3 = m->a64D7 = m->a64D9 = 0;
    m->view = 0;
    keys_clear(m);
    WORK(m, 3000);                                      /* SO0 and SM read (ovl3_804d) */
    for (int r = 24; r >= 0; r--) clear_row(m, r);
    colours(m, JR_EDITORS);                          /* rt_0c8f ovl3_7650 */
    mapmk_panel(m);
    m->rows = 1;
    text_rows_add(m, 0);
    if (m->a6485 == 3) {                                /* a new campaign */
        print_at(m, 1, 1, SO0(m, 0x48));                /* There are 6 steps to make a campaign. */
        print_at(m, 1, 2, SO0(m, 0x6F));                /* The first step is to make a map. */
        print_at(m, 1, 5, SO0(m, 0xB1));                /* Press   to begin or    if you've */
        print_at(m, 1, 6, SO0(m, 0xD2));                /* changed your mind. */
        icon_ds(m, 0x62B, 40, 13);
        draw_icon_from(m, m->ga->so0, sizeof m->ga->so0, 0xF6, 40, 41);   /* Esc */
        for (;;) {
            int k;
            while (!(k = mapmk_read_key(m))) maybe_yield(m), WORK(m, 40);
            if (k == PK_ESC) { beep_low(m); show_one_moment(m); return 5; }
            if (k == PK_ENTER) break;
        }
        keys_clear(m);
        beep_low(m);
        for (int i = 0, v = 1; i < 200; i++) {          /* the two plain seas, a checkerboard */
            if (i % 10 == 0 && ++v > 2) v = 1;
            m->ed.map[i] = (uint8_t)v;
            if (++v > 2) v = 1;
        }
    }
    mapmk_view(m);
    int r = 0;
    for (;;) {
        mapmk_keys(m);
        print_at(m, 1, 22, SO0(m, 0xE5));               /* PAGE 1 */
        gotoxy(m, 6, 22);
        char n[4];
        SDL_snprintf(n, sizeof n, "%d", m->a64D9 + 1);
        print(m, n);
        bool done = false;
        while (!done) {
            if (!mapmk_key(m)) continue;
            long steps = 0;
            clear_row(m, 24);
            show_one_moment(m);
            int ok = play_map_check(m->ga, m->ed.map, &steps);
            WORK(m, 4000 + steps * 60);
            if (ok < 1) {
                mtone(m, 0x1388, 0xBB8);                /* rt_30f9 */
                clear_row(m, 24);
                print(m, SO0(m, ok == 0 ? 0x1AA : 0x1CD));   /* ALL PORTS MUST HAVE A SUPPLY PATH. / BE NEXT TO WATER. */
                delay(m, 60000);
                break;                                  /* L7851: row 24 again */
            }
            if (m->a6485 == 0) {                        /* save the changes? */
                for (;;) {
                    clear_row(m, 24);
                    print(m, SO0(m, 0x12F));
                    int k;
                    while (!(k = mapmk_read_key(m))) delay(m, 0x1F4);
                    if (k == PK_Y) { print(m, SO0(m, 0x16F)); r = 100; break; }
                    if (k == PK_N) { print(m, SO0(m, 0x183)); r = 101; break; }
                }
                goto out;
            }
            for (;;) {                                  /* continue to the next step? */
                clear_row(m, 24);
                print(m, SO0(m, 0x186));
                delay(m, 0x1B58);
                int k;
                while (!(k = key802(m))) WORK(m, 20);
                if (k == PK_N) { m->n804 = 0; mapmk_keys(m); break; }
                if (k == PK_Y) { r = 0; keys_clear(m); goto out; }
            }
        }
    }
out:
    clear_row(m, 23);
    show_one_moment(m);
    return r;
}
