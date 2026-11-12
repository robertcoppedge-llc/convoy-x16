# Memory banking in Convoy

This is the current map. It describes what the program does today. The design notes in [game-plan.md](game-plan.md) are older in places; if they disagree with this file or with `build/CONVOY.map`, this file and the map win.

Related: [mechanics.md](mechanics.md) (what the sim and the views actually do).

Numbers below are from the current `build/CONVOY.map` and `build/CONVOY.PRG` (36,180 bytes). Rebuild and read the map again before treating a byte count as still true.

## Why the build says it is out of memory

The failure is not the PRG file, and it is not banked RAM.

The 65C02 can see 64 KB. This program is one resident image:

| Segment | Address | Size |
|---|---|---|
| CODE | `$082E`–`$8FED` | 34,752 bytes |
| RODATA | `$8FEE`–`$94EA` | 1,277 bytes |
| DATA | `$94EB`–`$9529` | 63 bytes |
| BSS | `$952C`–`$9CDE` | 1,971 bytes |
| C stack | `$9D00`–`$9EFF` | 512 bytes (`__STACKSIZE__=0x0200`) |
| Free before the stack | `$9CDF`–`$9CFF` | **33 bytes** |

BSS is the globals. It is not stored in the PRG; the loader clears it. The linker still places it immediately after the code. Every new function, string, or library call slides BSS up. When BSS would pass `$9CFF`, the link fails with `Segment 'BSS' overflows memory area 'BSS'`. That is the message we keep hitting.

The PRG cap in `x16project.json` is 36,864 bytes. The file is about 700 bytes under that cap. Raising the cap would not help. The collision is in the running machine, between the globals and the stack.

Banked RAM is a different 8 KB window at `$A000`–`$BFFF`. The CPU sees one bank at a time by writing `RAM_BANK`. A 512 KB machine has banks 0–63. Bank 0 is the KERNAL. Banks 1–44 hold art. **Banks 45–63 are empty (152 KB).** None of the game code lives in a bank. cc65 compiled `src/*.c` and the x16-lib objects into the main image above. That is why a 40-byte function can fail the link while 152 KB of banks sit unused.

VERA has its own 128 KB and does not bank. It is tight in a few places and loose in others. It is not what the linker is complaining about.

The stack stays at 512 bytes. It is the call stack, not spare room for features. Making it smaller to "free" BSS would make deep calls unsafe. Making it larger would move `$9D00` down and make the collision happen sooner.

## What actually uses the 1,971 bytes of globals

| Object | BSS |
|---|---|
| `platform.c` | 1,440 bytes |
| `game.c` | 404 bytes |
| `link.c` | 125 bytes |
| cc65 cursor and error | 2 bytes |

`platform.c` is large because it keeps a shadow of all 96 sprite slots so a frame can skip a sprite that did not move: an index, an x, a y, and two attribute bytes per slot. That table alone is 768 bytes. The hub text cache, the blit cache, and the bow-layer registers sit in the rest.

`game.c` is the sim: 11 ships, the wake rings, torpedoes, depth charges, and the solo-AI state. That part is small. The shortage is not the ship list.

`link.c` plus the x16-lib netcard and UART objects are in the main image even during a solo game. They are not the largest piece, but they are resident.

## Rules that follow from this

1. A new feature that adds code or a global in `src/*.c` spends the 33 bytes. Prefer cutting or moving something in the same change.
2. Art and big tables belong in a RAM bank or in VRAM, loaded with `file_load` / `file_load_to_banks`. The profile sheets and the overhead hulls already work this way. One 64×64 frame is copied into a hull slot when it is shown.
3. Code that runs every frame (`draw_map`, `draw_ahead`, the hub, the scopes) is what fills `$082E`–`$8FED`. Moving a whole draw path into a bank, and calling it through a small trampoline that switches `RAM_BANK` and returns, is the same pattern Star Trek uses. A trampoline has to switch back before it returns, because the caller is still in main RAM.
4. Do not put a second copy of a table in main RAM "for convenience" if the bytes already live in a bank.
5. cc65 does not inline. A helper that takes several pointers often costs more than the lines it replaced. Measure with a rebuild and `build/CONVOY.map`, not by counting C lines.

## RAM banks

One bank is 8,192 bytes at `$A000`. A 32,768-byte file occupies four banks. `file_load` of a larger file advances `RAM_BANK` as it goes. Only the bank in the window is visible. Reading a profile frame switches to that bank, copies 2,048 bytes, and switches back.

