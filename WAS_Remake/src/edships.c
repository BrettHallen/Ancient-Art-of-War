/*
 * The Ancient Art of War at Sea (1987) - remake
 * Nov 2026 using Claude Opus 5.5
 * "Guided" by Brett Hallen
 * Original game by Dave Murry & Barry Murry
 * Published by Brøderbund Software Inc.
 *
 * edships.c: step 7, the ships editor: overlay 2 of WAS.EXE (ovl2_7650),
 * run on overlay 1's thread (menu.c) as WAS.EXE runs it.
 *
 * The whole map is drawn into BUF (load_sm_tiles), 400 lines, and shown 182
 * at a time; the marker moves over it, the view follows. S makes a squadron
 * at the marker, and on a squadron I shows its Info panel (names, types,
 * sails and hull, friend or foe, supplies), G and P move it, R removes it;
 * F counts the fleets. WAS.EXE draws the squadrons and the marker into BUF,
 * saving what was under them, and takes them off again before each change;
 * the remake keeps BUF as the bare map and puts them on as BUF is shown
 * (ed_overlay), which comes to the same picture.
 */
#include "menu_int.h"

static const char *SO1(const Menu *m, unsigned off) { return assets_buf_string(m->ga->so1, sizeof m->ga->so1, off); }

static void buzz(Menu *m) { m->beeps++; delay(m, 0x258); }       /* rt:0C72 */
static void long_buzz(Menu *m) { m->beeps++; delay(m, 0xBB8); }  /* rt_30f9 */

static int sq_addr(const Menu *m, int i) { return (m->ed.y[i] + 1) * 80 + m->ed.x[i]; }   /* DS:5F85 (BUF-relative) */

/* list_ships (rt:0630): the (up to 3) ships of squadron sq */
void ed_list_ships(Menu *m, int sq)
{
    m->list[0] = m->list[1] = m->list[2] = -1;
    for (int s = 0, n = 0; s < 40; s++)
        if (m->ed.ship_sq[s] == sq && n < 3) m->list[n++] = (int8_t)s;
    WORK(m, 60 + 40 * 14);
}

/* mark_flagship_squadrons (rt:0688): the pennant over a squadron with a
 * flagship; a squadron with no ships left goes */
static void mark_flagships(Menu *m)
{
    for (int i = 0; i < 40; i++) {
        if (m->ed.mark[i] == 2) continue;
        ed_list_ships(m, i);
        m->ed.mark[i] = 0;
        if (m->list[0] == -1) { m->ed.speed[i] = 9999; continue; }
        for (int k = 0; k < 3 && m->list[k] != -1; k++)
            if (m->ed.type[m->list[k]] == 3) m->ed.mark[i] = 1;
        if (m->ed.speed[i] == 9999) m->ed.speed[i] = 0;
    }
}

/* ovl2_94d0: the fleets counted (ships, and flag, line, frigate of each side) */
static void count_fleets(Menu *m)
{
    int *n = m->counts;                                 /* 64BF line, 64C1 frigate, 64C3 flag; 64C5, 64C7, 64C9 the foe's */
    SDL_memset(m->counts, 0, sizeof m->counts);
    m->nships = 0;
    for (int i = 0; i < 40; i++) {
        if (m->ed.speed[i] == 9999 || m->ed.mark[i] == 2) continue;
        ed_list_ships(m, i);
        m->ed.mark[i] = 0;
        for (int k = 0; k < 3 && m->list[k] != -1; k++) {
            int s = m->list[k], f = m->ed.side[i] == -1 ? 0 : 3;   /* commandable (rt:0B3D): DS:5EE5 is -2 in the editor */
            m->nships++;
            if (m->ed.type[s] == 3) { n[f + 2]++; m->ed.mark[i] = 1; }
            else if (m->ed.type[s] == 1) n[f]++;
            else n[f + 1]++;
        }
    }
    if (m->sq >= 0 && m->sq < 40) ed_list_ships(m, m->sq);
}

static void count_ships(Menu *m)                     /* ovl2_949f */
{
    m->nships = 0;
    for (int s = 0; s < 40; s++) if (m->ed.ship_sq[s] != -1) m->nships++;
}

/* ------------------------------------------------------------------ BUF and the screen */

/* a byte of BUF onto a screen byte through op: the sprite and marker drawing
 * of WAS.EXE, on the screen as BUF is shown. force: the pixels set */
static void over_byte(Menu *m, int a, int field, int lines, uint8_t (*op)(uint8_t old, uint8_t v, uint8_t *force), uint8_t v)
{
    int lp = a / 80, xb = a % 80, line = (lp - m->view) * 2 + field;
    if (a < 0 || line < 0 || line >= lines || line >= SCREEN_H) return;
    uint8_t force = 0, old = m->vram[line * 80 + xb], b = op(old, v, &force);
    m->vram[line * 80 + xb] = b;
    uint8_t *d = &m->v16[line * SCREEN_W + xb * 4];
    for (int k = 0; k < 4; k++)
        if ((force >> (6 - 2 * k)) & 3) d[k] = m->c16[0][(b >> (6 - 2 * k)) & 3];
}

