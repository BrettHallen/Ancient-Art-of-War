/*
 * The Ancient Art of War at Sea (1987) - remake
 * Nov 2026 using Claude Opus 5.5
 * "Guided" by Brett Hallen
 * Original game by Dave Murry & Barry Murry
 * Published by Brøderbund Software Inc.
 *
 * gamelog.c: the debug log (see gamelog.h). Everything here only reads the
 * game: nothing calls random(), list_ships() or the routines that count
 * work for the XT's timing (play_cell, the tile tests ...), which have
 * copies of their own here.
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
    static char b[16][256];
    static int n;
    n = (n + 1) % 16;
    return b[n];
}

/* the tile classes (rt:14A6, 15C4, 1656, 12BD), without counting their work */
static bool t_land(int t)    { return (t >= 3 && t <= 6) || t == 40 || t == 41 || t == 46 || t == 47 || (t >= 51 && t <= 77); }
static bool t_rough(int t)   { return (t >= 10 && t <= 12) || (t >= 16 && t <= 18); }
static bool t_shallow(int t) { return (t >= 7 && t <= 9) || (t >= 13 && t <= 15); }
static bool t_sea(int t)
{
    return t <= 2 || t == 7 || t == 12 || t == 13 || t == 19 || (t >= 37 && t <= 39) ||
           (t >= 42 && t <= 45) || (t >= 48 && t <= 50) || (t >= 78 && t <= 81) || t >= 83;
}

static const char *terrain_name(int t)
{
    return t == 74 ? "repair port" : t == 75 ? "supply port" : t == 76 ? "the enemy's crown" : t == 77 ? "your crown" :
           t_rough(t) ? "rough water" : t_shallow(t) ? "shallow water" : t_land(t) ? "land" : t_sea(t) ? "sea" : "shore";
}

static char terrain_char(int t)
{
    return t == 74 ? 'R' : t == 75 ? 'S' : t == 76 ? 'C' : t == 77 ? 'Y' :
           t_rough(t) ? '~' : t_shallow(t) ? ',' : t_land(t) ? '#' : t_sea(t) ? '.' : '/';
}

static int tile_at(const Play *p, int cell) { return cell >= 0 && cell < NUM_CELLS ? p->map[cell] : 0; }

int gamelog_cell_of(const Play *p, int i)
{
    if (i < 0 || i >= MAX_SHIPS) return -1;
    return ((p->sq[i].y + 2) / 10) * 10 + (p->sq[i].x >> 3);
}

const char *gamelog_cell(const Play *p, int cell)
{
    char *b = buf();
    if (cell < 0 || cell >= NUM_CELLS) SDL_snprintf(b, 256, "cell %d (off the map)", cell);
    else SDL_snprintf(b, 256, "cell %d (row %d col %d, %s)", cell, cell / 10, cell % 10, terrain_name(tile_at(p, cell)));
    return b;
}

static const char *type_name(int t) { return t == 1 ? "line" : t == 2 ? "frigate" : "flag"; }

/* the squadron's ships, in list_ships' order (rt:0630) */
static int ships_of(const Play *p, int i, int out[3])
{
    int n = 0;
    for (int s = 0; s < MAX_SHIPS && n < 3; s++) if (p->ship[s].sq == i) out[n++] = s;
    return n;
}

static const char *ship_name(const Play *p, int s)
{
    return p->c && s >= 0 && s < MAX_SHIPS ? p->c->ships[s].name : "?";
}

const char *gamelog_ship(const Play *p, int s)
{
    char *b = buf();
    if (s < 0 || s >= MAX_SHIPS) { SDL_snprintf(b, 256, "ship %d", s); return b; }
    SDL_snprintf(b, 256, "%s (%s)", ship_name(p, s), type_name(p->ship[s].type));
    return b;
}

