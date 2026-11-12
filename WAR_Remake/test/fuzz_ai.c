/*
 * The Ancient Art of War (1984) - remake
 * Nov 2026 using Claude Opus 5.5
 * "Guided" by Brett Hallen
 * Original game by Dave Murry & Barry Murry
 * Published by Brøderbund Software Inc.
 *
 * test/fuzz_ai.c: a test or debugging aid for the remake (see README.md here)
 */
/* Fuzz: long games against the enemy commander, random keys, many seeds (ASan/UBSan). */
#include "../src/play.c"
static GameAssets ga; static Campaign camps[NUM_CAMPAIGNS]; static Play play;
int main(int argc, char **argv)
{
    (void)argc; char err[256];
    if (!assets_load(&ga, argv[1], err, sizeof err)) return 1;
    campaigns_load(camps, argv[1]);
    static const int keys[] = { PK_UP, PK_DOWN, PK_LEFT, PK_RIGHT, PK_M, PK_M, PK_C, PK_I, PK_E, PK_S, PK_T, PK_V, PK_N, PK_ENTER, PK_MINUS, PK_EQUALS, PK_D, PK_J, PK_F, PK_A, PK_K, PK_B, PK_E, PK_ESC, PK_ALT_S };
    uint32_t r = 99; long frames = 0; int saves = 0, games = 0, won = 0, lost = 0, splits = 0, maxroute = 0;
    for (int n = 0; n < 220; n++) {
        int c = n % NUM_CAMPAIGNS;
        if (!camps[c].loaded) continue;
        play_start(&play, &camps[c], &ga, (uint32_t)n * 7919u);
        play.time_div = 1;
        games++;
        int used0 = 0; for (int i = 0; i < MAX_SQUADS; i++) used0 += play.sq[i].used;
        for (int f = 0; f < 20000 && play.result == PLAY_RUNNING; f++) {
            r = r * 1103515245 + 12345;
            int k = 0;
            if ((r >> 16) % 6 == 0) k = keys[(r >> 20) % 25];
            if (play.ended && f % 50 == 0) k = PK_ENTER;
            if (k) play_key(&play, k, false);
            play_frame(&play, 0);
            if (play.result == PLAY_SAVE || play.result == PLAY_QUIT) { saves++; play_resume(&play); }
            frames++;
            int rb = 0; for (int q = 0; q < 200; q++) rb += play.route[q] != 0;
            if (rb > maxroute) maxroute = rb;
            for (int i = 0; i < MAX_SQUADS; i++) {
                const PSquad *q = &play.sq[i];
                if (q->used && (q->x < -2 || q->x > 80 || q->y < -2 || q->y > 200)) { SDL_Log("squad %d off the map (%d,%d) camp %d", i, q->x, q->y, c); return 2; }
            }
        }
        int used1 = 0; for (int i = 0; i < MAX_SQUADS; i++) used1 += play.sq[i].used;
        if (used1 > used0) splits++;
        if (play.result == PLAY_WON) won++;
        if (play.result == PLAY_LOST) lost++;
    }
    SDL_Log("%d saves/quits resumed, %d games, %ld frames, won %d lost %d, games with more squads at the end %d, most route bytes %d", saves, games, frames, won, lost, splits, maxroute);
    return 0;
}
