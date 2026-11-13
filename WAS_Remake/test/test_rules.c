/*
 * The Ancient Art of War at Sea (1987) - remake
 * Nov 2026 using Claude Opus 5.5
 * "Guided" by Brett Hallen
 * Original game by Dave Murry & Barry Murry
 * Published by Brøderbund Software Inc.
 *
 * test/test_rules.c: campaign play, step 3 (the rules), scripted as it was
 * done in the original in DOSBox (WAS_Analysis/tools/dosbox). Each scene
 * writes its screen as a BMP; the ones named in the comments matched the
 * DOSBox screenshots pixel for pixel (compare_shots.py), except where the
 * original's enemy, which is step 5, had moved.
 *
 *  r1-r4  The Bane of Oldain: M at F.D.Roosevelt (3,185), the tracing box,
 *         right and up until the land stops it, M, then sailing to the end.
 *  r5-r8  Join with Herbert Hoover, + , S, E; then Detach the two again
 *         (the new squadron goes one byte east).
 *  r9     Running the Gauntlet: a squadron sails north into the enemy:
 *         sighted, the encounter, the fight on the map.
 *  r10    Race for the Crown with your squadron put beside the enemy's
 *         crown (48,8): taken on tick 20, "You Won", a key ends the game.
 *  r11    The Bane of Oldain with only two enemy squadrons left: the
 *         opponent offers to surrender as the game begins.
 *  r12    The wind on (SCNF): "Wind is from the North" and the wind's face.
 *
 * Usage: test_rules GAME-FOLDER OUT-FOLDER
 */
#include <SDL3/SDL.h>
#include <stdarg.h>

#include "../src/assets.h"
#include "../src/campaign.h"
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
    play_draw(p, &scr, MODE_16);             /* the 16-colour screen draws too */
}

