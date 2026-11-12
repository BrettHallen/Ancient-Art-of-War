/*
 * The Ancient Art of War (1984) - remake
 * Nov 2026 using Claude Opus 5.5
 * "Guided" by Brett Hallen
 * Original game by Dave Murry & Barry Murry
 * Published by Brøderbund Software Inc.
 *
 * test/fuzz_battle.c: a test or debugging aid for the remake (see README.md here)
 */
/* Fuzz: many Zoom battles with random keys (run under ASan/UBSan). */
#include "../src/play.c"
#include "../src/battle.c"
static GameAssets ga; static Campaign camps[NUM_CAMPAIGNS]; static Play play; static Battle bt; static Screen scr;
int main(int argc, char **argv)
{
    (void)argc; char err[256];
    if (!assets_load(&ga, argv[1], err, sizeof err)) return 1;
    campaigns_load(camps, argv[1]);
    static const int keys[] = { PK_A, PK_B, PK_K, PK_S, PK_F, PK_R, PK_BACKSPACE, PK_Z, PK_ENTER };
    uint32_t r = 12345; int battles = 0, maxf = 0, retreats = 0, wins = 0, losses = 0;
    for (int n = 0; n < 600; n++) {
        int c = n % NUM_CAMPAIGNS;
        if (!camps[c].loaded) continue;
        play_start(&play, &camps[c], &ga, (uint32_t)n);
        int fr[MAX_SQUADS], en[MAX_SQUADS], nf = 0, ne = 0;
        for (int i = 0; i < MAX_SQUADS; i++) {
            const PSquad *q = &play.sq[i];
            if (!q->used || q->knights + q->archers + q->barbarians == 0) continue;
            if (q->friendly) fr[nf++] = i; else en[ne++] = i;
        }
        if (!nf || !ne) continue;
        r = r * 1103515245 + 12345; int a = fr[(r >> 16) % nf];
        r = r * 1103515245 + 12345; int e = en[(r >> 16) % ne];
        PSquad *A = &play.sq[a], *E = &play.sq[e];
        A->x = E->x + 1; A->y = E->y + 3;
        play.time_div = 1;
        for (int f = 0; f < 50 && !A->fighting; f++) play_frame(&play, 0);
        if (!A->fighting || A->foe != e) continue;
        int sel = (n & 1) ? a : e;      /* Zoom on either squad */
        if (!battle_start(&bt, &play, sel)) return 2;
        battles++;
        int f;
        for (f = 0; f < 5000 && !bt.done; f++) {
            r = r * 1103515245 + 12345;
            if ((r >> 16) % 4 == 0) {
                int k = keys[(r >> 20) % 9];
                if (k == PK_R && (r >> 24) % 8) k = PK_A;   /* retreat less often */
                battle_key(&bt, k);
            }
            battle_frame(&bt);
            if (f % 7 == 0) battle_draw(&bt, &scr, (f & 8) ? MODE_VGA : MODE_CGA);
        }
        if (!bt.done) { SDL_Log("battle %d did not end (camp %d, bg %d, retreat %d, endtimer %d)", n, c, bt.bg, bt.retreat, bt.endtimer); return 3; }
        if (f > maxf) maxf = f;
        if (bt.retreat) retreats++;
        int me = bt.me, them = bt.en;
        battle_finish(&bt);
        if (play.sq[them].men == 0) wins++;
        if (play.sq[me].men == 0) losses++;
        for (int k = 0; k < 200 && play.result == PLAY_RUNNING; k++) play_frame(&play, 0);
    }
    SDL_Log("%d battles, longest %d frames, %d retreats, %d enemy wiped out, %d you wiped out", battles, maxf, retreats, wins, losses);
    assets_free(&ga);
    return 0;
}
