/*
 * The Ancient Art of War at Sea (1987) - remake
 * Nov 2026 using Claude Opus 5.5
 * "Guided" by Brett Hallen
 * Original game by Dave Murry & Barry Murry
 * Published by Brøderbund Software Inc.
 *
 * test/test_app.c: the whole app (main.c) with SDL's offscreen video driver,
 * driven by key events from a thread-free script in the event watch: the
 * viewer to the first campaign map, Enter to play, the marker, Info, End,
 * Fleet, Esc and Y back to the viewer, then quit. Saves the window's last
 * frame of each step as a BMP.
 *
 * Usage: SDL_VIDEODRIVER=offscreen test_app GAME-FOLDER OUT-FOLDER
 */
#include <SDL3/SDL.h>

static const char *OUT;
static int frame_no = 0, step = 0;
static SDL_Keycode SCRIPT[] = {
    SDLK_RIGHT, SDLK_RIGHT, SDLK_RIGHT, SDLK_RIGHT, SDLK_RIGHT, SDLK_RIGHT, SDLK_RIGHT, SDLK_RIGHT, SDLK_RIGHT,
    SDLK_RIGHT, SDLK_RIGHT, SDLK_RIGHT, SDLK_RIGHT, SDLK_RIGHT, SDLK_RIGHT, SDLK_RIGHT, SDLK_RIGHT,   /* map 1 */
    SDLK_RETURN,                                   /* play */
    SDLK_LEFT, SDLK_LEFT, SDLK_LEFT, SDLK_DOWN,    /* the marker */
    SDLK_F, SDLK_SPACE, SDLK_T, SDLK_TAB, SDLK_TAB, /* Fleet, Time, 16 colours, Hercules */
    SDLK_ESCAPE, SDLK_Y,                           /* back to the viewer */
    SDLK_ESCAPE,                                   /* quit */
};
#define NSCRIPT ((int)(sizeof SCRIPT / sizeof SCRIPT[0]))

static bool test_present(SDL_Renderer *r)
{
    frame_no++;
    if (frame_no % 3 == 0 && step < NSCRIPT) {     /* a key every third frame */
        SDL_Event e;
        SDL_zero(e);
        e.type = SDL_EVENT_KEY_DOWN;
        e.key.key = SCRIPT[step];
        e.key.down = true;
        SDL_PushEvent(&e);
        SDL_Surface *s = SDL_RenderReadPixels(r, NULL);
        if (s) {
            char p[256];
            SDL_snprintf(p, sizeof p, "%s/app_%02d.bmp", OUT, step);
            SDL_SaveBMP(s, p);
            SDL_DestroySurface(s);
        }
        step++;
    }
    return SDL_RenderPresent(r);
}
#define SDL_RenderPresent test_present
#define main app_main
#include "../src/main.c"
#undef main
#undef SDL_RenderPresent

int main(int argc, char **argv)
{
    if (argc < 3) { SDL_Log("usage: test_app GAME-FOLDER OUT-FOLDER"); return 2; }
    OUT = argv[2];
    char *args[] = { argv[0], argv[1], NULL };
    int rc = app_main(2, args);
    SDL_Log("test_app: %d keys sent, %d frames, rc %d: %s", step, frame_no, rc, step == NSCRIPT && rc == 0 ? "passed" : "FAILED");
    return step == NSCRIPT && rc == 0 ? 0 : 1;
}
