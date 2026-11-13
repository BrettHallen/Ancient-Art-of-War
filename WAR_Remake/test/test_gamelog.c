/*
 * The Ancient Art of War (1984) - remake
 * Nov 2026 using Claude Opus 5.5
 * "Guided" by Brett Hallen
 * Original game by Dave Murry & Barry Murry
 * Published by Brøderbund Software Inc.
 *
 * test_gamelog.c: the debug log changes nothing. Every campaign is played
 * twice side by side with the same keys (orders, Time, Zoom and the battle
 * screen's keys): one game logged (normal, then verbose), one not. After each
 * iteration and each battle frame the two games must be the same, byte for
 * byte. Then the logs are checked for their header and the kinds of lines
 * expected, and the demo must not be logged.
 *
 * test_gamelog GAME-FOLDER LOG-FOLDER
 */
#include <SDL3/SDL.h>
#include "../src/play.h"
#include "../src/battle.h"
#include "../src/gamelog.h"

static GameAssets ga;
static Campaign camps[NUM_CAMPAIGNS];
static Play pa, pb;          /* pa is logged */
static Battle ba, bb;

static int fails;
#define CHECK(c, ...) do { if (!(c)) { SDL_Log("FAIL: " __VA_ARGS__); fails++; } } while (0)

static bool same_play(void)
{
    return SDL_memcmp(&pa, &pb, sizeof pa) == 0;
}

static bool same_battle(void)
{
    /* b->p points at its own game: compare everything after it */
    return SDL_memcmp((const char *)&ba + sizeof ba.p, (const char *)&bb + sizeof bb.p, sizeof ba - sizeof ba.p) == 0 && same_play();
}

static void both_key(int key, bool shift) { play_key(&pa, key, shift); play_key(&pb, key, shift); }

static void both_frame(int held, long *it)
{
    play_frame(&pa, held);
    play_frame(&pb, held);
    (*it)++;
}

/* one campaign, both games in step; returns false if they differ */
static bool play_both(int c, uint32_t seed, unsigned r, long *battles)
{
    long it = 0;
    gamelog_begin(&pa, &camps[c], "New campaign (test)", seed, 400);
    play_start(&pa, &camps[c], &ga, seed);
    play_start(&pb, &camps[c], &ga, seed);
    if (!same_play()) { SDL_Log("campaign %d differs at the start", c); return false; }
    static const int keys[] = { PK_M, PK_M, PK_C, PK_T, PK_I, PK_S, PK_E, PK_N, PK_MINUS, PK_EQUALS, PK_D, PK_J, PK_F, PK_E, PK_V, PK_E };
    int held = 0;
    for (int f = 0; f < 40000 && pa.result == PLAY_RUNNING; f++) {
        r = r * 1103515245u + 12345u;
        unsigned v = (r >> 16) % 100;
        if (v < 5) both_key(keys[(r >> 8) % (sizeof keys / sizeof keys[0])], false);
        if (v >= 5 && v < 18) held = PK_HOME + (int)((r >> 4) % 11);
        else if (v >= 18 && v < 30) held = 0;
        if (pa.mode == PM_PROMPT && !pa.ended) both_key(pa.prompt_offer && (v & 1) ? PK_Y : PK_N, false);   /* never surrender */
        if (pa.ended) both_key(PK_ENTER, false);
        if (v >= 96 && (pa.mode == PM_NORMAL || pa.mode == PM_SQUAD)) {   /* jump onto a squad and give it orders */
            int i = (int)((r >> 9) % MAX_SQUADS);
            if (pa.sq[i].used && pa.sq[i].friendly && pa.sq[i].x < 0x4E) {
                pa.cx = pb.cx = pa.sq[i].x;
                pa.cy = pb.cy = pa.sq[i].y;
                pa.view_top = pb.view_top = SDL_clamp(pa.cy - 40, 0, VIEW_TOP_MAX);
                both_frame(PK_RIGHT, &it);
                if (pa.mode == PM_SQUAD) {
                    both_key(PK_M, false); both_frame(0, &it);
                    int dir = PK_HOME + (int)((r >> 12) % 11);
                    for (int k = 0; k < 20 + (int)((r >> 3) % 40); k++) both_frame(dir, &it);
                    both_key(PK_M, false); both_frame(0, &it);
                }
            }
        }
        /* an encounter: Zoom to it now and then */
        for (int i = 0; i < MAX_SQUADS && (v % 3) == 0; i++) {
            const PSquad *q = &pa.sq[i];
            if (q->used && q->friendly && q->fighting && q->clock > 0 && q->clock < 50 && pa.mode == PM_NORMAL) {
                pa.cx = pb.cx = q->x + 1;
                pa.cy = pb.cy = q->y;
                both_frame(PK_RIGHT, &it);
                both_frame(PK_LEFT, &it);
                if (pa.mode == PM_SQUAD && pa.sq[pa.sel].fighting) { both_key(PK_Z, false); both_frame(0, &it); }
                break;
            }
        }
        if (pa.zoom_squad >= 0) {
            bool okA = battle_start(&ba, &pa, pa.zoom_squad), okB = battle_start(&bb, &pb, pb.zoom_squad);
            if (okA != okB || !same_battle()) { SDL_Log("campaign %d: the battles differ at the start", c); return false; }
            if (okA) {
                static const int bkeys[] = { PK_A, PK_K, PK_A, PK_B, PK_F, PK_S, PK_A, PK_B, PK_BACKSPACE, PK_R };
                for (int fr = 0; fr < 4000 && !ba.done; fr++) {
                    r = r * 1103515245u + 12345u;
                    if (fr == 25 || ((r >> 16) % 40) == 0) {
                        int k = fr == 25 ? PK_A : bkeys[(r >> 8) % (sizeof bkeys / sizeof bkeys[0] - ((r & 7) ? 1 : 0))];
                        battle_key(&ba, k);
                        battle_key(&bb, k);
                    }
                    battle_frame(&ba);
                    battle_frame(&bb);
                    if (!same_battle()) { SDL_Log("campaign %d: the battles differ at frame %d", c, fr); return false; }
                }
                battle_finish(&ba);
                battle_finish(&bb);
                (*battles)++;
            } else {
                pa.zoom_squad = pb.zoom_squad = -1;
            }
        }
        if (pa.mode == PM_SIZE || pa.mode == PM_INFO || pa.mode == PM_VIEW) both_key(PK_E, false);
        both_frame(held, &it);
        if (!same_play()) { SDL_Log("campaign %d differs after %ld iterations", c, it); return false; }
    }
    SDL_Log("campaign %d: %ld iterations, result %d (code %d), men lost %d/%d", c, it, pa.result, pa.result_code, pa.lost_mine, pa.lost_theirs);
    gamelog_end(&pa, pa.result == PLAY_RUNNING ? "Test over" : "The game ended");
    return true;
}

