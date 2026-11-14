/*
 * The Ancient Art of War (1984) - remake
 * Nov 2026 using Claude Opus 5.5
 * "Guided" by Brett Hallen
 * Original game by Dave Murry & Barry Murry
 * Published by Brøderbund Software Inc.
 *
 * savegame.h: carrying on a game saved by the original program. Its S file
 * is WAR.EXE's data segment from DS:8868 to A9E5 (save_game, ovl1:35C4);
 * the 16-colour program's SET is WARET.EXE's from DS:0218 to 26F3.
 */
#ifndef WAR_SAVEGAME_H
#define WAR_SAVEGAME_H

#include "campaign.h"
#include "play.h"

#define ORIGINAL_S_SIZE 0x217E
#define ORIGINAL_S_BASE 0x8868   /* the DS address of the file's first byte */
#define ORIGINAL_SET_SIZE 0x24DC /* WARET.EXE's SET: DS:0218-26F3 */
#define ORIGINAL_SET_BASE 0x0218

/* Fill c (the campaign: map, armies, opponent, rules) and p (the game) from
 * an original S (size ORIGINAL_S_SIZE) or SET (ORIGINAL_SET_SIZE). The
 * caller sets the title and story if it can find the campaign, the fight
 * delay from its options, then calls play_resume. */
bool savegame_import(Play *p, Campaign *c, const GameAssets *ga, const uint8_t *s, size_t size, uint32_t seed);

/* The original's campaign number ([A694]) and map, to find its title */
int  savegame_campaign_number(const uint8_t *s);
const uint8_t *savegame_map(const uint8_t *s);

#endif
