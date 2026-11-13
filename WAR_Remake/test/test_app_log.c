/*
 * The Ancient Art of War (1984) - remake
 * Nov 2026 using Claude Opus 5.5
 * "Guided" by Brett Hallen
 * Original game by Dave Murry & Barry Murry
 * Published by Brøderbund Software Inc.
 *
 * test_app_log.c: --log as the app does it. A campaign started (begin_game)
 * opens a new log named with the date, time and title; playing on writes to
 * it; Esc, Y closes it with the reason; a second campaign gets a new file,
 * and the demo writes none.
 *
 * test_app_log GAME-FOLDER LOG-FOLDER
 */
#include <SDL3/SDL.h>
#define main app_main
#include "../src/main.c"
#undef main

static App A;
static int fails;
#define CHECK(c, ...) do { if (!(c)) { SDL_Log("FAIL: " __VA_ARGS__); fails++; } } while (0)

static int logs(const char *dir, char *last, size_t size)
{
    int n = 0;
    char **g = SDL_GlobDirectory(dir, "*.log", 0, &n);
    if (g && n > 0 && last) SDL_snprintf(last, size, "%s%s", dir, g[n - 1]);
    SDL_free(g);
    return n;
}

static void run(int iterations)
{
    for (int k = 0; k < iterations && A.playing; k++) {
        if (A.in_battle) A.battle.done = true;
        play_step(&A);
    }
}

int main(int argc, char **argv)
{
    if (argc < 3) { SDL_Log("usage: test_app_log GAME-FOLDER LOG-FOLDER"); return 1; }
    char dir[600];
    SDL_snprintf(dir, sizeof dir, "%s/", argv[2]);
    gamelog_setup(GLOG_NORMAL, dir);
    if (!load_from(&A, argv[1], false)) return 1;

    begin_game(&A, &A.campaigns[3], 1234);
    CHECK(*gamelog_path(), "no log open after begin_game");
    run(2000);
    play_key(&A.play, PK_ESC, false); run(1);
    play_key(&A.play, PK_Y, false); run(3);
    CHECK(!A.playing, "Esc, Y didn't end the game");
    CHECK(!*gamelog_path(), "the log is still open");
    char path[1200] = "";
    CHECK(logs(dir, path, sizeof path) == 1, "one log");
    size_t size = 0;
    char *t = SDL_LoadFile(path, &size);
    CHECK(t && SDL_strstr(t, A.campaigns[3].title), "the title in the header");
    CHECK(t && SDL_strstr(t, "Random seed  1234"), "the seed in the header");
    CHECK(t && SDL_strstr(t, "YOU Esc: quit to the title page?"), "Esc logged");
    CHECK(t && SDL_strstr(t, "YOU quit to the title page (Y)"), "Y logged");
    CHECK(t && SDL_strstr(t, "You quit the game"), "the reason it ended");
    CHECK(t && SDL_strstr(t, "Log closed after"), "closed");
    CHECK(t && SDL_strstr(t, "] ENEMY "), "the enemy's orders");
    SDL_Log("%s: %zu bytes", path, size);
    SDL_free(t);

    SDL_Delay(1100);                     /* a new second, a new name */
    begin_game(&A, &A.campaigns[0], 99);
    run(300);
    begin_demo(&A, &A.campaigns[0], 5);  /* the demo closes it and isn't logged */
    run(3000);
    CHECK(logs(dir, NULL, 0) == 2, "two logs, not %d", logs(dir, NULL, 0));
    CHECK(!*gamelog_path(), "a log open after the demo");

    if (A.have_assets) assets_free(&A.ga);
    if (fails) { SDL_Log("%d FAILED", fails); return 1; }
    SDL_Log("test_app_log: all passed");
    return 0;
}
