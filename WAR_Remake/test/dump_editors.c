/*
 * The Ancient Art of War (1984) - remake
 * Nov 2026 using Claude Opus 5.5
 * "Guided" by Brett Hallen
 * Original game by Dave Murry & Barry Murry
 * Published by Brøderbund Software Inc.
 *
 * dump_editors.c: the editors' screens as palette indexes for each card, as dump_cards.c does,
 * with the keys WAR_Analysis/tools/dosbox/weditors.sh sends WARET.EXE: a new campaign's map
 * maker and squads (eNN), Change a campaign (cNN), then the formation editor (fNN).
 * dump_editors GAME DATA OUT */
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
static void k(int key, int frames) { menu_key(app.menu, key); menu_frames(frames); }

int main(int argc, char **argv)
{
    if (argc < 4) { SDL_Log("dump_editors GAME DATA OUT"); return 2; }
    out = argv[3];
    sound_init_offline();
    if (!load_from(&app, argv[1], false)) return 1;
    app.mode = MODE_VGA;
    app.menu = menu_create(&app.ga, app.campaigns, argv[2]);
    menu_start(app.menu); app.in_menu = true;
    menu_frames(5);
    k(PK_ENTER, 3); k(PK_EQUALS, 3);
    k(PK_ENTER, 30);  dump("e01");              /* the campaign menu */
    k(PK_ENTER, 10);  dump("e03");              /* the map maker's first page */
    k(PK_ENTER, 4);   dump("e04");              /* the map */
    k(PK_ENTER, 10);  dump("e05");              /* done? */
    k(PK_Y, 4);       dump("e06");              /* the squads' first page */
    k(PK_SPACE, 4);   dump("e07");              /* the squads */
    k(PK_S, 4);       dump("e08");              /* Size */
    for (int i = 0; i < 3; i++) k(PK_K, 2);
    dump("e09");
    k(PK_E, 4);       dump("e10");              /* the squad's commands */
    k(PK_I, 4);       dump("e11");              /* Info */
    k(PK_E, 4);       dump("e12");
    k(PK_F, 4);       dump("e13");              /* Formation */
    /* Change a campaign: the scroll of campaigns, the editor's menu */
    menu_title(app.menu); menu_frames(2);
    k(PK_EQUALS, 3); k(PK_ENTER, 30);
    k(PK_EQUALS, 10); dump("c01");
    k(PK_ENTER, 30);  dump("c03");              /* the campaigns */
    k(PK_ENTER, 30);  dump("c04");              /* RULEBOOK */
    k(PK_EQUALS, 30); dump("c05");              /* STORYBOOK */
    k(PK_EQUALS, 30); dump("c06");              /* MAP MAKER */
    /* the formation editor */
    menu_title(app.menu); menu_frames(2);
    k(PK_EQUALS, 3); k(PK_ENTER, 30);
    for (int i = 0; i < 4; i++) k(PK_EQUALS, 10);
    dump("f01");                                /* Teach squads new formations */
    k(PK_ENTER, 4);   dump("f02");              /* formation 1 */
    k(PK_EQUALS, 4); k(PK_EQUALS, 4);          /* formation 3 (WARET.EXE in DOSBox reads the held = twice) */
    dump("f03");
    for (int i = 0; i < 3; i++) k(PK_EQUALS, 4);
    dump("f04");                                /* formation 6 */
    k(PK_M, 4);       dump("f05");              /* the pointer */
    k(PK_LEFT, 2); k(PK_LEFT, 2);
    dump("f06");
    return 0;
}
