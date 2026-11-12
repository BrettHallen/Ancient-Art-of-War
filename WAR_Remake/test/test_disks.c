/*
 * The Ancient Art of War (1984) - remake
 * Nov 2026 using Claude Opus 5.5
 * "Guided" by Brett Hallen
 * Original game by Dave Murry & Barry Murry
 * Published by Brøderbund Software Inc.
 *
 * test/test_disks.c: a test or debugging aid for the remake (see README.md here)
 */
/* Data disk folders: migration, the Options Shoppe line, the disk list on the scroll */
#include "../src/menu.c"
static GameAssets ga; static Campaign camps[NUM_CAMPAIGNS]; static Screen scr; static uint32_t rgb[SCREEN_W * SCREEN_H];
static Menu *m; static const char *out;
static void shot(const char *name)
{
    menu_draw(m, &scr, MODE_CGA); screen_to_xrgb(&scr, rgb);
    SDL_Surface *s = SDL_CreateSurfaceFrom(SCREEN_W, SCREEN_H, SDL_PIXELFORMAT_XRGB8888, rgb, SCREEN_W * 4);
    char p[512]; SDL_snprintf(p, sizeof p, "%s/%s.bmp", out, name); SDL_SaveBMP(s, p); SDL_DestroySurface(s);
}
static void frames(int n) { for (int i = 0; i < n; i++) { menu_draw(m, &scr, MODE_CGA); menu_frame(m); menu_action(m); } }
static void key(int k) { menu_key(m, k); frames(5); }
int main(int argc, char **argv)
{
    (void)argc; char err[256]; out = argv[1];
    const char *root = argv[3];   /* empty folder ending in / */
    assets_load(&ga, argv[2], err, sizeof err); campaigns_load(camps, argv[2]);
    /* the old layout: TITLES, S and a T file at the top */
    char p[1200]; size_t n;
    void *t = assets_load_file(argv[2], "DTITLES", &n);
    uint8_t *tt = t; SDL_memcpy(tt + 3, "T1", 2);   /* make it list a campaign */
    SDL_snprintf(p, sizeof p, "%sTITLES", root); SDL_SaveFile(p, t, n);
    SDL_snprintf(p, sizeof p, "%sS", root); uint8_t junk[0x217E] = {0}; SDL_SaveFile(p, junk, sizeof junk);
    void *t1 = assets_load_file(argv[2], "T1", &n);
    SDL_snprintf(p, sizeof p, "%sT1", root); SDL_SaveFile(p, t1, n);
    m = menu_create(&ga, camps, root);
    SDL_Log("after migration: %d disks, [0] %s, drive %s", m->ndisks, m->disks[0], m->data_dir);
    SDL_snprintf(p, sizeof p, "%sData Disks/Data Disk 1/T1", root); SDL_Log("T1 moved: %d", SDL_GetPathInfo(p, NULL));
    SDL_Log("saved game on disk 1: %d", menu_has_saved_game(m));
    /* a floppy copied in with lower-case names */
    SDL_snprintf(p, sizeof p, "%sData Disks/My 1987 campaigns", root); SDL_CreateDirectory(p);
    void *t2 = assets_load_file(argv[2], "TITLES", NULL);
    SDL_snprintf(p, sizeof p, "%sData Disks/My 1987 campaigns/titles", root); SDL_SaveFile(p, t2, 876);
    SDL_snprintf(p, sizeof p, "%sData Disks/My 1987 campaigns/t1", root); SDL_SaveFile(p, t1, T_FILE_SIZE);
    menu_destroy(m);
    m = menu_create(&ga, camps, root);
    SDL_Log("rescan: %d disks: %s | %s, drive %s", m->ndisks, m->disks[0], m->disks[1], m->disks[m->disk]);
    /* Options Shoppe: change the disk */
    menu_title(m); frames(2);
    key(PK_DOWN); key(PK_ENTER); frames(30);             /* make changes */
    for (int i = 0; i < 5; i++) key(PK_DOWN);             /* to the Options Shoppe */
    key(PK_ENTER); frames(5);
    key(PK_DOWN); key(PK_DOWN); shot("shoppe_disk1");
    key(PK_SPACE); shot("shoppe_disk2");
    key(PK_SPACE); shot("shoppe_new");
    key(PK_SPACE); key(PK_ENTER); frames(3);              /* back to disk 1 */
    SDL_Log("after shoppe: drive %s", m->disks[m->disk]);
    /* the titles: ( data disk ) lists the disks */
    menu_title(m); frames(2); key(PK_ENTER); frames(40);
    for (int i = 0; i < 11; i++) key(PK_DOWN);
    key(PK_ENTER); frames(3); shot("disk_list");
    SDL_Log("disk list: %d, sel %d count %d", m->disk_list, m->sel, m->count);
    key(PK_DOWN); key(PK_ENTER); frames(3); shot("disk2_titles");
    SDL_Log("chose: drive %s, data list %d state %d (titles %d ask %d)", m->disks[m->disk], m->data_list, m->state, MS_TITLES, MS_ASK_SAVED);
    key(PK_DOWN); key(PK_ENTER); frames(40);
    SDL_Log("campaign from disk 2: %s, state %d (story %d)", m->game.title, m->state, MS_STORY);
    /* disk 1's saved game is the original's: the message */
    menu_saved_game_failed(m); frames(2); shot("original_save");
    /* a new disk from the Shoppe */
    menu_title(m); frames(2); key(PK_DOWN); key(PK_ENTER); frames(30);
    for (int i = 0; i < 5; i++) key(PK_DOWN);
    key(PK_ENTER); frames(5); key(PK_DOWN); key(PK_DOWN);
    for (int g = 0; g < 10 && m->shoppe_disk < m->ndisks; g++) key(PK_SPACE);
    SDL_Log("state %d (shoppe %d) item %d", m->state, MS_SHOPPE, m->shoppe_item);
    key(PK_ENTER); frames(3);
    SDL_Log("new disk: %d disks, drive %s", m->ndisks, m->disks[m->disk]);
    return 0;
}
