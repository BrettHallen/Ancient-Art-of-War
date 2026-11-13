/*
 * The Ancient Art of War at Sea (1987) - remake
 * Nov 2026 using Claude Opus 5.5
 * "Guided" by Brett Hallen
 * Original game by Dave Murry & Barry Murry
 * Published by Brøderbund Software Inc.
 *
 * fiber.h: a coroutine on an SDL thread. WAS.EXE's sea battle runs straight
 * through, drawing to the screen and calling delay() as it goes; the remake
 * runs that code as written on its own thread and hands control back and
 * forth, so only one side ever runs at a time (no locks are needed and the
 * result is the same on every run).
 */
#ifndef WAS_FIBER_H
#define WAS_FIBER_H

#include <SDL3/SDL.h>
#include <setjmp.h>

typedef struct Fiber {
    SDL_Thread    *thread;
    SDL_Semaphore *go, *back;
    void         (*fn)(void *arg);
    void          *arg;
    bool           started, done, cancel;
    jmp_buf        exit;
} Fiber;

/* Create the fiber; fn(arg) starts at the first fiber_resume. */
bool fiber_start(Fiber *f, void (*fn)(void *), void *arg);
/* Run the fiber until it yields or ends. Returns false once it has ended. */
bool fiber_resume(Fiber *f);
/* Inside the fiber: hand control back to whoever resumed it. */
void fiber_yield(Fiber *f);
/* Stop the fiber (it unwinds at its next yield) and free it. */
void fiber_stop(Fiber *f);

#endif
