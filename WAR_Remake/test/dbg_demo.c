/*
 * The Ancient Art of War (1984) - remake
 * Nov 2026 using Claude Opus 5.5
 * "Guided" by Brett Hallen
 * Original game by Dave Murry & Barry Murry
 * Published by Brøderbund Software Inc.
 *
 * test/dbg_demo.c: a test or debugging aid for the remake (see README.md here)
 */
#include "../src/menu.h"
#include <SDL3/SDL.h>
static GameAssets ga; static Campaign camps[NUM_CAMPAIGNS]; static Play play; static Campaign game;
int main(int argc, char **argv)
{
    char err[256]; (void)argc;
    assets_load(&ga, argv[1], err, sizeof err); campaigns_load(camps, argv[1]);
    for (int c = 3; c <= 10; c++) {
        game = camps[c]; game.opponent = 0; game.rules[RULE_ENEMY_SEEN] = 80;
        play_start_demo(&play, &game, &ga, 99);
        int f = 0;
        for (; f < 4000 && play.result == PLAY_RUNNING && play.zoom_squad < 0; f++) play_frame(&play, 0);
        int mine = 0, theirs = 0, wf = 0, bf = 0;
        for (int i = 0; i < MAX_SQUADS; i++) if (play.sq[i].used) { if (play.sq[i].friendly) mine++; else theirs++; }
        for (int i = 0; i < MAX_FLAGS; i++) if (play.flag[i].present) { if (play.flag[i].white) wf++; else bf++; }
        SDL_Log("%s %-28s f %4d result %d ended %d offer %d code %d zoom %d mode %d  squads %d/%d flags %d/%d %s",
            camps[c].file, camps[c].title, f, play.result, play.ended, play.prompt_offer, play.result_code, play.zoom_squad, play.mode, mine, theirs, wf, bf,
            play.text[24]);
    }
}
