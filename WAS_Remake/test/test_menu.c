/*
 * The Ancient Art of War at Sea (1987) - remake
 * Nov 2026 using Claude Opus 5.5
 * "Guided" by Brett Hallen
 * Original game by Dave Murry & Barry Murry
 * Published by Brøderbund Software Inc.
 *
 * test/test_menu.c: the menus, step 6, walked through as in the original in
 * DOSBox: "presented by", the title page, go to war (the titles, the story,
 * the rules with a rule changed, the opponents, the quote), make changes
 * (the campaign menu, Ye Olde Options Shoppe, the opening), the battle
 * report won and lost, saving a game and loading it again, and the demo.
 * Writes each screen as a BMP in CGA and in 16 colours (compare_shots.py
 * compares them with the DOSBox screenshots) and checks the timings that
 * were measured in DOSBox at 300 cycles.
 *
 * Usage: test_menu GAME-FOLDER OUT-FOLDER
 */
#include <SDL3/SDL.h>
#include <stdarg.h>

#include "../src/assets.h"
#include "../src/campaign.h"
#include "../src/dsmap.h"
#include "../src/menu.h"
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
static double now_s;                     /* the menus' time so far */
static MenuAction last;

static void save_screen(const char *name)
{
    static uint32_t rgb[SCREEN_W * SCREEN_H];
    char path[1200];
    SDL_snprintf(path, sizeof path, "%s/%s.bmp", outdir, name);
    screen_to_xrgb(&scr, rgb);
    SDL_Surface *surf = SDL_CreateSurfaceFrom(SCREEN_W, SCREEN_H, SDL_PIXELFORMAT_XRGB8888, rgb, SCREEN_W * 4);
    CHECK(surf && SDL_SaveBMP(surf, path), "can't write %s", path);
    SDL_DestroySurface(surf);
}

static void shot(Menu *m, const char *name)
{
    char n[200];
    menu_draw(m, &scr, MODE_CGA);
    SDL_snprintf(n, sizeof n, "m_%s", name);
    save_screen(n);
    if (ga.set[MODE_16].loaded) {
        menu_draw(m, &scr, MODE_16);
        SDL_snprintf(n, sizeof n, "m_%s_16", name);
        save_screen(n);
    }
}

/* the menus' clock, 60 times a second */
static void run(Menu *m, double seconds)
{
    for (double t = 0; t < seconds; t += 1.0 / 60) {
        menu_run(m, 1e6 / 60);
        now_s += 1.0 / 60;
        MenuAction a = menu_action(m);
        if (a != MENU_NONE) { last = a; return; }
    }
}

/* a key pressed for 0.1 s */
static void key(Menu *m, int k)
{
    menu_key(m, k);
    run(m, 0.1);
    menu_key_up(m);
    run(m, 0.1);
}

/* run until the title page waits for a key */
static double to_title(Menu *m)
{
    double t0 = now_s;
    for (int i = 0; i < 6000 && !menu_on_title(m); i++) run(m, 1.0 / 60);
    CHECK(menu_on_title(m), "no title page");
    return now_s - t0;
}

/* the text on a row of the CGA screen, read with the game's font */
static const char *row_text(Menu *m, int row)
{
    static char b[41];
    menu_draw(m, &scr, MODE_CGA);
    for (int c = 0; c < 40; c++) {
        b[c] = '?';
        for (int ch = 0; ch < FONT_COUNT; ch++) {
            bool same = true;
            for (int l = 0; l < 8 && same; l++)
                for (int x = 0; x < 8 && same; x++) {
                    bool on = (ga.font[ch][l] >> (7 - x)) & 1;
                    same = (scr.px[(row * 8 + l) * SCREEN_W + c * 8 + x] == 3) == on;
                }
            if (same) { b[c] = (char)(FONT_FIRST + ch); break; }
        }
    }
    b[40] = 0;
    for (int i = 39; i >= 0 && b[i] == ' '; i--) b[i] = 0;
    return b;
}

static SDL_EnumerationResult SDLCALL rm_found(void *ud, const char *dir, const char *name)
{
    (void)ud;
    char path[1400];
    SDL_snprintf(path, sizeof path, "%s%s", dir, name);
    SDL_PathInfo info;
    if (SDL_GetPathInfo(path, &info) && info.type == SDL_PATHTYPE_DIRECTORY) {
        SDL_strlcat(path, "/", sizeof path);
        SDL_EnumerateDirectory(path, rm_found, NULL);
    }
    SDL_RemovePath(path);
    return SDL_ENUM_CONTINUE;
}

static void rm_tree(const char *dir) { SDL_EnumerateDirectory(dir, rm_found, NULL); SDL_RemovePath(dir); }

