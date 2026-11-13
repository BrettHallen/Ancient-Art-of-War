/*
 * The Ancient Art of War (1984) - remake
 * Nov 2026 using Claude Opus 5.5
 * "Guided" by Brett Hallen
 * Original game by Dave Murry & Barry Murry
 * Published by Brøderbund Software Inc.
 *
 * battle.c: step 4, the battle screen (Zoom; WAR.EXE overlay 7).
 *
 * Each function names the WAR.EXE routine it follows. Positions are in the
 * units of the off-screen buffer: x a byte column (4 pixels), y a line pair;
 * addr = y * 80 + x.
 */
#include "battle.h"
#include "sound.h"
#include "gamelog.h"

#include <SDL3/SDL.h>

#define O6(off) (off)
#define O7(off) (BMEM_O7 + (off))
#define FRIEND  (-1)
#define ENEMY   0
#define END_FRIEND 101          /* scripts of your soldiers end with 101 */
#define END_ENEMY  102
#define DEAD    9

/* the debug log (gamelog.h): only reads the battle, never calls random() */
static void blog(const Battle *b, const char *fmt, ...) SDL_PRINTF_VARARG_FUNC(2);
static void blog(const Battle *b, const char *fmt, ...)
{
    if (!gamelog_on(b->p, GLOG_NORMAL)) return;
    char t[320];
    va_list ap;
    va_start(ap, fmt);
    SDL_vsnprintf(t, sizeof t, fmt, ap);
    va_end(ap);
    gamelog_printf(b->p, "BATTLE frame %4d (%3d.%d s) %s", b->frame, b->frame / BATTLE_HZ, b->frame % BATTLE_HZ * 10 / BATTLE_HZ, t);
}
#define BLOG(...) blog(b, __VA_ARGS__)
static const char *const MAN[3] = { "knight", "barbarian", "archer" };
static const char *const GROUND[14] = {
    "both in forts", "you on a hill", "the enemy on a hill", "you hold a fort", "the enemy holds a fort",
    "a bridge", "a river", "a forest", "a village", "a forest", "mountains", "hills", "plains", "plains" };

enum { KNIGHT, BARBARIAN, ARCHER };

/* ------------------------------------------------------------------ data */

/* Soldier pictures, by number (the table at DS:A931) */
static const int SPRITE[47] = {
    O6(0x798), O6(0x000), O6(0x074), O6(0x158), O6(0x158), O6(0x408), O6(0x640), O6(0x394), O6(0x204), O6(0x2E8),
    O6(0x2E8), O6(0x524), O6(0x6EC), O6(0xAF4), O6(0xB68), O6(0xC14), O6(0xE1C), O6(0xDA8), O6(0xC88), O6(0xD34),
    O6(0xEC8), O6(0xF74), O6(0x1020), O6(0x1020), O6(0x10CC), O6(0x11D0), O6(0x12D4), O6(0x1380), O6(0x1380), O6(0x1464),
    O6(0x1568), O6(0x166C), O6(0x16E8), O6(0x1764), O6(0x181C), O6(0x18D4), O6(0x1958), O6(0x844), O6(0x8B8), O6(0x99C),
    O6(0xA10), O7(0x434), O7(0x4A8), O7(0x51C), O7(0x590), O7(0x6B0), O7(0x604),
};

/* Arrow pictures by direction 1-20 (DS:0B40) */
static const int ARROW[21] = {
    0, O6(0x1A1C), O6(0x1A34), O6(0x1A60), O6(0x1A84), O6(0x1A84), O6(0x1AA0), O6(0x1AB4), O6(0x1AB4), O6(0x1AD0),
    O6(0x1AF4), O6(0x1B20), O6(0x1B38), O6(0x1B64), O6(0x1B88), O6(0x1B88), O6(0x1BA4), O6(0x1BB8), O6(0x1BB8),
    O6(0x1BD4), O6(0x1BF8),
};

/* The animation scripts: each action has one for knights, barbarians and
 * archers (pointer tables at DS:AAF1-AB73, named here by their address). */
enum {
    S_AAF1, S_AAF7, S_AAFD, S_AB03, S_AB09, S_AB0F, S_AB15, S_AB1B, S_AB21, S_AB27, S_AB2D,
    S_AB33, S_AB39, S_AB3F, S_AB45, S_AB4B, S_AB51, S_AB57, S_AB5D, S_AB63, S_AB69, S_AB6F, S_COUNT
};
static const int SCRIPTS[S_COUNT][3] = {
    [S_AAF1] = { O6(0x1C8A), O6(0x1E5A), O7(0x38) },  /* enemy: standing */
    [S_AAF7] = { O6(0x1D74), O6(0x1EE8), O7(0x13A) }, /* yours: standing */
    [S_AAFD] = { O6(0x1DDE), O7(0x1C), O7(0x13A) },   /* yours: getting ready */
    [S_AB03] = { O6(0x1CF4), O6(0x1EC4), O7(0x38) },  /* enemy: getting ready */
    [S_AB09] = { O6(0x1CAE), O6(0x1E7E), O7(0x4E) },  /* enemy: fighting */
    [S_AB0F] = { O6(0x1D98), O6(0x1F0C), O7(0x150) }, /* yours: fighting, backing off */
    [S_AB15] = { O6(0x1CA0), O6(0x1E70), O7(0x40) },  /* to the right */
    [S_AB1B] = { O6(0x1D8A), O6(0x1EFE), O7(0x142) }, /* to the left */
    [S_AB21] = { O6(0x1CBC), O6(0x1E8C), O7(0x62) },  /* up */
    [S_AB27] = { O6(0x1DA6), O6(0x1F1A), O7(0x15E) }, /* up */
    [S_AB2D] = { O6(0x1CCA), O6(0x1E9A), O7(0x70) },  /* down */
    [S_AB33] = { O6(0x1DB4), O6(0x1F28), O7(0x16C) }, /* down */
    [S_AB39] = { O6(0x1DEA), O6(0x1F36), O7(0x13A) }, /* yours: stand */
    [S_AB3F] = { O6(0x1CFC), O6(0x1ED2), O7(0x38) },  /* enemy: stand */
    [S_AB45] = { O6(0x1DC2), O7(0x00), O7(0x17A) },
    [S_AB4B] = { O6(0x1CD8), O6(0x1EA8), O7(0x7E) },
    [S_AB51] = { O6(0x1DD0), O7(0x0E), O7(0x188) },
    [S_AB57] = { O6(0x1CE6), O6(0x1EB6), O7(0x8C) },
    [S_AB5D] = { O6(0x1D04), O6(0x1EDA), O7(0x38) },
    [S_AB63] = { O6(0x1DF2), O7(0x2A), O7(0x13A) },
    [S_AB69] = { O6(0x1C92), O6(0x1E62), O7(0x38) },
    [S_AB6F] = { O6(0x1D7C), O6(0x1EF0), O7(0x13A) },
};

