# The Ancient Art of War - Remake
A modern version of The Ancient Art of War (Broderbund, 1984) for macOS, written in C with [SDL3](https://www.libsdl.org/). It also builds on Windows and Linux.

The remake uses the original backgrounds, map tiles and font, shown as the original showed them on each video card it supported: **CGA** (4 colours, WAR.EXE) on an RGB monitor or on a TV or composite monitor (the old and the new IBM CGA's colours), **EGA**, **Tandy 1000** and **VGA** (16 colours, WARET.EXE), or **Hercules** monochrome (the original's `WAR HERC`).

**No original game files are included.** The remake loads all graphics, campaigns, messages, soldiers and the opponents' values from your own copy of the game, so you need the files from the original disk: `WAR.EXE`, `M`, the `P` files, the `T` files, `TITLES`, `DTITLES`, `F`, `CNF`, `O2` to `O7`, plus `WARET.DAT` for the VGA graphics.

## Status
The game starts as the original does (overlay 1 of WAR.EXE):
- **the title pages**: *presented by*, then the barbarian with *go to war / make changes / quit* in the blinking box
- **the scrolls**: the campaign titles, the story and the rules unroll on the parchment, written in the original scroll font, with the sword pointer. The rules can be changed with the Space bar before you go to war. *( data disk )* at the end of the titles opens the data disk (see Saved games)
- **the opponents**: the eight portraits, the blinking frame and each general's description; then a quote from Sun Tzu and *PREPARE FOR WAR!*
- **make changes**: the six scenes of the campaign menu. *Ye Olde Options Shoppe* works (display, fight delay, which data disk is in the drive, sounds, joystick; saved as `CNF`) and *Return to title page*
- **the campaign editor** (overlays 1, 3 and 4 of WAR.EXE): *Make a new campaign* takes the original's six steps: the map maker (four pages of 18 tiles, A–S put one down, P turns the page), the squads and flags (Squad, Flag, Total; over a squad Info, Size, Formation, Get and Remove), the enemy leader, the rules, the story and the title, each written on the scroll with the quill. *Change a campaign* chooses one from the data disk, then the rulebook, storybook, map maker, enemy or army, each asking *Save the last changes?* as the original does. *Remove a campaign* wipes its title off the scroll. The campaigns are written to the data disk as the original writes them (a `T` file and `TITLES`), so the original game can play them too
- **the battle report**: victory or defeat, how it was won, and the men captured, the casualties and the flags taken on each side
- **the demo**: left alone on the title page for 30 seconds (as the manual says), the box says *DEMO* and the computer plays one of the campaigns T3–TA against the first opponent, every squad in sight, at the fastest time, scrolling to what happens. As in the original, only the enemy's black squads move: the demo never gives your white squads orders, so they wait where the campaign puts them. It zooms into the first encounter, and the title page comes back when that battle ends. Any key stops it
- **saved games**: **⌥S** (Option+S) on a Mac, **Alt+S** on Windows and Linux, during play saves the game (*WARNING: Any previously saved game ... will be erased*, *Saving Game*, *<C>ontinue with game or <Q>uit?*). To carry on a saved game, choose *( data disk )* at the end of the titles and answer **S** for *Saved game*

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

- **sound**: the PC speaker as WAR.EXE plays it: the opening tune at *presented by*, the victory and defeat tunes on the battle report, the bugle calls when a battle begins and when a side retreats (the battle waits for them, as in the original), the low tones at the start and end of a game (a new game opens with a "da-dum", a pause and two more: in the original the pause is the time the enemy commander spends planning its first five moves, about 0.8 s on an IBM PC/XT, which the remake keeps as silence), a beep for each message on the map and each order in battle, the buzz as the cursor comes onto a squad, the click of arrows and falling men, and the crackle of the unrolling scroll. The *Options Shoppe* sounds setting works as on a Tandy 1000 or PCjr: *all sounds are off*, *do not play music*, *internal speaker* (the PC speaker), or *external speaker*, which plays the tunes and bugle calls in three voices on the Tandy/PCjr sound chip

- **the formation editor** (overlay 5): *Teach squads new formations* shows the 14 men of each formation on the field, with how many of each kind and the formation's plan. - and + choose it; formations 1–5 can be looked at, 6–9 changed: **M** brings a pointer, **M** over a man picks him up, the arrows walk him across the field (the original's walking soldiers), **A**, **B** and **K** make him an archer, barbarian or knight, and **M** puts him down. **S** saves the formations; squads use them at once

