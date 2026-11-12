/*
 * The Ancient Art of War (1984) - remake
 * Nov 2026 using Claude Opus 5.5
 * "Guided" by Brett Hallen
 * Original game by Dave Murry & Barry Murry
 * Published by Brøderbund Software Inc.
 *
 * test/fort_import.c: a test or debugging aid for the remake (see README.md here)
 */
/* The fort case from an original S: my 14 archers in a fort, 1 knight + 13 barbarians beside it.
 * Zoom without pressing A (as in the DOSBox runs); "They lost N men" per battle, many seeds. */
static long n_kill_pre, n_kill_post, n_arrow_pre, n_arrow_post; static int *cur_retreat;
#define BATTLE_TRACE(fmt, k, side, ...) (SDL_strncmp(fmt, "arrow", 5) == 0 && (side) == -1 ? (*cur_retreat ? n_kill_post++ : n_kill_pre++) : 0)
#include "../src/play.c"
#include "../src/sound.h"
#define snd_arrow() ((*cur_retreat ? n_arrow_post++ : n_arrow_pre++), snd_arrow())
#include "../src/battle.c"
#include "../src/savegame.h"
static GameAssets ga; static Campaign camps[NUM_CAMPAIGNS], game; static Play p; static Battle b;
int main(int argc, char **argv)
{
    (void)argc; char err[256];
    assets_load(&ga, argv[1], err, sizeof err); campaigns_load(camps, argv[1]);
    size_t n; uint8_t *s = SDL_LoadFile(argv[2], &n);
    int runs = atoi(argv[3]), hist[16] = { 0 }; long total = 0, frames = 0;
    for (int r = 0; r < runs; r++) {
        if (!savegame_import(&p, &game, &ga, s, n, 1000 + r)) { SDL_Log("import failed"); return 1; }
        p.fight_delay = 400;
        play_resume(&p);
        if (r == 0) SDL_Log("sq16 %d,%d a%d tile %d; sq1 %d,%d k%d b%d", p.sq[16].x, p.sq[16].y, p.sq[16].archers, play_tile_under(&p, 16), p.sq[1].x, p.sq[1].y, p.sq[1].knights, p.sq[1].barbarians);
        int f = 0;
        while (f++ < 2000 && !(p.sq[16].fighting && p.sq[16].clock > 0)) play_frame(&p, 0);
        if (!p.sq[16].fighting) { SDL_Log("run %d: no encounter", r); continue; }
        cur_retreat = &b.retreat; b.retreat = 0;
        if (!battle_start(&b, &p, 16)) { SDL_Log("run %d: no battle", r); continue; }
        if (r == 0) SDL_Log("bg %d me %d en %d skill %d/%d", b.bg, b.me, b.en, b.skill_me, b.skill_en);
        int k = 0;
        while (!b.done && k++ < 5000) battle_frame(&b);
        frames += k;
        hist[SDL_min(b.lost_en, 15)]++; total += b.lost_en;
    }
    SDL_Log("%d battles: they lost %.2f men on average, %ld frames each", runs, (double)total / runs, frames / runs);
    char line[200]; int c = 0; for (int i = 0; i < 15; i++) c += SDL_snprintf(line + c, sizeof line - c, "%d:%d ", i, hist[i]); SDL_Log("%s", line);
    SDL_Log("arrows before the enemy gives up %ld (kills %ld), after %ld (kills %ld)", n_arrow_pre, n_kill_pre, n_arrow_post, n_kill_post);
    return 0;
}
