/*
 * The Ancient Art of War (1984) - remake
 * Nov 2026 using Claude Opus 5.5
 * "Guided" by Brett Hallen
 * Original game by Dave Murry & Barry Murry
 * Published by Brøderbund Software Inc.
 *
 * test/test_herc.c: a test or debugging aid for the remake (see README.md here)
 */
/* Hercules (WAR HERC): the conversion of the CGA screen to the card's 720x348,
 * Tab's three modes, and the app's screens (title page, titles, the map, a
 * battle) drawn as on a Hercules card. Every CGA screen must use only colours
 * 0-3, since the card shows each 2-bit pixel as two mono pixels.
 * Writes the Hercules screens to OUT for comparing with DOSBox (machine=hercules).
 * test_herc GAME DATA OUT [S-FILE] */
#define main app_main
#include "../src/main.c"
#include "../src/savegame.h"
#undef main

static App app;
static int fails;
#define CHECK(c, ...) do { if (c) SDL_Log("ok   " __VA_ARGS__); else { SDL_Log("FAIL " __VA_ARGS__); fails++; } } while (0)

static bool cga_only(void)
{
    for (int i = 0; i < SCREEN_W * SCREEN_H; i++) if (app.screen.px[i] > 3) return false;
    return true;
}

static void save(const char *out, const char *name)
{
    int w = app.herc ? HERC_W : SCREEN_W, h = app.herc ? HERC_H : SCREEN_H;
    SDL_Surface *s = SDL_CreateSurfaceFrom(w, h, SDL_PIXELFORMAT_XRGB8888, app.rgb, w * 4);
    char p[1200]; SDL_snprintf(p, sizeof p, "%s/%s.bmp", out, name);
    SDL_SaveBMP(s, p); SDL_DestroySurface(s);
}

/* draw the app's screen in CGA (colours 0-3 only?) and then as Hercules */
static void shot(const char *out, const char *name)
{
    app.herc = false; app.mode = MODE_CGA;
    draw_screen(&app);
    CHECK(cga_only(), "%s: the CGA screen uses colours 0-3 only", name);
    app.herc = true;
    draw_screen(&app);
    save(out, name);
}

static void menu_frames(int n) { for (int i = 0; i < n; i++) { draw_screen(&app); menu_step(&app); } }

