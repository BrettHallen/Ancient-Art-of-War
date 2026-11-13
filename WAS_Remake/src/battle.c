/*
 * The Ancient Art of War at Sea (1987) - remake
 * Nov 2026 using Claude Opus 5.5
 * "Guided" by Brett Hallen
 * Original game by Dave Murry & Barry Murry
 * Published by Brøderbund Software Inc.
 *
 * battle.c: the sea battle's flow (see battle.h): setting up (ovl6_c9b0),
 * the main loop (ovl5_7650), the keys (ovl5_7ac6), the panel at the top
 * (overlay 6) and the results taken back to the map (ovl6_d5aa).
 *
 * Each function names the routine it ports. Ship arrays are WAS.EXE's
 * (69E7 x, 69F3 y, ...: see battle.h); the pointers into its data segment
 * (sprites, strings, the animation scripts) are kept as DS offsets into the
 * battle's copy of the data segment, b->m.
 */
#include "battle_int.h"
#include "sound.h"
#include "gamelog.h"

#include <SDL3/SDL.h>

/* ------------------------------------------------------------------ the debug log */

bool blog_on(const Battle *b, int level) { return gamelog_on(b->p, (GameLogLevel)level); }

void blog(const Battle *b, const char *fmt, ...)
{
    if (!gamelog_on(b->p, GLOG_NORMAL)) return;
    char t[400];
    va_list ap;
    va_start(ap, fmt);
    SDL_vsnprintf(t, sizeof t, fmt, ap);
    va_end(ap);
    gamelog_printf(b->p, "BATTLE %6.1f s  %s", b->clock_us / 1e6, t);
}

static const char *blog_order(int o)
{
    static const char *const O[7] = { "?", "course", "mimic", "follow the ship in front", "general chase", "avoid battle", "board" };
    return o >= 1 && o <= 6 ? O[o] : o == 888 ? "gone" : "?";
}

const char *blog_str(const Battle *b, int ds)
{
    static char r[8][64];
    static int n;
    n = (n + 1) % 8;
    int k = 0;
    for (int a = ds; a > 0 && a < 0x10000 && b->m[a] && k < 63; a++) r[n][k++] = (char)b->m[a];
    r[n][k] = 0;
    return r[n];
}

#define NAME(i) blog_str(b, b->name[i])

const char *blog_ship(const Battle *b, int i)
{
    static char r[8][96];
    static int n;
    n = (n + 1) % 8;
    if (i < 0 || i >= BS) { SDL_snprintf(r[n], 96, "ship %d", i); return r[n]; }
    SDL_snprintf(r[n], 96, "%s %s (%s, sails %d hull %d)", b->owner[i] == -1 ? "your" : "the enemy's", blog_str(b, b->name[i]),
                 b->type[i] == 1 ? "line" : b->type[i] == 2 ? "frigate" : "flag", b->sails[i], b->hull[i]);
    return r[n];
}

double battle_iter_us = BATTLE_ITER_US;   /* the rest of an iteration's work (calibrated: see battle.c) */

/* ------------------------------------------------------------------ the clock and the keyboard */

static void maybe_yield(Battle *b)
{
    if (b->clock_us >= b->target_us) fiber_yield(&b->fiber);
}

void b_delay(Battle *b, int units)   /* delay() rt:2F6E: it calls random() units / 2 times on an XT */
{
    if (units > 0) b->clock_us += units * DELAY_UNIT_US;
    unsigned n = (unsigned)(units & 0xFFFF) >> 1;
    if (n == 0) n = 1;
    while (n--) play_random(b->p);
    maybe_yield(b);
}

void b_cost(Battle *b, double us)
{
    b->clock_us += us;
    maybe_yield(b);
}

/* rt_2e8e: DOS 0Bh / 08h. A key that DOS gives as 0 then its scan code is
 * returned as 1Bh, then the scan code on the next call. Each look at the
 * keyboard costs a little time, so a loop waiting for a key lets the main
 * loop run. */
int b_getkey(Battle *b)
{
    if (b->key_head == b->key_tail) {
        b_cost(b, 50);
        return 0;
    }
    int k = b->keys[b->key_head];
    b->key_head = (b->key_head + 1) % 64;
    return k;
}

int b_random(Battle *b) { return play_random(b->p); }   /* random() rt:2F0D */
/* the speaker at the battle's clock (sound.c): rt_2f3f with speaker_on before
 * the first note, rt_2f67, speaker_click */
void b_sound(Battle *b, int divisor)
{
    if (!b->spk) { b->beeps++; spk_on(SND_BATTLE, b->clock_us); b->spk = true; }
    spk_freq(SND_BATTLE, b->clock_us, divisor);
}
void b_sound_off(Battle *b) { spk_off(SND_BATTLE, b->clock_us); b->spk = false; }
void b_click(Battle *b) { spk_click(SND_BATTLE, b->clock_us); }

void b_beep(Battle *b)                                  /* beep_low (rt:0C32) */
{
    b_sound(b, 0x1F4);
    b_delay(b, 0x190);
    b_sound_off(b);
}

int b_scnf_byte(const Battle *b, int ds)
{
    if (ds < 0x684C || ds >= 0x684C + SCNF_SIZE) return 0;
    return (int8_t)b->ga->scnf[ds - 0x684C];
}
int b_scnf_word(const Battle *b, int ds) { return assets_scnf_word(b->ga, (unsigned)ds); }

/* [6867]: the guns' loading (Options Shoppe: 0, 3, 10); 10 in Practice (rt_0000) */
static int opt_guns(const Battle *b) { return b->practice ? 10 : b_scnf_byte(b, 0x6867); }
/* [6868]: 0 the player aims (Aim), 1 the gunners aim themselves */
static int opt_aim(const Battle *b) { return b_scnf_byte(b, 0x6868); }

static void flush_keys(Battle *b) { while (b_getkey(b)) ; }

/* ------------------------------------------------------------------ the panel (overlay 6) */

/* ovl6_f898(a, c, d, sound): a message on row 23: a, c, d (DS offsets, d may be 0) and a
 * full stop, and a two-note call (sound 1 down, 0 up, -1 one long note) */
void ovl6_message(Battle *b, int a, int c, int d, int sound)
{
    if (blog_on(b, GLOG_NORMAL)) blog(b, "\"%s%s%s.\"", blog_str(b, a), blog_str(b, c), d ? blog_str(b, d) : "");
    b->msg_timer = 100;
    g_clear_row(b, 23);
    g_print(b, a);
    g_print(b, c);
    if (d) g_print(b, d);
    g_putc(b, '.');
    int stop, step;
    if (sound) { stop = 0x984; step = -300; }
    else { stop = 0x234C; step = 3000; }
    if (sound == -1) {
        b_sound(b, 0x2EE0);
        b_delay(b, 3000);
    } else {
        for (int f = 0xBDC; f != stop; f += step) {
            b_sound(b, f);
            b_delay(b, 2000);
        }
    }
    b_sound_off(b);
}

void ovl6_error_beep(Battle *b)   /* ovl6_f9b1 */
{
    b_sound(b, 0x7D0);
    b_delay(b, 600);
    b_sound_off(b);
}

int ovl6_plus_minus(Battle *b, int k)   /* ovl6_fcaa */
{
    if (k == '+' || k == '=') { b_beep(b); return 1; }
    if (k == '-' || k == '_') { b_beep(b); return -1; }
    return 0;
}

void ovl6_menu_mark(Battle *b)   /* ovl6_fc39: the command being changed shown inverted */
{
    switch (b->menu) {
    case 'S': g_invert_block(b, 0x3C0, 10, 4); break;
    case 'A': g_invert_block(b, 0x3D4, 6, 4); break;
    case 'O': g_invert_block(b, 0x2A6, 12, 4); break;
    case 'C': g_invert_block(b, 0x2B4, 12, 4); break;
    }
}

void ovl6_time_text(Battle *b)   /* ovl6_e31e */
{
    static const int NAME[5] = { 126, 133, 138, 145, 150 };   /* faster fast medium slow slower */
    g_goto(b, 6, 24);
    g_print(b, SO6 + NAME[b->time >= 0 && b->time <= 3 ? b->time : 4]);
    g_print(b, SO6 + 157);
}

void ovl6_commands(Battle *b)   /* ovl6_e15d: Time (slower)  View  Pause  Leave */
{
    g_clear_row(b, 24);
    if (b->demo) {
        g_print(b, 0x3D9);
        g_clear_row(b, 23);
        g_goto(b, 5, 23);
        g_print(b, 0xEE4);
        return;
    }
    g_print(b, SO6 + 99);
    ovl6_time_text(b);
    g_goto(b, 15, 24);
    g_print(b, SO6 + 106);
    g_print(b, 0x3F4);
    g_goto(b, 34, 24);
    g_print(b, 0x3FB);
    if (rt_fit_ships(b)) {
        g_icon(b, 0x64D, 192, 55);
        g_icon(b, 0x66F, 192, 61);
    }
}