static uint8_t op_set(uint8_t old, uint8_t v, uint8_t *f) { (void)old; *f = 0xFF; return v; }
static uint8_t op_cross(uint8_t old, uint8_t v, uint8_t *f) { (void)v; *f = 0xC3; return (uint8_t)((old | 0xC0) & 0xFC); }
/* draw_map_sprite (rt:2941): side -1 (the player's) white with a black edge, 0 black with a white edge */
static uint8_t op_white(uint8_t old, uint8_t d, uint8_t *f)
{
    *f = (uint8_t)(d | (d >> 2) | (d << 2));
    return (uint8_t)(d | (old & (uint8_t)~(d >> 2) & (uint8_t)~(d << 2)));
}
static uint8_t op_black(uint8_t old, uint8_t d, uint8_t *f)
{
    *f = (uint8_t)(d | (d >> 2) | (d << 2));
    return (uint8_t)((old | (uint8_t)(d >> 2) | (uint8_t)(d << 2)) & (uint8_t)~d);
}

static void map_sprite(Menu *m, unsigned spr, int a, int side, int lines)
{
    const uint8_t *s = &m->ga->ds[spr];
    int w = s[0], h = s[1];
    s += 2;
    for (int r = 0; r < h; r++, a += 80)
        for (int f = 0; f < 2; f++)
            for (int i = 0; i < w; i++) over_byte(m, a + i, f, lines, side ? op_white : op_black, *s++);
}

/* What WAS.EXE has drawn into BUF over the map: the squadrons
 * (draw_squadrons rt:10D2, each at anchor in the editor, the pennant a line
 * pair above) and the marker (draw_cursor rt:280E) */
void ed_overlay(Menu *m, int lines)
{
    if (m->sq_on)
        for (int i = 0; i < 40; i++) {
            if (m->ed.speed[i] == 9999) continue;
            int a = sq_addr(m, i), side = m->ed.side[i];
            map_sprite(m, 0x49B, a, side, lines);
            if (m->ed.mark[i] == 1) map_sprite(m, 0x5AF, a - 80, side, lines);
            if (m->ed.mark[i] == 2) map_sprite(m, 0x5BD, a - 80, side, lines);
        }
    if (m->mk_on) {
        int a = m->mk_addr;
        over_byte(m, a, 0, lines, op_set, 0x0F);
        over_byte(m, a, 1, lines, op_set, 0x0F);
        over_byte(m, a + 80, 0, lines, op_set, 0x0F);
        over_byte(m, a + 159, 0, lines, op_set, 0x0F);
        over_byte(m, a + 159, 1, lines, op_set, 0x0F);
        over_byte(m, a + 160, 0, lines, op_cross, 0);
        over_byte(m, a + 160, 1, lines, op_cross, 0);
        over_byte(m, a + 161, 0, lines, op_set, 0x3F);
        over_byte(m, a + 161, 1, lines, op_set, 0x3F);
        over_byte(m, a + 240, 1, lines, op_set, 0x0F);
        over_byte(m, a + 320, 0, lines, op_set, 0x0F);
        over_byte(m, a + 320, 1, lines, op_set, 0x0F);
    }
}

static void erase_squadrons(Menu *m) { m->mk_on = m->sq_on = false; WORK(m, 3350); }   /* rt:104A */

static void redraw_squadrons(Menu *m)               /* rt:1084 */
{
    int shown = 0;
    for (int i = 0; i < 40; i++) shown += m->ed.speed[i] != 9999;
    m->sq_on = true;
    if (m->a65AA != 99) m->mk_on = true;
    WORK(m, 3350 + 500 * shown);
}

/* load_sm_tiles (rt:0532): the map's tiles into BUF, 10 x 20 tiles of 32 x 20 */
static void map_into_buf(Menu *m)
{
    const AssetSet *cga = &m->ga->set[MODE_CGA], *t16 = &m->ga->set[MODE_16];
    for (int cell = 0; cell < 200; cell++) {
        int t = m->ed.map[cell] - 1, row = cell / 10, col = cell % 10;
        for (int y = 0; y < TILE_H; y++) {
            int line = row * TILE_H + y, a = (line >> 1) * 80 + (line & 1) * FIELD + col * 8;
            for (int i = 0; i < 8; i++) {
                uint8_t b = 0;
                const Image *c = t >= 0 && t < NUM_TILES ? &cga->tiles[t] : NULL;
                if (c && c->px) {
                    const uint8_t *p = &c->px[y * c->w + i * 4];
                    b = (uint8_t)(((p[0] & 3) << 6) | ((p[1] & 3) << 4) | ((p[2] & 3) << 2) | (p[3] & 3));
                }
                m->buf[a + i] = b;
                uint8_t *d = &m->b16[line * SCREEN_W + col * 32 + i * 4];
                const Image *v = t16->loaded && t >= 0 && t < NUM_TILES ? &t16->tiles[t] : NULL;
                for (int k = 0; k < 4; k++)
                    d[k] = v && v->px ? (v->px[y * v->w + i * 4 + k] & 15) : m->c16[0][(b >> (6 - 2 * k)) & 3];
            }
        }
    }
    WORK(m, 90 * 3000 + 200 * 400);                     /* 90 tiles read and the map looked through for each */
    redraw_squadrons(m);
}

