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
 *   - the trace box: shorter on all three; on a Tandy its sides XOR 7
 *   - the map maker: the line over the bottom row; the tiles 4 pixels left on an
 *     EGA or VGA; the plan's cell light red (Tandy) or ORed with 12 (EGA, VGA)
 *   - the Options Shoppe's display and centering lines; its display line and the
 *     CGA cards go together (tv/composite: the CGA on a TV, rgb: the plain CGA)
 * and the CGA's composite colours (video=composite, composite2).
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

    /* the trace box (draw_cursor): CGA lines 0 and 12 with sides on 2, 4, 8, 10; WARET.EXE lines 2 and 10, sides 4, 6, 8 */
    {
        Play *p = &app.play;
        p->mode = PM_TRACE;
        int bx = p->cx * 4, l = p->cy * 2 - p->view_top * 2;
        if (l >= 0 && l + 12 < 182 && bx + 8 <= SCREEN_W) {
            static const uint8_t TOP[8] = { 0, 0, 3, 3, 3, 3, 0, 0 };
            bool ok = true;
            draw(CARD_CGA);
            for (int i = 0; i < 8; i++) ok = ok && app.screen.px[l * SCREEN_W + bx + i] == TOP[i] && app.screen.px[(l + 12) * SCREEN_W + bx + i] == TOP[i];
            CHECK(ok, "CGA: the trace box's top and bottom on lines 0 and 12");
            draw(CARD_EGA);
            ok = true;
            for (int i = 0; i < 8; i++) ok = ok && app.screen.px[(l + 2) * SCREEN_W + bx + i] == (TOP[i] ? 15 : 0) && app.screen.px[(l + 10) * SCREEN_W + bx + i] == (TOP[i] ? 15 : 0);
            CHECK(ok && app.screen.px[(l + 6) * SCREEN_W + bx] == 15 && app.screen.px[(l + 6) * SCREEN_W + bx + 7] == 15, "EGA: the box on lines 2 to 10, white sides");
            draw(CARD_TANDY);
            uint8_t side = app.screen.px[(l + 8) * SCREEN_W + bx + 7];   /* clear of the cross */
            set_card(&app, CARD_TANDY); p->mode = PM_NORMAL; draw_screen(&app);
            uint8_t under = app.screen.px[(l + 8) * SCREEN_W + bx + 7];
            p->mode = PM_TRACE; draw_screen(&app);
            CHECK(app.screen.px[(l + 2) * SCREEN_W + bx + 2] == 14 && side == (under ^ 7), "Tandy: a yellow top, the sides XOR 7 (%d over %d)", side, under);
        } else CHECK(false, "the cursor is on the screen for the trace box");
        p->mode = PM_NORMAL;
    }

    /* the composite colours: a solid colour is the same in every phase; old and new CGAs differ */
    {
        static Screen s; static uint32_t o[COMP_W * SCREEN_H], o2[COMP_W * SCREEN_H];
        gfx_card = CARD_COMPOSITE;
        screen_set_palette(&s, CGA_PALETTE, 4);
        screen_clear(&s, 0); screen_to_composite(&s, false, o);
        CHECK((o[0] & 0xFFFFFF) == 0 && (o[COMP_W * SCREEN_H - 1] & 0xFFFFFF) == 0, "composite: black is black");
        screen_clear(&s, 1); screen_to_composite(&s, false, o); screen_to_composite(&s, true, o2);
        bool same = true;                               /* away from the line's ends, which border black */
        for (int x = 4; x < COMP_W - 4; x++) same = same && o[COMP_W + x] == o[COMP_W + 100];
        CHECK(same && o[COMP_W + 100] != o2[COMP_W + 100], "composite: colour 1 is one colour along the line (%06X), another on the new CGA (%06X)",
              o[COMP_W + 100] & 0xFFFFFF, o2[COMP_W + 100] & 0xFFFFFF);
        screen_clear(&s, 3); screen_to_composite(&s, false, o);
        CHECK((o[COMP_W + 100] & 0xFFFFFF) == 0xFFFCFF, "composite: white (%06X)", o[COMP_W + 100] & 0xFFFFFF);
        static const RGB BW[4] = { { 0, 0, 0 }, { 0xCC, 0xCC, 0xCC }, { 0x9C, 0x9C, 0x9C }, { 0xFF, 0xFF, 0xFF } };   /* apply_display's greys */
        screen_set_palette(&s, BW, 4);
        screen_clear(&s, 0); s.px[1] = 3;
        screen_to_composite(&s, false, o);
        CHECK((o[0] & 0xFFFFFF) == 0 && (o[1] & 0xFFFFFF) == 0 && (o[2] & 0xFFFFFF) == 0xFFFFFF && (o[3] & 0xFFFFFF) == 0xFFFFFF,
              "composite: black and white is not coloured, each pixel twice");
    }

    /* the map maker: make a new campaign */
    app.in_menu = true; app.playing = false;
    menu_title(app.menu); frames(2);
    menu_key(app.menu, PK_EQUALS); frames(3);
    menu_key(app.menu, PK_ENTER); frames(30);               /* the campaign menu */
    menu_key(app.menu, PK_ENTER); frames(10);               /* the map maker's first page */
    draw(CARD_CGA);
    CHECK(box_colours(0, 190, SCREEN_W, 191) == (1u << 3), "map maker: a line over the bottom row");
    CHECK(app.screen.px[95 * SCREEN_W + 84] != 0 && app.screen.px[95 * SCREEN_W + 83] == 0, "CGA: the first tile at x 84");
    draw(CARD_TANDY);
    CHECK(app.screen.px[95 * SCREEN_W + 84] != 0 && app.screen.px[95 * SCREEN_W + 83] == 0, "Tandy: the first tile at x 84");
    draw(CARD_EGA);
    CHECK(app.screen.px[95 * SCREEN_W + 80] != 0 && app.screen.px[95 * SCREEN_W + 79] == 0, "EGA: the first tile at x 80");
    menu_key(app.menu, PK_ENTER); frames(4);                /* the map */
    draw(CARD_TANDY);
    CHECK(box_colours(12, 90, 16, 94) == (1u << 12), "Tandy: the plan's cell light red");
    draw(CARD_EGA);
    CHECK(app.screen.px[92 * SCREEN_W + 12] == 12 && app.screen.px[92 * SCREEN_W + 15] == 15 && app.screen.px[94 * SCREEN_W + 13] == 15,
          "EGA: light red ORed over the cell and its grid lines");
    draw(CARD_CGA);
    CHECK(box_colours(12, 90, 16, 94) == (1u << 3), "CGA: the plan's cell in colour 3");

    /* the display line and the CGA cards: Tab sets the line, the line sets the card */
    {
        set_card(&app, CARD_CGA); menu_set_display(app.menu, 1); card_to_display(&app);
        CHECK(menu_options(app.menu)->display == 2, "plain CGA: tv/composite becomes rgb");
        next_mode(&app); CHECK(menu_options(app.menu)->display == 1, "Tab to the composite CGA: tv/composite");
        next_mode(&app); CHECK(menu_options(app.menu)->display == 1, "the new CGA's composite: tv/composite");
        while (current_card(&app) != CARD_CGA) next_mode(&app);
        CHECK(menu_options(app.menu)->display == 2, "round to the plain CGA: rgb");
        menu_set_display(app.menu, 0); next_mode(&app); while (current_card(&app) != CARD_CGA) next_mode(&app);
        CHECK(menu_options(app.menu)->display == 0, "black/white stays, round the cards");
        menu_set_display(app.menu, 2);

        menu_title(app.menu); frames(2);                    /* the Options Shoppe, the pointer on the display line */
        menu_key(app.menu, PK_EQUALS); frames(3);
        menu_key(app.menu, PK_ENTER); frames(30);
        for (int i = 0; i < 5; i++) { menu_key(app.menu, PK_EQUALS); frames(3); }
        menu_key(app.menu, PK_ENTER); frames(10);
        int before = menu_options(app.menu)->display;       /* rgb -> black/white: the card stays */
        menu_key(app.menu, PK_SPACE); display_to_card(&app, before); frames(2);
        CHECK(menu_options(app.menu)->display == 0 && current_card(&app) == CARD_CGA, "Space: black/white, still the plain CGA");
        before = menu_options(app.menu)->display;
        menu_key(app.menu, PK_SPACE); display_to_card(&app, before); frames(2);
        CHECK(menu_options(app.menu)->display == 1 && current_card(&app) == CARD_COMPOSITE, "Space: tv/composite shows the CGA on a TV");
        before = menu_options(app.menu)->display;
        menu_key(app.menu, PK_SPACE); display_to_card(&app, before); frames(2);
        CHECK(menu_options(app.menu)->display == 2 && current_card(&app) == CARD_CGA, "Space: rgb, the plain CGA again");

        /* Tab to a 16-colour card with the pointer on the display line: it moves to the fight delay */
        int delay = menu_options(app.menu)->fight_delay;
        set_card(&app, CARD_EGA); frames(2);
        menu_key(app.menu, PK_SPACE); frames(2);
        CHECK(menu_options(app.menu)->fight_delay != delay, "EGA: the pointer on the fight delay (%d -> %d)", delay, menu_options(app.menu)->fight_delay);
        set_card(&app, CARD_CGA);
    }

    menu_destroy(app.menu);
    assets_free(&app.ga);
    SDL_Log("%d failed", fails);
    return fails != 0;
}