static int rd(const Battle *b, int off) { return (int16_t)(b->mem[off] | (b->mem[off + 1] << 8)); }
static void wr(Battle *b, int off, int v) { b->mem[off] = (uint8_t)v; b->mem[off + 1] = (uint8_t)(v >> 8); }
static int rdb(const Battle *b, int off) { return (int8_t)b->mem[off]; }
static int S(int name, int type) { return SCRIPTS[name][type]; }

static int R(Battle *b) { return play_rnd(b->p); }
#define RND (b->p->rnd)
static PSquad *SQ(Battle *b, int i) { return &b->p->sq[i]; }

static bool t_fort(int t)  { return t == 3; }
static bool t_village(int t) { return t == 4; }
static bool t_hill(int t)  { return t == 5 || t == 6; }
static bool t_bridge(int t) { return t == 9 || t == 10; }
static bool t_water(int t) { return t == 7 || t == 8 || (t >= 13 && t <= 15) || (t >= 19 && t <= 36); }
static bool t_mountain(int t) { return (t >= 37 && t <= 54) || t == 65 || t == 66 || t == 71 || t == 72; }
static bool t_forest(int t) { return (t >= 55 && t <= 57) || (t >= 61 && t <= 63) || (t >= 67 && t <= 69); }
static bool t_p7(int t) { return (t >= 58 && t <= 60) || t == 11 || t == 12 || t == 64 || t == 70 || (t >= 16 && t <= 18); }   /* ovl7:60B0 */

/* ------------------------------------------------------------------ text rows 23 and 24 */

static void tclear(Battle *b, int row) { SDL_memset(b->text[row - 23], ' ', 40); b->text[row - 23][40] = 0; b->trow = row; b->tcol = 0; }
static void tgoto(Battle *b, int col, int row) { b->trow = row; b->tcol = col; }
static void tprint(Battle *b, const char *s)
{
    for (; *s; s++) {
        if (b->trow >= 23 && b->trow <= 24 && b->tcol < 40) b->text[b->trow - 23][b->tcol] = *s;
        b->tcol++;
    }
}
static void tnum(Battle *b, int n) { char t[12]; SDL_snprintf(t, sizeof t, "%d", n); tprint(b, t); }
static const char *O6S(const Battle *b, int off) { return (const char *)b->mem + off; }
static const char *DSS(const Battle *b, unsigned off) { return assets_ds_string(b->p->ga, off); }

/* ovl7_6ae9: a bugle call from O7 (pairs of timer divisor and length).
 * WAR.EXE plays it before going on, so the battle waits while it sounds. */
static void bugle(Battle *b, unsigned off)
{
    snd_bugle(b->mem + O7(off), 0);
    b->hold = (int)SDL_ceil(sound_busy() * BATTLE_HZ);
}

static void show_groups(Battle *b)     /* ovl7:5148: Archer Barbarian Knight Squad   Retreat */
{
    tclear(b, 24);
    if (b->alive_type[ARCHER] && b->bg != 3) tprint(b, O6S(b, 0x1C51));
    if (b->alive_type[BARBARIAN]) tprint(b, O6S(b, 0x1C59));
    if (b->alive_type[KNIGHT]) tprint(b, O6S(b, 0x1C64));
    if (b->bg != 3) tprint(b, O6S(b, 0x1C6B));
    tgoto(b, 32, 24);
    tprint(b, O6S(b, 0x1C49));
    b->menu = 1;
}

static void show_orders(Battle *b)     /* ovl7:5204: Attack Forward Backoff Stop */
{
    snd_beep();                                /* ovl7:5207 */
    tclear(b, 24);
    if (b->bg != 3) tprint(b, O6S(b, 0x1C41));
    tprint(b, O6S(b, 0x1C72));
    tprint(b, O6S(b, 0x1C7B));
    tprint(b, O6S(b, 0x1C84));
    b->menu = 2;
}

static void show_losses(Battle *b)     /* ovl7:46A9: You lost N men ....  They lost N men */
{
    tclear(b, 23);
    tclear(b, 24);
    if (b->lost_me) {
        tprint(b, O6S(b, 0x1C24)); tnum(b, b->lost_me);
        tprint(b, DSS(b, b->lost_me > 1 ? 0x2C5 : 0x2C9));
        tprint(b, O6S(b, 0x1C39));
    }
    if (b->lost_en) {
        tprint(b, O6S(b, 0x1C2E)); tnum(b, b->lost_en);
        tprint(b, DSS(b, b->lost_en > 1 ? 0x2CD : 0x2D2));
    }
}

static void alive_types(Battle *b)     /* ovl7:5057 */
{
    b->alive_type[0] = b->alive_type[1] = b->alive_type[2] = false;
    for (int i = 0; i < MAX_MEN; i++)
        if (b->stat[i] != DEAD && b->addr[i] != -1 && b->side[i] == FRIEND) b->alive_type[b->type[i]] = true;
}

/* ------------------------------------------------------------------ setup */

/* rt:09D3: the men of a squad take the places of its formation */
static int place_formation(Battle *b, int first, int squad, int side01)
{
    const uint8_t *F = b->p->ga->formations;
    PSquad *q = SQ(b, squad);
    int form = SDL_clamp(q->formation, 0, 9);
    int ar = q->archers, ba = q->barbarians, kn = q->knights;
    for (int k = 0; k < 14 && first < MAX_MEN; k++) {
        int t = F[form * 14 + k];
        if (t == ARCHER && ar) ar--;
        else if (t == BARBARIAN && ba) ba--;
        else if (t == KNIGHT && kn) kn--;
        else if (kn) { kn--; t = KNIGHT; }
        else if (ba) { ba--; t = BARBARIAN; }
        else if (ar) { ar--; t = ARCHER; }
        else break;
        b->type[first] = t;
        b->side[first] = side01 == 0 ? FRIEND : ENEMY;
        b->x[first] = F[140 + form * 14 + k] - (t == ARCHER ? 2 : 0);
        b->y[first] = F[280 + form * 14 + k];
        b->addr[first] = 0;
        first++;
    }
    return first;
}

