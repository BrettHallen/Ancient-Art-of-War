/*
 * The Ancient Art of War at Sea (1987) - remake
 * Nov 2026 using Claude Opus 5.5
 * "Guided" by Brett Hallen
 * Original game by Dave Murry & Barry Murry
 * Published by Brøderbund Software Inc.
 *
 * test/test_enemy.c: the enemy commander, step 5. Each campaign is left to
 * run at the fastest time with nobody giving your squadrons orders, as was
 * done with the original in DOSBox (WAS_Analysis/tools/dosbox): the enemy
 * sets out, takes merchants, attacks, and in The Bane of Oldain makes for
 * your crown. Writes the map every so often as a BMP and logs how each
 * game ends. (The routines themselves are compared with WAS.EXE's, under
 * an emulator, by test/diff_ovl4.)
 *
 * Usage: test_enemy GAME-FOLDER OUT-FOLDER
 */
#include <SDL3/SDL.h>
#include <stdarg.h>

#include "../src/assets.h"
#include "../src/campaign.h"
#include "../src/play.h"

static int fails = 0;
static void fail(int line, const char *fmt, ...)
{
    char b[256];
    va_list ap;
    va_start(ap, fmt);
    SDL_vsnprintf(b, sizeof b, fmt, ap);
    va_end(ap);
    SDL_Log("FAIL line %d: %s", line, b);
    fails++;
}
#define CHECK(c, ...) do { if (!(c)) fail(__LINE__, __VA_ARGS__); } while (0)

static void save(const Play *p, const GameAssets *ga, const char *dir, const char *name)
{
    static Screen s;
    (void)ga;
    play_draw(p, &s, MODE_CGA);
    SDL_Surface *surf = SDL_CreateSurface(SCREEN_W, SCREEN_H, SDL_PIXELFORMAT_XRGB8888);
    for (int y = 0; y < SCREEN_H; y++)
        for (int x = 0; x < SCREEN_W; x++) {
            RGB c = s.pal[s.px[y * SCREEN_W + x]];
            ((uint32_t *)surf->pixels)[y * (surf->pitch / 4) + x] = (uint32_t)(c.r << 16 | c.g << 8 | c.b);
        }
    char path[512];
    SDL_snprintf(path, sizeof path, "%s/%s.bmp", dir, name);
    SDL_SaveBMP(surf, path);
    SDL_DestroySurface(surf);
}

static int enemy_moved(const Play *a, const int x0[], const int y0[])
{
    int n = 0;
    for (int i = 0; i < MAX_SHIPS; i++)
        if (a->sq[i].used && a->sq[i].side == 0 && a->sq[i].mark != 2 && (a->sq[i].x != x0[i] || a->sq[i].y != y0[i])) n++;
    return n;
}

int main(int argc, char **argv)
{
    if (argc < 3) { SDL_Log("usage: test_enemy GAME-FOLDER OUT-FOLDER"); return 2; }
    static GameAssets ga;
    static Campaign cs[NUM_CAMPAIGNS];
    static Play p;
    char err[256];
    if (!assets_load(&ga, argv[1], err, sizeof err)) { SDL_Log("load: %s", err); return 1; }
    CHECK(campaigns_load(cs, argv[1]) == NUM_CAMPAIGNS, "campaigns");
    for (int c = 0; c < NUM_CAMPAIGNS; c++) {
        int moved_total = 0, merchants = 0, routes = 0;
        for (int seed = 1; seed <= 3; seed++) {
            play_start(&p, &cs[c], &ga, (unsigned)seed);
            while (p.time_div != 1) { play_key(&p, PK_T, false); play_frame(&p, 0); }
            int x0[MAX_SHIPS], y0[MAX_SHIPS];
            for (int i = 0; i < MAX_SHIPS; i++) { x0[i] = p.sq[i].x; y0[i] = p.sq[i].y; }
            int f = 0, ticks = 0, last = p.ticks;
            for (; f < PLAY_HZ * 600 && p.mode != PM_END; f++) {
                if (p.stall) { p.stall--; continue; }   /* WAS.EXE working: the time an XT takes */
                if (p.mode == PM_OFFER) play_key(&p, PK_N, false);
                play_frame(&p, 0);
                if (p.ticks != last) { ticks++; last = p.ticks; }
                for (int i = 0; i < MAX_SHIPS; i++) {
                    if (p.sq[i].used && p.sq[i].mark == 2) merchants++;
                    if (p.sq[i].used && p.sq[i].has_route) routes++;
                }
                if (seed == 1 && f % (PLAY_HZ * 60) == 0) {
                    char n[32];
                    SDL_snprintf(n, sizeof n, "e%d_t%d", c, f / PLAY_HZ);
                    save(&p, &ga, argv[2], n);
                }
            }
            moved_total += enemy_moved(&p, x0, y0);
            static const char *HOW[15] = { "you lost your last flagship", "their surrender accepted", "they lost every ship", "their crown taken",
                "they lost every ship (a sea battle)", "", "", "", "", "", "you lost your last flagship", "", "you lost every ship", "your crown taken",
                "you lost every ship (a sea battle)" };
            SDL_Log("S%d seed %d: %s after %d s of play (%d ticks): %s", c, seed, p.mode == PM_END ? "over" : "still going", f / PLAY_HZ,
                    ticks, p.mode == PM_END && p.result_code >= 0 && p.result_code < 15 ? HOW[p.result_code] : "-");
            if (seed == 1) {
                char n[32];
                SDL_snprintf(n, sizeof n, "e%d_end", c);
                save(&p, &ga, argv[2], n);
            }
        }
        SDL_Log("S%d: %d enemy squadrons moved, %d merchant-frames, %d route-frames", c, moved_total, merchants, routes);
        CHECK(moved_total > 0 || c == 4, "S%d: the enemy never moved", c);
    }
    if (fails) SDL_Log("%d FAILED", fails);
    else SDL_Log("all passed");
    return fails != 0;
}
