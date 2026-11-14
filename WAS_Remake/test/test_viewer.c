/*
 * The Ancient Art of War at Sea (1987) - remake
 * Nov 2026 using Claude Opus 5.5
 * "Guided" by Brett Hallen
 * Original game by Dave Murry & Barry Murry
 * Published by Brøderbund Software Inc.
 *
 * test/test_viewer.c: the asset loader and the viewer (step 1).
 *
 * Loads the game, checks what the analysis found (WAS_file_reference.md),
 * drives the viewer with its keys and writes every screen in both colour
 * sets as BMPs (compare_extract.py then compares the pictures and tiles
 * with WAS_Graphics/extract_graphics.py's PNGs).
 *
 * Usage: test_viewer GAME-FOLDER OUT-FOLDER
 */
#include <SDL3/SDL.h>

#include "../src/assets.h"
#include "../src/campaign.h"
#include "../src/viewer.h"

static int fails = 0;

static void fail(int line, const char *fmt, ...)
{
    char msg[256];
    va_list ap;
    va_start(ap, fmt);
    SDL_vsnprintf(msg, sizeof msg, fmt, ap);
    va_end(ap);
    fails++;
    SDL_Log("FAIL line %d: %s", line, msg);
}
#define CHECK(c, ...) do { if (!(c)) fail(__LINE__, __VA_ARGS__); } while (0)

static void save(const Screen *s, const char *path)
{
    static uint32_t rgb[SCREEN_W * SCREEN_H];
    screen_to_xrgb(s, rgb);
    SDL_Surface *surf = SDL_CreateSurfaceFrom(SCREEN_W, SCREEN_H, SDL_PIXELFORMAT_XRGB8888, rgb, SCREEN_W * 4);
    CHECK(surf && SDL_SaveBMP(surf, path), "can't write %s", path);
    SDL_DestroySurface(surf);
}

