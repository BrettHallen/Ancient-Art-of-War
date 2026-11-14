/*
 * The Ancient Art of War at Sea (1987) - remake
 * Nov 2026 using Claude Opus 5.5
 * "Guided" by Brett Hallen
 * Original game by Dave Murry & Barry Murry
 * Published by Brøderbund Software Inc.
 *
 * play.c: campaign play on the sea map (overlay 4 of WAS.EXE; see play.h).
 *
 * Each function names the routine it ports. DS addresses are WAS.EXE's;
 * SO5 offsets are into the campaign-play messages (loaded to DS:4AF5).
 * WAS.EXE draws the map into a buffer of 400 lines (even lines in its first
 * 16000 bytes, odd lines in the next) and some rules look at its bytes:
 * buf_byte() works them out from the CGA tiles, in both colour modes.
 */
#include "play_int.h"
#include "gamelog.h"
#include "sound.h"

#include <SDL3/SDL.h>

/* the debug log (gamelog.h): only reads the game; its words (gamelog_squad,
   gamelog_cell_of ...) never count work, so the game's timing is untouched */
#define LOGN(...) GLOG(p, GLOG_NORMAL, __VA_ARGS__)
#define LOGV(...) GLOG(p, GLOG_VERBOSE, __VA_ARGS__)
#define SQN(i)    gamelog_squad(p, (i))
#define CELLN(c)  gamelog_cell(p, (c))
#define LOGGING   gamelog_on(p, GLOG_NORMAL)

/* ------------------------------------------------------------------ helpers */

static const char *DS(const Play *p, unsigned off) { return assets_ds_string(p->ga, off); }
static const char *SO5(const Play *p, unsigned off) { return assets_so5_string(p->ga, off); }

static int ds_word(const Play *p, unsigned off) { return (int16_t)(p->ga->ds[off] | (p->ga->ds[off + 1] << 8)); }
static int ds_sbyte(const Play *p, unsigned off) { return (int8_t)p->ga->ds[off]; }

#define OPP(p, addr) ((p)->opp[((addr) - 0x604D) / 2])

uint32_t play_ev[EV_COUNT];

/* the sounds' clock: the iterations so far, and within one the delays */
static double snd_t(Play *p)
{
    double base = p->snd_frames * (1e6 / PLAY_HZ);
    if (p->snd_us < base) p->snd_us = base;
    return p->snd_us;
}
static void pspk_on(Play *p) { p->beeps++; spk_on(SND_PLAY, snd_t(p)); }
static void pspk_freq(Play *p, int divisor) { spk_freq(SND_PLAY, snd_t(p), divisor); }
static void pspk_off(Play *p) { spk_off(SND_PLAY, snd_t(p)); }

/* a note: speaker_on, rt_2f3f, delay, rt_2f67 */
static void tone(Play *p, int divisor, int units)
{
    pspk_on(p);
    pspk_freq(p, divisor);
    delay(p, units);
    pspk_off(p);
}

static void beep(Play *p) { tone(p, 0x1F4, 0x190); }          /* beep_low (rt:0C32) */
static void beep_message(Play *p) { tone(p, 0x3E8, 0x546); }  /* rt:30E2 */
static void buzz(Play *p) { tone(p, 0x1F40, 0x258); }         /* rt:0C72: the marker comes onto a squadron */

/* play_end_sound (ovl4:C38F): two low notes, with all sounds on */
static void end_sound(Play *p)
{
    if (assets_scnf_word(p->ga, 0x684F) <= 1) return;
    pspk_on(p);
    pspk_freq(p, 0x2328);
    delay(p, 0x1388);
    pspk_freq(p, 0x2134);
    delay(p, 0x9C4);
    pspk_off(p);
}

/* port_empty_sound (ovl4:C57C): three notes, a port has nothing left */
static void port_empty_sound(Play *p)
{
    pspk_on(p);
    pspk_freq(p, 0xFA0);
    delay(p, 0x4B0);
    pspk_freq(p, 0xBB8);
    delay(p, 0x1A4);
    pspk_freq(p, 0x1F40);
    delay(p, 0x2EE);
    pspk_off(p);
}

/* delay() (rt:2F6E): WAS.EXE waits, calling random() all the while (units / 2
 * times on an XT); the remake skips that many iterations (the random()
 * calls are this time, not work of their own) */
void delay(Play *p, int units)
{
    p->stall += (int)(units * DELAY_UNIT_US * 1e-6 * PLAY_HZ + 0.5);
    p->snd_us = snd_t(p) + (units & 0xFFFF) * DELAY_UNIT_US;
    unsigned n = (unsigned)(units & 0xFFFF) >> 1;
    if (n == 0) n = 1;
    uint32_t keep = play_ev[EV_RND];
    while (n--) rnd(p);
    play_ev[EV_RND] = keep;
}

/* random() (rt:2F0D): adds successive words of WAS.EXE's own code, 0-127 */
int rnd(Play *p)
{
    EV(EV_RND, 1);
    if (++p->rnd_ptr == 0x2328) p->rnd_ptr = 0xBB8;
    unsigned w = p->ga->code[p->rnd_ptr] | (p->ga->code[p->rnd_ptr + 1] << 8);
    p->rnd = (uint16_t)((w + p->rnd) & 0x7F);
    return p->rnd;
}

static int sar1(int v) { return v >= 0 ? v >> 1 : -((-v - 1) >> 1) - 1; }   /* 8086 SAR by 1 */

static int vis(const Play *p) { return p->rules[5]; }   /* DS:64B7 */

/* rt_0b3d: does the player command this squadron? */
bool commandable(const Play *p, int i)
{
    EV(EV_SMALL, 1);
    const PSquad *q = &p->sq[i];
    if (q->ctrl == -1) return true;
    return q->ctrl != 0 && q->side == -1;
}

/* rt_0630: the ships of squadron i into list[] (DS:6456), -1 after the last */
void list_ships(Play *p, int i)
{
    for (int k = 0; k < 3; k++) p->list[k] = -1;
    int n = 0;
    int s = 0;
    for (; s < MAX_SHIPS && n < 3; s++)
        if (p->ship[s].sq == i) p->list[n++] = s;
    EV(EV_LIST, s);
}

int play_cell(const Play *p, int i)   /* squadron_cell (rt:1985) */
{
    EV(EV_SMALL, 1);
    return ((p->sq[i].y + 2) / 10) * 10 + (p->sq[i].x >> 3);
}

static int sq_addr(const Play *p, int i) { return (p->sq[i].y + 1) * 80 + p->sq[i].x; }   /* DS:5F85 */

int next_slot(int i) { return i + 1 > PATH_SIZE - 1 ? 0 : i + 1; }   /* path_next_slot (rt:19BC) */

static int path_at(const Play *p, int i) { return i >= 0 && i < PATH_SIZE + ROUTE_SIZE ? p->path[i] : 0; }

/* ------------------------------------------------------------------ tiles (rt) */

int map_at(const Play *p, int cell) { return cell >= 0 && cell < NUM_CELLS ? p->map[cell] : 0; }

bool tile_land(int t)     /* tile_is_land (rt:14A6) */
{
    EV(EV_TILE, 1);
    return (t >= 3 && t <= 6) || t == 40 || t == 41 || t == 46 || t == 47 || (t >= 51 && t <= 77);
}
bool tile_rough(int t)    { EV(EV_TILE, 1); return (t >= 10 && t <= 12) || (t >= 16 && t <= 18); }   /* rt:15C4 */
bool tile_shallow(int t)  { EV(EV_TILE, 1); return (t >= 7 && t <= 9) || (t >= 13 && t <= 15); }     /* rt:1656 */
bool tile_sea(int t)      /* rt:12BD */
{
    EV(EV_TILE, 1);
    return t <= 2 || t == 7 || t == 12 || t == 13 || t == 19 || (t >= 37 && t <= 39) ||
           (t >= 42 && t <= 45) || (t >= 48 && t <= 50) || (t >= 78 && t <= 81) || t >= 83;
}
/* the four coast shapes (rt:16E8, 176C, 17F0, 18AC) */
static bool coast_a(int t) { return t == 8 || t == 10 || (t >= 31 && t <= 35); }
static bool coast_b(int t) { return t == 9 || t == 11 || (t >= 25 && t <= 28); }
static bool coast_c(int t) { return t == 14 || t == 16 || t == 23 || t == 24 || t == 29 || t == 30; }
static bool coast_d(int t) { return t == 15 || t == 17 || (t >= 19 && t <= 22) || t == 36; }

/* rt_2523: the shape of each cell's shore (DS:425D), and two values for the enemy */
static void coast_init(Play *p)
{
    p->water_value[0] = p->water_value[1] = 160;
    for (int c = 0; c < NUM_CELLS; c++) {
        int t = p->map[c];
        uint16_t v;
        if (coast_a(t)) v = 0x371;
        else if (coast_b(t)) v = 0x372;
        else if (coast_c(t)) v = 0x373;
        else if (coast_d(t)) v = 0x374;
        else if (t == 49 || t == 50) v = 0x376;
        else if (t == 42 || t == 48) v = 0x375;
        else if (t > 42 && t < 46) v = 0x378;
        else if (t > 36 && t < 40) v = 0x377;
        else if (tile_sea(t) || tile_rough(t) || tile_shallow(t)) v = 0;
        else v = 0x379;
        p->coast[c] = v;
        if (t == 74) p->water_value[0] >>= 1;
        if (t == 75) p->water_value[1] >>= 1;
    }
    if (p->water_value[0] < 10) p->water_value[0] = 10;
    if (p->water_value[1] < 5) p->water_value[1] = 5;
}

int dir_off(const Play *p, int d) { return ds_word(p, 0x784 + 2 * d); }   /* 1 E, 2 SE, 3 S ... 8 NE */

/* rt_1c52: is the neighbour in direction d off the map? */
bool off_map(int cell, int col, int d)
{
    static const int8_t DX[9] = { 0, 1, 1, 0, -1, -1, -1, 0, 1 };
    static const int8_t DY[9] = { 0, 0, 1, 1, 1, 0, -1, -1, -1 };
    EV(EV_HELP, 1);
    int row = cell / 10;
    return col + DX[d] < 0 || col + DX[d] > 9 || row + DY[d] < 0 || row + DY[d] > 19;
}

/* rt_1c0c: the shore of this cell does not face direction d all along (DS:0796) */
int coast_block(const Play *p, int cell, int d)
{
    EV(EV_HELP, 1);
    if (cell < 0 || cell >= NUM_CELLS) return 0;
    int v = p->coast[cell];
    if (v < 0x371 || v > 0x374) return 0;
    return p->ga->ds[0x796 + ((0x375 - v) << 3) + d - 1];
}

/* rt_1ef4: may a ship enter this cell from direction d? (0 yes) */
int coast_pass(const Play *p, int d, int cell)
{
    EV(EV_HELP, 1);
    int v = cell >= 0 && cell < NUM_CELLS ? p->coast[cell] : 0;
    if (v == 0) v = 0x370;
    if (v < 0x370 || v > 0x379 || d < 1 || d > 8) return 0;   /* only a damaged saved game has others */
    return p->ga->ds[0x796 + ((v - 0x370) << 3) + d - 1];
}

/* rt_1e10: no way from this cell in direction d */
bool way_blocked(const Play *p, int d, int cell, int col)
{
    EV(EV_HELP, 1);
    bool b = false;
    if (col == 9 && (d == 2 || d == 8)) b = true;
    if (col == 0 && (d == 4 || d == 6)) b = true;
    if (cell > 189 && (d == 2 || d == 4)) b = true;
    if (cell < 10 && (d == 6 || d == 8)) b = true;
    if (coast_block(p, cell, d)) b = true;
    int v = p->coast[cell];
    if ((v == 0x376 && d == 5) || (v == 0x375 && d == 1) || (v == 0x378 && d == 7) || (v == 0x377 && d == 3)) b = true;
    return b;
}

/* rt_1aa8 (repair ports, tile 74) and rt_19d6 (supply ports, 75): the port
 * beside the cell with the least (most = false) or the most left */
static int port_beside(const Play *p, int cell, int tile, bool most)
{
    int col = cell % 10, best = -1, have = most ? -1 : 99;
    EV(EV_PORT, 1);
    for (int d = 1; d <= 8; d += 2) {
        if (off_map(cell, col, d) || coast_block(p, cell, d)) continue;
        int n = dir_off(p, d) + cell;
        if (map_at(p, n) != tile) continue;
        if ((!most && p->port_sup[n] < have) || (most && p->port_sup[n] > have)) { best = n; have = p->port_sup[n]; }
    }
    return best;
}
int repair_port(const Play *p, int cell, bool most) { return port_beside(p, cell, 74, most); }
int supply_port(const Play *p, int cell, bool most) { return port_beside(p, cell, 75, most); }

/* rt_1b7d: the crown beside the cell: side -1 looks for tile 76 (the enemy's
 * crown), side 0 for 77 (the player's) */
int crown_beside(const Play *p, int cell, int side)
{
    int tile = side ? 76 : 77, col = cell % 10;
    EV(EV_PORT, 1);
    for (int d = 1; d <= 8; d += 2) {
        if (off_map(cell, col, d) || coast_block(p, cell, d)) continue;
        int n = dir_off(p, d) + cell;
        if (map_at(p, n) == tile) return n;
    }
    return -1;
}

/* ovl3_8146: the map maker's check of a map (see play.h). Work arrays as
 * WAS.EXE's: the land marks (43ED), the way each cell was come into (3F3D)
 * and the stack of cells still to go on from (457D). */
int play_map_check(const GameAssets *ga, const uint8_t map[NUM_CELLS], long *steps)
{
    Play *p = SDL_calloc(1, sizeof *p);
    if (!p) return 1;
    uint32_t keep[EV_COUNT];
    SDL_memcpy(keep, play_ev, sizeof keep);
    p->ga = ga;
    SDL_memcpy(p->map, map, NUM_CELLS);
    bool land[NUM_CELLS];
    int16_t came[NUM_CELLS], stack[NUM_CELLS * 8 + 8];
    long n = 0;
    for (int c = 0; c < NUM_CELLS; c++) land[c] = tile_land(map[c]);
    coast_init(p);
    int result = 1;
    for (int c = 0; c < NUM_CELLS && result == 1; c++) {
        n++;
        if (land[c]) continue;
        if (repair_port(p, c, false) == -1 && supply_port(p, c, false) == -1 &&
            crown_beside(p, c, 0) == -1 && crown_beside(p, c, -1) == -1) continue;
        int col = c % 10;
        if (c < 10 || c > 189 || col == 0 || col == 9) continue;   /* at the edge already */
        SDL_memset(came, 0, sizeof came);
        came[c] = 9;
        stack[0] = (int16_t)c;
        int top = 1;
        bool out = false;
        while (!out) {
            if (--top < 0) { result = 0; break; }
            int at = stack[top], acol = at % 10;
            for (int d = 1; d <= 8; d++) {
                n++;
                if (!way_blocked(p, d, at, acol)) {
                    bool edge = (at < 10 && d > 5) || (at > 189 && d < 5 && d > 1) ||
                                (acol == 0 && d > 3 && d < 7) || (acol == 9 && (d == 8 || d == 1 || d == 2));
                    if (edge) {
                        if (at != c) { out = true; break; }
                    } else {
                        int to = dir_off(p, d) + at;
                        if (to >= 0 && to < NUM_CELLS && !land[to] && !coast_pass(p, d, to) && came[to] == 0 &&
                            top < (int)SDL_arraysize(stack)) {
                            came[to] = (int16_t)d;
                            stack[top++] = (int16_t)to;
                        }
                    }
                }
                if (at == c) d++;                  /* from the port's own cell: only N, E, S, W */
            }
        }
    }
    for (int c = 0; c < NUM_CELLS && result == 1; c++) {     /* every port and crown beside water */
        int t = map[c];
        if (t < 74 || t > 77) continue;
        int col = c % 10, row = c / 10, b;
        n++;
        b = c - 10;
        if (row != 0 && !land[b] && !coast_c(map[b]) && !coast_d(map[b])) continue;
        b = c + 10;
        if (row != 19 && !land[b] && !coast_a(map[b]) && !coast_b(map[b])) continue;
        b = c + 1;
        if (col != 9 && !land[b] && !coast_a(map[b]) && !coast_c(map[b])) continue;
        b = c - 1;
        if (col != 0 && !land[b] && !coast_b(map[b]) && !coast_d(map[b])) continue;
        result = -1;
    }
    SDL_memcpy(play_ev, keep, sizeof keep);
    SDL_free(p);
    if (steps) *steps = n;
    return result;
}

/* port_of_cell (ovl4:C268): the port or crown a squadron at this cell uses */
static int port_of_cell(const Play *p, int cell)
{
    int r = repair_port(p, cell, false), s = supply_port(p, cell, false);
    if (r == -1 && s == -1) {
        s = crown_beside(p, cell, 0);
        if (s == -1) s = crown_beside(p, cell, -1);
    } else if (r != -1 && s != -1) {
        if (p->port_sup[r] * 2 < p->port_sup[s]) s = r;
    } else if (r != -1) s = r;
    return s;
}

/* ovl4_c35c: invert a tile (rt_2e61), a port with nothing left */
void invert(Play *p, int cell)
{
    if (cell >= 0 && cell < NUM_CELLS) p->inverted[cell] = !p->inverted[cell];
}

/* the tile shown: the wind's face over tile 73 (ovl4_c658, rt_2a2e) */
static int shown_tile(const Play *p, int cell)
{
    int t = p->map[cell];
    if (t == 73 && p->wind_on) {
        static const uint8_t WIND_TILE[4] = { 94, 92, 91, 93 };   /* DS:67A8 N, 6660 E, 65BC S, 6704 W */
        t = WIND_TILE[p->wind & 3];
    }
    return t;
}