/* ------------------------------------------------------------------ the Info panel's drawing */

static void draw_vline(Menu *m, int line, int xb, int n)    /* rt:2B71 */
{
    for (int l = 0; l < 2 * n; l++) scr_byte(m, line + l, xb, 0x0F);
    WORK(m, 20 + 2 * n * 8);
}

static void draw_bar(Menu *m, int line, int xb, int len, uint8_t pattern)   /* rt:2B2A */
{
    for (int l = 0; l < 3; l++) for (int i = 0; i < 0x15; i++) scr_byte(m, line + l, xb + i, 0);
    for (int l = 0; l < 3; l++) for (int i = 0; i < len; i++) scr_byte(m, line + l, xb + i, pattern);
    WORK(m, 60 + 3 * (0x15 + len) * 2);
}

/* invert_block (rt:2DC8): XOR FFh, w bytes by 2 x h lines at a screen address */
static void invert_block(Menu *m, int a, int w, int h)
{
    int line = (a / 80) * 2, xb = a % 80;
    for (int l = 0; l < 2 * h; l++) for (int i = 0; i < w; i++) scr_xor(m, line + l, xb + i, 0xFF);
    WORK(m, 30 + 2 * h * (w * 4 + 8));
}

static void icon_8x6(Menu *m, int line, int xb, unsigned off)   /* draw_icon_8x6 (rt:2BB4) */
{
    for (int l = 0; l < 6; l++)
        for (int i = 0; i < 2; i++) scr_byte(m, line + l, xb + i, m->ga->ds[off + l * 2 + i]);
    WORK(m, 60);
}

static void print_number(Menu *m, int v)            /* rt:26EA */
{
    char s[8];
    SDL_snprintf(s, sizeof s, "%d", v);
    print(m, s);
}

static const char *type_name(const Menu *m, int t) { return DS(m, t == 1 ? 0x58 : t == 2 ? 0x5D : 0x65); }   /* LINE FRIGATE FLAG */

/* squadron_info (rt:074B): the panel for squadron [62FB] */
static void squadron_info(Menu *m)
{
    for (int l = 62; l < 62 + 74; l++) for (int i = 5; i < 5 + 70; i++) scr_byte(m, l, i, 0);   /* clear_info_box */
    WORK(m, 74 * 45);
    draw_vline(m, 62, 4, 37);
    draw_vline(m, 62, 34, 24);
    draw_vline(m, 62, 50, 24);
    draw_vline(m, 62, 74, 37);
    draw_vline(m, 110, 23, 5);
    draw_vline(m, 120, 50, 3);
    draw_vline(m, 120, 54, 2);
    draw_vline(m, 120, 59, 3);
    draw_vline(m, 120, 64, 2);
    draw_vline(m, 120, 70, 3);
    hline(m, 62, 5, 70);
    hline(m, 72, 5, 70);
    hline(m, 110, 5, 70);
    hline(m, 120, 5, 70);
    hline(m, 136, 5, 70);
    print_at(m, 7, 8, DS(m, 0x2D));                     /* NAME */
    print_at(m, 19, 8, DS(m, 0x32));                    /* TYPE */
    print_at(m, 27, 8, DS(m, 0x37));                    /* SAIL/HULL */
    print_at(m, 14, 14, DS(m, 0x41));                   /* SUPPLIES--- */
    print_at(m, 4, 14, DS(m, m->ed.side[m->sq] == -1 ? 0x51 : 0x4D));   /* FRIEND / FOE */
    static const struct { int xb; unsigned off; } TICKS[10] = {
        { 50, 0x5EF }, { 53, 0x607 }, { 55, 0x613 }, { 58, 0x613 }, { 60, 0x5EF },
        { 63, 0x61F }, { 65, 0x613 }, { 68, 0x5FB }, { 70, 0x5EF }, { 72, 0x5EF },
    };
    for (int k = 0; k < 10; k++) icon_8x6(m, 128, TICKS[k].xb, TICKS[k].off);   /* 0 25 50 75 100 */
    for (int k = 0; k < 3 && m->list[k] != -1; k++) {
        int s = m->list[k];
        char name[16];
        SDL_memcpy(name, m->ed.names[s], 15);
        name[15] = 0;
        print_at(m, 3, k + 10, name);
        print_at(m, 18, k + 10, type_name(m, m->ed.type[s]));
        int n = (5 - m->ed.sails[s]) * 4;
        draw_bar(m, k * 8 + 81, 51, n ? n : 1, 0xDD);
        n = (5 - m->ed.hull[s]) * 4;
        draw_bar(m, k * 8 + 84, 51, n ? n : 1, 0x11);
    }
    draw_bar(m, 112, 51, m->ed.supplies[m->sq], 0x55);
    draw_bar(m, 115, 51, m->ed.supplies[m->sq], 0x55);
}

