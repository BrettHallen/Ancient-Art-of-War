# The Ancient Art of War - Remake
A modern version of The Ancient Art of War (Broderbund, 1984) for macOS, written in C with [SDL3](https://www.libsdl.org/). It also builds on Windows and Linux.

The remake uses the original backgrounds, map tiles and font, in either the **CGA** (4-colour) or **VGA** (16-colour) versions.

**No original game files are included.** The remake loads all graphics (and later the campaigns) from your own copy of the game, so you need the files from the original disk: `WAR.EXE`, `M` and the `P` files, plus `WARET.DAT` for the VGA graphics.

## Status
The viewer shows every picture, the map tiles, the font and the maps of all 11 campaigns from the original files, in CGA or VGA colours. The campaign maps are the start of step 2 of the [plan](#plan).

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

### Keys
| Key | Action |
|-----|--------|
| Left / Right | Previous / next screen |
| Up / Down, - / +, mouse wheel | Scroll a campaign map (as in the original, - scrolls up) |
| Shift + - / +, Home / End | Top / bottom of the map |
| Tab | Switch between CGA and VGA colours |
| A | 4:3 like the original monitor, or square pixels |
| H | Show or hide the caption |
| F, Cmd+Enter | Full screen |
| O | Choose the game folder |
| Esc | Quit |

### Screenshots
```sh
AncientArtOfWar --screenshots OUTDIR GAME-FOLDER
```
This writes every screen in both colour sets as BMP files (320x200), without opening a window. Campaign maps are shown at the bottom of the map, where the game starts.

## Source
| File | Purpose |
|------|---------|
| `src/assets.c` | Loads and decodes the original files: the font from WAR.EXE, the CGA tiles (M) and pictures (P files), and the VGA pictures and tiles from WARET.DAT |
| `src/gfx.c` | The 320x200 game screen: palette, pictures, tiles and text in the original font |
| `src/campaign.c` | Loads the 11 campaigns (T0-TA) and their titles (TITLES), and draws a campaign map: 10 x 20 tiles, 9 rows on screen |
| `src/main.c` | Window, scaling (sharp 4:3 or whole-number square pixels), keys, choosing the game folder |

The file formats are described in [WAR_Analysis/README.md](../WAR_Analysis/README.md).

## Plan
1. Asset loader and viewer (done)
2. Campaign map: load the T files, draw the terrain with tiles, scroll (done), place squads and flags
3. Strategic rules: movement, terrain, food and supply lines, villages and forts, visibility, spies
4. Battles: the fight screen, formations, archers, knights and barbarians
5. Opponent AI for the 8 generals, ported from the original program
6. Menus, options and saved games
7. Campaign editor and formation editor
8. Sound: PC speaker and Tandy sound
