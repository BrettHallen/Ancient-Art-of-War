# diff_ovl4: the remake against WAS.EXE's own code

WAS.EXE's resident code and overlay 4 (campaign play, the enemy commander) are run under the [unicorn](https://www.unicorn-engine.org/) emulator, from the player's own WAS.EXE, on a data segment written out by the remake (`src/dsmap.c`: the remake's state in WAS.EXE's layout). The same routine then runs in the remake, and the two data segments are compared byte by byte. Interrupts are ignored and the drawing routines (erase_squadrons, redraw_squadrons, invert_tile_buf, draw_squadrons, draw_cursor) return at once.

Needs Python 3 with `unicorn` (and `numpy`, `scipy` for the timing fit), and the game folder at `../../../WAS_Original_Files_3.5` or in `$WAS_GAME`.

```
SDL3_DIR=/path/to/sdl3 ./build.sh          # libwas.so: the remake's campaign play
python3 diff.py turn 0,1,2,3,4,5,6,7,8,9,10 1,2 10
```

`diff.py FUNC CAMPAIGNS SEEDS SAMPLES [FRAMES]`: for each campaign and seed the remake plays on at the fastest time, FRAMES iterations (60) between samples; FUNC is one of

| FUNC | WAS.EXE | The remake |
|---|---|---|
| turn | ovl4:C79A (the enemy gives the next squadron orders) | enemy_turn |
| strength | ovl4:E805 (the strength map) | enemy_strength_map |
| pvalue | ovl4:EA51 (what the cells by ports and crowns are worth) | enemy_port_values |
| encounters | ovl4:8BCD | encounters |
| move | ovl4:96A0 | move_squadrons |
| eat | ovl4:9345 | eat |
| battles | ovl4:8EDE | map_battles |
| restock | ovl4:BCD2 | restock |
| ports | ovl4:C07C (the ports merchants sail to, sea_way ovl4:E5BF for each) | port_sources |

After a difference the remake carries on from WAS.EXE's state. 13 Nov 2026: every one of them matched in every sample (about 180 each over the 11 campaigns and two seeds). `ports` was added in step 6: the remake's sea_way had put each cell back in its queue at the wrong place (WAS.EXE's `queue[l6++]`), so it found only the ports beside the map's edge; fixed, it matched in all 132 samples.

**The timing** (`play.c`, EV_WEIGHT): `calib.py` counts the instructions WAS.EXE executes in each of these routines and the events the remake counts in the same call (`play_int.h`, EV); `fit.py` fits each event's weight in instructions and each routine's fixed part. What the drawing routines cost (skipped here) was measured by running them: about 3,350 + 500 a squadron shown for erase_squadrons + redraw_squadrons (`redraw_work`). sea_way's own events (EV_SEAPOP, EV_SEAQ, EV_SEAB) were fitted the same way to port_sources' counts (it is most of the pause before a new game's map: 1.27 million instructions for The Bane of Oldain).
