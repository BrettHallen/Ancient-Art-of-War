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
 */
#include "play.h"

#include <SDL3/SDL.h>

/* ------------------------------------------------------------------ helpers */

static const char *DS(const Play *p, unsigned off) { return assets_ds_string(p->ga, off); }
static const char *SO5(const Play *p, unsigned off) { return assets_so5_string(p->ga, off); }

static void beep(Play *p) { p->beeps++; }   /* beep_low and friends: sound is step 8 */

static int rule_visibility(const Play *p) { return (int16_t)p->c->rules[5]; }   /* DS:64B7 */

/* rt_0b3d: does the player command this squadron? */
static bool commandable(const Play *p, int i)
{
    const PSquad *q = &p->sq[i];
    if (q->ctrl == -1) return true;
    return q->ctrl != 0 && q->side == -1;
}

/* rt_0630: the ships of squadron i into list[] (DS:6456), -1 after the last */
static void list_ships(Play *p, int i)
{
    for (int k = 0; k < 3; k++) p->list[k] = -1;
    int n = 0;
    for (int s = 0; s < MAX_SHIPS && n < 3; s++)
        if (p->ship[s].sq == i) p->list[n++] = s;
}

/* ------------------------------------------------------------------ text area */

static void tpos(Play *p, int row, int col) { p->trow = row; p->tcol = col; }   /* goto_xy */

static void clear_row(Play *p, int row)    /* clear_text_row */
{
    if (row < 0 || row >= 25) return;
    SDL_memset(p->text[row], ' ', 40);
    p->text[row][40] = 0;
    tpos(p, row, 0);
}