/* ovl7:4738: the two squads, the enemy's formation, the background, the men */
static void battle_setup(Battle *b, int sel)
{
    Play *p = b->p;
    PSquad *s = SQ(b, sel);
    if (s->friendly) { b->me = sel; b->en = s->foe; }
    else             { b->en = sel; b->me = s->foe; }
    PSquad *me = SQ(b, b->me), *en = SQ(b, b->en);
    me->men = me->archers + me->knights + me->barbarians;
    en->men = en->archers + en->knights + en->barbarians;
    if (me->formation == 0) me->formation = 1;

    int form, opp = p->c->opponent;     /* the enemy's formation */
    R(b);
    if (opp == 1 || opp == 4) form = RND < 90 ? 3 : 2;
    else if (opp == 2) form = RND < 64 ? 5 : 4;
    else if (opp == 0) form = RND < 90 ? 4 : 2;
    else if (opp == 3) form = RND < 64 ? 5 : 1;
    else if (en->archers && en->knights + en->barbarians > 4) form = 2;
    else if (!en->archers && me->archers) form = 4;
    else form = 3;
    if (!me->archers && en->archers && R(b) < 100) form = 1;
    en->formation = form;

    int tm = play_tile_under(p, b->me), te = play_tile_under(p, b->en);
    R(b);
    if (t_fort(tm) && t_fort(te))           b->bg = 0;
    else if (t_fort(te))                    { en->formation = 0; b->bg = 4; }
    else if (t_fort(tm))                    { me->formation = 0; b->bg = 3; }
    else if (t_village(tm) || t_village(te)) b->bg = 8;
    else if (t_hill(tm) && !t_hill(te))     b->bg = 1;
    else if (t_hill(te) && !t_hill(tm))     b->bg = 2;
    else if (t_water(tm) || t_water(te))    b->bg = 6;
    else if (t_p7(tm))                      b->bg = 9;
    else if (t_forest(tm))                  b->bg = 7;
    else if (t_mountain(tm))                b->bg = 10;
    else if (t_bridge(tm) || t_bridge(te))  b->bg = 5;
    else if (t_hill(te) && t_hill(tm))      b->bg = 11;
    else                                    b->bg = R(b) < 75 ? 12 : 13;

#ifdef BATTLE_FORCE_BG                 /* tests: every background */
    if (BATTLE_FORCE_BG >= 0) {
        b->bg = BATTLE_FORCE_BG;
        if (b->bg == 3) me->formation = 0;
        if (b->bg == 4) en->formation = 0;
    }
#endif
    for (int i = 0; i < MAX_MEN; i++) b->addr[i] = -1;
    int n = place_formation(b, 0, b->me, 0);
    place_formation(b, n, b->en, 1);

    int e = play_edge(p, b->me, b->en) * 2;
    e = SDL_clamp(e, -40, 40);
    b->skill_me = 45 + e;
    b->skill_en = 45 - e;
}

/* P-file for each background number (A5CA): P0 is 0, P1 4, P2 3 ... */
static int picture_of(int bg)
{
    static const int P[14] = { 0, 4, 5, 2, 1, 10, 6, 8, 3, 7, 9, 11, 12, 13 };
    return P[bg];
}

/* ------------------------------------------------------------------ soldiers */

static void retreat(Battle *b);

/* ovl7:61CE: soldier j falls */
#ifndef BATTLE_TRACE
#define BATTLE_TRACE(...) ((void)0)
#endif

static void kill(Battle *b, int j)
{
    Play *p = b->p;
    BATTLE_TRACE("kill %d (side %d type %d at %d,%d)", j, b->side[j], b->type[j], b->x[j], b->y[j]);
    int sq;
    if (b->side[j] == FRIEND) { sq = b->me; b->lost_me++; p->lost_mine++; }
    else                      { sq = b->en; b->lost_en++; p->lost_theirs++; }
    BLOG("%s %s falls (you have lost %d, they %d)", b->side[j] == FRIEND ? "your" : "their", MAN[b->type[j] % 3],
         b->lost_me, b->lost_en);
    PSquad *q = SQ(b, sq);
    int *count = b->type[j] == ARCHER ? &q->archers : b->type[j] == KNIGHT ? &q->knights : &q->barbarians;
    (*count)--;
    if (b->side[j] == FRIEND && *count == 0 && b->menu != 2) {
        b->alive_type[b->type[j]] = false;
        show_groups(b);
    }
    q->men--;
    if (b->bg == 3 && b->side[j] == FRIEND)
        b->script[j] = b->y[j] < 42 ? O6(0x1E00) : b->y[j] < 60 ? O6(0x1E0C) : O6(0x1E24);   /* falling from the wall */
    else if (b->bg == 4 && b->side[j] == ENEMY)
        b->script[j] = b->y[j] < 42 ? O6(0x1D12) : b->y[j] < 60 ? O6(0x1D1E) : O6(0x1D30);
    else if (b->state[j] == END_FRIEND)
        b->script[j] = b->x[j] > 70 ? O6(0x1E38) : O6(0x1E46);
    else
        b->script[j] = b->x[j] < 10 ? O6(0x1D4A) : O6(0x1D58);
    if (!b->retreat) {
        if (q->archers <= 0 && (b->bg == 3 || b->bg == 4)) {   /* a fort without archers is lost */
            b->retreat = sq == b->en ? 2 : 1;
            retreat(b);
        } else if (sq == b->en && b->skill_en < 6 && R(b) > b->skill_en + 115 && b->bg != 4 &&
                   p->c->opponent != 0 && p->c->opponent != 4) {
            b->retreat = 2;
            retreat(b);
        } else if (sq == b->me && b->skill_me < 6 && R(b) > b->skill_me + 115 && b->bg != 3) {
            b->retreat = 3;                 /* your men break and run */
            retreat(b);
        }
    }
    b->stat[j] = DEAD;
    b->shot[j] = -1;
    snd_kill();                                /* ovl7:6511 */
}

/* ovl7:438A: one side retreats; it moves away on the map */
static void retreat(Battle *b)
{
    Play *p = b->p;
    PSquad *me = SQ(b, b->me), *en = SQ(b, b->en);
    BLOG("%s", b->retreat == 1 ? "your men retreat" : b->retreat == 2 ? "the enemy retreats" : "your men break and run");
    if (b->bg == 3) for (int t = 0; t < 3; t++) wr(b, S(S_AB1B, t), 0);
    if (b->bg == 4) for (int t = 0; t < 3; t++) wr(b, S(S_AB15, t), 2);
    tclear(b, 23);
    tclear(b, 24);
    /* the bugle (ovl7:44BF, 45F1): you sound the retreat, or the enemy does;
     * not when your men break and run */
    if (b->retreat == 1 && me->men) bugle(b, 0x7A8);
    if (b->retreat == 2 && en->men) bugle(b, 0x7BE);
    b->start = 0;
    b->endtimer = 75;
    b->key = 0;
    R(b);
    int dx = (me->x - en->x > 0) ? (RND & 3) + 1 : -(RND & 3) - 1;
    int dy = (me->y - en->y > 0) ? (RND & 7) : -(RND & 7);
    if (b->bg >= 1 && b->bg <= 4) { dx *= 2; dy *= 2; }
    if (b->retreat == 1 || b->retreat == 3) {
        b->retreat = 1;
        if (me->x > 64 || me->x < 16) dx = 0;
        if (me->y > 164 || me->y < 21) dy = 0;
        me->x += dx;
        me->y += dy;
        if (b->bg == 3) {
            for (int t = 0; t < 3; t++) { b->order[t] = 3; b->order_time[t] = 10; }
            me->formation = 1;
        }
        play_clear_path(p, b->me);
        en->condition = SDL_min(en->condition * 2 + 5, 98);
        b->skill_en = 85;
        BLOG("your squad %d falls back to %d,%d; the enemy's condition rises to %d", b->me, me->x, me->y, en->condition);
    } else {
        if (en->x > 64 || en->x < 16) dx = 0;
        if (en->y > 164 || en->y < 21) dy = 0;
        en->x -= dx;
        en->y -= dy;
        play_clear_path(p, b->en);
        b->skill_me = 85;
        BLOG("the enemy's squad %d falls back to %d,%d", b->en, en->x, en->y);
    }
}

