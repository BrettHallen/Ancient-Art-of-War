/*
 * The Ancient Art of War (1984) - remake
 * Nov 2026 using Claude Opus 5.5
 * "Guided" by Brett Hallen
 * Original game by Dave Murry & Barry Murry
 * Published by Brøderbund Software Inc.
 *
 * gamelog.c: the debug log (see gamelog.h). Everything here only reads the
 * game; nothing calls random().
 */
#include "gamelog.h"

#include <SDL3/SDL.h>

static struct {
    GameLogLevel level;
    char dir[1024];
    char path[1280];
    SDL_IOStream *f;
    const Play *play;           /* the game being logged */
    long iterations, ticks;     /* since the log was opened */
} G;

/* ------------------------------------------------------------------ words */

static char *buf(void)
{
    static char b[12][192];
    static int n;
    n = (n + 1) % 12;
    return b[n];
}

static bool t_fort(int t)     { return t == 3; }
static bool t_village(int t)  { return t == 4; }
static bool t_hill(int t)     { return t == 5 || t == 6; }
static bool t_bridge(int t)   { return t == 9 || t == 10; }
static bool t_water(int t)    { return t == 7 || t == 8 || (t >= 13 && t <= 15) || (t >= 19 && t <= 36); }
static bool t_mountain(int t) { return (t >= 37 && t <= 54) || t == 65 || t == 66 || t == 71 || t == 72; }
static bool t_forest(int t)   { return (t >= 55 && t <= 57) || (t >= 61 && t <= 63) || (t >= 67 && t <= 69); }

static int tile_of(const Play *p, int cell)
{
    if (!p->c || cell < 0 || cell >= MAP_W * MAP_H) return 0;
    return p->c->map[cell / MAP_W][cell % MAP_W];
}

static const char *terrain_name(int t)
{
    return t_fort(t) ? "fort" : t_village(t) ? "village" : t_hill(t) ? "hill" : t_bridge(t) ? "bridge" :
           t_water(t) ? "water" : t_mountain(t) ? "mountain" : t_forest(t) ? "forest" : "open";
}

static char terrain_char(int t)
{
    return t_fort(t) ? 'F' : t_village(t) ? 'V' : t_hill(t) ? 'h' : t_bridge(t) ? '=' :
           t_water(t) ? '~' : t_mountain(t) ? '^' : t_forest(t) ? 'T' : '.';
}

static int cell_of(const PSquad *q) { return ((q->y + 5) / 10) * 10 + ((q->x + 1) >> 3); }   /* rt:1330 */

const char *gamelog_cell(const Play *p, int cell)
{
    char *b = buf();
    if (cell < 0 || cell >= MAP_W * MAP_H) SDL_snprintf(b, 192, "cell %d (off the map)", cell);
    else SDL_snprintf(b, 192, "cell %d (row %d col %d, %s)", cell, cell / 10, cell % 10, terrain_name(tile_of(p, cell)));
    return b;
}

static const char *men(const PSquad *q)
{
    char *b = buf();
    SDL_snprintf(b, 192, "%dK %dB %dA %dS", q->knights, q->barbarians, q->archers, q->spies);
    return b;
}

const char *gamelog_squad(const Play *p, int i)
{
    char *b = buf();
    if (i < 0 || i >= MAX_SQUADS) { SDL_snprintf(b, 192, "squad %d (none)", i); return b; }
    const PSquad *q = &p->sq[i];
    int c = cell_of(q);
    SDL_snprintf(b, 192, "%s squad %d [%s] at %d,%d cell %d %s", q->friendly ? "your" : "enemy", i, men(q),
                 q->x, q->y, c, terrain_name(tile_of(p, c)));
    return b;
}