int main(int argc, char **argv)
{
    if (argc < 3) { SDL_Log("usage: test_menu GAME-FOLDER OUT-FOLDER"); return 2; }
    outdir = argv[2];
    char err[256];
    if (!assets_load(&ga, argv[1], err, sizeof err)) { SDL_Log("assets: %s", err); return 2; }
    CHECK(ga.have_menus, "no SO2/SO3/SO4/SITLES/SDITLES");
    campaigns_load(camps, argv[1]);
    char data[1200];
    SDL_snprintf(data, sizeof data, "%s/data/", outdir);
    rm_tree(data);
    SDL_CreateDirectory(data);

    Menu *m = menu_create(&ga, camps, data);
    CHECK(m != NULL, "menu_create");

    /* "presented by": the tune lasts 6.75 s at an XT's tempo */
    menu_start(m);
    run(m, 1.0);
    shot(m, "presented");
    double t = to_title(m) + 1.0;
    SDL_Log("presented by: %.2f s", t);
    CHECK(t > 6.0 && t < 8.0, "presented by %.2f s", t);
    run(m, 0.5);
    shot(m, "title");
    CHECK(!SDL_strcmp(row_text(m, 24), "?? or ?? to change, ?? to select."), "title row 24 '%s'", row_text(m, 24));
    key(m, PK_KP_PLUS);
    run(m, 0.5);
    shot(m, "title_changes");
    key(m, PK_KP_MINUS);
    run(m, 0.5);

    /* go to war */
    key(m, PK_ENTER);
    double t0 = now_s;
    run(m, 6);
    shot(m, "titles");
    CHECK(!SDL_strcmp(row_text(m, 23), "Select a campaign title.  ??? to back up"), "titles row 23 '%s'", row_text(m, 23));
    key(m, PK_ENTER);
    run(m, 6);
    shot(m, "story");
    CHECK(!SDL_strcmp(row_text(m, 23), "This is the story of this campaign."), "story row 23 '%s'", row_text(m, 23));
    key(m, PK_KP_PLUS);
    key(m, PK_KP_PLUS);
    run(m, 0.5);
    shot(m, "story_scrolled");
    key(m, PK_ENTER);
    run(m, 6);
    shot(m, "rules");
    key(m, PK_KP_PLUS);
    run(m, 0.5);
    key(m, PK_SPACE);
    run(m, 0.5);
    shot(m, "rules_changed");
    key(m, PK_ENTER);
    run(m, 4);
    shot(m, "opponent");
    key(m, PK_KP_PLUS);
    run(m, 2);
    shot(m, "opponent2");
    key(m, PK_KP_MINUS);
    run(m, 2);
    last = MENU_NONE;
    key(m, PK_ENTER);
    run(m, 0.2);
    shot(m, "quote");
    double tq = now_s;
    run(m, 10);
    CHECK(last == MENU_START_GAME, "no game started (%d)", last);
    SDL_Log("go to war: %.1f s from Enter on the title page; the quote %.2f s", tq - t0, now_s - tq);
    const Campaign *g = menu_game(m);
    CHECK(g->loaded && !SDL_strcmp(g->file, "S0"), "game %s", g->file);
    CHECK(g->rules[1] == 5, "the second rule changed (rarely to often): %d", g->rules[1]);
    CHECK(g->opponent == 0, "opponent %d", g->opponent);

    /* make changes: the campaign menu, the Options Shoppe, the opening */
    menu_title(m);
    to_title(m);
    key(m, PK_KP_PLUS);
    run(m, 0.5);
    key(m, PK_ENTER);
    run(m, 3);
    shot(m, "campaign_menu");
    key(m, PK_KP_MINUS);
    run(m, 1.5);
    shot(m, "campaign_menu_shoppe");
    key(m, PK_ENTER);
    run(m, 3);
    shot(m, "shoppe");
    key(m, PK_KP_PLUS);
    run(m, 0.3);
    key(m, PK_SPACE);                     /* the wind: realistic */
    run(m, 0.3);
    shot(m, "shoppe_wind");
    CHECK(assets_scnf_word(&ga, 0x6865) == 1, "wind %d", assets_scnf_word(&ga, 0x6865));
    key(m, PK_ENTER);
    run(m, 3);
    shot(m, "campaign_menu_again");
    char path[1300];
    SDL_snprintf(path, sizeof path, "%sSCNF", data);
    size_t sz = 0;
    void *scnf = SDL_LoadFile(path, &sz);
    CHECK(scnf && sz == SCNF_SIZE, "SCNF not saved");
    SDL_free(scnf);
    key(m, PK_KP_MINUS);
    run(m, 1.5);
    key(m, PK_ENTER);                     /* The Ancient Art of War. */
    run(m, 12);
    shot(m, "opening");
    key(m, PK_SPACE);
    run(m, 3);
    key(m, PK_KP_MINUS);
    run(m, 1.5);
    key(m, PK_ENTER);                     /* Return to title page */
    to_title(m);

    /* the battle report */
    static Play p;
    static Campaign c;
    c = camps[0];
    play_start(&p, &c, &ga, 7);
    p.result_code = 0; p.losses[0] = 3; p.losses[1] = 12; p.captured[0] = 1; p.captured[1] = 2; p.losses[2] = 0; p.losses[3] = 1;
    menu_report(m, &p);
    run(m, 20);
    shot(m, "report_won");
    CHECK(!SDL_strcmp(row_text(m, 24), "      Press any key to continue."), "report row 24 '%s'", row_text(m, 24));
    key(m, PK_SPACE);
    to_title(m);
    for (int o = 0; o < 6; o++) {
        c.opponent = o;
        p.result_code = 10 + o % 5;
        menu_report(m, &p);
        run(m, 20);
        char n[32];
        SDL_snprintf(n, sizeof n, "report_lost%d", o);
        shot(m, n);
        key(m, PK_SPACE);
        to_title(m);
    }

    /* as in DOSBox: The Bane of Oldain, surrendered at once to Thor Foote */
    c = camps[0];
    play_start(&p, &c, &ga, 7);
    p.result_code = 11;
    menu_report(m, &p);
    run(m, 20);
    shot(m, "report_surrender");
    key(m, PK_SPACE);
    to_title(m);

    /* saving: the game above, on the data disk */
    c.opponent = 3;
    play_start(&p, &c, &ga, 9);
    for (int i = 0; i < 300; i++) play_frame(&p, 0);
    menu_save(m, &p);
    run(m, 3);
    shot(m, "saved");
    CHECK(!SDL_strcmp(row_text(m, 10), "   <C>ontinue with game or <Q>uit?"), "saved row 10 '%s'", row_text(m, 10));
    last = MENU_NONE;
    key(m, PK_C);
    run(m, 1);
    CHECK(last == MENU_CONTINUE, "no continue (%d)", last);
    SDL_snprintf(path, sizeof path, "%sSS", menu_data_dir(m));
    void *ss = SDL_LoadFile(path, &sz);
    CHECK(ss && sz == 0x28CC, "SS %zu bytes", sz);
    SDL_free(ss);
    /* again: the warning first */
    menu_save(m, &p);
    run(m, 2);
    shot(m, "save_warning");
    key(m, PK_C);
    run(m, 2);
    key(m, 0x10);                          /* Q: the remake goes to the title page */
    to_title(m);

    /* loading it: ( data disk ), Saved game */
    key(m, PK_ENTER);
    run(m, 6);
    for (int i = 0; i < 11; i++) { key(m, PK_KP_PLUS); }
    run(m, 1);
    shot(m, "titles_data_disk");
    key(m, PK_ENTER);
    run(m, 4);
    shot(m, "saved_game_or_new");
    CHECK(!SDL_strcmp(row_text(m, 24), "Saved game             New campaigns"), "row 24 '%s'", row_text(m, 24));
    last = MENU_NONE;
    key(m, PK_S);
    run(m, 3);
    CHECK(last == MENU_RESUME_GAME, "no resume (%d)", last);
    static Play q;
    static Campaign qc;
    CHECK(menu_resume(m, &q, &qc), "resume");
    CHECK(!SDL_strcmp(qc.title, camps[0].title), "title '%s'", qc.title);
    CHECK(qc.opponent == 3, "opponent %d", qc.opponent);
    int same = 0;
    for (int i = 0; i < MAX_SHIPS; i++)
        same += q.sq[i].used == p.sq[i].used && (!q.sq[i].used || (q.sq[i].x == p.sq[i].x && q.sq[i].y == p.sq[i].y));
    CHECK(same == MAX_SHIPS, "squadrons differ: %d same", same);
    for (int i = 0; i < 200; i++) play_frame(&q, 0);
    play_draw(&q, &scr, MODE_CGA);
    save_screen("m_resumed_map");

    /* the demo: the title page left alone */
    menu_title(m);
    to_title(m);
    last = MENU_NONE;
    double td = now_s;
    run(m, 30);
    CHECK(last == MENU_DEMO, "no demo (%d)", last);
    SDL_Log("demo after %.1f s on the title page", now_s - td);
    g = menu_game(m);
    CHECK(g->opponent == 2 && g->rules[5] == 80, "demo opponent %d, seen %d", g->opponent, g->rules[5]);
    play_start_demo(&p, g, &ga, 5, 0xBB8);
    CHECK(p.time_div == 1 && p.demo == 1 && p.demo_key == PK_KP_MINUS, "demo start");
    int scrolled = 0, zoomed = 0;
    for (int i = 0; i < 20000 && p.result == PLAY_RUNNING && !p.battle_req; i++) {
        int v = p.view_top;
        play_frame(&p, 0);
        scrolled += v != p.view_top;
    }
    zoomed = p.battle_req == 1;
    play_draw(&p, &scr, MODE_CGA);
    save_screen("m_demo_map");
    SDL_Log("demo: scrolled %d times, zoom %d", scrolled, zoomed);
    CHECK(scrolled > 10, "the demo didn't scroll");

    menu_destroy(m);
    assets_free(&ga);
    SDL_Log("%s (%d failures)", fails ? "FAILED" : "passed", fails);
    return fails ? 1 : 0;
}