/* ovl7:6D01: the nearest enemy (someone attacking him first, and the type he beats) */
static void choose_target(Battle *b, int i);

static void check_target(Battle *b, int i)    /* ovl7:6C4E */
{
    b->target = b->tgt[i];
    if (b->target >= 0 && b->stat[b->target] == DEAD) b->target = -1;
    if (b->target > -1) {
        int dx = SDL_abs(b->x[i] - b->x[b->target]), dy = SDL_abs(b->y[i] - b->y[b->target]);
        if (dx > 4 || dy > 3) b->target = -2;          /* not within reach */
    } else {
        choose_target(b, i);
    }
}

static void choose_target(Battle *b, int i)
{
    int best = 500;
    for (int j = 0; j < MAX_MEN; j++) {
        if (b->addr[j] == -1 || b->stat[j] == DEAD || b->side[j] == b->side[i]) continue;
        int dx = SDL_abs(b->x[i] - b->x[j]), dy = SDL_abs(b->y[i] - b->y[j]);
        if (b->tgt[j] == i && best > 8) { best = 8; b->target = j; }
        if ((b->type[i] == BARBARIAN && b->type[j] == ARCHER) || (b->type[i] == KNIGHT && b->type[j] == BARBARIAN)) dx >>= 1;
        if (dy + dx < best) { best = dy + dx; b->target = j; }
    }
    if (best == 500) {                  /* no one left to fight */
        b->target = -1;
        if (b->endtimer == 0) {
            b->endtimer = 7;
            if (b->bg == 3 || b->bg == 4) b->endtimer += 2;
            PSquad *loser = SQ(b, b->side[i] == FRIEND ? b->en : b->me);   /* spies are captured */
            if (b->side[i] == FRIEND) b->p->captured_theirs += loser->spies;
            else b->p->captured_mine += loser->spies;
            loser->spies = 0;
            if (b->side[i] == FRIEND) play_enemy_beaten(b->p, b->en);   /* it may give up its target */
            for (int k = 0; k < MAX_ARROWS; k++) b->ax[k] = 0;
            tclear(b, 23);
            tclear(b, 24);
        }
    }
    b->tgt[i] = b->target;
    if (b->target >= 0) check_target(b, i);
}

/* ovl7:6F48: which way to go to reach the target: -160/0/160 +-1 */
static int toward(Battle *b, int i)
{
    b->target = b->tgt[i];
    if (b->target < 0) return 0;
    int t = b->target, d;
    int dy = b->y[i] - b->y[t];
    d = dy < 0 ? 160 : dy > 0 ? -160 : 0;
    int dx = b->x[i] - b->x[t];
    if (dx < -4) d++;
    else if (dx > 4) d--;
    if ((R(b) < 30 && !b->retreat) || b->bg == 3 || b->bg == 4) d = 0;
    return d;
}

/* ovl7:700C: close enough to fight, or stepping round a friend in the way */
static bool close_in(Battle *b, int i)
{
    int t = b->target;
    int dx = SDL_abs(b->x[i] - b->x[t]), dy = SDL_abs(b->y[i] - b->y[t]);
    if (dx < 3 && dy < 2) {
        b->script[i] = S(b->side[i] == ENEMY ? S_AB09 : S_AB0F, b->type[i]);
        return true;
    }
    for (int j = 0; j < MAX_MEN; j++) {
        if (b->side[j] != b->side[i] || j == i) continue;
        int d = b->x[i] - b->x[j];
        if (d <= -2 || d >= 2 || b->y[i] != b->y[j]) continue;
        b->script[i] = S(R(b) < 65 ? S_AB45 : S_AB51, b->type[i]);
        return true;
    }
    return false;
}

/* ovl7:745B: an archer looks for someone to shoot at */
static bool archer_aim(Battle *b, int i)
{
    int slot = 0;
    while (slot < MAX_ARROWS && b->ax[slot] != 0) slot++;
    PSquad *me = SQ(b, b->me), *en = SQ(b, b->en);
    int own = (b->side[i] == FRIEND ? me->archers : en->archers) - 1;
    int foes = (b->side[i] == FRIEND ? en->men : me->men) - 1;
    int chance = rd(b, O7(0x7D0) + 2 * SDL_clamp(own, 0, 13));
    if (slot > 9) return false;
    if (!(R(b) <= chance || b->retreat)) return false;
    if (b->endtimer > 0 && b->endtimer < 10) return false;
    if (b->shot[i] != -1) return false;
    for (int j = 0; j < MAX_MEN; j++) {
        if (b->side[i] == b->side[j] || b->addr[j] == -1 || b->stat[j] == DEAD) continue;
        if (b->bg == 3 && b->x[j] > 65) continue;
        if (b->bg == 4 && b->x[j] < 15) continue;
        int dir = 0, sc = 0;
        if (b->x[i] > b->x[j] - 3 && b->x[i] < b->x[j] + 3) {          /* straight up or down */
            if (b->y[i] > b->y[j]) { dir = 1; sc = O7(0x114); } else { dir = 11; sc = O7(0xEE); }
        } else if (b->y[i] > b->y[j] - 3 && b->y[i] < b->y[j] + 4) {   /* straight across */
            if (b->x[i] > b->x[j]) { dir = 16; sc = O7(0x196); } else { dir = 6; sc = O7(0x9A); }
        } else {
            int left = b->x[i] > b->x[j];          /* l12 != 0: the target is to the left */
            int up = b->y[i] > b->y[j];            /* l8 != 0: the target is above */
            int adx = SDL_abs(b->x[i] - b->x[j]), ady = SDL_abs(b->y[i] - b->y[j]);
            int ratio, steep;
            if (ady >= adx) { ratio = ady / adx; steep = 0; } else { ratio = adx / ady; steep = 1; }
            if (ratio == 1 && !steep) {
                if (left) { if (up) { dir = 19; sc = O7(0x1B6); } else { dir = 13; sc = O7(0x1D6); } }
                else      { if (up) { dir = 3; sc = O7(0xBA); } else { dir = 9; sc = O7(0xD4); } }
            } else if (ratio == 2 && !steep) {
                if (left) { if (up) { dir = 20; sc = O7(0x1B6); } else { dir = 12; sc = O7(0x1D6); } }
                else      { if (up) { dir = 2; sc = O7(0xBA); } else { dir = 10; sc = O7(0xD4); } }
            } else if (ratio == 2 || (ratio == 1 && steep)) {
                if (left) { if (up) { dir = 18; sc = O7(0x1B6); } else { dir = 14; sc = O7(0x1D6); } }
                else      { if (up) { dir = 4; sc = O7(0xBA); } else { dir = 8; sc = O7(0xD4); } }
            } else if (ratio >= 3 && ratio < 10 && steep) {
                if (left) { dir = up ? 17 : 15; sc = O7(0x196); }
                else      { dir = up ? 5 : 7; sc = O7(0x9A); }
            }
        }
        if (sc) b->script[i] = sc;
        int fire_chance = rd(b, O7(0x7D0) + 2 * SDL_clamp(foes, 0, 13));
        if (dir != 0 && ((R(b) < fire_chance || b->retreat) || dir == 6 || dir == 16)) {
            b->akind[slot] = dir;
            b->ax[slot] = 9999;             /* loosed when the shooting picture comes (ovl7:7A52) */
            if (dir == 11) { b->ay[slot] = b->y[i] + 3; b->aaddr[slot] = b->addr[i] + 0x280; }
            else           { b->ay[slot] = b->y[i]; b->aaddr[slot] = b->addr[i] + 0x140; }
            b->shot[i] = slot;
            b->aside[slot] = b->side[i];
            return true;
        }
        foes--;
    }
    return false;
}

