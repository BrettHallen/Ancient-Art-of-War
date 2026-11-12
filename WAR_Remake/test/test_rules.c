/*
 * The Ancient Art of War (1984) - remake
 * Nov 2026 using Claude Opus 5.5
 * "Guided" by Brett Hallen
 * Original game by Dave Murry & Barry Murry
 * Published by Brøderbund Software Inc.
 *
 * test/test_rules.c: a test or debugging aid for the remake (see README.md here)
 */
/* Test harness (not part of the app): supply, food, training, flags, View,
 * the end of a game and the opponent's surrender offer. Includes play.c to
 * reach its internal functions. */
#include "../src/play.c"

static GameAssets ga;
static Campaign camps[NUM_CAMPAIGNS];
static Play play;
static Screen scr;
static uint32_t rgb[SCREEN_W * SCREEN_H];
static const char *outdir;

static void shot(const char *name, GfxMode m)
{
    play_draw(&play, &scr, m);
    screen_to_xrgb(&scr, rgb);
    SDL_Surface *s = SDL_CreateSurfaceFrom(SCREEN_W, SCREEN_H, SDL_PIXELFORMAT_XRGB8888, rgb, SCREEN_W * 4);
    char p[512];
    SDL_snprintf(p, sizeof p, "%s/%s.bmp", outdir, name);
    SDL_SaveBMP(s, p);
    SDL_DestroySurface(s);
}
static void press(int k) { play_key(&play, k, false); play_frame(&play, 0); }
static void rows(void) { for (int r = 25 - play.text_rows; r < 25; r++) SDL_Log("  %2d [%s]", r, play.text[r]); }
static void ticks(int n) { play.time_div = 1; for (int i = 0; i < n; i++) play_frame(&play, 0); }

static void squads(const char *what)
{
    SDL_Log("%s", what);
    for (int i = 0; i < MAX_SQUADS; i++) {
        const PSquad *q = &play.sq[i];
        if (!q->used) continue;
        SDL_Log("  %2d %s (%2d,%3d) men %2d a%2d k%2d b%2d s%d cond %2d food %2d moving %d speed %d supply-dist %3d cell %3d tile %2d",
                i, q->friendly ? "F" : "E", q->x, q->y, q->men, q->archers, q->knights, q->barbarians, q->spies,
                q->condition, q->food, q->moving, q->speed, supply_distance(&play, i), cell_of(q), tile_at_cell(&play, cell_of(q)));
    }
}

static void supply_grid(void)
{
    for (int r = 0; r < MAP_H; r++) {
        char line[200]; int n = 0;
        for (int c = 0; c < MAP_W; c++) n += SDL_snprintf(line + n, sizeof line - n, "%5d%c", play.supply[r * 10 + c],
                                                            tile_village(play.c->map[r][c]) ? 'v' : tile_fort(play.c->map[r][c]) ? 'F' : ' ');
        SDL_Log("  %s", line);
    }
}

int main(int argc, char **argv)
{
    if (argc < 3) return 1;
    outdir = argv[1];
    char err[256];
    if (!assets_load(&ga, argv[2], err, sizeof err)) { SDL_Log("assets: %s", err); return 1; }
    campaigns_load(camps, argv[2]);

    /* T0: supply map, View, food over time */
    play_start(&play, &camps[0], &ga, 7);
    SDL_Log("T0 rules: villages %d forts %d range %d train %d shape %d", rule(&play, 0), rule(&play, 1), rule(&play, 2), rule(&play, 3), rule(&play, 4));
    SDL_Log("opponent %d: A4EA %d A4FA %d A4FC %d", play.c->opponent, OPP(&play, 0xA4EA), OPP(&play, 0xA4FA), OPP(&play, 0xA4FC));
    supply_grid();
    squads("start");
    press(PK_V); rows(); shot("r01_view", MODE_CGA); shot("r02_view_vga", MODE_VGA);
    press(PK_E);
    ticks(16 * 10);                       /* ten food rounds, standing still */
    squads("after 160 ticks");

    /* march a squad fast for a while: condition falls */
    PSquad *q = &play.sq[18];
    play.cx = q->x + 1; play.cy = q->y; check_over(&play, 1);
    press(PK_I); shot("r03_info", MODE_CGA); rows();
    press(PK_E);
    press(PK_M);
    for (int i = 0; i < 80; i++) play_frame(&play, PK_RIGHT);
    press(PK_M);
    q->speed = q->moving = 1;
    ticks(16 * 4);
    squads("after marching fast");

    /* a squad in a fort is trained up */
    int fort = -1;
    for (int c = 0; c < 200; c++) if (tile_fort(tile_at_cell(&play, c))) { fort = c; break; }
    SDL_Log("fort cell %d", fort);
    PSquad *t = &play.sq[16];
    t->x = cell_x(fort) - 1; t->y = cell_y(fort);
    t->archers = 5;
    count_men(&play);
    int before = t->men;
    for (int k = 0; k < 40; k++) forts_train(&play);
    SDL_Log("fort training: men %d -> %d (rule %d)", before, t->men, rule(&play, RULE_FORTS_TRAIN_MEN));

    /* take the black flag */
    for (int f = 0; f < MAX_FLAGS; f++)
        if (play.flag[f].present) SDL_Log("flag %d at (%d,%d) %s", f, play.flag[f].x, play.flag[f].y, play.flag[f].white ? "white" : "black");
    PSquad *g = &play.sq[19];
    g->x = play.flag[8].x; g->y = play.flag[8].y + 1;
    play.view_top = 0;
    ticks(1);
    rows();
    SDL_Log("mode %d ended %d outcome %d captured %d", play.mode, play.ended, play.outcome, play.flags_captured);
    shot("r04_won", MODE_CGA);
    press(PK_MINUS); press(PK_ENTER);
    SDL_Log("result %d", play.result);

    /* T3 Sherwood: only a black flag in the file, so a white one is placed */
    play_start(&play, &camps[3], &ga, 99);
    for (int f = 0; f < MAX_FLAGS; f++)
        if (play.flag[f].present) SDL_Log("T3 flag %d at (%d,%d) %s", f, play.flag[f].x, play.flag[f].y, play.flag[f].white ? "white" : "black");
    squads("T3 start (spies are small)");

    /* the opponent offers to surrender: leave one enemy squad */
    play_start(&play, &camps[0], &ga, 7);
    int kept = 0;
    for (int i = 0; i < MAX_SQUADS; i++)
        if (play.sq[i].used && !play.sq[i].friendly && kept++ > 0) play.sq[i].used = false;
    check_victory(&play, -1, -1);
    SDL_Log("offer: mode %d offer %d", play.mode, play.prompt_offer);
    shot("r05_offer", MODE_CGA);
    press(PK_N);
    SDL_Log("after N: mode %d offer %d A4EA %d", play.mode, play.prompt_offer, OPP(&play, 0xA4EA));
    shot("r06_after_no", MODE_CGA);

    /* lose by having no squads */
    for (int i = 0; i < MAX_SQUADS; i++) if (play.sq[i].friendly) play.sq[i].used = false;
    check_victory(&play, -1, -1);
    rows();
    shot("r07_lost", MODE_VGA);
    assets_free(&ga);
    return 0;
}