/* ------------------------------------------------------------------ row 24 */

static void text_area_reset(Menu *m)                /* rt:0C10 */
{
    if (m->rows > 2) text_rows_add(m, 2 - m->rows);
    clear_row(m, 23);
}

static void command_line(Menu *m)                   /* ovl2_7992 */
{
    count_ships(m);
    clear_row(m, 24);
    if (m->nships < 40) print(m, SO1(m, 0x26));         /* Squad */
    print(m, SO1(m, 0x194));                            /* Fleet */
    icon_ds(m, 0x62B, 192, 54);
    print_at(m, 30, 24, SO1(m, 0x33));                  /* when done */
    m->a65AA = 0;
}

static void on_squadron_line(Menu *m)               /* ovl2_7b2b */
{
    mark_flagships(m);
    text_area_reset(m);
    clear_row(m, 24);
    print(m, SO1(m, 0x2D));                             /* Info */
    print(m, SO1(m, 0x53));                             /* Get Remove */
    m->a65AA = 1;
    delay(m, 0x1B58);
}

static void name_line(Menu *m, bool plus_minus)     /* ovl2_8ee2 */
{
    clear_row(m, 24);
    print(m, SO1(m, 0x92));                             /* Type the ship's name. */
    if (plus_minus) { icon_ds(m, 0x64D, 192, 44); icon_ds(m, 0x66F, 192, 48); }
    icon_ds(m, 0x62B, 192, 54);
    print_at(m, 30, 24, SO1(m, 0x33));                  /* when done */
}

static void ship_line(Menu *m)                      /* ovl2_8dac */
{
    clear_row(m, 24);
    print(m, SO1(m, 0x1C2));                            /* Name */
    print(m, SO1(m, 0x1C8));                            /* Type Sails Hull Remove */
    print_at(m, 35, 24, SO1(m, 0x1BA));                 /* End */
    icon_ds(m, 0x64D, 192, 55);
    icon_ds(m, 0x66F, 192, 61);
}

static void squad_line(Menu *m)                     /* ovl2_8e0a */
{
    clear_row(m, 24);
    print(m, SO1(m, m->ed.side[m->sq] != 0 ? 0x82 : 0x8A));   /* Foe / Friend: what F makes it */
    print(m, SO1(m, 0x48));                             /* Supplies */
    print_at(m, 35, 24, SO1(m, 0x1BA));                 /* End */
    icon_ds(m, 0x64D, 192, 32);
    icon_ds(m, 0x66F, 192, 36);
}

static void bar_line(Menu *m)                       /* ovl2_8e87 */
{
    beep_low(m);
    clear_row(m, 24);
    icon_ds(m, 0x64D, 192, 0);
    icon_ds(m, 0x66F, 192, 4);
    icon_ds(m, 0x62B, 192, 54);
    print_at(m, 30, 24, SO1(m, 0x33));                  /* when done */
}

/* sqed_read_key (ovl2:7FC2): [0804], and a cursor key held down again and again */
static int sqed_read_key(Menu *m)
{
    m->a6469 = 2;
    WORK(m, 30);
    int k802 = m->up802 ? 0 : m->k802;                  /* [0802]: 0 once the key is let go */
    if (m->up802) { m->k802 = 0; m->up802 = false; }
    int k = m->n804 ? m->q804[0] : 0;
    if (k > 70) k = k802;
    m->q804[0] = k802;
    m->n804 = k802 ? 1 : 0;
    maybe_yield(m);
    return k;
}

static void keys_off(Menu *m) { m->n804 = 0; m->k802 = 0; m->up802 = false; }   /* [0804] = [0802] = 0 */

/* ------------------------------------------------------------------ a ship's name */

static void name_blink(Menu *m, int pos, int row, bool force)   /* ovl2_9443 */
{
    if (--m->a64CB > 0 && !force) return;
    m->a64CB = 150;
    m->a64CD ^= 1;
    invert_block(m, row * 0x140 + (pos + 3) * 2, 2, 4);
}

static bool name_taken(Menu *m, const uint8_t *buf)     /* ovl2_93b2 */
{
    int me = m->list[m->a64CF];
    for (int s = 0; s < 40; s++) {
        if (s == me || m->ed.ship_sq[s] == -1) continue;
        if (!SDL_memcmp(m->ed.names[s], buf, 14)) return true;
    }
    return false;
}

static bool other_flagship(Menu *m)                 /* ovl2_8d59 */
{
    for (int k = 0; k < 3; k++)
        if (m->list[k] != -1 && m->a64CF != k && m->ed.type[m->list[k]] == 3) return true;
    return false;
}