const char *gamelog_key(int key)
{
    static const struct { int k; const char *s; } K[] = {
        { PK_ESC, "Esc" }, { PK_BACKSPACE, "Backspace" }, { PK_ENTER, "Enter" }, { PK_SPACE, "Space" },
        { PK_MINUS, "-" }, { PK_EQUALS, "=" }, { PK_KP_MINUS, "keypad -" }, { PK_KP_PLUS, "keypad +" },
        { PK_ALT_S, "Alt+S" }, { PK_HOME, "Home" }, { PK_UP, "Up" }, { PK_PGUP, "PgUp" }, { PK_LEFT, "Left" },
        { PK_KP_5, "keypad 5" }, { PK_RIGHT, "Right" }, { PK_END, "End" }, { PK_DOWN, "Down" }, { PK_PGDN, "PgDn" },
        { PK_TAB, "Tab" }, { PK_CTRL, "Ctrl" },
    };
    for (size_t k = 0; k < SDL_arraysize(K); k++) if (K[k].k == key) return K[k].s;
    static const char ROW1[] = "QWERTYUIOP", ROW2[] = "ASDFGHJKL", ROW3[] = "ZXCVBNM";
    char *b = buf();
    if (key >= 0x10 && key <= 0x19) SDL_snprintf(b, 192, "%c", ROW1[key - 0x10]);
    else if (key >= 0x1E && key <= 0x26) SDL_snprintf(b, 192, "%c", ROW2[key - 0x1E]);
    else if (key >= 0x2C && key <= 0x32) SDL_snprintf(b, 192, "%c", ROW3[key - 0x2C]);
    else SDL_snprintf(b, 192, "key %02Xh", key);
    return b;
}

/* ------------------------------------------------------------------ the file */

void gamelog_setup(GameLogLevel level, const char *dir)
{
    G.level = level;
    SDL_strlcpy(G.dir, dir ? dir : "", sizeof G.dir);
}

GameLogLevel gamelog_level(void) { return G.level; }
const char *gamelog_path(void) { return G.f ? G.path : ""; }

bool gamelog_on(const Play *p, GameLogLevel level)
{
    return G.f && p && p == G.play && !p->demo && G.level >= level;
}

static void out(const char *fmt, ...) SDL_PRINTF_VARARG_FUNC(1);
static void out(const char *fmt, ...)
{
    if (!G.f) return;
    va_list ap;
    va_start(ap, fmt);
    SDL_IOvprintf(G.f, fmt, ap);
    va_end(ap);
}

void gamelog_printf(const Play *p, const char *fmt, ...)
{
    if (!G.f || p != G.play) return;
    long s = G.iterations / PLAY_HZ;
    out("[%3ld:%02ld tick %5ld] ", s / 60, s % 60, G.ticks);
    va_list ap;
    va_start(ap, fmt);
    SDL_IOvprintf(G.f, fmt, ap);
    va_end(ap);
    out("\n");
    SDL_FlushIO(G.f);
}

void gamelog_iteration(const Play *p) { if (G.f && p == G.play) G.iterations++; }
void gamelog_tick(const Play *p)      { if (G.f && p == G.play) G.ticks++; }

static void close_file(void)
{
    if (G.f) SDL_CloseIO(G.f);
    G.f = NULL;
    G.play = NULL;
}

/* "The Race for the Flags" -> a name a file system takes */
static void file_safe(char *d, size_t size, const char *s)
{
    size_t n = 0;
    for (; *s && n + 1 < size; s++) {
        unsigned char ch = (unsigned char)*s;
        if (ch < 32 || SDL_strchr("/\\:*?\"<>|", ch)) ch = '_';
        if (ch == ' ' && (n == 0 || d[n - 1] == ' ')) continue;
        d[n++] = (char)ch;
    }
    while (n > 0 && (d[n - 1] == ' ' || d[n - 1] == '.')) n--;
    d[n] = 0;
    if (!n) SDL_strlcpy(d, "campaign", size);
}