static void print(Play *p, const char *s)  /* print_string / put_char */
{
    for (; *s; s++) {
        if (p->trow >= 0 && p->trow < 25 && p->tcol < 40) {
            p->text[p->trow][p->tcol] = *s;
            if (p->trow < 23) p->panel[p->trow][p->tcol] = 1;   /* over the map: Info */
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

/* The map is put back over the Info panel (the original's next blit) */
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
    if (p->mode != PM_NORMAL) return;
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
    print(p, DS(p, 0x207));                 /* "Time (" */
    show_time(p);
    tpos(p, 24, 15);
    print(p, DS(p, 0x20E));                 /* "Fleet" */
    show_clear(p);
    tpos(p, 24, 29);
    print(p, SO5(p, 0x2BF));                /* "Surrender" */
}

/* ovl4:B252: a squadron of the same side close enough to join (index + 1, or 0).
 * Only a squadron of one ship can join another. */
static int join_partner(Play *p, int a)
{
    list_ships(p, a);
    if (p->sq[a].battle || p->list[1] != -1) return 0;
    for (int i = 0; i < MAX_SHIPS; i++) {
        const PSquad *q = &p->sq[i];
        if (!q->used || q->side != p->sq[a].side || q->mark == 2 || i == a || q->battle) continue;
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
    if (q->side == -1 && !q->battle) { print(p, DS(p, 0x23C)); pad -= 5; }   /* "Move " */
    print(p, DS(p, 0x242));                                                  /* "Info " */
    if (q->side == -1 && !q->battle) {
        list_ships(p, p->sel);
        if (p->list[1] != -1 && p->free_slot > -1) { print(p, SO5(p, 0x2B7)); pad -= 7; }   /* "Detach " */
        if (join_partner(p, p->sel)) { print(p, DS(p, 0x248)); pad -= 5; }                 /* "Join " */
    }
    if (q->battle && q->ctrl != -1 && q->ctrl != 0) { print(p, DS(p, 0x24E)); pad -= 4; }   /* "Zoom" */
    if (!q->battle && q->ctrl == -2 && commandable(p, p->sel)) { print(p, DS(p, 0x253)); pad -= 8; }   /* "Practice" */
    spaces(p, pad);
    p->mode = PM_SQUAD;
}

/* select_squad (ovl4:AB48): the squadron's command line. The original also
 * runs mark_flagship_squadrons and ovl4_8bcd (the end of the game) here. */
static void mark_flagships(Play *p);
static void cmd_squad(Play *p)
{
    mark_flagships(p);
    text_reset(p);
    p->free_slot = MAX_SHIPS - 1;            /* the last empty slot, for Detach */
    while (p->free_slot > -1 && p->sq[p->free_slot].used) p->free_slot--;
    cmd_squad_line(p);
}

/* ------------------------------------------------------------------ squadrons */

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
            q->used = false;                 /* rt_0f55 (paths) and 9999 */
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
    return q->used && (q->side != 0 || rule_visibility(p) == 80 || q->seen);
}

/* ovl4:A202: is the marker over a squadron? It is when it is in the
 * squadron's byte column, from 2 line pairs above to 2 below its map
 * address (y + 1). */
static void check_over(Play *p, int key)
{
    bool scroll = key == PK_MINUS || key == PK_KP_MINUS || key == PK_TAB ||
                  key == PK_EQUALS || key == PK_KP_PLUS || key == PK_CTRL;
    if (scroll && p->mode != PM_SQUAD) return;
    if (p->mode == PM_TRACE || p->mode == PM_PROMPT) return;
    bool found = false;
    p->sel = -1;
    for (int i = 0; i < MAX_SHIPS; i++) {
        const PSquad *q = &p->sq[i];
        if (!q->used || q->mark == 2) continue;
        if (rule_visibility(p) < 80 && !commandable(p, i) && !q->seen) continue;
        if (p->cx != q->x || p->cy < q->y - 1 || p->cy > q->y + 3) continue;
        found = true;                        /* the last one found wins, unless one is in an encounter */
        p->sel = i;
        if (!p->over_squad) { beep(p); cmd_squad(p); }   /* rt_0c72: the buzz, select_squad */
        if (q->battle) break;
    }
    if (!found && p->over_squad) cmd_normal(p);
    p->over_squad = found;
}

/* ------------------------------------------------------------------ the marker and scrolling */

/* DS:0739, 0744: the keypad's steps (Home .. PgDn) */
static const int8_t STEP_DX[11] = { -1, 0, 1, 0, -1, 0, 1, 0, -1, 0, 1 };
static const int8_t STEP_DY[11] = { -1, -1, -1, 0, 0, 0, 0, 0, 1, 1, 1 };

static bool scroll_up_key(int k)   { return k == PK_MINUS || k == PK_KP_MINUS || k == PK_TAB; }    /* rt_1217 */
static bool scroll_down_key(int k) { return k == PK_EQUALS || k == PK_KP_PLUS || k == PK_CTRL; }  /* rt_126a */

/* campaign_input ovl4:81A8-85CC: move the marker or scroll the view */
static void cursor_input(Play *p, int key)
{
    int maxx = CX_MAX, maxy = CY_MAX;
    bool up = scroll_up_key(key), down = scroll_down_key(key);
    if (!up && !down && !(key > 70 && key < 82)) return;
    int delta = 0;
    if (up) {
        if (p->view_top < 2) return;
        if (p->jump && p->mode != PM_TRACE) {         /* [081A]: to the top */
            p->view_top = 0; p->cx = 38; p->cy = 45; delta = 1;
        } else {
            p->view_top -= 2;
            if (p->view_top + VIEW_MARGIN < p->cy) { p->cy -= 2; delta = -160; }
        }
    } else if (down) {
        if (p->view_top >= VIEW_TOP_MAX - 1) return;
        if (p->jump && p->mode != PM_TRACE) {         /* to the bottom */
            p->view_top = VIEW_TOP_MAX; p->cx = 38; p->cy = 145; delta = 1;
        } else {
            p->view_top += 2;
            if (p->cy < p->view_top) { p->cy += 2; delta = 160; }
        }
    } else {
        if (p->mode == PM_PROMPT) return;
        int k = key - PK_HOME;
        p->cx += STEP_DX[k];
        p->cy += STEP_DY[k];
        delta = STEP_DY[k] * 80 + STEP_DX[k];
    }
    if (p->cx > maxx) { p->cx--; delta--; }
    if (p->cx < 1)    { p->cx++; delta++; }
    if (p->cy > maxy) { p->cy--; delta -= 80; }
    if (p->cy < 0)    { p->cy++; delta += 80; }
    if (p->cy < p->view_top) p->view_top--;
    else if (p->view_top + VIEW_MARGIN < p->cy) p->view_top++;
    p->view_top = SDL_clamp(p->view_top, 0, VIEW_TOP_MAX);
    if (delta != 0) check_over(p, key);
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
    if (p->sq[p->sel].battle) print(p, DS(p, 0x1C7));   /* "Encounter" ("In Port": step 3) */
    p->mode = PM_INFO;
}

/* ovl4:7D73: S changes the speed of a squadron on a traced path */
static void info_key(Play *p, int key)
{
    PSquad *q = &p->sq[p->sel];
    if (key == PK_S && commandable(p, p->sel) && q->has_path && !q->battle) {
        if (++q->speed > 2) q->speed = 0;
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

static void handle_key(Play *p, int key)     /* campaign_input (ovl4:7B65) */
{
    if (p->mode == PM_PROMPT) {
        if (p->ended) {
            if (key == PK_ENTER || key == PK_SPACE) p->result = p->outcome;
            else cursor_input(p, key);
        } else if (p->prompt_surrender) {       /* campaign_play ovl4:7AB1 */
            if (key == PK_Y) {
                p->prompt_surrender = false;
                text_reset(p);
                clear_row(p, 24);
                print(p, SO5(p, 0x47));         /* Your opponent accepts your surrender */
                p->ended = true;
                p->outcome = PLAY_LOST;
                p->result_code = 11;
            } else if (key == PK_N) {
                p->prompt_surrender = false;
                p->mode = PM_NORMAL;
                cmd_normal(p);
            }
        } else if (p->prompt_quit) {            /* the remake's Esc */
            if (key == PK_Y) {
                p->prompt_quit = false;
                p->result = PLAY_QUIT;
            } else if (key == PK_N || key == PK_ESC) {
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
        text_reset(p);
        clear_row(p, 24);
        print(p, "Quit to the title page? (Y or N)");
        p->mode = PM_PROMPT;
        p->prompt_quit = true;
        return;
    }
    if (p->mode == PM_NORMAL) {
        if (key == PK_S) {                      /* Surrender: campaign_play ovl4:7A83 asks */
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
            beep(p);
            show_time(p);
        } else if (key == PK_F) {               /* Fleet */
            show_fleet(p);
            return;
        } else if (key == PK_C && p->msg_rows) {   /* Clear the messages */
            beep(p);
            text_reset(p);
            show_clear(p);
        } else if (key == PK_P) {               /* Pause */
            beep(p);
            clear_row(p, 24);
            print(p, DS(p, 0x1673));            /* Press any key to continue. */
            p->mode = PM_PAUSE;
            return;
        }
    } else if (p->mode == PM_SQUAD) {
        if (key == PK_I) { show_info(p); return; }
        /* Move, Detach, Join, Zoom and Practice come with steps 3 and 4 */
    }
    cursor_input(p, key);
}

/* ------------------------------------------------------------------ public */

/* game_init (ovl1:77AD) and campaign_play's start (ovl4:7650) */
void play_start(Play *p, const Campaign *c, const GameAssets *ga)
{
    SDL_memset(p, 0, sizeof *p);
    p->c = c;
    p->ga = ga;
    for (int i = 0; i < MAX_SHIPS; i++) {
        const Squadron *s = &c->squadrons[i];
        PSquad *q = &p->sq[i];
        q->used = s->present;
        q->side = s->friendly ? -1 : 0;
        q->speed = 0;
        q->x = s->x;
        q->y = s->y;
        q->supplies = s->supplies;
        q->ctrl = -2;                        /* 5EE5 */
        q->mark = 0;
        const Ship *sh = &c->ships[i];
        PShip *ps = &p->ship[i];
        ps->sq = sh->squadron < MAX_SHIPS ? sh->squadron : -1;
        ps->sails = sh->sails_damage;
        ps->hull = sh->hull_damage;
        ps->type = sh->type;
    }
    p->view_top = VIEW_TOP_MAX;
    p->cx = 38;
    p->cy = 145;
    p->time_div = 64;
    p->text_rows = 2;
    for (int r = 0; r < 25; r++) clear_row(p, r);
    mark_flagships(p);                       /* campaign_play: mark_flagship_squadrons */
    p->sel = -1;
    p->free_slot = -1;
    cmd_normal(p);
    check_over(p, 0);
}

void play_key(Play *p, int key, bool shift)
{
    p->jump = shift;
    p->key = key;
}

void play_frame(Play *p, int held)
{
    if (p->result != PLAY_RUNNING) return;
    int key = p->key ? p->key : held;
    p->key = 0;
    p->tick_count++;
    bool frozen = p->mode == PM_PROMPT || p->mode == PM_INFO || p->mode == PM_FLEET || p->mode == PM_PAUSE;
    if (key == 0 && !frozen && p->tick_count >= p->time_div) {   /* campaign_play: one tick */
        p->tick_count = 0;
        p->ticks++;                          /* the rules come with step 3 */
        if (p->ticks > 30) p->ticks = 0;
    }
    handle_key(p, key);
}

/* ------------------------------------------------------------------ drawing */

typedef struct {
    Screen *s;
    int     top, h;          /* first map line shown, lines shown */
    uint8_t col[4];          /* CGA colours 0-3 in the screen palette */
} View;

static void vpx(const View *v, int x, int line, int c)
{
    int sy = line - v->top;
    if (x < 0 || x >= SCREEN_W || sy < 0 || sy >= v->h) return;
    v->s->px[sy * SCREEN_W + x] = v->col[c];
}

/* rt_280e: the marker, a cross (the box for tracing comes with step 3) */
static void draw_cursor(const View *v, int cx, int cy)
{
    int x = cx * 4, l = cy * 2;
    static const int arm[6] = { 0, 1, 2, 7, 8, 9 };
    for (int k = 0; k < 6; k++)
        for (int i = 0; i < 4; i++) vpx(v, x + i, l + arm[k], i < 2 ? 0 : 3);
    for (int k = 4; k <= 5; k++) {
        static const int8_t bar[12] = { 0, 0, 3, 3, 3, -1, -1, 0, 0, 3, 3, 3 };
        for (int i = 0; i < 12; i++)
            if (bar[i] >= 0) vpx(v, x - 4 + i, l + k, bar[i]);
    }
}

/* one CGA byte (4 pixels) at a screen line */
static void cga_byte(Screen *s, int line, int xbyte, uint8_t b, const uint8_t col[4])
{
    if (line < 0 || line >= SCREEN_H || xbyte < 0 || xbyte >= 80) return;
    for (int i = 0; i < 4; i++) s->px[line * SCREEN_W + xbyte * 4 + i] = col[(b >> (6 - 2 * i)) & 3];
}

static void vline(Screen *s, int line, int xbyte, int n, const uint8_t col[4])   /* rt_2b71: n line pairs of 0Fh */
{
    for (int j = 0; j < n * 2; j++) cga_byte(s, line + j, xbyte, 0x0F, col);
}

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

static void draw_bar(Screen *s, int line, int xbyte, int len, uint8_t pat, const uint8_t col[4])   /* rt:2B2A */
{
    for (int j = 0; j < 3; j++) {
        for (int i = 0; i < 21; i++) cga_byte(s, line + j, xbyte + i, 0, col);
        for (int i = 0; i < len; i++) cga_byte(s, line + j, xbyte + i, pat, col);
    }
}

/* squadron_info (rt:074B): the panel over the map */
static void info_graphics(const Play *p, Screen *s, const uint8_t col[4])
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
        draw_bar(s, k * 8 + 81, 51, len > 0 ? len : 1, 0xDD, col);
        len = (5 - sh->hull) * 4;
        draw_bar(s, k * 8 + 84, 51, len > 0 ? len : 1, 0x11, col);
    }
    int sup = SDL_clamp(p->sq[p->sel].supplies, 0, 21);
    draw_bar(s, 112, 51, sup, 0x55, col);
    draw_bar(s, 115, 51, sup, 0x55, col);
}

void play_draw(const Play *p, Screen *s, GfxMode mode)
{
    const AssetSet *set = &p->ga->set[mode];
    int map_h = play_map_h(p);
    campaign_draw_tiles(s, p->ga, mode, p->c, p->view_top * 2, map_h);

    View v = { s, p->view_top * 2, map_h, { 0, 1, 2, 3 } };
    int ncolors = set->tiles[0].ncolors;
    for (int i = 0; i < 4; i++)
        v.col[i] = screen_nearest(s, ncolors, CGA_PALETTE[i].r, CGA_PALETTE[i].g, CGA_PALETTE[i].b);
    uint8_t white = v.col[3], black = v.col[0];

    /* draw_squadrons (rt:10D2): the squadron, then the pennant or the cargo a line pair above */
    for (int i = 0; i < MAX_SHIPS; i++) {
        const PSquad *q = &p->sq[i];
        if (!visible(p, i)) continue;
        int px = q->x * 4, py = (q->y + 1) * 2 - v.top;
        int spr = q->battle ? MSPR_BATTLE
                : (q->speed == 0 && !q->has_route) ? MSPR_ANCHOR
                : MSPR_HEADING_0 + (q->heading & 15);
        int dy = (spr >= MSPR_HEADING_0 && spr < MSPR_HEADING_0 + 16) ? p->ga->heading_dy[q->heading & 15] * 2 : 0;
        bool friend = q->side != 0;
        screen_sprite(s, &p->ga->map_sprite[spr], px, py + dy, friend, white, black, 0, map_h);
        if (q->mark == 1) screen_sprite(s, &p->ga->map_sprite[MSPR_PENNANT], px, py - 2, friend, white, black, 0, map_h);
        if (q->mark == 2) screen_sprite(s, &p->ga->map_sprite[MSPR_CARGO], px, py - 2, friend, white, black, 0, map_h);
    }
    if (p->mode != PM_PROMPT || !p->ended) draw_cursor(&v, p->cx, p->cy);

    /* rt_2ae4: a white line under the map, then the text rows */
    screen_fill(s, 0, map_h, SCREEN_W, 1, white);
    screen_fill(s, 0, map_h + 1, SCREEN_W, SCREEN_H - map_h - 1, black);
    for (int r = 25 - p->text_rows; r < 25; r++)
        screen_text(s, p->ga, 0, r * 8, p->text[r], white, black);

    if (p->mode == PM_INFO) {
        info_graphics(p, s, v.col);
        char ch[2] = { 0, 0 };
        for (int r = 0; r < 23; r++)          /* put_char only where something was written */
            for (int c = 0; c < 40; c++)
                if (p->panel[r][c]) { ch[0] = p->text[r][c]; screen_text(s, p->ga, c * 8, r * 8, ch, white, black); }
    }
}