static void loose_arrow(Battle *b, int i)   /* ovl7:7A52 */
{
    if (b->shot[i] == -1) return;
    int s = b->shot[i];
    b->ax[s] = b->x[i];
    b->apow[s] = (b->side[i] == FRIEND ? b->skill_me : b->skill_en) * 2 + 40;
    b->shot[i] = -1;
    snd_arrow();                               /* ovl7:7ADD */
}

/* ovl7:5CBF: keep a soldier on the field, take his next script step, draw him */
static void step(Battle *b, int i)
{
    int *ps = &b->script[i];
    int T = b->type[i];
    bool mine = b->state[i] == END_FRIEND;
    if (b->stat[i] != DEAD) {
        if (b->x[i] > 72 && b->stat[i] != 3) {
            if (b->retreat) { b->addr[i] = -1; b->x[i] += 10; return; }   /* gone */
            *ps = S(S_AB1B, T);
        } else if (b->x[i] < 1 && b->stat[i] != 4) {
            if (b->retreat) { b->addr[i] = -1; b->x[i] -= 10; return; }
            *ps = S(S_AB15, T);
        } else if (b->y[i] < b->ymin[SDL_clamp(b->x[i], 0, 79)]) {
            if (b->bg == 1) *ps = S(mine ? S_AB51 : S_AB2D, T);
            else if (b->bg == 2) *ps = S(mine ? S_AB33 : S_AB57, T);
            else *ps = S(mine ? S_AB33 : S_AB2D, T);
        } else if (b->y[i] > b->ymax[SDL_clamp(b->x[i], 0, 79)]) {
            if (b->bg == 1) *ps = S(mine ? S_AB27 : S_AB4B, T);
            else if (b->bg == 2) *ps = S(mine ? S_AB45 : S_AB21, T);
            else *ps = S(mine ? S_AB27 : S_AB21, T);
        }
    }
    int sp = *ps;
    int d = rd(b, sp);
    b->addr[i] += d;
    if (d < -10)     { b->y[i] -= 2; b->x[i] += d + 80; b->addr[i] -= 80; }
    else if (d > 10) { b->y[i] += 2; b->x[i] += d - 80; b->addr[i] += 80; }
    else             b->x[i] += d;
    int pic = rd(b, sp + 2);
    if (pic >= 0 && pic < 47 && b->ndraw < MAX_MEN + MAX_ARROWS)
        b->draw[b->ndraw++] = (BattleDraw){ SPRITE[pic], b->addr[i], b->side[i] };
    b->stat[i] = rd(b, sp + 4);
    if (b->stat[i] == 2 && T == ARCHER) loose_arrow(b, i);
    *ps = sp + 6;
}

/* ovl7:7173: one pass of sorting by y, so nearer men are drawn over further ones */
static void sort_pass(Battle *b)
{
    for (int a = 0, c = 1; c < MAX_MEN; a++, c++) {
        if (b->addr[a] == -1 || b->addr[c] == -1 || b->y[a] <= b->y[c]) continue;
#define SWAP(arr) { int t_ = b->arr[a]; b->arr[a] = b->arr[c]; b->arr[c] = t_; }
        SWAP(side) SWAP(tgt) SWAP(stat) SWAP(addr) SWAP(x) SWAP(y) SWAP(state) SWAP(script) SWAP(type) SWAP(shot)
#undef SWAP
        for (int k = 0; k < MAX_MEN; k++) {
            if (b->tgt[k] == a) b->tgt[k] = c;
            else if (b->tgt[k] == c) b->tgt[k] = a;
        }
    }
}

static void begin_attack(Battle *b, bool you)   /* the attack starts: ovl7:52BE, 6554 */
{
    BLOG("%s", you ? "YOU attack (A)" : "the enemy attacks");
    snd_beep();                                /* ovl7:52F7, 6564 */
    show_groups(b);
    b->start = 7;
    b->asking = false;
    if (!b->p->demo)                           /* ovl7:52CE: in the demo your men keep attacking */
        for (int t = 0; t < 3; t++) b->order[t] = 0;
    if (b->bg == 3) b->order[ARCHER] = 1;      /* archers on the wall shoot at once */
    bugle(b, you ? 0x792 : 0x778);             /* ovl7:659E, 5302: the charge */
}

