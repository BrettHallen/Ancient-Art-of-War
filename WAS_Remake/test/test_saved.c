/*
 * The Ancient Art of War at Sea (1987) - remake
 * Nov 2026 using Claude Opus 5.5
 * "Guided" by Brett Hallen
 * Original game by Dave Murry & Barry Murry
 * Published by Brøderbund Software Inc.
 *
 * test/test_saved.c: saved games (SS, DS:3F39-6804). An SS written by the
 * original WAS.EXE (in DOSBox) is read the way the menus read it, played on
 * and drawn (compare_shots.py compares the map with the original's); the
 * remake's own SS is written and read back, byte for byte; and damaged files
 * (bytes changed at random) must load without harm.
 *
 * Usage: test_saved GAME-FOLDER OUT-FOLDER [ORIGINAL-SS]
 */
#include <SDL3/SDL.h>
#include <stdarg.h>

#include "../src/assets.h"
#include "../src/campaign.h"
#include "../src/dsmap.h"
#include "../src/play.h"

static int fails = 0;
static void fail(int line, const char *fmt, ...)
{
    char msg[256];
    va_list ap;
    va_start(ap, fmt);
    SDL_vsnprintf(msg, sizeof msg, fmt, ap);
    va_end(ap);
    fails++;
    SDL_Log("FAIL line %d: %s", line, msg);
}
#define CHECK(c, ...) do { if (!(c)) fail(__LINE__, __VA_ARGS__); } while (0)

static GameAssets ga;
static Campaign camps[NUM_CAMPAIGNS];
static Screen scr;
static const char *outdir;
static uint8_t ds[0x10000], ds2[0x10000];

static void save(const Play *p, const char *name)
{
    static uint32_t rgb[SCREEN_W * SCREEN_H];
    play_draw(p, &scr, MODE_CGA);
    screen_to_xrgb(&scr, rgb);
    SDL_Surface *surf = SDL_CreateSurfaceFrom(SCREEN_W, SCREEN_H, SDL_PIXELFORMAT_XRGB8888, rgb, SCREEN_W * 4);
    char path[1200];
    SDL_snprintf(path, sizeof path, "%s/%s.bmp", outdir, name);
    CHECK(surf && SDL_SaveBMP(surf, path), "can't write %s", path);
    SDL_DestroySurface(surf);
}

static void settle(Play *p) { for (int n = 0; n < 5000 && play_busy(p); n++) play_frame(p, 0); }

int main(int argc, char **argv)
{
    if (argc < 3) { SDL_Log("usage: test_saved GAME-FOLDER OUT-FOLDER [ORIGINAL-SS]"); return 2; }
    outdir = argv[2];
    char err[256];
    if (!assets_load(&ga, argv[1], err, sizeof err)) { SDL_Log("assets: %s", err); return 2; }
    campaigns_load(camps, argv[1]);
    static Play p, q;
    static Campaign c, qc;

    /* the original's SS */
    if (argc > 3) {
        size_t n = 0;
        uint8_t *d = SDL_LoadFile(argv[3], &n);
        CHECK(d && n == 0x28CC, "%s: %zu bytes", argv[3], n);
        if (d && n == 0x28CC) {
            SDL_memset(ds, 0, sizeof ds);
            SDL_memcpy(ds + 0x3F39, d, n);
            CHECK(play_load_ds(&p, &c, &ga, ds, 5, 0xBB8), "play_load_ds");
            CHECK(!SDL_strcmp(c.ships[0].name, camps[0].ships[0].name), "ship 0 '%s'", c.ships[0].name);
            play_resume(&p);
            settle(&p);
            play_frame(&p, 0);
            save(&p, "ss_original_resumed");
            SDL_Log("original SS: view %d, marker %d,%d, time %d, opponent %d", p.view_top, p.cx, p.cy, p.time_div, c.opponent);
            for (int i = 0; i < 3000 && p.result == PLAY_RUNNING; i++) play_frame(&p, 0);
            save(&p, "ss_original_played");
        }
        SDL_free(d);
    }

    /* the remake's: written, read back, written again: the same bytes */
    for (int k = 0; k < NUM_CAMPAIGNS; k++) {
        c = camps[k];
        c.opponent = k % 6;
        play_start(&p, &c, &ga, (unsigned)(k + 3));
        settle(&p);
        for (int i = 0; i < 400 + 97 * k && p.result == PLAY_RUNNING; i++) play_frame(&p, 0);
        if (p.mode != PM_NORMAL && p.mode != PM_SQUAD) continue;
        SDL_memset(ds, 0, sizeof ds);
        play_save_ds(&p, ds);
        CHECK(play_load_ds(&q, &qc, &ga, ds, p.rnd, p.rnd_ptr), "S%d: load", k);
        SDL_memset(ds2, 0, sizeof ds2);
        play_save_ds(&q, ds2);
        int diff = 0;
        for (int a = 0x3F39; a < 0x3F39 + 0x28CC; a++) {
            if (a >= 0x6316 && a < 0x6316 + 80) continue;   /* the paths' ends: forgotten on loading, as WAS.EXE does */
            if (ds[a] != ds2[a] && diff++ < 6) SDL_Log("S%d: DS:%04X %02X %02X", k, a, ds[a], ds2[a]);
        }
        CHECK(diff == 0, "S%d: %d bytes differ after reading back", k, diff);
        CHECK(qc.opponent == c.opponent && !SDL_strcmp(qc.ships[5].name, c.ships[5].name), "S%d: campaign", k);
        play_resume(&q);
        settle(&q);
        if (k == 1) {                        /* for the original: SS_remake, and the map it should show */
            char path[1200];
            SDL_snprintf(path, sizeof path, "%s/SS_remake", outdir);
            CHECK(SDL_SaveFile(path, ds + 0x3F39, 0x28CC), "can't write %s", path);
            play_frame(&q, 0);
            save(&q, "ss_remake_resumed");
        }
        for (int i = 0; i < 2000 && q.result == PLAY_RUNNING; i++) play_frame(&q, 0);
    }
    play_draw(&q, &scr, MODE_CGA);

    /* damaged files */
    SDL_Log("damaged files");
    uint32_t r = 12345;
    for (int round = 0; round < 3000; round++) {
        c = camps[round % NUM_CAMPAIGNS];
        if (round % 50 == 0) { play_start(&p, &c, &ga, (unsigned)round); settle(&p); }
        SDL_memset(ds, 0, sizeof ds);
        play_save_ds(&p, ds);
        int hits = 1 + round % 200;
        for (int h = 0; h < hits; h++) {
            r = r * 1103515245u + 12345u;
            int a = 0x3F39 + (int)((r >> 8) % 0x28CC);
            r = r * 1103515245u + 12345u;
            ds[a] = (uint8_t)(r >> 16);
        }
        if (!play_load_ds(&q, &qc, &ga, ds, 1, 0xBB8)) continue;
        play_resume(&q);
        for (int i = 0; i < 150 && q.result == PLAY_RUNNING; i++) play_frame(&q, (int)(r >> 20) % 0x52);
        play_draw(&q, &scr, MODE_CGA);
    }

    assets_free(&ga);
    SDL_Log("%s (%d failures)", fails ? "FAILED" : "passed", fails);
    return fails ? 1 : 0;
}
