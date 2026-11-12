/*
 * The Ancient Art of War (1984) - remake
 * Nov 2026 using Claude Opus 5.5
 * "Guided" by Brett Hallen
 * Original game by Dave Murry & Barry Murry
 * Published by Brøderbund Software Inc.
 *
 * sqedit.c: step 7, putting squads and flags on the map, following overlay
 * 3 of WAR.EXE.
 *
 * Like the original, the editor uses the game's own map: the squads, flags,
 * cursor, text rows, Info bars and the formation box are drawn by
 * play_draw. Every squad is shown (the rule "enemy is seen" is set to
 * always while editing, ovl3:30FE).
 *
 * The cursor moves as in the game. Over a squad its commands are shown
 * (Info, Size, Formation, Get, Remove), over a flag Friend or Foe and
 * Remove; elsewhere Squad, Flag and Total.
 */
#include "sqedit.h"
#include "sound.h"

#include <SDL3/SDL.h>

enum {
    SQ_INTRO,                 /* "The second step is to put squads and flags on the map." */
    SQ_MAIN,                  /* [A92F] 0: Squad Flag Total */
    SQ_SQUAD,                 /* 1: Info Size Formation Get Remove */
    SQ_FLAG,                  /* 10: Friend/Foe Remove */
    SQ_MOVE,                  /* 20: Move to the new location, Put */
    SQ_TOTAL,                 /* Total men .... */
    SQ_INFO,                  /* 4 */
    SQ_SIZE,                  /* ovl3_463A */
    SQ_FORMATION,             /* 50 */
    SQ_ASK,                   /* Continue to the next step? / Save the last changes? */
    SQ_ANSWER,                /* "Yes" or "No" shows for a moment */
    SQ_ERROR,                 /* MUST HAVE BOTH FRIEND AND FOE SQUADS. */
};

#define MAX_MEN 14            /* the places in a formation */

/* the cursor's bounds in the editor (ovl3:3BFB-3E07): bytes and line pairs */
#define CX_MIN 3
#define CX_MAX 0x4C
#define CY_MIN 4
#define CY_MAX 0xC2
#define VIEW_MARGIN 0x56

static const char *O1S(const SquadEdit *e, unsigned off) { return assets_buf_string(e->o1, sizeof e->o1, off); }

/* ------------------------------------------------------------------ text rows (as in play.c) */

static void tpos(Play *p, int row, int col) { p->trow = row; p->tcol = col; }

static void clear_row(Play *p, int row)
{
    if (row < 0 || row >= 25) return;
    SDL_memset(p->text[row], ' ', 40);
    p->text[row][40] = 0;
    tpos(p, row, 0);
}

static void print(Play *p, const char *s)
{
    for (; *s; s++) {
        if (p->trow >= 0 && p->trow < 25 && p->tcol < 40) p->text[p->trow][p->tcol] = *s;
        if (++p->tcol >= 40) { p->tcol = 0; p->trow++; }
    }
}

static void print_num(Play *p, int n)
{
    char b[16];
    SDL_snprintf(b, sizeof b, "%d", n);
    print(p, b);
}

static void text_rows_add(Play *p, int n)
{
    int old = p->text_rows;
    p->text_rows = SDL_clamp(p->text_rows + n, 2, 25);
    for (int r = 25 - p->text_rows; r < 25 - old; r++) clear_row(p, r);
}

static void text_reset(Play *p)            /* text_area_reset (rt:0F80) */
{
    if (p->text_rows > 2) text_rows_add(p, 2 - p->text_rows);
    clear_row(p, 23);
}

/* ------------------------------------------------------------------ squads */

static int count_squads(const SquadEdit *e)
{
    int n = 0;
    for (int i = 0; i < MAX_SQUADS; i++) n += e->p.sq[i].used;
    return n;
}

static int count_flags(const SquadEdit *e)
{
    int n = 0;
    for (int i = 0; i < MAX_FLAGS; i++) n += e->p.flag[i].present;
    return n;
}