/* the rules as the Rulebook words them (ovl1_5030, the O3 texts) */
static void rules_text(const uint16_t *r)
{
    char t[RULE_COUNT][64];
    int v;
    SDL_snprintf(t[0], 64, "Villages %ssupply food", r[0] ? "" : "don't ");
    SDL_snprintf(t[1], 64, "Forts %ssupply food", r[1] ? "" : "don't ");
    v = r[RULE_SUPPLY_LINE_RANGE];
    SDL_snprintf(t[2], 64, "Supply line range is %s", v == 1 ? "short" : v == 2 ? "medium" : "long");
    v = r[RULE_FORTS_TRAIN_MEN];
    SDL_snprintf(t[3], 64, "Forts train men %s", v == 30 ? "rarely" : v == 60 ? "often" : "very often");
    v = r[RULE_MEN_SHAPE];
    SDL_snprintf(t[4], 64, "Your men are %s shape", v == 4 ? "out of" : v == 2 ? "in good" : "in prime");
    v = r[RULE_ENEMY_SEEN];
    SDL_snprintf(t[5], 64, "Enemy is seen %s", v == 20 ? "when close" : v == 40 ? "when far" : "always");
    v = r[RULE_WATER];
    SDL_snprintf(t[6], 64, "Water is %s", v == 10 ? "shallow and calm" : v == 20 ? "deep but calm" : "deep and dangerous");
    v = r[RULE_MOUNTAIN];
    SDL_snprintf(t[7], 64, "Mountain is %s", v == 4 ? "low and safe" : v == 8 ? "high but safe" : "high and risky");
    v = r[RULE_FOREST];
    SDL_snprintf(t[8], 64, "Forest is %s", v == 3 ? "sparse" : "dense");
    out("Rules (as the Rulebook words them; the value in the T file):\n");
    for (int k = 0; k < RULE_COUNT; k++) out("  %-36s %5d\n", t[k], (int16_t)r[k]);
    out("\n");
}

/* WAR_Analysis/README.md, Opponents' values */
static const char *const OPP_USE[OPP_PARAMS] = {
    "squads at the target before it chooses a new one",
    "(not used)",
    "(not used)",
    "chance of going straight for your flag; 150+: never holds ground",
    "how much your hold on a flag counts against attacking it",
    "chance that a squad holding ground may move on",
    "squads kept guarding its own flag",
    "surrender ratio (your men / its men; 500: never)",
    "food under which a squad goes to eat",
    "condition a squad holding ground needs before moving on",
    "chance of a random change in each choice",
    "distance (cells) at which a marching squad attacks",
    "least strength to attack a squad",
    "chance of going for one of your flags within 30 steps",
    "(not used)",
    "mountain factor for its squads",
    "forest factor for its squads",
    "speed going for a flag or a squad (1 fast, 2 march, 3 slow)",
    "how strongly you must hold a cell before crossing it costs more",
};

static void opponent_values(const int16_t *v)
{
    for (int k = 0; k < OPP_PARAMS; k++) out("  %04X %6d  %s\n", 0xA4DC + 2 * k, v[k], OPP_USE[k]);
    out("\n");
}

static void map_text(const Campaign *c)
{
    out("Map (cell = row x 10 + column; row 0 is the top, the north):\n      0123456789\n");
    for (int r = 0; r < MAP_H; r++) {
        out("  %2d  ", r);
        for (int k = 0; k < MAP_W; k++) {
            out("%c", terrain_char(c->map[r][k]));
        }
        out("\n");
    }
    out("  F fort, V village, h hill, = bridge, ~ water, ^ mountain, T forest, . open ground\n\n");
}

static const char *speed_name(int s)
{
    return s == 1 ? "fast" : s == 2 ? "march" : s == 3 ? "slow" : "stopped";
}

static void squads_table(const Play *p)
{
    out("  slot side   men (K B A S)   x,y      cell             speed  moving   form cond food  plan\n");
    for (int i = 0; i < MAX_SQUADS; i++) {
        const PSquad *q = &p->sq[i];
        if (!q->used) continue;
        int c = cell_of(q);
        char plan[96] = "";
        if (q->fighting) SDL_snprintf(plan, sizeof plan, "fighting squad %d (%s)", q->foe,
                                      q->clock == FIGHT_ON ? "on the map" : "encounter");
        else if (!q->friendly && q->planned) SDL_snprintf(plan, sizeof plan, "to cell %d%s", q->dest, q->on_route ? ", on a route" : "");
        else if (!q->friendly && q->on_route) SDL_snprintf(plan, sizeof plan, "going for a flag or a squad");
        else if (q->friendly && q->marked) SDL_snprintf(plan, sizeof plan, "to %d,%d", q->mark_x, q->mark_y);
        out("  %4d %-6s %-15s %3d,%-3d  %3d %-11s  %-6s %-7s  %4d %4d %4d  %s\n", i, q->friendly ? "yours" : "enemy",
            men(q), q->x, q->y, c, terrain_name(tile_of(p, c)), speed_name(q->speed), speed_name(q->moving),
            q->formation, q->condition, q->food, plan);
    }
}

