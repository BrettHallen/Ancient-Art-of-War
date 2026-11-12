/*
 * The Ancient Art of War at Sea (1987) - remake
 * Nov 2026 using Claude Opus 5.5
 * "Guided" by Brett Hallen
 * Original game by Dave Murry & Barry Murry
 * Published by Brøderbund Software Inc.
 *
 * test/fuzz_play.c: random keys in campaign play, every campaign, under
 * AddressSanitizer (build.sh). Checks that the marker and the view stay in
 * their limits and that every frame draws.
 *
 * Usage: fuzz_play GAME-FOLDER [FRAMES-PER-CAMPAIGN]
 */
#include <SDL3/SDL.h>

#include "../src/assets.h"
#include "../src/campaign.h"
#include "../src/play.h"

static uint32_t seed = 4321;
static uint32_t rnd(void) { seed = seed * 1103515245u + 12345u; return seed >> 8; }

static const int KEYS[] = {
    PK_ESC, PK_MINUS, PK_EQUALS, PK_TAB, PK_E, PK_T, PK_Y, PK_I, PK_P, PK_ENTER, PK_CTRL, PK_S, PK_D, PK_F,
    PK_J, PK_Z, PK_C, PK_N, PK_M, PK_SPACE, PK_HOME, PK_UP, PK_PGUP, PK_KP_MINUS, PK_LEFT, PK_KP_5, PK_RIGHT,
    PK_KP_PLUS, PK_END, PK_DOWN, PK_PGDN, PK_ALT_S, 0x77,
};

int main(int argc, char **argv)
{
    if (argc < 2) { SDL_Log("usage: fuzz_play GAME-FOLDER [FRAMES]"); return 2; }
    int frames = argc > 2 ? SDL_atoi(argv[2]) : 20000;
    static GameAssets ga;
    static Campaign cs[NUM_CAMPAIGNS];
    static Play p;
    static Screen s;
    char err[256];
    if (!assets_load(&ga, argv[1], err, sizeof err)) { SDL_Log("load: %s", err); return 1; }
    campaigns_load(cs, argv[1]);
    int bad = 0, ended = 0;
    for (int c = 0; c < NUM_CAMPAIGNS; c++) {
        play_start(&p, &cs[c], &ga, 1234);
        for (int f = 0; f < frames; f++) {
            if (p.result != PLAY_RUNNING) { ended++; play_start(&p, &cs[c], &ga, 1234); }
            uint32_t r = rnd() % 10;
            if (r < 3) play_key(&p, KEYS[rnd() % (sizeof KEYS / sizeof KEYS[0])], rnd() % 4 == 0);
            int held = (r >= 3 && r < 8) ? PK_HOME + (int)(rnd() % 11) : 0;
            play_frame(&p, held);
            if (f % 7 == 0) play_draw(&p, &s, (GfxMode)(f % 2));
            if (p.cx < 1 || p.cx > CX_MAX || p.cy < 0 || p.cy > CY_MAX || p.view_top < 0 || p.view_top > VIEW_TOP_MAX ||
                p.cy < p.view_top || p.cy > p.view_top + VIEW_MARGIN || p.text_rows < 2 || p.text_rows > 25) {
                if (bad++ < 10) SDL_Log("S%d frame %d: marker %d,%d view %d rows %d", c, f, p.cx, p.cy, p.view_top, p.text_rows);
            }
        }
    }
    /* the rules: no Esc or Y (the game goes on), the clock at its fastest */
    int paths = 0, ends = 0, lost = 0, msgs = 0;
    for (int c = 0; c < NUM_CAMPAIGNS; c++) {
        play_start(&p, &cs[c], &ga, 99 + c);
        for (int f = 0; f < frames; f++) {
            if (p.result != PLAY_RUNNING) {
                ends++;
                lost += p.losses[0] + p.losses[1];
                play_start(&p, &cs[c], &ga, 99 + c + f);
            }
            if (f == frames - 1) lost += p.losses[0] + p.losses[1];
            if (p.msg_rows && p.text[25 - p.text_rows][0] != ' ' && p.stall == 1) msgs++;
            if (p.time_div != 1 && (p.mode == PM_NORMAL)) play_key(&p, PK_T, false);
            if (f % 150 == 0 && (p.mode == PM_NORMAL || p.mode == PM_SQUAD) && !p.stall) {   /* a squadron, M, a path, M */
                int i = (int)(rnd() % MAX_SHIPS);
                if (p.sq[i].used && p.sq[i].side == -1 && p.sq[i].x > 2) {
                    p.cx = p.sq[i].x - 1;
                    p.cy = p.sq[i].y + 1;
                    p.view_top = SDL_clamp(p.cy - 40, 0, VIEW_TOP_MAX);
                    play_frame(&p, PK_RIGHT);
                    play_key(&p, PK_M, false);
                    play_frame(&p, 0);
                    int dir = PK_HOME + (int)(rnd() % 11);
                    for (int t = 0; t < 400 && p.mode == PM_TRACE; t++) {
                        play_frame(&p, t < 60 ? dir : 0);
                        if (t == 60) play_key(&p, PK_M, false);
                    }
                }
            }
            uint32_t r = rnd() % 10;
            int k = KEYS[rnd() % (sizeof KEYS / sizeof KEYS[0])];
            if (r < 2 && k != PK_ESC && k != PK_Y) play_key(&p, k, false);
            if (p.mode == PM_END && r == 9) play_key(&p, PK_ENTER, false);
            int held = (r >= 3 && r < 7) ? PK_HOME + (int)(rnd() % 11) : 0;
            play_frame(&p, held);
            if (f % 13 == 0) play_draw(&p, &s, (GfxMode)(f % 2));
            for (int i = 0; i < MAX_SHIPS; i++) {   /* a sailing squadron's path ends with 99 */
                const PSquad *q = &p.sq[i];
                if (!q->used || !q->has_path) continue;
                paths++;
                int k2 = q->path_pos, n = 0;
                while (n < PATH_SIZE && p.path[k2] != PATH_END && p.path[k2] != 0) { k2 = k2 + 1 > PATH_SIZE - 1 ? 0 : k2 + 1; n++; }
                if (p.path[k2] != PATH_END && bad++ < 10) SDL_Log("S%d frame %d: squadron %d's path is broken", c, f, i);
                if (q->x < 0 || q->x > 79 || q->y < -2 || q->y > 199) { if (bad++ < 10) SDL_Log("S%d: squadron %d at %d,%d", c, i, q->x, q->y); }
            }
            for (int i = 0; i < MAX_SHIPS; i++)
                if (p.sq[i].used && (p.sq[i].supplies < 0 || p.sq[i].supplies > 20) && bad++ < 10) SDL_Log("S%d: supplies %d", c, p.sq[i].supplies);
        }
    }
    SDL_Log("fuzz_play rules: %d games ended, %d path checks, %d ships lost, %d messages", ends, paths, lost, msgs);
    assets_free(&ga);
    SDL_Log("fuzz_play: %d frames a campaign, %d games ended, %d problems", frames, ended, bad);
    return bad ? 1 : 0;
}