/* update_squads (rt:0B27): the men, spies-only squads, empty squads go */
static void update_squads(SquadEdit *e)
{
    for (int i = 0; i < MAX_SQUADS; i++) {
        PSquad *q = &e->p.sq[i];
        q->men = q->archers + q->knights + q->barbarians + q->spies;
        q->small = q->spies && !q->archers && !q->knights && !q->barbarians;
        if (q->men == 0) q->used = false;
        else if (!q->used) { q->used = true; q->moving = 0; }
    }
}

static PSquad *sel_squad(SquadEdit *e) { return &e->p.sq[SDL_clamp(e->p.sel, 0, MAX_SQUADS - 1)]; }

/* ------------------------------------------------------------------ command lines */

static void main_line(SquadEdit *e)        /* ovl3_340E */
{
    Play *p = &e->p;
    clear_row(p, 24);
    if (count_squads(e) < MAX_SQUADS) print(p, O1S(e, 0x26));   /* Squad */
    if (count_flags(e) < MAX_FLAGS) print(p, O1S(e, 0x2D));     /* Flag */
    print(p, O1S(e, 0x194));                                    /* Total */
    tpos(p, 24, 30);
    print(p, O1S(e, 0x33));                                     /* when done */
    p->mode = PM_NORMAL;
    e->state = SQ_MAIN;
}

static void squad_line(SquadEdit *e)       /* ovl3_3541 */
{
    Play *p = &e->p;
    update_squads(e);
    text_reset(p);
    clear_row(p, 24);
    print(p, O1S(e, 0x3D));                                     /* Info Size */
    const PSquad *q = sel_squad(e);
    if (!q->small && q->friendly) print(p, O1S(e, 0x48));       /* Formation */
    print(p, O1S(e, 0x53));                                     /* Get Remove */
    p->mode = PM_SQUAD;
    e->state = SQ_SQUAD;
}

static void flag_line(SquadEdit *e)        /* ovl3_35E8 */
{
    Play *p = &e->p;
    text_reset(p);
    clear_row(p, 24);
    print(p, O1S(e, e->p.flag[e->flag_sel].white ? 0x82 : 0x86));   /* Foe / Friend: what F makes it */
    print(p, O1S(e, 0x77));                                     /* Remove */
    p->mode = PM_NORMAL;
    e->state = SQ_FLAG;
}

/* ------------------------------------------------------------------ Info (ovl3_3644) */

static void info_text(SquadEdit *e)
{
    Play *p = &e->p;
    const PSquad *q = sel_squad(e);
    clear_row(p, 17);                                           /* ovl3_3803 */
    print(p, O1S(e, 0xA8));                                     /* Speed - when moving: */
    print(p, O1S(e, q->speed == 3 ? 0xBE : q->speed == 2 ? 0xC9 : 0xD4));
    clear_row(p, 18);                                           /* ovl3_3890 */
    print(p, O1S(e, 0xDF));                                     /* Type: */
    print(p, O1S(e, q->friendly ? 0xE6 : 0xED));
    clear_row(p, 20);
    print(p, O1S(e, 0xF4));                                     /* Condition ---- (bar: info_graphics) */
    clear_row(p, 21);
    print(p, O1S(e, 0x103));                                    /* Food --------- */
    clear_row(p, 24);
    print(p, O1S(e, 0x8D));                                     /* Speed Type Condition Food */
    tpos(p, 24, 26);
    print(p, O1S(e, e->increase ? 0x165 : 0x15B));              /* the other way: Decrease / Increase */
    print(p, " ");
    tpos(p, 24, 36);
    print(p, O1S(e, 0x1BA));                                    /* End */
}

static void info_start(SquadEdit *e)       /* ovl3_430F */
{
    Play *p = &e->p;
    e->increase = false;
    text_reset(p);
    text_rows_add(p, 6);
    p->cond_mark = p->food_mark = 0;
    p->mode = PM_INFO;
    e->state = SQ_INFO;
    info_text(e);
}

