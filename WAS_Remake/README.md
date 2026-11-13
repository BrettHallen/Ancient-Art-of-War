# The Ancient Art of War at Sea - Remake
A modern version of The Ancient Art of War at Sea (Brøderbund, 1987) for macOS, written in C with [SDL3](https://www.libsdl.org/). It also builds on Windows and Linux. It is built the same way as the [WAR remake](../WAR_Remake): the original program is ported routine by routine from its disassembly (`../WAS_Analysis/WAS.EXE.asm`) and checked against the original.

The remake uses the original pictures, map tiles, sprites and font, in either the **CGA** (4-colour) version or the **16-colour** version made for the Tandy 1000, or shown as on a **Hercules** monochrome card (the original's `WAS HERC`).

**No original game files are included.** The remake loads everything from your own copy of the game, so you need the files from the original disk: `WAS.EXE`, `SM`, the `SP` files, `AD`, `B`, the `S` files and `SITLES`, plus `WAST.DAT` for the 16-colour graphics.

## Status
Steps 1 to 4 of the plan (`WAS_Remake_plan.md`) are done.

**Sea battles and boarding** (step 4, overlays 5 to 7 of WAS.EXE):
- **Zoom** (**Z** over your squadron while an encounter counts down) fights the encounter as a sea battle; **Practice** (**P** over a squadron that has no enemy near) sails it on its own, with the guns loading at their fastest
- the battle: your ships and the enemy's in the overview or the close **View**, each under its orders (Course, follow, chase, avoid ...), sailing with the wind, turning, firing broadsides; sails get holed, hulls damaged, ships strike their colours, sink, run aground or grapple one another. Your selected ship is steered and fought from the panel: **G**un side, **C**ourse, **S**peed (sails), **O**rders, **A**im, **F**ire, **+** / **-** for your next ship, **T**ime, **P**ause, **L**eave
- **boarding**: when ships grapple, *X grapples Y. Zoom Pass*: **Z** goes on deck. Forty men of the two crews, as many on each side as the ships' damage, types and other ships alongside allow. **A**ttack (or **L**eave), then send your men FROM one part of your deck TO another (**F**lag, **Q**uarterdeck, **P**oopdeck, **R**ailing; **C**ross to the enemy's flag; **A**gain sends another; Esc starts over). The side that clears the other's deck or reaches its flag takes the ship
- afterwards the map shows the ships captured and lost, and the game goes on

The battle runs at the speed it had on an IBM PC/XT, as the rest of the game does: its first screens and the boarding's deck match the original in DOSBox pixel for pixel, and its timings were measured there. In the 16-colour mode the battle is shown in its CGA colours for now (how the Tandy version draws it is still to be found out). The battle's sounds come with step 8.

**Campaign play, the rules** (step 3, overlay 4 of WAS.EXE): your squadrons can now be sailed, and the game follows the original's rules:
- **Move**: over one of your squadrons press **M**, trace its path with the marker (it becomes a box that stays on the sea and moves at half speed, as in the original), and press **M** to mark the end (**C** cancels). The squadron sets out at full sail and a little cross marks where it will stop; in **Info**, **S** changes its speed (anchored, half sail, full sail)
- sailing: how long each step takes depends on the speed, the kind of ships (ships of the line are slower than frigates), torn sails and, with the wind on in the Options Shoppe's file (SCNF), the wind; the wind's face on the map shows where it blows from, and it changes now and then
- rough water (dark) may sink frigates, shallow water (light) ships of the line, as the campaign's rules say
- supplies: squadrons eat; hungry ones suffer damage; a squadron at anchor beside a supply port, a repair port or its crown takes supplies and repairs ("In Port" in Info), and a port with nothing left is drawn inverted
- sighting: the enemy's squadrons appear when yours come close enough (the campaign's visibility), with *ALERT - ENEMY SIGHTED IN THE ...*
- encounters: squadrons that meet are marked with crossed swords; unless you Zoom (step 4), they fight it out on the map after a while: *WARNING - ENCOUNTER*, *A L E R T - FIGHTING*, *NOTICE - BATTLE WON / LOST*, and flagships taken
- **Detach** (**D**) and **Join** (**J**): move ships between a squadron and a new one, or one beside it (**-** / **+** mark a ship, **S** switches it, **E** ends)
- the end: the last ship or the last flagship of a side lost, or a crown taken (*BLACK CROWN CAPTURED*), shows *You Won* or *You Lost*; the opponent may offer to surrender (Y or N)

The tracing box, the path's mark, sailing to the end of a path, Join, Detach, the messages, "In Port", the wind's face, a crown taken with *You Won*, and the opponent's offer match the original pixel for pixel (checked in DOSBox). The enemy does not move yet (step 5), and neither do the merchants that bring supplies to ports (step 5, as they use the enemy's routes).

**Campaign play, the map** (step 2): press **Enter** on a campaign's map in the viewer to play it. The game starts as the original does, at the bottom of the map with the marker in the middle:
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

The rest follows the build order in the plan: the enemy, the menus, the editors and sound.

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

**Hercules:** as `WAS HERC` did, `HERC` on the command line starts the app on a Hercules card (Tab gets there too):
```sh
build/AncientArtOfWarAtSea.app/Contents/MacOS/AncientArtOfWarAtSea HERC
```
WAS.EXE drew the same picture for Hercules as for CGA, with each 4-colour pixel shown as two black or lit pixels, 640x200 in the middle of the card's 720x348 screen; the remake shows that whole screen (4:3, or square pixels with F2), lit pixels in DOSBox's grey. It matches DOSBox's Hercules pixel for pixel.

### Viewer keys
| Key | Action |
|-----|--------|
| Left / Right, Space | Previous / next screen |
| Up / Down, - / +, mouse wheel | Scroll a campaign map (elsewhere: previous / next screen) |
| Shift + - / +, Home / End | Top / bottom of the map |
| Enter | On a map: play that campaign |
| I | On a map: the story, then the squadrons and ships, then the map again |
| Tab | Switch between CGA, 16 colours and Hercules |
| F2 or A | 4:3 like the original monitor, or square pixels |
| H | Show or hide the caption |
| F, Cmd+Enter, Alt+Enter | Full screen |
| O | Choose the game folder |
| Esc | Quit |

`--screenshots OUTDIR [game-folder]` writes every screen in both colour sets as BMP files, without opening a window.

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
| Y, N | Answer the opponent's offer to surrender |
| Enter | After the game is over: back to the viewer |

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

And the remake's own: **Tab** switches between CGA, 16 colours and Hercules (in the original Tab scrolls up; - does the same), **F2** changes the aspect, **Alt+Enter** or **Cmd+Enter** full screen, and **Esc** asks *Quit to the title page? (Y or N)* (for now it goes back to the viewer).

## Tests
See `test/README.md`.
