/*
 * The Ancient Art of War at Sea (1987) - remake
 * Nov 2026 using Claude Opus 5.5
 * "Guided" by Brett Hallen
 * Original game by Dave Murry & Barry Murry
 * Published by Brøderbund Software Inc.
 *
 * test/test_play.c: campaign play, step 2 (the map, the marker, the command
 * lines, Info, Fleet, Time, Pause, Surrender), scripted as it was done in the
 * original in DOSBox (WAS_Analysis/tools/dosbox): The Bane of Oldain, the
 * marker to F.D.Roosevelt's squadron (byte 3, line pair 186), Info, End, the
 * marker to (38, 150), Fleet, a key, Time, Pause, a key, Surrender. Checks
 * the text rows against what the original showed and writes a BMP of each
 * screen (compare_play.py compares them with the DOSBox screenshots).
 *
 * Usage: test_play GAME-FOLDER OUT-FOLDER
 */
#include <SDL3/SDL.h>
#include <stdarg.h>

#include "../src/assets.h"
#include "../src/campaign.h"
#include "../src/play.h"

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

static Screen scr;
static const char *outdir;

static void save(const Play *p, GfxMode mode, const char *name)
{
    static uint32_t rgb[SCREEN_W * SCREEN_H];
    play_draw(p, &scr, mode);
    screen_to_xrgb(&scr, rgb);
    SDL_Surface *surf = SDL_CreateSurfaceFrom(SCREEN_W, SCREEN_H, SDL_PIXELFORMAT_XRGB8888, rgb, SCREEN_W * 4);
    char path[1200];
    SDL_snprintf(path, sizeof path, "%s/%s.bmp", outdir, name);
    CHECK(surf && SDL_SaveBMP(surf, path), "can't write %s", path);
    SDL_DestroySurface(surf);
}

/* the text row r with trailing spaces removed */
static const char *row(const Play *p, int r)
{
    static char b[41];
    SDL_memcpy(b, p->text[r], 40);
    b[40] = 0;
    for (int i = 39; i >= 0 && b[i] == ' '; i--) b[i] = 0;
    return b;
}

static void press(Play *p, int key) { play_key(p, key, false); play_frame(p, 0); }

/* hold the keypad key toward (x, y) until the marker is there */
static void move_to(Play *p, int x, int y)
{
    for (int n = 0; n < 500 && (p->cx != x || p->cy != y); n++) {
        int dx = (x > p->cx) - (x < p->cx), dy = (y > p->cy) - (y < p->cy);
        play_frame(p, PK_HOME + (dy + 1) * 4 + (dx + 1));
    }
    CHECK(p->cx == x && p->cy == y, "marker at %d,%d not %d,%d", p->cx, p->cy, x, y);
    play_frame(p, 0);
}