static void new_ship(Menu *m)                       /* ovl2_8ce2: the first free ship into the squadron */
{
    int s = 0;
    while (s < 40 && m->ed.ship_sq[s] != -1) s++;
    if (s >= 40) return;
    m->list[m->a64CF] = (int8_t)s;
    m->ed.ship_sq[s] = (int8_t)m->sq;
    m->ed.type[s] = other_flagship(m) ? 1 : 3;
    m->ed.hull[s] = m->ed.sails[s] = 0;
    m->nships++;
}

/* ovl2_8f6b: type the name of ship item; returns the + or - that left the
 * line empty, or 0 */
static int name_input(Menu *m, int item)
{
    uint8_t buf[16] = { 0 };                            /* DS:6366 */
    if (m->list[item] != -1) SDL_memcpy(buf, m->ed.names[m->list[item]], 15);
    text_begin(m);
    int row = item + 10, pos = 0, left = 0;
    gotoxy(m, 3, row);
    dos_key(m);
    dos_key(m);
    m->a64CB = m->a64CD = 0;
    for (;;) {
        int k = dos_key(m);
        if (!k) { name_blink(m, pos, row, false); continue; }
        if (m->a64CD) name_blink(m, pos, row, true);
        if (pos == 0 && !buf[0] && (k == '+' || k == '=' || k == '_' || k == '-')) { left = k; break; }
        if (k == 8) {
            if (--pos < 0) { pos = 0; continue; }
            if (pos == 12) { print(m, DS(m, 0x1AE)); buf[13] = 0; }
            print(m, DS(m, 0x1B1));                     /* back, space, back */
            buf[pos] = ' ';
            if (!buf[pos + 1]) buf[pos] = 0;
            name_line(m, pos == 0 && !buf[0]);
            gotoxy(m, pos + 3, row);
            continue;
        }
        if (k == 27) {
            k = dos_key(m);
            if (pos == 0 && !buf[0] && (k == PK_KP_MINUS || k == PK_KP_PLUS)) { left = k; break; }
            if (k == PK_LEFT) {
                if (--pos < 0) { pos = 0; name_line(m, !buf[0]); }
                if (!buf[pos + 1] && buf[pos] == ' ') buf[pos] = 0;
                gotoxy(m, pos + 3, row);
            } else if (k == PK_RIGHT) {
                if (!buf[pos]) buf[pos] = ' ';
                if (++pos > 13) { pos = 13; continue; }
                if (pos == 1) name_line(m, false);
                gotoxy(m, pos + 3, row);
            } else { m->beeps++; delay(m, 0x4B0); }      /* ovl2_8f4e */
            continue;
        }
        if (k >= 32 && k <= 122 && pos < 14) {
            if (k >= 'a' && k <= 'z') k -= 32;
            buf[pos] = (uint8_t)k;
            put_char(m, k);
            if (++pos > 13) pos = 13;
            if (pos == 1) name_line(m, false);
            gotoxy(m, pos + 3, row);
            if (pos == 13) gotoxy(m, 16, row);
            continue;
        }
        if (k == 10 || k == 13) break;
        m->beeps++;
        delay(m, 0x4B0);
    }
    for (pos = 13; pos >= 0 && (buf[pos] == ' ' || !buf[pos]); pos--) buf[pos] = 0;
    pos++;
    text_end(m);
    keys_off(m);
    if (left) return left;
    if (pos == 0) return 0;
    if (m->list[item] == -1) new_ship(m);
    if (m->list[item] == -1) return 0;
    SDL_memcpy(m->ed.names[m->list[item]], buf, 15);
    if (name_taken(m, buf)) {
        long_buzz(m);
        clear_row(m, 24);
        print(m, SO1(m, 0x116));                        /* WARNING, ANOTHER SHIP HAS THIS NAME. */
        delay(m, 60000);
        name_line(m, true);
    }
    return 0;
}

/* ------------------------------------------------------------------ the Info panel */

static void item_mark(Menu *m) { invert_block(m, m->a64CF * 0x140 + 0xC85, 1, 4); }

static int wait_key(Menu *m)
{
    int k;
    while (!(k = sqed_read_key(m))) WORK(m, 20);
    return k;
}

static bool is_plus(int k)  { return k == PK_EQUALS || k == 43 || k == 61 || k == PK_KP_PLUS; }
static bool is_minus(int k) { return k == PK_MINUS || k == 45 || k == 95 || k == PK_KP_MINUS; }

/* a bar changed with + and -, Enter when done (sails, hull, supplies) */
static void bar_edit(Menu *m, int8_t *v, int lo, int hi, bool plus_up, int line, int line2, uint8_t pattern)
{
    bar_line(m);
    for (;;) {
        keys_off(m);
        int k = wait_key(m);
        int d = (k == PK_EQUALS || k == PK_KP_PLUS) ? 1 : (k == PK_MINUS || k == PK_KP_MINUS) ? -1 : 0;
        if (d) {
            int n = *v + (plus_up ? d : -d);
            *v = (int8_t)SDL_clamp(n, lo, hi);
            int len = plus_up ? *v : (5 - *v) * 4;
            draw_bar(m, line, 51, len, pattern);
            if (line2) draw_bar(m, line2, 51, len, pattern);
        }
        if (k == PK_ENTER) break;
    }
    beep_low(m);
    keys_off(m);
}