void ovl6_change_prompt(Battle *b)   /* ovl6_e394: "+ and - to change, Enter when done" */
{
    g_clear_row(b, 24);
    g_print(b, SO6 + 161);
    g_icon(b, 0x64D, 192, 0);
    g_icon(b, 0x66F, 192, 14);
    g_icon(b, 0x62B, 192, 42);
    b_beep(b);
}

static bool starboard(const Battle *b) { return b->crew_side[b->sel] != 0 || b->demo; }

void ovl6_fire_label(Battle *b)   /* ovl6_e266: "Aim Fire" when the guns are ready */
{
    int s = b->sel;
    g_goto(b, 10, 3);
    bool show = b->menu == 'A';
    if (!show) {
        bool ready = (b->crew_side[s] == 0 && b->gun_l[s] == 13) || (b->crew_side[s] != 0 && b->gun_r[s] == 13);
        if (ready && (b->crew_x == 96 || b->crew_x == 97 || b->crew_x == 121 || b->crew_x == 120)) show = true;
    }
    if (show) {
        if (opt_aim(b) == 0) g_print(b, SO6 + 116);
        else g_spaces(b, 3);
        g_print(b, SO6 + 120);
    } else g_spaces(b, 8);
}

void ovl6_order_text(Battle *b)   /* ovl6_e3e3 */
{
    int s = b->sel;
    if (b->order[s] < 1 || b->order[s] > 6) b->order[s] = 4;
    int o = b->order[s];
    g_goto(b, 26, 2); g_spaces(b, 9);
    g_goto(b, 19, 3); g_spaces(b, 16);
    g_goto(b, 26, 2);
    g_print(b, SO6 + mw(b, 0x18ED + 2 * (o - 1)));
    if (o > 1 && o < 6) {
        g_goto(b, 19, 3);
        g_print(b, SO6 + mw(b, 0x18F9 + 2 * (o - 1)));
    }
}

void ovl6_speed_icon(Battle *b)   /* ovl6_df4b: the sails and the wind */
{
    int line = 0, src;
    g_goto(b, 0, 0);
    g_putc(b, ' ');
    g_mast(b, b->sail_set[b->sel], b->sails[b->sel]);
    if (!b->p->wind_on) return;
    switch (b->p->wind) {
    case 0: src = 0x17FF; break;
    case 1: src = 0x17F5; line = 1; break;
    case 2: src = 0x1807; break;
    default: src = SO6 + 0x247; line = 1; break;
    }
    g_icon(b, src, line, 0);
}

static int heading_plus_turn(const Battle *b, int s)   /* rt_524c */
{
    int h = b->heading[s] + b->turn[s];
    while (h > 15) h -= 16;
    while (h < 0) h += 16;
    return h;
}

void ovl6_compass(Battle *b)   /* ovl6_dfe2: the guns' side, the ship and the course */
{
    int s = b->sel, d = b->heading[s];
    if (b->crew_side[s]) { d += 4; if (d > 15) d -= 16; }
    else { d -= 4; if (d < 0) d += 16; }
    if (d == 1 || d == 5 || d == 9 || d == 13) d++;
    d >>= 1;
    int was = b->view;
    if (b->view == 3) { b->view = 2; ovl6_sprites(b); }
    g_icon(b, SO6 + 0x103, 0, 72);
    int a = mw(b, 0x10B6 + 2 * d);
    g_fill(b, a % 80, (a / 80) << 1, 1, 2, 0xAA);
    g_needle(b, b->heading_spr[b->heading[s]], 74, msb(b, 0x17E5 + b->heading[s]) + 12);
    a = mw(b, 0x10C6 + 2 * heading_plus_turn(b, s));
    if (b->order[s] == 1) g_fill(b, a % 80, (a / 80) << 1, 1, 2, 0xFF);
    if (was == 3) { b->view = 3; ovl6_sprites(b); }
}

static void ship_name(Battle *b)   /* ovl6_e204: centred on row 1 */
{
    int a = b->name[b->sel], col = 27;
    for (;;) {
        if (mb(b, a++) == 0) break;
        if (mb(b, a++) == 0) break;
        col--;
    }
    g_goto(b, col, 1);
    g_print(b, b->name[b->sel]);
}

/* ovl6_ed60: the gun crew's panel buffer: cleared, the two guns at their elevations,
 * and the powder and shot when the guns load slowly */
void ovl6_crew_panel(Battle *b)
{
    int s = b->sel;
    memset(&b->m[PANEL], 0, 0x18C * 2);
    b_cost(b, 410 * (1000.0 / 300.0));   /* memsetw */
    int l4 = 90, l2 = 125;
    if (starboard(b)) {
        if (b->gun_r[s] < 11) l2 = 122;
        if (b->gun_r[s] == 11) l2 = b->crew_x - 84;
    } else {
        if (b->gun_l[s] < 11) l4 = 93;
        if (b->gun_l[s] == 11) l4 = b->crew_x - 92;
    }
    g_panel_sprite(b, b->gun_r_spr[b->aim_r_set[s]], l2, 0, 0);
    g_panel_sprite(b, b->gun_l_spr[b->aim_l_set[s]], l4, 0, 0);
    if (opt_guns(b) >= 10 || b->demo) return;
    if (opt_guns(b) == 0) {
        if (b->gun_r[s] < 7 || b->gun_r[s] > 9) g_panel_sprite(b, SO6 + 0x31D, 0x17B, 0, 0);
        if (b->gun_l[s] < 7 || b->gun_l[s] > 9) g_panel_sprite(b, SO6 + 0x31D, 0x171, 0, 0);
    }
    if (b->gun_l[s] < 4) g_panel_sprite(b, SO6 + 0x329, 0x277, 1, 0);
    if (b->gun_r[s] < 4) g_panel_sprite(b, SO6 + 0x329, 0x284, 1, 0);
}

/* ovl6_e7f1: the gun crew. While a script (DS:1111 on) runs, one frame of it each
 * call; at its end (99) the next script is chosen from the loading state 6AB3 / 6ABF
 * (0-13) and where the man stands (6C77). */