/* a byte of WAS.EXE's map buffer (BUF): addr = line pair * 80 + byte, + 16000
 * for the odd line. From the CGA tiles; the squadrons drawn in it are left out. */
static int buf_byte(const Play *p, int addr)
{
    int field = 0;
    if (addr >= 16000) { addr -= 16000; field = 1; }
    if (addr < 0 || addr >= 16000) return 0;
    int line = (addr / 80) * 2 + field, xb = addr % 80;
    int cell = (line / TILE_H) * 10 + xb / 8;
    int t = shown_tile(p, cell) - 1;
    const Image *im = t >= 0 && t < NUM_TILES ? &p->ga->set[MODE_CGA].tiles[t] : NULL;
    int b = 0;
    if (im && im->px) {
        const uint8_t *px = &im->px[(line % TILE_H) * TILE_W + (xb % 8) * 4];
        b = (px[0] & 3) << 6 | (px[1] & 3) << 4 | (px[2] & 3) << 2 | (px[3] & 3);
    }
    return p->inverted[cell] ? b ^ 0xFF : b;
}
static bool land_byte(int b) { return b == 0x44 || b == 0x11; }   /* rt_2e48(addr, 68 or 17) */

/* ovl4_aeae, af08: the middle of a cell, with the shore's slant */
int cell_mid_x(const Play *p, int cell)
{
    int v = p->coast[cell], a = 0;
    if (v == 0x371 || v == 0x373) a = 1;
    else if (v == 0x372 || v == 0x374) a = -1;
    return (cell % 10) * 8 + 3 + a;
}
int cell_mid_y(const Play *p, int cell)
{
    int v = p->coast[cell], a = 0;
    if (v == 0x374 || v == 0x373) a = -1;
    else if (v == 0x372 || v == 0x371) a = 1;
    return (cell / 10) * 10 + 2 + a;
}

/* ovl4_9a3c: the heading (0-15, N clockwise) of a step; another value is returned as it is */
int heading_of(int step)
{
    switch (step) {
    case -80: return 0;   case -79: return 3;   case 1: return 4;    case 81: return 5;
    case 80: return 8;    case 79: return 11;   case -1: return 12;  case -81: return 13;
    default: return step;
    }
}

/* ------------------------------------------------------------------ text area */

static void tpos(Play *p, int row, int col) { p->trow = row; p->tcol = col; }   /* goto_xy */

static void clear_row(Play *p, int row)    /* clear_text_row */
{
    if (row < 0 || row >= 25) return;
    SDL_memset(p->text[row], ' ', 40);
    p->text[row][40] = 0;
    p->icon[row][0].off = p->icon[row][1].off = 0;
    tpos(p, row, 0);
}

/* clear_text_row over the map: the row is blacked out (the offer) */
static void clear_row_over(Play *p, int row)
{
    clear_row(p, row);
    if (row >= 0 && row < 23) SDL_memset(p->panel[row], 1, 40);
}

static void print(Play *p, const char *s)  /* print_string / put_char */
{
    for (; *s; s++) {
        if (p->trow >= 0 && p->trow < 25 && p->tcol < 40) {
            p->text[p->trow][p->tcol] = *s;
            if (p->trow < 23) p->panel[p->trow][p->tcol] = 1;   /* over the map */
        }
        if (++p->tcol >= 40) { p->tcol = 0; p->trow++; }
    }
}

static void print_num(Play *p, int n)       /* print_number */
{
    char b[16];
    SDL_snprintf(b, sizeof b, "%d", n);
    print(p, b);
}

static void spaces(Play *p, int n) { while (n-- > 0) print(p, " "); }   /* print_spaces */

/* rt_2cc4 on a text row: an icon from DS (width in bytes, height / 2, rows) */
static void row_icon(Play *p, int row, int xbyte, unsigned off)
{
    if (row < 0 || row >= 25) return;
    int k = p->icon[row][0].off ? 1 : 0;
    p->icon[row][k].off = (uint16_t)off;
    p->icon[row][k].x = (int8_t)xbyte;
}

/* text_rows_add (rt:0BC2): grow or shrink the text area below the map by n rows */
static void text_rows_add(Play *p, int n)
{
    int old = p->text_rows;
    p->text_rows = SDL_clamp(p->text_rows + n, 2, 25);
    if (p->text_rows < 3) p->msg_rows = 0;
    for (int r = 25 - p->text_rows; r < 25 - old; r++) clear_row(p, r);
}

/* text_area_reset (rt:0C10): back to two text rows, row 23 cleared */
static void text_reset(Play *p)
{
    if (p->text_rows > 2) text_rows_add(p, 2 - p->text_rows);
    clear_row(p, 23);
}

/* The map is put back over Info's panel (the original's next blit) */
static void clear_panel(Play *p)
{
    SDL_memset(p->panel, 0, sizeof p->panel);
}

int play_map_h(const Play *p) { return SCREEN_H - 8 * p->text_rows - 2; }

/* ------------------------------------------------------------------ command lines */

/* show_time_setting (ovl4:AAA8) */
static void show_time(Play *p)
{
    tpos(p, 24, 6);
    print(p, DS(p, p->time_div == 1 ? 0x214 : p->time_div == 4 ? 0x21B : p->time_div == 16 ? 0x220 : 0x227));
    print(p, DS(p, 0x22C));                 /* ")  " */
}

/* ovl4:AB00: "Clear" when there are messages, else "Pause" */
static void show_clear(Play *p)
{
    if (p->mode != PM_NORMAL || p->demo) return;
    tpos(p, 24, 22);
    print(p, DS(p, p->msg_rows ? 0x230 : 0x236));
}

/* show_command_line (ovl4:AA1C): Time (slow)  Fleet  Pause  Surrender */
static void cmd_normal(Play *p)
{
    if (p->mode == PM_PROMPT) return;
    text_reset(p);
    clear_row(p, 24);
    p->mode = PM_NORMAL;
    if (p->demo) {                           /* ovl4:AA3C */
        print(p, DS(p, 0x1EC));             /* Press any key to end demo. */
        tpos(p, 23, 5);
        print(p, DS(p, 0xEE4));             /* THE ANCIENT ART OF WAR AT SEA */
        return;
    }
    print(p, DS(p, 0x207));                 /* "Time (" */
    show_time(p);
    tpos(p, 24, 15);
    print(p, DS(p, 0x20E));                 /* "Fleet" */
    show_clear(p);
    tpos(p, 24, 29);
    print(p, SO5(p, 0x2BF));                /* "Surrender" */
}

/* ovl4:B252: a squadron of the same side close enough to join (index + 1, or 0).
 * A squadron of three ships cannot join another. */
int join_partner(Play *p, int a)
{
    list_ships(p, a);
    if (p->sq[a].partner || p->list[2] != -1) return 0;
    for (int i = 0; i < MAX_SHIPS; i++) {
        const PSquad *q = &p->sq[i];
        if (!q->used || q->side != p->sq[a].side || q->mark == 2 || i == a || q->partner) continue;
        if (SDL_abs(q->x - p->sq[a].x) < 4 && SDL_abs(q->y - p->sq[a].y) < 8) return i + 1;
    }
    return 0;
}

/* ovl4:AB82: Move Info Detach Join Zoom Practice */
static void cmd_squad_line(Play *p)
{
    const PSquad *q = &p->sq[p->sel];
    int pad = 34;
    clear_row(p, 24);
    if (q->side == -1 && !q->partner) { print(p, DS(p, 0x23C)); pad -= 5; }   /* "Move " */
    print(p, DS(p, 0x242));                                                  /* "Info " */
    if (q->side == -1 && !q->partner) {
        list_ships(p, p->sel);
        if (p->list[1] != -1 && p->free_slot > -1) { print(p, SO5(p, 0x2B7)); pad -= 7; }   /* "Detach " */
        if (join_partner(p, p->sel)) { print(p, DS(p, 0x248)); pad -= 5; }                 /* "Join " */
    }
    if (q->partner && q->ctrl != -1 && q->ctrl != 0) { print(p, DS(p, 0x24E)); pad -= 4; }   /* "Zoom" */
    if (!q->partner && q->ctrl == -2 && commandable(p, p->sel)) { print(p, DS(p, 0x253)); pad -= 8; }   /* "Practice" */
    spaces(p, pad);
    p->mode = PM_SQUAD;
}

/* ------------------------------------------------------------------ squadrons */

static void mark_flagships(Play *p);
static void encounters(Play *p);

/* select_squad (ovl4:AB48): the squadron's command line, then ovl4_8bcd */
static void cmd_squad(Play *p)
{
    mark_flagships(p);
    text_reset(p);
    p->free_slot = MAX_SHIPS - 1;            /* the last empty slot, for Detach */
    while (p->free_slot > -1 && p->sq[p->free_slot].used) p->free_slot--;
    cmd_squad_line(p);
    encounters(p);
}

/* rt_0f55: forget a squadron's path and route */
void clear_paths(Play *p, int i)
{
    PSquad *q = &p->sq[i];
    if (q->has_path) {
        int k = q->path_pos;
        do {
            if (k >= 0 && k < PATH_SIZE) p->path[k] = 0;
            k = next_slot(k);
        } while (p->path[k] != 0);
        q->dest = NO_DEST;
        q->has_path = false;
    }
    if (q->has_route) {
        int k = q->route_pos;
        for (;;) {
            if (k >= 0 && k < ROUTE_SIZE) p->path[PATH_SIZE + k] = 0;
            k++;
            if (k > ROUTE_SIZE - 1 || p->path[PATH_SIZE + k] == 0) break;
        }
        q->has_route = false;
    }
    if (q->used) { q->move_delay = 0; q->speed = 0; }
}

/* mark_flagship_squadrons (rt:0688): the pennant over a squadron with a
 * flagship; a squadron left with no ships is removed, and an empty slot that
 * has ships becomes a squadron (speed 0). Merchants (2) are left alone. */
static void mark_flagships(Play *p)
{
    for (int i = 0; i < MAX_SHIPS; i++) {
        PSquad *q = &p->sq[i];
        if (q->mark == 2) continue;
        list_ships(p, i);
        q->mark = 0;
        if (p->list[0] == -1) {
            clear_paths(p, i);
            q->used = false;
            continue;
        }
        for (int k = 0; k < 3 && p->list[k] != -1; k++)
            if (p->ship[p->list[k]].type == 3) q->mark = 1;
        if (!q->used) { q->used = true; q->speed = 0; }
    }
}

/* draw_squadrons (rt:10D2): who is shown */
static bool visible(const Play *p, int i)
{
    const PSquad *q = &p->sq[i];
    return q->used && (q->side != 0 || vis(p) == 80 || q->seen);
}

/* the work of erase_squadrons (rt:104A) and redraw_squadrons (rt:1084):
 * every squadron shown is put back and drawn again (the remake draws the
 * whole screen every frame; this is for the timing: about 3,350 + 500 a
 * squadron instructions, measured under the emulator) */
void redraw_work(const Play *p)
{
    EV(EV_BASE, 3350);
    for (int i = 0; i < MAX_SHIPS; i++) if (visible(p, i)) EV(EV_BASE, 500);
}

static bool waiting(const Play *p)   /* time stands still in these */
{
    return p->mode == PM_PROMPT || p->mode == PM_END || p->mode == PM_OFFER || p->mode == PM_DETACH;
}

/* ovl4:A202: is the marker over a squadron? It is when it is in the
 * squadron's byte column, from 2 line pairs above to 2 below its map
 * address (y + 1). */
static void check_over(Play *p, int key)
{
    bool scroll = key == PK_MINUS || key == PK_KP_MINUS || key == PK_TAB ||
                  key == PK_EQUALS || key == PK_KP_PLUS || key == PK_CTRL;
    if (scroll && p->mode != PM_SQUAD) return;
    if (p->mode == PM_TRACE || waiting(p)) return;
    bool found = false;
    p->sel = -1;
    for (int i = 0; i < MAX_SHIPS; i++) {
        const PSquad *q = &p->sq[i];
        if (!q->used || q->mark == 2) continue;
        if (vis(p) < 80 && !commandable(p, i) && !q->seen) continue;
        if (p->cx != q->x || p->cy < q->y - 1 || p->cy > q->y + 3) continue;
        found = true;                        /* the last one found wins, unless one is in an encounter */
        p->sel = i;
        if (!p->over_squad) { buzz(p); cmd_squad(p); }   /* rt_0c72: the buzz, select_squad */
        if (q->partner) break;
    }
    if (!found && p->over_squad) cmd_normal(p);
    p->over_squad = found;
}

/* ------------------------------------------------------------------ messages */

/* ovl4_bbbc: a message row: up to two above the command lines, then they scroll */
static void msg_row(Play *p)
{
    if (p->msg_rows < 2) {
        text_rows_add(p, 1);
        p->msg_rows++;
    } else {                                 /* scroll_text_area (rt:2C09): row 21 to 22 */
        SDL_memcpy(p->text[22], p->text[21], sizeof p->text[22]);
        clear_row(p, 21);
    }
    tpos(p, 25 - p->text_rows, 0);
}

static int region_code(int x, int y)        /* map_region_code (ovl4:BBEB) */
{
    int r = 0;
    if (y < 60) r--;
    else if (y > 140) r++;
    if (x < 30) r -= 2;
    else if (x > 50) r += 2;
    return r;
}

/* print_map_region (ovl4:BC2D): " IN THE NORTHWEST", then a pause */
static void print_region(Play *p, int i)
{
    int x = p->sq[i].x, y = p->sq[i].y;
    beep_message(p);
    print(p, SO5(p, 0x29D));
    bool ns = false;
    if (y < 60) { print(p, DS(p, 0x2BC)); ns = true; if (p->demo) p->demo_key = PK_KP_MINUS; }   /* ovl4:BC5B */
    else if (y > 140) { print(p, DS(p, 0x2C2)); ns = true; if (p->demo) p->demo_key = PK_KP_PLUS; }
    if (x < 30) print(p, DS(p, 0x2C8));
    else if (x > 50) print(p, DS(p, 0x2CD));
    else if (!ns) print(p, SO5(p, 0x2A6));   /* INTERIOR */
    show_clear(p);
    delay(p, 5000);
}

static void msg_sighted(Play *p, int i)     /* ovl4_b612: ALERT - ENEMY SIGHTED */
{
    int code = region_code(p->sq[i].x, p->sq[i].y) + 5;
    if (p->msg_last == code || p->demo) return;   /* ovl4:B644: not in the demo */
    p->msg_last = code;
    p->msg_timer = 20;
    msg_row(p);
    print(p, SO5(p, 234));
    print_region(p, i);
}

static void msg_encounter(Play *p, int i)   /* ovl4:B68C: WARNING - ENCOUNTER */
{
    p->msg_last = 0;
    msg_row(p);
    print(p, SO5(p, 0x100));
    print_region(p, i);
    if (p->mode == PM_SQUAD) cmd_squad_line(p);
    if (p->demo) { p->demo = 2; p->sel = i; }   /* ovl4:B6CB: the demo zooms in next */
}

static void msg_merchant(Play *p, int i)    /* ovl4_b6e5: a merchant taken */
{
    if (p->msg_last == 99 && p->sq[i].side == -1) return;
    if (p->msg_last == 98 && p->sq[i].side == 0) return;
    msg_row(p);
    int code;
    if (p->sq[i].side == -1) {
        print(p, SO5(p, 0x2C9));             /* NOTICE - MERCHANT LOST */
        if (rnd(p) < OPP(p, 0x606D) << 3) {
            p->crown_threat = play_cell(p, i);
            p->merchant_lost = 1;
            LOGN("ENEMY learns from the lost merchant where to strike: %s becomes its standing target (chance 606D x 8 = %d/128)",
                 CELLN(p->crown_threat), OPP(p, 0x606D) << 3);
        }
        code = 99;
    } else {
        code = 98;
        print(p, SO5(p, 0x2E0));             /* ENEMY MERCHANT CAPTURED */
    }
    print_region(p, i);
    p->msg_last = code;
    p->msg_timer = 20;
}

static void msg_resupplied(Play *p, int i)  /* ovl4_b7b0: MERCHANT SUPPLIES SQUAD */
{
    p->msg_last = 0;
    msg_row(p);
    print(p, DS(p, 0x27F));
    print_region(p, i);
}

static void msg_fighting(Play *p, int i)    /* ovl4:B7E4: A L E R T - FIGHTING (yours only) */
{
    if (p->sq[i].side == 0) return;
    p->msg_last = 0;
    msg_row(p);
    print(p, SO5(p, 0x114));
    print_region(p, i);
    if (p->mode == PM_SQUAD) cmd_squad_line(p);
}

static void msg_battle(Play *p, int i, bool won)   /* ovl4:B83B: NOTICE - BATTLE WON / LOST */
{
    p->msg_last = 0;
    msg_row(p);
    print(p, SO5(p, 0x12A));
    print(p, DS(p, won ? 0x297 : 0x29B));
    print_region(p, i);
}

static void msg_flag(Play *p, int i, bool yours)   /* ovl4_b88b, b8f6: URGENT - FLAG TAKEN / ENEMY FLAG CAPTURED */
{
    int code = region_code(p->sq[i].x, p->sq[i].y) + 10;
    if (p->msg_last == code) return;
    p->msg_last = code;
    msg_row(p);
    print(p, SO5(p, yours ? 0x147 : 0x15B));
    print_region(p, i);
}

/* ovl4_b590: a ship lost in rough or shallow water (no region, no pause) */
static void msg_sunk(Play *p, int ship, int tile, int i)
{
    p->msg_last = 0;
    beep_message(p);
    msg_row(p);
    if (p->sq[i].side == -1) print(p, p->c->ships[ship].name);
    else print(p, DS(p, tile_rough(tile) ? 0x266 : 0x274));   /* ENEMY FRIGATE / ENEMY SHIP */
    print(p, " ");
    print(p, tile_rough(tile) ? SO5(p, 212) : SO5(p, 0x244));   /* SUNK IN ROUGH WATERS / LOST IN SHALLOW WATER */
}

