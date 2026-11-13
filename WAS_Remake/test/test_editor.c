/*
 * The Ancient Art of War at Sea (1987) - remake
 * Nov 2026 using Claude Opus 5.5
 * "Guided" by Brett Hallen
 * Original game by Dave Murry & Barry Murry
 * Published by Brøderbund Software Inc.
 *
 * test/test_editor.c: step 7, the editors walked through as in the original
 * in DOSBox: a new campaign's six steps (the map maker, the ships editor, the
 * opponent, the rules, the story, the title), the campaign played from the
 * data disk, each scene of the change menu, and the campaign removed. Checks
 * the files written (SITLES, the S file, as WAS.EXE writes them) and writes
 * each screen as a BMP in CGA and 16 colours (compare_shots.py compares them
 * with the DOSBox screenshots).
 *
 * Usage: test_editor GAME-FOLDER OUT-FOLDER
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
    SDL_snprintf(n, sizeof n, "e_%s", name);
    save_screen(n);
    if (ga.set[MODE_16].loaded) {
        menu_draw(m, &scr, MODE_16);
        SDL_snprintf(n, sizeof n, "e_%s_16", name);
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


/* a key tapped (let go before the next look at the keyboard: the ships
 * editor reads a key held down again) */
static void tap(Menu *m, int k)
{
    menu_key(m, k);
    run(m, 0.06);
    menu_key_up(m);
    run(m, 0.15);
}

/* typed on the keyboard (the editors' typing) */
static void type(Menu *m, const char *s)
{
    for (; *s; s++) {
        char c[2] = { *s, 0 };
        menu_text(m, c);
        run(m, 0.05);
    }
}

