/*
 * The Ancient Art of War (1984) - remake
 * Nov 2026 using Claude Opus 5.5
 * "Guided" by Brett Hallen
 * Original game by Dave Murry & Barry Murry
 * Published by Brøderbund Software Inc.
 *
 * test/test_editor.c: a test or debugging aid for the remake (see README.md here)
 */
/* Step 7: make a campaign (6 steps), change it, remove it; screenshots in OUT */
#include "../src/menu.c"
static GameAssets ga; static Campaign camps[NUM_CAMPAIGNS]; static Screen scr; static uint32_t rgb[SCREEN_W * SCREEN_H];
static Menu *m; static const char *out; static GfxMode gm = MODE_CGA;
static void shot(const char *name)
{
    menu_draw(m, &scr, gm); screen_to_xrgb(&scr, rgb);
    SDL_Surface *s = SDL_CreateSurfaceFrom(SCREEN_W, SCREEN_H, SDL_PIXELFORMAT_XRGB8888, rgb, SCREEN_W * 4);
    char p[512]; SDL_snprintf(p, sizeof p, "%s/%s%s.bmp", out, name, gm == MODE_VGA ? "_vga" : ""); SDL_SaveBMP(s, p); SDL_DestroySurface(s);
}
static int held;
static void frames(int n) { for (int i = 0; i < n; i++) { menu_draw(m, &scr, gm); if (menu_wants_held(m)) menu_held(m, held); menu_frame(m); menu_action(m); } }
static void key(int k) { menu_key(m, k); frames(3); }
static void type(const char *t) { char c[2] = { 0, 0 }; for (; *t; t++) { c[0] = *t; menu_text(m, c); frames(1); } }
static void hold(int k, int n) { held = k; frames(n); held = 0; frames(1); }
static void st(const char *what) { SDL_Log("%-28s state %d a692 %d edit %d", what, m->state, m->a692, m->edit); }
int main(int argc, char **argv)
{
    (void)argc; char err[256]; out = argv[1];
    const char *root = argv[3];
    if (argc > 4) gm = MODE_VGA;
    sound_init_offline();
    assets_load(&ga, argv[2], err, sizeof err); campaigns_load(camps, argv[2]);
    m = menu_create(&ga, camps, root);
    menu_title(m); frames(2);
    key(PK_DOWN); key(PK_ENTER); frames(30); st("campaign menu");
    key(PK_ENTER); frames(2); st("new: map maker intro"); shot("01_map_intro");
    key(PK_ENTER); frames(2); shot("02_map");
    /* a river down column 4, a fort, a village */
    key(PK_RIGHT); key(PK_RIGHT); key(PK_RIGHT); key(PK_RIGHT);
    for (int r = 0; r < 6; r++) { key(PK_P); key(PK_D); key(PK_P); key(PK_P); key(PK_P); key(PK_DOWN); }
    key(PK_RIGHT); key(PK_C); key(PK_RIGHT); key(PK_D);
    for (int r = 0; r < 12; r++) key(PK_DOWN);
    key(PK_LEFT); key(PK_LEFT); key(PK_LEFT); key(PK_LEFT); key(PK_C);
    shot("03_map_edited");
    key(PK_P); shot("04_map_page2");
    key(PK_ENTER); frames(10); shot("05_map_continue");
    key(PK_Y); frames(2); st("squads intro"); shot("06_squads_intro");
    key(PK_SPACE); frames(2); shot("07_squads_main");
    /* a friendly squad near the bottom: 7 knights, 3 archers */
    hold(PK_LEFT, 10);
    key(PK_S); frames(2); st("size"); shot("08_size");
    for (int i = 0; i < 7; i++) key(PK_K);
    for (int i = 0; i < 3; i++) key(PK_A);
    shot("09_size_filled");
    key(PK_E); frames(2); shot("10_squad_menu");
    key(PK_I); frames(2); shot("11_info");
    key(PK_I); key(PK_C); key(PK_C); key(PK_S); key(PK_E); frames(2);
    key(PK_F); frames(2); shot("12_formation"); key(PK_EQUALS); key(PK_EQUALS); shot("13_formation3"); key(PK_E); frames(2);
    /* a flag beside it */
    hold(PK_RIGHT, 6); st("after move"); shot("14_main_again");
    key(PK_F); frames(2); shot("15_flag");
    /* an enemy squad further up */
    hold(PK_UP, 40); hold(PK_RIGHT, 10);
    key(PK_S); for (int i = 0; i < 9; i++) key(PK_B); key(PK_E); frames(2);
    key(PK_I); key(PK_T); key(PK_E); frames(2);
    hold(PK_RIGHT, 5); key(PK_F); key(PK_F); frames(2); shot("16_enemy_flag");
    hold(PK_DOWN, 3); key(PK_T); frames(2); shot("17_total"); key(PK_E); frames(2);
    key(PK_ENTER); frames(10); shot("18_squads_done"); key(PK_Y); frames(5);
    st("opponent"); shot("19_opponent_intro");
    key(PK_SPACE); frames(10); key(PK_EQUALS); frames(10); key(PK_EQUALS); frames(10); shot("20_opponent");
    key(PK_ENTER); frames(60); st("rules"); shot("21_rules_intro");
    key(PK_SPACE); frames(2); key(PK_DOWN); key(PK_SPACE); key(PK_DOWN); key(PK_SPACE); shot("22_rules");
    key(PK_ENTER); frames(80); st("story"); shot("23_story_intro");
    key(PK_SPACE); frames(2); shot("24_story");
    type("This is a Test campaign, made by"); key(PK_ENTER);
    type("the remake's editor. Two squads, one"); key(PK_ENTER); type("river!");
    key(PK_BACKSPACE); type("?");
    shot("25_story_typed");
    key(PK_ESC); frames(80); st("title"); shot("26_title_anim");
    frames(20); shot("27_title");
    type("The Remake Test"); shot("28_title_typed");
    key(PK_ENTER); frames(40); st("complete"); shot("29_complete");
    frames(60); st("after");
    /* the files */
    char p[1200]; size_t n = 0;
    SDL_snprintf(p, sizeof p, "%sTITLES", m->data_dir);
    uint8_t *t = SDL_LoadFile(p, &n);
    SDL_Log("TITLES %zu bytes, names %.3s %.3s, row 5: '%.32s'", n, t, t + 3, t + 36 + 5 * 32);
    Campaign c;
    bool ok = campaign_load_file(&c, m->data_dir, "T1");
    SDL_Log("T1 loaded %d: opp %d, %d friend %d foe, rules %d %d %d, map[0][4]=%d [6][5]=%d, story '%.40s'",
            ok, c.opponent, c.num_friendly, c.num_enemy, c.rules[0], c.rules[1], c.rules[2], c.map[0][4], c.map[6][5], c.story);
    for (int i = 0; i < MAX_FLAGS; i++) if (c.flags[i].present) SDL_Log("  flag %d at %d,%d white %d", i, c.flags[i].x, c.flags[i].y, c.flags[i].white);
    for (int i = 0; i < MAX_SQUADS; i++) if (c.squads[i].present) SDL_Log("  squad %d at %d,%d f%d k%d b%d a%d s%d form %d cond %d speed %d", i, c.squads[i].x, c.squads[i].y, c.squads[i].friendly, c.squads[i].knights, c.squads[i].barbarians, c.squads[i].archers, c.squads[i].spies, c.squads[i].formation, c.squads[i].condition, c.squads[i].speed);

    /* change it: the story */
    menu_title(m); frames(2); key(PK_DOWN); key(PK_ENTER); frames(30);
    key(PK_DOWN); key(PK_ENTER); frames(60); st("choose to change"); shot("30_change_list");
    key(PK_ENTER); frames(60); st("editor menu"); shot("31_editor_menu");
    key(PK_DOWN); frames(10); key(PK_ENTER); frames(80); st("story change"); shot("32_story_change");
    key(PK_DOWN); key(PK_DOWN); key(PK_DOWN); type("Changed."); key(PK_ESC); frames(3); shot("33_save_q");
    key(PK_Y); frames(80); st("back to editor menu");
    campaign_load_file(&c, m->data_dir, "T1"); SDL_Log("story now: '%s'", c.story);
    key(PK_DOWN); frames(10); key(PK_ENTER); frames(5); st("map maker (change)"); shot("34_map_change");
    key(PK_B); key(PK_ENTER); frames(10); key(PK_N); frames(20); st("discarded");
    campaign_load_file(&c, m->data_dir, "T1"); SDL_Log("map[0][0] after No: %d", c.map[0][0]);
    for (int i = 0; i < 3; i++) { key(PK_DOWN); frames(10); }
    key(PK_ENTER); frames(5); st("army (change)"); shot("35_army_change");
    key(PK_ENTER); frames(10); key(PK_Y); frames(20); st("saved army");
    key(PK_UP); frames(10); key(PK_ENTER); frames(10); st("enemy (change)"); shot("36_enemy_change");
    key(PK_EQUALS); frames(10); key(PK_ENTER); frames(10); st("saved enemy");
    campaign_load_file(&c, m->data_dir, "T1"); SDL_Log("opponent now %d", c.opponent);
    /* exit */
    while (m->edit_item != 3) { key(PK_DOWN); frames(10); }
    key(PK_ENTER); frames(5); st("exit");
    /* play it from the data disk */
    menu_title(m); frames(2); key(PK_ENTER); frames(40);
    for (int i = 0; i < 11; i++) key(PK_DOWN);
    key(PK_ENTER); frames(5); st("data disk titles"); shot("37_data_titles");
    key(PK_DOWN); key(PK_ENTER); frames(60); st("story to play"); shot("38_play_story");
    SDL_Log("game: %s", m->game.title);
    if (SDL_getenv("KEEP")) return 0;
    /* remove it */
    menu_title(m); frames(2); key(PK_DOWN); key(PK_ENTER); frames(30);
    key(PK_DOWN); key(PK_DOWN); key(PK_ENTER); frames(60); st("remove list"); shot("39_remove_list");
    key(PK_ENTER); frames(3); shot("40_erase_q"); key(PK_Y); frames(8); shot("41_erasing"); frames(80); st("removed");
    t = SDL_LoadFile(p, &n);
    SDL_Log("TITLES names %.3s %.3s ... last %.3s, row 5: '%.32s'", t, t + 3, t + 33, t + 36 + 5 * 32);
    return 0;
}