/* ovl4_b961: WHITE / BLACK CROWN CAPTURED. With the sound on the crown
 * flashes to a tune (step 8); it is left inverted either way. */
static void msg_crown(Play *p, int i, int side)
{
    msg_row(p);
    print(p, DS(p, side == 0 ? 0x2A0 : 0x2A6));
    print(p, DS(p, 0x2AC));
    print_region(p, i);
    int port = port_of_cell(p, play_cell(p, i));
    invert(p, port);
    if (assets_scnf_word(p->ga, 0x684F) == 2) {      /* the tune, the crown flashing (ovl4_bab0: delay 600, invert) */
        static const int16_t TUNE[] = { 0xBDC, 0x708, 17, -1, 0xE16, 0x384, 17, 0x4B0, 0xE16, -1, 17, 0x4B0,
                                        0xD50, -1, 17, 0x4B0, 0xE16, 0x7D0 };
        pspk_on(p);
        for (int k = 0; k < (int)SDL_arraysize(TUNE); k += 2) {
            pspk_freq(p, TUNE[k]);
            if (TUNE[k + 1] >= 0) delay(p, TUNE[k + 1]);
            else { delay(p, 0x258); invert(p, port); }
        }
        pspk_off(p);
        for (int k = 0; k < 3; k++) { invert(p, port); delay(p, 0x1388); }
    }
}

/* ------------------------------------------------------------------ the wind */

/* ovl4_c658: a new wind (start = false) or the wind shown at the start. The
 * face over tile 73 shows it (shown_tile). */
static void wind_change(Play *p, bool start)
{
    if (!p->wind_on) return;
    if (!start) {
        int old = p->wind;
        do p->wind = rnd(p) & 7; while (p->wind > 3);
        if (p->wind == old) return;
        LOGN("EVENT the wind now blows from the %s", p->wind == 0 ? "North" : p->wind == 1 ? "East" : p->wind == 2 ? "South" : "West");
        p->msg_last = 0;
        if (p->demo) return;                 /* ovl4:C69A: no message in the demo */
        msg_row(p);
        print(p, DS(p, 0x2D2));              /* Wind is from the */
        beep_message(p);
        static const unsigned NAME[4] = { 0x2E4, 0x2F0, 0x2EA, 0x2F5 };   /* North, East, South, West */
        print(p, DS(p, NAME[p->wind]));
        show_clear(p);
    }
}

/* ------------------------------------------------------------------ the end */

/* ovl4_a129: "You Lost" (1) or "You Won" (0, 2) blinks until a key; all the
 * enemy's squadrons are shown. Then Enter (or the key that ended it, if it
 * was Enter) leaves. */
static void end_game(Play *p, int how)
{
    if (p->mode == PM_END || p->ended) return;
    LOGN("EVENT the game is over: you %s", how == 1 ? "lost" : "won");
    end_sound(p);                            /* play_end_sound */
    p->rules[5] = 80;
    text_rows_add(p, 0);
    p->mode = PM_END;
    p->end_won = how;
    p->end_frame = 0;
    p->end_key = 0;
    p->outcome = how == 1 ? PLAY_LOST : PLAY_WON;
    clear_row(p, 23);
}

/* ovl4_c3c9: no flagships left on a side (that began with one): 1 yours, 2 theirs */
static int flags_check(Play *p)
{
    int yours = 0, theirs = 0;
    for (int i = 0; i < MAX_SHIPS; i++) {
        if (p->sq[i].mark != 1 || !p->sq[i].used) continue;
        if (commandable(p, i)) yours++;
        else theirs++;
    }
    if (yours == 0 && !p->no_flags[0]) return 1;
    if (theirs == 0 && !p->no_flags[1]) return 2;
    return 0;
}

/* ovl4_d549: squadrons of a side at a cell (anchored ones only, unless any) */
int count_at(const Play *p, int cell, int side, bool any)
{
    int n = 0;
    EV(EV_S40, MAX_SHIPS);
    for (int i = 0; i < MAX_SHIPS; i++) {
        const PSquad *q = &p->sq[i];
        if (!q->used || q->mark == 2) continue;
        if (q->speed != 0 && !any) continue;
        if (q->side == side && play_cell(p, i) == cell) n++;
    }
    return n;
}

/* ovl4_c44e: a squadron beside the other side's crown takes it, unless a
 * squadron of that side is in the same cell. The game ends. */
static bool crown_check(Play *p, int i)
{
    PSquad *q = &p->sq[i];
    if (q->mark == 2) return false;
    int other = q->side == -1 ? 0 : -1;
    int cell = play_cell(p, i);
    if (crown_beside(p, cell, q->side) == -1) return false;
    if (count_at(p, cell, other, true)) return false;
    text_rows_add(p, 0);
    LOGN("EVENT %s takes %s crown beside it", SQN(i), q->side == -1 ? "the enemy's" : "your");
    msg_crown(p, i, q->side);
    if (q->side == -1) { p->result_code = 3; end_game(p, 0); }
    else { p->result_code = 13; end_game(p, 1); }
    return true;
}

/* ovl4_9e4f: after a squadron is lost: the counts, the end of the game, and
 * the opponent's offer to surrender. a0 and a1 are the squadrons involved. */
static int end_check(Play *p, int a0, int a1)
{
    int theirs = 0, yours = 0, surv = a0;
    for (int i = 0; i < MAX_SHIPS; i++) {
        PSquad *q = &p->sq[i];
        if (!q->used) {
            if (i == a0) surv = a1;
            clear_paths(p, i);
            q->ctrl = -2;
        } else if (commandable(p, i)) yours++;
        else theirs++;
        if ((i == a0 || i == a1) && q->has_path) q->speed = q->port_speed & 255;
    }
    /* the opponent's values (step 5 uses them) */
    int16_t was_opp[OPP_WORDS];
    SDL_memcpy(was_opp, p->opp, sizeof was_opp);
    if (yours < 3 || (theirs >> 2) > yours || theirs < 3) { OPP(p, 0x6057) = 110; OPP(p, 0x6073) = 1; }
    if (OPP(p, 0x6073) * p->crowns + 2 > theirs) {
        OPP(p, 0x6073) = (int16_t)(p->crowns ? (theirs - 2) / p->crowns : 0);
        if (OPP(p, 0x6073) <= 0) {
            OPP(p, 0x6065) = 90; OPP(p, 0x6067) = 120; OPP(p, 0x6071) = 0; OPP(p, 0x6073) = 1;
        }
    }
    if (LOGGING && SDL_memcmp(was_opp, p->opp, sizeof was_opp) != 0) {
        char t[200] = "";
        size_t n = 0;
        for (int k = 0; k < OPP_WORDS && n < sizeof t - 24; k++)
            if (was_opp[k] != p->opp[k]) n += (size_t)SDL_snprintf(t + n, sizeof t - n, " %04X %d->%d", 0x604D + 2 * k, was_opp[k], p->opp[k]);
        LOGN("ENEMY changes its plans: your squadrons %d, its %d, crowns %d:%s (6057 forgetting/staying, 6073 crown guard, "
             "6065 attack margin, 6071 caution)", yours, theirs, p->crowns, t);
    }
    if (a0 != a1 && surv >= 0 && crown_check(p, surv)) return 0;
    mark_flagships(p);
    if (yours == 0) { end_game(p, 1); p->result_code = p->zoomed ? 14 : 12; return 0; }
    if (theirs == 0) { end_game(p, 2); p->result_code = p->zoomed ? 4 : 2; return 0; }
    int f = flags_check(p);
    if (f == 1) { end_game(p, 1); p->result_code = 10; return 0; }
    if (f == 2) { end_game(p, 2); p->result_code = 0; return 0; }
    p->zoomed = false;
    if (yours / theirs >= OPP(p, 0x605B) && yours > 6 && p->mode != PM_END && p->mode != PM_OFFER) {
        LOGN("ENEMY offers to surrender: your %d squadrons against its %d (ratio %d at least 605B %d, and you more than 6)",
             yours, theirs, yours / theirs, OPP(p, 0x605B));
        OPP(p, 0x6057) = 110; OPP(p, 0x605B) = 500; OPP(p, 0x6073) = 1;
        beep_message(p);
        clear_row_over(p, 3);
        clear_row_over(p, 6);
        clear_row_over(p, 4);
        print(p, SO5(p, 110));               /* Your opponent offers to surrender. */
        clear_row_over(p, 5);
        print(p, SO5(p, 148));               /* Do you accept? (Y or N) */
        p->offer_ends = !p->in_encounters;
        p->back_mode = p->mode;
        p->mode = PM_OFFER;
    }
    return 0;
}

/* ------------------------------------------------------------------ fights on the map */

/* ovl4_92b7: count a lost ship. way != 0: by side; 0: by who commands the squadron */
static void count_loss(Play *p, int ship, int i, int way)
{
    bool yours = way ? p->sq[i].side == -1 : p->sq[i].ctrl == -1;
    p->losses[yours ? 0 : 1]++;
    if (p->ship[ship].type == 3) p->losses[yours ? 2 : 3]++;
}

/* ovl4_9154: squadron a0 may be hit. True when its last ship is lost. */
static bool hit(Play *p, int a0, int way)
{
    list_ships(p, a0);
    int chance = commandable(p, a0) ? 65 : OPP(p, 0x6053);
    if (crown_beside(p, a0, p->sq[a0].side) != -1) chance -= 10;   /* as WAS.EXE: the squadron's number as the cell */
    if (rnd(p) >= chance) return false;
    if (p->list[0] == -1) return false;
    int k, s;
    do { k = rnd(p) / 43; s = p->list[k]; } while (s == -1);
    PShip *sh = &p->ship[s];
    if (sh->type == 1 && rnd(p) < 64) return false;
    if (sh->type == 3 && rnd(p) < 90) return false;
    if (sh->sails > 4 || rnd(p) < 66) sh->hull++;
    else if (sh->sails < 5) sh->sails++;
    LOGN("EVENT fight on the map: %s of %s squadron %d is hit (sails %d, hull %d; chance %d/128 x 40/128 an iteration)",
         gamelog_ship(p, s), p->sq[a0].ctrl == -1 ? "your" : "the enemy's", a0, sh->sails, sh->hull, chance);
    if (sh->hull <= 4) return false;
    LOGN("EVENT %s is sunk in the fight on the map", gamelog_ship(p, s));
    if (sh->type == 3) msg_flag(p, a0, p->sq[a0].ctrl == -1);
    count_loss(p, s, a0, way);
    sh->sq = -1;
    return k == 0 && p->list[1] == -1;
}

/* ovl4_910f: the enemy's squadrons */
static int enemy_count(const Play *p)
{
    int n = 0;
    EV(EV_S40, MAX_SHIPS);
    for (int i = 0; i < MAX_SHIPS; i++) if (p->sq[i].used && p->sq[i].side == 0) n++;
    return n;
}

/* ovl4_8ede (every iteration): an encounter not zoomed into counts up to the
 * fight delay (faster with many enemies or a fast clock), then the two fight
 * it out on the map: each iteration the crossed swords change colour and a
 * squadron may be hit, until one has no ships left. */
static int map_battles(Play *p)
{
    if (p->mode == PM_PROMPT) return 0;
    for (int i = 0; i < MAX_SHIPS; i++) {
        PSquad *q = &p->sq[i];
        EV(EV_S40, 1);
        if (!q->used || q->ctrl == -2) continue;
        int other = q->partner - 1;
        if (q->ctrl > 0 && q->ctrl < p->fight_delay) {
            q->ctrl++;
            if (q->ctrl < p->fight_delay - 6) {
                int n = enemy_count(p);
                if (n > 5) q->ctrl++;
                if (p->time_div < 16) {
                    q->ctrl++;
                    if (n > 10) { q->ctrl++; if (p->time_div == 1) q->ctrl++; }
                    if (n > 15) { q->ctrl++; if (p->time_div == 1) q->ctrl++; }
                }
            }
        } else if (q->ctrl == p->fight_delay) {
            LOGN("EVENT %s fights it out on the map with squadron %d (the fight delay %d is up)", SQN(i), other, p->fight_delay);
            q->ctrl = q->side;
            msg_fighting(p, i);
        } else {
            q->side = q->side == 0 ? -1 : 0;
        }
        if (q->ctrl != -1 && q->ctrl != 0) continue;
        if (other < 0 || other >= MAX_SHIPS) continue;
        if (rnd(p) >= 40) continue;
        if (!hit(p, other, 0)) continue;
        PSquad *b = &p->sq[other];
        LOGN("EVENT squadron %d wins the fight on the map: squadron %d has no ships left", i, other);
        b->side = b->ctrl;
        if (b->ctrl == 0 && rnd(p) < OPP(p, 0x6057) && play_cell(p, other) == p->crown_threat) {
            p->crown_threat = -1;
            LOGN("ENEMY forgets its standing target after the win there (chance 6057 %d/128)", OPP(p, 0x6057));
        }
        q->side = q->ctrl;
        msg_battle(p, other, q->side != 0);
        b->partner = 0;
        q->partner = 0;
        mark_flagships(p);
        int r = end_check(p, i, other);
        if (r) return r;
        b->ctrl = -2;
        q->ctrl = -2;
        check_over(p, 0);
    }
    return 0;
}

/* ------------------------------------------------------------------ meeting at sea */

/* ovl4_c5b9: close enough for an encounter? Sets the distance (DS:689A). */
bool close_to(Play *p, int a, int b)
{
    int dx = SDL_abs(p->sq[a].x - p->sq[b].x), dy = SDL_abs(p->sq[a].y - p->sq[b].y);
    EV(EV_SMALL, 1);
    p->dist = dx * 2 + dy;
    return dx < 3 && dy < 5;
}

/* ovl4_bacd: warship w takes merchant m and its cargo */
static void take_merchant(Play *p, int m, int w)
{
    LOGN("EVENT %s takes %s and its cargo (supplies %d + 10, at most 20)", SQN(w), SQN(m), p->sq[w].supplies);
    msg_merchant(p, m);
    p->sq[w].supplies += 10;
    if (p->sq[w].supplies > 20) p->sq[w].supplies = 20;
    p->sq[m].mark = 0;
    p->sq[m].used = false;
    end_check(p, m, w);
}

/* ovl4_bb29: a merchant meets a warship of its own side and fills its holds;
 * it is used up if the warship had less than half */
static void meet_merchant(Play *p, int a, int b)
{
    int w = a, m = b;
    if (p->sq[a].mark == 2) { w = b; m = a; }
    if (p->sq[w].supplies < 20)
        LOGN("EVENT %s meets %s: supplies %d -> 20%s", SQN(m), SQN(w), p->sq[w].supplies,
             p->sq[w].supplies < 10 ? "; the merchant is used up" : "");
    if (p->sq[w].side == -1 && p->sq[w].supplies < 18) msg_resupplied(p, a);
    if (p->sq[w].supplies < 10) {
        p->sq[m].mark = 0;
        p->sq[m].used = false;
        end_check(p, w, m);
    }
    p->sq[w].supplies = 20;
}

/* ovl4_8bcd (every tick, and when a squadron is selected): your squadrons and
 * all merchants against the enemy's and all merchants: encounters, merchants
 * met or taken, and sighting. */
static void encounters(Play *p)
{
    bool flag[MAX_SHIPS] = { false }, logged[MAX_SHIPS] = { false };
    EV(EV_BASE, 2212);
    p->in_encounters = true;
    for (int a = 0; a < MAX_SHIPS; a++) {
        PSquad *A = &p->sq[a];
        bool a_war = A->mark != 2;
        EV(EV_S40, 1);
        if (!A->used || (A->side == 0 && a_war) || A->partner) continue;
        if (!a_war && play_cell(p, a) == A->merchant_dest) continue;
        for (int b = 0; b < MAX_SHIPS; b++) {
            EV(EV_S40, 1);
            if (b == a) continue;
            PSquad *B = &p->sq[b];
            bool b_war = B->mark != 2;
            if ((B->side == -1 && b_war) || !B->used || B->partner) continue;
            if (!a_war && !b_war) continue;
            if (!b_war && play_cell(p, b) == B->merchant_dest) continue;
            if (close_to(p, a, b)) {
                redraw_work(p);
                if (A->side == B->side) meet_merchant(p, a, b);
                else if (!a_war && b_war) take_merchant(p, a, b);
                else if (!b_war && a_war) take_merchant(p, b, a);
                else {
                    flag[b] = flag[a] = true;
                    LOGN("EVENT encounter: %s meets %s; both stop; they fight it out on the map after %d ticks of countdown "
                         "unless you Zoom", SQN(a), SQN(b), p->fight_delay);
                    A->partner = b + 1;
                    B->partner = a + 1;
                    msg_encounter(p, a);
                    B->ctrl = 1;
                    A->ctrl = 1;
                    A->port_speed = A->speed;
                    B->port_speed = B->speed;
                    B->speed = 0;
                    A->speed = 0;
                }
                break;
            }
            int v = vis(p);
            bool sighted = (p->dist < v && v < 80) || (v >= 80 && p->dist < 20);
            if (sighted && a_war && b_war) {
                flag[a] = flag[b] = true;
                if (!B->seen) {
                    if (!logged[b]) LOGN("EVENT %s sighted by your squadron %d (distance 2|dx|+|dy| = %d, visibility %d)", SQN(b), a, p->dist, v);
                    logged[b] = true;
                    msg_sighted(p, b);
                }
            }
        }
    }
    for (int i = 0; i < MAX_SHIPS; i++) p->sq[i].seen = flag[i] || p->sq[i].partner;
    p->in_encounters = false;
}