static void show(Menu *m, const char *what)
{
    SDL_Log("%s:", what);
    for (int r = 0; r < 25; r++) {
        const char *t = row_text(m, r);
        bool any = false;
        for (const char *c = t; *c; c++) any |= *c != '?';
        if (any) SDL_Log("  %2d %s", r, t);
    }
}
int main(int argc, char **argv)
{
    if (argc < 3) { SDL_Log("usage: test_editor GAME-FOLDER OUT-FOLDER"); return 2; }
    outdir = argv[2];
    char err[256];
    if (!assets_load(&ga, argv[1], err, sizeof err)) { SDL_Log("assets: %s", err); return 2; }
    CHECK(ga.have_menus, "no SO0-SO4/SITLES/SDITLES");
    campaigns_load(camps, argv[1]);
    char data[1200];
    SDL_snprintf(data, sizeof data, "%s/data/", outdir);
    rm_tree(data);
    SDL_CreateDirectory(data);
    Menu *m = menu_create(&ga, camps, data);
    CHECK(m != NULL, "menu_create");
    menu_start(m);
    to_title(m);
    key(m, PK_KP_PLUS);
    key(m, PK_ENTER);
    run(m, 4);
    show(m, "campaign menu");
    key(m, PK_ENTER);
    run(m, 4);
    show(m, "map maker intro");
    shot(m, "n1");
    key(m, PK_ENTER);
    run(m, 3);
    show(m, "map maker");
    shot(m, "n2");
    for (int i = 0; i < 5; i++) key(m, PK_RIGHT);
    for (int i = 0; i < 5; i++) key(m, PK_DOWN);
    for (int i = 0; i < 4; i++) key(m, PK_EQUALS);
    key(m, 0x30);                                       /* B: tile 74, a port */
    run(m, 1);
    shot(m, "n3");
    show(m, "map maker, a port");
    key(m, PK_ENTER);
    run(m, 3);
    show(m, "continue?");
    key(m, PK_Y);
    run(m, 5);
    show(m, "ships intro");
    shot(m, "s1");
    key(m, PK_SPACE);
    run(m, 2);
    show(m, "ships");
    shot(m, "s2");
    menu_shift(m, true);                                /* Shift and +: the bottom of the map, and back */
    tap(m, PK_EQUALS);
    run(m, 0.5);
    shot(m, "s2_bottom");
    tap(m, PK_MINUS);
    menu_shift(m, false);
    run(m, 0.5);
    key(m, PK_S);
    run(m, 2);
    show(m, "new squad");
    shot(m, "s3");
    type(m, "alpha");
    run(m, 0.5);
    shot(m, "s4");
    key(m, PK_ENTER);
    run(m, 2);
    show(m, "named");
    shot(m, "s5");
    key(m, PK_EQUALS);                                  /* the next ship */
    run(m, 1);
    type(m, "Bravo");
    key(m, PK_ENTER);
    run(m, 1);
    key(m, PK_T);                                       /* frigate */
    run(m, 1);
    key(m, PK_S);                                       /* its sails */
    run(m, 0.5);
    key(m, PK_MINUS);
    key(m, PK_MINUS);
    key(m, PK_ENTER);
    run(m, 1);
    show(m, "two ships");
    shot(m, "s6");
    key(m, PK_E);
    run(m, 2);
    show(m, "ended");
    shot(m, "s7");
    for (int i = 0; i < 20; i++) tap(m, PK_RIGHT);
    for (int i = 0; i < 10; i++) tap(m, PK_DOWN);
    key(m, PK_S);
    run(m, 2);
    type(m, "CHARLIE");
    key(m, PK_ENTER);
    run(m, 1);
    key(m, PK_MINUS);                                   /* to the squadron's line */
    run(m, 1);
    key(m, PK_F);                                       /* foe */
    run(m, 1);
    show(m, "foe");
    shot(m, "s8");
    key(m, PK_E);
    run(m, 2);
    tap(m, PK_LEFT);                                    /* off the squadron */
    run(m, 1);
    tap(m, PK_F);
    run(m, 1);
    show(m, "fleet");
    shot(m, "s9");
    key(m, PK_SPACE);
    run(m, 1);
    key(m, PK_ENTER);
    run(m, 2);
    show(m, "continue?");
    key(m, PK_Y);
    run(m, 5);
    show(m, "opponent intro");
    shot(m, "o1");
    key(m, PK_SPACE);
    run(m, 3);
    key(m, PK_EQUALS);
    run(m, 3);
    show(m, "opponent");
    shot(m, "o2");
    key(m, PK_ENTER);
    run(m, 8);
    show(m, "rules intro");
    shot(m, "r1");
    key(m, PK_SPACE);
    run(m, 1);
    key(m, PK_SPACE);
    run(m, 1);
    show(m, "rules");
    shot(m, "r2");
    key(m, PK_ENTER);
    run(m, 8);
    show(m, "story intro");
    shot(m, "t1");
    type(m, " ");
    run(m, 0.5);
    run(m, 1);
    type(m, "Once upon a time.");
    key(m, PK_ENTER);
    type(m, "The End");
    run(m, 1);
    shot(m, "t2");
    key(m, PK_ESC);
    run(m, 10);
    show(m, "title");
    shot(m, "t3");
    type(m, "Test Campaign");
    run(m, 1);
    shot(m, "t4");
    key(m, PK_ENTER);
    run(m, 5);
    show(m, "complete");
    shot(m, "t5");
    run(m, 5);
    show(m, "after");
    shot(m, "t6");
    to_title(m);

    /* the files, as WAS.EXE writes them */
    char disk[1300];
    SDL_snprintf(disk, sizeof disk, "%sData Disks/Data Disk 1/", data);
    size_t size = 0;
    uint8_t *t = assets_load_file(disk, "SITLES", &size);
    CHECK(t && size == 876, "SITLES %d bytes", (int)size);
    if (t) CHECK(!SDL_memcmp(t + 36 + 5 * 32, "  T\1est C\1ampaign", 17), "the new title in SITLES");
    SDL_free(t);
    for (int k = 0; k < 2; k++) {                       /* copies to look at beside the original's */
        const char *f = k ? "S1" : "SITLES";
        char to[1400];
        void *d = assets_load_file(disk, f, &size);
        SDL_snprintf(to, sizeof to, "%s/new_%s", outdir, f);
        if (d) SDL_SaveFile(to, d, size);
        SDL_free(d);
    }
    Campaign c;
    CHECK(campaign_load_file(&c, disk, "S1"), "S1");
    CHECK(c.map[5][5] == 74, "the port %d", c.map[5][5]);
    CHECK(c.num_friendly == 1 && c.num_enemy == 1, "squadrons %d %d", c.num_friendly, c.num_enemy);
    CHECK(!SDL_strcmp(c.ships[0].name, "ALPHA") && !SDL_strcmp(c.ships[1].name, "BRAVO") && !SDL_strcmp(c.ships[2].name, "CHARLIE"), "names");
    CHECK(c.ships[0].type == SHIP_FLAG && c.ships[1].type == SHIP_FRIGATE && c.ships[1].sails_damage == 2, "ships");
    CHECK(c.opponent == 1 && c.rules[0] == 80, "opponent %d rules %d", c.opponent, c.rules[0]);
    CHECK(!SDL_strncmp(c.story, "Once upon a time.", 17), "story '%.20s'", c.story);

    /* go to war with it, from the data disk */
    key(m, PK_ENTER);
    run(m, 6);
    for (int i = 0; i < 11; i++) key(m, PK_EQUALS);
    key(m, PK_ENTER);                                   /* ( data disk ) */
    run(m, 6);
    show(m, "data disk titles");
    shot(m, "w1");
    key(m, PK_EQUALS);
    key(m, PK_ENTER);
    run(m, 6);
    shot(m, "w2");
    key(m, PK_ENTER);                                   /* the story */
    run(m, 6);
    key(m, PK_ENTER);                                   /* the rules */
    run(m, 6);
    last = MENU_NONE;
    key(m, PK_ENTER);                                   /* the opponent */
    run(m, 6);
    CHECK(last == MENU_START_GAME, "start %d", (int)last);
    const Campaign *g = menu_game(m);
    CHECK(!SDL_strcmp(g->title, "Test Campaign") && g->opponent == 1 && g->num_friendly == 1, "the game '%s' %d", g->title, g->opponent);

    /* change it: each scene of the change menu */
    menu_title(m);
    to_title(m);
    key(m, PK_KP_PLUS);
    key(m, PK_ENTER);
    run(m, 4);
    key(m, PK_EQUALS);                                  /* Change a campaign */
    run(m, 2);
    key(m, PK_ENTER);
    run(m, 6);
    show(m, "change: titles");
    shot(m, "c1");
    key(m, PK_ENTER);
    run(m, 6);
    show(m, "change menu");
    shot(m, "c2");
    key(m, PK_ENTER);                                   /* RULEBOOK */
    run(m, 8);
    show(m, "rulebook");
    shot(m, "c3");
    key(m, PK_SPACE);
    key(m, PK_ENTER);
    run(m, 6);
    key(m, PK_EQUALS);                                  /* STORYBOOK */
    run(m, 2);
    key(m, PK_ENTER);
    run(m, 8);
    show(m, "storybook");
    shot(m, "c4");
    type(m, "Twice");
    key(m, PK_ESC);
    run(m, 2);
    show(m, "save story?");
    key(m, PK_Y);
    run(m, 6);
    key(m, PK_EQUALS);                                  /* MAP MAKER */
    run(m, 2);
    key(m, PK_ENTER);
    run(m, 4);
    show(m, "map maker");
    shot(m, "c5");
    key(m, PK_C);
    run(m, 1);
    key(m, PK_ENTER);
    run(m, 3);
    show(m, "save map?");
    key(m, PK_Y);
    run(m, 4);
    key(m, PK_EQUALS);
    run(m, 2);
    key(m, PK_EQUALS);                                  /* ENEMY */
    run(m, 2);
    key(m, PK_ENTER);
    run(m, 4);
    show(m, "enemy");
    shot(m, "c6");
    key(m, PK_EQUALS);
    run(m, 3);
    key(m, PK_ENTER);
    run(m, 4);
    key(m, PK_EQUALS);                                  /* FLEET */
    run(m, 2);
    key(m, PK_ENTER);
    run(m, 4);
    show(m, "fleet");
    shot(m, "c7");
    tap(m, PK_S);
    run(m, 2);
    type(m, "DELTA");
    key(m, PK_ENTER);
    run(m, 1);
    key(m, PK_E);
    run(m, 2);
    tap(m, PK_LEFT);
    run(m, 1);
    key(m, PK_ENTER);
    run(m, 2);
    show(m, "save fleet?");
    key(m, PK_Y);
    run(m, 5);
    key(m, PK_MINUS);
    run(m, 2);
    key(m, PK_MINUS);                                   /* EXIT */
    run(m, 2);
    show(m, "exit");
    key(m, PK_ENTER);
    run(m, 4);
    to_title(m);
    CHECK(campaign_load_file(&c, disk, "S1"), "S1");
    CHECK(c.rules[0] == 20, "rule changed %d", c.rules[0]);
    CHECK(!SDL_strncmp(c.story, "Twice", 5), "story changed '%.20s'", c.story);
    CHECK(c.map[0][0] == 3, "map changed %d", c.map[0][0]);
    CHECK(c.opponent == 2, "opponent changed %d", c.opponent);
    CHECK(!SDL_strcmp(c.ships[3].name, "DELTA") && c.num_friendly == 2, "fleet changed '%s' %d", c.ships[3].name, c.num_friendly);

    /* remove it */
    key(m, PK_KP_PLUS);
    key(m, PK_ENTER);
    run(m, 4);
    key(m, PK_EQUALS);
    run(m, 2);
    key(m, PK_EQUALS);                                  /* Remove a campaign */
    run(m, 2);
    key(m, PK_ENTER);
    run(m, 6);
    show(m, "remove: titles");
    shot(m, "x1");
    key(m, PK_ENTER);
    run(m, 1);
    show(m, "erase?");
    shot(m, "x2");
    key(m, PK_Y);
    run(m, 1);
    shot(m, "x3");
    run(m, 8);
    show(m, "removed");
    t = assets_load_file(disk, "SITLES", &size);
    CHECK(t && size == 876, "SITLES");
    if (t) {
        CHECK(!SDL_memcmp(t, "S0\0S3\0", 6) && !SDL_memcmp(t + 33, "S1\0", 3), "names moved %.36s", (const char *)t);
        bool blank = true;
        for (int i = 0; i < 32; i++) blank &= t[36 + 5 * 32 + i] == ' ';
        CHECK(blank, "title gone");
    }
    SDL_free(t);
    menu_destroy(m);
    SDL_Log("%s", fails ? "FAILED" : "all passed");
    return fails != 0;
}