static void info_key(SquadEdit *e, int k)
{
    PSquad *q = sel_squad(e);
    switch (k) {
    case PK_C:                                                  /* Condition, 1-49 */
        q->condition += e->increase ? 1 : -1;
        if (q->condition > 49) q->condition = 49;
        else if (q->condition < 1) q->condition = 1;
        else snd_beep();
        break;
    case PK_F:                                                  /* Food, 1-49 */
        q->food += e->increase ? 1 : -1;
        if (q->food > 49) q->food = 49;
        else if (q->food < 1) q->food = 1;
        else snd_beep();
        break;
    case PK_D: if (e->increase) { snd_beep(); e->increase = false; } break;
    case PK_I: if (!e->increase) { snd_beep(); e->increase = true; } break;
    case PK_S:                                                  /* Speed: slow, march, fast */
        snd_beep();
        if (--q->speed < 1) q->speed = 3;
        break;
    case PK_T:                                                  /* Type: friend or foe */
        snd_beep();
        q->friendly = !q->friendly;
        break;
    case PK_E:
        snd_beep();
        squad_line(e);
        return;
    default:
        return;
    }
    info_text(e);
}

/* ------------------------------------------------------------------ Size (ovl3_463A) */

static int men(const PSquad *q) { return q->knights + q->barbarians + q->archers + q->spies; }

static void size_text(SquadEdit *e)
{
    Play *p = &e->p;
    clear_row(p, 24);
    print(p, O1S(e, 0x1C2));                                    /* Decrease Knight Barb. Archer Spy    End */
    tpos(p, 24, 0);
    print(p, O1S(e, e->increase ? 0x165 : 0x15B));              /* the other way */
}

static void size_start(SquadEdit *e)
{
    e->increase = true;
    text_reset(&e->p);
    e->p.mode = PM_SIZE;
    e->state = SQ_SIZE;
    if (men(sel_squad(e)) >= MAX_MEN) e->increase = false;
    size_text(e);
}

static void size_key(SquadEdit *e, int k)
{
    PSquad *q = sel_squad(e);
    int *n = k == PK_K ? &q->knights : k == PK_B ? &q->barbarians : k == PK_A ? &q->archers : k == PK_S ? &q->spies : NULL;
    if (n) {
        /* the remake: never more than 14 men (the original lets I add more) */
        if (e->increase && men(q) < MAX_MEN) { snd_beep(); (*n)++; }
        else if (!e->increase && *n > 0) { snd_beep(); (*n)--; }
    } else if (k == PK_I && !e->increase) {
        snd_beep();
        e->increase = true;
    } else if (k == PK_D && e->increase) {
        snd_beep();
        e->increase = false;
    } else if (k == PK_E) {
        snd_beep();
        update_squads(e);
        squad_line(e);
        return;
    } else {
        return;
    }
    if (men(q) <= 0 && !e->increase) e->increase = true;        /* ovl3:4805 */
    else if (men(q) >= MAX_MEN && e->increase) e->increase = false;
    snd_tone(5000, 1200);                                       /* ovl3_4A78 */
    size_text(e);
}

/* ------------------------------------------------------------------ Formation (ovl3_4A95) */

static void formation_text(SquadEdit *e)   /* rt:0EB9: Formation: N */
{
    Play *p = &e->p;
    tpos(p, 23, 21);
    print(p, assets_ds_string(e->ga, 57));
    print_num(p, sel_squad(e)->formation);
    print(p, " ");
}

static void formation_start(SquadEdit *e)
{
    Play *p = &e->p;
    PSquad *q = sel_squad(e);
    if (q->formation < 1 || q->formation > 9) q->formation = 1;
    text_reset(p);
    text_rows_add(p, 1);
    clear_row(p, 24);
    tpos(p, 24, 7);
    print(p, O1S(e, 0x1BA));                                    /* [+] [-] End */
    formation_text(e);
    p->mode = PM_FORMATION;
    e->state = SQ_FORMATION;
}

static void formation_key(SquadEdit *e, int k)
{
    PSquad *q = sel_squad(e);
    bool down = k == PK_EQUALS || k == PK_KP_PLUS, up = k == PK_MINUS || k == PK_KP_MINUS;
    if (down) { if (++q->formation > 9) q->formation = 1; snd_beep(); formation_text(e); }
    else if (up) { if (--q->formation < 1) q->formation = 9; snd_beep(); formation_text(e); }
    else if (k == PK_E || (k >= PK_HOME && k <= PK_PGDN)) {     /* End, or a cursor key */
        if (k == PK_E) snd_beep();
        text_reset(&e->p);
        squad_line(e);
    }
}

/* ------------------------------------------------------------------ the cursor */