/* ------------------------------------------------------------------ ports */

/* ovl4_c141: is a merchant sailing for this port? */
static bool merchant_bound(const Play *p, int cell)
{
    for (int i = 0; i < MAX_SHIPS; i++)
        if (p->sq[i].used && p->sq[i].mark == 2 && p->sq[i].merchant_dest == cell) return true;
    return false;
}

/* ovl4_ee4e: the worst damage among a squadron's ships */
int worst_damage(Play *p, int i)
{
    int w = 0;
    list_ships(p, i);
    for (int k = 0; k < 3 && p->list[k] != -1; k++) {
        const PShip *s = &p->ship[p->list[k]];
        if (s->sails > w) w = s->sails;
        if (s->hull > w) w = s->hull;
    }
    return w;
}

/* ovl4_c102: where this port's merchants come from (the cell itself if none) */
static int port_source_of(const Play *p, int cell)
{
    for (int k = 0; k < MAX_PORTS; k++) if (p->port_cell[k] == cell) return p->port_source[k];
    return cell;
}

/* ovl4_bd86: a port at this cell was used. When it runs low it is restocked
 * at once if it is beside a crown or no merchant can reach it; otherwise a
 * merchant sets out from the edge of the map (ovl4:BEE3, with the enemy's
 * routes: step 5). */
void port_call(Play *p, int cell, int side)
{
    bool crown = crown_beside(p, cell, -1) != -1 || crown_beside(p, cell, 0) != -1;
    int port = port_of_cell(p, cell);
    if (port == -1) return;
    if (merchant_bound(p, cell)) return;
    int rep = repair_port(p, cell, false), sup = supply_port(p, cell, false);
    bool need = false;
    for (int j = 0; j < MAX_SHIPS && !need; j++) {
        const PSquad *q = &p->sq[j];
        if (!q->used || q->mark == 2 || play_cell(p, j) != cell) continue;
        if (sup != -1 && q->supplies < 20) need = true;
        else if (rep != -1 && worst_damage(p, j)) need = true;
    }
    if (!need && p->port_sup[port] > 10) return;
    int src = port_source_of(p, cell);
    if (src == cell || crown) {
        LOGV("EVENT the port at %s is restocked at once (%s)", CELLN(port), crown ? "beside a crown" : "no merchant can reach it");
        if (p->port_sup[port] == 0) invert(p, rep);   /* as WAS.EXE: the repair port */
        p->port_sup[port] = 20;
        return;
    }
    /* ovl4:BEE3: a merchant sets out from the edge of the map, in the last free squadron slot */
    int k = MAX_SHIPS - 1;
    while (k > -1 && (k == p->free_slot || p->sq[k].used)) k--;
    if (k == -1) return;
    PSquad *m = &p->sq[k];
    m->has_route = 0;
    m->has_path = false;
    m->ctrl = -2;
    m->port_speed = 0;
    m->seen = false;
    m->used = true;
    m->speed = 0;
    m->partner = 0;
    m->x = cell_mid_x(p, src);
    m->y = cell_mid_y(p, src);
    m->supplies = 20;
    m->side = side;
    m->mark = 2;
    m->heading = 0;
    if (m->x > 70) m->heading = 12;
    if (m->x < 10) m->heading = 4;
    if (m->y < 10) m->heading = 8;
    p->ai_target = cell;
    m->merchant_dest = cell;
    p->ai_sq = k;
    LOGN("EVENT %s sets out from %s for the port at %s (it has %d left)", SQN(k), CELLN(src), CELLN(port), p->port_sup[port]);
    if (enemy_route(p)) m->routed = 1;
}

/* ovl4_bcd2 (tick 10): ports with 2 or less left call for a merchant */
static void restock(Play *p)
{
    for (int k = 0; k < MAX_PORTS && p->port_cell[k] != -1; k++) {
        int cell = p->port_cell[k], port = port_of_cell(p, cell);
        if (port != -1 && p->port_sup[port] <= 2) {
            LOGV("EVENT tick 10: the port at %s has %d left and calls for a merchant", CELLN(port), p->port_sup[port]);
            redraw_work(p);
            port_call(p, cell, enemy_port_side(p, cell));
        }
    }
}

/* ovl4_e5bf: a way by sea from the port's cell to the edge of the map, the
 * cell on the edge or -1. WAS.EXE's search, queue and all (it looks at the
 * first cell in its queue twice and drops some). */
static int sea_way(Play *p, int c)
{
    int col = c % 10;
    if (c < 10 || c > 189 || col == 0 || col == 9) return c;
    /* WAS.EXE's work arrays: came[] (DS:3F3D) and the queue (DS:457D), which
       runs on into the strength map when it holds more than 100 cells */
    int16_t *came = p->came, *queue = p->queue;
    const int qmax = AREA_WORDS - (int)(p->queue - p->area);
    SDL_memset(came, 0, NUM_CELLS * sizeof came[0]);
    came[c] = 9;
    queue[0] = (int16_t)c;
    int n = 1;
    for (;;) {
        if (--n < 0) return -1;
        int cur = queue[0];
        EV(EV_SEAPOP, 1);
        EV(EV_SEAQ, n);
        for (int k = 0; k < n; k++) queue[k] = queue[k + 1];
        col = cur % 10;
        int d = 1, base = 1;
        if (rnd(p) < 64) { d = 5; base = 5; }
        for (int k = 1; k <= 8; k++) {
            if (!way_blocked(p, d, cur, col)) {
                if (off_map(cur, col, d)) {           /* ovl4:E6D6: the way back fills the queue */
                    int res = cur, m = 0;
                    for (;;) {
                        queue[0] = (int16_t)cur;
                        if (cur == c) return res;
                        if (cur < 0 || cur >= NUM_CELLS) return -1;
                        cur -= dir_off(p, came[cur]);
                        if (++m > 99) return -1;
                        EV(EV_SEAB, m);
                        for (int j = m; j > 0; j--) queue[j] = queue[j - 1];
                    }
                }
                int nb = cur + dir_off(p, d);
                if (nb >= 0 && nb < NUM_CELLS && p->coast[nb] != 0x379 && !coast_pass(p, d, nb) && came[nb] == 0) {
                    came[nb] = (int16_t)d;
                    if (n >= qmax) return -1;
                    queue[n++] = (int16_t)nb;      /* ovl4:E7BB: queue[l6++] */
                }
            }
            if (k < 8) {                     /* SO5+12h: 1, -1, 2, -2, 3, -3, 4 */
                static const int8_t TURN[8] = { 0, 1, -1, 2, -2, 3, -3, 4 };
                d = TURN[k] + base;
                if (d > 8) d -= 8;
                if (d < 1) d += 8;
            }
        }
    }
}

/* ovl4_c07c: the ports merchants can reach, and from where */
void port_sources(Play *p)   /* ovl4:C07C */
{
    for (int k = 0; k < MAX_PORTS; k++) p->port_cell[k] = p->port_source[k] = -1;
    int n = 0;
    for (int c = 0; c < NUM_CELLS && n < MAX_PORTS; c++) {
        if (tile_land(p->map[c]) || port_of_cell(p, c) == -1) continue;
        int src = sea_way(p, c);
        if (src == -1) continue;
        p->port_cell[n] = c;
        p->port_source[n] = src;
        n++;
    }
}

/* ovl4_c197: what each port and crown holds at the start (rule 2) */
static void port_supplies(Play *p)
{
    for (int c = 0; c < NUM_CELLS; c++) {
        int t = p->map[c];
        p->port_sup[c] = 0;
        if (t == 74) p->port_sup[c] = p->rules[1];
        if (t == 75 || t == 76 || t == 77) p->port_sup[c] = p->rules[1] << 1;
    }
}

/* ovl4_9345 (two squadrons a tick): eating, hunger, and an anchored squadron
 * taking supplies and repairs from a port or crown beside it */
static void eat(Play *p, int i)
{
    EV(EV_BASE, 201);
    PSquad *q = &p->sq[i];
    if (!q->used || q->mark == 2 || q->partner) return;
    int chance = commandable(p, i) ? (p->rules[4] << 5) - 10 : OPP(p, 0x6059);
    if (rnd(p) < chance) q->supplies -= 2;
    if (q->supplies < 0) q->supplies = 0;
    list_ships(p, i);
    if (q->supplies < 4 && rnd(p) < 20) {
        for (int k = 0; k < 3 && p->list[k] != -1; k++) {
            PShip *s = &p->ship[p->list[k]];
            if (s->sails < 4 && rnd(p) < 64) s->sails++;
            if (s->hull < 4 && rnd(p) < 64) s->hull++;
        }
        LOGN("EVENT %s is short of supplies (%d): its ships fall into disrepair", SQN(i), q->supplies);
    }
    if (q->speed != 0) return;
    int cell = play_cell(p, i), other = q->side == -1 ? 0 : -1;
    if (count_at(p, cell, other, true)) return;   /* the enemy is here */
    redraw_work(p);
    if (q->supplies < 20) {
        int port = supply_port(p, cell, true);
        if (port == -1) port = crown_beside(p, cell, -1);
        if (port == -1) port = crown_beside(p, cell, 0);
        if (port != -1 && p->port_sup[port] > 0) {
            if (p->rules[1] < 20) {          /* 20 and more: ports never run out */
                if (--p->port_sup[port] <= 0) { invert(p, port); port_empty_sound(p); LOGN("EVENT the port at %s is empty", CELLN(port)); }   /* ovl4_c57c */
                port_call(p, cell, q->side);
            }
            LOGV("EVENT %s takes on supplies at %s (%d + 5; the port has %d left)", SQN(i), CELLN(port), q->supplies, p->port_sup[port]);
            q->supplies += 5;
        }
    }
    if (q->supplies > 20) q->supplies = 20;
    if (rnd(p) < p->rules[0]) {
        int port = repair_port(p, cell, true);
        if (port == -1) port = crown_beside(p, cell, -1);
        if (port == -1) port = crown_beside(p, cell, 0);
        if (port == -1) return;
        for (int k = 0; k < 3; k++) {        /* list[] as it is now: port_call may have changed it, as in WAS.EXE */
            if (p->port_sup[port] == 0) break;
            int s = p->list[k];
            if (s == -1) break;
            PShip *sh = &p->ship[s];
            if (sh->sails == 0 && sh->hull == 0) continue;
            if (p->rules[1] < 20) {
                if (--p->port_sup[port] <= 0) { invert(p, port); port_empty_sound(p); LOGN("EVENT the port at %s is empty", CELLN(port)); }
                port_call(p, cell, q->side);
            }
            if (sh->sails) sh->sails--;
            if (sh->hull) sh->hull--;
            LOGV("EVENT %s is repaired at %s: sails %d, hull %d (chance 64AD %d/128)", gamelog_ship(p, s), CELLN(port), sh->sails, sh->hull,
                 p->rules[0]);
        }
    }
}

/* ------------------------------------------------------------------ sailing */

/* ovl4_c302: after a loss at sea the squadron's first other ship of the line
 * becomes a flagship (whatever was lost, as in WAS.EXE) */
static void promote(Play *p, int lost)
{
    for (int k = 0; k < 3; k++) {
        int s = p->list[k];
        if (s == -1 || s == lost) continue;
        if (p->ship[s].type == 1) {
            p->ship[s].type = 3;
            LOGN("EVENT %s becomes the squadron's flagship", gamelog_ship(p, s));
            break;
        }
    }
}

static const char *t_name_water(int t) { return (t >= 10 && t <= 12) || (t >= 16 && t <= 18) ? "rough water" : "shallow water"; }

/* ovl4_9d7e: ship s (list[k]) of squadron i is lost in rough or shallow water */
static int lost_at_sea(Play *p, int k, int s, int cell, int i)
{
    LOGN("EVENT %s of %s is lost in %s (rule %s)", gamelog_ship(p, s), SQN(i), t_name_water(p->map[cell]),
         p->ship[s].type == 2 ? "64B3, rough water" : "64B9, shallow water");
    count_loss(p, s, i, 0);
    msg_sunk(p, s, p->map[cell], i);
    promote(p, s);
    p->ship[s].sq = -1;
    p->list[k] = -1;
    int j = 0;
    while (j < 3 && p->list[j] == -1) j++;
    if (j == 3) {
        p->sq[i].used = false;
        int r = end_check(p, i, i);
        if (r) return r;
        check_over(p, 0);
    }
    mark_flagships(p);
    int f = flags_check(p);
    if (f == 1) { end_game(p, 1); p->result_code = 10; }
    if (f == 2) { end_game(p, 2); p->result_code = 0; }
    return 0;
}

/* ovl4_9ab2: how many ticks each sailing squadron's step takes: 3 - speed,
 * doubled for ships of the line and flagships, more with torn sails, and
 * shifted left by DS:0ECD when sailing into the wind. Ships of the line in
 * shallow water and frigates in rough water may be lost (rules 7 and 4). */
static int set_delays(Play *p)
{
    for (int i = 0; i < MAX_SHIPS; i++) {
        PSquad *q = &p->sq[i];
        EV(EV_S40, 1);
        if (!q->used || q->speed == 0 || q->partner) continue;
        if (q->mark == 2) q->move_delay = 3;
        else {
            int cell = play_cell(p, i);
            bool rough = tile_rough(p->map[cell]), shallow = tile_shallow(p->map[cell]);
            q->move_delay = 3 - q->speed;
            int slow = 0;
            bool big = false;
            list_ships(p, i);
            for (int k = 0; k < 3; k++) {
                int s = p->list[k];
                if (s == -1) break;
                bool lose = false;
                if (p->ship[s].type != 2) {
                    if (!big) { q->move_delay = (3 - q->speed) << 1; big = true; }
                    if (shallow && p->rules[6] != 1 && (rnd(p) < 2 || (p->rules[6] == 0 && rnd(p) < 2)))
                        lose = q->side == -1 || rnd(p) < 32;
                } else if (rough && p->rules[3] != 1 && (rnd(p) < 2 || (p->rules[3] == 0 && rnd(p) < 2))) {
                    lose = q->side == -1 || rnd(p) < 32;
                }
                if (lose) {
                    int r = lost_at_sea(p, k, s, cell, i);
                    if (r) return r;
                }
                if (p->ship[s].sails > 2 && slow == 0) { q->move_delay++; slow++; }
                if (p->ship[s].sails > 4 && slow < 2) { q->move_delay++; slow++; }
            }
        }
        if (!p->wind_on) continue;
        int h = q->heading, idx;
        if (p->wind == 0 && (h > 12 || h < 4)) idx = h - 13;
        else if (p->wind == 1 && h > 0 && h < 8) idx = h - 1;
        else if (p->wind == 2 && h > 4 && h < 12) idx = h - 5;
        else if (p->wind == 3 && h > 8) idx = h - 9;
        else continue;
        if (idx < 0) idx += 16;
        q->move_delay <<= p->ga->ds[0xECD + idx];
    }
    return 0;
}

/* ovl4_96a0: the squadrons on paths take a step when their countdown runs
 * out, heading the average way of their next two steps. The marker follows
 * the squadron selected. */
static int move_squadrons(Play *p)
{
    EV(EV_BASE, 1100);
    redraw_work(p);
    bool moved = false;
    int r = set_delays(p);
    if (r) return r;
    for (int i = 0; i < MAX_SHIPS; i++) {
        PSquad *q = &p->sq[i];
        EV(EV_S40, 1);
        if (!q->has_path || !q->used || q->speed == 0 || q->partner) continue;
        bool friend_war = q->side == -1 && q->mark != 2;
        if (--q->countdown > 0) continue;
        int old = q->heading, first = 0, sum = 0, k;
        for (k = 1; k < 3; k++) {
            int st = path_at(p, q->path_pos + k);
            if (st == PATH_END || st == 0) break;
            int h = heading_of(st);
            if (k == 1) first = h;
            else if (first <= 6 && h >= 12) h -= 16;
            else if (first >= 12 && h <= 6) h += 16;
            sum += h;
        }
        if (k == 3) {
            sum = sar1(sum);
            if (sum < 0) sum += 16;
            if (sum > 15) sum -= 16;
            q->heading = sum;
        }
        if (q->heading < 0 || q->heading > 15) q->heading = old;
        if (q->heading < 0 || q->heading > 15) q->heading = 0;
        if (friend_war) moved = true;
        q->countdown = q->move_delay;
        int st = path_at(p, q->path_pos);
        if (q->path_pos >= 0 && q->path_pos < PATH_SIZE) p->path[q->path_pos] = 0;
        if (st == PATH_END) {
            if (friend_war) LOGN("EVENT %s has sailed to the end of its path and anchors", SQN(i));
            else LOGV("EVENT %s has sailed to the end of its path and anchors", SQN(i));
            q->dest = NO_DEST;
            q->port_speed = 0;
            q->speed = 0;
            q->has_path = false;
            if (!friend_war) p->ai_done = i - 1;
            else port_call(p, play_cell(p, i), q->side);
            if (crown_check(p, i)) return 0;
            continue;
        }
        if (st > 78) { q->y++; st -= 80; }
        else if (st < -78) { q->y--; st += 80; }
        q->x += st;
        q->path_pos = next_slot(q->path_pos);
        if (p->mode != PM_NORMAL && p->mode != PM_TRACE && p->sel == i && (vis(p) >= 80 || friend_war || q->seen)) {
            p->cx = q->x;                    /* the marker follows */
            p->cy = q->y + 1;
        }
    }
    if (moved && p->mode == PM_SQUAD) cmd_squad_line(p);
    return 0;
}

/* ------------------------------------------------------------------ the clock */

/* campaign_play ovl4:7959-7A80, a tick, in its parts: 1 the counters and
 * encounters, 2-5 the enemy's orders, 6 move_squadrons, 7 eating, 8 the rest.
 * WAS.EXE asks key_waiting (ovl4:85F8: a key held down or in the buffer)
 * before parts 2-8 and goes straight to campaign_input if so (play_frame).
 * True when the game ends at once. */