const char *gamelog_ships(const Play *p, int i)
{
    char *b = buf();
    b[0] = 0;
    if (i < 0 || i >= MAX_SHIPS) return b;
    int l[3], n = ships_of(p, i, l);
    size_t len = 0;
    for (int k = 0; k < n; k++) {
        const PShip *sh = &p->ship[l[k]];
        char d[16] = "";
        if (sh->sails || sh->hull) SDL_snprintf(d, sizeof d, " s%dh%d", sh->sails, sh->hull);
        len += (size_t)SDL_snprintf(b + len, 256 - len, "%s%s %s%s", k ? ", " : "", ship_name(p, l[k]), type_name(sh->type), d);
        if (len >= 255) break;
    }
    if (n == 0) SDL_strlcpy(b, "no ships", 256);
    return b;
}

const char *gamelog_squad(const Play *p, int i)
{
    char *b = buf();
    if (i < 0 || i >= MAX_SHIPS) { SDL_snprintf(b, 256, "squadron %d (none)", i); return b; }
    const PSquad *q = &p->sq[i];
    int c = gamelog_cell_of(p, i);
    const char *side = q->side == -1 ? "your" : "the enemy's";
    if (q->mark == 2) SDL_snprintf(b, 256, "%s merchant %d at %d,%d cell %d %s", side, i, q->x, q->y, c, terrain_name(tile_at(p, c)));
    else SDL_snprintf(b, 256, "%s squadron %d (%s) at %d,%d cell %d %s", side, i, gamelog_ships(p, i), q->x, q->y, c,
                      terrain_name(tile_at(p, c)));
    return b;
}

int gamelog_strength(const Play *p, int i)
{
    if (i < 0 || i >= MAX_SHIPS) return 0;
    if (p->sq[i].mark == 2) return 1;
    int l[3], n = ships_of(p, i, l), s = 0;
    for (int k = 0; k < n; k++) {
        const PShip *sh = &p->ship[l[k]];
        s += 6;
        if (sh->type != 2) s += 6;
        if (sh->type == 3) s += 6;
        s -= ((int8_t)sh->sails + (int8_t)sh->hull) / 2;
    }
    return s;
}

int gamelog_worst(const Play *p, int i)
{
    if (i < 0 || i >= MAX_SHIPS) return 0;
    int l[3], n = ships_of(p, i, l), w = 0;
    for (int k = 0; k < n; k++) {
        if (p->ship[l[k]].sails > w) w = p->ship[l[k]].sails;
        if (p->ship[l[k]].hull > w) w = p->ship[l[k]].hull;
    }
    return w;
}

const char *gamelog_dirs(const int8_t *steps, int n)
{
    static const char *const D[9] = { "?", "E", "SE", "S", "SW", "W", "NW", "N", "NE" };
    char *b = buf();
    size_t len = 0;
    b[0] = 0;
    for (int k = 0; k < n && len < 240; k++) {
        int d = steps[k];
        len += (size_t)SDL_snprintf(b + len, 256 - len, "%s%s", k ? " " : "", d >= 1 && d <= 8 ? D[d] : "?");
    }
    return b;
}

const char *gamelog_key(int key)
{
    static const struct { int k; const char *s; } K[] = {
        { PK_ESC, "Esc" }, { PK_ENTER, "Enter" }, { PK_SPACE, "Space" }, { PK_MINUS, "-" }, { PK_EQUALS, "=" },
        { PK_KP_MINUS, "keypad -" }, { PK_KP_PLUS, "keypad +" }, { PK_ALT_S, "Alt+S" }, { PK_HOME, "Home" },
        { PK_UP, "Up" }, { PK_PGUP, "PgUp" }, { PK_LEFT, "Left" }, { PK_KP_5, "keypad 5" }, { PK_RIGHT, "Right" },
        { PK_END, "End" }, { PK_DOWN, "Down" }, { PK_PGDN, "PgDn" }, { PK_TAB, "Tab" }, { PK_CTRL, "Ctrl" },
    };
    for (size_t k = 0; k < SDL_arraysize(K); k++) if (K[k].k == key) return K[k].s;
    static const char ROW1[] = "QWERTYUIOP", ROW2[] = "ASDFGHJKL", ROW3[] = "ZXCVBNM";
    char *b = buf();
    if (key >= 0x10 && key <= 0x19) SDL_snprintf(b, 256, "%c", ROW1[key - 0x10]);
    else if (key >= 0x1E && key <= 0x26) SDL_snprintf(b, 256, "%c", ROW2[key - 0x1E]);
    else if (key >= 0x2C && key <= 0x32) SDL_snprintf(b, 256, "%c", ROW3[key - 0x2C]);
    else SDL_snprintf(b, 256, "key %02Xh", key);
    return b;
}