/* the item after (dir 1) or before (-1) the marked one; false: a ship's name to type */
static bool item_step(Menu *m, int dir, bool unmark)
{
    if (unmark) item_mark(m);
    if (dir > 0) {
        do {
            if (++m->a64CF > 4) m->a64CF = 0;
            if (m->a64CF == 1 && m->list[0] == -1) m->a64CF++;
            if (m->a64CF == 2 && m->list[1] == -1) m->a64CF++;
        } while (m->nships >= 40 && m->a64CF < 3 && m->list[m->a64CF] == -1);
        if (m->a64CF == 3) m->a64CF++;
    } else {
        do {
            if (--m->a64CF < 0) { m->a64CF = 4; squad_line(m); }
            if (m->a64CF == 3) m->a64CF--;
            if (m->a64CF == 2) ship_line(m);
            if (m->a64CF == 2 && m->list[1] == -1) m->a64CF--;
            if (m->a64CF == 1 && m->list[0] == -1) m->a64CF--;
        } while (m->nships >= 40 && m->a64CF < 3 && m->list[m->a64CF] == -1);
    }
    item_mark(m);
    beep_low(m);
    keys_off(m);
    if (m->a64CF == 4) { if (dir > 0) squad_line(m); return true; }
    if (m->list[m->a64CF] != -1) { ship_line(m); return true; }
    return false;
}

/* ovl2_8457: Info, the squadron's ships and its side and supplies */
static void squadron_panel(Menu *m)
{
    m->a64CF = 0;
    ed_list_ships(m, m->sq);
    squadron_info(m);
    item_mark(m);
    delay(m, 0x1388);
    bool name = m->list[0] == -1;
    if (!name) ship_line(m);
    for (;;) {
        int k = 0;
        if (!name) {
            k = wait_key(m);
            if (is_plus(k)) { name = !item_step(m, 1, true); if (!name) continue; }
            else if (is_minus(k)) { name = !item_step(m, -1, true); if (!name) continue; }
            else if (k == PK_E) { beep_low(m); break; }
            else if (m->a64CF < 3 && k == PK_N) name = true;
        }
        if (name) {                                     /* L86E8 */
            name = false;
            item_mark(m);
            name_line(m, true);
            int r = name_input(m, m->a64CF);
            beep_low(m);
            if (r == '+' || r == '=' || r == PK_KP_PLUS) { name = !item_step(m, 1, false); continue; }
            if (r == '-' || r == '_' || r == PK_KP_MINUS) { name = !item_step(m, -1, false); continue; }
            squadron_info(m);
            item_mark(m);
            ship_line(m);
        } else if (m->a64CF < 3) {
            int s = m->list[m->a64CF];
            if (k == PK_T && s != -1) {
                if (++m->ed.type[s] > 3) m->ed.type[s] = 1;
                print_at(m, 18, m->a64CF + 10, SO1(m, m->ed.type[s] == 1 ? 0xA8 : m->ed.type[s] == 2 ? 0xB0 : 0xB8));
                beep_low(m);
                keys_off(m);
            } else if (k == PK_S && s != -1) {
                bar_edit(m, &m->ed.sails[s], 0, 4, false, m->a64CF * 8 + 81, 0, 0xDD);
                ship_line(m);
            } else if (k == 0x23 && s != -1) {          /* H */
                bar_edit(m, &m->ed.hull[s], 0, 4, false, m->a64CF * 8 + 84, 0, 0x11);
                ship_line(m);
            } else if (k == 0x13 && s != -1) {          /* R: the ship leaves the squadron */
                SDL_memset(m->ed.names[s], 0, 15);
                m->ed.ship_sq[s] = -1;
                m->list[m->a64CF] = -1;
                ed_list_ships(m, m->sq);
                squadron_info(m);
                beep_low(m);
                keys_off(m);
                item_mark(m);
                m->nships--;
            }
        } else if (k == PK_F) {
            gotoxy(m, 4, 14);
            if (m->ed.side[m->sq] == 0) { m->ed.side[m->sq] = -1; print(m, SO1(m, 0xE6)); }   /* FRIEND */
            else { m->ed.side[m->sq] = 0; print(m, SO1(m, 0xED)); }                           /* FOE */
            squad_line(m);
            beep_low(m);
            keys_off(m);
        } else if (k == PK_S) {
            bar_edit(m, &m->ed.supplies[m->sq], 0, 20, true, 112, 115, 0x55);
            squad_line(m);
        }
        delay(m, m->a6469);
        delay(m, 0x1388);
    }
    count_fleets(m);                                    /* L8CAE */
    if (m->list[0] != -1) on_squadron_line(m);
    else { m->ed.speed[m->sq] = 9999; command_line(m); }
    redraw_squadrons(m);
    blit(m, m->blit_lines);
}

