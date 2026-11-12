/*
 * The Ancient Art of War (1984) - remake
 * Nov 2026 using Claude Opus 5.5
 * "Guided" by Brett Hallen
 * Original game by Dave Murry & Barry Murry
 * Published by Brøderbund Software Inc.
 *
 * test/test_diskerr.c: a test or debugging aid for the remake (see README.md here)
 */
/* Files that can't be written: "Unable to use data disk." (ovl1_61fd) waits
 * for a key over the screen; screenshots in OUT.
 * test_diskerr OUT GAME-FOLDER EMPTY-ROOT [vga] */
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
static int fails;
static void check(bool ok, const char *what) { SDL_Log("%s %s", ok ? "ok  " : "FAIL", what); fails += !ok; }
int main(int argc, char **argv)
{
    char err[256]; out = argv[1]; if (argc > 4) gm = MODE_VGA;
    sound_init_offline();
    assets_load(&ga, argv[2], err, sizeof err); campaigns_load(camps, argv[2]);
    m = menu_create(&ga, camps, argv[3]);
    menu_title(m); frames(2);
    check(!m->disk_error, "no message at the start");

    /* 1. the formation editor: F can't be written (a folder is in the way) */
    char p[1200]; root_path(m, "F", p, sizeof p); SDL_CreateDirectory(p);
    key(PK_DOWN); key(PK_ENTER); frames(30);
    for (int i = 0; i < 4; i++) { key(PK_DOWN); frames(10); }
    key(PK_ENTER); frames(4);
    for (int i = 0; i < 5; i++) key(PK_EQUALS);             /* formation 6 */
    frames(4);
    uint8_t before[420]; SDL_memcpy(before, ga.formations, 420);
    m->formed.f[6 * 14] ^= 1;                                /* a change to save */
    key(PK_S); frames(12); SDL_Log("mode %d saving %d save %d", m->formed.mode, m->formed.saving, m->formed.save);
    check(m->disk_error, "F: the message is up");
    check(!SDL_memcmp(ga.formations, m->formed.f, 420), "F: the formations are used anyway");
    shot("01_formed_error");
    int frame = m->formed.tick, state = m->state;
    frames(40);
    check(m->formed.tick == frame && m->state == state, "F: the editor waits");
    check(!menu_wants_held(m), "F: held keys are not wanted");
    key(PK_SPACE);
    check(!m->disk_error, "F: a key takes it away");
    shot("02_formed_after");
    frames(10); check(m->state == MS_FORMED, "F: back in the formation editor");

    /* 2. the campaign editor: the data disk's folder is gone (a file in its place) */
    menu_title(m); frames(2);
    SDL_strlcpy(m->edit_file, "T1", sizeof m->edit_file);
    SDL_memcpy(&m->game, &camps[0], sizeof m->game);
    char disk[1200]; SDL_strlcpy(disk, m->data_dir, sizeof disk); disk[SDL_strlen(disk) - 1] = 0;
    char moved[1300]; SDL_snprintf(moved, sizeof moved, "%s.moved", disk);
    check(SDL_RenamePath(disk, moved), "moved the data disk folder away");
    SDL_SaveFile(disk, "x", 1);
    check(!edit_save(m), "T: not written");
    check(m->disk_error, "T: the message is up");
    shot("03_title_error");
    m->disk_error = false;
    check(!titles_save(m), "TITLES: not written");
    check(m->disk_error, "TITLES: the message is up");
    m->disk_error = false;

    /* 3. typing in the scroll editor waits too */
    m->state = MS_EDIT_TEXT; m->drawn = true; m->anim = m->ed_anim = 0; m->ed_wait = 0;
    bool wanted = menu_wants_text(m);
    disk_error(m);
    check(wanted && !menu_wants_text(m), "text: not taken while the message is up");
    key(PK_ENTER);
    check(!m->disk_error && menu_wants_text(m), "text: taken again after a key");

    /* 4. VGA colours over a VGA picture */
    menu_title(m); frames(2); gm = MODE_VGA; frames(2); disk_error(m); shot("04_title_error");
    key(PK_ENTER); gm = MODE_CGA;

    /* put things back */
    SDL_RemovePath(disk); SDL_RenamePath(moved, disk); SDL_RemovePath(p);
    m->disk_error = false;
    check(edit_save(m) && !m->disk_error, "T: written once the folder is back");
    menu_destroy(m);
    SDL_Log("%d failed", fails);
    return fails != 0;
}