static char *read_all(const char *path, size_t *size)
{
    void *d = SDL_LoadFile(path, size);
    return d;
}

static int count_of(const char *text, const char *word)
{
    int n = 0;
    for (const char *s = text; (s = SDL_strstr(s, word)) != NULL; s++) n++;
    return n;
}

/* the files in the log folder, with their sizes added up */
static int list_logs(const char *dir, char names[64][512], int max)
{
    int n = 0, count = 0;
    char **g = SDL_GlobDirectory(dir, "*.log", 0, &count);
    for (int k = 0; g && k < count && n < max; k++) SDL_snprintf(names[n++], 512, "%s%s", dir, g[k]);
    SDL_free(g);
    return n;
}

int main(int argc, char **argv)
{
    char err[256];
    if (argc < 3 || !assets_load(&ga, argv[1], err, sizeof err)) { SDL_Log("usage: test_gamelog GAME-FOLDER LOG-FOLDER"); return 1; }
    campaigns_load(camps, argv[1]);
    static char names[64][512];

    long total_bytes[3] = { 0 }, battles = 0;
    for (int level = GLOG_NORMAL; level <= GLOG_VERBOSE; level++) {
        char dir[600];
        SDL_snprintf(dir, sizeof dir, "%s/%s/", argv[2], level == GLOG_VERBOSE ? "verbose" : "normal");
        gamelog_setup((GameLogLevel)level, dir);
        for (int c = 0; c < NUM_CAMPAIGNS; c++) {
            if (!camps[c].loaded) continue;
            CHECK(play_both(c, (uint32_t)(c * 37 + level), 777u + (unsigned)c, &battles),
                  "campaign %d plays differently with the %s log", c, level == GLOG_VERBOSE ? "verbose" : "normal");
        }
        int n = list_logs(dir, names, 64);
        CHECK(n >= 1, "no logs in %s", dir);
        int enemy = 0, you = 0, event = 0, battle = 0, holds = 0, header = 0;
        for (int k = 0; k < n; k++) {
            size_t size = 0;
            char *t = read_all(names[k], &size);
            if (!t) continue;
            total_bytes[level] += (long)size;
            header += SDL_strstr(t, "Rules (as the Rulebook") && SDL_strstr(t, "Opponent ") && SDL_strstr(t, "Map (cell") &&
                      SDL_strstr(t, "The game at the start") && SDL_strstr(t, "Log closed");
            enemy += count_of(t, "] ENEMY ");
            you += count_of(t, "] YOU ");
            event += count_of(t, "] EVENT ");
            battle += count_of(t, "] BATTLE ");
            holds += count_of(t, " holds ");
            SDL_free(t);
        }
        SDL_Log("%s: %d logs, %ld bytes; lines: ENEMY %d, YOU %d, EVENT %d, BATTLE %d", dir, n, total_bytes[level], enemy, you, event, battle);
        CHECK(header == n, "%d of %d logs have the full header and ending", header, n);
        CHECK(enemy > 0 && you > 0 && event > 0, "lines of each kind");
        CHECK(battle > 0 || battles == 0, "battle lines");
        if (level == GLOG_VERBOSE) CHECK(holds > 0, "verbose: squads holding ground");
    }
    CHECK(total_bytes[GLOG_VERBOSE] > total_bytes[GLOG_NORMAL], "the verbose logs are longer");
    SDL_Log("%ld battles fought on the battle screen", battles);

    /* the demo is not logged */
    char dir[600];
    SDL_snprintf(dir, sizeof dir, "%s/demo/", argv[2]);
    gamelog_setup(GLOG_VERBOSE, dir);
    play_start_demo(&pa, &camps[0], &ga, 5);
    for (int f = 0; f < 3000 && pa.result == PLAY_RUNNING; f++) play_frame(&pa, 0);
    CHECK(list_logs(dir, names, 64) == 0, "the demo wrote a log");
    CHECK(!gamelog_on(&pa, GLOG_NORMAL), "the demo is logged");

    /* logging off: no file */
    SDL_snprintf(dir, sizeof dir, "%s/off/", argv[2]);
    gamelog_setup(GLOG_OFF, dir);
    gamelog_begin(&pa, &camps[0], "New campaign", 1, 400);
    play_start(&pa, &camps[0], &ga, 1);
    CHECK(list_logs(dir, names, 64) == 0 && !*gamelog_path(), "a log with logging off");

    if (fails) { SDL_Log("%d FAILED", fails); return 1; }
    SDL_Log("test_gamelog: all passed");
    return 0;
}
