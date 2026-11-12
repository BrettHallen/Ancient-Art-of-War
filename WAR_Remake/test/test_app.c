/*
 * The Ancient Art of War (1984) - remake
 * Nov 2026 using Claude Opus 5.5
 * "Guided" by Brett Hallen
 * Original game by Dave Murry & Barry Murry
 * Published by Brøderbund Software Inc.
 *
 * test/test_app.c: a test or debugging aid for the remake (see README.md here)
 */
/* Test harness (not part of the app): runs the viewer with SDL's offscreen
 * video driver, feeds it key presses and saves what it draws. */
#include <SDL3/SDL.h>
static int frame_no = 0;
static const char *OUT = "/root/shots/app/win";
static bool test_present(SDL_Renderer *r)
{
    SDL_Surface *s = SDL_RenderReadPixels(r, NULL);
    if (s) {
        char p[256];
        SDL_snprintf(p, sizeof p, "%s_%02d_%dx%d.bmp", OUT, frame_no++, s->w, s->h);
        SDL_SaveBMP(s, p);
        SDL_DestroySurface(s);
    }
    return SDL_RenderPresent(r);
}
#define SDL_RenderPresent test_present
#define main viewer_main
#include "../src/main.c"
#undef main

static void key(SDL_Keycode k)
{
    SDL_Event e; SDL_zero(e);
    e.type = SDL_EVENT_KEY_DOWN; e.key.key = k; e.key.down = true;
    SDL_PushEvent(&e);
}
static void wheel(float y)
{
    SDL_Event e; SDL_zero(e);
    e.type = SDL_EVENT_MOUSE_WHEEL; e.wheel.y = y;
    SDL_PushEvent(&e);
}
static void keymod(SDL_Keycode k, SDL_Keymod m)
{
    SDL_Event e; SDL_zero(e);
    e.type = SDL_EVENT_KEY_DOWN; e.key.key = k; e.key.mod = m; e.key.down = true;
    SDL_PushEvent(&e);
}
static Uint32 SDLCALL script(void *u, SDL_TimerID id, Uint32 iv)
{
    (void)u; (void)id; (void)iv;
    static int step = 0;
    switch (step++) {
    case 0: key(SDLK_RETURN); break;                     /* skip "presented by" */
    case 1: key(SDLK_RETURN); break;                     /* go to war */
    case 3: key(SDLK_RETURN); break;                     /* The Race for the Flags */
    case 5: key(SDLK_RETURN); break;                     /* story */
    case 7: key(SDLK_RETURN); break;                     /* rules */
    case 9: key(SDLK_RETURN); break;                     /* opponent */
    case 10: key(SDLK_RETURN); break;                    /* quote: play */
    case 11: key(SDLK_UP); break;                        /* tap: cursor up */
    case 12: key(SDLK_T); break;                         /* time: medium */
    case 13: key(SDLK_TAB); break;                       /* VGA */
    case 14: keymod(SDLK_S, SDL_KMOD_ALT); break;        /* save */
    case 16: key(SDLK_C); break;                         /* continue with game */
    case 17: key(SDLK_ESCAPE); break;                    /* leave? */
    case 18: key(SDLK_Y); break;                         /* yes: title page */
    case 19: key(SDLK_F12); break;                       /* the viewer */
    case 20: key(SDLK_ESCAPE); break;                    /* back to the title */
    case 21: { SDL_Event e; SDL_zero(e); e.type = SDL_EVENT_QUIT; SDL_PushEvent(&e); return 0; }
    default: break;
    }
    return 300;
}
int main(int argc, char **argv)
{
    SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "offscreen");
    SDL_SetHint(SDL_HINT_RENDER_DRIVER, "software");
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);
    SDL_AddTimer(400, script, NULL);
    return viewer_main(argc, argv);
}