static void flags_table(const Play *p)
{
    for (int f = 0; f < MAX_FLAGS; f++) {
        const PFlag *fl = &p->flag[f];
        if (!fl->present) continue;
        int c = ((fl->y + 5) / 10) * 10 + ((fl->x + 1) >> 3);   /* rt:1383 */
        out("  flag %d  %-5s  at %d,%d  %s\n", f, fl->white ? "yours" : "enemy", fl->x, fl->y, gamelog_cell(p, c));
    }
}

static void grid(const char *title, const int16_t *v)
{
    out("%s\n       ", title);
    for (int k = 0; k < MAP_W; k++) out("%6d", k);
    out("\n");
    for (int r = 0; r < MAP_H; r++) {
        out("  %3d  ", r);
        for (int k = 0; k < MAP_W; k++) out("%6d", v[r * MAP_W + k]);
        out("\n");
    }
    out("\n");
}

static const char *time_name(int div)
{
    return div == 64 ? "slow" : div == 16 ? "medium" : div == 4 ? "fast" : div == 1 ? "faster" : "?";
}

void gamelog_begin(const Play *p, const Campaign *c, const char *how, uint32_t seed, int fight_delay)
{
    close_file();
    if (G.level == GLOG_OFF || !p || !c) return;
    SDL_CreateDirectory(G.dir);
    SDL_Time now = 0;
    SDL_DateTime dt;
    SDL_zero(dt);
    SDL_GetCurrentTime(&now);
    SDL_TimeToDateTime(now, &dt, true);
    char name[64];
    file_safe(name, sizeof name, c->title);
    for (int n = 1; n < 100 && !G.f; n++) {
        char extra[8] = "";
        if (n > 1) SDL_snprintf(extra, sizeof extra, " (%d)", n);
        SDL_snprintf(G.path, sizeof G.path, "%s%04d-%02d-%02d %02d-%02d-%02d %s%s.log", G.dir, dt.year, dt.month, dt.day,
                     dt.hour, dt.minute, dt.second, name, extra);
        SDL_PathInfo info;
        if (SDL_GetPathInfo(G.path, &info)) continue;   /* two games in one second */
        G.f = SDL_IOFromFile(G.path, "w");
    }
    if (!G.f) { SDL_Log("Could not open a log in %s: %s", G.dir, SDL_GetError()); return; }
    G.play = p;
    G.iterations = G.ticks = 0;

    out("The Ancient Art of War - remake: game log (%s)\n", G.level >= GLOG_VERBOSE ? "verbose" : "normal");
    out("===========================================================\n");
    out("Opened       %04d-%02d-%02d %02d:%02d:%02d (local time)\n", dt.year, dt.month, dt.day, dt.hour, dt.minute, dt.second);
    out("Game         %s\n", how);
    out("Campaign     %s (%s)\n", c->title, c->file[0] ? c->file : "no file");
    int opp = c->opponent & 7;
    out("Opponent     %s (opponent %d)\n", OPPONENT_NAMES[opp], c->opponent);
    out("Random seed  %u: random() starts at %02Xh (WAR.EXE takes the BIOS clock's low byte)\n", (unsigned)seed, (unsigned)(seed & 0xFF));
    out("Fights       an encounter turns into a fight after %d iterations (Options Shoppe: %s)\n", fight_delay,
        fight_delay == 200 ? "short" : fight_delay == 400 ? "medium" : fight_delay == 800 ? "long" : "?");
    out("Squads       %d yours, %d the enemy's\n", c->num_friendly, c->num_enemy);
    out("\n");
    rules_text(c->rules);
    if (p->ga && p->ga->have_o2) {
        out("%s's values (O2 + 277h, copied to DS:A4DC; chances are out of 128):\n", OPPONENT_NAMES[opp]);
        opponent_values(p->ga->opponent[opp]);
    }
    map_text(c);
    out("How to read the log\n");
    out("  [mm:ss tick N]  time on the map since the log opened (20 iterations a second; the battle\n");
    out("                  screen and menus don't count) and game ticks (the Time setting: a tick each\n");
    out("                  64, 16, 4 or 1 iterations). Squads are named by slot (0-19), with their men as\n");
    out("                  knights, barbarians, archers and spies (2K 1B 3A 0S), x,y on the map (x in\n");
    out("                  4-pixel columns 0-79, y in line pairs 0-199), and the cell they are in.\n");
    out("  ENEMY           what the enemy commander decides, and why (the values above).\n");
    out("  YOU             your orders.\n");
    out("  EVENT           sightings, encounters, fights, captures, flags, food, training.\n");
    out("  BATTLE          the battle screen (Zoom), with its frame (about 8 a second).\n");
    if (G.level >= GLOG_VERBOSE)
        out("  verbose         also each squad the enemy looks at and squads holding ground, each\n"
            "                  choice's candidates, the cell tables (worth, route cost, supply), and a\n"
            "                  table of all squads every 16 ticks. A route's cells cost their crossing\n"
            "                  cost, plus half of (A500 - supply) + 3 where you hold them more strongly.\n");
    out("\n");
    SDL_FlushIO(G.f);
}

