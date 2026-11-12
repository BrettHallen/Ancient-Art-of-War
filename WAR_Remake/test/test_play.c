/*
 * The Ancient Art of War (1984) - remake
 * Nov 2026 using Claude Opus 5.5
 * "Guided" by Brett Hallen
 * Original game by Dave Murry & Barry Murry
 * Published by Brøderbund Software Inc.
 *
 * test/test_play.c: a test or debugging aid for the remake (see README.md here)
 */
/* Test harness (not part of the app): plays T0 through the play API with
 * scripted keys and saves screens as BMPs. */
#include <SDL3/SDL.h>
#include "../src/assets.h"
#include "../src/campaign.h"
#include "../src/gfx.h"
#include "../src/play.h"

static GameAssets ga;
static Campaign camps[NUM_CAMPAIGNS];
static Play play;
static Screen scr;
static uint32_t rgb[SCREEN_W * SCREEN_H];
static const char *outdir;
static GfxMode gmode = MODE_CGA;

static void shot(const char *name)
{
    play_draw(&play, &scr, gmode);
    screen_to_xrgb(&scr, rgb);
    SDL_Surface *s = SDL_CreateSurfaceFrom(SCREEN_W, SCREEN_H, SDL_PIXELFORMAT_XRGB8888, rgb, SCREEN_W * 4);
    char p[512];
    SDL_snprintf(p, sizeof p, "%s/%s.bmp", outdir, name);
    SDL_SaveBMP(s, p);
    SDL_DestroySurface(s);
}

static void frames(int n, int held) { while (n-- > 0) play_frame(&play, held); }
static void press(int k) { play_key(&play, k, false); play_frame(&play, 0); }

static void dump(const char *what)
{
    const PSquad *q = &play.sq[18];
    SDL_Log("%-22s mode=%d sel=%d cursor=(%d,%d) view=%d rows=%d | sq18 (%d,%d) moving=%d speed=%d path=%d pos=%d delay=%d cd=%d marked=%d",
            what, play.mode, play.sel, play.cx, play.cy, play.view_top, play.text_rows, q->x, q->y, q->moving, q->speed,
            q->has_path, q->path_pos, q->delay, q->countdown, q->marked);
    SDL_Log("    row23 [%s]", play.text[23]);
    SDL_Log("    row24 [%s]", play.text[24]);
}

int main(int argc, char **argv)
{
    if (argc < 3) return 1;
    outdir = argv[1];
    char err[256];
    if (!assets_load(&ga, argv[2], err, sizeof err)) { SDL_Log("assets: %s", err); return 1; }
    campaigns_load(camps, argv[2]);
    int ci = argc > 3 ? SDL_atoi(argv[3]) : 0;
    play_start(&play, &camps[ci], &ga, 7);
    dump("start"); shot("p00_start");

    frames(44, PK_DOWN);                  /* cursor down onto squad 18 at (36,189) */
    dump("down 44");
    frames(1, PK_LEFT);
    dump("left 1 (over squad)"); shot("p01_over_squad");

    press(PK_I); dump("info"); shot("p02_info");
    for (int i = 0; i < 40; i++) SDL_Log("    row%-2d [%s]", i, i < 25 ? play.text[i] : "");
    press(PK_S); dump("info speed"); shot("p03_info_speed");
    press(PK_S); press(PK_S);             /* back to march */
    press(PK_E); dump("end info");
    press(PK_S); dump("size"); shot("p04_size");
    press(PK_E); dump("end size");

    press(PK_M); dump("trace"); shot("p05_trace");
    frames(40, PK_UP);                    /* the box moves every other frame: 20 up */
    frames(20, PK_PGUP);                  /* 10 up and right */
    dump("traced"); shot("p06_traced");
    press(PK_M); dump("marked"); shot("p07_marked");

    press(PK_T); press(PK_T); press(PK_T);   /* slow -> medium -> fast -> faster */
    dump("time faster"); shot("p08_faster");
    for (int i = 0; i < 400 && play.sq[18].has_path; i++) {
        play_frame(&play, 0);
        if (i == 60) { dump("marching"); shot("p09_marching"); }
    }
    dump("arrived"); shot("p10_arrived");
    gmode = MODE_VGA; shot("p11_arrived_vga");
    press(PK_I); shot("p12_info_vga"); press(PK_E);

    /* path memory and messages */
    int used = 0;
    for (int i = 0; i < PATH_SIZE; i++) used += play.path[i] != 0;
    SDL_Log("path slots in use after arriving: %d", used);

    /* scroll to the top with Shift+- and surrender */
    play_key(&play, PK_MINUS, true); play_frame(&play, 0);
    dump("jump top"); shot("p13_top");
    press(PK_S); dump("surrender?"); shot("p14_surrender");
    press(PK_Y); dump("surrendered"); shot("p15_surrendered");
    SDL_Log("result %d", play.result);
    assets_free(&ga);
    return 0;
}