enum { TS_START = 1, TS_ENEMY = 2, TS_MOVE = 6, TS_EAT = 7, TS_REST = 8 };

static bool tick_part(Play *p, int s)
{
    if (s == TS_START) {
        gamelog_tick(p);
        p->tick_count = 0;
        p->ticks++;
        if (++p->sq_turn >= MAX_SHIPS) p->sq_turn = 0;
        encounters(p);
        return false;
    }
    if (s < TS_MOVE) {                       /* the enemy, four times */
        if (p->mode != PM_TRACE) enemy_turn(p);
        return false;
    }
    if (s == TS_MOVE) { move_squadrons(p); return false; }
    if (s == TS_EAT) {
        eat(p, p->sq_turn);
        if (++p->sq_turn >= MAX_SHIPS) p->sq_turn = 0;
        eat(p, p->sq_turn);
        return false;
    }
    if (p->ticks == 10 && rnd(p) < 75) restock(p);
    if (p->ticks == 20) {                    /* ovl4_c51a: a crown taken ends the game after the key */
        for (int i = 0; i < MAX_SHIPS; i++) {
            const PSquad *q = &p->sq[i];
            if (!q->used || q->partner || q->mark == 2) continue;
            if (crown_check(p, i)) { p->end_quit = true; return true; }
        }
    }
    if (p->ticks == 25 && p->wind_on) {
        int chance = p->rules[7] == 1 ? 16 : p->rules[7] == 2 ? 64 : 0;
        if (rnd(p) < chance) wind_change(p, false);
    }
    if (p->ticks > 30) {
        if (--p->msg_timer < 0) p->msg_last = 0;
        p->ticks = 0;
        enemy_strength_map(p);
        gamelog_grid(p, "The strength map worked out again (tick 31; DS:4645: the enemy's +, yours -, -7 your flagship):", p->strength);
    }
    return false;
}

static bool tick_over(const Play *p) { return p->mode == PM_END || p->mode == PM_OFFER; }

/* a whole tick with no key waiting (play_debug_call) */
static bool tick(Play *p)
{
    for (int s = TS_START; s <= TS_REST; s++) {
        if (tick_part(p, s)) return true;
        if (tick_over(p)) return false;
    }
    return false;
}

/* ------------------------------------------------------------------ the marker and scrolling */

/* DS:0739, 0744: the keypad's steps (Home .. PgDn) */
static const int8_t STEP_DX[11] = { -1, 0, 1, 0, -1, 0, 1, 0, -1, 0, 1 };
static const int8_t STEP_DY[11] = { -1, -1, -1, 0, 0, 0, 0, 0, 1, 1, 1 };

static bool scroll_up_key(int k)   { return k == PK_MINUS || k == PK_KP_MINUS || k == PK_TAB; }    /* rt_1217 */
static bool scroll_down_key(int k) { return k == PK_EQUALS || k == PK_KP_PLUS || k == PK_CTRL; }  /* rt_126a */

/* ovl4_b3e5: may the tracing box move this way (k: Home=0 .. PgDn=10)? 77 if not:
 * land under it, in the tiles or in the map's pixels, or the slant of a shore */
static int trace_blocked(const Play *p, int k)
{
    int x = ds_word(p, 0xF16 + 2 * k) + p->cx, y = ds_word(p, 0xF2C + 2 * k) + p->cy;
    int c1 = ((y + 1) / 10) * 10 + (x >> 3), c2 = ((y + 2) / 10) * 10 + (x >> 3);
    if (c1 < 0 || c1 >= NUM_CELLS || c2 < 0 || c2 >= NUM_CELLS) return 77;
    int t1 = p->map[c1], t2 = p->map[c2];
    bool blocked = false;
    if (tile_land(t1) || tile_land(t2)) blocked = true;
    else {
        if (t2 > 77 && t2 < 91) return 0;    /* the names and pictures on the sea */
        int base = y * 80 + x;
        for (int j = 0; j < 10 && !blocked; j++)
            if (land_byte(buf_byte(p, base + ds_word(p, 0xF02 + 2 * j)))) blocked = true;
    }
    if (!blocked) {
        int rx = x - cell_mid_x(p, c1), ry = y - cell_mid_y(p, c1), v = p->coast[c1];
        if ((v == 0x371 && -(ry + 1) > rx) || (v == 0x372 && ry + 1 < rx) ||
            (v == 0x374 && 6 - ry < rx) || (v == 0x373 && ry - 8 > rx)) blocked = true;
    }
    return blocked ? 77 : 0;
}

static void end_trace(Play *p);

/* campaign_input ovl4:854B: the marker moved d while tracing: the steps go
 * into the path ring (at most 80 at a time); running into another path ends it */
static void trace_step(Play *p, int d)
{
    for (;;) {
        p->path_w = next_slot(p->path_w);
        if (p->path[p->path_w] != 0) {
            if (--p->path_w < 0) p->path_w = PATH_SIZE - 1;
            p->path[p->path_w] = 0;
            p->path_w--;
            end_trace(p);
            return;
        }
        if (d > 120) { d -= 80; p->path[p->path_w] = 80; continue; }
        if (d < -120) { d += 80; p->path[p->path_w] = -80; continue; }
        p->path[p->path_w] = (int8_t)d;
        return;
    }
}

/* campaign_input ovl4:81A8-85CC (every iteration): move the marker or scroll
 * the view. While tracing it moves every other iteration, stays on the sea and
 * leaves a path. */
static void cursor_input(Play *p, int key)
{
    int maxx = CX_MAX, maxy = CY_MAX;
    bool trace = p->mode == PM_TRACE;
    if (trace) {
        if (++p->trace_skip > 1) p->trace_skip = 0;
        maxx = CX_MAX - 1;
        maxy = CY_MAX - 2;
    } else p->trace_skip = 0;
    if (p->trace_skip) return;
    bool up = scroll_up_key(key), down = scroll_down_key(key);
    if (!up && !down && !(key > 70 && key < 82)) return;
    int delta = 0;
    if (up) {
        if (p->view_top < 2) { if (p->demo) p->demo_key = 0; return; }   /* ovl4:824A */
        if (p->jump && !trace) {             /* [081A]: to the top */
            p->view_top = 0; p->cx = 38; p->cy = 45; delta = 1;
        } else {
            if (trace) {
                if (trace_blocked(p, 1) == 77 && p->view_top + VIEW_MARGIN < p->cy) return;
                p->cy--;
                bool stop = trace_blocked(p, 1) == 77 && p->view_top + VIEW_MARGIN - 3 < p->cy;
                p->cy++;
                if (stop) return;
            }
            p->view_top -= 2;
            if (p->view_top + VIEW_MARGIN < p->cy) { p->cy -= 2; delta = -160; }
        }
    } else if (down) {
        if (p->view_top >= VIEW_TOP_MAX - 1) { if (p->demo) p->demo_key = 0; return; }   /* ovl4:8334 */
        if (p->jump && !trace) {             /* to the bottom */
            p->view_top = VIEW_TOP_MAX; p->cx = 38; p->cy = 145; delta = 1;
        } else {
            if (trace) {
                if (trace_blocked(p, 9) == 77 && p->cy < p->view_top) return;
                p->cy++;
                bool stop = trace_blocked(p, 9) == 77 && p->view_top + 3 > p->cy;
                p->cy--;
                if (stop) return;
            }
            p->view_top += 2;
            if (p->cy < p->view_top) { p->cy += 2; delta = 160; }
        }
    } else if (p->mode != PM_PROMPT) {
        int k = key - PK_HOME;
        if (trace && trace_blocked(p, k) == 77) return;
        p->cx += STEP_DX[k];
        p->cy += STEP_DY[k];
        delta = ds_sbyte(p, 0x74F + k);      /* the step as an address */
    }
    if (p->cx > maxx) { p->cx--; delta--; }
    if (p->cx < 1 || (p->cx < 2 && trace)) { p->cx++; delta++; }
    if (p->cy > maxy) { p->cy--; delta -= 80; }
    if (p->cy < 0) { p->cy++; delta += 80; }
    if (p->cy < p->view_top) p->view_top--;
    else if (p->view_top + VIEW_MARGIN < p->cy) p->view_top++;
    p->view_top = SDL_clamp(p->view_top, 0, VIEW_TOP_MAX);
    if (delta == 0) return;
    if (!trace) check_over(p, key);
    else trace_step(p, delta);
}

/* ------------------------------------------------------------------ tracing a path */

/* ovl4_ecd1: the longest free run in the path ring (from just after the
 * writing position), -1 if shorter than 15 */
int find_free_run(const Play *p)
{
    int run = 0, best = 0;
    int start = next_slot(p->path_w + 1), k = start, at = start;
    for (;;) {
        while (p->path[k] == 0 && k != p->path_w) { run++; k = next_slot(k); EV(EV_PRING, 1); }
        if (run > best) { best = run; at = start; }
        run = 0;
        while (p->path[k] != 0 && k != p->path_w) { k = next_slot(k); EV(EV_PRING, 1); }
        start = k;
        if (k == p->path_w) break;
    }
    return best < 15 ? -1 : at;
}

/* ovl4_8a2c: the selected squadron's path is cancelled; it stops */
static void cancel_path(Play *p)
{
    PSquad *q = &p->sq[p->sel];
    q->port_speed = 0;
    q->speed = 0;
    q->dest = NO_DEST;
    p->cy++;
    clear_paths(p, p->sel);
}

/* ovl4_8a75: M (or running into another path) ends the tracing: the path
 * ends with 99 and the squadron sets out at full sail */
static void end_trace(Play *p)
{
    PSquad *q = &p->sq[p->sel];
    beep(p);
    if (q->path_pos == p->path_w) {          /* nothing traced */
        LOGN("YOU Mark with nothing traced: %s stays", SQN(p->sel));
        cancel_path(p);
        p->mode = PM_NORMAL;
        cmd_normal(p);
        return;
    }
    p->cy++;
    p->mode = PM_NORMAL;
    q->has_path = true;
    q->dest = p->cy * 80 + p->cx;
    p->path_w = next_slot(p->path_w);
    if (p->path[p->path_w] != 0 && --p->path_w < 0) p->path_w = PATH_SIZE - 1;
    p->path[p->path_w] = PATH_END;
    p->path_w = next_slot(p->path_w);
    q->path_pos = next_slot(q->path_pos);
    q->port_speed = 2;
    q->speed = 2;
    q->countdown = 2;
    int h = heading_of(path_at(p, q->path_pos + 1));
    q->heading = h < 0 || h > 15 ? 0 : h;
    if (LOGGING) {
        int n = 0;
        for (int k = q->path_pos, g = 0; g < PATH_SIZE && p->path[k] != PATH_END && p->path[k] != 0; k = next_slot(k), g++) n++;
        LOGN("YOU Mark: %s sails at full sail to %d,%d (%s): %d steps", SQN(p->sel), p->cx, p->cy,
             CELLN(((p->cy + 1) / 10) * 10 + (p->cx >> 3)), n);
    }
    cmd_normal(p);
}

/* campaign_input ovl4:8123, start_move_order (ovl4:89E4) and ovl4_acda: M */
static void start_trace(Play *p)
{
    PSquad *q = &p->sq[p->sel];
    if (q->side != -1 || q->partner) return;
    LOGN("YOU Move: trace a path for %s%s", SQN(p->sel), q->has_path ? " (its old path is cancelled)" : "");
    beep(p);
    if (q->has_path) cancel_path(p);
    int start = find_free_run(p);
    if (start == -1) return;
    p->path_w = start;
    q->path_pos = start;
    text_reset(p);
    print(p, SO5(p, 0x209));                 /* Trace path to destination and press [M] */
    row_icon(p, 23, 72, 0x691);
    clear_row(p, 24);
    print(p, SO5(p, 0x22D));                 /* " Mark Cancel" */
    p->mode = PM_TRACE;
    p->cx = q->x;
    p->cy = q->y;
    delay(p, 2000);
    delay(p, 5000);                          /* command_pause */
}

/* ------------------------------------------------------------------ Detach and Join */

/* rt_1f2d (place_detached): squadron b gets a's side and is put by squadron at's
 * place, (steps * 3 + 1) bytes east or west, or twice that many line pairs north
 * or south, where there is sea. Detach: steps 0, at = a; after a sea battle the
 * ships that sailed off it: steps 1, at = the other squadron. */
void play_place_detached(Play *p, int from, int to, int steps, int at)
{
    PSquad *a = &p->sq[from], *b = &p->sq[to];
    b->side = a->side;
    int cell = play_cell(p, at), t = map_at(p, cell), col = cell % 10, row = cell / 10;
    bool east = col == 9 || coast_b(t) || coast_d(t) || t == 42 || t == 48 ||
                !tile_sea(map_at(p, cell + 1)) || tile_land(map_at(p, cell + 1));
    bool west = col == 0 || coast_a(t) || coast_c(t) || t == 49 || t == 50 ||
                !tile_sea(map_at(p, cell - 1)) || tile_land(map_at(p, cell - 1));
    bool north = row == 0 || coast_a(t) || coast_b(t) || (t >= 43 && t <= 45) ||
                 !tile_sea(map_at(p, cell - 10)) || tile_land(map_at(p, cell - 10));
    int n = steps * 3 + 1;
    b->has_route = 0; b->has_path = false; b->routed = 0;
    b->port_speed = 0;
    b->speed = 0;
    b->used = true;
    b->x = p->sq[at].x;
    b->y = p->sq[at].y;
    if (east && west) b->y += north ? 2 * n : -2 * n;
    else if (east) b->x -= n;
    else b->x += n;
    b->supplies = a->supplies;
    b->mark = 0;
    b->partner = 0;
    b->heading = 0;
}

static void place_new(Play *p, int from, int to) { play_place_detached(p, from, to, 0, from); }

/* ovl4_a952: the two columns of names */
static void dj_names(Play *p)
{
    for (int k = 0; k < 3; k++) {
        tpos(p, 10 + k, 6);
        spaces(p, 14);
        if (p->list[k] != -1) { tpos(p, 10 + k, 6); print(p, p->c->ships[p->list[k]].name); }
    }
    for (int k = 0; k < 3; k++) {
        tpos(p, 10 + k, 21);
        spaces(p, 14);
        if (p->dj_other[k] != -1) { tpos(p, 10 + k, 21); print(p, p->c->ships[p->dj_other[k]].name); }
    }
}

/* ovl4_a347: D or J: the selected squadron and the empty slot (Detach) or
 * the squadron beside it (Join) share their supplies; then ships are moved
 * between them with -/+ and S */
static void dj_start(Play *p, bool join)
{
    list_ships(p, p->free_slot);
    int n2 = 0, n1 = 0;
    for (int k = 0; k < 3; k++) { p->dj_other[k] = p->list[k]; if (p->list[k] != -1) n2++; }
    list_ships(p, p->sel);
    for (int k = 0; k < 3; k++) if (p->list[k] != -1) n1++;
    PSquad *a = &p->sq[p->sel], *b = &p->sq[p->free_slot];
    a->supplies = (b->supplies * n2 + a->supplies * n1) / (n1 + n2);
    b->supplies = (b->supplies * n2 + a->supplies * n1) / (n1 + n2);
    p->dj_new = p->dj_other[0] == -1;
    beep(p);
    clear_panel(p);
    tpos(p, 8, 9);
    print(p, SO5(p, 0x1C9));                 /* SQUAD  1       SQUAD  2 */
    dj_names(p);
    clear_row(p, 23);
    print(p, SO5(p, 0x1E1));                 /* Move cursor to ship you wish to switch. */
    clear_row(p, 24);
    row_icon(p, 24, 0, 0x64D);               /* + */
    row_icon(p, 24, 4, 0x66F);               /* - */
    tpos(p, 24, 6);
    print(p, DS(p, 0x1E1));                  /* Switch */
    tpos(p, 24, 36);
    print(p, DS(p, 0x1E8));                  /* End */
    p->dj_pos = join ? 3 : 0;
    p->back_mode = p->mode;
    p->mode = PM_DETACH;
    if (join) LOGN("YOU Join: %s with %s (supplies shared: %d)", SQN(p->sel), SQN(p->free_slot), a->supplies);
    else LOGN("YOU Detach: %s, into the empty slot %d (supplies shared: %d)", SQN(p->sel), p->free_slot, a->supplies);
}

/* the marked ship must be one that can move: ovl4:A655 */
static void dj_fix(Play *p)
{
    for (int guard = 0; guard < 16; guard++) {
        if (p->dj_pos < 3) {
            if (p->list[p->dj_pos] == -1) { if (--p->dj_pos < 0) p->dj_pos = 5; continue; }
            if (p->dj_other[2] != -1) p->dj_pos = 5;
        }
        if (p->dj_pos >= 3) {
            if (p->dj_other[p->dj_pos - 3] == -1) { if (--p->dj_pos < 0) p->dj_pos = 5; continue; }
            if (p->list[2] != -1) p->dj_pos = 2;
        }
        return;
    }
}

static void dj_compact(int *l)
{
    for (int k = 0; k < 2; k++) if (l[k] == -1) { l[k] = l[k + 1]; l[k + 1] = -1; }
}

