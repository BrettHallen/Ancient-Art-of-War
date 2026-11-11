# The Ancient Art of War - Remake
A modern version of The Ancient Art of War (Broderbund, 1984) for macOS, written in C with [SDL3](https://www.libsdl.org/). It also builds on Windows and Linux.

The remake uses the original backgrounds, map tiles and font, in either the **CGA** (4-colour) or **VGA** (16-colour) versions.

**No original game files are included.** The remake loads all graphics, campaigns, messages, soldiers and the opponents' values from your own copy of the game, so you need the files from the original disk: `WAR.EXE`, `M`, the `P` files, the `T` files, `TITLES`, `F`, `O2`, `O5`, `O6` and `O7`, plus `WARET.DAT` for the VGA graphics.

## Status
The viewer shows every picture, the map tiles, the font and all 11 campaigns from the original files, in CGA or VGA colours. Each campaign is shown as the game starts it: the map, every squad (your white squads and the enemy's black squads) and the flags.

Press **Enter** on a campaign map to play it. These parts work as they do in the original program (overlay 6 of WAR.EXE):
- the map cursor, scrolling, and the command lines: *View Time Surrender* and, over a squad, *Move Info Size Formation Detach Join*
- **Move**: trace a path with the open box and press M to mark the destination. Squads march along it at their speed, slowed by forest, mountains and water according to the campaign's rules (enemy squads by their commander's own values). Knights are slower in rough ground. Squads can lose men, or be lost, in deep water and high mountains
- **food and supply lines**: every 16 ticks each side's hold on every part of the map is worked out from its squads. Squads near a village or fort that their side holds are fed (if the rules let villages or forts supply food) and go hungry further away; inside one they are fully fed
- **condition**: marching wears squads down (fast march twice as much, more so if your men are out of shape), resting and villages or forts restore it, hunger costs more. Tired squads drop to a slower march
- **forts train men**: a squad in a fort may gain an archer, knight or barbarian, as often as the rules say
- **flags**: a squad that reaches the other side's flag takes it (*URGENT – FLAG TAKEN*, *ENEMY FLAG CAPTURED*). The side left with no flags loses. A side that starts with no flag gets one placed at random, as in the original
- **the end**: losing every squad or every flag ends the game (*You Won / You Lost … [Enter] to continue*, with every enemy squad shown). An opponent who is outnumbered enough offers to surrender
- **Info** (orders, condition and food bars with + or -, S to change the speed, which also halts a moving squad), **Size** and **View** (a small map of who holds what, and how many men each side has)
- **Time**: slow, medium, fast, faster. Time stops while you trace a path or hold a key, as in the original
- sighting: enemy squads stay hidden until your squads come within range (spies double it), with *ALERT – ENEMY SIGHTED IN THE …* messages
- **Surrender**
- **battles on the map**: squads that meet stop with *WARNING – ENCOUNTER*. A much stronger squad captures the weaker one without a fight (*ENEMY CAPTURED* / *YOUR SQUAD CAPTURED*). Otherwise Zoom shows on the command line for about 20 seconds, then the two squads blink and fight it out on the map (*A L E R T – FIGHTING*, then *NOTICE – BATTLE WON / LOST*). The odds come from the number of men, their condition, hills, water and spies, and archers beat knights, knights beat barbarians and barbarians beat archers

- **the battle screen**: press **Z** for Zoom while two squads are about to fight, and the battle is fought man to man, as in overlay 7 of WAR.EXE. The background comes from the ground the two squads stand on (forts, villages, hills, water, bridges, forest, mountains and open country: 14 pictures). Both sides line up in their formations; the enemy's formation depends on its commander. The soldiers are moved by the original animation scripts from O6 and O7: knights and barbarians close in and fight, archers aim in one of 20 directions and their arrows fly, lose power and can hit friends. Knights' armour turns some arrows and barbarians dodge some. Press **A** to attack (or wait for the enemy to), then give orders to your **A**rchers, **B**arbarians, **K**nights or the whole **S**quad: **A**ttack, **F**orward, **B**ackoff or **S**top (Backspace goes back). **R** retreats: your squad falls back on the map. A side may also break and run, and a fort is lost when its archers are gone. At the end you see *You lost N men .... They lost N men*, and both squads come back to the map tired

Still to come: the enemy's moves, and Formation, Detach and Join.

## Building on macOS
Install the tools with [Homebrew](https://brew.sh):
```sh
brew install cmake sdl3
```

Then build:
```sh
cd WAR_Remake
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

This makes `build/AncientArtOfWar.app`.

## Running
```sh
open build/AncientArtOfWar.app
```
On first launch the app asks for the folder that holds your original game files, and remembers it. You can also give the folder on the command line:
```sh
build/AncientArtOfWar.app/Contents/MacOS/AncientArtOfWar ../WAR_Original_Files_3.5
```

The app also looks in a `game` folder next to itself and in the current folder. Press **O** to choose a different folder.

### Viewer keys
| Key | Action |
|-----|--------|
| Left / Right | Previous / next screen |
| Up / Down, - / +, mouse wheel | Scroll a campaign map (as in the original, - scrolls up) |
| Shift + - / +, Home / End | Top / bottom of the map |
| Enter | Play the campaign shown |
| Tab | Switch between CGA and VGA colours |
| A | 4:3 like the original monitor, or square pixels |
| H | Show or hide the caption |
| F, Cmd+Enter | Full screen |
| O | Choose the game folder |
| Esc | Quit |

### Playing
The original keys:

| Key | Action |
|-----|--------|
| Arrows, keypad | Move the cursor (Home, Page Up, End, Page Down and keypad 7, 9, 1, 3 go diagonally; holding two arrows does too) |
| - / + | Scroll the map; with Shift, jump to the top or bottom |
| M | Move the squad under the cursor: trace the path, then M again to mark the destination |
| C | Cancel the path; otherwise clear the messages |
| V | View: who holds which part of the map (any key returns) |
| I | Info about the squad: E ends, S changes the speed |
| S | Size of the squad; with no squad under the cursor, Surrender (Y or N) |
| T | Time: slow, medium, fast, faster |
| Y / N | Answer a question (Surrender, or the opponent's offer to surrender) |
| Z | Zoom: fight the coming battle on the battle screen |
| Enter | When the game is over, back to the viewer |

On the battle screen:

| Key | Action |
|-----|--------|
| A | Attack (at the start) |
| A, B, K, S | Choose your Archers, Barbarians, Knights or the whole Squad |
| then A, F, B, S | Attack, Forward, Backoff or Stop; Backspace goes back |
| R | Retreat |

Tab switches between CGA and VGA colours, F2 changes the aspect, Cmd+Enter or Alt+Enter is full screen, and Esc goes back to the viewer.

### Screenshots
```sh
AncientArtOfWar --screenshots OUTDIR GAME-FOLDER
```
This writes every screen in both colour sets as BMP files (320x200), without opening a window. Campaign maps are shown at the bottom of the map, where the game starts.

## Source
| File | Purpose |
|------|---------|
| `src/assets.c` | Loads and decodes the original files: the font and the squad and flag sprites from WAR.EXE, the CGA tiles (M) and pictures (P files), and the VGA pictures and tiles from WARET.DAT |
| `src/gfx.c` | The 320x200 game screen: palette, pictures, tiles and text in the original font |
| `src/campaign.c` | Loads the 11 campaigns (T0-TA), their titles (TITLES), squads and flags, and draws a campaign map: 10 x 20 tiles, 9 rows on screen, with the squads and flags on top |
| `src/play.c` | Campaign play, following overlay 6 of WAR.EXE routine by routine (each function names the routine): cursor, commands, path memory, movement, terrain, supply, food and condition, forts, flags, sighting, messages, Info, Size, View and the end of the game. Runs at 20 iterations a second, about the speed of an IBM PC/XT |
| `src/battle.c` | The battle screen, following overlay 7 of WAR.EXE: the background from the terrain, formations, the soldiers' animation scripts, choosing targets, fighting, archers and arrows, retreats, the order menus and the losses. Runs at 8 frames a second |
| `src/main.c` | Window, scaling (sharp 4:3 or whole-number square pixels), keys, choosing the game folder, the 20 Hz game loop and the 8 Hz battle loop |

The file formats are described in [WAR_Analysis/README.md](../WAR_Analysis/README.md).

## Plan
1. Asset loader and viewer (done)
2. Campaign map: load the T files, draw the terrain with tiles, scroll, place squads and flags (done)
3. Strategic rules: movement, terrain, food and supply lines, villages and forts, visibility, spies, flags (done; Detach, Join and Formation come with battles)
4. Battles: the fight screen, formations, archers, knights and barbarians (battles on the map and the battle screen done)
5. Opponent AI for the 8 generals, ported from the original program
6. Menus, options and saved games
7. Campaign editor and formation editor
8. Sound: PC speaker and Tandy sound
