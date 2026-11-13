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
    static const char *const SUFFIX[CARD_COUNT] = { "_cga", "_ega", "_tandy", "", "_herc" };
    char p[1200]; SDL_snprintf(p, sizeof p, "%s/%s%s.idx", out, name, SUFFIX[card]);
    SDL_IOStream *f = SDL_IOFromFile(p, "wb");
    SDL_WriteIO(f, app.screen.px, sizeof app.screen.px);
    SDL_WriteIO(f, app.screen.pal, sizeof app.screen.pal);
    SDL_CloseIO(f);
}

static void dump(const char *name) { dump1(name, CARD_CGA); dump1(name, CARD_EGA); dump1(name, CARD_TANDY); dump1(name, CARD_VGA); }

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
    begin_game(&app, &app.campaigns[0], 1);
    app.in_menu = false; app.playing = true;
    dump("map");
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
