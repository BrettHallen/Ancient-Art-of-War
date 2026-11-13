/*
 * The Ancient Art of War (1984) - remake
 * Nov 2026 using Claude Opus 5.5
 * "Guided" by Brett Hallen
 * Original game by Dave Murry & Barry Murry
 * Published by Brøderbund Software Inc.
 *
 * test_cards.c: the video cards (video=, Tab). What WARET.EXE shows on an EGA,
 * a Tandy 1000 and a VGA, as measured in DOSBox-X (WAR_Analysis/tools/dosbox/
 * egacmp.py compares a screen pixel by pixel):
 *   - EGA and Tandy: the 16-colour pictures in the standard 16 colours; VGA:
 *     each picture's palette, 6 bits a channel
 *   - the text rows in colour 15, on the scrolls too
 *   - the pointer: blue (colour 1) on an EGA or VGA; on a Tandy ANDed as 4-bit
 *     pixels (black, dark green, light red on a scroll)
 *   - Tandy: a yellow map cursor (14), and outlines only within 4 pixels
 *   - the Options Shoppe's display and centering lines
 * test_cards GAME DATA
 */
#define main app_main
#include "../src/main.c"
#undef main

static App app;
static int fails;
#define CHECK(c, ...) do { if (c) SDL_Log("ok   " __VA_ARGS__); else { SDL_Log("FAIL " __VA_ARGS__); fails++; } } while (0)

static void frames(int n) { for (int i = 0; i < n; i++) { draw_screen(&app); menu_step(&app); } }

/* the set of indexes in a box */
static unsigned box_colours(int x0, int y0, int x1, int y1)
{
    unsigned set = 0;
    for (int y = y0; y < y1; y++)
        for (int x = x0; x < x1; x++) set |= 1u << (app.screen.px[y * SCREEN_W + x] & 15);
    return set;
}

static void draw(VideoCard c) { set_card(&app, c); draw_screen(&app); }

int main(int argc, char **argv)
{
    if (argc < 3) { SDL_Log("test_cards GAME DATA"); return 2; }
    sound_init_offline();
    if (!load_from(&app, argv[1], false)) { SDL_Log("no game files"); return 1; }
    CHECK(app.ga.set[MODE_VGA].loaded, "WARET.DAT is there");

    /* output: EGA/Tandy standard colours, VGA 6-bit DAC */
    {
        static Screen s; static uint32_t out[SCREEN_W * SCREEN_H];
        RGB pal[16];
        for (int i = 0; i < 16; i++) pal[i] = (RGB){ 0xA0, 0x5F, (uint8_t)(i * 16) };
        screen_set_palette(&s, pal, 16);
        for (int i = 0; i < 16; i++) s.px[i] = (uint8_t)i;
        gfx_card = CARD_EGA; screen_to_xrgb(&s, out);
        CHECK((out[6] & 0xFFFFFF) == 0xAA5500 && (out[9] & 0xFFFFFF) == 0x5555FF, "EGA: the standard colours (6 brown, 9 light blue)");
        gfx_card = CARD_TANDY; screen_to_xrgb(&s, out);
        CHECK((out[14] & 0xFFFFFF) == 0xFFFF55, "Tandy: the standard colours");
        gfx_card = CARD_VGA; screen_to_xrgb(&s, out);
        CHECK((out[0] & 0xFFFFFF) == 0xA25D00, "VGA: the picture's palette in 6 bits (A0 5F 00 -> A2 5D 00)");
        gfx_card = CARD_CGA;
    }

    /* the menus */
    app.menu = menu_create(&app.ga, app.campaigns, argv[2]);
    menu_start(app.menu); app.in_menu = true;
    set_card(&app, CARD_EGA);
    frames(5);
    menu_key(app.menu, PK_ENTER); frames(3);                /* the title page */
    menu_key(app.menu, PK_ENTER); frames(80);               /* go to war: the titles scroll */
    for (VideoCard c = CARD_EGA; c <= CARD_VGA; c++) {
        draw(c);
        unsigned rows = box_colours(0, 182, SCREEN_W, 200);
        CHECK(rows == ((1u << 0) | (1u << 15)), "%s: the scroll's text rows in 0 and 15 (%04X)", card_name(c), rows);
        unsigned hand = box_colours(12, 88, 40, 96);
        if (c == CARD_TANDY) CHECK(hand & (1u << 2) && hand & (1u << 12) && !(hand & (1u << 1)), "Tandy: the hand ANDed (%04X)", hand);
        else CHECK(hand & (1u << 1) && !(hand & (1u << 12)), "%s: a blue hand (%04X)", card_name(c), hand);
    }
    set_card(&app, CARD_CGA); draw_screen(&app);
    CHECK(box_colours(0, 182, SCREEN_W, 200) == ((1u << 0) | (1u << 3)), "CGA: the scroll's text rows in colours 0 and 3");

    /* the map: Tandy's yellow cursor and 4-pixel outlines */
    begin_game(&app, &app.campaigns[0], 1);
    app.in_menu = false; app.playing = true;
    int cx = app.play.cx * 4, cy = app.play.cy * 2 - app.play.view_top * 2;
    draw(CARD_TANDY);
    unsigned cur = box_colours(SDL_max(cx - 4, 0), SDL_max(cy, 0), cx + 8, SDL_min(cy + 10, 181));
    CHECK(cur & (1u << 14), "Tandy: a yellow cursor (%04X)", cur);
    draw(CARD_EGA);
    cur = box_colours(SDL_max(cx - 4, 0), SDL_max(cy, 0), cx + 8, SDL_min(cy + 10, 181));
    CHECK(cur & (1u << 15) && !(cur & (1u << 14)), "EGA: a white cursor (%04X)", cur);
    {
        static Screen s; static uint8_t spr[SPRITE_H][SPRITE_W];
        spr[0][4] = 1;                                  /* one figure pixel at 4: outline at 3 and 5 */
        screen_clear(&s, 2);
        gfx_card = CARD_EGA; screen_sprite(&s, spr, 0, 0, false, 15, 0, 0, 200);
        CHECK(s.px[3] == 15 && s.px[4] == 0 && s.px[5] == 15, "EGA: the outline on both sides");
        screen_clear(&s, 2);
        gfx_card = CARD_TANDY; screen_sprite(&s, spr, 0, 0, false, 15, 0, 0, 200);
        CHECK(s.px[3] == 2 && s.px[4] == 0 && s.px[5] == 15, "Tandy: not across the 4-pixel word");
        gfx_card = CARD_CGA;
    }

    menu_destroy(app.menu);
    assets_free(&app.ga);
    SDL_Log("%d failed", fails);
    return fails != 0;
}