/* ovl7:5258: each soldier's next script, then his step */
static void battle_soldiers(Battle *b)
{
    bool moving[3] = { false, false, false };   /* AB8D, AB8F, AB91 */
    int opp = b->p->c->opponent;
    if (b->start) {
        if (b->start < 11) b->start--;
        else if (opp == 0 || ((opp == 2 || opp == 5) && R(b) < 2) || (R(b) == 5 && opp != 3))
            begin_attack(b, false);         /* the enemy attacks first */
    }
    for (int i = 0; i < MAX_MEN; i++) {
        if (b->addr[i] == -1) continue;
        int T = b->type[i];
        int *ps = &b->script[i];
        check_target(b, i);
        int w = rd(b, *ps);
        if (w > 100) {                      /* the script has ended: choose the next */
            b->state[i] = w;
            bool mine = w == END_FRIEND;
            if (b->start) {
                *ps = S(b->start > 6 ? (mine ? S_AAF7 : S_AAF1) : b->start > 3 ? (mine ? S_AAFD : S_AB03)
                                      : (mine ? S_AB39 : S_AB3F), T);
            } else if (b->stat[i] == DEAD) {
                *ps = w == END_ENEMY ? O6(0x1D64) : O6(0x1E52);
            } else if (b->endtimer && (b->endtimer < 4 || !b->retreat)) {
                if ((b->retreat == 1 && b->side[i] == FRIEND) || (b->retreat == 2 && b->side[i] != FRIEND)) continue;
                *ps = O6(0x1D6C);
            } else if (b->order[T] == 3 && b->side[i] == FRIEND) {          /* Backoff */
                moving[T] = true;
                *ps = S((b->x[i] > 71 || (b->y[i] > 73 && b->bg == 3)) ? S_AB39 : S_AB0F, T);
            } else if (b->endtimer && ((b->retreat == 1 && b->side[i] == FRIEND) || (b->retreat == 2 && b->side[i] == ENEMY))) {
                *ps = S(b->retreat == 1 ? S_AB15 : S_AB1B, T);              /* running away */
            } else if (T == ARCHER) {
                if (b->order[ARCHER] == 1 || b->side[i] == ENEMY) {
                    if (!archer_aim(b, i)) {
                        if (b->bg != 4 && b->side[i] == ENEMY) {
                            R(b);
                            if (RND < 5 || (b->retreat == 1 && b->bg != 3)) {
                                *ps = (b->bg == 3 && b->x[i] > 45) ? O7(0x4E) : O7(0x40);
                            } else if ((RND < 10 && b->x[i] > 5) || b->x[i] > 42) {
                                *ps = O7(0x4E);
                            } else if (RND > 120) *ps = O7(0x7E);
                            else if (RND > 113) *ps = O7(0x8C);
                            else *ps = O7(0x38);
                        } else if (b->side[i] == ENEMY) {
                            *ps = O7(0x38);
                        } else {
                            R(b);
                            if (b->x[i] < 38) *ps = O7(0x150);
                            else if (RND < 105 || b->bg == 3) *ps = O7(0x13A);
                            else if (RND > 116) *ps = O7(0x17A);
                            else *ps = O7(0x188);
                        }
                    }
                } else if (b->order[ARCHER] == 2) {                         /* Forward */
                    moving[ARCHER] = true;
                    int l2 = (b->y[i] - 5) >> 3;
                    if ((b->bg == 3 && b->x[i] < l2 + 56) || (b->bg == 4 && b->x[i] < 37 - l2)) *ps = O7(0x13A);
                    else *ps = O7(0x142);
                } else {
                    *ps = mine ? O7(0x13A) : O7(0x38);
                }
            } else if (b->target >= 0) {                                    /* someone within reach */
                if (!close_in(b, i)) {
                    if (b->x[i] > b->x[b->target])
                        *ps = (T == BARBARIAN && R(b) < 30) ? O7(0x75C) : S(S_AB63, T);
                    else
                        *ps = (T == BARBARIAN && R(b) < 30) ? O7(0x76A) : S(S_AB5D, T);
                }
            } else if (b->order[T] == 1 || b->side[i] == ENEMY) {           /* Attack: go for him */
                int d = toward(b, i);
                switch (d) {
                case 0:    *ps = S(mine ? S_AB39 : S_AB3F, T); break;
                case -161: *ps = S(S_AB27, T); break;
                case -160: *ps = S(mine ? S_AB45 : S_AB4B, T); break;
                case -159: *ps = S(S_AB21, T); break;
                case -1:   *ps = S(b->retreat ? S_AB6F : S_AB1B, T); break;
                case 1:    *ps = S(b->retreat ? S_AB69 : S_AB15, T); break;
                case 161:  *ps = S(S_AB2D, T); break;
                case 160:  *ps = S(mine ? S_AB51 : S_AB57, T); break;
                case 159:  *ps = S(S_AB33, T); break;
                default: break;
                }
            } else if (b->order[T] == 2) {                                  /* Forward */
                moving[T] = true;
                int l2 = (b->y[i] - 5) >> 3;
                if ((b->bg == 3 && b->x[i] < l2 + 56) || (b->bg == 4 && b->x[i] < 37 - l2)) *ps = S(S_AB39, T);
                else *ps = S(S_AB1B, T);
            } else {
                *ps = S(w == END_ENEMY ? S_AB3F : S_AB39, T);               /* Stop */
            }
        }
        /* a blow lands? (5B84) */
        if (b->stat[i] == 2 && b->target >= 0 && b->stat[b->target] != DEAD && T != ARCHER) {
            int chance = (b->side[i] == FRIEND ? b->skill_me : b->skill_en) + 40;
            bool go = b->type[b->target] == ARCHER || b->retreat ||
                      (T == KNIGHT && b->type[b->target] == BARBARIAN && R(b) < 35) || R(b) < 90;
            if (go && (R(b) < chance || b->retreat)) {
                BATTLE_TRACE("blow by %d (side %d type %d)", i, b->side[i], T);
                kill(b, b->target);
                b->tgt[i] = -1;
            }
        }
        step(b, i);
    }
    for (int t = 0; t < 3; t++)
        if (moving[t] && --b->order_time[t] < 0) b->order[t] = 0;   /* Forward/Backoff wear off */
    sort_pass(b);
}

/* ovl7:7D0C: arrows fly */
static void arrows_fly(Battle *b)
{
    for (int k = 0; k < MAX_ARROWS; k++) {
        if (b->ax[k] == 0 || b->ax[k] == 9999) continue;
        int dir = b->akind[k];
        b->apow[k] -= 4;
        b->ax[k] += rdb(b, O7(0x220) + dir);
        b->ay[k] += rdb(b, O7(0x235) + dir);
        b->aaddr[k] += rd(b, O7(0x24A) + 2 * dir);   /* a word table */
        int x = b->ax[k], y = b->ay[k];
        if ((b->bg == 3 && x > ((y - 5) >> 3) + 60) || (b->bg == 4 && x < 20 - ((y - 5) >> 3))) { b->ax[k] = 0; continue; }
        if (x > 76 || x < 0 || y < b->ymin[SDL_clamp(x, 0, 79)] || y > 88 || (dir >= 8 && dir <= 14 && y > 82)) { b->ax[k] = 0; continue; }
        if (b->ndraw < MAX_MEN + MAX_ARROWS) b->draw[b->ndraw++] = (BattleDraw){ ARROW[dir], b->aaddr[k], b->aside[k] };
    }
}

/* ovl7:7AFB: arrows hit */
static void arrows_hit(Battle *b)
{
    for (int k = 0; k < MAX_ARROWS; k++) {
        if (b->ax[k] == 0 || b->ax[k] == 9999) continue;
        int dir = b->akind[k];
        int w = rdb(b, O7(0x1F6) + dir), h = rdb(b, O7(0x20B) + dir);
        for (int j = 0; j < MAX_MEN; j++) {
            if (b->stat[j] == DEAD || b->addr[j] == -1) continue;
            if (!(b->ay[k] > b->y[j] - 3 && b->ay[k] < b->y[j] + h && b->ax[k] >= b->x[j] && b->ax[k] < b->x[j] + w)) continue;
            if (R(b) >= b->apow[k]) continue;
            if (!(R(b) < 85 || b->type[j] == ARCHER)) continue;
            if (b->aside[k] == ENEMY && b->side[j] == ENEMY) continue;              /* never their own */
            if (b->aside[k] == FRIEND && b->side[j] == FRIEND && (R(b) < 100 || b->retreat)) continue;
            if (b->type[j] == KNIGHT && R(b) > 88) continue;                        /* armour */
            if (b->type[j] == BARBARIAN && R(b) > 50) continue;                     /* quick */
            b->ax[k] = 0;
            BATTLE_TRACE("arrow %d (side %d dir %d at %d,%d)", k, b->aside[k], dir, b->ax[k], b->ay[k]);
            kill(b, j);
            break;
        }
    }
}

