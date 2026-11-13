/*
 * The Ancient Art of War at Sea (1987) - remake
 * Nov 2026 using Claude Opus 5.5
 * "Guided" by Brett Hallen
 * Original game by Dave Murry & Barry Murry
 * Published by Brøderbund Software Inc.
 *
 * test_gamelog.c: the debug log changes nothing. Every campaign is played
 * twice side by side with the same keys (paths traced, Info and Speed,
 * Detach and Join, Time, Fleet, Pause, Clear, the questions, Zoom and
 * Practice, the sea battle's keys and boarding's): one game logged (normal,
 * then verbose), one not. After each iteration and each slice of a battle
 * the two games must be the same, byte for byte (the Play structs, and the
 * battles but for their fibers). Then the logs are checked for their header
 * and the kinds of lines expected, and the demo and logging off must write
 * nothing.
 *
 * test_gamelog GAME-FOLDER LOG-FOLDER
 */
#include <SDL3/SDL.h>
#include <stddef.h>
#include "../src/play.h"
#include "../src/battle.h"
#include "../src/gamelog.h"

static GameAssets ga;
static Campaign camps[NUM_CAMPAIGNS];
static Play pa, pb;          /* pa is logged */
static Battle ba, bb;

static int fails;
#define CHECK(c, ...) do { if (!(c)) { SDL_Log("FAIL: " __VA_ARGS__); fails++; } } while (0)

static bool same_play(void) { return SDL_memcmp(&pa, &pb, sizeof pa) == 0; }

static bool same_battle(void)
{
    /* b->p points at its own game and the fibers are their own: compare the rest */
    size_t a = offsetof(Battle, m), f = offsetof(Battle, fiber), c = offsetof(Battle, clock_us);
    return SDL_memcmp((const char *)&ba + a, (const char *)&bb + a, f - a) == 0 &&
           SDL_memcmp((const char *)&ba + c, (const char *)&bb + c, sizeof ba - c) == 0 && same_play();
}

static void both_key(int key) { play_key(&pa, key, false); play_key(&pb, key, false); }
/* a battle asked for starts before the next frame (main.c: play_step) */
static void both_frame(int held, long *it)
{
    if (pa.battle_req) return;
    play_frame(&pa, held);
    play_frame(&pb, held);
    (*it)++;
}

static unsigned R;
static unsigned rnd(void) { R = R * 1103515245u + 12345u; return (R >> 16) & 0x7FFF; }

/* the sea battle, both at once; false if they differ */
static bool battle_both(int c, bool practice, long *battles, int *boardings)
{
    bool okA = battle_start(&ba, &pa, practice), okB = battle_start(&bb, &pb, practice);
    if (okA != okB) { SDL_Log("campaign %d: one battle didn't start", c); return false; }
    if (!okA) { pa.battle_req = pb.battle_req = 0; return true; }
    static const char keys[] = "oo+\rss-\r+c+\rgafat\r+-vv o+\r";
    bool boarded = false;
    for (int ms = 0; ms < 240000 && ba.state == BATTLE_RUNNING; ms += 16) {
        if (ba.menu == 'B' && rnd() % 8 == 0) {          /* a grapple: Zoom (the boarding) or Pass */
            int k = rnd() % 3 ? 'z' : 'p';
            if (k == 'z') boarded = true;
            battle_key(&ba, k, 0); battle_key(&bb, k, 0);
        } else if (rnd() % 60 == 0) {
            int k = keys[rnd() % (sizeof keys - 1)];
            if (boarded && rnd() % 3 == 0) k = "afqprcA"[rnd() % 7];
            if (ms > 150000 && rnd() % 4 == 0) k = 'l';   /* time to Leave */
            battle_key(&ba, k, 0); battle_key(&bb, k, 0);
        }
        battle_run(&ba, 16000);
        battle_run(&bb, 16000);
        if (ba.state != bb.state || !same_battle()) { SDL_Log("campaign %d: the battles differ after %d ms", c, ms); return false; }
    }
    if (ba.state == BATTLE_RUNNING) { ba.state = bb.state = BATTLE_QUIT; }
    battle_finish(&ba);
    battle_finish(&bb);
    (*battles)++;
    if (boarded) (*boardings)++;
    return same_play();
}