The asset viewer from step 1 is still there: press **F12** on the title page, or start with `--viewer`.

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

The app's icon is the opening title page, drawn during the build from your own game files, so no original artwork is kept in the repository. CMake finds them in `../WAR_Original_Files_3.5`; if they are somewhere else, add `-DWAR_GAME_DIR=/path/to/your/game` to the first `cmake` command. Without them the app builds with no icon in Finder, and the game still shows the icon in the Dock once it has loaded your files. If Finder keeps showing an old icon, run `touch build/AncientArtOfWar.app`.

## Running
```sh
open build/AncientArtOfWar.app
```
On first launch the app asks for the folder that holds your original game files, and remembers it. You can also give the folder on the command line:
```sh
build/AncientArtOfWar.app/Contents/MacOS/AncientArtOfWar ../WAR_Original_Files_3.5
```

The app also looks in a `game` folder next to itself and in the current folder. Press **O** in the viewer to choose a different folder.

**Video card:** `video=` on the command line chooses the card the game starts on (CGA if not given); Tab goes through them in the game, CGA → CGA composite → CGA composite (new) → EGA → Tandy → VGA → Hercules:
```sh
build/AncientArtOfWar.app/Contents/MacOS/AncientArtOfWar video=ega      # or cga, composite, composite2, tandy, vga, herc
build/AncientArtOfWar.app/Contents/MacOS/AncientArtOfWar HERC           # as WAR HERC: the same as video=herc
```
The original's 16-colour program, WARET.EXE, drew the same pictures (WARET.DAT) on all three 16-colour cards, but only on a VGA did it load each picture's own palette; an **EGA** and a **Tandy 1000** showed them in the standard 16 colours, so the same pictures look quite different (on the opening page the woven border is red instead of brown, "Broderbund" yellow instead of green). The Tandy version also drew a few things differently: the pointer (a blue hand or sword on an EGA or VGA, a red and black one on a Tandy), a yellow map cursor, and the outlines of squads and flags. The text rows are white on all three, the scrolls' too, and the Options Shoppe's first line names the card ("EGA: sixteen colors"; no black/white or composite display to choose), with "EGA: no centering" on an EGA or VGA. WARET.EXE drew other small things its own way too: on an EGA or VGA a picture placed half-way through a byte of 8 pixels moves 4 pixels left (the title page's item, your numbers on the battle report, the map maker's tiles), and icons leave their black pixels undrawn; the trace box is 8 lines high instead of 12; Info's bars are yellow and the View map shows your ground in green; the map maker marks its plan's cell in light red; on a Tandy the trace box's sides and the gap in the cursor are drawn over what is there (XOR 7, AND 14), and the soldiers' outlines on the battle screen and in the formation editor stay within 4 pixels. All of this was measured in DOSBox-X with WARET.EXE on each machine, and the remake matches its screens pixel for pixel on all three cards, apart from what moves or blinks: the opening and title pages, the scrolls, the opponents, the quote and PREPARE FOR WAR!, the campaign menu, the Options Shoppe, the map and its boxes (Info, Size, Formation, Detach, a traced path, View, Time, Surrender), the battle report, the battle screen's soldiers, and the editors (the map maker, the squads, Change a campaign's menu, the formation editor).

**CGA on a TV:** a CGA's composite output (to a TV or a composite colour monitor) shows WAR.EXE's 4 colours as other colours, made by the TV from the dots' pattern: each pixel's colour depends on its neighbours. `video=composite` shows the original IBM CGA, `video=composite2` the later one (its colours are paler); both are 640 dots across, each colour taken from the pixel and its neighbours as DOSBox-X draws them (`machine=cga` with `composite` on, measured with a test pattern of every pixel triple). The Options Shoppe's black-and-white display is shown in greys, without the colours.

