/*
 * The Ancient Art of War at Sea (1987) - remake
 * Nov 2026 using Claude Opus 5.5
 * "Guided" by Brett Hallen
 * Original game by Dave Murry & Barry Murry
 * Published by Brøderbund Software Inc.
 *
 * test/test_battle.c: the sea battle, step 4, scripted as it was done in
 * the original in DOSBox (WAS_Analysis/tools/dosbox). Each scene writes its
 * screen as a BMP.
 *
 *  b1-b3  The Bane of Oldain: Practice with F.D.Roosevelt (3,186): the
 *         overview, then View (the close view).
 *  z1-z3  The Bane of Oldain with GEO.WASHINGTON (squadron 16) put at
 *         44,145 in the lake and MACABRE's squadron (7) at 44,141 (in the
 *         DOSBox copy of S0; SCNF's fight delay 1000): the encounter, Zoom:
 *         the battle's start, then View.
 *  z4-z8  Back to the overview, Time faster until a ship grapples another
 *         ("Zoom Pass"), Zoom: the boarding (z5, "Attack Leave"), Attack
 *         (z6), the fight every 10 s (z7_t*) and the battle after it (z8).
 *         In DOSBox MACABRE grapples GEO.WASHINGTON; the deck's picture
 *         matches pixel for pixel, the men stand where the random numbers
 *         put them.
 *
 * Usage: test_battle GAME-FOLDER OUT-FOLDER
 */
#include <SDL3/SDL.h>
#include <stdarg.h>

#include "../src/assets.h"
#include "../src/battle.h"
#include "../src/campaign.h"
#include "../src/gfx.h"
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

static Screen scr;
static const char *outdir;

static void save_screen(const char *name)
{
    static uint32_t rgb[SCREEN_W * SCREEN_H];
    screen_to_xrgb(&scr, rgb);
    SDL_Surface *surf = SDL_CreateSurfaceFrom(SCREEN_W, SCREEN_H, SDL_PIXELFORMAT_XRGB8888, rgb, SCREEN_W * 4);
    char path[1200];
    SDL_snprintf(path, sizeof path, "%s/%s.bmp", outdir, name);
    CHECK(surf && SDL_SaveBMP(surf, path), "can't write %s", path);
    SDL_DestroySurface(surf);
}

/* each scene in CGA, and as the PCjr (WAS.EXE's registers) and the Tandy
 * version (WAST.EXE) show it: name_pcjr, name_tandy */
static void save_battle(const Battle *b, const char *name)
{
    char n2[200];
    battle_draw(b, &scr, MODE_CGA);
    save_screen(name);
    battle_draw(b, &scr, MODE_CGA);
    screen_apply_jr(&scr, &b->jr);
    SDL_snprintf(n2, sizeof n2, "%s_pcjr", name);
    save_screen(n2);
    if (b->ga->set[MODE_16].loaded) {
        battle_draw(b, &scr, MODE_16);
        SDL_snprintf(n2, sizeof n2, "%s_tandy", name);
        save_screen(n2);
    }
}

static void settle(Play *p) { for (int n = 0; n < 1000 && play_busy(p); n++) play_frame(p, 0); }
static void press(Play *p, int key) { settle(p); play_key(p, key, false); play_frame(p, 0); settle(p); }
static void hold(Play *p, int key, int n) { for (int i = 0; i < n; i++) { settle(p); play_frame(p, key); } }

static void move_to(Play *p, int x, int y)
{
    for (int n = 0; n < 900 && (p->cx != x || p->cy != y); n++) {
        int dx = (x > p->cx) - (x < p->cx), dy = (y > p->cy) - (y < p->cy);
        hold(p, PK_HOME + (dy + 1) * 4 + (dx + 1), 1);
    }
    CHECK(p->cx == x && p->cy == y, "marker at %d,%d not %d,%d", p->cx, p->cy, x, y);
    hold(p, 0, 1);
}

static void run_ms(Battle *b, int ms)
{
    for (int t = 0; t < ms && b->state == BATTLE_RUNNING; t += 16) battle_run(b, 16000);
}

static void key(Battle *b, int ch, int ms) { battle_key(b, ch, 0); run_ms(b, ms); }

