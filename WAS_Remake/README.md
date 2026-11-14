# The Ancient Art of War at Sea - Remake
A modern version of The Ancient Art of War at Sea (Brøderbund, 1987) for macOS, written in C with [SDL3](https://www.libsdl.org/). It also builds on Windows and Linux. It is built the same way as the [WAR remake](../WAR_Remake): the original program is ported routine by routine from its disassembly (`../WAS_Analysis/WAS.ASM`) and checked against the original.

The remake uses the original pictures, map tiles, sprites and font, shown as the original showed them on each kind of display it supported: **CGA** (4 colours; an EGA or VGA card showed the same), a **PCjr** (WAS.EXE chose its own four colours for each screen there), the **Tandy 1000** (the 16-colour version, WAST.EXE) or a **Hercules** monochrome card (the original's `WAS HERC`).

**No original game files are included.** The remake loads everything from your own copy of the game, so you need the files from the original disk: `WAS.EXE`, `SM`, the `SP` files, `SO0`-`SO8`, `AD`, `B`, the `S` files, `SITLES` and `SDITLES`, plus `WAST.DAT` for the 16-colour graphics.

## Status
Steps 1 to 9 of the plan (`WAS_Remake_plan.md`) are done (9: the video modes).

**Sound** (step 8): the PC speaker, as the original drives it: every beep, buzz and tune, the scroll crackling as it unrolls, the low notes as a game starts and ends, a port running empty, the crown's tune, and the battle's guns, crashes, calls and boarding. Each note lasts the time the original's delays gave it on an IBM PC/XT, played at the time the game made it. The Options Shoppe's sound line works as in the original: *all sounds are off*, *do not play music* (the tunes stay quiet), *all sounds are on*.

**The campaign editors** (step 7, overlays 1 to 3 of WAS.EXE): **make changes** now makes, changes and removes your own campaigns on a data disk, as in the original.
- *Make a new campaign.* takes six steps: the **map maker** (a page of 18 tiles at a time, 90 in all: - and + turn the pages, the arrows move the cursor over the map, the letters A-S put a tile down; a port or crown must be next to water and have a way by sea to the map's edge), the **ships editor** (S makes a squadron at the marker; on a squadron, I opens its panel: type each ship's name, - and + go from ship to ship, T changes its type, S and H its sails and hull, R takes it out; on the squadron's line F makes it friend or foe and S sets its supplies; E ends; G and P move a squadron, R removes it; F counts the fleets), the **enemy leader**, the **rules**, the **story** (typed on the scroll; Esc when done) and the **title** (typed on the titles scroll; Enter when done). Each step asks *Continue to the next step? (Y or N)*; Esc on a step's first page gives up
- *Change a campaign.*: choose it from the data disk's titles, then the change menu: RULEBOOK, STORYBOOK, MAP MAKER, ENEMY, FLEET, EXIT; each asks *Save the changes? (Y or N)*
- *Remove a campaign from a data disk.*: choose it, *Erase this title? (Y or N)*, and the title is rubbed out
- the campaigns are written in the original's format (the 2,180-byte `S` files and `SITLES`), so the original game can play and change them, and the remake the original's; checked byte for byte against files the original wrote in DOSBox. To play one, choose *( data disk )* on the titles scroll

**The menus** (step 6, overlay 1 of WAS.EXE): the remake now starts as the original does.
- *presented by* (the opening tune's length; a key skips it), then the title page: **go to war**, **make changes** or **quit**, chosen with - and + and Enter
- **go to war**: the scroll unrolls with the titles (**Esc** goes back), then the campaign's story (- and + scroll it), then its rules (**Space** changes the rule under the sword), then the six opponents (+ goes through them; their portraits blink while you choose), then a quote and *PREPARE FOR WAR!* while the campaign loads
- **make changes**: the campaign menu (- and + move the frame, Enter): *The Ancient Art of War.* plays the opening (Sun Tzu's lines in time with its tune), *Ye Olde Options Shoppe.* sets the fight delay, the wind, the reload time, the cannon aim, the display, the data disk, the sounds and the joystick (Space changes the line under the sword, Enter keeps them, in `SCNF`); making, changing and removing campaigns are step 7's (above)
- **saving**: **Alt+S** during play saves the game on the data disk (*<C>ontinue with game or <Q>uit?*); a saved game there is asked about when you open *( data disk )* on the titles scroll: *Saved game* or *New campaigns*
- the **battle report** at the end of a game, won or lost, with the picture for your opponent, the ships sunk and captured and the flags taken
- the **demo**: left alone on the title page for about 7 seconds, the game plays one of the campaigns by itself, scrolling to the news and zooming into a battle; any key ends it

The pages are drawn as WAS.EXE draws them and take the time they took on an IBM PC/XT (the scroll unrolling, the pause while a campaign loads); they match the original in DOSBox pixel for pixel. Saved games are in the original's format (`SS`, the 10,444 bytes of WAS.EXE's data it saves): the original's saved games load in the remake, and the remake's in the original. Data disks are folders in `~/Library/Application Support/AncientArtOfWarAtSea/Remake/Data Disks/`, with the original's files (`SITLES`, the `S` files, `SS`); the Options Shoppe's data disk line chooses one, or makes a new one. In the Tandy mode the pages are the Tandy 1000 version's pictures, with its white text and its pointer.

**The enemy commander** (step 5, overlay 4 of WAS.EXE): the enemy's squadrons now sail. Each in turn is given orders as in the original: stay in port to take on supplies or repairs, guard its crown, go for yours or for your flagship, or for wherever your squadrons look weak on its strength map, finding a route that keeps clear of your strength and of water its ships should avoid, and attacking any squadron it can beat on the way. Merchants sail to the ports that call for them and fill them up, and can be taken. How bold and how careful the enemy is comes from the opponent you chose. Its routines were checked against WAS.EXE's own code, run in an emulator from the same game state, and matched byte for byte. The time this thinking took on an IBM PC/XT is kept: at the fastest time a game goes at about two ticks a second, as the original does.

**Sea battles and boarding** (step 4, overlays 5 to 7 of WAS.EXE):
- **Zoom** (**Z** over your squadron while an encounter counts down) fights the encounter as a sea battle; **Practice** (**P** over a squadron that has no enemy near) sails it on its own, with the guns loading at their fastest
- the battle: your ships and the enemy's in the overview or the close **View**, each under its orders (Course, follow, chase, avoid ...), sailing with the wind, turning, firing broadsides; sails get holed, hulls damaged, ships strike their colours, sink, run aground or grapple one another. Your selected ship is steered and fought from the panel: **G**un side, **C**ourse, **S**peed (sails), **O**rders, **A**im, **F**ire, **+** / **-** for your next ship, **T**ime, **P**ause, **L**eave
- **boarding**: when ships grapple, *X grapples Y. Zoom Pass*: **Z** goes on deck. Forty men of the two crews, as many on each side as the ships' damage, types and other ships alongside allow. **A**ttack (or **L**eave), then send your men FROM one part of your deck TO another (**F**lag, **Q**uarterdeck, **P**oopdeck, **R**ailing; **C**ross to the enemy's flag; **A**gain sends another; Esc starts over). The side that clears the other's deck or reaches its flag takes the ship
- afterwards the map shows the ships captured and lost, and the game goes on

The battle runs at the speed it had on an IBM PC/XT, as the rest of the game does: its first screens and the boarding's deck match the original in DOSBox pixel for pixel, and its timings were measured there. In the Tandy mode the battle is drawn as WAST.EXE draws it: in the 4-colour mode, in light blue, brown, black and white, with plain sea and land, and boarding in 16 colours on the Tandy version's deck. These, the map with its marker, Info and Fleet, and the first menu pages (the title pages, the titles, the story, the rules, the opponents) match WAST.EXE in DOSBox-X pixel for pixel, as the PCjr's title page, scroll, map and battle match WAS.EXE on a PCjr.

**Campaign play, the rules** (step 3, overlay 4 of WAS.EXE): your squadrons can now be sailed, and the game follows the original's rules:
- **Move**: over one of your squadrons press **M**, trace its path with the marker (it becomes a box that stays on the sea and moves at half speed, as in the original), and press **M** to mark the end (**C** cancels). The squadron sets out at full sail and a little cross marks where it will stop; in **Info**, **S** changes its speed (anchored, half sail, full sail)
- sailing: how long each step takes depends on the speed, the kind of ships (ships of the line are slower than frigates), torn sails and, with the wind on in the Options Shoppe's file (SCNF), the wind; the wind's face on the map shows where it blows from, and it changes now and then
- rough water (dark) may sink frigates, shallow water (light) ships of the line, as the campaign's rules say
- supplies: squadrons eat; hungry ones suffer damage; a squadron at anchor beside a supply port, a repair port or its crown takes supplies and repairs ("In Port" in Info), and a port with nothing left is drawn inverted
- sighting: the enemy's squadrons appear when yours come close enough (the campaign's visibility), with *ALERT - ENEMY SIGHTED IN THE ...*
- encounters: squadrons that meet are marked with crossed swords; unless you Zoom (step 4), they fight it out on the map after a while: *WARNING - ENCOUNTER*, *A L E R T - FIGHTING*, *NOTICE - BATTLE WON / LOST*, and flagships taken
- **Detach** (**D**) and **Join** (**J**): move ships between a squadron and a new one, or one beside it (**-** / **+** mark a ship, **S** switches it, **E** ends)
- the end: the last ship or the last flagship of a side lost, or a crown taken (*BLACK CROWN CAPTURED*), shows *You Won* or *You Lost*; the opponent may offer to surrender (Y or N)

The tracing box, the path's mark, sailing to the end of a path, Join, Detach, the messages, "In Port", the wind's face, a crown taken with *You Won*, and the opponent's offer match the original pixel for pixel (checked in DOSBox).

**Campaign play, the map** (step 2): **go to war** from the title page (or press **Enter** on a campaign's map in the viewer to play it straight away). The game starts as the original does, at the bottom of the map with the marker in the middle:
- the marker (the arrows or the keypad, held down to keep moving; it moves 27 times a second, as the original on an IBM PC/XT) and scrolling (- and +; with Shift, to the top or bottom)
- the command line: *Time (slow) Fleet Pause Surrender*; over a squadron *Move Info Detach Join Zoom Practice*, as the original shows them
- **Info**: the squadron's ships in a panel over the map: name, type, sails and hull, supplies, speed. **E** ends
- **Fleet**: how many flagships, ships of the line and frigates each side has
- **Time**: slow, medium, fast, faster. **Pause**. **Surrender** (Y or N)
- the squadrons are drawn as the original draws them: at anchor or under sail in 16 headings, the pennant over a squadron with a flagship; the enemy's are hidden unless seen or the campaign's rules make them always seen

**The asset viewer** (step 1) shows
- every picture: the title pages, the scrolls, the opponents, the menu scenes, the Options Shoppe, the four battle reports, the opening's face and the deck for boarding (the 16-colour version has no third and fourth battle report, so those two are shown in their CGA colours)
- the 94 map tiles
- the font
- the map sprites: the anchor, the crossed swords of an encounter, a ship under sail in 16 headings, the flagship's pennant and the merchant's cargo, each as the player's (white with a black outline) and the enemy's (black with a white outline)
- the eleven campaigns: the map with every squadron where it starts (at anchor, with the pennant over a squadron that has a flagship), the opponent and the number of ships on each side; **I** shows the campaign's story, then its squadrons and ships; **Enter** plays it

All the steps of the plan's build order are done.

## Building on macOS
Install the tools with [Homebrew](https://brew.sh):
```sh
brew install cmake sdl3
```

Then build:
```sh
cd WAS_Remake
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

This makes `build/AncientArtOfWarAtSea.app`.

The app's icon is the title page, drawn during the build from your own game files, so no original artwork is kept in the repository. CMake finds them in `../WAS_Original_Files_3.5`; if they are somewhere else, add `-DWAS_GAME_DIR=/path/to/your/game` to the first `cmake` command. Without them the app builds with no icon in Finder, and the program still shows the icon in the Dock once it has loaded your files. If Finder keeps showing an old icon, run `touch build/AncientArtOfWarAtSea.app`.

## Running
```sh
open build/AncientArtOfWarAtSea.app
```
On first launch the app asks for the folder that holds your original game files, and remembers it (in `~/Library/Application Support/AncientArtOfWarAtSea/Remake/`). You can also give the folder on the command line:
```sh
build/AncientArtOfWarAtSea.app/Contents/MacOS/AncientArtOfWarAtSea ../WAS_Original_Files_3.5
```

The app also looks in a `game` folder next to itself and in the current folder. Press **O** to choose a different folder.

**Video mode:** the app starts in CGA; `--video=` chooses another, and Tab goes round them (CGA, PCjr, Tandy, Hercules):
```sh
build/AncientArtOfWarAtSea.app/Contents/MacOS/AncientArtOfWarAtSea --video=tandy    # cga, pcjr, tandy or herc
```
`--video=ega` and `--video=vga` start in CGA, as WAS.EXE looked the same on those cards. The game plays the same in every mode: the Tandy version's own test for land when tracing a path (its 16-colour coastlines, a few pixels different at some shores) is not used, WAS.EXE's is (`WAS_Analysis/README.md`, "The Tandy version"). The Tandy mode needs `WAST.DAT`. On a PCjr the Options Shoppe's display line (rgb, tv/composite, black/white) chooses which of each screen's colour sets is used, as in the original.

**Hercules:** as `WAS HERC` did, `HERC` (or `--video=herc`) on the command line starts the app on a Hercules card:
```sh
build/AncientArtOfWarAtSea.app/Contents/MacOS/AncientArtOfWarAtSea HERC
```
WAS.EXE drew the same picture for Hercules as for CGA, with each 4-colour pixel shown as two black or lit pixels, 640x200 in the middle of the card's 720x348 screen; the remake shows that whole screen (4:3, or square pixels with F2), lit pixels in DOSBox's grey. It matches DOSBox's Hercules pixel for pixel.

**Game log (debugging):** `--log` writes a log of each campaign played, to see what the enemy commander was "thinking" (as in the WAR remake):
```sh
build/AncientArtOfWarAtSea.app/Contents/MacOS/AncientArtOfWarAtSea --log            # decisions and events
build/AncientArtOfWarAtSea.app/Contents/MacOS/AncientArtOfWarAtSea --log=verbose    # and much more (or: --log verbose)
```
Each campaign started (or saved game carried on) opens a new text file in `Logs/` in the remake's folder (`~/Library/Application Support/AncientArtOfWarAtSea/Remake/Logs/` on macOS), named with the date, time and campaign, such as `2026-11-13 23-08-50 1805 Trafalgar.log`. It starts with the campaign and its settings: the title and S file, the opponent and its 25 values (what each one does), the rules as the Rulebook words them, the Options Shoppe's fight delay, wind, reload time and cannon aim, random()'s state, and a map of the cells; then the game at the start: the ports and crowns, every squadron with its ships, and the enemy's plans. Then one line per decision or event, after the time on the map and the game tick (squadrons are named by slot with their ships, x,y and cell):
```
[  0:00 tick     0] ENEMY new standing target for the enemy's squadron 1 (BLACK PLAGUE line, ...) at 62,192 cell 197 sea: cell 13 (row 1 col 3, sea), the best score 41 (...)
[  0:00 tick     0] ENEMY route for squadron 1 from cell 197 (row 19 col 7, sea) to cell 13 (row 1 col 3, sea): 25 cells (cost 0), sails the first 10: W NW W NW NW NW NW N NE NE
[  0:20 tick    16] ENEMY the enemy's squadron 8 (...) attacks your squadron 0 (...), 8 steps away (strength 36 + 6065 90 against 24; attacks within 6063 x 8 = 24)
[  4:46 tick   338] BATTLE   93.5 s  "WICKED DESIRE grapples DRUID."
[  4:46 tick   338] BATTLE  107.6 s  boarding: one of your men falls; you have 15 men standing, they 24
```
- **ENEMY**: the commander's choices and the reasons, with the values behind them: going for supplies or repairs, guarding its crown, its standing target (your crown, your flagship, or the cell with the best score) and dropping it, its routes (cells, cost and the directions of the first part), the squadrons it attacks, arriving, joining ships, merchants unloading, what it learns from a lost merchant, the changes to its values as the game goes, offering to surrender.
- **YOU**: your orders and answers: Move (where to, how many steps), Mark, Cancel, Speed, Detach, Join, End, Time, Zoom, Practice, Surrender, Esc, Alt+S, the offer.
- **EVENT**: sightings, encounters, fights on the map with each hit and ship sunk, merchants met and taken and setting out, empty ports, ships lost in rough or shallow water and flagships made, squadrons short of supplies, the wind, crowns taken, the end of the game.
- **BATTLE**, with the battle's own clock: the two squadrons, the shore and each ship, your orders (sails, course, orders, guns, aim, fire, time, leave), every message on the battle screen (grapples, boards, strikes her colours, sinks, retreats), each hit, and the boarding: Zoom or Pass, the crews, your commands, each man who falls, who won; then the losses.

`--log=verbose` adds each squadron the enemy looks at and why it stays (taking on supplies, repairs, guarding its crown, your squadron close), every cell it weighed for a target with its score, its route searches, the strength and port-value maps whenever they are worked out again, supplies and repairs taken in port, every broadside fired, and a table of all squadrons every 16 ticks. A game ends the log with the reason (you won: you took the enemy's crown ... you quit) and the state at the end; after Alt+S the log stays open in case you carry on. The demo isn't logged. Logging only reads the game: it never calls the random number routine or the routines that count the XT's time, so a logged game plays exactly as one not logged (`test/test_gamelog.c` checks this byte for byte).

### Viewer keys
| Key | Action |
|-----|--------|
| Left / Right, Space | Previous / next screen |
| Up / Down, - / +, mouse wheel | Scroll a campaign map (elsewhere: previous / next screen) |
| Shift + - / +, Home / End | Top / bottom of the map |
| Enter | On a map: play that campaign |
| I | On a map: the story, then the squadrons and ships, then the map again |
| Tab | Switch between CGA, PCjr, Tandy and Hercules |
| F2 or A | 4:3 like the original monitor, or square pixels |
| H | Show or hide the caption |
| F, Cmd+Enter, Alt+Enter | Full screen |
| O | Choose the game folder |
| Esc | Back to the title page |

The viewer opens with **F12** on the title page. `--screenshots OUTDIR [game-folder]` writes every screen in both colour sets as BMP files, without opening a window.

### The menus
The original keys: **-** and **+** (on the keypad, or - and = on the main keyboard) to choose, **Enter** to select, **Esc** to go back a page, **Space** to change a rule or an option, and the letters the pages ask for (S or N, C or Q). Any key ends the demo.

### The editors
The original keys, shown on the bottom row of each page. Map maker: arrows move the cursor, - and + turn the tile pages, A-S (not P) put a tile down, Enter when done. Ships editor: arrows and Home, Page Up, End, Page Down move the marker (hold them), - and + scroll the map (with Shift, to the top or bottom), S a new squadron, I Info, G Get and P Put, R Remove, F Fleet, Enter when done; in the Info panel - and + choose a ship or the squadron's line, N name, T type, S sails (or supplies), H hull, R remove, F friend or foe, E end. The story and the titles are typed; the arrows move the pen, Backspace rubs out.

### Playing
The original keys:

| Key | Action |
|-----|--------|
| Arrows, keypad | Move the marker (Home, Page Up, End, Page Down and keypad 7, 9, 1, 3 go diagonally; holding two arrows does too) |
| - / +, Tab / Ctrl | Scroll the map; with Shift, jump to the top or bottom |
| M | Over your squadron: trace its path; M again marks the end, C cancels |
| I | Info about the squadron under the marker (S: its speed, on a traced path); E ends |
| Z | Over your squadron in an encounter: Zoom into the sea battle |
| P | Over your squadron with no enemy near: Practice (elsewhere: Pause) |
| D, J | Detach, Join: - and + mark a ship, S switches it to the other squadron, E ends |
| F | Fleet: the ships on each side; any key returns |
| T | Time: slow, medium, fast, faster |
| P | Pause; any key goes on |
| C | Clear the messages |
| S | Surrender (Y or N) |
| Alt+S | Save the game on the data disk |
| Y, N | Answer the opponent's offer to surrender |
| Enter | After the game is over: the battle report |

In the sea battle:

| Key | Action |
|-----|--------|
| G | The side the gun crew works |
| C, S, A | Course, Speed (sails), Aim: - and + change it, Enter ends |
| O | Orders for the selected ship: - and + choose, Enter ends |
| F | Fire the loaded guns |
| + / - | Your next / previous ship |
| V | The overview or the close view |
| Space | The view back on your ship |
| 1-9, keypad | Scroll the view |
| T | Time: steps through the five speeds |
| P | Pause |
| L | Leave the battle |
| Z, P | When ships grapple: Zoom (board) or Pass |
| A, L | On deck: Attack, or Leave |
| F, Q, P, R, C, A | On deck: FROM / TO Flag, Quarterdeck, Poopdeck, Railing, Cross to the enemy's flag; Again sends another man; Esc starts over |

And the remake's own: **Tab** switches between CGA, PCjr, Tandy and Hercules (in the original Tab scrolls up; - does the same), **F2** changes the aspect, **Alt+Enter** or **Cmd+Enter** full screen, and **Esc** asks *Quit to the title page? (Y or N)*.

## Tests
See `test/README.md`.
