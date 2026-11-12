/*
 * The Ancient Art of War (1984) - remake
 * Nov 2026 using Claude Opus 5.5
 * "Guided" by Brett Hallen
 * Original game by Dave Murry & Barry Murry
 * Published by Brøderbund Software Inc.
 *
 * test/watch_ai.c: a test or debugging aid for the remake (see README.md here)
 */
/* Watch the enemy commander: each campaign runs with nobody at the keys. */
#include "../src/play.c"
static GameAssets ga; static Campaign camps[NUM_CAMPAIGNS]; static Play play; static Screen scr;
static uint32_t rgb[SCREEN_W * SCREEN_H];
static void shot(const char *dir, const char *name)
{
    play.reveal = true;
    play_draw(&play, &scr, MODE_VGA); screen_to_xrgb(&scr, rgb);
    SDL_Surface *s = SDL_CreateSurfaceFrom(SCREEN_W, SCREEN_H, SDL_PIXELFORMAT_XRGB8888, rgb, SCREEN_W * 4);
    char p[512]; SDL_snprintf(p, sizeof p, "%s/%s.bmp", dir, name); SDL_SaveBMP(s, p); SDL_DestroySurface(s);
    play.reveal = false;
}
int main(int argc, char **argv)
{
    (void)argc; char err[256];
    if (!assets_load(&ga, argv[2], err, sizeof err)) return 1;
    campaigns_load(camps, argv[2]);
    int only = argc > 3 ? SDL_atoi(argv[3]) : -1;
    for (int c = 0; c < NUM_CAMPAIGNS; c++) {
        if (only >= 0 && c != only) continue;
        if (!camps[c].loaded) continue;
        play_start(&play, &camps[c], &ga, 1234 + c);
        play.time_div = 1;
        int moved = 0, planned = 0, routes = 0;
        int sx[MAX_SQUADS], sy[MAX_SQUADS];
        for (int i = 0; i < MAX_SQUADS; i++) { sx[i] = play.sq[i].x; sy[i] = play.sq[i].y; }
        SDL_Log("== campaign %d (opponent %d): home flag %d cell %d, attack flag %d, target %d", c, camps[c].opponent,
                play.home_flag, play.home_cell, play.attack_flag, play.target_cell);
        int f;
        for (f = 1; f <= 6000 && play.result == PLAY_RUNNING && !play.ended; f++) {
            if (play.mode == PM_PROMPT && play.prompt_offer) play_key(&play, PK_N, false);
            play_frame(&play, 0);
            for (int i = 0; i < MAX_SQUADS; i++) {
                if (play.sq[i].planned) planned |= 1 << i;
                if (play.sq[i].on_route) routes |= 1 << i;
            }
            if (f % (argc > 4 ? SDL_atoi(argv[4]) : 1500) == 0) {
                char n[64]; SDL_snprintf(n, sizeof n, "c%d_f%04d", c, f);
                if (only >= 0) shot(argv[1], n);
                int en = 0, enmen = 0, my = 0, mymen = 0;
                for (int i = 0; i < MAX_SQUADS; i++) {
                    const PSquad *q = &play.sq[i];
                    if (!q->used) continue;
                    if (q->friendly) { my++; mymen += q->men; } else { en++; enmen += q->men; }
                }
                SDL_Log("  f%d: you %d squads/%d men, enemy %d/%d; target %d attack flag %d hold %d; lost %d/%d taken %d captured %d",
                        f, my, mymen, en, enmen, play.target_cell, play.attack_flag, play.ai_hold,
                        play.lost_mine, play.lost_theirs, play.flags_taken, play.flags_captured);
            }
        }
        for (int i = 0; i < MAX_SQUADS; i++)
            if (!play.sq[i].friendly && (play.sq[i].x != sx[i] || play.sq[i].y != sy[i])) moved++;
        for (int r = 18; r < 25; r++) SDL_Log("    %2d [%s]", r, play.text[r]);
        if (only >= 0) for (int i = 0; i < MAX_SQUADS; i++) {
            const PSquad *q = &play.sq[i];
            if (!q->used || q->friendly) continue;
            SDL_Log("    sq%d (%d,%d) cell %d tile %d men %d speed %d moving %d cond %d food %d route %d@%d planned %d dest %d value %d",
                    i, q->x, q->y, cell_of(q), tile_at_cell(&play, cell_of(q)), q->men, q->speed, q->moving, q->condition, q->food,
                    q->on_route, q->route_pos, q->planned, q->dest, play.cell_value[cell_of(q)]);
        }
        int route_bytes = 0;
        for (int k = 0; k < 200; k++) if (play.route[k]) route_bytes++;
        SDL_Log("  end after %d frames: %s; enemy squads that moved %d, planned mask %05x, route mask %05x, route bytes %d",
                f, play.ended ? (play.outcome == PLAY_WON ? "you won" : "you lost") : "running", moved, planned, routes, route_bytes);
    }
    return 0;
}