void ovl6_crew_anim(Battle *b)
{
    int s = b->sel;
    if (b->menu == 'A') return;
    bool right = b->crew_side[s] != 0 || b->demo;
    int *st = right ? &b->gun_r[s] : &b->gun_l[s];
    ovl6_crew_panel(b);
    if (mw(b, b->script) == 99) {
        if (right) {
            for (;;) {
                int v = *st;
                if (v >= 0 && v <= 3) {
                    if (b->crew_x == 117) { *st = 4; b->script = 0x121D; }
                    else b->script = b->crew_x > 117 ? 0x1149 : 0x115F;
                } else if (v == 4) {
                    if (b->crew_x >= 128) { (*st)++; b->script = 0x1111; } else b->script = 0x119F;
                } else if (v == 5) {
                    g_panel_sprite(b, PANEL + 0x89E, b->crew_x - 1, 0, 0);
                    (*st)++;
                    g_panel_copy(b, b->crew_scroll);
                    return;
                } else if (v == 6) {
                    if (opt_guns(b) != 0 && b->crew_x < 119) *st = 9;
                    if (b->crew_x == 115 || *st == 9) { (*st)++; ovl6_crew_panel(b); b->script = 0x1111; }
                    else b->script = b->crew_x > 115 ? 0x1149 : 0x115F;
                } else if (v == 7) {
                    if (b->crew_x >= 126) { (*st)++; b->script = 0x1111; } else b->script = 0x11F3;
                } else if (v == 8) {
                    b->script = 0x1353; (*st)++;
                } else if (v == 9) {
                    if (b->crew_x == 116) { (*st)++; ovl6_crew_panel(b); b->script = 0x1111; }
                    else b->script = b->crew_x < 116 ? 0x11F3 : 0x11C9;
                } else if (v == 10) {
                    if (b->crew_x == 119) { b->crew_x += 88; *st = 11; continue; }
                    b->script = b->crew_x < 119 ? 0x115F : 0x1149;
                } else if (v == 11) {
                    if (b->crew_x == 209) { b->crew_x -= 88; *st = 12; b->aim_r_set[s] = b->aim_r[s]; continue; }
                    b->script = 0x12CD;
                } else {
                    if (*st != 13) { *st = 13; ovl6_fire_label(b); }
                    b->script = b->crew_x < 121 ? 0x115F : 0x1133;
                    if (b->crew_x == 120) ovl6_fire_label(b);
                }
                break;
            }
        } else {
            for (;;) {
                int v = *st;
                if (v >= 0 && v <= 3) {
                    if (b->crew_x == 102) { *st = 4; b->script = 0x1265; }
                    else b->script = b->crew_x < 102 ? 0x115F : 0x1149;
                } else if (v == 4) {
                    if (b->crew_x <= 90) { (*st)++; b->script = 0x1111; } else b->script = 0x1175;
                } else if (v == 5) {
                    g_panel_sprite(b, PANEL + 0x8D0, b->crew_x, 0, 0);
                    (*st)++;
                    g_panel_copy(b, b->crew_scroll);
                    return;
                } else if (v == 6) {
                    if (opt_guns(b) != 0 && b->crew_x > 98) *st = 9;
                    if (b->crew_x == 103 || *st == 9) { (*st)++; ovl6_crew_panel(b); b->script = 0x1111; }
                    else b->script = b->crew_x < 103 ? 0x115F : 0x1149;
                } else if (v == 7) {
                    if (b->crew_x <= 89) { (*st)++; b->script = 0x1111; } else b->script = 0x11C9;
                } else if (v == 8) {
                    b->script = 0x12ED; (*st)++;
                } else if (v == 9) {
                    if (b->crew_x == 103) { (*st)++; ovl6_crew_panel(b); b->script = 0x1111; }
                    else b->script = b->crew_x > 103 ? 0x11C9 : 0x11F3;
                } else if (v == 10) {
                    if (b->crew_x == 97) { b->crew_x += 88; *st = 11; continue; }
                    b->script = b->crew_x > 97 ? 0x1149 : 0x115F;
                } else if (v == 11) {
                    if (b->crew_x == 182) { b->crew_x -= 85; *st = 12; b->aim_l_set[s] = b->aim_l[s]; continue; }
                    b->script = 0x12AD;
                } else {
                    if (*st != 13) { *st = 13; ovl6_fire_label(b); }
                    b->script = b->crew_x > 96 ? 0x1149 : 0x111D;
                    if (b->crew_x == 97) ovl6_fire_label(b);
                }
                break;
            }
        }
    }
    /* LEC64: one frame: entries of dx, picture, half, overwrite, more */
    for (int guard = 0; guard < 64; guard++) {
        int e = b->script;
        b->crew_x += mw(b, e);
        g_panel_sprite(b, b->crew_spr[mw(b, e + 2) & 31], b->crew_x, mw(b, e + 4), mw(b, e + 6));
        b->script = e + 10;
        if (mw(b, e + 8) != 1) break;
    }
    if (*st >= 8 && *st <= 9 &&
        (b->crew_x == 89 || b->crew_x == 126 || b->crew_x == 1 || b->crew_x == 38)) {
        if (right) g_panel_sprite(b, b->gun_r_spr[b->aim_r_set[s]], 122, 0, 0);
        else g_panel_sprite(b, b->gun_l_spr[b->aim_l_set[s]], 93, 0, 0);
    }
    g_panel_copy(b, b->crew_scroll);
}

void ovl6_anim_tick(Battle *b)   /* ovl6_ef34: where the men of the other ships stand */
{
    for (int i = 0; i < BS; i++) {
        if (b->anim_from[i] == 0) continue;
        if (b->crew_side[i] || b->demo) {
            if (++b->anim_from[i] > 116) b->anim_from[i] = 0;
        } else if (--b->anim_from[i] < 98) b->anim_from[i] = 0;
    }
}

/* ovl6_ddc7: the panel at the top for your ship [69D1] */
void ovl6_panel(Battle *b)
{
    int s = b->sel;
    g_clear_row(b, 3);
    if (b->hull[s] < 6) g_print(b, SO6 + 46);          /* Speed */
    g_goto(b, 6, 3);
    g_print(b, SO6 + 52);                              /* Gun */
    ovl6_fire_label(b);
    g_clear_row(b, 0);
    g_clear_row(b, 1);
    g_clear_row(b, 2);
    g_goto(b, 19, 2);
    g_print(b, SO6 + 56);                              /* Orders- */
    ovl6_compass(b);
    g_vline(b, 0, 10, 16);
    g_vline(b, 0, 36, 16);
    g_bar3f(b, 71, 0, 16);
    g_hline(b, 12, 37, 35);
    g_hline(b, 32, 0, 80);
    g_hline(b, 182, 0, 80);
    g_hline(b, 22, 11, 26);
    ship_name(b);
    g_goto(b, 19, 0);
    if (b->type[s] == 1) g_print(b, SO6 + 64);
    else if (b->type[s] == 3) g_print(b, SO6 + 81);
    else g_print(b, SO6 + 91);
    ovl6_speed_icon(b);
    g_speed_bar(b, b->speed[s]);
    b->crew_scroll = (b->crew_side[s] || b->demo) ? 20 : 0;
    b->script = 0x110F;
    ovl6_crew_anim(b);
    ovl6_order_text(b);
    b->joy = 0;
}

/* ovl6_e651: another of your ships in the panel: where its gun crew's man stands */
void ovl6_show_ship(Battle *b)
{
    int s = b->sel, st;
    if (b->aim_r_set[s] > 3) b->aim_r_set[s] = 3;
    if (b->aim_l_set[s] > 3) b->aim_l_set[s] = 3;
    if (b->crew_side[s] || b->demo) {
        st = b->gun_r[s];
        b->crew_scroll = 20;
        b->crew_x = 118;
        if (st == 2 || st == 5) b->crew_x = 128;
        if (st == 8) b->crew_x = 126;
        if (st == 11) b->gun_r[s]--;
        if (st == 13) b->crew_x = 121; else b->aim_r_set[s] = 2;
    } else {
        st = b->gun_l[s];
        b->crew_scroll = 0;
        b->crew_x = 99;
        if (st == 2 || st == 5) b->crew_x = 90;
        if (st == 8) b->crew_x = 89;
        if (st == 11) b->gun_l[s]--;
        if (st == 13) b->crew_x = 96; else b->aim_l_set[s] = 2;
    }
    if (b->anim_from[s] > 0 && b->anim_from[s] < 130 && st != 11) b->crew_x = b->anim_from[s];
    b->anim_from[s] = 0;
    ovl6_panel(b);
    ovl6_commands(b);
}

/* ovl6_f9ce: the ships' pictures for each heading: SO8's in the close view, the
 * map's in the overview (DS:607F) */
void ovl6_sprites(Battle *b)
{
    static const int MAP[16] = { 0x4BF, 0x4D1, 0x4E3, 0x4F1, 0x4FF, 0x509, 0x517, 0x525,
                                 0x537, 0x549, 0x55B, 0x569, 0x577, 0x581, 0x58F, 0x59D };
    for (int i = 0; i < 16; i++)
        b->heading_spr[i] = b->view == 3 ? BMEM_SO8 + mw(b, 0x180F + 2 * i) : MAP[i];
}

void ovl6_background(Battle *b)   /* ovl6_fb95: the sea and the shore */
{
    if (b->shore == 0) { g_sea(b); return; }
    if (b->view == 2) { g_shore_overview(b, BMEM_SHORE); return; }
    int a = BMEM_SHORE;
    for (int n = (b->by - 18) * 2; n > 0; n--)
        while (msb(b, a++) != 0x63) ;
    g_shore_close(b, a, b->bx - 4);
}

/* ------------------------------------------------------------------ rt helpers */

int rt_fit_ships(Battle *b)   /* rt_585c: your ships still fighting (0 if fewer than 2) */
{
    int n = 0;
    for (int i = 0; i < BS; i++)
        if (b->owner[i] == -1 && b->x[i] != NOSHIP && b->hull[i] < 6) n++;
    return n < 2 ? 0 : n;
}

bool rt_battle_over(Battle *b)   /* rt_51cf: one side has no ship fit to fight */
{
    int yours = 0, theirs = 0;
    for (int i = 0; i < BS; i++) {
        if (b->x[i] == NOSHIP || b->hull[i] > 5) continue;
        if (b->owner[i] == -1) yours++; else theirs++;
    }
    return yours == 0 || (theirs == 0 && !b->practice);
}

void rt_view_from_sel(Battle *b)   /* rt_550d: the close view around your ship */
{
    int s = b->sel;
    b->vx = b->x[s] - 34;
    b->vy = b->y[s] - 28;
    if (b->vx < 6) b->vx = 6;
    if (b->vy < 6) b->vy = 6;
    if (b->vx > 0x189) b->vx = 0x189;
    if (b->vy > 0x168) b->vy = 0x168;
    b->by = b->vy / 6 + 16;
    b->bx = b->vx / 6 - 1;
}