/* DS:649F, as the battle report reads it (ovl1:9704) */
const char *gamelog_result(int code)
{
    switch (code) {
    case 0:  return "You won: the enemy has no flagships left";
    case 1:  return "You won: you accepted the enemy's surrender";
    case 2:  return "You won: the enemy has no ships left";
    case 3:  return "You won: you took the enemy's crown";
    case 4:  return "You won: the enemy has no ships left (after a sea battle)";
    case 10: return "You lost: you have no flagships left";
    case 11: return "You lost: you surrendered";
    case 12: return "You lost: you have no ships left";
    case 13: return "You lost: the enemy took your crown";
    case 14: return "You lost: you have no ships left (after a sea battle)";
    default: return "The game ended";
    }
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

void gamelog_iteration(const Play *p) { if (G.f && p == G.play && !p->demo) G.iterations++; }
void gamelog_tick(const Play *p)
{
    if (!G.f || p != G.play || p->demo) return;
    if (G.level >= GLOG_VERBOSE && G.ticks > 0 && G.ticks % 16 == 0) gamelog_squads(p, "Every 16 ticks: the squadrons");
    G.ticks++;
}

static void close_file(void)
{
    if (G.f) SDL_CloseIO(G.f);
    G.f = NULL;
    G.play = NULL;
}

/* "1805 Trafalgar" -> a name a file system takes */
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

/* the rules as the Rulebook words them (rules_write ovl1:9428, the SO3 texts) */
static void rules_text(const uint16_t *r)
{
    char t[NUM_RULES][64];
    SDL_snprintf(t[0], 64, "Ports repair ships %s", r[0] == 20 ? "slowly" : r[0] == 40 ? "steadily" : "briskly");
    SDL_snprintf(t[1], 64, "Ports need supplies %s", r[1] == 20 ? "never" : r[1] == 10 ? "rarely" : "often");
    SDL_snprintf(t[2], 64, "(not in the Rulebook)");
    SDL_snprintf(t[3], 64, "Rough water is %s", r[3] == 1 ? "mild" : r[3] == 2 ? "risky" : "very risky");
    SDL_snprintf(t[4], 64, "You have a%s crew", r[4] == 4 ? " green" : r[4] == 2 ? "n average" : " crack");
    SDL_snprintf(t[5], 64, "Visibility is %s", r[5] == 20 ? "poor" : r[5] == 40 ? "good" : "unlimited");
    SDL_snprintf(t[6], 64, "Shallow water is %s", r[6] == 1 ? "safe" : r[6] == 2 ? "risky" : "very risky");
    SDL_snprintf(t[7], 64, "Wind changes %s", r[7] == 0 ? "never" : r[7] == 1 ? "rarely" : "often");
    SDL_snprintf(t[8], 64, "(not in the Rulebook)");
    static const char *const USE[NUM_RULES] = {
        "chance (of 128) of a repair each time an anchored squadron is fed",
        "what a repair port holds; supply ports and crowns twice that; 20+: never run out",
        "",
        "0 very risky, 1 safe, else risky (frigates)",
        "how fast your squadrons eat: chance (x 32 - 10)/128 of 2 supplies",
        "2|dx| + |dy| under it is seen; 80: always",
        "0 very risky, 1 safe, else risky (ships of the line)",
        "chance 16 (1) or 64 (2) of 128 every 31 ticks",
        "",
    };
    out("Rules (as the Rulebook words them; DS:64AD-64BD, the value in the S file):\n");
    for (int k = 0; k < NUM_RULES; k++) out("  %04X %-32s %5d  %s\n", 0x64AD + 2 * k, t[k], (int16_t)r[k], USE[k]);
    out("\n");
}

/* WAS_Analysis/README.md and WAS_file_reference.md, the opponents' values */
static const char *const OPP_USE[OPP_WORDS] = {
    "squadrons bound for a crown target that make it crowded (dropped with chance 60/128)",
    "(the demo sets 3)",
    "(not used)",
    "chance its squadron is hit in a fight on the map (yours: 65)",
    "a flagship squadron's caution (holds back from attacking); in battle: in line, not abreast",
    "chance of forgetting a threatened crown after a win; /8: a squadron by a port simply stays; 115: never stays for supplies or repairs",
    "how fast its squadrons eat (of 128)",
    "the ratio of your squadrons to theirs that makes it offer to surrender (500: never)",
    "hunger: supplies under which a squadron goes for supplies",
    "the damage a squadron puts up with before going for repairs",
    "chance of a random amount off a cell's score in choosing targets",
    "how far it goes to attack (x 8 steps)",
    "the strength margin it attacks with",
    "(changed by end_check)",
    "(not used)",
    "in battle: 0 closer at the start on the open sea",
    "(x 8 of 128) how often a lost merchant tells it where your crown is",
    "(not used)",
    "how much your strength puts it off (its routes, its targets)",
    "squadrons kept guarding its own crown",
    "chance of going for your crown or flagship (once neither side has ports)",
    "boarding: officers",
    "boarding: their pistols",
    "boarding: the men's blows",
    "(not used)",
};

static void opponent_values(const int16_t *v)
{
    for (int k = 0; k < OPP_WORDS; k++) out("  %04X %6d  %s\n", 0x604D + 2 * k, v[k], OPP_USE[k]);
    out("\n");
}

static void map_text(const Campaign *c)
{
    out("Map (cell = row x 10 + column; row 0 is the top, the north):\n      0123456789\n");
    for (int r = 0; r < MAP_H; r++) {
        out("  %2d  ", r);
        for (int k = 0; k < MAP_W; k++) out("%c", terrain_char(c->map[r][k]));
        out("\n");
    }
    out("  . sea, , shallow water, ~ rough water, / shore, # land, R repair port, S supply port,\n"
        "  C the enemy's crown, Y your crown\n\n");
}

static const char *speed_name(int s) { return s == 0 ? "anchored" : s == 1 ? "half sail" : s == 2 ? "full sail" : "?"; }

static void squads_table(const Play *p)
{
    out("  slot side   x,y      cell            speed      sup  strength  ships; what it is doing\n");
    for (int i = 0; i < MAX_SHIPS; i++) {
        const PSquad *q = &p->sq[i];
        if (!q->used) continue;
        int c = gamelog_cell_of(p, i);
        char plan[128] = "";
        if (q->partner) SDL_snprintf(plan, sizeof plan, "; %s squadron %d (%s)", q->ctrl > 0 ? "encounter with" : "fighting",
                                     q->partner - 1, q->ctrl > 0 ? "countdown" : "on the map");
        else if (q->mark == 2) SDL_snprintf(plan, sizeof plan, "; merchant for %s", gamelog_cell(p, q->merchant_dest));
        else if (q->side == 0 && q->has_route) SDL_snprintf(plan, sizeof plan, "; on a route to %s%s", gamelog_cell(p, q->merchant_dest),
                                                           q->has_route == 2 ? " (to attack)" : "");
        else if (q->has_path && q->dest != NO_DEST) SDL_snprintf(plan, sizeof plan, "; path to %d,%d", q->dest % 80, q->dest / 80);
        else if (q->has_path) SDL_snprintf(plan, sizeof plan, "; on a path");
        out("  %4d %-6s %3d,%-3d  %3d %-13s  %-9s  %3d  %8d  %s%s%s\n", i, q->side == -1 ? "yours" : "enemy", q->x, q->y, c,
            terrain_name(tile_at(p, c)), speed_name(q->speed), q->supplies, gamelog_strength(p, i),
            q->mark == 2 ? "merchant" : gamelog_ships(p, i), q->seen ? " (seen)" : "", plan);
    }
}

static void ports_table(const Play *p)
{
    for (int c = 0; c < NUM_CELLS; c++) {
        int t = p->map[c];
        if (t < 74 || t > 77) continue;
        out("  %-17s cell %3d (row %2d col %d): %2d left%s\n", terrain_name(t), c, c / 10, c % 10, p->port_sup[c],
            p->inverted[c] ? " (shown empty)" : "");
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

static const char *wind_name(int w)
{
    return w == 0 ? "North" : w == 1 ? "East" : w == 2 ? "South" : w == 3 ? "West" : "?";
}

void gamelog_begin(const Play *p, const Campaign *c, const GameAssets *ga, const char *how, unsigned rnd, unsigned ptr)
{
    close_file();
    if (G.level == GLOG_OFF || !p || !c || !ga) return;
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

    int opp = SDL_clamp(c->opponent, 0, NUM_OPPONENTS - 1);
    int fd = assets_scnf_word(ga, 0x6851), wind = assets_scnf_word(ga, 0x6865);
    int reload = (int8_t)ga->scnf[0x6867 - 0x684C], aim = (int8_t)ga->scnf[0x6868 - 0x684C];
    out("The Ancient Art of War at Sea - remake: game log (%s)\n", G.level >= GLOG_VERBOSE ? "verbose" : "normal");
    out("==================================================================\n");
    out("Opened       %04d-%02d-%02d %02d:%02d:%02d (local time)\n", dt.year, dt.month, dt.day, dt.hour, dt.minute, dt.second);
    out("Game         %s\n", how);
    out("Campaign     %s (%s)\n", c->title, c->file[0] ? c->file : "no file");
    out("Opponent     %s (opponent %d)\n", OPPONENT_NAMES[opp], c->opponent);
    out("Random       random() at %02Xh, its pointer at %04Xh (WAS.EXE: the BIOS clock, then the menus' calls)\n", rnd & 0xFFFF, ptr & 0xFFFF);
    out("Squadrons    %d yours (%d ships), %d the enemy's (%d ships)\n", c->num_friendly, c->ships_friendly, c->num_enemy, c->ships_enemy);
    out("Options      fight delay %d ticks (%s): an encounter not zoomed into is fought out on the map after it\n", fd,
        fd == 250 ? "short" : fd == 500 ? "medium" : fd == 1000 ? "long" : "?");
    out("             wind %s; reload time %s; cannon aim %s\n", wind ? "realistic" : "not realistic",
        reload == 10 ? "short" : reload == 3 ? "medium" : reload == 0 ? "realistic" : "?", aim ? "automatic" : "manual");
    out("\n");
    rules_text(c->rules);
    if (ga->have_so2) {
        out("%s's values (SO2 + 277h, copied to DS:604D; chances are out of 128):\n", OPPONENT_NAMES[opp]);
        opponent_values(ga->opp[opp]);
    }
    map_text(c);
    out("How to read the log\n");
    out("  [mm:ss tick N]  time on the map since the log opened (27 iterations a second, as an XT;\n");
    out("                  the battles and menus don't count) and game ticks (the Time setting: a tick\n");
    out("                  each 64, 16, 4 or 1 iterations). Squadrons are named by slot (0-39) with their\n");
    out("                  ships (type; sails and hull damage as s1h2, 0-5), x,y on the map (x in 4-pixel\n");
    out("                  columns 0-79, y in line pairs 0-199), and the cell they are in (row x 10 + col).\n");
    out("  ENEMY           what the enemy commander decides, and why (the values above).\n");
    out("  YOU             your orders and answers.\n");
    out("  EVENT           sightings, encounters, fights on the map, merchants, ports, the wind, ships\n");
    out("                  lost at sea, crowns, the end.\n");
    out("  BATTLE          the sea battle (Zoom, Practice) and boarding, with the battle's own clock.\n");
    if (G.level >= GLOG_VERBOSE)
        out("  verbose         also each squadron the enemy looks at and why it stays, each cell it weighs\n"
            "                  for a target with its score, the strength and port-value maps when they are\n"
            "                  worked out again, supplies and repairs taken in port, broadsides fired, and a\n"
            "                  table of all squadrons every 16 ticks. A route's cells cost the danger near\n"
            "                  your strength ((6071 - strength) / 2 + 3 where that is over 0), plus rough or\n"
            "                  shallow water for the ships that fear it (rules 64B3, 64B9: 32 x rule, 120 at 0).\n");
    out("\n");
    SDL_FlushIO(G.f);
}

void gamelog_state(const Play *p, const char *title)
{
    if (!gamelog_on(p, GLOG_NORMAL)) return;
    long s = G.iterations / PLAY_HZ;
    out("---- %s [%ld:%02ld tick %ld] ----\n", title, s / 60, s % 60, G.ticks);
    out("Time setting: %s. Wind: %s. Random state %02Xh at %04Xh.\n", time_name(p->time_div),
        p->wind_on ? wind_name(p->wind) : "off", p->rnd, p->rnd_ptr);
    if (p->ga && p->ga->have_so2 && p->c) {
        const int16_t *was = p->ga->opp[SDL_clamp(p->c->opponent, 0, NUM_OPPONENTS - 1)];
        int changed = 0;
        for (int k = 0; k < OPP_WORDS; k++) changed += p->opp[k] != was[k];
        if (!changed) out("Opponent's values: as in SO2.\n");
        else {
            out("Opponent's values changed in play:\n");
            for (int k = 0; k < OPP_WORDS; k++)
                if (p->opp[k] != was[k]) out("  %04X %6d  %s (was %d)\n", 0x604D + 2 * k, p->opp[k], OPP_USE[k], was[k]);
        }
    }
    out("Ports and crowns (supply ports %d, repair ports %d, crowns %d of the enemy's; counted as WAS.EXE does):\n",
        p->ports_supply, p->ports_repair, p->crowns);
    ports_table(p);
    int nports = 0;
    while (nports < MAX_PORTS && p->port_cell[nports] != -1) nports++;
    out("Ports merchants can reach: %d.\n", nports);
    out("Squadrons:\n");
    squads_table(p);
    out("Enemy's plans: standing target %s; target dropped last %s; %s.\n",
        p->crown_threat < 0 ? "none" : gamelog_cell(p, p->crown_threat),
        p->ai_dropped < 0 ? "none" : gamelog_cell(p, p->ai_dropped),
        p->merchant_lost ? "a standing target is set (6898)" : "no standing target set (6898 0)");
    out("Losses: yours %d (flagships %d), theirs %d (flagships %d); taken: your ships %d, theirs %d.\n",
        p->losses[0], p->losses[2], p->losses[1], p->losses[3], p->captured[0], p->captured[1]);
    out("\n");
    if (G.level >= GLOG_VERBOSE) {
        grid("Strength map (DS:4645: the enemy's squadrons +, yours -, -7 your flagship):", p->strength);
        grid("Port values (DS:47D5: what the cells beside ports and crowns are worth to the enemy):", p->pvalue);
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