int main(int argc, char **argv)
{
    if (argc < 3) { SDL_Log("usage: test_viewer GAME-FOLDER OUT-FOLDER"); return 2; }
    static GameAssets ga;
    static Campaign cs[NUM_CAMPAIGNS];
    static Screen s;
    char err[256];
    if (!assets_load(&ga, argv[1], err, sizeof err)) { SDL_Log("load: %s", err); return 1; }
    CHECK(ga.set[MODE_CGA].loaded && ga.set[MODE_16].loaded, "both colour sets");

    /* the pictures: every CGA one, and all but SPP and SPQ in 16 colours */
    for (int i = 0; i < PIC_COUNT; i++) {
        CHECK(ga.set[MODE_CGA].pictures[i].px != NULL, "CGA %s", picture_file(i));
        bool want16 = i != PIC_REPORT_3 && i != PIC_REPORT_4;
        CHECK((ga.set[MODE_16].pictures[i].px != NULL) == want16, "16-colour %s", picture_file(i));
    }
    for (int t = 0; t < NUM_TILES; t++)
        CHECK(ga.set[MODE_CGA].tiles[t].px && ga.set[MODE_16].tiles[t].px, "tile %d", t + 1);

    /* the font is WAR.EXE's: 'A' has its top row 30h (..XX....) */
    CHECK(ga.font['A' - FONT_FIRST][0] == 0x30, "font 'A' = %02X", ga.font['A' - FONT_FIRST][0]);
    CHECK(SDL_strcmp(assets_ds_string(&ga, 0x2D2), "Wind is from the ") == 0, "DS string");
    /* sprites: the anchor is 8 x 8, the east-bound ship 8 x 4, one line pair lower */
    CHECK(ga.map_sprite[MSPR_ANCHOR].w == 8 && ga.map_sprite[MSPR_ANCHOR].h == 8, "anchor size");
    CHECK(ga.map_sprite[MSPR_HEADING_0 + 4].h == 4 && ga.heading_dy[4] == 1 && ga.heading_dy[0] == 0, "heading E");

    /* the campaigns */
    int n = campaigns_load(cs, argv[1]);
    CHECK(n == NUM_CAMPAIGNS, "%d campaigns", n);
    CHECK(SDL_strcmp(cs[0].title, "The Bane of Oldain") == 0, "title 0 '%s'", cs[0].title);
    CHECK(SDL_strcmp(cs[5].title, "1588 The Spanish Armada") == 0, "title 5 '%s'", cs[5].title);
    CHECK(SDL_strcmp(cs[10].title, "You Only Live Thrice") == 0, "title 10 '%s'", cs[10].title);
    CHECK(cs[0].opponent == 0 && cs[5].opponent == 1, "Thor Foote, Medina Sidonia");
    CHECK(cs[0].num_friendly == 11 && cs[0].num_enemy == 23, "S0 squadrons %d v %d", cs[0].num_friendly, cs[0].num_enemy);
    CHECK(SDL_strcmp(cs[0].ships[0].name, "REPULSIVE") == 0, "S0 ship 0 '%s'", cs[0].ships[0].name);
    CHECK(SDL_strcmp(cs[5].ships[39].name, "NAPOLITANA") == 0 && !cs[5].ships[39].present, "S5 ship 39 in reserve");
    /* the Armada: each ship's squadron is on its own side (Spanish 0, English -1) */
    CHECK(SDL_strcmp(cs[5].ships[4].name, "SAN MARTIN") == 0 && !cs[5].squadrons[cs[5].ships[4].squadron].friendly &&
          cs[5].ships[4].type == SHIP_FLAG, "San Martin, Medina Sidonia's flagship");
    CHECK(SDL_strcmp(cs[5].ships[7].name, "ARK") == 0 && cs[5].squadrons[cs[5].ships[7].squadron].friendly, "Ark is English");
    for (int i = 0; i < NUM_CAMPAIGNS; i++) {
        int per[MAX_SHIPS] = { 0 };
        for (int k = 0; k < MAX_SHIPS; k++) if (cs[i].ships[k].present) per[cs[i].ships[k].squadron]++;
        for (int q = 0; q < MAX_SHIPS; q++) CHECK(per[q] <= 3, "S%d squadron %d has %d ships", i, q, per[q]);
    }
    CHECK(cs[0].rules[5] == 80, "S0 rule 5 = %d", cs[0].rules[5]);
    CHECK(SDL_strncmp(cs[0].story, " ... The Bane of Oldain ...", 27) == 0, "S0 story");
    for (int i = 0; i < NUM_CAMPAIGNS; i++)
        for (int r = 0; r < MAP_H; r++)
            for (int c = 0; c < MAP_W; c++)
                CHECK(cs[i].map[r][c] >= 1 && cs[i].map[r][c] <= NUM_TILES, "S%d map tile", i);

    /* the viewer: every screen in CGA, PCjr and Tandy (Tab's order), the captions off, maps at the top and bottom */
    Viewer v;
    viewer_init(&v, &ga, cs);
    viewer_key(&v, VK_CAPTION);
    char path[1200];
    for (int m = 0; m < 3; m++) {
        CHECK(viewer_video(&v) == (Video)m, "Tab %d times: %d", m, (int)viewer_video(&v));
        for (int i = 0; i < NUM_SCREENS; i++) {
            viewer_draw(&v, &s);
            SDL_snprintf(path, sizeof path, "%s/%s_%02d.bmp", argv[2], viewer_mode_name(&v), i + 1);
            save(&s, path);
            if (viewer_is_map(&v)) {
                viewer_key(&v, VK_TOP);
                viewer_draw(&v, &s);
                SDL_snprintf(path, sizeof path, "%s/%s_%02d_top.bmp", argv[2], viewer_mode_name(&v), i + 1);
                save(&s, path);
                viewer_key(&v, VK_STORY);
                viewer_draw(&v, &s);
                SDL_snprintf(path, sizeof path, "%s/%s_%02d_story.bmp", argv[2], viewer_mode_name(&v), i + 1);
                save(&s, path);
                viewer_key(&v, VK_STORY);
                viewer_draw(&v, &s);
                SDL_snprintf(path, sizeof path, "%s/%s_%02d_ships.bmp", argv[2], viewer_mode_name(&v), i + 1);
                save(&s, path);
                viewer_key(&v, VK_STORY);
                CHECK(v.page == 0, "Enter three times: the map again");
                viewer_key(&v, VK_BOTTOM);
            }
            viewer_key(&v, VK_NEXT);
        }
        CHECK(v.index == 0, "wrapped round");
        viewer_key(&v, VK_MODE);
    }
    CHECK(v.mode == MODE_CGA && v.herc, "Tab three times: Hercules");
    CHECK(SDL_strcmp(viewer_mode_name(&v), "Hercules") == 0, "Hercules' name");
    viewer_key(&v, VK_MODE);
    CHECK(v.mode == MODE_CGA && !v.herc && !v.pcjr, "Tab four times: CGA");

    /* Hercules (WAS HERC): each CGA pixel is two mono pixels, the screen at 32,72 of 720x348 */
    {
        static uint32_t h[HERC_W * HERC_H];
        screen_set_palette(&s, CGA_PALETTE, 4);
        screen_fill(&s, 0, 0, SCREEN_W, SCREEN_H, 0);
        s.px[0] = 1; s.px[1] = 2; s.px[2] = 3;
        s.px[SCREEN_W * SCREEN_H - 1] = 3;
        screen_to_herc(&s, h);
        const uint32_t L = 0xAAAAAA;
        CHECK(h[72 * HERC_W + 32] == 0 && h[72 * HERC_W + 33] == L, "colour 1: dark, lit");
        CHECK(h[72 * HERC_W + 34] == L && h[72 * HERC_W + 35] == 0, "colour 2: lit, dark");
        CHECK(h[72 * HERC_W + 36] == L && h[72 * HERC_W + 37] == L, "colour 3: lit, lit");
        CHECK(h[271 * HERC_W + 671] == L && h[271 * HERC_W + 672] == 0 && h[272 * HERC_W + 671] == 0, "the last pixel at 670,271");
        CHECK(h[0] == 0 && h[71 * HERC_W + 33] == 0, "black round it");
        v.herc = true;
        v.index = PIC_TITLE_SHIP;
        viewer_draw(&v, &s);
        screen_to_herc(&s, h);
        SDL_Surface *surf = SDL_CreateSurfaceFrom(HERC_W, HERC_H, SDL_PIXELFORMAT_XRGB8888, h, HERC_W * 4);
        SDL_snprintf(path, sizeof path, "%s/Hercules_title.bmp", argv[2]);
        CHECK(surf && SDL_SaveBMP(surf, path), "write %s", path);
        SDL_DestroySurface(surf);
        v.herc = false;
        v.index = 0;
    }

    /* keys: wrap backwards, scroll limits, caption */
    viewer_key(&v, VK_PREV);
    CHECK(v.index == NUM_SCREENS - 1 && viewer_is_map(&v), "last screen is a map");
    for (int k = 0; k < 40; k++) viewer_key(&v, VK_UP);
    CHECK(v.scroll[NUM_CAMPAIGNS - 1] == 0, "scrolled to the top");
    for (int k = 0; k < 40; k++) viewer_key(&v, VK_DOWN);
    CHECK(v.scroll[NUM_CAMPAIGNS - 1] == SCROLL_BOTTOM, "scrolled to the bottom");
    viewer_key(&v, VK_FIRST);
    viewer_key(&v, VK_UP);
    CHECK(v.index == NUM_SCREENS - 1, "Up off a map is the previous screen");
    viewer_key(&v, VK_CAPTION);
    v.mode = MODE_16;
    v.index = PIC_REPORT_3;
    viewer_draw(&v, &s);
    SDL_snprintf(path, sizeof path, "%s/caption_cga_only.bmp", argv[2]);
    save(&s, path);

    assets_free(&ga);
    SDL_Log("%s (%d failures)", fails ? "FAILED" : "passed", fails);
    return fails ? 1 : 0;
}
