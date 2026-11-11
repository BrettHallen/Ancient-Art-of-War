# The Ancient Art of War - Remake
A modern version of The Ancient Art of War (Broderbund, 1984) for macOS, written in C with [SDL3](https://www.libsdl.org/). It also builds on Windows and Linux.

The remake uses the original backgrounds, map tiles and font, in either the **CGA** (4-colour) or **VGA** (16-colour) versions.

**No original game files are included.** The remake loads all graphics, campaigns, messages, soldiers and the opponents' values from your own copy of the game, so you need the files from the original disk: `WAR.EXE`, `M`, the `P` files, the `T` files, `TITLES`, `DTITLES`, `F`, `CNF`, `O2` to `O7`, plus `WARET.DAT` for the VGA graphics.

## Status
The game starts as the original does (overlay 1 of WAR.EXE):
- **the title pages**: *presented by*, then the barbarian with *go to war / make changes / quit* in the blinking box
- **the scrolls**: the campaign titles, the story and the rules unroll on the parchment, written in the original scroll font, with the sword pointer. The rules can be changed with the Space bar before you go to war. *( data disk )* at the end of the titles opens the data disk (see Saved games)
- **the opponents**: the eight portraits, the blinking frame and each general's description; then a quote from Sun Tzu and *PREPARE FOR WAR!*
- **make changes**: the six scenes of the campaign menu. *Ye Olde Options Shoppe* works (display, fight delay, sounds, joystick; saved as `CNF`) and *Return to title page*; the campaign and formation editors come in step 7
- **the battle report**: victory or defeat, how it was won, and the men captured, the casualties and the flags taken on each side
- **the demo**: left alone on the title page for 23 seconds, the box says *DEMO* and the computer plays one of the campaigns T3–TA against the first opponent, every squad in sight, at the fastest time, scrolling to what happens. It zooms into the first encounter, and the title page comes back when that battle ends. Any key stops it
- **saved games**: **Alt+S** during play saves the game (*WARNING: Any previously saved game ... will be erased*, *Saving Game*, *<C>ontinue with game or <Q>uit?*). To carry on a saved game, choose *( data disk )* at the end of the titles and answer **S** for *Saved game*

The map, the rules and the enemy work as they do in the original program (overlay 6 of WAR.EXE):
- the map cursor, scrolling, and the command lines: *View Time Surrender* and, over a squad, *Move Info Size Formation Detach Join*
- **Formation**: + and - choose one of the 9 formations, shown in a box with your men as A, B and K; E ends
- **Detach** and **Join**: a table of the two squads; K, B, A and S move a knight, barbarian, archer or spy, J and D change the direction, E ends. Food and condition are shared out
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

- **the enemy commander**: each of the eight generals moves his squads as in the original, following his values from O2. Squads hold forts, villages, bridges, hills and flags, rest until they are fit, go to eat when hungry, guard their own flag, and march on a target: one of your flags, or the place most worth taking. Routes are planned cell by cell around water, mountains and ground you hold strongly, half a route at a time. On the way a squad turns aside for a flag or for one of your squads it is strong enough to beat. Squads join up when they arrive, and full squads in a fort split off new ones. A general who is losing badly, or whose surrender you refuse, attacks with everything

Still to come: the campaign editor and the formation editor (step 7), and the sound (step 8). The asset viewer from step 1 is still there: press **F12** on the title page, or start with `--viewer`.

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

The app also looks in a `game` folder next to itself and in the current folder. Press **O** in the viewer to choose a different folder.

### Saved games and the data disk
The original kept saved games and new campaigns on a *data disk*. The remake uses a folder for it: `~/Library/Application Support/AncientArtOfWar/Remake/` on macOS. It holds `S` (the saved game, in the remake's own format), `CNF` (the options) and `TITLES` (the data disk's list of campaigns, first made from `DTITLES`).

Campaigns made with the original game's editor can be played: copy the files from your old data disk (`TITLES` and the `T` files) into this folder, then choose *( data disk )* at the end of the titles. Saved games from the original (`S` on the data disk) can't be carried on yet.

### Title page and menus
| Key | Action |
|-----|--------|
| - / +, arrows | Change the choice, move the pointer, scroll the story |
| Enter | Select, or done |
| Esc | Back up (titles, story, rules, opponents) |
| Space | Change a rule, or an option in the Options Shoppe |
| S / N | *Saved game* or *New campaigns* |
| C / Q | After saving: continue with the game, or quit to the title page |
| F12 | The asset viewer |
| (no key for 23 s) | The demo; any key stops it |

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
| Esc | Back to the title page |

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
| F | Formation of the squad: - / + choose, E ends |
| D / J | Detach men into a new squad, or Join a squad nearby: K, B, A, S move a man, E ends |
| Alt+S | Save the game |
| Enter | When the game is over, on to the battle report |
| Esc | Leave the game (Y or N), back to the title page |

On the battle screen:

| Key | Action |
|-----|--------|
| A | Attack (at the start) |
| A, B, K, S | Choose your Archers, Barbarians, Knights or the whole Squad |
| then A, F, B, S | Attack, Forward, Backoff or Stop; Backspace goes back |
| R | Retreat |

Tab switches between CGA and VGA colours (in the original, Tab scrolled up), F2 changes the aspect, and Cmd+Enter or Alt+Enter is full screen.

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
| `src/play.c` | Campaign play, following overlay 6 of WAR.EXE routine by routine (each function names the routine): cursor, commands, path memory, movement, terrain, supply, food and condition, forts, flags, sighting, messages, Info, Size, View, the end of the game, and the enemy commander (the ai_ functions). Runs at 20 iterations a second, about the speed of an IBM PC/XT |
| `src/battle.c` | The battle screen, following overlay 7 of WAR.EXE: the background from the terrain, formations, the soldiers' animation scripts, choosing targets, fighting, archers and arrows, retreats, the order menus and the losses. Runs at 8 frames a second |
| `src/menu.c` | The title pages, scrolls, opponents, quote, campaign menu, Options Shoppe, battle report and saving, following overlay 1 of WAR.EXE. It draws into its own canvas as the original draws into video memory (pictures, XOR boxes, the AND-drawn pointer), so a scroll unrolls over what was on the screen |
| `src/main.c` | Window, scaling (sharp 4:3 or whole-number square pixels), keys, choosing the game folder, the menus (20 Hz), the 20 Hz game loop and the 8 Hz battle loop |

The file formats are described in [WAR_Analysis/README.md](../WAR_Analysis/README.md).

## Plan
1. Asset loader and viewer (done)
2. Campaign map: load the T files, draw the terrain with tiles, scroll, place squads and flags (done)
3. Strategic rules: movement, terrain, food and supply lines, villages and forts, visibility, spies, flags, Detach, Join and Formation (done)
4. Battles: the fight screen, formations, archers, knights and barbarians (battles on the map and the battle screen done)
5. Opponent AI for the 8 generals, ported from the original program (done)
6. Menus, options, saved games, the battle report and the demo (done)
7. Campaign editor and formation editor
8. Sound: PC speaker and Tandy sound