static int ship_sprite(Battle *b, int s)   /* the picture for a ship's heading (as ovl5_afc6 picks it) */
{
    if (b->turning[s] && b->view == 3) {
        int h = b->prev_heading[s];
        return b->type[s] == 2 ? b->spr_frigate_turn[h] : b->spr_ship_turn[h];
    }
    if (b->type[s] != 2 && b->view == 3) return b->spr_ship[b->heading[s]];
    return b->heading_spr[b->heading[s]];
}

void rt_blink_sel(Battle *b)   /* rt_559c: the view to your ship, which blinks three times */
{
    int s = b->sel;
    rt_view_from_sel(b);
    rt_draw_all(b);
    ovl6_background(b);
    if (b->view == 2) g_box(b, b->bx, b->by, 0);
    ovl5_draw_ships(b);
    int spr = ship_sprite(b, s), x, y, wait;
    if (b->view == 3) {
        x = ovl6_xoff(b, s) + (b->x[s] - b->vx);
        y = b->y[s] - b->vy;
        wait = 100;
    } else {
        x = b->x[s] / 6;
        y = b->y[s] / 6;
        wait = 0x190;
    }
    for (int k = 0; k < 3; k++) {
        g_ship(b, spr, y, -1, x, b->half[s], b->sails[s], b->type[s], b->hull[s]);
        b_delay(b, wait);
        g_ship(b, spr, y, 0, x, b->half[s], b->sails[s], b->type[s], b->hull[s]);
        b_delay(b, wait);
    }
}

int rt_next_ship(Battle *b)   /* rt_5783: if your ship in the panel is gone, the next */
{
    int s = b->sel;
    if (b->x[s] != NOSHIP && b->owner[s] == -1 && b->hull[s] < 6) return b->hull[s];
    for (b->sel = 0; b->sel < BS; b->sel++)
        if (b->owner[b->sel] == -1 && b->x[b->sel] != NOSHIP && b->hull[b->sel] < 6) break;
    if (b->sel >= BS) { b->sel = 0; return 0; }
    b->menu = 0;
    if (b->view == 2) g_box(b, b->bx, b->by, 0);
    else ovl6_background(b);
    ovl5_draw_ships(b);
    if (b->view == 2) g_box(b, b->bx, b->by, 1);
    ovl6_show_ship(b);
    return 0;
}

void rt_flag_tune(Battle *b)   /* rt_5d3d: "You are now on the ..." */
{
    b_delay(b, 0x1B58);
    for (int i = 0; mw(b, 0xB94 + 2 * i) != 0; i++) {
        b_sound(b, mw(b, 0xB94 + 2 * i));
        b_delay(b, 0x12C);
        b_sound(b, 17);
        b_delay(b, 0xBB8);
    }
    b_sound_off(b);
    ovl6_message(b, 0x82, b->name[b->sel], 0, 1);
    b->flag_tune = true;
    b_getkey(b); b_getkey(b); b_getkey(b);
}

/* ------------------------------------------------------------------ setting up (ovl6_c9b0) */

static void list_sorted(Battle *b, int sq, int list[3])   /* list_ships, then ovl6_d523 (least torn sails first) */
{
    Play *p = b->p;
    play_list_ships(p, sq);
    for (int k = 0; k < 3; k++) list[k] = p->list[k];
    for (int pass = 0; pass < 2; pass++)
        for (int k = 0; k < 2; k++) {
            int a = list[k], c = list[k + 1];
            if (c == -1) break;
            if (p->ship[c].sails < p->ship[a].sails) { list[k] = c; list[k + 1] = a; }
        }
    for (int k = 0; k < 3; k++) p->list[k] = list[k];
}

static void ship_init(Battle *b, int i, int ship, int heading)   /* ovl6_d463 */
{
    const PShip *sh = &b->p->ship[ship];
    b->type[i] = sh->type;
    b->sail_set[i] = 6;
    b->speed[i] = 9;
    b->prev_heading[i] = heading;
    b->heading[i] = heading;
    b->name[i] = 0x609F + ship * 15;
    b->sails[i] = sh->sails > 5 ? 5 : sh->sails;
    b->hull[i] = sh->hull > 5 ? 5 : sh->hull;
    b->ship_no[i] = ship;
    b_getkey(b);
}

static bool load_into(Battle *b, const char *file, int at, int size)
{
    size_t n = 0;
    void *d = assets_load_file(b->ga->dir, file, &n);
    if (!d) return false;
    memcpy(&b->m[at], d, n < (size_t)size ? n : (size_t)size);
    SDL_free(d);
    return true;
}

