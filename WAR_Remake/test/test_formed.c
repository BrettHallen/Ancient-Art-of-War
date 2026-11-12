/*
 * The Ancient Art of War (1984) - remake
 * Nov 2026 using Claude Opus 5.5
 * "Guided" by Brett Hallen
 * Original game by Dave Murry & Barry Murry
 * Published by Brøderbund Software Inc.
 *
 * test/test_formed.c: a test or debugging aid for the remake (see README.md here)
 */
/* Step 7: Teach squads new formations; screenshots in OUT */
#include "../src/menu.c"
static GameAssets ga; static Campaign camps[NUM_CAMPAIGNS]; static Screen scr; static uint32_t rgb[SCREEN_W * SCREEN_H];
static Menu *m; static const char *out; static GfxMode gm = MODE_CGA; static int held;
static void shot(const char *name)
{
    menu_draw(m, &scr, gm); screen_to_xrgb(&scr, rgb);
    SDL_Surface *s = SDL_CreateSurfaceFrom(SCREEN_W, SCREEN_H, SDL_PIXELFORMAT_XRGB8888, rgb, SCREEN_W * 4);
    char p[512]; SDL_snprintf(p, sizeof p, "%s/%s%s.bmp", out, name, gm == MODE_VGA ? "_vga" : ""); SDL_SaveBMP(s, p); SDL_DestroySurface(s);
}
static void frames(int n) { for (int i = 0; i < n; i++) { menu_draw(m, &scr, gm); if (menu_wants_held(m)) menu_held(m, held); menu_frame(m); menu_action(m); } }
static void key(int k) { menu_key(m, k); frames(3); }
static void hold(int k, int n) { held = k; frames(n); held = 0; frames(1); }
int main(int argc, char **argv)
{
    char err[256]; out = argv[1]; if (argc > 4) gm = MODE_VGA;
    sound_init_offline();
    assets_load(&ga, argv[2], err, sizeof err); campaigns_load(camps, argv[2]);
    uint8_t before[420]; SDL_memcpy(before, ga.formations, 420);
    m = menu_create(&ga, camps, argv[3]);
    menu_title(m); frames(2); key(PK_DOWN); key(PK_ENTER); frames(30);
    for (int i = 0; i < 4; i++) { key(PK_DOWN); frames(10); }
    key(PK_ENTER); frames(4); SDL_Log("state %d", m->state); shot("01_form1");
    key(PK_EQUALS); frames(4); shot("02_form2");
    for (int i = 0; i < 4; i++) key(PK_EQUALS);
    frames(4); shot("03_form6");
    key(PK_M); frames(2); shot("04_pointer");
    FormEdit *e = &m->formed;
    /* walk the pointer onto man 0 */
    int tx = e->x[0] + 1, ty = e->y[0] + 100 + 2;
    for (int i = 0; i < 200 && (e->cx != tx || e->cy != ty); i++) {
        int k = e->cx < tx ? (e->cy < ty ? PK_PGDN : e->cy > ty ? PK_PGUP : PK_RIGHT)
              : e->cx > tx ? (e->cy < ty ? PK_END : e->cy > ty ? PK_HOME : PK_LEFT)
              : (e->cy < ty ? PK_DOWN : PK_UP);
        hold(k, 1);
    }
    SDL_Log("pointer %d,%d over %d (man0 %d,%d type %d slot %d)", e->cx, e->cy, e->over, e->x[0], e->y[0], e->type[0], e->slot[0]);
    shot("05_over_man");
    key(PK_M); frames(2); shot("06_move_mode");
    int sel = e->sel, x0 = e->x[sel], y0 = e->y[sel];
    hold(PK_LEFT, 20); shot("07_walked_left");
    hold(PK_UP, 10); hold(PK_PGDN, 6);
    SDL_Log("man %d moved from %d,%d to %d,%d", sel, x0, y0, e->x[sel], e->y[sel]);
    key(PK_A); frames(2); shot("08_archer");
    key(PK_M); frames(2); key(PK_E); frames(2); shot("09_back");
    key(PK_S); frames(2); shot("10_saving"); frames(10);
    char p[1200]; root_path(m, "F", p, sizeof p); size_t n = 0; uint8_t *f = SDL_LoadFile(p, &n);
    int diff = 0; for (int i = 0; i < 420; i++) diff += f && f[i] != before[i];
    SDL_Log("F written: %zu bytes, %d bytes differ from the game's F, ga updated %d", n, diff, f && !SDL_memcmp(f, ga.formations, 420));
    for (int k = 0; k < 14; k++) SDL_Log("  F6 man %d: type %d x %d y %d", k, f[6*14+k], f[140+6*14+k], f[280+6*14+k]);
    /* a change, then Enter: Save the last changes? N */
    key(PK_M); frames(2); key(PK_E); key(PK_ENTER); frames(2); shot("11_ask"); key(PK_N); frames(3);
    SDL_Log("after N: state %d (title %d)", m->state, MS_TITLE);
    /* formations 1-5 can't be changed */
    menu_title(m); frames(2); key(PK_DOWN); key(PK_ENTER); frames(30);
    for (int i = 0; i < 4; i++) { key(PK_DOWN); frames(10); }
    key(PK_ENTER); frames(4); key(PK_M); key(PK_S); frames(3); SDL_Log("form 1: mode %d save %d", m->formed.mode, m->formed.saving);
    key(PK_ENTER); frames(3); SDL_Log("enter: state %d", m->state);
    /* reloading picks up the saved F */
    Menu *m2 = menu_create(&ga, camps, argv[3]); SDL_Log("reload ok %d", !SDL_memcmp(ga.formations, f, 420)); menu_destroy(m2);
    return 0;
}