static void dj_key(Play *p, int key)
{
    if (key == PK_EQUALS || key == PK_KP_PLUS) {          /* the next ship */
        if (++p->dj_pos > 5) { p->dj_pos = 0; if (p->dj_other[2] != -1) p->dj_pos = 3; }
        else if (p->dj_pos == 3 && p->list[2] != -1) p->dj_pos = 0;
        if (p->dj_pos < 3 && p->list[p->dj_pos] == -1) p->dj_pos = 3;
        if (p->dj_pos >= 3 && p->dj_other[p->dj_pos - 3] == -1) p->dj_pos = 0;
        beep(p);
        delay(p, 5000);
    } else if (key == PK_MINUS || key == PK_KP_MINUS) {   /* the one before */
        if (--p->dj_pos < 0) p->dj_pos = 5;
        dj_fix(p);
        beep(p);
        delay(p, 5000);
    } else if (key == PK_S) {                             /* Switch: to the other squadron */
        if (p->dj_pos < 3 && p->dj_other[2] == -1) {
            int k = 0;
            while (k < 3 && p->dj_other[k] != -1) k++;
            p->dj_other[k] = p->list[p->dj_pos];
            p->list[p->dj_pos] = -1;
            dj_compact(p->list);
        } else if (p->dj_pos >= 3 && p->list[2] == -1) {
            int k = 0;
            while (k < 3 && p->list[k] != -1) k++;
            p->list[k] = p->dj_other[p->dj_pos - 3];
            p->dj_other[p->dj_pos - 3] = -1;
            dj_compact(p->dj_other);
        }
        dj_names(p);
        dj_fix(p);
        beep(p);
        delay(p, 5000);
    } else if (key == PK_E || key == PK_ENTER) {          /* End */
        beep(p);
        for (int k = 0; k < 3; k++) if (p->list[k] != -1) p->ship[p->list[k]].sq = p->sel;
        for (int k = 0; k < 3; k++) if (p->dj_other[k] != -1) p->ship[p->dj_other[k]].sq = p->free_slot;
        if (p->dj_other[0] != -1) {
            if (p->dj_new) place_new(p, p->sel, p->free_slot);
        } else {
            clear_paths(p, p->free_slot);
            p->sq[p->free_slot].used = false;
        }
        if (p->list[0] == -1) {
            clear_paths(p, p->sel);
            p->sq[p->sel].used = false;
        } else p->over_squad = false;
        LOGN("YOU End: now %s; %s", p->sq[p->sel].used ? SQN(p->sel) : "the first squadron is gone",
             p->sq[p->free_slot].used ? SQN(p->free_slot) : "the second squadron is gone");
        clear_panel(p);
        p->mode = p->back_mode;
        mark_flagships(p);
        check_over(p, 0);
        clear_row(p, 23);
        delay(p, 5000);
    }
}

/* ------------------------------------------------------------------ Info, Fleet, Pause */

/* rt_0ac7: SPEED: Anchored / Half Sail / Full Sail */
static void show_speed(Play *p)
{
    if (!commandable(p, p->sel)) return;
    tpos(p, 16, 3);
    print(p, SO5(p, 0x18F));
    int s = p->sq[p->sel].speed;
    print(p, SO5(p, s == 0 ? 0x197 : s == 1 ? 0x1A1 : 0x1AB));
}

/* squadron_info (rt:074B), the text: the panel's lines and bars are drawn in play_draw */
static void info_text(Play *p)
{
    tpos(p, 8, 7);  print(p, DS(p, 0x2D));   /* NAME */
    tpos(p, 8, 19); print(p, DS(p, 0x32));   /* TYPE */
    tpos(p, 8, 27); print(p, DS(p, 0x37));   /* SAIL/HULL */
    tpos(p, 14, 14); print(p, DS(p, 0x41));  /* SUPPLIES--- */
    tpos(p, 14, 4); print(p, DS(p, commandable(p, p->sel) ? 0x51 : 0x4D));   /* FRIEND / FOE */
    for (int k = 0; k < 3 && p->list[k] != -1; k++) {
        const PShip *s = &p->ship[p->list[k]];
        tpos(p, 10 + k, 3);
        print(p, p->c->ships[p->list[k]].name);
        tpos(p, 10 + k, 18);
        print(p, DS(p, s->type == 1 ? 0x58 : s->type == 2 ? 0x5D : 0x65));   /* LINE / FRIGATE / FLAG */
    }
    show_speed(p);
}

/* campaign_input ovl4:7CAC: I over a squadron */
static void show_info(Play *p)
{
    text_reset(p);
    clear_row(p, 24);
    if (commandable(p, p->sel) && p->sq[p->sel].has_path) print(p, DS(p, 0x1BD));   /* "Speed" */
    tpos(p, 24, 36);
    print(p, DS(p, 0x1C3));                  /* "End" */
    list_ships(p, p->sel);
    clear_panel(p);
    info_text(p);
    tpos(p, 16, 10);
    const PSquad *q = &p->sq[p->sel];
    if (q->partner) print(p, DS(p, 0x1C7));  /* "Encounter" */
    else if (port_of_cell(p, play_cell(p, p->sel)) != -1 && q->speed == 0 && commandable(p, p->sel))
        print(p, DS(p, 0x1D1));              /* "In Port " */
    p->mode = PM_INFO;
}

/* ovl4:7D73: S changes the speed of a squadron on a traced path */
static void info_key(Play *p, int key)
{
    PSquad *q = &p->sq[p->sel];
    if (key == PK_S && commandable(p, p->sel) && q->has_path && !q->partner) {
        if (++q->speed > 2) q->speed = 0;
        LOGN("YOU Speed: %s now %s", SQN(p->sel), q->speed == 0 ? "anchored" : q->speed == 1 ? "at half sail" : "at full sail");
        show_speed(p);
        q->port_speed = q->speed;
        beep(p);
    }
    if (key == PK_E || key == PK_ENTER) {
        clear_panel(p);
        cmd_squad(p);                        /* select_squad */
    }
}

/* ovl4:EF92: count the flagships, ships of the line and frigates on each side */
static void count_fleet(Play *p)
{
    SDL_memset(p->fleet, 0, sizeof p->fleet);
    for (int i = 0; i < MAX_SHIPS; i++) {
        PSquad *q = &p->sq[i];
        if (!q->used || q->mark == 2) continue;
        list_ships(p, i);
        q->mark = 0;
        int side = commandable(p, i) ? 0 : 3;
        for (int k = 0; k < 3 && p->list[k] != -1; k++) {
            int t = p->ship[p->list[k]].type;
            if (t == 3) { p->fleet[side]++; q->mark = 1; }
            else if (t == 1) p->fleet[side + 1]++;
            else p->fleet[side + 2]++;
        }
    }
}

/* ovl4:AD63: F, the fleets */
static void show_fleet(Play *p)
{
    text_reset(p);
    count_fleet(p);
    beep(p);
    text_rows_add(p, 2);
    for (int side = 0; side < 2; side++) {
        clear_row(p, 21 + side);
        if (side == 0) print(p, SO5(p, 0x16F));   /* "Friend: " */
        else { print(p, DS(p, 0x25C)); tpos(p, 22, 8); }   /* "Foe: " */
        static const unsigned LABEL[3] = { 0x178, 0x17F, 0x186 };   /* Flag, Line, Frigate. */
        for (int k = 0; k < 3; k++) {
            int n = p->fleet[side * 3 + k];
            if (n < 10) print(p, " ");       /* rt_32d4(32) */
            print_num(p, n);
            print(p, SO5(p, LABEL[k]));
        }
    }
    clear_row(p, 24);
    print(p, DS(p, 0x262));                  /* "End" */
    p->mode = PM_FLEET;
}

/* ------------------------------------------------------------------ keys */

/* campaign_input ovl4:7EEA: Zoom or Practice: the shore for the battle from the map
 * cell (686B), then campaign_play returns 2 and rt_0000 runs the sea battle */
static void battle_ask(Play *p, int kind)
{
    int cell = play_cell(p, p->sel), t = map_at(p, cell), shore = 0;
    if (t == 42 || t == 48 || t == 53 || t == 54) shore = 1;
    else if (t >= 49 && t <= 52) shore = 2;
    else if (t >= 43 && t <= 47) shore = 3;
    else if (t >= 37 && t <= 41) shore = 4;
    else if (coast_b(t)) shore = 5;
    else if (coast_d(t)) shore = 6;
    else if (coast_c(t)) shore = 7;
    else if (coast_a(t)) shore = 8;
    int port = port_of_cell(p, cell);
    if (port != -1) {
        if (cell + 1 == port) shore = 9;
        else if (cell - 1 == port) shore = 10;
        else if (cell - 10 == port) shore = 11;
        else shore = 12;
    }
    if (shore == 0) {
        int col = cell % 10;
        if (col < 9 && tile_land(map_at(p, cell + 1))) shore = 1;
        if (col > 0 && tile_land(map_at(p, cell - 1))) shore = 2;
        if (cell > 9 && tile_land(map_at(p, cell - 10))) shore = 3;
        if (cell < 190 && tile_land(map_at(p, cell + 10))) shore = 4;
    }
    p->battle_shore = shore;
    p->battle_req = kind;
    if (kind == 1) LOGN("YOU Zoom: %s against %s, to the sea battle (shore %d)", SQN(p->sel), SQN(p->sq[p->sel].partner - 1), shore);
    else LOGN("YOU Practice: %s, to the sea battle (shore %d)", SQN(p->sel), shore);
}

static void handle_key(Play *p, int key)     /* campaign_input (ovl4:7B65) */
{
    if (p->mode == PM_PROMPT) {
        if (p->ended) {
            if (key == PK_ENTER) { p->result = p->outcome; LOGN("YOU Enter: the game ends"); }
            else cursor_input(p, key);
        } else if (p->prompt_surrender) {       /* campaign_play ovl4:7AB1 */
            if (key == PK_Y) {
                LOGN("YOU surrender (Y)");
                p->prompt_surrender = false;
                text_reset(p);
                clear_row(p, 24);
                print(p, SO5(p, 0x47));         /* Your opponent accepts your surrender */
                p->ended = true;
                p->outcome = PLAY_LOST;
                p->result_code = 11;
            } else if (key == PK_N) {
                LOGN("YOU don't surrender after all (N)");
                p->prompt_surrender = false;
                p->mode = PM_NORMAL;
                cmd_normal(p);
            }
        } else if (p->prompt_quit) {            /* the remake's Esc */
            if (key == PK_Y) {
                LOGN("YOU quit to the title page (Y)");
                p->prompt_quit = false;
                p->result = PLAY_QUIT;
            } else if (key == PK_N || key == PK_ESC) {
                LOGN("YOU carry on (%s)", gamelog_key(key));
                p->prompt_quit = false;
                p->mode = PM_NORMAL;
                cmd_normal(p);
                p->over_squad = false;
                check_over(p, 0);
            }
        }
        return;
    }
    if (p->mode == PM_INFO) { if (key) info_key(p, key); return; }
    if (p->mode == PM_FLEET || p->mode == PM_PAUSE) {   /* any key */
        if (key) {
            beep(p);
            cmd_normal(p);
        }
        return;
    }
    if (key == PK_ESC && (p->mode == PM_NORMAL || p->mode == PM_SQUAD)) {   /* the remake: leave the game */
        LOGN("YOU Esc: quit to the title page?");
        text_reset(p);
        clear_row(p, 24);
        print(p, "Quit to the title page? (Y or N)");
        p->mode = PM_PROMPT;
        p->prompt_quit = true;
        return;
    }
    if (p->mode == PM_NORMAL) {
        if (key == PK_ALT_S) {                  /* Save: campaign_input ovl4:7BB8 returns 100 */
            LOGN("YOU save the game (Alt+S)");
            beep(p);
            clear_row(p, 24);
            tpos(p, 24, 10);
            print(p, DS(p, 0x6A));              /* One moment, please */
            p->result = PLAY_SAVE;
            return;
        }
        if (key == PK_S) {                      /* Surrender: campaign_play ovl4:7A83 asks */
            LOGN("YOU Surrender?");
            beep(p);
            clear_row(p, 23);
            clear_row(p, 24);
            print(p, SO5(p, 0x22));             /* Surrender -- Are you sure? (Y or N) */
            p->mode = PM_PROMPT;
            p->prompt_surrender = true;
            return;
        }
        if (key == PK_T) {                      /* Time: slow, medium, fast, faster */
            p->time_div >>= 2;
            if (p->time_div < 1) p->time_div = 64;
            LOGN("YOU Time: %s (a tick each %d iterations)", p->time_div == 64 ? "slow" : p->time_div == 16 ? "medium" :
                 p->time_div == 4 ? "fast" : "faster", p->time_div);
            beep(p);
            show_time(p);
        } else if (key == PK_F) {               /* Fleet */
            LOGV("YOU Fleet");
            show_fleet(p);
            return;
        } else if (key == PK_C && p->msg_rows) {   /* Clear the messages */
            beep(p);
            text_reset(p);
            show_clear(p);
        } else if (key == PK_P) {               /* Pause */
            LOGV("YOU Pause");
            beep(p);
            clear_row(p, 24);
            print(p, DS(p, 0x1673));            /* Press any key to continue. */
            p->mode = PM_PAUSE;
            return;
        }
    } else if (p->mode == PM_SQUAD) {
        const PSquad *q = &p->sq[p->sel];
        if (key == PK_I) { LOGV("YOU Info: %s", SQN(p->sel)); show_info(p); return; }
        if (key == PK_D && p->free_slot > -1 && q->side == -1 && !q->partner) {   /* Detach */
            list_ships(p, p->sel);
            if (p->list[1] != -1) { dj_start(p, false); return; }
        }
        if (key == PK_J && q->side == -1 && !q->partner) {                       /* Join */
            p->free_slot = join_partner(p, p->sel) - 1;
            if (p->free_slot > -1) { dj_start(p, true); return; }
        }
        if (key == PK_Z && q->partner && q->ctrl != 0 && q->ctrl != -1) { battle_ask(p, 1); return; }   /* Zoom */
        if (key == PK_P && !q->partner && q->ctrl == -2 && commandable(p, p->sel)) {               /* Practice */
            p->sq[p->sel].port_speed = q->speed;
            battle_ask(p, 2);
            return;
        }
        if (key == PK_M && q->side == -1 && !q->partner) { start_trace(p); return; }
    } else if (p->mode == PM_TRACE) {
        if (key == PK_C) {                      /* Cancel */
            LOGN("YOU Cancel: %s stays where it is", SQN(p->sel));
            beep(p);
            cancel_path(p);
            p->mode = PM_NORMAL;
            cmd_normal(p);
            p->over_squad = false;
            check_over(p, 0);
            delay(p, 5000);
        } else if (key == PK_M) {               /* Mark: the end of the path */
            end_trace(p);
            delay(p, 5000);
        }
    }
    cursor_input(p, key);
}

static void start_rest(Play *p);

/* ovl4:A0E7: Y or N to the opponent's offer */
static void offer_key(Play *p, int key)
{
    if (key == PK_Y) {
        LOGN("YOU accept the enemy's surrender (Y)");
        beep(p);
        print(p, DS(p, 0x1DA));              /* Yes */
        p->result_code = 1;
        if (p->offer_ends) { p->outcome = PLAY_WON; p->result = PLAY_WON; return; }
    } else if (key == PK_N) {
        LOGN("YOU refuse the enemy's surrender (N)");
        beep(p);
        print(p, DS(p, 0x1DE));              /* No */
    } else return;
    clear_panel(p);
    p->mode = p->back_mode;
    if (p->offer_at_start) start_rest(p);
    else check_over(p, 0);
}

/* ovl4:A18A: the blinking; a key is seen at the end of a blink */
static void end_frame(Play *p, int key)
{
    int half = (int)(7000 * DELAY_UNIT_US * 1e-6 * PLAY_HZ + 0.5);
    if (key && !p->end_key) p->end_key = key;
    int f = p->end_frame % (2 * half);
    if (f == 0) {
        if (p->end_key) {
            int k = p->end_key;
            p->end_key = 0;
            p->mode = PM_PROMPT;
            p->ended = true;
            if (p->end_quit) { p->result = p->outcome; return; }
            handle_key(p, k);
            return;
        }
        clear_row(p, 24);
    } else if (f == half) {
        tpos(p, 24, p->end_won == 1 ? 2 : 3);
        print(p, SO5(p, p->end_won == 1 ? 0x278 : 0x282));   /* You Lost / You Won */
        print(p, SO5(p, 177));               /* ...   to continue. */
        row_icon(p, 24, 29, 0x62B);          /* Enter */
    }
    p->end_frame++;
}

/* ------------------------------------------------------------------ public */

/* campaign_play ovl4:7876: the command line, the marker, the empty ports and the wind */
static void start_rest(Play *p)
{
    p->offer_at_start = false;
    if (p->mode != PM_END) {
        cmd_normal(p);
        check_over(p, 0);
    }
    for (int cl = 0; cl < NUM_CELLS; cl++)   /* ovl4_c21c */
        if (p->map[cl] >= 74 && p->map[cl] <= 76 && p->port_sup[cl] == 0) invert(p, cl);
    end_sound(p);                            /* ovl4:78C4: a new game's third */
    p->wind = -1;
    wind_change(p, false);
}

/* ovl1_7a3f: a side with no flagship and no crown on the map gets one */
static void promote_flagship(Play *p, int side)
{
    bool frigates = false;
    for (int guard = 0; guard < 10000; guard++) {
        for (int s = 0; s < MAX_SHIPS; s++) {
            PShip *sh = &p->ship[s];
            if (sh->sq < 0 || p->sq[sh->sq].side != side) continue;
            if (rnd(p) < 115) continue;
            if (!frigates && sh->type == 2) continue;
            sh->type = 3;
            LOGN("EVENT %s has no flagship and no crown: %s becomes a flagship", side == -1 ? "your side" : "the enemy",
                 gamelog_ship(p, s));
            return;
        }
        if (rnd(p) < 40) frigates = true;
    }
}

static bool map_has(const Play *p, int tile)   /* rt_1953 */
{
    for (int c = 0; c < NUM_CELLS; c++) if (p->map[c] == tile) return true;
    return false;
}