int main(int argc, char **argv)
{
    if (argc < 3) { SDL_Log("usage: test_play GAME-FOLDER OUT-FOLDER"); return 2; }
    outdir = argv[2];
    static GameAssets ga;
    static Campaign cs[NUM_CAMPAIGNS];
    static Play p;
    char err[256];
    if (!assets_load(&ga, argv[1], err, sizeof err)) { SDL_Log("load: %s", err); return 1; }
    CHECK(ga.have_so5, "SO5");
    CHECK(campaigns_load(cs, argv[1]) == NUM_CAMPAIGNS, "campaigns");

    play_start(&p, &cs[0], &ga, 1234);
    CHECK(p.view_top == 109 && p.cx == 38 && p.cy == 145, "start: the bottom of the map, marker 38,145");
    CHECK(play_map_h(&p) == 182, "182 lines of map");
    CHECK(SDL_strcmp(row(&p, 24), "Time (slow)    Fleet  Pause  Surrender") == 0, "command line '%s'", row(&p, 24));
    save(&p, MODE_CGA, "p1_start");
    save(&p, MODE_16, "p1_start_16");

    move_to(&p, 3, 186);
    CHECK(p.mode == PM_SQUAD, "over a squadron");
    CHECK(SDL_strcmp(row(&p, 24), "Move Info Join Practice") == 0, "squadron line '%s'", row(&p, 24));
    save(&p, MODE_CGA, "p2_squadron");

    press(&p, PK_I);
    CHECK(p.mode == PM_INFO, "Info");
    CHECK(SDL_strcmp(row(&p, 24), "                                    End") == 0, "Info line '%s'", row(&p, 24));
    CHECK(SDL_strncmp(p.text[10] + 3, "F.D.ROOSEVELT", 13) == 0, "ship name");
    CHECK(SDL_strncmp(p.text[16] + 3, "SPEED: Anchored", 15) == 0, "speed");
    save(&p, MODE_CGA, "p3_info");
    save(&p, MODE_16, "p3_info_16");
    press(&p, PK_E);
    CHECK(p.mode == PM_SQUAD, "Info ended");

    move_to(&p, 38, 150);
    CHECK(p.mode == PM_NORMAL, "not over a squadron");
    press(&p, PK_F);
    CHECK(p.mode == PM_FLEET && p.text_rows == 4 && play_map_h(&p) == 166, "Fleet: 4 text rows");
    CHECK(SDL_strcmp(row(&p, 21), "Friend: 10Flag,  0Line,  2Frigate.") == 0, "fleet friend '%s'", row(&p, 21));
    CHECK(SDL_strcmp(row(&p, 22), "Foe:     7Flag, 11Line,  9Frigate.") == 0, "fleet foe '%s'", row(&p, 22));
    CHECK(SDL_strcmp(row(&p, 24), "End") == 0, "fleet End");
    save(&p, MODE_CGA, "p4_fleet");
    press(&p, PK_SPACE);
    CHECK(p.mode == PM_NORMAL && p.text_rows == 2, "Fleet ended");

    press(&p, PK_T);
    CHECK(SDL_strcmp(row(&p, 24), "Time (medium)  Fleet  Pause  Surrender") == 0, "time '%s'", row(&p, 24));
    press(&p, PK_T); press(&p, PK_T);
    CHECK(SDL_strcmp(row(&p, 24), "Time (faster)  Fleet  Pause  Surrender") == 0, "time '%s'", row(&p, 24));
    press(&p, PK_T);
    CHECK(p.time_div == 64, "time wraps to slow");

    press(&p, PK_P);
    CHECK(p.mode == PM_PAUSE && SDL_strcmp(row(&p, 24), "Press any key to continue.") == 0, "pause");
    press(&p, PK_SPACE);
    CHECK(p.mode == PM_NORMAL, "pause ended");

    press(&p, PK_S);
    CHECK(p.mode == PM_PROMPT && SDL_strcmp(row(&p, 24), "Surrender -- Are you sure? (Y or N)") == 0, "surrender '%s'", row(&p, 24));
    save(&p, MODE_CGA, "p5_surrender");
    press(&p, PK_N);
    CHECK(p.mode == PM_NORMAL, "not surrendered");

    /* scrolling: to the top with Shift+-, and back down a step at a time */
    play_key(&p, PK_MINUS, true); play_frame(&p, 0);
    CHECK(p.view_top == 0 && p.cx == 38 && p.cy == 45, "jump to the top");
    save(&p, MODE_CGA, "p6_top");
    for (int k = 0; k < 60; k++) play_frame(&p, PK_KP_PLUS);
    CHECK(p.view_top == VIEW_TOP_MAX - 1 || p.view_top == VIEW_TOP_MAX, "scrolled down to %d", p.view_top);
    CHECK(p.cy >= p.view_top && p.cy <= p.view_top + VIEW_MARGIN, "the marker stays in view");
    for (int k = 0; k < 400; k++) play_frame(&p, PK_PGDN);
    CHECK(p.cx == CX_MAX && p.cy == CY_MAX, "marker limits %d,%d", p.cx, p.cy);

    /* Esc, then surrender for real */
    press(&p, PK_ESC);
    CHECK(p.prompt_quit, "Esc asks");
    press(&p, PK_N);
    move_to(&p, 38, 150);
    press(&p, PK_S);
    press(&p, PK_Y);
    CHECK(p.ended && p.result_code == 11, "surrendered");
    CHECK(SDL_strcmp(row(&p, 24), "  Your opponent accepts your surrender") == 0, "accepted '%s'", row(&p, 24));
    press(&p, PK_ENTER);
    CHECK(p.result == PLAY_LOST, "game over");

    /* every campaign starts and draws; a hidden enemy is not selectable */
    for (int i = 0; i < NUM_CAMPAIGNS; i++) {
        play_start(&p, &cs[i], &ga, 1234);
        play_draw(&p, &scr, MODE_CGA);
        play_draw(&p, &scr, MODE_16);
        CHECK(p.result == PLAY_RUNNING, "S%d", i);
    }
    assets_free(&ga);
    SDL_Log("%s (%d failures)", fails ? "FAILED" : "passed", fails);
    return fails ? 1 : 0;
}