The Options Shoppe's *display* line goes with these cards (the remake's own convenience: in WAR.EXE the setting only changed the palette registers of a PCjr or Tandy 1000 running the CGA version, and a plain CGA looked the same whatever it said). On a CGA, choosing *tv/composite* shows the CGA on a TV and *rgb* the plain CGA; *black/white* turns the picture grey on either. The other way round, Tab onto a composite card makes the line say *tv/composite* and Tab back to the plain CGA makes it *rgb* (black/white stays as it is). On an EGA, Tandy or VGA the line names the card and the pointer can't reach it, as in WARET.EXE.

**Hercules:** WAR.EXE drew the same picture for Hercules as for CGA, with each 4-colour pixel shown as two black or lit pixels, 640x200 in the middle of the card's 720x348 screen; the remake shows that whole screen (4:3, or square pixels with F2), lit pixels in DOSBox's grey. It matches DOSBox's Hercules screens pixel for pixel.

**Game log (debugging):** `--log` writes a log of each campaign played, to see what the enemy commander was "thinking":
```sh
build/AncientArtOfWar.app/Contents/MacOS/AncientArtOfWar --log            # decisions and events
build/AncientArtOfWar.app/Contents/MacOS/AncientArtOfWar --log=verbose    # and much more
```
Each campaign started (or saved game carried on) opens a new text file in `Logs/` in the remake's folder (`~/Library/Application Support/AncientArtOfWar/Remake/Logs/` on macOS), named with the date, time and campaign, such as `2026-11-13 20-15-31 Sherwood Forest.log`. It starts with the campaign and its settings: the title, the opponent and its 19 values (what each one does), the rules as the Rulebook words them, the map, how long encounters take to become fights (Options Shoppe), the random seed, and every squad and flag. Then one line per decision or event, after the time on the map and the game tick (squads are named by slot, with their knights, barbarians, archers and spies, x,y and cell):
```
[  0:00 tick     0] ENEMY new target: cell 160 (row 16 col 0, open), the best score 230 (its worth, ...)
[  0:00 tick     0] ENEMY sends enemy squad 12 [1K 13B 0A 0S] at 32,139 cell 144 open to its target cell 160 (...)
[  0:00 tick     0] ENEMY route for squad 12 from cell 144 (...) to cell 160 (...): 4 cells, cost 4; marches the first part: SW SW
[  0:05 tick     1] YOU order your squad 5 [7K 0B 7A 0S] at 13,149 cell 151 open to march to 1,149 in cell 150 (...): 12 steps
[  0:18 tick     9] EVENT encounter: both stop; they fight on the map after 400 iterations unless you Zoom
[  0:18 tick     9] BATTLE frame   46 (  5.7 s) their barbarian falls (you have lost 0, they 1)
```
- **ENEMY**: the commander's choices and the reasons, with the values behind them: its target (and the score that won), the flag it goes for and the one it guards, where each squad is sent (to eat, to guard its flag, to the target), its routes, the squads it attacks and the flags it goes for on the way, joining and splitting squads, giving up a target, going all out, offering to surrender.
- **YOU**: your orders: Move (where to, how fast, how many steps), Cancel, Speed, Detach, Join, Formation, Time, Zoom, Surrender, Esc, Alt+S, and your answers to its questions.
- **EVENT**: sightings, encounters, captures, fights on the map and each man lost, battles won and lost, flags taken, men lost in deep water or high mountains, forts training men, squads too tired to march, the end of the game.
- **BATTLE**: the battle screen: the two squads and formations, the ground and the skills, your orders, each man who falls, retreats and the losses.

`--log=verbose` adds each squad the enemy looks at and why it holds its ground (a fort, a valuable cell, its flag, resting), every candidate it weighed with its score, its cell tables (what each cell is worth, what it costs to cross, the supply map) whenever they change, and a table of all squads every 16 ticks. A game ends the log with the reason (you won: the enemy has no flags ... you quit) and the state at the end. The demo isn't logged. Logging only reads the game and never calls the random number routine, so a logged game plays exactly as one not logged (`test/test_gamelog.c` checks this byte for byte).

### Saved games and the data disks
The original kept saved games and new campaigns on *data disks*, swapped in the drive. The remake keeps them in its folder, `~/Library/Application Support/AncientArtOfWar/Remake/` on macOS:

| | |
|---|---|
| `CNF` | the options |
| `F` | the formations, once some have been taught (the remake uses it in place of the game's own `F`) |
| `DRIVE` | the name of the data disk in the drive |
| `Data Disks/Data Disk 1/` | a data disk: `TITLES` (its list of campaigns, first made from `DTITLES`), the `T` files of its campaigns, and `S`, its saved game (`SET` is the 16-colour program's) |
| `Data Disks/…/` | more data disks: any folder here is one (up to 12) |

The data disk in the drive is chosen in the *Options Shoppe* (*disk …*, or *a new data disk*), or from the list that *( data disk )* shows when there is more than one. New campaigns are written to it, and its saved game is the one carried on.

To use an old data disk, make a new folder in `Data Disks` (any name) and copy the floppy's files into it as they are; upper or lower case doesn't matter. Its campaigns can be played and changed, and a game the original saved on it (`S`, or `SET` from the 16-colour program WARET.EXE) can be carried on: choose *( data disk )*, then **S** for *Saved game*. With both on the disk, the one saved last is carried on. The remake finds the campaign's title by its map, and carries on from the same moment: squads, paths, marks (not from a `SET`, as WARET.EXE itself forgets them), battles under way, the enemy's plans and the time setting.

If a file can't be written there (the disk is full, say), the remake says *Game not saved!* for a saved game, and *Unable to use data disk. Press any key to continue.* for the options, a campaign, `TITLES` or `F`, as the original does for a bad data disk. Your changes to a campaign are kept while you stay in the editor, so the next save that works writes them all (answering **N** to *Save the last changes?* reloads the campaign from its file); taught formations are used until you quit.

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
| (no key for 30 s) | The demo; any key stops it |

### Campaign editor
| Where | Keys |
|-------|------|
| Map maker | Arrows move the cursor (left and right go on to the next row), **A**–**S** put down a tile of the page shown (there is no P), **P** the next page, **Enter** when done |
| Squads and flags | Arrows and - / + move the cursor and scroll; **S** a new squad, **F** a new flag, **T** the total of men; over a squad **I**nfo, **S**ize, **F**ormation, **G**et (then **P**ut) and **R**emove; over a flag **F** friend or foe and **R**emove; **Enter** when done. In Size, **K B A S** add or take away a knight, barbarian, archer or spy, **I** and **D** switch between adding and taking away, **E** ends (14 men at most). In Info, **C** condition, **F** food, **S** speed, **T** friend or foe |
| Story and title | Type on the scroll (capitals take two places); arrows move the quill, Backspace rubs out, Enter starts the next line. **Esc** when the story is done, **Enter** when the title is done |
| Formations | - / + choose the formation; for 6–9 **M** the pointer, **S** save, **Enter** when done. With the pointer: arrows move it, **M** picks up the man under it, **E** ends. Moving a man: arrows walk him, **A B K** archer, barbarian or knight, **M** puts him down |
| The questions | **Y** or **N** |

### Viewer keys
| Key | Action |
|-----|--------|
| Left / Right | Previous / next screen |
| Up / Down, - / +, mouse wheel | Scroll a campaign map (as in the original, - scrolls up) |
| Shift + - / +, Home / End | Top / bottom of the map |
| Enter | Play the campaign shown |
| Tab | Switch the video card: CGA, CGA composite (old, new), EGA, Tandy, VGA, Hercules |
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
| ⌥S (Alt+S on Windows and Linux) | Save the game |
| Enter | When the game is over, on to the battle report |
| Esc | Leave the game (Y or N), back to the title page |

On the battle screen:

| Key | Action |
|-----|--------|
| A | Attack (at the start) |
| A, B, K, S | Choose your Archers, Barbarians, Knights or the whole Squad |
| then A, F, B, S | Attack, Forward, Backoff or Stop; Backspace goes back |
| R | Retreat |

Tab switches between CGA, the CGA on a TV (old and new), EGA, Tandy, VGA and Hercules (in the original, Tab scrolled up), F2 changes the aspect, and Cmd+Enter or Alt+Enter is full screen.

### Screenshots
```sh
AncientArtOfWar --screenshots OUTDIR GAME-FOLDER
```
This writes every screen as BMP files for each card: CGA, EGA, Tandy and VGA (320x200), the CGA on a TV (640x200) and Hercules (720x348), without opening a window. Campaign maps are shown at the bottom of the map, where the game starts.

## Source
| File | Purpose |
|------|---------|
| `src/assets.c` | Loads and decodes the original files: the font and the squad and flag sprites from WAR.EXE, the CGA tiles (M) and pictures (P files), and the VGA pictures and tiles from WARET.DAT |
| `src/gfx.c` | The 320x200 game screen: palette, pictures, tiles and text in the original font; the video cards (the standard 16 colours of an EGA or Tandy, a VGA's 6-bit palette, Hercules) |
| `src/composite.c` | A CGA screen as a TV shows it: the colour of each of 640 dots from the pixel, its neighbours and the palette, for the old and the new CGA (tables measured in DOSBox-X) |
| `src/campaign.c` | Loads the 11 campaigns (T0-TA), their titles (TITLES), squads and flags, and draws a campaign map: 10 x 20 tiles, 9 rows on screen, with the squads and flags on top |
| `src/play.c` | Campaign play, following overlay 6 of WAR.EXE routine by routine (each function names the routine): cursor, commands, path memory, movement, terrain, supply, food and condition, forts, flags, sighting, messages, Info, Size, View, the end of the game, and the enemy commander (the ai_ functions). Runs at 20 iterations a second, about the speed of an IBM PC/XT |
| `src/battle.c` | The battle screen, following overlay 7 of WAR.EXE: the background from the terrain, formations, the soldiers' animation scripts, choosing targets, fighting, archers and arrows, retreats, the order menus and the losses. Runs at 8 frames a second |
| `src/icon.c`, `tools/make_icon.c` | The app icon from the player's own copy of the game: the opening title page in its 4:3 shape on a rounded square; `make_icon` writes the macOS `.iconset` at build time and `iconutil` turns it into `AppIcon.icns` |
| `src/sound.c` | The PC speaker (timer 2 square wave) and the Tandy/PCjr SN76496 (three tone voices), played through SDL audio. The sounds and tunes are WAR.EXE's own: its timer divisors and lengths, the tunes from O2 and the bugle calls from O7, at the speed of an IBM PC/XT: about 50 microseconds per unit of its delay routine (DOSBox plays them about 2.6 times faster) |
| `src/menu.c` | The title pages, scrolls, opponents, quote, campaign menu, Options Shoppe, battle report and saving, following overlay 1 of WAR.EXE. It draws into its own canvas as the original draws into video memory (pictures, XOR boxes, the AND-drawn pointer), so a scroll unrolls over what was on the screen. Also the data disks, and the campaign editor's steps: the editor menu, the scroll editor for the story and title, writing the `T` files and `TITLES` |
| `src/mapedit.c` | The map maker, following overlay 4 of WAR.EXE |
| `src/sqedit.c` | Putting squads and flags on the map, following overlay 3 of WAR.EXE. As in the original, it uses the game's own map drawing (play.c) |
| `src/savegame.c` | Carrying on a game saved by the original: its `S` is WAR.EXE's data from DS:8868 to A9E5, copied field by field into the game |
| `src/formed.c` | Teaching squads new formations, following overlay 5 of WAR.EXE: the soldiers walk by O8's scripts |
| `src/gamelog.c` | The game log (`--log`, `--log=verbose`): the header for each campaign and the lines play.c and battle.c write |
| `src/main.c` | Window, scaling (sharp 4:3 or whole-number square pixels), keys, choosing the game folder, the menus (20 Hz), the 20 Hz game loop and the 8 Hz battle loop |

The file formats are described in [WAR_Analysis/README.md](../WAR_Analysis/README.md).

## Plan
1. Asset loader and viewer (done)
2. Campaign map: load the T files, draw the terrain with tiles, scroll, place squads and flags (done)
3. Strategic rules: movement, terrain, food and supply lines, villages and forts, visibility, spies, flags, Detach, Join and Formation (done)
4. Battles: the fight screen, formations, archers, knights and barbarians (battles on the map and the battle screen done)
5. Opponent AI for the 8 generals, ported from the original program (done)
6. Menus, options, saved games, the battle report and the demo (done)
7. Campaign editor and formation editor (done)
8. Sound: PC speaker and Tandy sound (done)
