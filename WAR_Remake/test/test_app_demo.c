/*
 * The Ancient Art of War (1984) - remake
 * Nov 2026 using Claude Opus 5.5
 * "Guided" by Brett Hallen
 * Original game by Dave Murry & Barry Murry
 * Published by Brøderbund Software Inc.
 *
 * test/test_app_demo.c: a test or debugging aid for the remake (see README.md here)
 */
/* The whole app: the title page left alone, the demo, a key, the demo again
 * until its battle ends. Logs what the app is doing every second. */
#include <SDL3/SDL.h>
#include "../src/menu.h"
#include "../src/battle.h"
static const Play *g_play; static const char *g_last = "?"; static int g_frames;
#define play_draw(p, s, m) (g_play = (p), g_last = "map", g_frames++, play_draw(p, s, m))
#define battle_draw(b, s, m) (g_last = "battle", g_frames++, battle_draw(b, s, m))
#define menu_draw(m, s, mo) (g_last = "menu", g_frames++, menu_draw(m, s, mo))
#define main viewer_main
#include "../src/main.c"
#undef main
static void key(SDL_Keycode k)
{
    SDL_Event e; SDL_zero(e);
    e.type = SDL_EVENT_KEY_DOWN; e.key.key = k; e.key.down = true;
    SDL_PushEvent(&e);
}
static Uint32 SDLCALL script(void *u, SDL_TimerID id, Uint32 iv)
{
    (void)u; (void)id; (void)iv;
    static int t = 0;
    t++;
    SDL_Log("t %3d: %-6s demo %d view %3d key %2x frames %d", t, g_last, g_play ? g_play->demo : -1, g_play ? g_play->view_top : -1, g_play ? g_play->demo_key : 0, g_frames);
    if (t == 2) key(SDLK_RETURN);                       /* past "presented by" */
    if (t == 40) key(SDLK_X);                           /* a key ends the demo (it starts after 30 s) */
    if (t == 110) { SDL_Event e; SDL_zero(e); e.type = SDL_EVENT_QUIT; SDL_PushEvent(&e); return 0; }
    return 1000;
}
static bool test_present(SDL_Renderer *r) { return SDL_RenderPresent(r); }
int main(int argc, char **argv)
{
    SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "offscreen");
    SDL_SetHint(SDL_HINT_RENDER_DRIVER, "software");
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);
    SDL_AddTimer(1000, script, NULL);
    (void)test_present;
    return viewer_main(argc, argv);
}
