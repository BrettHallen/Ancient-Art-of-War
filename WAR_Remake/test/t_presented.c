/*
 * The Ancient Art of War (1984) - remake
 * Nov 2026 using Claude Opus 5.5
 * "Guided" by Brett Hallen
 * Original game by Dave Murry & Barry Murry
 * Published by Brøderbund Software Inc.
 *
 * test/t_presented.c: a test or debugging aid for the remake (see README.md here)
 */
#include "../src/menu.c"
static GameAssets ga; static Campaign camps[NUM_CAMPAIGNS]; static Screen scr;
int main(int argc, char **argv)
{
    (void)argc; char err[256];
    assets_load(&ga, argv[1], err, sizeof err); campaigns_load(camps, argv[1]);
    for (int pass = 0; pass < 2; pass++) {
        sound_init_offline();
        Menu *m = menu_create(&ga, camps, "/tmp/dd4/");
        sound_setting(pass ? SOUND_NO_MUSIC : SOUND_ON);
        menu_start(m);
        float buf[44100 / MENU_HZ];
        int f = 0;
        for (; f < 400 && m->state == MS_PRESENTED; f++) { menu_draw(m, &scr, MODE_CGA); menu_frame(m); sound_render(buf, 44100 / MENU_HZ); }
        SDL_Log("%s: title page after %.2f s", pass ? "no music" : "music", f / (double)MENU_HZ);
        menu_destroy(m); sound_stop();
    }
}