/* ------------------------------------------------------------------ the map */

static void fleet_counts(Menu *m)                   /* ovl2_79f6 */
{
    count_fleets(m);
    text_rows_add(m, 2);
    const int *n = m->counts;
    for (int side = 0; side < 2; side++) {
        clear_row(m, 21 + side);
        print(m, SO1(m, side ? 0x1B3 : 0x1AA));         /* Friend: / Foe: */
        if (side) gotoxy(m, 8, 22);
        static const int ORDER[3] = { 2, 0, 1 };        /* flag, line, frigate */
        static const unsigned WORD[3] = { 0xC0, 0xC8, 0xD0 };
        for (int k = 0; k < 3; k++) {
            int v = n[side * 3 + ORDER[k]];
            if (v < 10) put_char(m, ' ');
            print_number(m, v);
            print(m, SO1(m, WORD[k]));
        }
    }
    clear_row(m, 24);
    print(m, SO1(m, 0x1BA));                            /* End */
}

static void put_squadron(Menu *m, int i)            /* ovl2:7CB4 and ovl2_8395: at the marker */
{
    m->ed.x[i] = (int16_t)SDL_max(m->mk_x, 3);
    m->ed.y[i] = (int16_t)(m->mk_y - 2);
}

/* ovl2_8277: is the marker on a squadron? */
static void marker_on_squadron(Menu *m)
{
    bool on = false;
    m->sq = -1;
    for (int i = 0; i < 40; i++) {
        if (m->ed.speed[i] == 9999) continue;
        int a = sq_addr(m, i), d = m->mk_addr - a;
        if (d == -160 || d == -80 || d == 0 || d == 80 || d == 160) {
            on = true;
            m->sq = i;
            if (!m->a64DB) { buzz(m); on_squadron_line(m); }
        }
    }
    if (!on && m->a64DB) command_line(m);
    m->a64DB = on;
}

/* ovl2_7ba2: one key; 1 when Enter ends the editing */
static int sqed_key(Menu *m)
{
    int k = sqed_read_key(m);
    if (m->a65AA == 0) {
        if (k == PK_F) {
            beep_low(m);
            fleet_counts(m);
            delay(m, 0x1B58);
            while (!sqed_read_key(m)) WORK(m, 20);
            beep_low(m);
            text_area_reset(m);
            command_line(m);
            keys_off(m);
        }
        if (k == PK_ENTER) return 1;
        if (k == PK_S && m->nships < 40) {
            m->sq = 0;
            while (m->sq < 40 && m->ed.speed[m->sq] != 9999) m->sq++;
            if (m->sq < 40) {                           /* ovl2_8395: a new squadron at the marker */
                beep_low(m);
                erase_squadrons(m);
                m->ed.speed[m->sq] = 0;
                m->ed.side[m->sq] = -1;
                m->ed.supplies[m->sq] = 20;
                m->ed.mark[m->sq] = 0;
                put_squadron(m, m->sq);
                m->a64DB = 1;
                squadron_panel(m);
            }
            keys_off(m);
        }
    } else if (m->a65AA == 1) {
        if (k == 0x13) {                                /* R: ovl2_8336 */
            beep_low(m);
            erase_squadrons(m);
            ed_list_ships(m, m->sq);
            for (int j = 0; j < 3; j++) if (m->list[j] != -1) m->ed.ship_sq[m->list[j]] = -1;
            m->ed.speed[m->sq] = 9999;
            redraw_squadrons(m);
            command_line(m);
            keys_off(m);
        }
        if (k == PK_I) {
            beep_low(m);
            erase_squadrons(m);
            squadron_panel(m);
            keys_off(m);
        }
        if (k == 0x22) {                                /* G: sqed_get */
            clear_row(m, 23);
            print(m, SO1(m, 0x5E));                     /* Move to the new location */
            clear_row(m, 24);
            print(m, SO1(m, 0x1BE));                    /* Put */
            m->a65AA = 20;
            beep_low(m);
            keys_off(m);
        }
    } else if (m->a65AA == 20 && k == PK_P) {
        beep_low(m);
        erase_squadrons(m);
        put_squadron(m, m->sq);
        redraw_squadrons(m);
        on_squadron_line(m);
        keys_off(m);
    }
    m->mk_on = false;                                   /* restore_cursor_background */
    bool moved = false;
    int dx = 0, dy = 0;
    switch (k) {
    case PK_HOME: dx = -1; dy = -1; break;
    case PK_UP:   dy = -1; break;
    case PK_PGUP: dx = 1; dy = -1; break;
    case PK_LEFT: dx = -1; break;
    case PK_RIGHT: dx = 1; break;
    case PK_END:  dx = -1; dy = 1; break;
    case PK_DOWN: dy = 1; break;
    case PK_PGDN: dx = 1; dy = 1; break;
    default: break;
    }
    if (dx || dy) {
        if (dx < 0 && m->mk_x > 2) { m->mk_x--; m->mk_addr--; }
        if (dx > 0 && m->mk_x < 76) { m->mk_x++; m->mk_addr++; }
        if (dy < 0 && m->mk_y > 4) { m->mk_y--; m->mk_addr -= 80; }
        if (dy > 0 && m->mk_y < 194) { m->mk_y++; m->mk_addr += 80; }
        moved = true;
    }
    if (key_up(k) && m->view > 1) {                     /* the view up two line pairs; with Shift to the top */
        if (m->shift) { m->view = 0; m->mk_x = 38; m->mk_y = 45; m->mk_addr = 0xE36; }
        else {
            m->view -= 2;
            if (m->view + 86 < m->mk_y) { m->mk_y -= 2; m->mk_addr -= 160; }
        }
        moved = true;
    }
    if (key_down(k) && m->view < 108) {                 /* down; with Shift to the bottom */
        if (m->shift) { m->view = 109; m->mk_x = 38; m->mk_y = 145; m->mk_addr = 0x2D76; }
        else {
            m->view += 2;
            if (m->mk_y < m->view) { m->mk_y += 2; m->mk_addr += 160; }
        }
        moved = true;
    }
    if (m->mk_y < m->view) m->view--;                   /* the view follows the marker */
    else if (m->view + 86 < m->mk_y) m->view++;
    m->mk_on = true;                                    /* save_cursor_background, draw_cursor */
    WORK(m, 200);
    if (moved && m->a65AA != 20) marker_on_squadron(m);
    return 0;
}

