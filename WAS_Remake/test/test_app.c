/*
 * The Ancient Art of War at Sea (1987) - remake
 * Nov 2026 using Claude Opus 5.5
 * "Guided" by Brett Hallen
 * Original game by Dave Murry & Barry Murry
 * Published by Brøderbund Software Inc.
 *
 * test/test_app.c: the whole app (main.c) with SDL's offscreen video driver,
 * driven by key events from a thread-free script in the event watch: the
 * menus ("presented by", the title page), F12 to the viewer, the first
 * campaign map, Enter to play, the marker, Fleet, Time, Tab, Esc and Y back
 * to the title page, then quit from it. Saves the window's last frame of each
 * step as a BMP. It starts with --video=pcjr: the title page must show the
 * PCjr's colours for it (black, cyan, red, white), and the four Tabs go round
 * the modes back to the PCjr.
 *
 * Usage: SDL_VIDEODRIVER=offscreen test_app GAME-FOLDER OUT-FOLDER
 */
#include <SDL3/SDL.h>

static const char *OUT;
static int frame_no = 0, step = 0;
static const struct { int wait_ms; SDL_Keycode key; } SCRIPT[] = {
    { 9000, SDLK_F12 },                             /* the title page (after "presented by"): the viewer */
    { 400, SDLK_RIGHT }, { 400, SDLK_RIGHT }, { 400, SDLK_RIGHT }, { 400, SDLK_RIGHT }, { 400, SDLK_RIGHT },
    { 400, SDLK_RIGHT }, { 400, SDLK_RIGHT }, { 400, SDLK_RIGHT }, { 400, SDLK_RIGHT }, { 400, SDLK_RIGHT },
    { 400, SDLK_RIGHT }, { 400, SDLK_RIGHT }, { 400, SDLK_RIGHT }, { 400, SDLK_RIGHT }, { 400, SDLK_RIGHT },
    { 400, SDLK_RIGHT }, { 400, SDLK_RIGHT },       /* map 1 */
    { 400, SDLK_RETURN },                           /* play */
    { 9000, SDLK_LEFT }, { 400, SDLK_LEFT }, { 400, SDLK_LEFT }, { 400, SDLK_DOWN },   /* the marker, once the map shows */
    { 400, SDLK_F }, { 400, SDLK_SPACE }, { 400, SDLK_T }, { 400, SDLK_TAB }, { 400, SDLK_TAB }, { 400, SDLK_TAB },
    { 400, SDLK_TAB }, { 400, SDLK_ESCAPE }, { 400, SDLK_Y },          /* to the title page */
    { 2000, SDLK_MINUS }, { 1500, SDLK_RETURN },    /* quit */
};
#define NSCRIPT ((int)(sizeof SCRIPT / sizeof SCRIPT[0]))

static bool test_present(SDL_Renderer *r)
{
    frame_no++;
    static Uint64 last = 0;
    Uint64 now = SDL_GetTicks();
    if (last == 0) last = now;
    if (step < NSCRIPT && now - last >= (Uint64)SCRIPT[step].wait_ms) {   /* time for WAS.EXE's work and delays */
        last = now;
        SDL_Event e;
        SDL_zero(e);
        e.type = SDL_EVENT_KEY_DOWN;
        e.key.key = SCRIPT[step].key;
        e.key.down = true;
        SDL_PushEvent(&e);
        e.type = SDL_EVENT_KEY_UP;
        e.key.down = false;
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
    char *args[] = { argv[0], "--video=pcjr", argv[1], NULL };
    int rc = app_main(3, args);
    /* the title page (step 0's frame) in the PCjr's colours: 0, 3 (cyan), 4 (red), 15 */
    bool pcjr = false;
    char p[256];
    SDL_snprintf(p, sizeof p, "%s/app_00.bmp", OUT);
    SDL_Surface *t = SDL_LoadBMP(p);
    if (t) {
        bool cyan = false, red = false, cga = false;
        for (int y = 0; y < t->h; y += 3)
            for (int x = 0; x < t->w; x += 3) {
                Uint8 r, g, b;
                SDL_ReadSurfacePixel(t, x, y, &r, &g, &b, NULL);
                if (r == 0 && g == 170 && b == 170) cyan = true;
                if (r == 170 && g == 0 && b == 0) red = true;
                if (r == 85 && g == 255 && b == 255) cga = true;
            }
        pcjr = cyan && red && !cga;
        SDL_DestroySurface(t);
    }
    if (!pcjr) SDL_Log("FAIL: the title page is not in the PCjr's colours (--video=pcjr)");
    bool ok = step == NSCRIPT && rc == 0 && pcjr;
    SDL_Log("test_app: %d keys sent, %d frames, rc %d: %s", step, frame_no, rc, ok ? "passed" : "FAILED");
    return ok ? 0 : 1;
}
