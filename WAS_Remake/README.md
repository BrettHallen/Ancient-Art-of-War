# The Ancient Art of War at Sea - Remake
A modern version of The Ancient Art of War at Sea (Brøderbund, 1987) for macOS, written in C with [SDL3](https://www.libsdl.org/). It also builds on Windows and Linux. It is built the same way as the [WAR remake](../WAR_Remake): the original program is ported routine by routine from its disassembly (`../WAS_Analysis/WAS.EXE.asm`) and checked against the original.

The remake uses the original pictures, map tiles, sprites and font, in either the **CGA** (4-colour) version or the **16-colour** version made for the Tandy 1000.

**No original game files are included.** The remake loads everything from your own copy of the game, so you need the files from the original disk: `WAS.EXE`, `SM`, the `SP` files, `AD`, `B`, the `S` files and `SITLES`, plus `WAST.DAT` for the 16-colour graphics.

## Status
Steps 1 and 2 of the plan (`WAS_Remake_plan.md`) are done.

**Campaign play, the map** (step 2, overlay 4 of WAS.EXE): press **Enter** on a campaign's map in the viewer to play it. The game starts as the original does, at the bottom of the map with the marker in the middle:
- the marker (the arrows or the keypad, held down to keep moving; it moves 27 times a second, as the original on an IBM PC/XT) and scrolling (- and +; with Shift, to the top or bottom)
- the command line: *Time (slow) Fleet Pause Surrender*; over a squadron *Move Info Detach Join Zoom Practice*, as the original shows them
- **Info**: the squadron's ships in a panel over the map: name, type, sails and hull, supplies, speed. **E** ends
- **Fleet**: how many flagships, ships of the line and frigates each side has
- **Time**: slow, medium, fast, faster. **Pause**. **Surrender** (Y or N)
- the squadrons are drawn as the original draws them: at anchor, the pennant over a squadron with a flagship; the enemy's are hidden unless the campaign's rules make them always seen

These screens match the original pixel for pixel (checked in DOSBox). Time passes, but nothing moves yet: sailing, the wind, ports and the rest of the rules come with step 3.

**The asset viewer** (step 1) shows
- every picture: the title pages, the scrolls, the opponents, the menu scenes, the Options Shoppe, the four battle reports, the opening's face and the deck for boarding (the 16-colour version has no third and fourth battle report, so those two are shown in their CGA colours)
- the 94 map tiles
- the font
- the map sprites: the anchor, the crossed swords of an encounter, a ship under sail in 16 headings, the flagship's pennant and the merchant's cargo, each as the player's (white with a black outline) and the enemy's (black with a white outline)
- the eleven campaigns: the map with every squadron where it starts (at anchor, with the pennant over a squadron that has a flagship), the opponent and the number of ships on each side; **I** shows the campaign's story, then its squadrons and ships; **Enter** plays it

The rest follows the build order in the plan: the rules, the sea battle and boarding, the enemy, the menus, the editors and sound.

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

### Viewer keys
| Key | Action |
|-----|--------|
| Left / Right, Space | Previous / next screen |
| Up / Down, - / +, mouse wheel | Scroll a campaign map (elsewhere: previous / next screen) |
| Shift + - / +, Home / End | Top / bottom of the map |
| Enter | On a map: play that campaign |
| I | On a map: the story, then the squadrons and ships, then the map again |
| Tab | Switch between the CGA and 16 colours |
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
| I | Info about the squadron under the marker; E ends |
| F | Fleet: the ships on each side; any key returns |
| T | Time: slow, medium, fast, faster |
| P | Pause; any key goes on |
| C | Clear the messages |
| S | Surrender (Y or N) |
| Enter | After the game is over: back to the viewer |

And the remake's own: **Tab** switches between the CGA and 16 colours (in the original Tab scrolls up; - does the same), **F2** changes the aspect, **Alt+Enter** or **Cmd+Enter** full screen, and **Esc** asks *Quit to the title page? (Y or N)* (for now it goes back to the viewer).

## Tests
See `test/README.md`.