static bool battle_setup(Battle *b)
{
    Play *p = b->p;
    b->joy = 0;
    b->sunk_ship = 0; b->sunk_how = 0;
    b->sel = 0; b->menu = 0;
    p->zoomed = false;
    b->view_moved = false;
    b->enemy_fired = false;
    b->left = false;
    b->flag_tune = false;
    b->view = 2;
    b->stale = 6;
    b->follow = true;
    b->time = b->demo ? 0 : 4;
    if (b->practice) b->enemy_fired = true;
    if (!load_into(b, "SO6", SO6, 0x355)) return false;
    if (!load_into(b, "SO7", SO7, 0x70E)) return false;
    g_build_shore(b);
    if (!load_into(b, "SO8", BMEM_SO8, 0x483C)) return false;
    ovl6_sprites(b);
    for (int i = 0; i < 16; i++) {
        b->spr_frigate_turn[i] = BMEM_SO8 + mw(b, 0x1839 + 2 * i);
        b->spr_ship[i] = BMEM_SO8 + mw(b, 0x1859 + 2 * i);
        b->spr_ship_turn[i] = BMEM_SO8 + mw(b, 0x1879 + 2 * i);
        b->crew_spr[i + 7] = PANEL + mw(b, 0x1899 + 2 * i);
    }
    for (int i = 0; i < 6; i++) {
        b->crew_spr[i + 1] = BMEM_SO8 + mw(b, 0x18B5 + 2 * i);
        b->crew_spr[i + 21] = BMEM_SO8 + mw(b, 0x18C1 + 2 * i);
    }
    for (int i = 0; i < 4; i++) {
        b->gun_l_spr[i] = PANEL + mw(b, 0x18CD + 2 * i);   /* 6C11 */
        b->gun_r_spr[i] = PANEL + mw(b, 0x18D5 + 2 * i);   /* 6C09 */
        b->aim_l_spr[i] = PANEL + mw(b, 0x18DD + 2 * i);   /* 6C5F */
        b->aim_r_spr[i] = PANEL + mw(b, 0x18E5 + 2 * i);   /* 6C67 */
    }
    b->crew_spr[0] = SO6 + 0x31D;
    b->crew_spr[27] = SO6 + 0x32F;     /* 6CCD */
    b->crew_spr[28] = SO6 + 0x329;     /* 6CCF */
    b->crew_spr[29] = PANEL + 0x6B2;   /* 6CD1 */
    b->crew_spr[30] = PANEL + 0x704;   /* 6CD3 */
    b->tbl_turn = SO6 + 0x253; b->tbl_turn2 = SO6 + 0x293; b->tbl_skill = SO6 + 0x2D3;
    b->tbl_pos_l = SO6 + 0x2E5; b->tbl_pos_r = SO6 + 0x2ED;
    b->boom_spr[0] = BMEM_SO8 + 0x4562; b->boom_spr[1] = BMEM_SO8 + 0x4594;
    b->boom_spr[2] = BMEM_SO8 + 0x45F0; b->boom_spr[3] = BMEM_SO8 + 0x4682;
    for (int i = 0; i < BPART; i++) { b->pnext[i] = 0; b->pstate[i] = 0; }

    if (!b->practice) {
        b->sq_b = p->sq[p->sel].partner - 1;
        b->sq_a = p->sel;
        if (p->sq[b->sq_b].side == 0) { int t = b->sq_b; b->sq_b = b->sq_a; b->sq_a = t; }
    } else b->sq_a = b->sq_b = p->sel;
    p->battle_a = b->sq_a;
    p->battle_b = b->sq_b;

    for (int i = 0; i < BS; i++) {
        b->order[i] = b->practice ? 1 : 4;
        b->dd[i] = 0;
        b->moved[i] = 0;
        b->anim_from[i] = 0;
        b->speed_wait[i] = 2;
        b->aim_l[i] = b->aim_r[i] = b->aim_l_set[i] = b->aim_r_set[i] = 2;
        b->crew_side[i] = 1;
        b->gun_l[i] = b->gun_r[i] = 13;
        b->x[i] = NOSHIP;
        b->draw_order[i] = i;
        b->half[i] = 0;
        b->turning[i] = 0;
        b->turn[i] = 0;
        b->ycount[i] = b->xcount[i] = 0;
        b->grappled[i] = -1;
        b->target[i] = -1;
        b->ship_no[i] = -1;
        b_getkey(b);
    }
    b->crew_scroll = 20;
    b->crew_x = 121;
    b->script = 0x110F;

    /* where the two squadrons start: from their places on the map, or ahead of you in Practice */
    int dx, dy, hd;
    if (b->practice) {
        hd = p->sq[b->sq_b].heading;
        dy = 0; dx = 0;
        if (hd > 13 || hd < 3) dy = 2;
        if (hd > 5 && hd < 11) dy = -2;
        if (hd > 1 && hd < 7) dx = -2;
        if (hd > 9 && hd < 15) dx = 2;
    } else {
        dx = p->sq[b->sq_b].x - p->sq[b->sq_a].x;
        dy = p->sq[b->sq_b].y - p->sq[b->sq_a].y;
    }
    if (dx == 0 && dy == 0) dy = 4;
    int adx = dx < 0 ? -dx : dx, ady = dy < 0 ? -dy : dy;
    if (adx < 2 && ady < 4) ady = 4;
    int k = adx * 5 + ady - 4;
    if (k < 0 || k > 10) k = 0;
    int sx = msb(b, 0x10E6 + k), sy = msb(b, 0x10F1 + k);
    hd = msb(b, 0x10FC + k);
    if (dy > 0) { sy = -sy; hd = 8 - hd; }
    if (dx > 0) { sx = -sx; hd = 16 - hd; }
    if (hd < 0) hd += 16;
    dx *= 36;
    dy *= 12;
    if (b->shore) { dx /= 2; dy /= 2; }
    if (sx == 0) dy += dy > 0 ? 45 : -45;
    if (sx == 0 && sy == 0) sx = 30;
    int list[3];
    list_sorted(b, b->sq_b, list);
    int oy = 0, ox = 0;
    switch (b->shore) {
    case 1: ox = -72; break;
    case 2: ox = 60; break;
    case 3: oy = 54; break;
    case 4: oy = -60; break;
    case 5: ox = -72; oy = 54; break;
    case 6: oy = -54; ox = -72; break;
    case 7: oy = -54; ox = 60; break;
    case 8: oy = 54; ox = 60; break;
    default: break;
    }
    bool big = false;
    for (int i = 0; i < 3; i++) {
        b->owner[i] = -1;
        int sh = list[i];
        if (sh == -1) continue;
        ship_init(b, i, sh, hd);
        b->x[i] = dx + 234 + ox + i * sx;
        b->y[i] = dy + 204 + oy + i * sy;
        if (p->sq[b->sq_b].port_speed == 0 && !b->demo) { b->speed[i] = 0; b->sail_set[i] = 0; }
        if (rt_can_aim(b, i) && !b->demo) b->order[i] = 3;
        if (b->type[i] != 2) big = true;
    }
    for (int i = 0; i < 3; i++) {
        if (b->x[i] != NOSHIP && p->sq[b->sq_b].port_speed != 0 && b->type[i] == 2 && big) {
            b->y[i] += 3;
            b->sail_set[i]--;
            b->speed[i]--;
        }
        b_getkey(b);
    }
    if (!b->practice) {
        if (p->opp[(0x606B - 0x604D) / 2] == 0 && b->shore == 0) { dx /= 2; dy /= 2; }
        list_sorted(b, b->sq_a, list);
        bool abreast = false;
        if (b_random(b) > p->opp[(0x6055 - 0x604D) / 2]) {
            BLOGN("the enemy's ships come abreast (random over 6055 %d)", p->opp[(0x6055 - 0x604D) / 2]);
            dx += sx; dy += sy;
            abreast = true;
            int t = sx; sx = sy; sy = t;
            dx += sx; dy += sy;
        }
        hd -= 8;
        if (hd < 0) hd += 16;
        big = false;
        for (int j = 0; j < 3; j++) {
            int i = j + 3, sh = list[j];
            b->owner[i] = 0;
            if (sh == -1) continue;
            ship_init(b, i, sh, hd);
            b->x[i] = 234 - (j * sx + dx) + ox;
            b->y[i] = 204 - (j * sy + dy) + oy;
            if (p->sq[b->sq_a].port_speed == 0 && !b->demo) { b->speed[i] = 0; b->sail_set[i] = 0; }
            if (rt_can_aim(b, i) && !abreast && p->ship[sh].sails <= b->sails[3]) b->order[i] = 3;
            if (b->type[i] != 2) big = true;
        }
        for (int i = 3; i < 6; i++)
            if (b->x[i] != NOSHIP && p->sq[b->sq_a].port_speed != 0 && b->type[i] == 2 && big) {
                b->y[i] += 3;
                b->sail_set[i]--;
                b->speed[i]--;
            }
    } else for (int i = 3; i < 6; i++) b->owner[i] = 0;
    if (blog_on(b, GLOG_NORMAL)) {
        static const char *const SHORE[13] = { "the open sea", "land to the east", "land to the west", "land to the north",
            "land to the south", "a shore NE", "a shore SE", "a shore SW", "a shore NW", "a port to the east",
            "a port to the west", "a port to the north", "a port to the south" };
        char who[64];
        if (b->practice) SDL_snprintf(who, sizeof who, "Practice: your squadron %d", b->sq_b);
        else SDL_snprintf(who, sizeof who, "Zoom: your squadron %d against squadron %d", b->sq_b, b->sq_a);
        blog(b, "%s; %s; time %d (slower), reload %d, aim %s", who, b->shore >= 0 && b->shore < 13 ? SHORE[b->shore] : "?", b->time, opt_guns(b),
             opt_aim(b) ? "automatic" : "manual");
        for (int i = 0; i < BS; i++)
            if (b->x[i] != NOSHIP)
                blog(b, "  %s at %d,%d heading %d, sails set %d, order %s", blog_ship(b, i), b->x[i], b->y[i], b->heading[i],
                     b->sail_set[i], blog_order(b->order[i]));
    }
    g_goto(b, 0, 4);
    g_spaces(b, 40);
    return true;
}

/* ------------------------------------------------------------------ the results (ovl6_d5aa) */

static int retreated_into(Battle *b, int i, int ship)   /* ovl6_db72: 1 if there is no room */
{
    int k = b->owner[i] == -1 ? 0 : 1;
    if (b->new_n[k] >= 3) return 1;
    b->p->ship[ship].sq = b->new_sq[k];
    b->new_n[k]++;
    return 0;
}

static void write_back(Battle *b, int ship, int i)   /* ovl6_dbd0 */
{
    PShip *sh = &b->p->ship[ship];
    sh->sails = b->sails[i] > 5 ? 5 : b->sails[i];
    sh->hull = b->hull[i] > 5 ? 5 : b->hull[i];
}

/* ovl6_dc2b (after Leave): ships taken in the battle change squadrons in pairs */
static void exchange(Battle *b)
{
    Play *p = b->p;
    int theirs[3], yours[3];
    list_sorted(b, b->sq_a, theirs);
    list_sorted(b, b->sq_b, yours);
    int pair[BS];
    for (int i = 0; i < BS; i++) pair[i] = i;
    for (int i = 0; i < 3; i++) {
        if (!(b->owner[i] == 0 || b->x[i] == NOSHIP)) continue;
        for (int j = 3; j < 6; j++) {
            if (pair[j] != j) continue;
            if (b->owner[j] != -1) {
                if (b->x[j] != NOSHIP) continue;
                if (b->x[i] == NOSHIP) continue;
            }
            pair[i] = j; pair[j] = i;
            if (b->owner[i] == 0 && b->x[i] != NOSHIP) p->captured[0]++;
            if (b->owner[j] == -1 && b->x[j] != NOSHIP) p->captured[1]++;
            int a = yours[i], c = theirs[j - 3];
            if (a != -1) p->ship[a].sq = b->sq_a;
            if (c != -1) p->ship[c].sq = b->sq_b;
            break;
        }
    }
    for (int i = 0; i < BS; i++) b->speed_wait[i] = pair[i];
}