/* game_init (ovl1:77AD) and campaign_play's start (ovl4:7650) */
static void load_pause(Play *p, const uint32_t *before);
static const double EV_WEIGHT[EV_COUNT];

static void start(Play *p, const Campaign *c, const GameAssets *ga, uint16_t seed, uint16_t ptr, bool demo)
{
    uint32_t before[EV_COUNT];
    SDL_memcpy(before, play_ev, sizeof before);
    SDL_memset(p, 0, sizeof *p);
    p->c = c;
    p->ga = ga;
    p->rnd = seed;
    p->rnd_ptr = ptr >= 0xBB8 && ptr < 0x2328 ? ptr : 0xBB8;
    p->demo = demo;
    for (int i = 0; i < MAX_SHIPS; i++) {
        const Squadron *s = &c->squadrons[i];
        PSquad *q = &p->sq[i];
        q->used = s->present;
        q->side = s->friendly ? -1 : 0;
        q->speed = 0;
        q->x = s->x;
        q->y = s->y;
        q->supplies = s->supplies;
        const Ship *sh = &c->ships[i];
        PShip *ps = &p->ship[i];
        ps->sq = sh->squadron < MAX_SHIPS ? sh->squadron : -1;
        ps->sails = sh->sails_damage;
        ps->hull = sh->hull_damage;
        ps->type = sh->type;
    }
    for (int r = 0; r < MAP_H; r++)
        for (int col = 0; col < MAP_W; col++) p->map[r * 10 + col] = c->map[r][col];
    for (int k = 0; k < NUM_RULES; k++) p->rules[k] = (int16_t)c->rules[k];
    if (ga->have_so2) SDL_memcpy(p->opp, ga->opp[SDL_clamp(c->opponent, 0, 5)], sizeof p->opp);
    if (demo) { OPP(p, 0x604F) = 3; OPP(p, 0x6057) = 128; p->rules[5] = 80; }   /* ovl1:8894 */
    p->fight_delay = assets_scnf_word(ga, 0x6851);
    if (p->fight_delay <= 6) p->fight_delay = 500;
    p->wind_on = assets_scnf_word(ga, 0x6865) != 0;

    /* game_init */
    p->view_top = VIEW_TOP_MAX;
    p->cx = 38;
    p->cy = 145;
    int enemies = 0;
    for (int i = 0; i < MAX_SHIPS; i++) {
        PSquad *q = &p->sq[i];
        q->dest = NO_DEST;
        q->merchant_dest = -1;
        q->ctrl = -2;
        q->mark = 0;
        if (q->side == 0 && q->used) enemies++;
    }
    if (OPP(p, 0x6073) > enemies >> 1) OPP(p, 0x6073) = (int16_t)(enemies >> 1);
    p->time_div = demo ? 1 : 64;             /* the demo: the fastest time */
    p->crown_threat = -1;
    int yours = 0, theirs = 0;
    for (int s = 0; s < MAX_SHIPS; s++) {
        if (p->ship[s].sq < 0 || p->ship[s].type != 3) continue;
        if (p->sq[p->ship[s].sq].side == -1) yours++;
        else theirs++;
    }
    if (yours == 0) {
        if (map_has(p, 77)) p->no_flags[0] = true;
        else promote_flagship(p, -1);
    }
    if (theirs == 0) {
        if (map_has(p, 76)) p->no_flags[1] = true;
        else promote_flagship(p, 0);
    }
    do p->wind = rnd(p) & 7; while (p->wind > 3);
    if (!p->wind_on) p->wind = 0;
    for (int cl = 0; cl < NUM_CELLS; cl++) {
        int t = p->map[cl];
        if (t == 75 || t == 76) p->ports_supply++;
        if (t == 74 || t == 76) p->ports_repair++;
        if (t == 76) p->crowns++;
    }

    /* campaign_play */
    for (int r = 0; r < 25; r++) clear_row(p, r);
    p->text_rows = 2;
    mark_flagships(p);
    wind_change(p, true);
    coast_init(p);
    enemy_strength_map(p);
    enemy_port_values(p);
    p->battle_a = p->battle_b = -1;
    flags_check(p);
    end_sound(p);                            /* play_end_sound (ovl4:76EB), then the enemy's first orders */
    uint32_t work0[EV_COUNT];
    SDL_memcpy(work0, play_ev, sizeof work0);
    port_sources(p);
    enemy_turn(p);
    for (int i = 0; i < MAX_SHIPS; i++) {    /* squadrons put on land are lost */
        PSquad *q = &p->sq[i];
        if (!q->used) continue;
        int t = p->map[play_cell(p, i)];
        bool gone = false;
        if (tile_land(t) && (t < 40 || t > 54)) gone = true;
        else for (int off = 160; off < 320; off += 80)
            if (land_byte(buf_byte(p, sq_addr(p, i) + off))) gone = true;
        if (!gone) continue;
        LOGN("EVENT %s was put on land: it is lost", SQN(i));
        q->used = false;
        list_ships(p, i);
        for (int k = 0; k < 3 && p->list[k] != -1; k++) p->ship[p->list[k]].sq = -1;
    }
    double work = 0;                         /* the sounds wait for that work */
    for (int k = 0; k < EV_COUNT; k++) work += EV_WEIGHT[k] * (double)(play_ev[k] - work0[k]);
    p->snd_us = snd_t(p) + work * (1000.0 / 300.0);
    end_sound(p);                            /* ovl4:7800 */
    if (demo) p->demo_key = PK_KP_MINUS;     /* ovl4:7803: it scrolls up */
    port_supplies(p);
    p->sel = -1;
    p->free_slot = -1;
    end_check(p, p->battle_b, p->battle_a);
    if (p->mode == PM_OFFER) p->offer_at_start = true;   /* the rest comes after the answer */
    else start_rest(p);
    gamelog_state(p, "The game at the start");
    load_pause(p, before);
}

void play_start(Play *p, const Campaign *c, const GameAssets *ga, unsigned seed)
{
    start(p, c, ga, (uint16_t)(seed & 0xFF), 0xBB8, false);   /* seed_random (rt:2EF9): the BIOS clock */
}

void play_start_rnd(Play *p, const Campaign *c, const GameAssets *ga, uint16_t rnd_, uint16_t ptr)
{
    start(p, c, ga, rnd_, ptr, false);
}

void play_start_demo(Play *p, const Campaign *c, const GameAssets *ga, uint16_t rnd_, uint16_t ptr)
{
    start(p, c, ga, rnd_, ptr, true);
}

int  play_random(Play *p) { return rnd(p); }
void play_list_ships(Play *p, int sq) { list_ships(p, sq); }
void play_clear_paths(Play *p, int sq) { clear_paths(p, sq); }

/* campaign_play (ovl4:7650) entered again after a sea battle: the counts, the end of
 * the game (end_check with the battle's squadrons, unless you left it), and the map's
 * command line; the enemy's squadron in the battle takes on ships beside it (ovl4_ed83). */
void play_after_battle(Play *p, bool practice, bool left)
{
    uint32_t before[EV_COUNT];
    SDL_memcpy(before, play_ev, sizeof before);
    (void)practice;
    p->battle_req = 0;
    mark_flagships(p);
    p->mode = PM_NORMAL;
    p->key = 0;
    p->keybuf_n = 0;
    p->msg_last = 0;
    p->msg_rows = 0;
    p->text_rows = 2;
    for (int r = 0; r < 25; r++) clear_row(p, r);
    clear_panel(p);
    coast_init(p);
    enemy_strength_map(p);
    enemy_port_values(p);
    gamelog_grid(p, "The strength map worked out again (campaign play starts again):", p->strength);
    if (!left) end_check(p, p->battle_b, p->battle_a);
    if (p->mode != PM_END && p->battle_a >= 0 && p->sq[p->battle_a].used && p->sq[p->battle_a].side == 0 && !left) {
        p->ai_sq = p->battle_a;              /* ovl4_ed83: the enemy's squadron takes on ships beside it */
        enemy_join(p);
    }
    p->over_squad = false;
    if (p->mode != PM_END && p->mode != PM_OFFER) {
        cmd_normal(p);
        check_over(p, 0);
    }
    for (int cl = 0; cl < NUM_CELLS; cl++)   /* the map buffer is drawn again, then ovl4_c21c */
        p->inverted[cl] = p->map[cl] >= 74 && p->map[cl] <= 76 && p->port_sup[cl] == 0;
    text_rows_add(p, 0);
    load_pause(p, before);
}

void play_resume(Play *p)
{
    p->result = PLAY_RUNNING;
    p->ended = p->prompt_quit = p->prompt_surrender = false;
    p->stall = 0;
    p->work_us = 0;
    p->tick_stage = 0;
    p->battle_req = 0;
    play_after_battle(p, false, false);
}

void play_key(Play *p, int key, bool shift)
{
    if (p->key == 0 && p->keybuf_n == 0) {
        p->jump = shift;
        p->key = key;
    } else if (p->keybuf_n < 15) {          /* a full buffer drops the key (the BIOS beeps) */
        p->keybuf[p->keybuf_n] = key;
        p->keybuf_shift[p->keybuf_n++] = shift;
    }
}

/* the next key from the buffer, once this iteration has read its own */
static void next_key(Play *p)
{
    if (p->key != 0 || p->keybuf_n == 0) return;
    p->key = p->keybuf[0];
    p->jump = p->keybuf_shift[0];
    p->keybuf_n--;
    SDL_memmove(p->keybuf, p->keybuf + 1, (size_t)p->keybuf_n * sizeof p->keybuf[0]);
    SDL_memmove(p->keybuf_shift, p->keybuf_shift + 1, (size_t)p->keybuf_n * sizeof p->keybuf_shift[0]);
}

/* What the counted events cost in 8086 instructions: fitted to WAS.EXE's own
 * routines run under an emulator (WAS_Remake/test/diff_ovl4, calib.py: the
 * enemy's orders, encounters, sailing, eating, map battles, restocking and the
 * strength map, about 2,200 calls over the 11 campaigns; the fit is within
 * about 10% on average, the route searches included). EV_BASE carries each
 * routine's own fixed part. */
static const double EV_WEIGHT[EV_COUNT] = {
    [EV_RND] = 14.9, [EV_LIST] = 10.7, [EV_SMALL] = 81.9, [EV_HELP] = 0, [EV_PORT] = 593.1, [EV_S40] = 21.7,
    [EV_C200] = 8.6, [EV_QUEUE] = 16.0, [EV_NEIGH] = 137.6, [EV_STEP] = 20.3, [EV_RING] = 4.0,
    [EV_PRING] = 27.2, [EV_TILE] = 117.0, [EV_BASE] = 1,
    /* sea_way (ovl4:E5BF), fitted the same way to port_sources (ovl4:C07C) */
    [EV_SEAQ] = 48.4, [EV_SEAPOP] = 947.7, [EV_SEAB] = 321.3,
};
/* what an iteration with no tick and no fight counts already: the 27
 * iterations a second measured in DOSBox include it */
#define WORK_IDLE 868

/* campaign_play's start (ovl4:7650): loading SM and SO5 and drawing the map
 * took this long besides the counted work (DOSBox at 300 cycles: The Bane of
 * Oldain 7.2 s from the quote to the map, The Race for the Crown 4.9 s) */
#define LOAD_US 2050000.0

/* The pause before the map appears: the counted work since before, the
 * loading, and play_end_sound's 7500 delay units each time it plays. The last
 * page of the menus (or of the battle) stays meanwhile (main.c: hidden). */
static void load_pause(Play *p, const uint32_t *before)
{
    double n = 0;
    for (int k = 0; k < EV_COUNT; k++) n += EV_WEIGHT[k] * (double)(play_ev[k] - before[k]);
    double us = LOAD_US + n * (1000.0 / 300.0);     /* play_end_sound's delays are in the stall already */
    p->stall += (int)(us / (1e6 / PLAY_HZ) + 0.5);
    p->hidden = true;
}

/* the work counted since before, on an XT at 300 instructions a millisecond,
 * as iterations stalled (as WAS.EXE's delay(): keys wait). The frame's own
 * time (idle_left) is set against it first: an iteration's idle work (the 27
 * iterations a second measured include it), or the whole frame when a tick
 * goes on (tick_go). */
static void work_time(Play *p, uint32_t *before)
{
    double n = 0;
    for (int k = 0; k < EV_COUNT; k++) n += EV_WEIGHT[k] * (double)(play_ev[k] - before[k]);
    SDL_memcpy(before, play_ev, sizeof play_ev);
    if (n <= p->idle_left) { p->idle_left -= n; return; }
    n -= p->idle_left;
    p->idle_left = 0;
    p->work_us += n * (1000.0 / 300.0);
    const double iter = 1e6 / PLAY_HZ;
    int it = (int)(p->work_us / iter);
    p->stall += it;
    p->work_us -= it * iter;
}

static void iteration(Play *p, int held, uint32_t *before);
static void tick_go(Play *p, int held, uint32_t *before);

void play_frame(Play *p, int held)
{
    if (p->result != PLAY_RUNNING) return;
    p->snd_frames++;
    gamelog_iteration(p);
    if (p->stall > 0) { p->stall--; return; }   /* the key waits, as in the keyboard buffer */
    p->hidden = false;
    uint32_t before[EV_COUNT];
    SDL_memcpy(before, play_ev, sizeof before);
    bool going_on = p->tick_stage != 0;
    if (going_on) {                          /* the tick goes on: this frame is its time */
        p->idle_left = 1e6 / PLAY_HZ * 0.3;
        tick_go(p, held, before);
    } else {
        p->idle_left = WORK_IDLE;
        iteration(p, held, before);
    }
    next_key(p);
    work_time(p, before);
    if (going_on && p->tick_stage == 0 && p->idle_left > 0) {   /* the tick ended within this frame: */
        p->work_us -= p->idle_left * (1000.0 / 300.0);        /* the rest of it goes to the next work */
        if (p->work_us < -1e6 / PLAY_HZ) p->work_us = -1e6 / PLAY_HZ;
    }
}

bool play_busy(const Play *p) { return p->stall > 0 || p->tick_stage != 0; }

/* poll_input (ovl4:8645) in the demo: the key it plays goes on, and now
 * and then (random() <= 4) it scrolls the other way */
static int demo_poll(Play *p)
{
    int k = rnd(p) > 4 ? p->demo_key : p->rnd > 1 ? PK_KP_MINUS : PK_KP_PLUS;
    p->demo_key = k;
    return k;
}

/* campaign_input (ovl4:7A83) */
static void input_part(Play *p, int key)
{
    if (p->demo == 2 && (p->mode == PM_NORMAL || p->mode == PM_SQUAD)) {   /* campaign_input ovl4:7B76: Zoom */
        if (p->sel >= 0 && p->sel < MAX_SHIPS) {
            p->demo_key = key = PK_Z;
            p->mode = PM_SQUAD;
        } else p->mode = PM_NORMAL;          /* the squadron has gone (its fight is over) */
    }
    handle_key(p, key);
}

/* The tick's parts from p->tick_stage on. A part that leaves work to wait
 * for stops it here (play_frame goes on with the next part after the stall);
 * before each part after the first, key_waiting (ovl4:85F8: a key held down,
 * or one pressed meanwhile) goes straight to campaign_input, leaving the rest
 * of the tick undone, as WAS.EXE does (ovl4:7984-79F4). */
static void tick_go(Play *p, int held, uint32_t *before)
{
    while (p->tick_stage) {
        int s = p->tick_stage;
        if (s > TS_START && (held || p->key || p->keybuf_n)) break;
        p->tick_stage = s == TS_REST ? 0 : s + 1;
        if (tick_part(p, s) || tick_over(p)) { p->tick_stage = 0; return; }
        work_time(p, before);
        if (p->stall > 0 && p->tick_stage) return;
    }
    p->tick_stage = 0;
    input_part(p, 0);
}

static void iteration(Play *p, int held, uint32_t *before)
{
    int real = p->key ? p->key : held;
    int key = real;
    p->key = 0;
    if (p->demo) {
        if (p->mode == PM_END || p->ended) {     /* the remake: the end of a demo game stays 3 s */
            if (++p->demo_end > 3 * PLAY_HZ) p->result = PLAY_QUIT;
            if (p->mode == PM_END) end_frame(p, 0);
            return;
        }
        key = demo_poll(p);
    }
    switch (p->mode) {
    case PM_END:    end_frame(p, key); return;
    case PM_OFFER:  offer_key(p, key); return;
    case PM_DETACH: if (key) dj_key(p, key); return;
    case PM_INFO: case PM_FLEET: case PM_PAUSE: handle_key(p, key); return;
    default: break;
    }
    if (!real) {                             /* key_waiting, then ovl4_8ede */
        map_battles(p);
        if (p->mode == PM_END || p->mode == PM_OFFER) return;
    }
    p->tick_count++;
    /* a key read by poll_input (DS:6896) skips the tick (ovl4:792B) */
    if (key == 0 && (p->mode == PM_NORMAL || p->mode == PM_SQUAD) && p->tick_count >= p->time_div) {
        p->tick_stage = TS_START;
        tick_go(p, held, before);
        return;
    }
    input_part(p, key);
}

/* ------------------------------------------------------------------ drawing */

typedef struct {
    Screen *s;
    int     top, h;          /* first map line shown, lines shown */
    uint8_t col[4];          /* CGA colours 0-3 in the screen palette */
    bool    tandy;           /* the Tandy version's (WAST.EXE) marker: yellow */
} View;

static void vpx(const View *v, int x, int line, int c)
{
    int sy = line - v->top;
    if (x < 0 || x >= SCREEN_W || sy < 0 || sy >= v->h) return;
    v->s->px[sy * SCREEN_W + x] = v->col[c];
}

/* a pixel of the Tandy version's own colour (0-15) */
static void vpx16(const View *v, int x, int line, int c)
{
    int sy = line - v->top;
    if (x < 0 || x >= SCREEN_W || sy < 0 || sy >= v->h) return;
    v->s->px[sy * SCREEN_W + x] = (uint8_t)c;
}