/* ovl3_419D and ovl3_4256: is the cursor on a squad or a flag? (the column
 * after its left edge, from the line pair above it to three below) */
static bool on(int cx, int cy, int x, int y) { return cx == x + 1 && cy >= y - 1 && cy <= y + 3; }

static void check_over(SquadEdit *e)
{
    Play *p = &e->p;
    bool found = false;
    for (int i = 0; i < MAX_SQUADS; i++) {
        const PSquad *q = &p->sq[i];
        if (!q->used || !on(p->cx, p->cy, q->x, q->y)) continue;
        found = true;
        p->sel = i;
        if (!e->over_squad) { snd_buzz(); squad_line(e); }
    }
    if (!found && e->over_squad) main_line(e);
    e->over_squad = found;

    found = false;
    for (int i = 0; i < MAX_FLAGS; i++) {
        const PFlag *f = &p->flag[i];
        if (!f->present || !on(p->cx, p->cy, f->x, f->y)) continue;
        found = true;
        e->flag_sel = i;
        if (!e->over_flag) { snd_buzz(); flag_line(e); }
    }
    if (!found && e->over_flag) main_line(e);
    e->over_flag = found;
}

/* ovl3:3BF3: the cursor keys and -/+ (two line pairs at a time) */
static void cursor_key(SquadEdit *e, int k)
{
    Play *p = &e->p;
    bool moved = true;
    switch (k) {
    case PK_HOME: if (p->cx > CX_MIN) p->cx--; if (p->cy > CY_MIN) p->cy--; break;
    case PK_UP:   if (p->cy > CY_MIN) p->cy--; break;
    case PK_PGUP: if (p->cx < CX_MAX) p->cx++; if (p->cy > CY_MIN) p->cy--; break;
    case PK_LEFT: if (p->cx > CX_MIN) p->cx--; break;
    case PK_RIGHT: if (p->cx < CX_MAX) p->cx++; break;
    case PK_END:  if (p->cx > CX_MIN) p->cx--; if (p->cy < CY_MAX) p->cy++; break;
    case PK_DOWN: if (p->cy < CY_MAX) p->cy++; break;
    case PK_PGDN: if (p->cx < CX_MAX) p->cx++; if (p->cy < CY_MAX) p->cy++; break;
    case PK_MINUS: case PK_KP_MINUS:
        if (p->view_top <= 1) return;
        p->view_top -= 2;
        if (p->view_top + VIEW_MARGIN < p->cy) p->cy -= 2;
        break;
    case PK_EQUALS: case PK_KP_PLUS:
        if (p->view_top >= VIEW_TOP_MAX - 1) return;
        p->view_top += 2;
        if (p->cy < p->view_top) p->cy += 2;
        break;
    default:
        moved = false;
        break;
    }
    if (!moved) return;
    if (p->cy < p->view_top) p->view_top--;
    else if (p->view_top + VIEW_MARGIN < p->cy) p->view_top++;
    p->view_top = SDL_clamp(p->view_top, 0, VIEW_TOP_MAX);
    if (e->state != SQ_MOVE) check_over(e);
}

static bool cursor_state(const SquadEdit *e)
{
    return e->state == SQ_MAIN || e->state == SQ_SQUAD || e->state == SQ_FLAG || e->state == SQ_MOVE;
}

/* ------------------------------------------------------------------ public */