int main(int argc, char **argv)
{
    if (argc < 3) { SDL_Log("usage: test_battle GAME-FOLDER OUT-FOLDER"); return 2; }
    outdir = argv[2];
    unsigned zseed = argc > 3 ? (unsigned)SDL_atoi(argv[3]) : 78;   /* the Zoom game's random numbers */
    static GameAssets ga;
    static Campaign cs[NUM_CAMPAIGNS];
    static Play p;
    static Battle b;
    char err[256];
    if (!assets_load(&ga, argv[1], err, sizeof err)) { SDL_Log("load: %s", err); return 1; }
    CHECK(campaigns_load(cs, argv[1]) == NUM_CAMPAIGNS, "campaigns");

    /* b1-b2: Practice */
    play_start(&p, &cs[0], &ga, 77);
    move_to(&p, 3, 186);
    CHECK(p.mode == PM_SQUAD && p.sel == 21, "F.D.Roosevelt selected");
    press(&p, PK_P);
    CHECK(p.battle_req == 2, "Practice asked for");
    CHECK(battle_start(&b, &p, true), "the battle starts");
    run_ms(&b, 3000);
    save_battle(&b, "b1_practice");
    key(&b, 'v', 2000);
    save_battle(&b, "b2_close");
    run_ms(&b, 4000);
    save_battle(&b, "b3_close_later");
    key(&b, 'l', 500);
    CHECK(b.state == BATTLE_OVER, "Leave ends the battle");
    battle_finish(&b);
    CHECK(p.mode == PM_NORMAL || p.mode == PM_SQUAD, "back on the map (mode %d)", p.mode);

    /* z1-z3: Zoom */
    static Campaign c0;
    c0 = cs[0];
    c0.squadrons[16].x = 44; c0.squadrons[16].y = 145;
    c0.squadrons[7].x = 44; c0.squadrons[7].y = 141;
    play_start(&p, &c0, &ga, zseed);
    move_to(&p, 44, 145);
    for (int f = 0; f < 400 && !(p.sq[16].partner && p.sq[16].ctrl > 0); f++) play_frame(&p, 0);
    CHECK(p.sq[16].partner == 8 && p.sq[16].ctrl > 0, "the encounter");
    press(&p, PK_Z);
    CHECK(p.battle_req == 1, "Zoom asked for");
    CHECK(battle_start(&b, &p, false), "the battle starts");
    run_ms(&b, 3000);
    save_battle(&b, "z1_zoom");
    run_ms(&b, 3000);
    save_battle(&b, "z2_zoom_later");
    key(&b, 'v', 2000);
    save_battle(&b, "z3_close");
    /* z4-z6: faster time until a ship grapples another ("Zoom Pass"), then
     * Zoom: the boarding */
    key(&b, 'v', 2000);
    key(&b, 't', 500);
    SDL_Log("view %d time %d", b.view, b.time);
    int t = 0;
    for (; t < 3000 && b.state == BATTLE_RUNNING && b.menu != 'B'; t++) run_ms(&b, 100);
    CHECK(b.menu == 'B', "a grapple after %d.%d s", t / 10, t % 10);
    SDL_Log("grapple after %d.%d s", t / 10, t % 10);
    run_ms(&b, 1000);
    save_battle(&b, "z4_grapple");
    key(&b, 'z', 3000);
    save_battle(&b, "z5_boarding");
    key(&b, 'a', 6000);
    save_battle(&b, "z6_attack");
    for (t = 0; t < 300 && b.state == BATTLE_RUNNING && b.menu == 'B'; t++) {
        run_ms(&b, 1000);
        if (t % 10 == 0) {
            char n[32];
            SDL_snprintf(n, sizeof n, "z7_t%d", t);
            save_battle(&b, n);
        }
    }
    SDL_Log("boarding over after %d s; owners %d %d %d %d %d %d", t, b.owner[0], b.owner[1], b.owner[2], b.owner[3], b.owner[4], b.owner[5]);
    run_ms(&b, 2000);
    save_battle(&b, "z8_after");

    /* x1-x3: an explosion (step 9, 14 Nov). WAS.EXE's chance of a hit blowing
     * a ship up is 1 in 128, so battle_test_blow_up makes the first broadside
     * blow up the enemy's ship 3, as the patched WAST.EXE did in DOSBox-X.
     * There, during the flashes (BIOS palette 0) colours 1-3 showed as 10, 12,
     * 14 and otherwise, and after it, as the battle's own 9, 6, 15. */
    if (ga.set[MODE_16].loaded) {
        static Battle x;   /* (b still holds the Zoom battle's fiber) */
        play_start(&p, &c0, &ga, zseed);
        move_to(&p, 44, 145);
        for (int f = 0; f < 400 && !(p.sq[16].partner && p.sq[16].ctrl > 0); f++) play_frame(&p, 0);
        press(&p, PK_Z);
        CHECK(battle_start(&x, &p, false), "the battle starts (explosion)");
        run_ms(&x, 3000);
        save_battle(&x, "x1_before");
        static const uint8_t OWN[4] = { 0, 9, 6, 15 }, P0[4] = { 0, 10, 12, 14 };
        CHECK(SDL_memcmp(x.tpal, OWN, 4) == 0, "the Tandy battle's colours 0, 9, 6, 15");
        battle_test_blow_up = 3;
        battle_key(&x, 'f', 0);
        int flash = 0, t2 = 0;
        for (; t2 < 20000 && x.state == BATTLE_RUNNING; t2 += 16) {
            battle_run(&x, 16000);
            if (x.palette == 0 && !flash) {
                flash = t2;
                save_battle(&x, "x2_flash");
                CHECK(SDL_memcmp(x.tpal, P0, 4) == 0, "the flash: 0, 10, 12, 14 (%d %d %d)", x.tpal[1], x.tpal[2], x.tpal[3]);
                battle_draw(&x, &scr, MODE_16);
                int n[16] = { 0 };
                for (int i = 0; i < SCREEN_W * SCREEN_H; i++) n[scr.px[i]]++;
                CHECK(n[0] + n[10] + n[12] + n[14] == SCREEN_W * SCREEN_H, "the flash shows only 0, 10, 12, 14");
                CHECK(n[10] > 20000 && n[9] == 0, "the sea light green in the flash");
            }
            if (flash && t2 > flash + 8000) break;
        }
        battle_test_blow_up = -1;
        CHECK(flash > 0, "an explosion");
        SDL_Log("explosion's first flash after %d ms", flash);
        save_battle(&x, "x3_after");
        CHECK(x.palette == 1 && SDL_memcmp(x.tpal, OWN, 4) == 0, "after it the battle's own colours (%d %d %d)",
              x.tpal[1], x.tpal[2], x.tpal[3]);
        key(&x, 'l', 500);
        SDL_Log("after Leave: state %d", x.state);
        for (int i = 0; i < 100 && x.state == BATTLE_RUNNING; i++) key(&x, 'y', 500);
        CHECK(x.state != BATTLE_RUNNING, "Leave ends the battle (explosion)");
        battle_finish(&x);
    }

    if (fails) SDL_Log("%d FAILED", fails);
    else SDL_Log("all passed");
    return fails != 0;
}
