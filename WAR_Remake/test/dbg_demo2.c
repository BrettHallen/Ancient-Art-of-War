/*
 * The Ancient Art of War (1984) - remake
 * Nov 2026 using Claude Opus 5.5
 * "Guided" by Brett Hallen
 * Original game by Dave Murry & Barry Murry
 * Published by Brøderbund Software Inc.
 *
 * test/dbg_demo2.c: a test or debugging aid for the remake (see README.md here)
 */
#include "../src/menu.h"
#include <SDL3/SDL.h>
static GameAssets ga; static Campaign camps[NUM_CAMPAIGNS]; static Play play; static Campaign game;
int main(int argc, char **argv)
{
    char err[256]; (void)argc;
    assets_load(&ga, argv[1], err, sizeof err); campaigns_load(camps, argv[1]);
    int c = SDL_atoi(argv[2]);
    game = camps[c]; game.opponent = 0; game.rules[RULE_ENEMY_SEEN] = 80;
    play_start_demo(&play, &game, &ga, SDL_atoi(argv[3]));
    for (int i = 0; i < MAX_SQUADS; i++) if (play.sq[i].used) SDL_Log("sq %d %s x %d y %d men %d spies %d small %d", i, play.sq[i].friendly ? "MINE " : "enemy", play.sq[i].x, play.sq[i].y, play.sq[i].men, play.sq[i].spies, play.sq[i].small);
    for (int i = 0; i < MAX_FLAGS; i++) if (play.flag[i].present) SDL_Log("flag %d %s x %d y %d", i, play.flag[i].white ? "white" : "black", play.flag[i].x, play.flag[i].y);
    char last[3][41] = {{0}};
    int f = 0;
    for (; f < 4000 && play.result == PLAY_RUNNING && play.zoom_squad < 0; f++) {
        play_frame(&play, 0);
        for (int r = 22; r <= 24; r++) if (SDL_strcmp(last[r-22], play.text[r])) { SDL_strlcpy(last[r-22], play.text[r], 41); if (r < 24) SDL_Log("f %4d tick-ish row %d: %s", f, r, play.text[r]); }
    }
    SDL_Log("end f %d result %d code %d zoom %d", f, play.result, play.result_code, play.zoom_squad);
}