static void battle_end(Battle *b)   /* ovl6_d5aa */
{
    Play *p = b->p;
    int lost_theirs = 0, lost_yours = 0, captured = 0;
    b->new_n[0] = b->new_n[1] = 10;
    for (b->new_sq[0] = 39; b->new_sq[0] > -1; b->new_sq[0]--)
        if (!p->sq[b->new_sq[0]].used) { b->new_n[0] = 0; break; }
    for (b->new_sq[1] = 39; b->new_sq[1] > -1; b->new_sq[1]--)
        if (!p->sq[b->new_sq[1]].used && b->new_sq[1] != b->new_sq[0]) { b->new_n[1] = 0; break; }
    bool theirs_hold = true;   /* l8: the first ship fit to fight is theirs (or none is) */
    for (int i = 0; i < BS; i++) {
        if (b->x[i] == NOSHIP || b->hull[i] >= 6) continue;
        if (b->owner[i] == -1) theirs_hold = false;
        break;
    }
    int list[3];
    list_sorted(b, b->sq_b, list);
    for (int i = 0; i < 3; i++) {
        int sh = list[i];
        if (sh == -1) break;
        int how = 0;   /* 1 lost, 2 removed */
        if ((b->x[i] == NOSHIP && b->order[i] != 888) || b->hull[i] > 7) how = 1;
        else {
            if (b->owner[i] == 0 || (b->hull[i] > 5 && theirs_hold)) {
                if (!b->left) {
                    b->owner[i] = 0;
                    p->captured[0]++;
                    if (b->type[i] == 3) p->losses[2]++;
                    lost_yours++;
                }
            }
            if (b->order[i] == 888 && retreated_into(b, i, sh)) how = b->owner[i] != 0 ? 1 : 2;
            if (!how) write_back(b, sh, i);
        }
        if (how == 1) {
            lost_yours++;
            p->losses[0]++;
            if (b->type[i] == 3) p->losses[2]++;
        }
        if (how) p->ship[sh].sq = -1;
    }
    list_sorted(b, b->sq_a, list);
    for (int j = 0; j < 3; j++) {
        int i = j + 3, sh = list[j];
        if (sh == -1) break;
        int how = 0;
        if ((b->x[i] == NOSHIP && b->order[i] != 888) || b->hull[i] > 7) how = 1;
        else {
            if (b->owner[i] == -1 || (b->hull[i] > 5 && !theirs_hold)) {
                if (!b->left) {
                    b->owner[i] = -1;
                    p->captured[1]++;
                    if (b->type[i] == 3) p->losses[3]++;
                    captured++;
                }
            }
            if (b->order[i] == 888 && retreated_into(b, i, sh)) how = b->owner[i] == -1 ? 2 : 1;
            if (!how) write_back(b, sh, i);
        }
        if (how == 1) {
            lost_theirs++;
            p->losses[1]++;
            if (b->type[i] == 3) p->losses[3]++;
        }
        if (how) p->ship[sh].sq = -1;
    }
    if (b->new_n[0] != 0 && b->new_n[0] != 10) play_place_detached(p, b->sq_b, b->new_sq[0], 1, b->sq_a);
    if (b->new_n[1] != 0 && b->new_n[1] != 10) play_place_detached(p, b->sq_a, b->new_sq[1], 1, b->sq_b);
    if (!b->left) {
        if (theirs_hold) { p->sq[b->sq_b].side = 0; play_clear_paths(p, b->sq_b); }
        else { p->sq[b->sq_a].side = -1; play_clear_paths(p, b->sq_a); }
        p->sq[b->sq_a].partner = 0;
        p->sq[b->sq_b].partner = 0;
        p->sq[b->sq_a].ctrl = -2;
        p->sq[b->sq_b].ctrl = -2;
    } else {
        lost_yours = captured = lost_theirs = 0;
        p->zoomed = false;
        p->sq[b->sq_b].ctrl = p->sq[b->sq_b].side;
        p->sq[b->sq_a].ctrl = p->sq[b->sq_a].side;
        exchange(b);
    }
    g_clear_row(b, 3);
    g_clear_row(b, 0);
    g_clear_row(b, 2);
    g_clear_row(b, 1);
    g_hline(b, 0, 0, 80);
    g_goto(b, 0, 1);
    if (lost_yours || captured) {
        g_print(b, 0x373);                               /* You */
        if (captured) {
            g_print(b, 0x377); g_number(b, captured); g_print(b, 0x382);   /* captured N ship */
            if (captured > 1) g_putc(b, 's');
        }
        if (lost_yours) {
            if (captured) g_print(b, 0x388);             /*  and */
            g_print(b, 0x38D); g_number(b, lost_yours); g_print(b, 0x394);   /* lost N ship */
            if (lost_yours > 1) g_putc(b, 's');
        }
        g_putc(b, '.');
        g_goto(b, 0, 2);
    }
    if (lost_theirs || captured) {
        g_print(b, 0x39A); g_number(b, captured + lost_theirs); g_print(b, 0x3A5);   /* They lost N ship */
        if (captured + lost_theirs > 1) g_putc(b, 's');
        g_putc(b, '.');
    }
    if (!lost_theirs && !captured && !lost_yours)
        g_print(b, b->left ? 0x3C5 : 0x3AB);   /* Leaving the battle. / No losses on either side. */
    if (b->left) BLOGN("the battle is over: you left it; the ships taken change squadrons");
    else BLOGN("the battle is over: you captured %d and lost %d ship%s; they lost %d (%s)", captured, lost_yours,
               lost_yours == 1 ? "" : "s", captured + lost_theirs, theirs_hold ? "they hold the field" : "you hold the field");
    play_list_ships(p, b->sq_a);
    if (p->list[0] == -1) { play_clear_paths(p, b->sq_a); p->sq[b->sq_a].used = false; }
    play_list_ships(p, b->sq_b);
    if (p->list[0] == -1) { play_clear_paths(p, b->sq_b); p->sq[b->sq_b].used = false; }
}

/* ------------------------------------------------------------------ the keys (ovl5_7ac6) */

static int read_key(Battle *b)   /* ovl5_87e5 (the joystick, ovl5_886f, is not kept) */
{
    return b_getkey(b);
}

static int upper(int k) { return k >= 'a' && k <= 'z' ? k - 32 : k; }

static void menu_start(Battle *b)   /* ovl5_883b */
{
    ovl6_menu_mark(b);
    ovl6_change_prompt(b);
}

static void menu_mark(Battle *b)    /* ovl5_8853 */
{
    ovl6_menu_mark(b);
}

static void redraw_view(Battle *b)   /* ovl5:8506 */
{
    rt_draw_all(b);
    ovl6_background(b);
    ovl5_draw_ships(b);
    if (b->view == 2) g_box(b, b->bx, b->by, 1);
}

static void recentre(Battle *b, int k)   /* ovl5:84CB */
{
    b->follow = true;
    if (k == 1 || b->demo) rt_view_from_sel(b);
    else {
        rt_blink_sel(b);
        if (b->flag_tune) g_clear_row(b, 23);
    }
    b->view_moved = false;
    b->flag_tune = false;
    redraw_view(b);
}

static void reset_loading(int *st)   /* ovl5:7C72 */
{
    if (*st < 5) *st = 3;
    else if (*st < 8) *st = 6;
    else if (*st < 10) *st = 10;
    else *st = 12;
}

