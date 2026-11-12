/*
 * The Ancient Art of War (1984) - remake
 * Nov 2026 using Claude Opus 5.5
 * "Guided" by Brett Hallen
 * Original game by Dave Murry & Barry Murry
 * Published by Brøderbund Software Inc.
 *
 * test/test_cmenu.c: a test or debugging aid for the remake (see README.md here)
 */
#include "../src/menu.h"
#include <SDL3/SDL.h>
static GameAssets ga; static Campaign camps[NUM_CAMPAIGNS]; static Screen scr;
static Menu *m;
static void frames(int n) { for (int i = 0; i < n; i++) { menu_draw(m, &scr, MODE_CGA); menu_frame(m); } }
int main(int argc, char **argv) { (void)argc; char e[256]; assets_load(&ga, argv[1], e, 256); campaigns_load(camps, argv[1]);
    m = menu_create(&ga, camps, argv[2]); menu_title(m); frames(1);
    menu_key(m, PK_EQUALS); menu_key(m, PK_ENTER); frames(10);
    for (int rep = 0; rep < 2; rep++) {
        menu_key(m, PK_EQUALS);
        char a[64] = "", b[64] = "";
        for (int i = 0; i < 24; i++) {
            menu_draw(m, &scr, MODE_CGA);
            int top = 0; for (int x = 0; x < 320; x++) top += scr.px[12 * 320 + x] == 3;   /* line 12: inside the box, below the text */
            a[i] = top > 0 ? 'B' : '.';
            b[i] = scr.px[60 * 320 + 109] == 3 && scr.px[27 * 320 + 150] == 3 ? 'F' : '.';
            menu_frame(m);
        }
        SDL_Log("top rows %s", a); SDL_Log("frame    %s", b);
    }
    return 0; }