/* ovl2_7650: the ships editor. 5 when Esc leaves the first page of a new
 * campaign, 0 to go on to the next step, 100 / 101 to save or forget the
 * changes (a campaign changed, [6485] = 0) */
int ships_editor(Menu *m)
{
    keys_clear(m);
    m->a64DB = 0;
    m->view = 0;
    m->sq = -1;
    WORK(m, 2000);                                      /* SO1 read */
    m->mk_x = 38;
    m->mk_y = 45;
    m->mk_addr = 0xE36;
    m->mk_on = m->sq_on = false;
    m->a65AA = 0;
    mark_flagships(m);
    count_fleets(m);
    map_into_buf(m);
    m->rows = 2;
    m->a6469 = 2;
    text_rows_add(m, 0);
    palette(m, false);
    text_area_reset(m);
    int r = 0;
    if (m->a6485 == 9) {                                /* a new campaign */
        blit(m, m->blit_lines);
        print(m, SO1(m, 0));                            /* The second step is to put ships on */
        clear_row(m, 24);
        print(m, SO1(m, 0x1EA));                        /* the map. Press any key. */
        draw_icon_from(m, m->ga->so1, sizeof m->ga->so1, 0x259, 192, 50);   /* Esc */
        print_at(m, 29, 24, SO1(m, 0x3D));              /* to quit. */
        int k;
        while (!(k = key802(m))) WORK(m, 20);
        beep_low(m);
        if (k == PK_ESC) { clear_row(m, 23); show_one_moment(m); r = 5; goto out; }
        clear_row(m, 23);
        delay(m, 0x1B58);
    }
    for (;;) {
        command_line(m);
        for (;;) {                                      /* L77BF */
            blit(m, m->blit_lines);
            delay(m, 0x1F4);
            if (!sqed_key(m)) continue;
            beep_low(m);
            clear_row(m, 24);
            if (m->a6485 == 0) {
                for (;;) {                              /* save the changes? */
                    clear_row(m, 24);
                    print(m, SO1(m, 0x174));
                    delay(m, 0x1B58);
                    int k;
                    while (!(k = key802(m))) WORK(m, 20);
                    if (k == PK_Y) { print(m, SO1(m, 0x190)); r = 100; break; }
                    if (k == PK_N) { print(m, SO1(m, 0x20C)); r = 101; break; }
                }
                break;
            }
            bool next = false;
            for (;;) {                                  /* continue to the next step? */
                clear_row(m, 24);
                print(m, SO1(m, 0x20F));
                delay(m, 0x1B58);
                int k;
                while (!(k = key802(m))) WORK(m, 20);
                if (k == PK_N) { m->n804 = 0; command_line(m); break; }
                if (k == PK_Y) { r = 0; keys_off(m); next = true; break; }
            }
            if (next) break;
        }
        clear_row(m, 23);
        show_one_moment(m);
        if (r == 101) break;
        int friend_ships = 0, foe_ships = 0;
        for (int i = 0; i < 40; i++) {
            if (m->ed.speed[i] == 9999) continue;
            ed_list_ships(m, i);
            for (int k = 0; k < 3 && m->list[k] != -1; k++) {
                if (m->ed.side[i] == -1) friend_ships++;
                else foe_ships++;
            }
        }
        if (friend_ships && foe_ships) break;
        long_buzz(m);
        clear_row(m, 24);
        print(m, SO1(m, 0x233));                        /* MUST HAVE BOTH FRIEND AND FOE SQUADS. */
        delay(m, 60000);
    }
out:
    m->mk_on = m->sq_on = false;
    return r;
}