/* one key from the battle's keyboard; returns 1 when the battle is to end */
int ovl5_input(Battle *b)
{
    int s, k = upper(read_key(b));
again:
    s = b->sel;
    if (k == 0) {
        if (b->view_moved && b->follow && b->view == 2 && b_random(b) < 20) { recentre(b, 1); return 0; }
        if (b->demo) {
            if (b->sail_set[s] != 6) k = 'S';
            else if (b_random(b) < 20) {
                if (b->view == 3 && b_random(b) < 15) k = ' ';
                else if (rt_closest_own(b) != s) {
                    b->sel = rt_closest_own(b);
                    b_beep(b);
                    if (s != b->sel) ovl6_show_ship(b);
                    recentre(b, 0);
                    return 0;
                } else if (rt_enemy_near(b)) { if (b->view == 2) k = 'V'; }
                else if (b->view == 3) k = 'V';
            }
        }
        if (k == 0) return 0;
    } else if (b->demo) {
        g_clear_row(b, 23);
        g_clear_row(b, 24);
        return 1;
    }
    if (k != 27) flush_keys(b);

    switch (k) {
    case 27: case '9': case '8': case '7': case '6': case '4': case '3': case '2': case '1': {   /* move the view */
        b->no_redraw = true;
        if (k == 27) k = b_getkey(b);
        flush_keys(b);
        int st = b->view == 2 ? 12 : 6;
        switch (k) {
        case '9': case 0x49: b->vx += st; b->vy -= st; break;
        case '8': case 0x48: b->vy -= st; break;
        case '3': case 0x51: b->vy += st; b->vx += st; break;
        case '6': case 0x4D: b->vx += st; break;
        case '1': case 0x4F: b->vx -= st; b->vy += st; break;
        case '2': case 0x50: b->vy += st; break;
        case '7': case 0x47: b->vy -= st; b->vx -= st; break;
        case '4': case 0x4B: b->vx -= st; break;
        default: break;
        }
        if (b->vx < 6) b->vx = 6;
        if (b->vy < 6) b->vy = 6;
        if (b->vx > 0x189) b->vx = 0x189;
        if (b->vy > 0x168) b->vy = 0x168;
        b->bx = b->vx / 6 - 1;
        b->by = b->vy / 6 + 16;
        b->follow = false;
        b_click(b);
        redraw_view(b);
        return 0;
    }
    case 'P': {   /* Pause */
        BLOGV("you Pause");
        b_getkey(b);
        b_beep(b);
        int was = b->menu;
        b->menu = 'P';
        g_clear_row(b, 24);
        g_print(b, 0x1673);
        while (read_key(b) == 0) ;
        b_getkey(b);
        b->menu = was;
        if (was == 0) ovl6_commands(b); else ovl6_change_prompt(b);
        b_beep(b);
        ovl6_fire_label(b);
        return 0;
    }
    case 'T':     /* Time */
        if (--b->time < 0) b->time = 4;
        BLOGN("you set the Time to %s", b->time == 0 ? "faster" : b->time == 1 ? "fast" : b->time == 2 ? "medium" : b->time == 3 ? "slow" : "slower");
        b_beep(b);
        if (b->menu == 0) ovl6_time_text(b);
        break;    /* on to ovl5:822B (nothing more for T) */
    case 'V': {   /* View: the overview or close */
        b->view = b->view == 2 ? 3 : 2;
        BLOGV("you View: %s", b->view == 2 ? "the overview" : "close");
        for (int i = 0; i < BPART; i++) b->pstate[i] = 0;
        for (int i = 0; i < BS; i++) if (b->x[i] != NOSHIP) b->turning[i] = 0;
        ovl6_sprites(b);
        rt_draw_all(b);
        ovl6_background(b);
        ovl5_draw_ships(b);
        b_beep(b);
        return 0;
    }
    case 'O': {   /* Orders */
        int was_order = b->order[s];
        menu_start(b);
        b->menu = 'O';
        ovl6_fire_label(b);
        menu_mark(b);
        ovl6_crew_anim(b);
        for (;;) {
            k = upper(read_key(b));
            if (k == 10 || k == 13 || k == 'S' || k == 'F' || k == 'A' || k == 'G') break;
            if (k == 'C' && b->order[s] == 1) goto course;
            if (k == 'O' || k == 'o') k = '+';
            int d = ovl6_plus_minus(b, k);
            if (d == 0) continue;
            if (b->order[s] == 1) ovl6_compass(b);
            for (int guard = 0; guard < 64; guard++) {
                b->order[s] += d;
                if (!rt_can_aim(b, s) && (b->order[s] == 2 || b->order[s] == 3)) continue;
                if (b->order[s] < 1) { b->order[s] = 7; continue; }
                if (b->order[s] > 6 || (b->order[s] == 6 && !rt_struck_left(b))) {
                    if (d > 0) b->order[s] = 0;
                    continue;
                }
                if (b->practice && b->order[s] > 3) continue;
                break;
            }
            ovl6_order_text(b);
            ovl6_compass(b);
            if (b->order[s] == 4) rt_nearest_target(b, s, 0);
            if (b->order[s] == 6) rt_nearest_target(b, s, 2);
        }
        ovl6_menu_mark(b);
        b->menu = 0;
        ovl6_commands(b);
        if (b->order[s] != was_order) BLOGN("you order the %s: %s", NAME(s), blog_order(b->order[s]));
        if (k == 10 || k == 13) { b_beep(b); return 0; }
        goto again;
    }
    case 'S':     /* Speed */
        if (b->menu == 'S') {
            int *v = &b->sail_set[s];
            *v = *v == 6 ? 0 : 6;
            b_beep(b);
            goto sails_done;
        }
        menu_start(b);
        b->menu = 'S';
        menu_mark(b);
        return 0;
    case 'C':     /* Course */
        if (b->order[s] != 1) {
            b->order[s] = 1;
            BLOGN("you order the %s: course", NAME(s));
            ovl6_compass(b);
            ovl6_order_text(b);
        }
    course:
        menu_start(b);
        b->menu = 'C';
        ovl6_fire_label(b);
        menu_mark(b);
        return 0;
    case 'G': {   /* Gun: the crew to the other side */
        int *st;
        b->crew_side[s] ^= 1;
        BLOGN("you send the %s's gun crew to the %s guns", NAME(s), b->crew_side[s] ? "starboard" : "port");
        b->script = 0x110F;
        if (b->crew_side[s]) {
            for (int i = 1; i < 21; i++) { g_panel_copy(b, i); b_delay(b, 1000); }
            if (b->crew_x < 115 || b->crew_x > 119) b->crew_x = 115;
            b->crew_scroll = 20;
            st = &b->gun_r[s];
        } else {
            for (int i = 19; i >= 0; i--) { g_panel_copy(b, i); b_delay(b, 1000); }
            if (b->crew_x < 97 || b->crew_x > 103) b->crew_x = 103;
            b->crew_scroll = 0;
            st = &b->gun_l[s];
        }
        reset_loading(st);
        if (b->menu == 'A') { b->menu = 0; ovl6_commands(b); }
        ovl6_fire_label(b);
        b_beep(b);
        ovl6_compass(b);
        ovl6_crew_anim(b);
        return 0;
    }
    case ' ':
        recentre(b, k);
        return 0;
    default:
        break;
    }

    /* ovl5:822B: Aim and Fire, with the man at his place */
    if ((b->crew_x == 96 || b->crew_x == 121) && k == 'A') {
        if (b->menu == 'A' || opt_aim(b) != 0) return 0;
        if ((b->crew_side[s] && b->gun_r[s] == 13) || (!b->crew_side[s] && b->gun_l[s] == 13)) {
            menu_start(b);
            b->menu = 'A';
            ovl6_fire_label(b);
            menu_mark(b);
            ovl6_crew_panel(b);
            if (b->crew_side[s])
                g_panel_sprite(b, b->aim_r_spr[b->aim_r_set[s]], mw(b, 0x1107 + 2 * b->aim_r_set[s]) + b->crew_x, 0, 0);
            else
                g_panel_sprite(b, b->aim_l_spr[b->aim_l_set[s]], mw(b, 0x1107 + 2 * b->aim_l_set[s]) + b->crew_x - 2, 0, 0);
            g_panel_copy(b, b->crew_scroll);
            b_beep(b);
        }
    } else if ((b->crew_x == 96 || b->crew_x == 121) && k == 'F') {
        BLOGN("you Fire the %s's %s guns (elevation %d)", NAME(s), b->crew_side[s] ? "starboard" : "port",
              b->crew_side[s] ? b->aim_r_set[s] : b->aim_l_set[s]);
        if (b->menu == 'A') { b->menu = 0; ovl6_commands(b); }
        if (opt_aim(b) != 0) {
            int ok = rt_aim(b, s, b->crew_side[s]);
            if (!ok) ok = rt_aim(b, s, b->crew_side[s]);
            if (b->crew_side[s]) { if (!ok) b->aim_r_set[s] = 3; }
            else if (!ok) b->aim_l_set[s] = 3;
        }
        ovl6_crew_anim(b);
        b_delay(b, 1000);
        if (ovl5_fire(b, s)) return 1;
    }

    /* ovl5:8443 */
    if (b->menu != 0) {
        if (k == 10 || k == 13) {
            ovl6_menu_mark(b);
            b->joy = 0;
            b->menu = 0;
            ovl6_commands(b);
            b_beep(b);
            ovl6_fire_label(b);
            return 0;
        }
        int d = ovl6_plus_minus(b, k);
        if (d == 0) return 0;
        if (b->menu == 'C') {
            b->turn[s] += d;
            BLOGN("you turn the %s %s (%+d to go)", NAME(s), d > 0 ? "right" : "left", b->turn[s]);
            ovl6_compass(b);
            b->turn_wait[s] = b->type[s] == 2 ? 2 : 3;
        } else if (b->menu == 'A') {
            int *v = b->crew_side[s] ? &b->aim_r_set[s] : &b->aim_l_set[s];
            *v += d;
            if (*v < 0) { *v = 0; ovl6_error_beep(b); }
            if (*v > 3) { *v = 3; ovl6_error_beep(b); }
            BLOGN("you Aim the %s's guns: elevation %d", NAME(s), *v);
            ovl6_crew_panel(b);
            if (b->crew_side[s]) {
                g_panel_sprite(b, b->aim_r_spr[*v], mw(b, 0x1107 + 2 * *v) + b->crew_x, 0, 1);
                g_panel_copy(b, 20);
            } else {
                g_panel_sprite(b, b->aim_l_spr[*v], mw(b, 0x1107 + 2 * *v) + b->crew_x - 2, 0, 1);
                g_panel_copy(b, 0);
            }
        } else if (b->menu == 'S') {
            b->sail_set[s] += d;
        sails_done:
            if (b->sail_set[s] < 0) { b->sail_set[s] = 0; ovl6_error_beep(b); }
            if (b->sail_set[s] > 6) { b->sail_set[s] = 6; ovl6_error_beep(b); }
            BLOGN("you set the %s's sails to %d", NAME(s), b->sail_set[s]);
            ovl6_speed_icon(b);
            ovl6_mimic(b, s);
        }
        return 0;
    }
    if (k == 'L') {   /* Leave */
        BLOGN("you Leave the battle");
        b->left = true;
        b_beep(b);
        g_clear_row(b, 24);
        return 1;
    }
    if (k == '=' || k == '+' || k == '_' || k == '-') {   /* your next / previous ship */
        b->anim_from[s] = b->crew_x;
        int was = s, d = (k == '=' || k == '+') ? 1 : -1;
        for (int guard = 0; guard < 16; guard++) {
            b->sel += d;
            if (b->sel < 0) b->sel = 6;
            if (b->sel >= 6 && d > 0) b->sel = 0;
            int t = b->sel;
            if (t < 6 && b->x[t] != NOSHIP && b->owner[t] == -1 && b->hull[t] < 6) break;
        }
        b_beep(b);
        if (was != b->sel) { ovl6_show_ship(b); BLOGV("you look at %s", blog_ship(b, b->sel)); }
        recentre(b, k);
        return 0;
    }
    return 0;
}

