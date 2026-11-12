/*
 * The Ancient Art of War (1984) - remake
 * Nov 2026 using Claude Opus 5.5
 * "Guided" by Brett Hallen
 * Original game by Dave Murry & Barry Murry
 * Published by Brøderbund Software Inc.
 *
 * test/route_dbg.c: a test or debugging aid for the remake (see README.md here)
 */
#include "../src/play.c"
static GameAssets ga; static Campaign camps[NUM_CAMPAIGNS]; static Play play;
int main(int argc, char **argv)
{
    (void)argc; char err[256]; assets_load(&ga, argv[1], err, sizeof err); campaigns_load(camps, argv[1]);
    int c = SDL_atoi(argv[2]);
    play_start(&play, &camps[c], &ga, 5);
    for (int i = 0; i < MAX_SQUADS; i++) {
        PSquad *q = &play.sq[i];
        if (!q->used || q->friendly || !q->on_route) continue;
        printf("squad %d at cell %d dest %d route:", i, cell_of(q), q->dest);
        for (int k = q->route_pos; k < 200 && play.route[k] && play.route[k] != 99; k++) printf(" %d", play.route[k]);
        printf("\n");
    }
    printf("costs (# water, ^ mountain, f forest, F fort, v village, digits = cost) and squads (E enemy, Y you, * flag W/B)\n");
    for (int row = 0; row < 20; row++) {
        for (int col = 0; col < 10; col++) {
            int cell = row * 10 + col, t = tile_at_cell(&play, cell);
            char ch = tile_water(t) ? '#' : tile_mountain(t) ? '^' : tile_forest(t) ? 'f' : tile_fort(t) ? 'F' : tile_village(t) ? 'v' : tile_bridge(t) ? '=' : '.';
            char sq = ' ';
            for (int i = 0; i < MAX_SQUADS; i++) if (play.sq[i].used && cell_of(&play.sq[i]) == cell) sq = play.sq[i].friendly ? 'Y' : 'E';
            for (int f = 0; f < MAX_FLAGS; f++) if (play.flag[f].present && flag_cell(&play, f) == cell) sq = play.flag[f].white ? 'W' : 'B';
            printf("%c%c%3d ", ch, sq, play.cell_value[cell]);
        }
        printf("\n");
    }
    printf("home cell %d target %d attack flag %d (cell %d)\n", play.home_cell, play.target_cell, play.attack_flag, flag_cell(&play, play.attack_flag));
}
