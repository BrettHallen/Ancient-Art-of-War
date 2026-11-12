/*
 * The Ancient Art of War (1984) - remake
 * Nov 2026 using Claude Opus 5.5
 * "Guided" by Brett Hallen
 * Original game by Dave Murry & Barry Murry
 * Published by Brøderbund Software Inc.
 *
 * test/test_tune_once.c: a test or debugging aid for the remake (see README.md here)
 */
/* Tab (CGA <-> VGA) on "presented by" and the battle report must not start the tune again */
#include "../src/menu.c"
static GameAssets ga; static Campaign camps[NUM_CAMPAIGNS]; static Screen scr;
static Menu *m; static GfxMode gm = MODE_CGA; static int fails;
static float buf[44100 / MENU_HZ];
static void frames(int n) { for (int i = 0; i < n; i++) { menu_draw(m, &scr, gm); menu_frame(m); menu_action(m); sound_render(buf, 44100 / MENU_HZ); } }
static void check(bool ok, const char *what) { SDL_Log("%s %s", ok ? "ok  " : "FAIL", what); fails += !ok; }
int main(int argc, char **argv)
{
    (void)argc; char err[256];
    sound_init_offline();
    assets_load(&ga, argv[1], err, sizeof err); campaigns_load(camps, argv[1]);
    m = menu_create(&ga, camps, argv[2]);
    menu_start(m); frames(20);
    double b1 = sound_busy();
    for (int i = 0; i < 4; i++) { gm = gm == MODE_CGA ? MODE_VGA : MODE_CGA; frames(5); }
    double b2 = sound_busy();
    SDL_Log("presented: %.2f s left, after 4 switches %.2f s", b1, b2);
    check(b1 > 1 && b2 < b1, "the opening tune isn't started again");
    frames((int)((b2 + 4) * MENU_HZ));
    check(m->state == MS_TITLE, "the title page follows the tune");
    m->report_code = 0; go(m, MS_REPORT); frames(5);
    b1 = sound_busy();
    gm = gm == MODE_CGA ? MODE_VGA : MODE_CGA; frames(5);
    b2 = sound_busy();
    SDL_Log("report: %.2f s left, after a switch %.2f s", b1, b2);
    check(b1 > 0.5 && b2 < b1, "the victory tune isn't started again");
    frames((int)((b2 + 1) * MENU_HZ));
    check(m->step == 1, "the prompt after the tune");
    gm = gm == MODE_CGA ? MODE_VGA : MODE_CGA; frames(3);
    check(m->step == 1 && sound_busy() <= 0.0, "switching after the tune: no tune, the prompt stays");
    menu_key(m, PK_ENTER); frames(2);
    check(m->state == MS_TITLE, "a key ends the report");
    SDL_Log("%d failed", fails);
    return fails != 0;
}
