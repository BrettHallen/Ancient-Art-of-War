/*
 * The Ancient Art of War at Sea (1987) - remake
 * Nov 2026 using Claude Opus 5.5
 * "Guided" by Brett Hallen
 * Original game by Dave Murry & Barry Murry
 * Published by Brøderbund Software Inc.
 *
 * icon.h: the app icon, drawn from the player's own game files: the
 * title page (the "presented by" picture SPM: the 16-colour one if
 * WAST.DAT is there, else CGA) in its 4:3 shape on a black rounded square,
 * as macOS icons are laid out.
 */
#ifndef WAS_ICON_H
#define WAS_ICON_H

#include "assets.h"

/* Draw the icon at size x size into rgba (size * size * 4 bytes, straight
 * alpha). False if the game files have no title page. */
bool icon_render(const GameAssets *ga, int size, uint8_t *rgba);

#endif
