/*
 * The Ancient Art of War (1984) - remake
 * Nov 2026 using Claude Opus 5.5
 * "Guided" by Brett Hallen
 * Original game by Dave Murry & Barry Murry
 * Published by Brøderbund Software Inc.
 *
 * test/fuzz_play.c: a test or debugging aid for the remake (see README.md here)
 */
/* Random key presses on every campaign, under ASan/UBSan. */
#include <SDL3/SDL.h>
#include "../src/play.h"
static GameAssets ga; static Campaign camps[NUM_CAMPAIGNS]; static Play p; static Screen s;
int main(int argc, char **argv)
{
    char err[256];
    if (argc < 2 || !assets_load(&ga, argv[1], err, sizeof err)) return 1;
    campaigns_load(camps, argv[1]);
    static const int keys[] = { PK_M, PK_M, PK_C, PK_T, PK_I, PK_S, PK_E, PK_N, PK_MINUS, PK_EQUALS, PK_KP_MINUS, PK_KP_PLUS, PK_D, PK_V, PK_Z, PK_V, PK_ENTER };
    unsigned r = 12345;
    long marks = 0, steps = 0, lost = 0;
    for (int c = 0; c < NUM_CAMPAIGNS; c++) {
        for (int run = 0; run < 3; run++) {
            play_start(&p, &camps[c], &ga, (uint32_t)run);
            int held = 0;
            for (int f = 0; f < 30000 && p.result == PLAY_RUNNING; f++) {
                r = r * 1103515245u + 12345u;
                unsigned v = (r >> 16) % 100;
                if (v < 6) play_key(&p, keys[(r >> 8) % (sizeof keys / sizeof keys[0])], (r & 1) != 0);
                if (v >= 6 && v < 20) held = PK_HOME + (int)((r >> 4) % 11);
                else if (v >= 20 && v < 30) held = 0;
                if (v >= 97 && (p.mode == PM_NORMAL || p.mode == PM_SQUAD)) {   /* jump onto a squad */
                    int i = (int)((r >> 9) % MAX_SQUADS);
                    if (p.sq[i].used && p.sq[i].friendly && p.sq[i].x < 0x4E) {
                        p.cx = p.sq[i].x; p.cy = p.sq[i].y;
                        p.view_top = SDL_clamp(p.cy - 40, 0, VIEW_TOP_MAX);
                        play_frame(&p, PK_RIGHT);
                        if (p.mode == PM_SQUAD) { play_key(&p, PK_M, false); play_frame(&p, 0); }
                    }
                }
                if (p.mode == PM_SIZE || p.mode == PM_INFO || p.mode == PM_VIEW) play_key(&p, PK_E, false);
                int had = 0; for (int i = 0; i < MAX_SQUADS; i++) had += p.sq[i].has_path;
                play_frame(&p, held);
                if ((f & 255) == 0) play_draw(&p, &s, (f & 256) ? MODE_VGA : MODE_CGA);
                for (int i = 0; i < MAX_SQUADS; i++) {
                    const PSquad *q = &p.sq[i];
                    if (!q->used) continue;
                    if (q->x < -2 || q->x > 82 || q->y < -2 || q->y > 202) { SDL_Log("squad %d out of map (%d,%d) c=%d", i, q->x, q->y, c); return 2; }
                }
                int now = 0; for (int i = 0; i < MAX_SQUADS; i++) now += p.sq[i].has_path;
                if (now > had) marks++;
                steps++;
            }
            if (p.result == PLAY_LOST) lost++;
            SDL_Log("  campaign %d run %d: result %d after flags taken %d captured %d, lost mine %d theirs %d", c, run, p.result, p.flags_taken, p.flags_captured, p.lost_mine, p.lost_theirs);
        }
        int used = 0; for (int i = 0; i < PATH_SIZE; i++) used += p.path[i] != 0;
        int alive = 0, men = 0; for (int i = 0; i < MAX_SQUADS; i++) if (p.sq[i].used && p.sq[i].friendly) { alive++; men += p.sq[i].men; }
        SDL_Log("campaign %d done, path slots used %d, result %d, friendly squads %d men %d, rows %d", c, used, p.result, alive, men, p.text_rows);
    }
    SDL_Log("frames %ld, orders %ld, lost/surrendered %ld", steps, marks, lost);
    return 0;
}