/* ------------------------------------------------------------------ public */

bool battle_start(Battle *b, Play *p, int sel)
{
    SDL_memset(b, 0, sizeof *b);
    b->p = p;
    const GameAssets *ga = p->ga;
    if (!ga->have_battle) return false;
    SDL_memcpy(b->mem, ga->o6, sizeof ga->o6);
    SDL_memcpy(b->mem + BMEM_O7, ga->o7, sizeof ga->o7);
    battle_setup(b, sel);

    /* scripts changed for the ground (battle 3862) */
    int l8 = 0, l6 = 2, l4 = -1;
    if (b->bg == 1) { l8 = 80; l6 = -78; l4 = 79; }
    if (b->bg == 2) { l8 = -80; l6 = 82; l4 = -81; }
    wr(b, S(S_AB6F, 1), l8); wr(b, S(S_AB6F, 0), l8);
    for (int t = 0; t < 3; t++) { wr(b, S(S_AB09, t), l8); wr(b, S(S_AB0F, t), l6); wr(b, S(S_AB15, t), l6); wr(b, S(S_AB1B, t), l4); }
    wr(b, S(S_AB69, 1), l6); wr(b, S(S_AB69, 0), l6);
    if (b->bg == 3) {
        for (int t = 0; t < 3; t++) { wr(b, S(S_AB1B, t), -81); wr(b, S(S_AB0F, t), 81); }
        wr(b, S(S_AB27, 1), -79); wr(b, S(S_AB27, 0), -79);
    } else if (b->bg == 4) {
        for (int t = 0; t < 3; t++) { wr(b, S(S_AB15, t), -79); wr(b, S(S_AB09, t), 81); }
    }

    /* where soldiers may stand (battle 3964) */
    for (int x = 0; x < 80; x++) {
        int lo, hi;
        switch (b->bg) {
        case 1:  lo = 45 - (x >> 1); hi = x < 31 ? 77 : 91 - (x >> 1); break;
        case 2:  lo = (x >> 1) + 5; hi = 77; break;
        case 3:  lo = x < 55 ? 10 : 5; hi = x < 55 ? 77 : 70; break;
        case 4:  lo = x < 25 ? 5 : 10; hi = x < 25 ? 70 : 77; break;
        case 5:  lo = 26; hi = 45; break;
        case 6:  lo = 18; hi = 48; break;
        case 7:  lo = 26; hi = 53; break;
        case 8: case 10: lo = 30; hi = 77; break;
        default: lo = 28; hi = 77; break;
        }
        b->ymin[x] = lo;
        b->ymax[x] = hi;
    }

    /* the men take their places (battle 3C3B) */
    for (int i = 0; i < MAX_MEN; i++) {
        b->tgt[i] = -1;
        b->shot[i] = -1;
        b->stat[i] = 0;
        if (b->addr[i] == -1) continue;
        int t = b->type[i];
        if (b->side[i] == FRIEND) { b->script[i] = S(S_AAF7, t); b->state[i] = END_FRIEND; }
        else                      { b->script[i] = S(S_AAF1, t); b->state[i] = END_ENEMY; }
        if ((b->side[i] == FRIEND && b->bg == 1) || (b->side[i] == ENEMY && b->bg == 2))
            b->y[i] -= (b->y[i] - 10) / 3;                 /* up the hill */
        else if ((b->side[i] == FRIEND && b->bg == 2) || (b->side[i] == ENEMY && b->bg == 1))
            b->y[i] += ((80 - b->y[i]) >> 1) >> 1;
        else if (b->bg == 5 || b->bg == 6)
            b->y[i] -= 10;
        if (b->side[i] == ENEMY) b->x[i] = 76 - b->x[i];   /* the enemy faces you from the left */
        if (b->side[i] == ENEMY && (b->bg == 3 || b->bg == 4) && t != ARCHER) {
            b->x[i] = 2;
            if (b->bg == 4) b->y[i] += 10;
        }
        int x = SDL_clamp(b->x[i], 0, 79);
        if (b->y[i] < b->ymin[x]) {
            b->y[i] = b->ymin[x] + R(b) / 50;
            b->x[i] += R(b) / 30 - 2;
        }
        x = SDL_clamp(b->x[i], 0, 79);
        if (b->y[i] > b->ymax[x]) {
            b->y[i] = b->ymax[x] - R(b) / 50;
            b->x[i] += R(b) / 30 - 2;
        }
        b->x[i] = SDL_clamp(b->x[i], 1, 72);
        b->addr[i] = b->y[i] * 80 + b->x[i];
    }
    b->start = 20;
    for (int k = 0; k < 10; k++) sort_pass(b);
    for (int t = 0; t < 3; t++) b->order[t] = b->p->demo ? 1 : 0;   /* ovl7:3FEA: the demo attacks */
    b->target = -1;
    alive_types(b);
    tclear(b, 23);
    tclear(b, 24);                          /* ovl7:5103: Attack ... Retreat */
    tprint(b, O6S(b, 0x1C41));
    tgoto(b, 32, 24);
    tprint(b, O6S(b, 0x1C49));
    b->asking = true;
    b->ndraw = 0;
    BLOG("on the battle screen: your squad %d [%dK %dB %dA] in formation %d against the enemy's squad %d [%dK %dB %dA] in formation %d",
         b->me, SQ(b, b->me)->knights, SQ(b, b->me)->barbarians, SQ(b, b->me)->archers, SQ(b, b->me)->formation,
         b->en, SQ(b, b->en)->knights, SQ(b, b->en)->barbarians, SQ(b, b->en)->archers, SQ(b, b->en)->formation);
    BLOG("ground: %s (background %d, P%X); skill yours %d, theirs %d (45 + 2 x battle edge; blows land with skill + 40 of 128)",
         GROUND[b->bg], b->bg, picture_of(b->bg), b->skill_me, b->skill_en);
    battle_soldiers(b);
    b->ok = true;
    return true;
}

void battle_key(Battle *b, int key) { b->key = key; }

