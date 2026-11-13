/*
 * The Ancient Art of War at Sea (1987) - remake
 * Nov 2026 using Claude Opus 5.5
 * "Guided" by Brett Hallen
 * Original game by Dave Murry & Barry Murry
 * Published by Brøderbund Software Inc.
 *
 * test/fuzz_battle.c: random keys in the sea battle and the boarding, under
 * AddressSanitizer (build.sh). Each round puts GEO.WASHINGTON's squadron
 * beside MACABRE's (as test_battle's z scenes), Zooms in and presses random
 * keys, Zoom whenever a ship grapples another, so most rounds board; the
 * last rounds are Practice. Checks that every battle ends and draws.
 *
 * Usage: fuzz_battle GAME-FOLDER [ROUNDS]
 */
#include <SDL3/SDL.h>

#include "../src/assets.h"
#include "../src/battle.h"
#include "../src/campaign.h"
#include "../src/play.h"

static uint32_t seed = 2468;
static uint32_t rnd(void) { seed = seed * 1103515245u + 12345u; return seed >> 8; }

/* the battle's keys, and the boarding's (A F Q P R C, Esc, Backspace, L) */
static const char KEYS[] = "gcsoafvtpl +=-_123456789\r" "AFQPRCzZ\x1b\x08";

static void settle(Play *p) { for (int n = 0; n < 1000 && play_busy(p); n++) play_frame(p, 0); }

int main(int argc, char **argv)
{
    if (argc < 2) { SDL_Log("usage: fuzz_battle GAME-FOLDER [ROUNDS]"); return 2; }
    int rounds = argc > 2 ? SDL_atoi(argv[2]) : 12;
    static GameAssets ga;
    static Campaign cs[NUM_CAMPAIGNS], c0;
    static Play p;
    static Battle b;
    static Screen s;
    char err[256];
    if (!assets_load(&ga, argv[1], err, sizeof err)) { SDL_Log("load: %s", err); return 1; }
    campaigns_load(cs, argv[1]);
    int bad = 0, boards = 0, battles = 0;
    for (int r = 0; r < rounds; r++) {
        bool practice = r >= rounds - 2;
        c0 = cs[0];
        c0.squadrons[16].x = 44; c0.squadrons[16].y = 145;
        c0.squadrons[7].x = 44; c0.squadrons[7].y = 141;
        play_start(&p, &c0, &ga, 500 + r);
        /* the marker to the squadron, then wait for the encounter */
        for (int n = 0; n < 900 && (p.cx != 44 || p.cy != 145); n++) {
            int dx = (44 > p.cx) - (44 < p.cx), dy = (145 > p.cy) - (145 < p.cy);
            settle(&p);
            play_frame(&p, PK_HOME + (dy + 1) * 4 + (dx + 1));
        }
        settle(&p);
        play_frame(&p, 0);
        if (practice) {
            p.sq[16].partner = 0;
            p.sq[16].ctrl = -2;
            settle(&p);
            play_key(&p, PK_P, false);
        } else {
            for (int f = 0; f < 400 && !(p.sq[16].partner && p.sq[16].ctrl > 0); f++) play_frame(&p, 0);
            settle(&p);
            play_key(&p, PK_Z, false);
        }
        play_frame(&p, 0);
        settle(&p);
        if (p.battle_req == 0) { SDL_Log("round %d: no battle", r); continue; }
        if (!battle_start(&b, &p, p.battle_req == 2)) { SDL_Log("round %d: battle_start failed", r); bad++; continue; }
        battles++;
        battle_key(&b, 't', 0);
        bool in_board = false;
        int f = 0;
        for (; f < 60 * 60 * 8 && b.state == BATTLE_RUNNING; f++) {
            if (b.menu == 'B') {
                if (!in_board) boards++;
                in_board = true;
                if (rnd() % 6 == 0) battle_key(&b, (rnd() % 3) ? 'z' : KEYS[rnd() % (sizeof KEYS - 1)], 0);
            } else {
                in_board = false;
                if (rnd() % 40 == 0) {
                    char k = KEYS[rnd() % (sizeof KEYS - 1)];
                    if (k == 'l' && f < 60 * 60 * 4) k = 't';   /* don't Leave too soon */
                    battle_key(&b, k, 0);
                }
                if (rnd() % 400 == 0) battle_key(&b, 0, 0x47 + (int)(rnd() % 11));   /* keypad */
            }
            battle_run(&b, 16667);
            if (f % 5 == 0) battle_draw(&b, &s, (GfxMode)(f % 2));
        }
        /* Leave (with Enter in between to close a question; a boarding first ends by itself) */
        for (int k = 0; k < 60 * 300 && b.state == BATTLE_RUNNING; k++) {
            if (k % 120 == 0 && b.menu != 'B') battle_key(&b, 'l', 0);   /* a key read flushes the rest: one at a time */
            if (k % 120 == 60) battle_key(&b, '\r', 0);
            battle_run(&b, 16667);
        }
        if (b.state == BATTLE_RUNNING) { SDL_Log("round %d: the battle doesn't end (menu %d, clock %.0f s, count %d result %d time %d view %d ending %d keys %d-%d)", r, b.menu, b.clock_us / 1e6, (int16_t)(b.m[0x64DD] | b.m[0x64DE] << 8), b.m[0x6DC1], b.time, b.view, b.ending, b.key_head, b.key_tail); bad++; }
        battle_finish(&b);
        SDL_Log("round %d: %s, %d s, boardings so far %d, owners %d %d %d %d %d %d", r, practice ? "practice" : "zoom",
                f / 60, boards, b.owner[0], b.owner[1], b.owner[2], b.owner[3], b.owner[4], b.owner[5]);
        for (int k = 0; k < 200; k++) play_frame(&p, 0);
        play_draw(&p, &s, MODE_CGA);
    }
    SDL_Log("%d battles, %d boardings", battles, boards);
    if (bad || boards == 0) { SDL_Log("FAILED (%d)", bad); return 1; }
    SDL_Log("all passed");
    return 0;
}
