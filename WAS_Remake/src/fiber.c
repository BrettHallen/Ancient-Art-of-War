/*
 * The Ancient Art of War at Sea (1987) - remake
 * Nov 2026 using Claude Opus 5.5
 * "Guided" by Brett Hallen
 * Original game by Dave Murry & Barry Murry
 * Published by Brøderbund Software Inc.
 *
 * fiber.c: a coroutine on an SDL thread (see fiber.h).
 */
#include "fiber.h"

static int fiber_main(void *data)
{
    Fiber *f = data;
    SDL_WaitSemaphore(f->go);
    if (!f->cancel && setjmp(f->exit) == 0)
        f->fn(f->arg);
    f->done = true;
    SDL_SignalSemaphore(f->back);
    return 0;
}

bool fiber_start(Fiber *f, void (*fn)(void *), void *arg)
{
    SDL_zerop(f);
    f->fn = fn;
    f->arg = arg;
    f->go = SDL_CreateSemaphore(0);
    f->back = SDL_CreateSemaphore(0);
    if (!f->go || !f->back) return false;
    f->thread = SDL_CreateThread(fiber_main, "battle", f);
    f->started = f->thread != NULL;
    return f->started;
}

bool fiber_resume(Fiber *f)
{
    if (!f->started || f->done) return false;
    SDL_SignalSemaphore(f->go);
    SDL_WaitSemaphore(f->back);
    return !f->done;
}

void fiber_yield(Fiber *f)
{
    SDL_SignalSemaphore(f->back);
    SDL_WaitSemaphore(f->go);
    if (f->cancel) longjmp(f->exit, 1);
}

void fiber_stop(Fiber *f)
{
    if (f->started) {
        if (!f->done) {
            f->cancel = true;
            SDL_SignalSemaphore(f->go);
            SDL_WaitSemaphore(f->back);
        }
        SDL_WaitThread(f->thread, NULL);
    }
    if (f->go) SDL_DestroySemaphore(f->go);
    if (f->back) SDL_DestroySemaphore(f->back);
    SDL_zerop(f);
}