void sqedit_start(SquadEdit *e, const GameAssets *ga, const Campaign *c, bool new_campaign)
{
    SDL_memset(e, 0, sizeof *e);
    e->ga = ga;
    size_t size = 0;
    uint8_t *d = assets_load_file(ga->dir, "O1", &size);
    if (d) {
        SDL_memcpy(e->o1, d, SDL_min(size, sizeof e->o1));
        e->have_o1 = true;
    }
    SDL_free(d);
    e->new_campaign = new_campaign;
    e->c = *c;
    e->c.rules[RULE_ENEMY_SEEN] = 80;          /* [A70F] = 80 while editing */

    /* the squads and flags as the T file has them (not play_start: no flags
     * are added, spies stay spies, and the enemy makes no plans) */
    Play *p = &e->p;
    p->c = &e->c;
    p->ga = ga;
    const uint8_t *a = c->armies;
#define W(off, i) ((int16_t)(a[(off) + 2 * (i)] | (a[(off) + 2 * (i) + 1] << 8)))
    for (int i = 0; i < MAX_SQUADS; i++) {
        PSquad *q = &p->sq[i];
        q->used = W(80, i) != 9999;
        q->friendly = W(0, i) == -1;
        q->speed = W(40, i);
        q->x = W(120, i);
        q->y = W(160, i);
        q->formation = a[200 + i];
        q->condition = a[220 + i];
        q->food = a[240 + i];
        q->archers = a[320 + i];
        q->knights = a[340 + i];
        q->barbarians = a[360 + i];
        q->spies = a[380 + i];
    }
    for (int i = 0; i < MAX_FLAGS; i++) {
        p->flag[i].present = W(260, i) >= 0;
        p->flag[i].x = W(260, i);
        p->flag[i].y = W(280, i);
        p->flag[i].white = W(300, i) == -1;
    }
#undef W
    p->view_top = VIEW_TOP_MAX;                /* the bottom of the map, as in the game */
    p->cx = 0x26;
    p->cy = 0x91;
    p->reveal = true;
    p->result = PLAY_RUNNING;
    p->zoom_squad = -1;
    p->sel = 0;
    e->flag_sel = 0;
    e->result = SQEDIT_RUNNING;
    update_squads(e);
    for (int r = 0; r < 25; r++) clear_row(p, r);
    p->text_rows = 2;
    p->msg_rows = 0;
    if (new_campaign) {                        /* ovl3:31CC */
        tpos(p, 23, 0);
        print(p, O1S(e, 0x00));                /* The second step is to put squads and */
        clear_row(p, 24);
        print(p, O1S(e, 0x1EA));               /* flags on the map.  Press any key. */
        p->mode = PM_NORMAL;
        e->state = SQ_INTRO;
    } else {
        main_line(e);
    }
}

static void ask(SquadEdit *e)
{
    Play *p = &e->p;
    clear_row(p, 24);
    print(p, O1S(e, e->new_campaign ? 0x20F : 0x16F));   /* Continue to the next step? / Save the last changes? */
    e->state = SQ_ASK;
    e->wait = 7;                               /* delay(7000) */
}

static bool both_sides(const SquadEdit *e)     /* ovl3:3319 */
{
    bool friend = false, foe = false;
    for (int i = 0; i < MAX_SQUADS; i++) {
        if (!e->p.sq[i].used) continue;
        if (e->p.sq[i].friendly) friend = true; else foe = true;
    }
    return friend && foe;
}

static void finish(SquadEdit *e, SqEditResult r)
{
    if (r != SQEDIT_DISCARD && !both_sides(e)) {
        snd_error();
        clear_row(&e->p, 24);
        print(&e->p, O1S(e, 0x233));           /* MUST HAVE BOTH FRIEND AND FOE SQUADS. */
        e->state = SQ_ERROR;
        e->wait = (int)(50000 * SOUND_UNIT_US * 1e-6 * EDIT_HZ);   /* delay(50000) */
        return;
    }
    e->result = r;
}