/* one campaign, both games in step; returns false if they differ */
static bool play_both(int c, unsigned seed, long *battles, int *boardings)
{
    long it = 0;
    pa.demo = 0;
    gamelog_begin(&pa, &camps[c], &ga, "New campaign (test)", seed & 0xFF, 0xBB8);
    play_start(&pa, &camps[c], &ga, seed);
    play_start(&pb, &camps[c], &ga, seed);
    if (!same_play()) { SDL_Log("campaign %d differs at the start", c); return false; }
    static const int keys[] = { PK_T, PK_T, PK_F, PK_P, PK_C, PK_I, PK_S, PK_D, PK_J, PK_E, PK_M, PK_M, PK_MINUS, PK_EQUALS, PK_ESC, PK_Z };
    int held = 0, zooms = 0;
    for (int f = 0; f < 30000 && pa.result == PLAY_RUNNING; f++) {
        if (pa.battle_req) {                     /* as main.c: the battle starts straight after the frame that asked */
            if (!battle_both(c, pa.battle_req == 2, battles, boardings)) return false;
            continue;
        }
        unsigned v = rnd() % 100;
        if (v < 4) both_key(keys[rnd() % (sizeof keys / sizeof keys[0])]);
        if (v >= 4 && v < 14) held = PK_HOME + (int)(rnd() % 11);
        else if (v >= 14 && v < 30) held = 0;
        if (pa.mode == PM_PROMPT && !pa.ended && !play_busy(&pa)) both_key(PK_N);   /* never surrender or quit */
        if (pa.mode == PM_OFFER && !play_busy(&pa)) both_key(rnd() % 2 ? PK_Y : PK_N);
        if ((pa.mode == PM_END || pa.ended) && !play_busy(&pa)) both_key(PK_ENTER);
        if ((pa.mode == PM_INFO || pa.mode == PM_FLEET || pa.mode == PM_PAUSE || pa.mode == PM_DETACH) && rnd() % 20 == 0)
            both_key(rnd() % 3 ? PK_E : PK_S);
        if (v >= 97 && (pa.mode == PM_NORMAL || pa.mode == PM_SQUAD) && !play_busy(&pa)) {   /* jump onto a squadron: orders */
            int i = (int)(rnd() % MAX_SHIPS);
            const PSquad *q = &pa.sq[i];
            if (q->used && q->side == -1 && q->mark != 2 && q->x > 2 && q->x < CX_MAX) {
                pa.cx = pb.cx = q->x - 1;
                pa.cy = pb.cy = q->y + 1;
                pa.view_top = pb.view_top = SDL_clamp(pa.cy - 40, 0, VIEW_TOP_MAX);
                both_frame(PK_RIGHT, &it);
                if (pa.mode == PM_SQUAD) {
                    unsigned w = rnd() % 10;
                    if (w < 6) {                         /* Move: trace a path */
                        both_key(PK_M); both_frame(0, &it);
                        int dir = PK_HOME + (int)(rnd() % 11);
                        for (int k = 0, n = 20 + (int)(rnd() % 60); k < n; k++) both_frame(dir, &it);
                        both_key(PK_M); both_frame(0, &it);
                    } else if (w < 7) { both_key(PK_P); both_frame(0, &it); }   /* Practice */
                    else if (w < 8) { both_key(PK_D); both_frame(0, &it); }
                    else if (w < 9) { both_key(PK_J); both_frame(0, &it); }
                    else { both_key(PK_I); both_frame(0, &it); both_key(PK_S); both_frame(0, &it); }
                }
            }
        }
        /* an encounter: Zoom to it now and then */
        if (pa.battle_req) continue;
        if (v % 4 == 0 && pa.mode == PM_NORMAL && !play_busy(&pa) && zooms < 6)
            for (int i = 0; i < MAX_SHIPS; i++) {
                const PSquad *q = &pa.sq[i];
                if (!q->used || q->side != -1 || !q->partner || q->ctrl <= 0 || q->ctrl > pa.fight_delay - 20 || q->x < 2) continue;
                pa.cx = pb.cx = q->x - 1;
                pa.cy = pb.cy = q->y + 1;
                pa.view_top = pb.view_top = SDL_clamp(pa.cy - 40, 0, VIEW_TOP_MAX);
                both_frame(PK_RIGHT, &it);
                if (pa.mode == PM_SQUAD && pa.sel >= 0 && pa.sq[pa.sel].partner) { both_key(PK_Z); both_frame(0, &it); zooms++; }
                break;
            }
        if (pa.battle_req) continue;
        both_frame(held, &it);
        if (!same_play()) { SDL_Log("campaign %d differs after %ld iterations", c, it); return false; }
    }
    SDL_Log("campaign %d: %ld iterations, result %d (code %d), losses %d/%d", c, it, pa.result, pa.result_code, pa.losses[0], pa.losses[1]);
    gamelog_end(&pa, pa.result == PLAY_RUNNING ? "Test over" : gamelog_result(pa.result_code));
    return true;
}