void gamelog_state(const Play *p, const char *title)
{
    if (!gamelog_on(p, GLOG_NORMAL)) return;
    long s = G.iterations / PLAY_HZ;
    out("---- %s [%ld:%02ld tick %ld] ----\n", title, s / 60, s % 60, G.ticks);
    out("Time setting: %s. Random state %04Xh at %04Xh.\n", time_name(p->time_div), p->rnd, p->rnd_ptr);
    if (p->ga && p->ga->have_o2 && p->c) {
        const int16_t *was = p->ga->opponent[p->c->opponent & 7];
        int changed = 0;
        for (int k = 0; k < OPP_PARAMS; k++) changed += p->opp[k] != was[k];
        if (!changed) out("Opponent's values: as in O2.\n");
        else {
            out("Opponent's values changed in play:\n");
            for (int k = 0; k < OPP_PARAMS; k++)
                if (p->opp[k] != was[k]) out("  %04X %6d  %s (was %d)\n", 0xA4DC + 2 * k, p->opp[k], OPP_USE[k], was[k]);
        }
    }
    out("Flags:\n");
    flags_table(p);
    out("Squads:\n");
    squads_table(p);
    out("Enemy's plans: guards flag %d at %s; target %s; going for your flag %d.\n", p->home_flag,
        gamelog_cell(p, p->home_cell), p->target_cell < 0 ? "none" : gamelog_cell(p, p->target_cell), p->attack_flag);
    out("Worth: fort %d, village %d, bridge %d, your flag +%d. Men lost: yours %d, theirs %d; captured: yours %d, theirs %d.\n",
        p->value_fort, p->value_village, p->value_bridge, p->value_flag, p->lost_mine, p->lost_theirs,
        p->captured_mine, p->captured_theirs);
    out("\n");
    if (G.level >= GLOG_VERBOSE) {
        grid("Cell values (what the enemy thinks each cell is worth):", p->cell_value);
        grid("Cell costs (for the enemy's route searches):", p->cell_cost);
        grid("Supply map (each side's strength over each cell; minus = yours):", p->supply);
    }
    SDL_FlushIO(G.f);
}

void gamelog_squads(const Play *p, const char *title)
{
    if (!gamelog_on(p, GLOG_NORMAL)) return;
    long s = G.iterations / PLAY_HZ;
    out("---- %s [%ld:%02ld tick %ld] ----\n", title, s / 60, s % 60, G.ticks);
    squads_table(p);
    out("\n");
    SDL_FlushIO(G.f);
}

void gamelog_grid(const Play *p, const char *title, const int16_t *v)
{
    if (!gamelog_on(p, GLOG_VERBOSE)) return;
    grid(title, v);
    SDL_FlushIO(G.f);
}

void gamelog_end(const Play *p, const char *how)
{
    if (!G.f || (p && p != G.play)) return;
    if (p) {
        gamelog_printf(p, "%s", how);
        gamelog_state(p, "At the end");
    } else {
        out("%s\n", how);
    }
    long s = G.iterations / PLAY_HZ;
    out("Log closed after %ld:%02ld on the map, %ld ticks.\n", s / 60, s % 60, G.ticks);
    close_file();
}
