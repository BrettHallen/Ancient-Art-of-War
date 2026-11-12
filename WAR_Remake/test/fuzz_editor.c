/*
 * The Ancient Art of War (1984) - remake
 * Nov 2026 using Claude Opus 5.5
 * "Guided" by Brett Hallen
 * Original game by Dave Murry & Barry Murry
 * Published by Brøderbund Software Inc.
 *
 * test/fuzz_editor.c: a test or debugging aid for the remake (see README.md here)
 */
/* random keys and text through the menus and editors (ASan) */
#include "../src/menu.c"
static GameAssets ga; static Campaign camps[NUM_CAMPAIGNS]; static Screen scr;
int main(int argc, char **argv)
{
    (void)argc; char err[256];
    sound_init_offline();
    assets_load(&ga, argv[2], err, sizeof err); campaigns_load(camps, argv[2]);
    Menu *m = menu_create(&ga, camps, argv[1]);
    static const int K[] = { PK_ENTER, PK_ESC, PK_UP, PK_DOWN, PK_LEFT, PK_RIGHT, PK_HOME, PK_END, PK_PGUP, PK_PGDN,
        PK_MINUS, PK_EQUALS, PK_KP_PLUS, PK_KP_MINUS, PK_SPACE, PK_BACKSPACE, PK_Y, PK_N, PK_A, PK_B, PK_C, PK_D, PK_E,
        PK_F, PK_G, PK_H, PK_I, PK_K, PK_L, PK_M, PK_O, PK_P, PK_Q, PK_R, PK_S, PK_T, PK_U, PK_W, PK_X };
    uint32_t r = (uint32_t)atoi(argv[3]);
    int states[64] = { 0 };
    menu_title(m);
    for (long i = 0; i < atol(argv[4]); i++) {
        r = r * 1103515245u + 12345u;
        int pick = (r >> 16) % 100;
        if (m->state == MS_TITLE && pick < 50) { menu_key(m, PK_DOWN); menu_key(m, PK_ENTER); }   /* into the campaign menu */
        else if (m->state == MS_CAMPAIGN_MENU && pick < 60) { int n = (r >> 8) % 5; if (n == 3) n = 4; for (int k = 0; k < n; k++) menu_key(m, PK_DOWN); menu_key(m, PK_ENTER); }
        else if (pick < 35) menu_key(m, K[(r >> 8) % (sizeof K / sizeof K[0])]);
        else if (pick < 50) { char c[2] = { (char)(0x20 + (r >> 9) % 95), 0 }; menu_text(m, c); }
        if (menu_wants_held(m)) menu_held(m, pick > 80 ? PK_HOME + (int)((r >> 4) % 11) : 0);
        menu_draw(m, &scr, (r >> 20) % 7 == 0 ? MODE_VGA : MODE_CGA);
        menu_frame(m);
        MenuAction a = menu_action(m);
        if (a == MENU_START_GAME || a == MENU_DEMO || a == MENU_RESUME_GAME || a == MENU_QUIT || a == MENU_CONTINUE) menu_title(m);
        states[m->state & 63]++;
    }
    for (int s = 0; s < 32; s++) if (states[s]) SDL_Log("state %2d: %d", s, states[s]);
    menu_destroy(m);
    return 0;
}