static int count_of(const char *text, const char *word)
{
    int n = 0;
    for (const char *s = text; (s = SDL_strstr(s, word)) != NULL; s++) n++;
    return n;
}

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
    int boardings = 0;
    for (int level = GLOG_NORMAL; level <= GLOG_VERBOSE; level++) {
        char dir[600];
        SDL_snprintf(dir, sizeof dir, "%s/%s/", argv[2], level == GLOG_VERBOSE ? "verbose" : "normal");
        gamelog_setup((GameLogLevel)level, dir);
        for (int c = 0; c < NUM_CAMPAIGNS; c++) {
            if (!camps[c].loaded) continue;
            R = 777u + (unsigned)c * 31u + (unsigned)level;
            CHECK(play_both(c, (unsigned)(c * 37 + level), &battles, &boardings),
                  "campaign %d plays differently with the %s log", c, level == GLOG_VERBOSE ? "verbose" : "normal");
        }
        int n = list_logs(dir, names, 64);
        CHECK(n >= 1, "no logs in %s", dir);
        int enemy = 0, you = 0, event = 0, battle = 0, looks = 0, header = 0;
        for (int k = 0; k < n; k++) {
            size_t size = 0;
            char *t = SDL_LoadFile(names[k], &size);
            if (!t) continue;
            total_bytes[level] += (long)size;
            header += SDL_strstr(t, "Rules (as the Rulebook") && SDL_strstr(t, "Opponent ") && SDL_strstr(t, "Map (cell") &&
                      SDL_strstr(t, "The game at the start") && SDL_strstr(t, "Log closed");
            enemy += count_of(t, "] ENEMY ");
            you += count_of(t, "] YOU ");
            event += count_of(t, "] EVENT ");
            battle += count_of(t, "] BATTLE ");
            looks += count_of(t, "] ENEMY looks at ");
            SDL_free(t);
        }
        SDL_Log("%s: %d logs, %ld bytes; lines: ENEMY %d, YOU %d, EVENT %d, BATTLE %d", dir, n, total_bytes[level], enemy, you, event, battle);
        CHECK(header == n, "%d of %d logs have the full header and ending", header, n);
        CHECK(enemy > 0 && you > 0 && event > 0, "lines of each kind");
        CHECK(battle > 0 || battles == 0, "battle lines");
        if (level == GLOG_VERBOSE) CHECK(looks > 0, "verbose: the squadrons the enemy looks at");
        else CHECK(looks == 0, "normal: no verbose lines");
    }
    CHECK(total_bytes[GLOG_VERBOSE] > total_bytes[GLOG_NORMAL], "the verbose logs are longer");
    SDL_Log("%ld sea battles (%d with a boarding) fought side by side", battles, boardings);
    CHECK(battles > 0, "no battles were fought");

    /* the demo is not logged */
    char dir[600];
    SDL_snprintf(dir, sizeof dir, "%s/demo/", argv[2]);
    gamelog_setup(GLOG_VERBOSE, dir);
    play_start_demo(&pa, &camps[0], &ga, 5, 0xBB8);
    for (int f = 0; f < 3000 && pa.result == PLAY_RUNNING; f++) play_frame(&pa, 0);
    CHECK(list_logs(dir, names, 64) == 0, "the demo wrote a log");
    CHECK(!gamelog_on(&pa, GLOG_NORMAL), "the demo is logged");

    /* logging off: no file */
    SDL_snprintf(dir, sizeof dir, "%s/off/", argv[2]);
    gamelog_setup(GLOG_OFF, dir);
    pa.demo = 0;
    gamelog_begin(&pa, &camps[0], &ga, "New campaign", 1, 0xBB8);
    play_start(&pa, &camps[0], &ga, 1);
    CHECK(list_logs(dir, names, 64) == 0 && !*gamelog_path(), "a log with logging off");

    if (fails) { SDL_Log("%d FAILED", fails); return 1; }
    SDL_Log("test_gamelog: all passed");
    return 0;
}