| Banks | File | What it is |
|---|---|---|
| 0 | — | KERNAL. Do not use. |
| 1–4 | `escort.bin` | Overhead escort, 16 aspects, 64×64 |
| 5–8 | `merchant.bin` | Overhead merchant |
| 9–12 | `tanker.bin` | Overhead tanker |
| 13–16 | `uboat.bin` | Overhead U-boat (all four types share it) |
| 17–20 | `wake.bin` | Overhead wake aspects |
| 21–24 | `mid.bin` | 32-pixel hulls |
| 25 | `midwake.bin` | 32-pixel wakes |
| 26 | `tiny.bin` | 16-pixel hulls |
| 27 | `tinywake.bin` | 16-pixel wakes |
| 28 | `charge.bin` | Depth-charge body and splash |
| 29–32 | `profesc.bin` | Bridge/glass escort, 16 aspects |
| 33–36 | `profmer.bin` | Bridge/glass merchant |
| 37–40 | `proftnk.bin` | Bridge/glass tanker |
| 41–44 | `profubt.bin` | Bridge/glass U-boat |
| 45–63 | free | 152 KB. Nothing loads here. |

The names are `BANK_ESCORT` through `BANK_PROF` in `src/platform.c`. A profile frame is `bank = BANK_PROF + kind * 4 + (frame >> 2)`, then 2,048 bytes at offset `(frame & 3) * 2048` inside that bank.

Zoom art, scope faces, weapons, gauges, the bow, and the sea tiles do not use these banks. They load straight into VRAM.

## VRAM

128 KB, addresses `$00000`–`$1FFFF`. Layer 0 is the sea. Layer 1 is the KERNAL text layer, except on the bridge, where it is the bow picture. Sprites that must hide behind the mask or the bow use Z between the two layers. Sprites in front of layer 1 (the scopes, the gauges, the crosshair) draw over text.

| Address | Size | What |
|---|---|---|
| `$00000` | 768 | Sea tiles, six 16×16 images (deep, mid, chop, foam, sky, horizon). The sky and horizon tiles are not on layer 0 anymore. |
| `$00400` | 8,192 | Layer 0 map, 64×64. The program fills it. Ahead views are all sea. The chart is all sea. |
| `$02400` | 4,992 | Scope beam, face, blips, foam, wake ring |
| `$03800` | 2,048 | Periscope crosshair |
| `$04000` | 256 | Plot marks |
| `$04200` | 2,560 | Zoomed hulls |
| `$04C00` | 640 | Escort numbers |
| `$05000` | 45,056 | 22 slots of 2 KB. The visible hull or profile frame is copied here. Ends at `$10000`. |
| `$10000` | 16,384 | Bow picture map, 128×64 cells of 8×8. Sky and the horizon line are in this map. |
| `$14000` | 15,456 | Bow tiles for the side you are playing, plus a blank tile, a sky tile, and a horizon tile. The other bow stays on disk until the match starts. Ends at `$17C20`. |
| `$18000` | 4,608 | Torpedo, its streak, and the shell |
| `$19200` | — | Depth-charge blit target. The frames themselves are in bank 28. |
| `$19400` | 6,656 | Control-room dials, needles, lamps |
| `$1B000` | KERNAL | Text map. Do not load art here. |
| `$1F000` | KERNAL | Charset, then palette, PSG, and sprite attributes through `$1FFFF` |

Gaps that are real and unused: about 128 bytes after the scope art, 256 bytes after the marks, 384 bytes before the hull slots, 992 bytes after the bow tiles (`$17C20`–`$17FFF`), and about 512 bytes before the text map. None of those is large enough for another 64×64 sheet. A new sheet belongs in banks 45–63, with one frame copied into a hull slot, or it replaces something already in VRAM.

The bow map and the bow tiles meet at `$14000` with no spare between them. Do not grow the map. Growing the picture has to stay under `$18000`.

## Sprite slots

`SPR_N` is 96. Lower index draws in front. The cache in main RAM is why this list costs memory even when a slot is hidden.

| Slots | Use |
|---|---|
| 0–7 | Hulls on the chart, or profile frames in the ahead view |
| 8–11 | Torpedo streaks in the ahead view |
| 11–21 | Chart foam. Hidden. The wake is the rings below. |
| 22–25 | Torpedoes on the chart |
| 26–29 | Depth charges |
| 30 | Contact mark |
| 32–39 | Wake rings. Eight copies of one sprite, left where they were dropped. |
| 40 | Crosshair, or the first control-room gauge |
| 41–58 | Control-room gauges. Ahead-view sprites must not be left here. |
| 59 | Shell |
| 60–63 | Torpedo wakes on the chart |
| 64–67 | Escort numbers |
| 68–95 | Radar, sonar, and hydrophone faces. Chart only. |

## What is not short

- Banked RAM after bank 44.
- The PRG file cap.
- The sim's ship array.

What is short is the 33 bytes between the last global and the stack, because the draw code and the sprite cache both live in the 64 KB the CPU can see without a bank switch.