void sqedit_key(SquadEdit *e, int k)
{
    if (e->result != SQEDIT_RUNNING || !k) return;
    Play *p = &e->p;
    PSquad *q = sel_squad(e);
    switch (e->state) {
    case SQ_INTRO:
        snd_beep();
        clear_row(p, 23);
        main_line(e);
        return;
    case SQ_TOTAL:
        snd_beep();
        clear_row(p, 23);
        main_line(e);
        return;
    case SQ_INFO: info_key(e, k); return;
    case SQ_SIZE: size_key(e, k); return;
    case SQ_FORMATION: formation_key(e, k); return;
    case SQ_ASK:
        if (e->wait > 0) return;
        if (k == PK_Y) {
            if (e->new_campaign) {
                clear_row(p, 23);
                finish(e, SQEDIT_NEXT);
            } else {
                print(p, O1S(e, 0x190));       /* Yes */
                e->answer_yes = true;
                e->state = SQ_ANSWER;
                e->wait = 8;
            }
        } else if (k == PK_N) {
            if (e->new_campaign) main_line(e);
            else {
                print(p, O1S(e, 0x20C));       /* No */
                e->answer_yes = false;
                e->state = SQ_ANSWER;
                e->wait = 8;
            }
        }
        return;
    case SQ_ANSWER: case SQ_ERROR:
        return;
    case SQ_MAIN:
        if (k == PK_T) {                       /* Total men .... N friend, N foe. */
            snd_beep();
            int mine = 0, theirs = 0;
            for (int i = 0; i < MAX_SQUADS; i++) {
                const PSquad *s = &p->sq[i];
                if (!s->used) continue;
                if (s->friendly) mine += men(s); else theirs += men(s);
            }
            clear_row(p, 23);
            print(p, O1S(e, 0x19A));
            print_num(p, mine);
            print(p, O1S(e, 0x1AA));
            print_num(p, theirs);
            print(p, O1S(e, 0x1B4));
            clear_row(p, 24);
            print(p, O1S(e, 0x1BA));           /* End */
            e->state = SQ_TOTAL;
            return;
        }
        if (k == PK_ENTER) { snd_beep(); ask(e); return; }
        if (k == PK_S) {                       /* a new squad at the cursor (ovl3_4552) */
            int i = MAX_SQUADS - 1;
            while (i >= 0 && p->sq[i].used) i--;
            if (i < 0) break;
            snd_beep();
            p->sel = i;
            PSquad *n = &p->sq[i];
            n->knights = n->barbarians = n->archers = n->spies = 0;
            n->speed = 2;
            n->friendly = true;
            n->condition = n->food = 35;
            n->formation = 1;
            n->x = p->cx - 1;
            n->y = p->cy - 3;
            e->over_squad = true;
            size_start(e);
            return;
        }
        if (k == PK_F) {                       /* a new flag at the cursor */
            int i = MAX_FLAGS - 1;
            while (i >= 0 && p->flag[i].present) i--;
            if (i < 0) break;
            snd_beep();
            e->flag_sel = i;
            p->flag[i].present = true;
            p->flag[i].white = true;
            p->flag[i].x = p->cx - 1;
            p->flag[i].y = p->cy - 3;
            e->over_flag = true;
            flag_line(e);
            return;
        }
        break;
    case SQ_SQUAD:
        if (k == PK_R) {                       /* Remove (ovl3_4516) */
            snd_beep();
            q->knights = q->barbarians = q->archers = q->spies = 0;
            update_squads(e);
            main_line(e);
            return;
        }
        if (k == PK_I) { snd_beep(); info_start(e); return; }
        if (k == PK_S) { snd_beep(); size_start(e); return; }
        if (k == PK_F && !q->small && q->friendly) { snd_beep(); formation_start(e); return; }
        if (k == PK_G) {                       /* Get (ovl3_35AF) */
            snd_beep();
            clear_row(p, 23);
            print(p, O1S(e, 0x5E));            /* Move to the new location */
            clear_row(p, 24);
            print(p, O1S(e, 0x1BE));           /* Put */
            e->state = SQ_MOVE;
            return;
        }
        break;
    case SQ_MOVE:
        if (k == PK_P) {                       /* Put */
            snd_beep();
            q->x = p->cx - 1;
            q->y = p->cy - 3;
            squad_line(e);
            return;
        }
        break;
    case SQ_FLAG:
        if (k == PK_R) {                       /* Remove */
            snd_beep();
            p->flag[e->flag_sel].present = false;
            e->over_flag = false;
            main_line(e);
            return;
        }
        if (k == PK_F) {                       /* Friend or Foe */
            snd_beep();
            p->flag[e->flag_sel].white = !p->flag[e->flag_sel].white;
            flag_line(e);
            return;
        }
        break;
    default:
        break;
    }
    /* -/+ scroll the map (the cursor keys, held down, come from sqedit_frame) */
    if (cursor_state(e) && (k == PK_MINUS || k == PK_KP_MINUS || k == PK_EQUALS || k == PK_KP_PLUS)) cursor_key(e, k);
}