static int vget(const View *v, int x, int line)
{
    int sy = line - v->top;
    if (x < 0 || x >= SCREEN_W || sy < 0 || sy >= v->h) return -1;
    return v->s->px[sy * SCREEN_W + x];
}

/* rt_280e: the marker, a cross. The Tandy version's (WAST draw_cursor rt:0231A)
 * is yellow (EEh), and its middle's two untouched pixels lose their bit 0
 * (AND E0EEh after OR E0h) */
static void draw_cursor(const View *v, int cx, int cy)
{
    int x = cx * 4, l = cy * 2;
    static const int arm[6] = { 0, 1, 2, 7, 8, 9 };
    for (int k = 0; k < 6; k++)
        for (int i = 0; i < 4; i++) {
            if (v->tandy) vpx16(v, x + i, l + arm[k], i < 2 ? 0 : 14);
            else vpx(v, x + i, l + arm[k], i < 2 ? 0 : 3);
        }
    for (int k = 4; k <= 5; k++) {
        static const int8_t bar[12] = { 0, 0, 3, 3, 3, -1, -1, 0, 0, 3, 3, 3 };
        for (int i = 0; i < 12; i++) {
            if (!v->tandy) { if (bar[i] >= 0) vpx(v, x - 4 + i, l + k, bar[i]); continue; }
            if (bar[i] >= 0) vpx16(v, x - 4 + i, l + k, bar[i] ? 14 : 0);
            else { int c = vget(v, x - 4 + i, l + k); if (c >= 0) vpx16(v, x - 4 + i, l + k, c & 0xE); }
        }
    }
}

/* rt_280e while tracing: a box on the even lines of five line pairs below the marker */
static void draw_trace_box(const View *v, int cx, int cy)
{
    int x = cx * 4;
    for (int k = 0; k < 5; k++) {
        int line = 2 * (cy + 1 + k);
        if (k == 0 || k == 4) {
            static const uint8_t edge[8] = { 0, 0, 3, 3, 3, 3, 0, 0 };
            for (int i = 0; i < 8; i++) {
                if (v->tandy) vpx16(v, x + i, line, edge[i] ? 14 : 0);
                else vpx(v, x + i, line, edge[i]);
            }
        } else if (v->tandy) {                 /* WAST: the sides XOR 77h, each pixel's colour XOR 7 */
            static const int sx[4] = { 0, 1, 6, 7 };
            for (int i = 0; i < 4; i++) { int c = vget(v, x + sx[i], line); if (c >= 0) vpx16(v, x + sx[i], line, c ^ 7); }
        } else {
            vpx(v, x, line, 3); vpx(v, x + 1, line, 3);
            vpx(v, x + 6, line, 3); vpx(v, x + 7, line, 3);
        }
    }
}

/* rt_2ab8: a little cross where a traced path ends */
static void draw_dest(const View *v, int addr)
{
    int x = (addr % 80) * 4, l = (addr / 80) * 2;
    for (int j = 0; j < 6; j++)
        for (int i = 0; i < 4; i++)
            if ((j == 2 || j == 3) || i == 1 || i == 2) vpx(v, x + i, l + j, 3);
}

/* one CGA byte (4 pixels) at a screen line */
static void cga_byte(Screen *s, int line, int xbyte, uint8_t b, const uint8_t col[4])
{
    if (line < 0 || line >= SCREEN_H || xbyte < 0 || xbyte >= 80) return;
    for (int i = 0; i < 4; i++) s->px[line * SCREEN_W + xbyte * 4 + i] = col[(b >> (6 - 2 * i)) & 3];
}

static void xor_block(Screen *s, int line, int xbyte, int lines, const uint8_t col[4])   /* rt_2dc8 */
{
    for (int j = 0; j < lines; j++) {
        int y = line + j;
        if (y < 0 || y >= SCREEN_H) continue;
        for (int i = 0; i < 4; i++) {
            uint8_t *px = &s->px[y * SCREEN_W + xbyte * 4 + i];
            int c = 0;
            for (int k = 0; k < 4; k++) if (col[k] == *px) c = k;
            *px = col[3 - c];
        }
    }
}

/* rt_2b71: n line pairs of 0Fh. The Tandy version (WAST draw_vline rt:0277B in
 * mode 9) writes only the byte of the two right pixels (FFh), so the left two
 * keep what was there */
static void vline_t(Screen *s, int line, int xbyte, int n, const uint8_t col[4], bool tandy)
{
    for (int j = 0; j < n * 2; j++) {
        if (!tandy) { cga_byte(s, line + j, xbyte, 0x0F, col); continue; }
        if (line + j < 0 || line + j >= SCREEN_H || xbyte < 0 || xbyte >= 80) continue;
        s->px[(line + j) * SCREEN_W + xbyte * 4 + 2] = s->px[(line + j) * SCREEN_W + xbyte * 4 + 3] = col[3];
    }
}
#define vline(s, line, xbyte, n, col) vline_t(s, line, xbyte, n, col, tandy)

static void hline(Screen *s, int line, int xbyte, int len, const uint8_t col[4])   /* draw_hline (rt:2B95) */
{
    for (int i = 0; i < len; i++) cga_byte(s, line, xbyte + i, 0xFF, col);
}

static void ds_icon(Screen *s, const GameAssets *ga, int line, int xbyte, unsigned off, const uint8_t col[4])   /* draw_icon_8x6 */
{
    for (int r = 0; r < 6; r++)
        for (int b = 0; b < 2; b++)
            if (off + 2 * r + b < DS_SIZE) cga_byte(s, line + r, xbyte + b, ga->ds[off + 2 * r + b], col);
}

static void ds_icon_sized(Screen *s, const GameAssets *ga, int line, int xbyte, unsigned off, const uint8_t col[4])   /* rt_2cc4 */
{
    if (off + 2 > DS_SIZE) return;
    int w = ga->ds[off], h = ga->ds[off + 1] * 2;
    for (int r = 0; r < h; r++)
        for (int b = 0; b < w; b++)
            if (off + 2 + r * w + b < DS_SIZE) cga_byte(s, line + r, xbyte + b, ga->ds[off + 2 + r * w + b], col);
}

/* rt:2B2A. t16 >= 0: the Tandy version's (WAST rt:0272D), a solid colour */
static void draw_bar(Screen *s, int line, int xbyte, int len, uint8_t pat, const uint8_t col[4], int t16)
{
    for (int j = 0; j < 3; j++) {
        for (int i = 0; i < 21; i++) cga_byte(s, line + j, xbyte + i, 0, col);
        for (int i = 0; i < len; i++) {
            if (t16 < 0) cga_byte(s, line + j, xbyte + i, pat, col);
            else if (line + j >= 0 && line + j < SCREEN_H && xbyte + i >= 0 && xbyte + i < 80)
                SDL_memset(&s->px[(line + j) * SCREEN_W + (xbyte + i) * 4], t16, 4);
        }
    }
}

/* squadron_info (rt:074B): the panel over the map */
static void info_graphics(const Play *p, Screen *s, const uint8_t col[4], bool tandy)
{
    for (int y = 62; y < 62 + 74; y++)                   /* rt_2e04 */
        for (int b = 5; b < 75; b++) cga_byte(s, y, b, 0, col);
    vline(s, 62, 4, 37, col);   vline(s, 62, 34, 24, col);
    vline(s, 62, 50, 24, col);  vline(s, 62, 74, 37, col);
    vline(s, 110, 23, 5, col);
    vline(s, 120, 50, 3, col);  vline(s, 120, 54, 2, col);
    vline(s, 120, 59, 3, col);  vline(s, 120, 64, 2, col);
    vline(s, 120, 70, 3, col);
    static const int HL[5] = { 62, 72, 110, 120, 136 };
    for (int k = 0; k < 5; k++) hline(s, HL[k], 5, 70, col);
    static const struct { int x; unsigned off; } SCALE[10] = {   /* 0 25 50 75 100 */
        { 50, 0x5EF }, { 53, 0x607 }, { 55, 0x613 }, { 58, 0x613 }, { 60, 0x5EF },
        { 63, 0x61F }, { 65, 0x613 }, { 68, 0x5FB }, { 70, 0x5EF }, { 72, 0x5EF },
    };
    for (int k = 0; k < 10; k++) ds_icon(s, p->ga, 128, SCALE[k].x, SCALE[k].off, col);
    for (int k = 0; k < 3 && p->list[k] != -1; k++) {
        const PShip *sh = &p->ship[p->list[k]];
        int len = (5 - sh->sails) * 4;
        draw_bar(s, k * 8 + 81, 51, len > 0 ? len : 1, 0xDD, col, tandy ? 14 : -1);   /* WAST: EEEEh, yellow */
        len = (5 - sh->hull) * 4;
        draw_bar(s, k * 8 + 84, 51, len > 0 ? len : 1, 0x11, col, tandy ? 6 : -1);    /* 6666h, brown */
    }
    int sup = SDL_clamp(p->sq[p->sel].supplies, 0, 21);
    draw_bar(s, 112, 51, sup, 0x55, col, tandy ? 10 : -1);                           /* AAAAh, light green */
    draw_bar(s, 115, 51, sup, 0x55, col, tandy ? 10 : -1);
}

/* The Tandy version prints SAIL, HULL and SUPPLIES in colour (squadron_info
 * rt:006BC sets the text colour DS:2E2D: EEEEh, 6666h, AAAAh): the white
 * pixels of those cells */
static void info_text_colours(const Play *p, Screen *s)
{
    static const struct { int row, col, n; uint8_t c; } T[3] = { { 8, 27, 4, 14 }, { 8, 32, 4, 6 }, { 14, 14, 8, 10 } };
    for (int k = 0; k < 3; k++)
        for (int c = T[k].col; c < T[k].col + T[k].n; c++) {
            if (!p->panel[T[k].row][c]) continue;
            for (int y = 0; y < 8; y++)
                for (int x = 0; x < 8; x++) {
                    uint8_t *q = &s->px[(T[k].row * 8 + y) * SCREEN_W + c * 8 + x];
                    if (*q == 15) *q = T[k].c;
                }
        }
}

/* ovl4_a347: the Detach / Join panel (rt_2e26 and the lines) */
static void detach_graphics(Screen *s, const uint8_t col[4], bool tandy)
{
    vline(s, 60, 10, 24, col);
    vline(s, 60, 40, 24, col);
    vline(s, 60, 71, 24, col);
    hline(s, 60, 11, 61, col);
    hline(s, 76, 11, 61, col);
    hline(s, 108, 11, 61, col);
}

/* the map tiles, with the wind's face and the inverted ports */
static void draw_tiles(const Play *p, Screen *s, GfxMode mode, int top, int h)
{
    const AssetSet *set = &p->ga->set[mode];
    screen_set_palette(s, set->tiles[0].pal, set->tiles[0].ncolors);
    screen_fill(s, 0, 0, SCREEN_W, h, 0);
    int inv = set->tiles[0].ncolors - 1;
    for (int sy = 0; sy < h; sy++) {
        int line = top + sy;
        if (line >= MAP_PIXEL_H) break;
        int row = line / TILE_H, ty = line % TILE_H;
        for (int col = 0; col < MAP_W; col++) {
            int cell = row * 10 + col, t = shown_tile(p, cell) - 1;
            if (t < 0 || t >= NUM_TILES || !set->tiles[t].px) continue;
            const uint8_t *src = &set->tiles[t].px[ty * TILE_W];
            uint8_t *dst = &s->px[sy * SCREEN_W + col * TILE_W];
            if (p->inverted[cell]) for (int x = 0; x < TILE_W; x++) dst[x] = (uint8_t)(src[x] ^ inv);
            else SDL_memcpy(dst, src, TILE_W);
        }
    }
}

void play_draw(const Play *p, Screen *s, GfxMode mode)
{
    const AssetSet *set = &p->ga->set[mode];
    int map_h = play_map_h(p);
    draw_tiles(p, s, mode, p->view_top * 2, map_h);

    View v = { s, p->view_top * 2, map_h, { 0, 1, 2, 3 }, mode == MODE_16 };
    int ncolors = set->tiles[0].ncolors;
    for (int i = 0; i < 4; i++)
        v.col[i] = screen_nearest(s, ncolors, CGA_PALETTE[i].r, CGA_PALETTE[i].g, CGA_PALETTE[i].b);
    uint8_t white = v.col[3], black = v.col[0];

    for (int i = 0; i < MAX_SHIPS; i++)      /* rt_1084: the ends of the traced paths */
        if (p->sq[i].dest != NO_DEST && p->sq[i].used) draw_dest(&v, p->sq[i].dest);

    /* draw_squadrons (rt:10D2): the squadron, then the pennant or the cargo a line pair above */
    for (int i = 0; i < MAX_SHIPS; i++) {
        const PSquad *q = &p->sq[i];
        if (!visible(p, i)) continue;
        int px = q->x * 4, py = (q->y + 1) * 2 - v.top;
        int spr = q->partner ? MSPR_BATTLE
                : (q->speed == 0 && !q->has_route) ? MSPR_ANCHOR
                : MSPR_HEADING_0 + (q->heading & 15);
        int dy = (spr >= MSPR_HEADING_0 && spr < MSPR_HEADING_0 + 16) ? p->ga->heading_dy[q->heading & 15] * 2 : 0;
        bool friend = q->side != 0;
        screen_sprite(s, &p->ga->map_sprite[spr], px, py + dy, friend, white, black, 0, map_h);
        if (q->mark == 1) screen_sprite(s, &p->ga->map_sprite[MSPR_PENNANT], px, py - 2, friend, white, black, 0, map_h);
        if (q->mark == 2) screen_sprite(s, &p->ga->map_sprite[MSPR_CARGO], px, py - 2, friend, white, black, 0, map_h);
    }
    if (p->mode == PM_TRACE) draw_trace_box(&v, p->cx, p->cy);
    else if (p->mode != PM_END && !p->ended) draw_cursor(&v, p->cx, p->cy);

    /* rt_2ae4: a white line under the map, then the text rows */
    screen_fill(s, 0, map_h, SCREEN_W, 1, white);
    screen_fill(s, 0, map_h + 1, SCREEN_W, SCREEN_H - map_h - 1, black);
    for (int r = 25 - p->text_rows; r < 25; r++) {
        screen_text(s, p->ga, 0, r * 8, p->text[r], white, black);
        for (int k = 0; k < 2; k++)
            if (p->icon[r][k].off) ds_icon_sized(s, p->ga, r * 8, p->icon[r][k].x, p->icon[r][k].off, v.col);
    }

    bool over = p->mode == PM_INFO || p->mode == PM_DETACH || p->mode == PM_OFFER;
    if (p->mode == PM_INFO) info_graphics(p, s, v.col, v.tandy);
    if (p->mode == PM_DETACH)                /* rt_2e26: the panel is cleared; its lines go over the title */
        for (int y = 60; y < 60 + 48; y++)
            for (int b = 11; b < 71; b++) cga_byte(s, y, b, 0, v.col);
    if (over) {
        char ch[2] = { 0, 0 };
        for (int r = 0; r < 23; r++)          /* put_char only where something was written */
            for (int c = 0; c < 40; c++)
                if (p->panel[r][c]) { ch[0] = p->text[r][c]; screen_text(s, p->ga, c * 8, r * 8, ch, white, black); }
        if (p->mode == PM_INFO && v.tandy) info_text_colours(p, s);
    }
    if (p->mode == PM_DETACH) detach_graphics(s, v.col, v.tandy);
    if (p->mode == PM_DETACH)                /* the ship marked */
        xor_block(s, 80 + 8 * (p->dj_pos % 3), p->dj_pos < 3 ? 11 : 41, 8, v.col);
    if (p->mode == PM_OFFER) {               /* ovl4:A0B5: the box round the offer */
        vline_t(s, 24, 0, 17, v.col, v.tandy);
        vline_t(s, 24, 79, 17, v.col, v.tandy);
        hline(s, 24, 1, 79, v.col);
        hline(s, 56, 1, 79, v.col);
    }
}

/* the tests run single routines against WAS.EXE's (test/diff_ovl4) */
/* The offer to surrender as a game begins: WAS.EXE asks before it draws the
 * map, so the question shows over the last page of the menus */
void play_draw_offer(const Play *p, Screen *s, GfxMode mode)
{
    View v = { s, 0, SCREEN_H, { 0, 1, 2, 3 }, mode == MODE_16 };
    int ncolors = mode == MODE_CGA ? 4 : 16;
    for (int i = 0; i < 4; i++)
        v.col[i] = screen_nearest(s, ncolors, CGA_PALETTE[i].r, CGA_PALETTE[i].g, CGA_PALETTE[i].b);
    char ch[2] = { 0, 0 };
    for (int r = 0; r < 23; r++)
        for (int c = 0; c < 40; c++)
            if (p->panel[r][c]) { ch[0] = p->text[r][c]; screen_text(s, p->ga, c * 8, r * 8, ch, v.col[3], v.col[0]); }
    if (p->mode == PM_OFFER) {               /* ovl4:A0B5: the box round the offer */
        vline_t(s, 24, 0, 17, v.col, v.tandy);
        vline_t(s, 24, 79, 17, v.col, v.tandy);
        hline(s, 24, 1, 79, v.col);
        hline(s, 56, 1, 79, v.col);
    }
}

int play_debug_call(Play *p, int which, int arg)
{
    switch (which) {
    case 0: encounters(p); return 0;
    case 1: return move_squadrons(p);
    case 2: eat(p, arg); return 0;
    case 3: return map_battles(p);
    case 4: restock(p); return 0;
    case 5: return tick(p);
    case 6: port_sources(p); return 0;
    default: return 0;
    }
}