int main(int argc, char **argv)
{
    if (argc < 4) { SDL_Log("test_herc GAME DATA OUT"); return 2; }
    const char *out = argv[3];
    sound_init_offline();

    /* the conversion: each CGA pixel is two mono pixels, the screen at 32,72 of 720x348 */
    {
        static Screen s; static uint32_t h[HERC_W * HERC_H];
        screen_set_palette(&s, CGA_PALETTE, 4);
        screen_clear(&s, 0);
        s.px[0] = 1; s.px[1] = 2; s.px[2] = 3;
        s.px[SCREEN_W * SCREEN_H - 1] = 3;
        screen_to_herc(&s, h);
        const uint32_t L = 0xFF000000u | HERC_LIT, D = 0xFF000000u;
        CHECK(h[72 * HERC_W + 32] == D && h[72 * HERC_W + 33] == L, "colour 1: dark, lit");
        CHECK(h[72 * HERC_W + 34] == L && h[72 * HERC_W + 35] == D, "colour 2: lit, dark");
        CHECK(h[72 * HERC_W + 36] == L && h[72 * HERC_W + 37] == L, "colour 3: lit, lit");
        CHECK(h[271 * HERC_W + 670] == L && h[271 * HERC_W + 671] == L && h[271 * HERC_W + 672] == D && h[272 * HERC_W + 671] == D,
              "the last pixel at 670-671, line 271");
        CHECK(h[0] == D && h[71 * HERC_W + 33] == D && h[72 * HERC_W + 31] == D, "black round it");
        screen_set_palette(&s, CGA_PALETTE_0, 4);   /* the palette makes no difference */
        static uint32_t h2[HERC_W * HERC_H];
        screen_to_herc(&s, h2);
        CHECK(SDL_memcmp(h, h2, sizeof h) == 0, "the palette makes no difference");
    }

    if (!load_from(&app, argv[1], false)) { SDL_Log("no game files"); return 1; }

    /* Tab: CGA, VGA, Hercules, CGA */
    app.mode = MODE_CGA; app.herc = false;
    next_mode(&app); CHECK(app.mode == MODE_VGA && !app.herc, "Tab: VGA");
    next_mode(&app); CHECK(app.mode == MODE_CGA && app.herc, "Tab twice: Hercules");
    CHECK(SDL_strcmp(display_name(&app), "Hercules") == 0, "its name");
    next_mode(&app); CHECK(app.mode == MODE_CGA && !app.herc, "Tab three times: CGA");
    app.ga.set[MODE_VGA].loaded = false;              /* without WARET.DAT: CGA, Hercules */
    next_mode(&app); CHECK(app.herc, "no VGA: Tab goes to Hercules");
    next_mode(&app); CHECK(!app.herc && app.mode == MODE_CGA, "and back to CGA");
    app.ga.set[MODE_VGA].loaded = true;

    /* the menus */
    app.menu = menu_create(&app.ga, app.campaigns, argv[2]);
    menu_start(app.menu); app.in_menu = true;
    app.herc = true;
    menu_frames(5);
    shot(out, "herc_presented");
    menu_key(app.menu, PK_ENTER); menu_frames(3);      /* the title page */
    shot(out, "herc_title");
    menu_key(app.menu, PK_EQUALS); menu_frames(3);     /* make changes: the campaign menu */
    menu_key(app.menu, PK_ENTER); menu_frames(10);
    shot(out, "herc_cmenu");
    for (int i = 0; i < 5; i++) { menu_key(app.menu, PK_EQUALS); menu_frames(3); }
    shot(out, "herc_cmenu5");
    menu_key(app.menu, PK_ENTER); menu_frames(10);     /* Ye Olde Options Shoppe */
    shot(out, "herc_shoppe");
    menu_key(app.menu, PK_ENTER); menu_frames(5);      /* back to the title page, "go to war" */
    menu_key(app.menu, PK_ENTER); menu_frames(80);     /* go to war: the titles */
    shot(out, "herc_titles");
    menu_key(app.menu, PK_ENTER); menu_frames(80);     /* the story */
    shot(out, "herc_story");
    menu_key(app.menu, PK_ENTER); menu_frames(80);     /* the rules */
    shot(out, "herc_rules");
    menu_key(app.menu, PK_ENTER); menu_frames(80);     /* the opponent */
    shot(out, "herc_opponent");

    /* the first campaign, as it starts */
    begin_game(&app, &app.campaigns[0], 1);
    shot(out, "herc_map");

    /* a battle: one of your squads beside an enemy squad, Zoom */
    {
        Play *p = &app.play;
        PSquad *a = &p->sq[16], *e = &p->sq[6];
        a->archers = 10; a->knights = 2; a->barbarians = 2; a->spies = 0;
        e->archers = 0; e->knights = 7; e->barbarians = 7;
        a->men = a->archers + a->knights + a->barbarians; e->men = e->knights + e->barbarians;
        a->x = e->x + 1; a->y = e->y + 3; p->time_div = 1;
        for (int f = 0; f < 50 && !a->fighting; f++) play_frame(p, 0);
        CHECK(a->fighting, "an encounter");
        if (a->fighting && battle_start(&app.battle, p, 16)) {
            app.in_battle = true;
            for (int f = 0; f < 3; f++) battle_frame(&app.battle);
            shot(out, "herc_battle");
            battle_key(&app.battle, PK_A);
            for (int f = 0; f < 40; f++) battle_frame(&app.battle);
            shot(out, "herc_battle_40");
        } else CHECK(false, "the battle screen");
    }

    /* an original saved game (optional 4th argument), Zoom at once: the battle screen as the original shows it */
    if (argc > 4) {
        size_t n; uint8_t *sv = SDL_LoadFile(argv[4], &n);
        static Campaign g;
        if (sv && savegame_import(&app.play, &g, &app.ga, sv, n, 7)) {
            app.game = g; app.play.c = &app.game;
            app.play.fight_delay = 400;
            play_resume(&app.play);
            int sq = app.play.sel;
            for (int f = 0; f < 400 && !(app.play.sq[sq].fighting && app.play.sq[sq].clock > 0); f++) play_frame(&app.play, 0);
            if (battle_start(&app.battle, &app.play, sq)) {
                app.playing = true; app.in_battle = true; app.in_menu = false;
                battle_frame(&app.battle);
                shot(out, "herc_saved_battle");
            } else CHECK(false, "the saved game's battle");
        } else CHECK(false, "import %s", argv[4]);
        SDL_free(sv);
    }

    /* the asset viewer's screens */
    app.playing = false; app.in_battle = false; app.in_menu = false; app.caption = false;
    for (int i = 0; i < NUM_SCREENS; i++) {
        app.index = i;
        app.herc = false; app.mode = MODE_CGA;
        draw_screen(&app);
        if (!cga_only()) { CHECK(false, "viewer screen %d uses colours above 3", i); break; }
        if (i == NUM_SCREENS - 1) CHECK(true, "every viewer screen in CGA uses colours 0-3 only");
    }

    menu_destroy(app.menu);
    assets_free(&app.ga);
    SDL_Log("%d failed", fails);
    return fails != 0;
}