static void battle_input(Battle *b)   /* ovl7:652F */
{
    int k = b->key;
    if (!k) return;
    if (b->p->demo) return;            /* ovl7:6541: a key ends the demo (main.c) */
    if (b->asking) {
        if (k == PK_A) begin_attack(b, true);
    } else if (b->menu == 1) {
        bool chosen = false;
        if (k == PK_B && b->alive_type[BARBARIAN]) {
            b->group = 1; tclear(b, 23); tprint(b, " "); tprint(b, O6S(b, 0x1C59)); b->group_col = 12; chosen = true;
        }
        if (k == PK_A && b->alive_type[ARCHER] && b->bg != 3) {
            b->group = 2; tclear(b, 23); tprint(b, " "); tprint(b, O6S(b, 0x1C51)); b->group_col = 9; chosen = true;
        }
        if (k == PK_K && b->alive_type[KNIGHT]) {
            b->group = 0; tclear(b, 23); tprint(b, " "); tprint(b, O6S(b, 0x1C64)); b->group_col = 10; chosen = true;
        }
        if (k == PK_S && b->bg != 3) {
            b->group = 5; tclear(b, 23); tprint(b, O6S(b, 0x1C6B)); b->group_col = 7; chosen = true;
        }
        if (chosen) { snd_beep(); show_orders(b); }   /* ovl7:66CB */
    } else if (b->menu == 2) {
        tgoto(b, b->group_col, 23);
        int order = -1, word = 0;
        if (k == PK_BACKSPACE) { snd_beep(); snd_alert(); tclear(b, 23); show_groups(b); return; }   /* ovl7:66F5, 5153 */
        if (k == PK_A && b->bg != 3) { order = 1; word = 0x1C41; }
        if (k == PK_F) { order = 2; word = 0x1C72; }
        if (k == PK_B) { order = 3; word = 0x1C7B; }
        if (k == PK_S) { order = 0; word = 0x1C84; }
        if (order >= 0) {
            static const char *const ORDER[4] = { "Stop", "Attack", "Forward", "Backoff" };
            BLOG("YOU order %s: %s", b->group == 5 ? "the squad" : b->group == 0 ? "the knights" :
                 b->group == 1 ? "the barbarians" : "the archers", ORDER[order]);
            for (int t = 0; t < 3; t++) {
                if (b->group != 5 && b->group != t) continue;
                b->order[t] = order;
                if (order == 2 || order == 3) b->order_time[t] = 15;
            }
            tprint(b, O6S(b, word));
            snd_alert();                       /* ovl7:6756, 67BF, 682F, 687C */
            show_groups(b);
        }
    }
}

void battle_frame(Battle *b)   /* the battle loop (battle 406F) */
{
    if (b->done) return;
    b->frame++;
    if (b->hold > 0) { b->hold--; return; }   /* a bugle call is sounding (a key waits) */
    b->ndraw = 0;
    battle_soldiers(b);
    alive_types(b);
    arrows_fly(b);
    arrows_hit(b);
    PSquad *me = SQ(b, b->me), *en = SQ(b, b->en);
    if (b->endtimer == 0) {
        battle_input(b);
        if (b->key == PK_R && !b->retreat) {   /* Retreat */
            BLOG("YOU Retreat (R)");
            snd_beep();                         /* ovl7:40ED */
            b->retreat = 1;
            retreat(b);
        }
        if (R(b) < 3 && b->start == 0 && !b->retreat) {   /* a fort's attackers give up */
            if (en->archers < RND && b->bg == 3 && (en->archers == 0 || R(b) < 20)) {
                BLOG("the enemy gives up attacking your fort");
                b->retreat = 2; retreat(b);
            } else if (me->archers < RND && b->bg == 4 && (en->archers == 0 || R(b) < 20)) {
                BLOG("your men give up attacking the fort");
                b->retreat = 1; retreat(b);
            }
        }
    } else {
        if (--b->endtimer <= 0) b->done = true;
        if (b->endtimer == 1) {
            for (int k = 0; k < MAX_ARROWS; k++) b->ax[k] = 0;
            show_losses(b);
        }
        int left = 1;
        if (b->retreat == 1 || b->retreat == 2) {   /* the retreating men still on the field */
            left = 0;
            for (int i = 0; i < MAX_MEN; i++)
                if (b->addr[i] != -1 && b->stat[i] != DEAD && (b->retreat == 1 ? b->side[i] == FRIEND : b->side[i] == ENEMY)) left++;
        }
        if (!left && b->endtimer > 8) {
            if (b->bg == 3 || b->bg == 4) { if (b->endtimer > 15) b->endtimer = 16; }
            else b->endtimer = 8;
        }
    }
    b->key = 0;
}

void battle_finish(Battle *b)   /* battle 42E8 */
{
    PSquad *me = SQ(b, b->me), *en = SQ(b, b->en);
    BLOG("over: you lost %d men, they lost %d%s; both squads' condition is halved", b->lost_me, b->lost_en,
         b->retreat == 1 ? " (you retreated)" : b->retreat == 2 ? " (they retreated)" : "");
    en->condition = SDL_max(en->condition >> 1, 1);
    me->condition = SDL_max(me->condition >> 1, 1);
    play_after_battle(b->p, b->me, b->en);
}

/* ------------------------------------------------------------------ drawing */

/* rt:1B17 on a 2-bit sprite: a white figure with a black outline (yours) or
 * the reverse, the outline taken within each 8-pixel word */
static void draw_sprite(Screen *s, const Battle *b, int off, int addr, int side, const uint8_t col[4], int clip)
{
    int ww = rd(b, off), hh = rd(b, off + 2);
    if (ww <= 0 || ww > 10 || hh <= 0 || hh > 40) return;
    int x0 = (addr % 80) * 4, y0 = (addr / 80) * 2;
    if (addr < 0) return;
    for (int y = 0; y < hh * 2; y++) {
        int sy = y0 + y;
        if (sy < 0 || sy >= clip) continue;
        const uint8_t *row = b->mem + off + 4 + y * ww * 2;
        for (int x = 0; x < ww * 8; x++) {
            int sx = x0 + x;
            if (sx < 0 || sx >= SCREEN_W) continue;
            int v = (row[x / 4] >> (6 - 2 * (x & 3))) & 3;
            int in = x & 7;
            int l = in > 0 ? (row[(x - 1) / 4] >> (6 - 2 * ((x - 1) & 3))) & 3 : 0;
            int r = in < 7 ? (row[(x + 1) / 4] >> (6 - 2 * ((x + 1) & 3))) & 3 : 0;
            uint8_t *d = &s->px[sy * SCREEN_W + sx];
            if (side == FRIEND) { if (v) *d = col[v]; else if (l | r) *d = col[0]; }
            else                { if (v) *d = col[0]; else if (l | r) *d = col[3]; }
        }
    }
}

void battle_draw(const Battle *b, Screen *s, GfxMode mode)
{
    const AssetSet *set = &b->p->ga->set[mode];
    const Image *bg = &set->pictures[PIC_BATTLE_FIRST + picture_of(b->bg)];
    screen_set_palette(s, bg->pal, bg->ncolors);
    screen_blit(s, bg, 0, 0);
    uint8_t col[4];
    for (int i = 0; i < 4; i++) col[i] = screen_nearest(s, bg->ncolors, CGA_PALETTE[i].r, CGA_PALETTE[i].g, CGA_PALETTE[i].b);
    const int map_h = 182;
    for (int k = 0; k < b->ndraw; k++) draw_sprite(s, b, b->draw[k].sprite, b->draw[k].addr, b->draw[k].side, col, map_h);
    screen_fill(s, 0, map_h, SCREEN_W, 1, col[3]);
    screen_fill(s, 0, map_h + 1, SCREEN_W, SCREEN_H - map_h - 1, col[0]);
    for (int r = 0; r < 2; r++) screen_text(s, b->p->ga, 0, (23 + r) * 8, b->text[r], col[3], col[0]);
}
