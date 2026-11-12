/*
 * The Ancient Art of War (1984) - remake
 * Nov 2026 using Claude Opus 5.5
 * "Guided" by Brett Hallen
 * Original game by Dave Murry & Barry Murry
 * Published by Brøderbund Software Inc.
 *
 * test/test_tab_rows.c: a test or debugging aid for the remake (see README.md here)
 */
/* Tab on the scroll pages (titles, story, rules): the help rows at the bottom
 * must look the same (screenshots to OUT: test_tab_rows GAME DATA OUT) after CGA -> VGA -> CGA (and VGA -> CGA -> VGA) as before. */
#include "../src/menu.c"
static GameAssets ga; static Campaign camps[NUM_CAMPAIGNS]; static Screen scr;
static uint32_t a[SCREEN_W * SCREEN_H], b[SCREEN_W * SCREEN_H];
static const char *outdir; static Menu *m; static GfxMode gm; static int fails;
static void frames(int n) { for (int i = 0; i < n; i++) { menu_draw(m, &scr, gm); menu_frame(m); menu_action(m); } }
static void key(int k) { menu_key(m, k); frames(3); }
static void grab(uint32_t *out) { menu_draw(m, &scr, gm); screen_to_xrgb(&scr, out); }
static int lit(const uint32_t *px) { int n = 0; for (int i = 184 * SCREEN_W; i < SCREEN_W * SCREEN_H; i++) n += (px[i] & 0xFFFFFF) != 0; return n; }
static void check_page(const char *name, GfxMode first)
{
    GfxMode other = first == MODE_CGA ? MODE_VGA : MODE_CGA;
    gm = first; frames(2); grab(a);
    gm = other; frames(2);
    gm = first; frames(2); grab(b);
    int diff = 0; for (int i = 184 * SCREEN_W; i < SCREEN_W * SCREEN_H; i++) diff += a[i] != b[i];
    bool ok = diff == 0 && lit(a) > 500;
    SDL_Log("%s %s %s: help rows %d lit pixels, %d differ after %s and back", ok ? "ok  " : "FAIL", name, first == MODE_CGA ? "CGA" : "VGA", lit(a), diff, other == MODE_CGA ? "CGA" : "VGA");
    fails += !ok;
    SDL_Surface *sf = SDL_CreateSurfaceFrom(SCREEN_W, SCREEN_H, SDL_PIXELFORMAT_XRGB8888, b, SCREEN_W * 4);
    char path[600]; SDL_snprintf(path, sizeof path, "%s/%s_%s.bmp", outdir, name, first == MODE_CGA ? "cga" : "vga");
    SDL_SaveBMP(sf, path); SDL_DestroySurface(sf);
}
int main(int argc, char **argv)
{
    (void)argc; char err[256];
    sound_init_offline();
    assets_load(&ga, argv[1], err, sizeof err); campaigns_load(camps, argv[1]);
    outdir = argv[3]; m = menu_create(&ga, camps, argv[2]);
    for (int g = 0; g < 2; g++) {
        GfxMode first = g ? MODE_VGA : MODE_CGA;
        gm = first;
        menu_title(m); frames(3);
        key(PK_ENTER); frames(80);                       /* go to war: the titles unroll */
        check_page("titles", first);
        key(PK_ENTER); frames(80);                       /* the story */
        check_page("story", first);
        key(PK_ENTER); frames(80);                       /* the rules */
        check_page("rules", first);
    }
    SDL_Log("%d failed", fails);
    return fails != 0;
}