/* ------------------------------------------------------------------ the battle (ovl5_7650) */

static void battle_main(void *arg)
{
    Battle *b = arg;
    Play *p = b->p;
    if (!battle_setup(b)) { b->result = 0x270F; b->state = BATTLE_OVER; return; }
    rt_view_from_sel(b);
    rt_draw_all(b);
    g_palette(b, 1);
    ovl6_background(b);
    ovl5_draw_ships(b);
    g_box(b, b->bx, b->by, 1);
    ovl6_panel(b);
    g_clear_row(b, 23);
    ovl6_commands(b);
    ovl6_speeds(b);
    b->msg_timer = 0;
    int pending = 0, slow = 0, shot = 0, every = 0, it = 0;   /* l2, l4, l6, l8, l10 */
    b->ending = 0;
    for (;;) {
        b->no_redraw = false;
        if (b->ending == 0) {
            int phase = mb(b, 0x10B0 + b->time);
            if (it == phase || (b->view == 2 && phase + 1 == it)) {
                ovl6_crew_anim(b);
                ovl6_anim_tick(b);
            }
            if (ovl5_input(b)) break;
        }
        if (b->view == 2) it++;
        if (++shot >= 6) shot = 0;
        ovl6_particles(b, shot);
        if (b->msg_timer > 0 && --b->msg_timer == 0) g_clear_row(b, 23);
        if (++it >= mb(b, 0x10AA + b->time)) {
            it = 0;
            rt_ai_step(b);
            if (!b->practice && b_random(b) < 40 && rt_enemy_fire(b)) break;
            bool skip = false;
            if (b->ending) {
                if (--b->ending <= 0) break;
                skip = true;
            }
            if (!skip) {
                if (rt_battle_over(b)) {
                    memset(&b->m[PANEL], 0, 0x18C * 2);
                    g_panel_copy(b, b->crew_scroll);
                    b->ending = 40;
                    bool sinking = false;
                    for (int i = 0; i < BS; i++) if (b->x[i] != NOSHIP && b->hull[i] > 7) sinking = true;
                    if (b->view == 2 || !sinking) b->ending = 10;
                    g_clear_row(b, 24);
                    b_sound(b, 0x11C6); b_delay(b, 0x3E8);
                    b_sound(b, 0xA91); b_delay(b, 0x3E8);
                    b_sound(b, 0x1FA0); b_delay(b, 0x3E8);
                    b_sound_off(b);
                    b->time = 0;
                } else if (b->x[b->sel] == NOSHIP) {
                    rt_next_ship(b);
                    rt_flag_tune(b);
                }
            }
            if (b->ending == 0) {
                ovl6_crew_anim(b);
                ovl6_anim_tick(b);
                if (ovl5_input(b)) break;
            }
            if (!b->no_redraw) ovl5_sail(b, 1);
            if (b->ending == 0) {
                ovl6_crew_anim(b);
                ovl6_anim_tick(b);
            }
            if (++slow > 7) {
                slow = 0;
                ovl6_speeds(b);
                if (b->view == 2) ovl6_speeds(b);
            }
            if (++every >= 6) {
                every = 0;
                b->view_moved = true;
            }
            if (b->owner[every] == 0 && b->order[every] == 3 && b_random(b) < 7 &&
                b_random(b) > p->opp[(0x6055 - 0x604D) / 2])
                b->order[every] = 1;
            if (--pending < 0) {
                pending = ((10 - opt_guns(b)) >> 1) + 1;
                for (int i = 0; i < BS; i++) {
                    if (i == b->sel) continue;
                    int *st = b->crew_side[i] ? &b->gun_r[i] : &b->gun_l[i];
                    if (*st >= 13) continue;
                    (*st)++;
                    if (*st == 13) {
                        if (b->crew_side[i]) b->aim_r_set[i] = b->aim_r[i];
                        else b->aim_l_set[i] = b->aim_l[i];
                    }
                    if (opt_guns(b) == 3 && *st >= 6 && *st < 10) *st = 10;
                    b->anim_from[i] = 0;
                }
            }
        }
        if (!b->no_redraw) ovl5_draw_ships(b);
        rt_sort_ships(b);
        if (b->time != 0 || b->practice) b_delay(b, 0x12C);
        b_cost(b, battle_iter_us);
    }
    if (!b->practice) battle_end(b);
    b->result = 0;
    b->state = BATTLE_OVER;
}

/* ------------------------------------------------------------------ public */

bool battle_start(Battle *b, Play *p, bool practice)
{
    SDL_memset(b, 0, sizeof *b);
    b->p = p;
    b->ga = p->ga;
    b->practice = practice;
    b->demo = p->demo != 0;              /* [6493] */
    memcpy(b->m, p->ga->ds, DS_SIZE);
    for (int i = 0; i < MAX_SHIPS; i++)   /* the ship names, at DS:609F as WAS.EXE loads them */
        memcpy(&b->m[0x609F + i * 15], p->c->ships[i].name, 15);
    b->shore = p->battle_shore;
    b->palette = 1;
    b->state = BATTLE_RUNNING;
    b->clock_us = b->target_us = 0;
    if (!fiber_start(&b->fiber, battle_main, b)) return false;
    return true;
}

void battle_key(Battle *b, int ch, int scan)
{
    int n = (b->key_tail + 1) % 64;
    if (ch == 0) {
        if ((n + 1) % 64 == b->key_head) return;
        b->keys[b->key_tail] = 27;
        b->key_tail = n;
        b->keys[b->key_tail] = scan;
        b->key_tail = (b->key_tail + 1) % 64;
        return;
    }
    if (n == b->key_head) return;
    b->keys[b->key_tail] = ch;
    b->key_tail = n;
}

void battle_run(Battle *b, double us)
{
    if (b->state != BATTLE_RUNNING) return;
    if (us > 100000) us = 100000;
    b->target_us += us;
    if (b->clock_us < b->target_us) fiber_resume(&b->fiber);
}

void battle_draw(const Battle *b, Screen *s, GfxMode mode)
{
    (void)mode;
    const RGB *pal = b->palette ? CGA_PALETTE : CGA_PALETTE_0;
    screen_set_palette(s, pal, 4);
    for (int y = 0; y < SCREEN_H; y++) {
        const uint8_t *row = &b->vram[row_addr(y)];
        uint8_t *out = &s->px[y * SCREEN_W];
        for (int xb = 0; xb < 80; xb++) {
            uint8_t v = row[xb];
            out[xb * 4 + 0] = (v >> 6) & 3;
            out[xb * 4 + 1] = (v >> 4) & 3;
            out[xb * 4 + 2] = (v >> 2) & 3;
            out[xb * 4 + 3] = v & 3;
        }
    }
}

void battle_finish(Battle *b)
{
    spk_silence(SND_BATTLE);
    fiber_stop(&b->fiber);
    Play *p = b->p;
    if (gamelog_on(p, GLOG_NORMAL) && p->battle_a >= 0 && p->battle_b >= 0) {
        if (b->practice) gamelog_printf(p, "EVENT back on the map after the practice: %s", gamelog_squad(p, p->battle_b));
        else gamelog_printf(p, "EVENT back on the map after the battle: %s; %s",
                            p->sq[p->battle_b].used ? gamelog_squad(p, p->battle_b) : "your squadron is gone",
                            p->sq[p->battle_a].used ? gamelog_squad(p, p->battle_a) : "the enemy's squadron is gone");
    }
    play_after_battle(b->p, b->practice, b->left);
}
