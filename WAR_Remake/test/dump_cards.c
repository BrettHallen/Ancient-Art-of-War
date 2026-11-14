/*
 * The Ancient Art of War (1984) - remake
 * Nov 2026 using Claude Opus 5.5
 * "Guided" by Brett Hallen
 * Original game by Dave Murry & Barry Murry
 * Published by Brøderbund Software Inc.
 *
 * dump_cards.c: the app's screens as palette indexes for each card (NAME_cga.idx, NAME_ega.idx,
 * NAME_tandy.idx and NAME.idx for VGA: 64000 bytes, then 16 x RGB), to compare with WARET.EXE's
 * screens in DOSBox-X (WAR_Analysis/tools/dosbox: egacmp.py, vgacmp.py).
 * dump_cards GAME DATA OUT [S-FILE] */
#define main app_main
#include "../src/main.c"
#include "../src/savegame.h"
#undef main

static App app;
static const char *out;

static void dump1(const char *name, VideoCard card)
{
    set_card(&app, card);
    draw_screen(&app);
    static const char *const SUFFIX[CARD_COUNT] = { "_cga", "_comp", "_comp2", "_ega", "_tandy", "", "_herc" };
    char p[1200]; SDL_snprintf(p, sizeof p, "%s/%s%s.idx", out, name, SUFFIX[card]);
    SDL_IOStream *f = SDL_IOFromFile(p, "wb");
    SDL_WriteIO(f, app.screen.px, sizeof app.screen.px);
    SDL_WriteIO(f, app.screen.pal, sizeof app.screen.pal);
    SDL_CloseIO(f);
    if (card == CARD_COMPOSITE || card == CARD_COMPOSITE2) {   /* the 640 x 200 picture a TV shows */
        to_pixels(&app, &app.screen);
        SDL_snprintf(p, sizeof p, "%s/%s%s.rgb", out, name, SUFFIX[card]);
        f = SDL_IOFromFile(p, "wb");
        SDL_WriteIO(f, app.rgb, COMP_W * SCREEN_H * 4);
        SDL_CloseIO(f);
    }
}

static void dump(const char *name) { for (int c = 0; c < CARD_HERC; c++) dump1(name, (VideoCard)c); }

static void menu_frames(int n) { for (int i = 0; i < n; i++) { draw_screen(&app); menu_step(&app); } }

int main(int argc, char **argv)
{
    if (argc < 4) { SDL_Log("dump_vga GAME DATA OUT [S]"); return 2; }
    out = argv[3];
    sound_init_offline();
    if (!load_from(&app, argv[1], false)) return 1;
    app.mode = MODE_VGA;
    app.menu = menu_create(&app.ga, app.campaigns, argv[2]);
    menu_start(app.menu); app.in_menu = true;
    menu_frames(5);                                     dump("presented");
    menu_key(app.menu, PK_ENTER); menu_frames(3);       dump("title");
    menu_key(app.menu, PK_EQUALS); menu_frames(3);
    menu_key(app.menu, PK_ENTER); menu_frames(10);      dump("cmenu");
    for (int k = 0; k < 12; k++) { char n[16]; SDL_snprintf(n, sizeof n, "cmenu%02d", k); menu_frames(2); dump(n); }
    for (int i = 0; i < 5; i++) { menu_key(app.menu, PK_EQUALS); menu_frames(3); }
    menu_key(app.menu, PK_ENTER); menu_frames(10);      dump("shoppe");
    menu_key(app.menu, PK_ENTER); menu_frames(5);
    menu_key(app.menu, PK_ENTER); menu_frames(80);      dump("titles");
    menu_key(app.menu, PK_ENTER); menu_frames(80);      dump("story");
    menu_key(app.menu, PK_ENTER); menu_frames(80);      dump("rules");
    menu_key(app.menu, PK_ENTER); menu_frames(80);      dump("opponent");
    menu_key(app.menu, PK_ENTER); menu_frames(2);       dump("quote");
    begin_game(&app, &app.campaigns[0], 1);
    app.in_menu = false; app.playing = true;
    dump("map");
    /* the boxes on the map, as WAR_Analysis/tools/dosbox/wboxes.sh does with WARET.EXE: the cursor onto
     * your squad 16, Info, Size, Formation, Detach, a traced path, View, Time, Surrender, the report */
    {
        Play *p = &app.play;
        p->cx = 40; p->cy = 191;
        static const struct { int key; const char *shot; int frames; } K[] = {
            { PK_RIGHT, "k01", 2 }, { PK_I, "k02", 2 }, { PK_E, NULL, 2 }, { PK_S, "k03", 2 }, { PK_E, NULL, 2 },
            { PK_F, "k04", 2 }, { PK_E, NULL, 2 }, { PK_D, "k05", 2 }, { PK_E, NULL, 2 }, { PK_M, "k06", 2 },
            { PK_UP, NULL, 1 }, { PK_UP, "k07", 2 }, { PK_M, "k08", 2 }, { PK_V, "k09", 40 }, { PK_E, NULL, 2 },
            { PK_T, "k10", 2 }, { PK_UP, NULL, 1 }, { PK_UP, NULL, 1 }, { PK_UP, NULL, 1 }, { PK_UP, NULL, 1 },
            { PK_UP, NULL, 1 }, { PK_UP, NULL, 1 }, { PK_UP, NULL, 1 }, { PK_UP, NULL, 1 }, { PK_UP, "k11", 2 },
            { PK_S, "k12", 2 },
            { PK_Y, "k13", 40 }, { PK_ENTER, NULL, 2 },
        };
        for (size_t k = 0; k < SDL_arraysize(K); k++) {
            play_key(p, K[k].key, false);
            for (int f = 0; f < K[k].frames && app.playing; f++) play_step(&app);
            if (K[k].shot) dump(K[k].shot);
        }
        for (int f = 0; f < 120 && app.playing; f++) play_step(&app);
        for (int f = 0; f < 200; f++) { draw_screen(&app); menu_step(&app); }
        dump("k14");
        app.in_menu = false;
    }
    if (argc > 4) {
        size_t n; uint8_t *sv = SDL_LoadFile(argv[4], &n);
        static Campaign g;
        if (sv && savegame_import(&app.play, &g, &app.ga, sv, n, 7)) {
            app.game = g; app.play.c = &app.game; app.play.fight_delay = 400;
            play_resume(&app.play);
            dump("saved_map");
            int sq = app.play.sel;
            for (int f = 0; f < 400 && !(app.play.sq[sq].fighting && app.play.sq[sq].clock > 0); f++) play_frame(&app.play, 0);
            if (battle_start(&app.battle, &app.play, sq)) {
                app.in_battle = true;
                battle_frame(&app.battle);
                dump("saved_battle");
            }
        }
    }
    return 0;
}