void sqedit_frame(SquadEdit *e, int held)
{
    if (e->result != SQEDIT_RUNNING) return;
    if (e->wait > 0) e->wait--;
    if (e->state == SQ_ANSWER && e->wait == 0) {
        if (e->answer_yes) finish(e, SQEDIT_SAVE);
        else e->result = SQEDIT_DISCARD;
    }
    if (e->state == SQ_ERROR && e->wait == 0) main_line(e);
    if (cursor_state(e) && held && held != PK_KP_MINUS && held != PK_KP_PLUS) cursor_key(e, held);
}

void sqedit_draw(const SquadEdit *e, Screen *s, GfxMode mode)
{
    play_draw(&e->p, s, mode);
    const AssetSet *set = &e->ga->set[mode];
    int ncolors = set->loaded ? set->tiles[0].ncolors : 4;
    uint8_t col[4];
    for (int i = 0; i < 4; i++) col[i] = screen_nearest(s, ncolors, CGA_PALETTE[i].r, CGA_PALETTE[i].g, CGA_PALETTE[i].b);

    if (e->state == SQ_MAIN) {                 /* [Enter] when done (ovl3:3450) */
        int w = 0, h = 0;
        assets_ds_bitmap_px(e->ga, 0x424, 0, 0, &w, &h);
        for (int y = 0; y < h; y++)
            for (int x = 0; x < w; x++)
                s->px[(0xC0 + y) * SCREEN_W + 0x36 * 4 + x] = col[assets_ds_bitmap_px(e->ga, 0x424, x, y, NULL, NULL)];
    }
    if (e->state == SQ_SIZE) {                 /* the box of men (ovl3_4BBC) */
        const PSquad *q = &e->p.sq[SDL_clamp(e->p.sel, 0, MAX_SQUADS - 1)];
        for (int y = 0x92; y < 0x92 + 0x2A; y++) {
            for (int x = 0; x < 0x23 * 4; x++) s->px[y * SCREEN_W + x] = col[x < 2 ? 3 : 0];
            s->px[y * SCREEN_W + 0x23 * 4 + 2] = col[3];           /* draw_vline */
            s->px[y * SCREEN_W + 0x23 * 4 + 3] = col[3];
        }
        screen_fill(s, 0, 0x92, 0x24 * 4, 1, col[3]);              /* draw_hline */
        screen_fill(s, 0, 0xBC, 0x24 * 4, 1, col[3]);
        static const unsigned LABEL[4] = { 0x112, 0x120, 0x12E, 0x13C };   /* KNIGHTS, BARBARIANS, ARCHERS, SPIES */
        int n[4] = { q->knights, q->barbarians, q->archers, q->spies };
        for (int r = 0; r < 4; r++) {
            char num[8];
            SDL_snprintf(num, sizeof num, "%2d", n[r]);
            screen_text(s, e->ga, 8, (19 + r) * 8, O1S(e, LABEL[r]), col[3], col[0]);
            screen_text(s, e->ga, 15 * 8, (19 + r) * 8, num, col[3], col[0]);
        }
    }
}

void sqedit_store(const SquadEdit *e, Campaign *c)
{
    uint8_t *a = c->armies;
#define PUT(off, i, v) do { int16_t w_ = (int16_t)(v); a[(off) + 2 * (i)] = (uint8_t)w_; a[(off) + 2 * (i) + 1] = (uint8_t)(w_ >> 8); } while (0)
    for (int i = 0; i < MAX_SQUADS; i++) {
        const PSquad *q = &e->p.sq[i];
        PUT(0, i, q->friendly ? -1 : 0);
        PUT(40, i, q->speed);
        PUT(80, i, q->used ? 0 : 9999);
        PUT(120, i, q->x);
        PUT(160, i, q->y);
        a[200 + i] = (uint8_t)q->formation;
        a[220 + i] = (uint8_t)q->condition;
        a[240 + i] = (uint8_t)q->food;
        a[320 + i] = (uint8_t)q->archers;
        a[340 + i] = (uint8_t)q->knights;
        a[360 + i] = (uint8_t)q->barbarians;
        a[380 + i] = (uint8_t)q->spies;
    }
    for (int i = 0; i < MAX_FLAGS; i++) {
        const PFlag *f = &e->p.flag[i];
        PUT(260, i, f->present ? f->x : -1);
        PUT(280, i, f->y);
        PUT(300, i, f->white ? -1 : 0);
    }
#undef PUT
    campaign_refresh(c);
}