static const char *row(const Play *p, int r)
{
    static char b[41];
    SDL_memcpy(b, p->text[r], 40);
    b[40] = 0;
    for (int i = 39; i >= 0 && b[i] == ' '; i--) b[i] = 0;
    return b;
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

static int count_used(const Play *p, int side)
{
    int n = 0;
    for (int i = 0; i < MAX_SHIPS; i++) if (p->sq[i].used && p->sq[i].side == side) n++;
    return n;
}

int main(int argc, char **argv)
{
    if (argc < 3) { SDL_Log("usage: test_rules GAME-FOLDER OUT-FOLDER"); return 2; }
    outdir = argv[2];
    static GameAssets ga;
    static Campaign cs[NUM_CAMPAIGNS], c;
    static Play p;
    char err[256];
    if (!assets_load(&ga, argv[1], err, sizeof err)) { SDL_Log("load: %s", err); return 1; }
    CHECK(ga.have_so2, "SO2: the opponents");
    CHECK(campaigns_load(cs, argv[1]) == NUM_CAMPAIGNS, "campaigns");

    /* every campaign starts; none is over at once (Race for the Crown has no flagships) */
    for (int i = 0; i < NUM_CAMPAIGNS; i++) {
        play_start(&p, &cs[i], &ga, 77);
        CHECK(p.mode == PM_NORMAL || p.mode == PM_SQUAD, "S%d starts in mode %d", i, p.mode);
    }

    /* r1-r4: tracing a path and sailing it */
    play_start(&p, &cs[0], &ga, 77);
    CHECK(p.port_cell[0] == 17 && p.port_source[0] == 90, "S0's first port %d, its merchants from %d",   /* as WAS.EXE's ovl4:C07C (diff_ovl4 ports) */
          p.port_cell[0], p.port_source[0]);
    move_to(&p, 3, 186);
    CHECK(p.mode == PM_SQUAD && p.sel == 21, "F.D.Roosevelt selected");
    press(&p, PK_M);
    CHECK(p.mode == PM_TRACE && p.cx == 3 && p.cy == 185, "tracing from the squadron");
    CHECK(SDL_strcmp(row(&p, 23), "Trace path to destination and press") == 0, "trace prompt '%s'", row(&p, 23));
    CHECK(SDL_strcmp(row(&p, 24), " Mark Cancel") == 0, "trace line '%s'", row(&p, 24));
    save(&p, "r1_trace");
    hold(&p, PK_RIGHT, 80);
    CHECK(p.cx == 15, "the land stops the box at x 15, not %d", p.cx);
    hold(&p, PK_UP, 600);
    CHECK(p.cy == 170, "and going up at line pair 170, not %d", p.cy);
    save(&p, "r2_traced");
    press(&p, PK_M);
    const PSquad *q = &p.sq[21];
    CHECK(p.mode == PM_NORMAL && q->has_path && q->speed == 2 && q->dest == 171 * 80 + 15, "the path is marked");
    save(&p, "r3_marked");
    press(&p, PK_T); press(&p, PK_T); press(&p, PK_T);
    CHECK(p.time_div == 1, "time faster");
    for (int f = 0; f < 3000 && q->has_path; f++) play_frame(&p, 0);
    CHECK(!q->has_path && q->x == 15 && q->y == 170 && q->speed == 0 && q->dest == NO_DEST, "sailed to %d,%d", q->x, q->y);
    save(&p, "r4_arrived");
    for (int f = 0; f < 3000; f++) play_frame(&p, 0);
    CHECK(q->supplies < 20, "the squadron eats (%d)", q->supplies);

    /* r5-r8: Join, then Detach */
    play_start(&p, &cs[0], &ga, 77);
    move_to(&p, 3, 186);
    press(&p, PK_J);
    CHECK(p.mode == PM_DETACH && p.dj_pos == 3, "Join: Hoover marked");
    CHECK(SDL_strncmp(p.text[8] + 9, "SQUAD  1       SQUAD  2", 23) == 0, "the panel's title");
    CHECK(SDL_strcmp(row(&p, 23), "Move cursor to ship you wish to switch.") == 0, "Join prompt");
    save(&p, "r5_join");
    press(&p, PK_KP_PLUS);
    CHECK(p.dj_pos == 0, "+ to Roosevelt");
    press(&p, PK_S);
    CHECK(p.list[0] == -1 && p.dj_other[1] != -1 && p.dj_pos == 4, "Roosevelt switched");
    save(&p, "r6_switched");
    press(&p, PK_E);
    CHECK(!p.sq[21].used && p.mode == PM_NORMAL, "the empty squadron is gone");
    move_to(&p, 5, 190);
    CHECK(p.sel == 22 && SDL_strcmp(row(&p, 24), "Move Info Detach Join Practice") == 0, "two ships: Detach '%s'", row(&p, 24));
    press(&p, PK_D);
    CHECK(p.mode == PM_DETACH && p.dj_new, "Detach into an empty slot");
    int fs = p.free_slot;
    save(&p, "r7_detach");
    press(&p, PK_S);
    press(&p, PK_E);
    CHECK(fs >= 0 && p.sq[fs].used && p.sq[fs].x == 6 && p.sq[fs].y == 189, "the new squadron beside: %d,%d", p.sq[fs].x, p.sq[fs].y);
    save(&p, "r8_detached");

    /* r9: sighting, the encounter and the fight */
    play_start(&p, &cs[2], &ga, 77);
    move_to(&p, 33, 169);
    press(&p, PK_M);
    hold(&p, PK_UP, 40);
    press(&p, PK_M);
    bool sighted = false, met = false, fought = false, ended = false;
    int ours = count_used(&p, -1);
    for (int f = 0; f < 20000 && !ended; f++) {
        play_frame(&p, 0);
        for (int r = 21; r <= 22; r++) {
            if (SDL_strstr(p.text[r], "ALERT - ENEMY SIGHTED IN THE SOUTH")) sighted = true;
            if (SDL_strstr(p.text[r], "WARNING - ENCOUNTER IN THE SOUTH")) met = true;
        }
        if (p.sq[1].partner && p.sq[1].ctrl == -1) fought = true;
        if (fought && !p.sq[1].partner) ended = true;
        if (f == 600) save(&p, "r9_sighted");
    }
    CHECK(sighted && met && fought && ended, "sighted %d met %d fought %d over %d", sighted, met, fought, ended);
    CHECK(count_used(&p, -1) < ours || p.losses[1] > 0, "someone lost a ship");
    CHECK(p.losses[0] + p.losses[1] > 0, "losses counted");

    /* r10: the enemy's crown taken */
    c = cs[1];
    c.squadrons[0].x = 48;
    c.squadrons[0].y = 8;
    play_start(&p, &c, &ga, 77);
    int f = 0;   /* iterations, not counting those WAS.EXE spends working */
    for (int g = 0; g < 8000 && p.mode != PM_END; g++) {
        if (!play_busy(&p)) f++;
        play_frame(&p, 0);
    }
    CHECK(f == 1280 && p.result_code == 3 && p.end_quit, "taken on tick 20 (iteration %d), code %d", f, p.result_code);
    CHECK(SDL_strcmp(row(&p, 22), "BLACK CROWN CAPTURED IN THE NORTH") == 0, "crown message '%s'", row(&p, 22));
    for (int g = 0; g < 400 && p.end_frame < 14; g++) play_frame(&p, 0);
    CHECK(SDL_strcmp(row(&p, 24), "   You Won ...   to continue.") == 0, "won '%s'", row(&p, 24));
    save(&p, "r10_won");
    play_key(&p, PK_SPACE, false);
    for (int g = 0; g < 40 && p.result == PLAY_RUNNING; g++) play_frame(&p, 0);
    CHECK(p.result == PLAY_WON, "a key ends it");

    /* r11: the opponent offers to surrender at once */
    c = cs[0];
    for (int i = 0; i < MAX_SHIPS; i++) {
        if (c.squadrons[i].friendly || !c.squadrons[i].present || i == 2 || i == 3) continue;
        c.squadrons[i].present = false;
        for (int s = 0; s < MAX_SHIPS; s++) if (c.ships[s].squadron == i) c.ships[s].squadron = 255;
    }
    play_start(&p, &c, &ga, 4);
    CHECK(p.mode == PM_OFFER, "the offer");
    CHECK(SDL_strcmp(row(&p, 4), "   Your opponent offers to surrender.") == 0, "offer '%s'", row(&p, 4));
    save(&p, "r11_offer");
    press(&p, PK_N);
    CHECK(p.mode == PM_NORMAL && SDL_strcmp(row(&p, 24), "Time (slow)    Fleet  Pause  Surrender") == 0, "declined");
    play_start(&p, &c, &ga, 4);
    press(&p, PK_Y);
    CHECK(p.result == PLAY_WON && p.result_code == 1, "accepted");

    /* r12: the wind */
    ga.scnf[0x6865 - 0x684C] = 1;
    play_start(&p, &cs[0], &ga, 7);
    CHECK(p.wind_on && p.wind == 0 && SDL_strcmp(row(&p, 22), "Wind is from the North") == 0, "wind '%s'", row(&p, 22));
    save(&p, "r12_wind");
    ga.scnf[0x6865 - 0x684C] = 0;

    /* rough water: a frigate squadron left on rough water when the rule is "very risky" */
    c = cs[0];
    play_start(&p, &c, &ga, 9);
    int lost_before = p.losses[0] + p.losses[1];
    for (int i = 0; i < MAX_SHIPS; i++) {
        PSquad *s = &p.sq[i];
        if (!s->used || s->side != -1) continue;
        for (int cl = 0; cl < NUM_CELLS; cl++) {
            int t = p.map[cl];
            if (!((t >= 10 && t <= 12) || (t >= 16 && t <= 18))) continue;
            s->x = (cl % 10) * 8 + 3;
            s->y = (cl / 10) * 10 + 3;
            s->speed = 2;                    /* sailing: the risk is taken when the delays are set */
            break;
        }
    }
    p.rules[3] = 0;
    for (int k = 0; k < MAX_SHIPS; k++) if (p.ship[k].sq >= 0 && p.sq[p.ship[k].sq].side == -1) p.ship[k].type = 2;
    p.time_div = 1;
    for (int g = 0; g < 4000 && p.losses[0] + p.losses[1] == lost_before; g++) { settle(&p); play_frame(&p, 0); }
    /* ovl4_9d7e counts it by DS:5EE5 (-1 only while fighting): as the enemy's loss, as in WAS.EXE */
    CHECK(p.losses[1] > lost_before, "a frigate is lost in rough water");
    CHECK(SDL_strstr(p.text[22], "SUNK IN ROUGH WATERS") != NULL, "the message '%s'", row(&p, 22));

    assets_free(&ga);
    SDL_Log("%s (%d failures)", fails ? "FAILED" : "passed", fails);
    return fails ? 1 : 0;
}
