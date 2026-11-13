# Memory banking in Convoy

This is the current map. It describes what the program does today. The design notes in [game-plan.md](game-plan.md) are older in places; if they disagree with this file or with `build/CONVOY.map`, this file and the map win.

Related: [mechanics.md](mechanics.md) (what the sim and the views actually do).

Numbers below are from the current `build/CONVOY.map` and `build/CONVOY.PRG` (36,021 bytes). Rebuild and read the map again before treating a byte count as still true.

## Why the build says it is out of memory

The failure is not the PRG file, and it is not banked RAM.

The 65C02 can see 64 KB. This program is one resident image:

| Segment | Address | Size |
|---|---|---|
| CODE | `$082E`–`$8EEC` | 34,495 bytes |
| RODATA | `$8EED`–`$944B` | 1,375 bytes |
| DATA | `$944C`–`$948A` | 63 bytes |
| BSS | `$948D`–`$9C47` | 1,979 bytes |
| C stack | `$9D00`–`$9EFF` | 512 bytes (`__STACKSIZE__=0x0200`) |
| Free before the stack | `$9C48`–`$9CFF` | **184 bytes** |
| PHASECODE | `$A000`–`$A354` | 853 bytes, bank 63, file `phase.bin` |

BSS is the globals. It is not stored in the PRG; the loader clears it. The linker still places it immediately after the code. Every new function, string, or library call slides BSS up. When BSS would pass `$9CFF`, the link fails with `Segment 'BSS' overflows memory area 'BSS'`. That is the message we keep hitting.

The PRG cap in `x16project.json` is 36,864 bytes. The file is about 800 bytes under that cap. Raising the cap would not help. The collision is in the running machine, between the globals and the stack.

Banked RAM is a different 8 KB window at `$A000`–`$BFFF`. The CPU sees one bank at a time by writing `RAM_BANK`. A 512 KB machine has banks 0–63. Bank 0 is the KERNAL. Banks 1–50 hold art once a side has been chosen. Bank 63 holds `phase.bin` (the title card, the credits card, and the match loader). Banks 51–62 are empty. The draw path still lives in main RAM, because it runs every frame.

VERA has its own 128 KB and does not bank. It is tight in a few places and loose in others. It is not what the linker is complaining about.

The stack stays at 512 bytes. It is the call stack, not spare room for features. Making it smaller to "free" BSS would make deep calls unsafe. Making it larger would move `$9D00` down and make the collision happen sooner.

## Phases

Disk is for a phase change. A view change is not a phase change.

| Phase | What is on the disk | What is in memory |
|---|---|---|
| Title | `title.bin`, a 2bpp 640×480 card (`assets/pictures/title.x16pic.json`) | Layer 0 is that card. No hull sheets, no sea tiles. `C` opens credits. |
| Credits | `credits.bin`, the same size and slot | Same layer. Any key returns to the title. |
| Match | The side you picked, loaded once before the first frame | Chart, bridge, glasses, zoom steps, and (U-boat) the control room. Already in banks or VRAM. `B`, `G`, `C`, and `Z` do not open a file. |

`1` and `2` are the moment the disk runs. The loader is `phase_run` in bank 63. It fills the hull banks, the profile banks and their smaller copies, the chart's 32-pixel and 16-pixel sheets, and the VRAM the views share (sea, scopes, weapons, zoom sprites, and that side's bow). A U-boat match also loads the periscope crosshair and the control-room gauges. An escort match does not. The other bow stays on disk.

The title card occupies `$00000`–`$12BFF`, which is the same VRAM the sea and the scopes use. Leaving the title and starting a match reloads that VRAM. The sheets in banks 1–44 are not in that range, so a second match does not read them from disk again.

The chart wake on screen is the shared ring in `scope.bin`. `wake.bin`, `midwake.bin`, and `tinywake.bin` stay on disk; nothing reads them.

## What actually uses the 1,978 bytes of globals

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

1. A new feature that adds code or a global in main RAM spends the 184 bytes. Prefer cutting or moving something in the same change. Title, credits, and the match loader already live in bank 63.
2. Art and big tables belong in a RAM bank or in VRAM, loaded with `file_load` / `file_load_to_banks`. The profile sheets and the overhead hulls already work this way. One 64×64 frame is copied into a hull slot when it is shown.
3. Code that runs every frame (`draw_map`, `draw_ahead`, the hub, the scopes) stays in main RAM so a view change does not switch banks in the draw. `phase_call` in `src/phasecall.s` is the trampoline for bank 63. It switches to that bank, calls `phase_run`, and switches back. Do not call it from the frame loop.
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
| 17–20 | `wake.bin` | On disk. The chart wake is the ring in `scope.bin`. |
| 21–24 | `mid.bin` | 32-pixel hulls, loaded with the match |
| 25 | `midwake.bin` | On disk. Not read. |
| 26 | `tiny.bin` | 16-pixel hulls, loaded with the match |
| 27 | `tinywake.bin` | On disk. Not read. |
| 28 | `charge.bin` | Depth-charge body and splash, loaded with the match |
| 29–32 | `profesc.bin` | Bridge/glass escort, 16 aspects |
| 33–36 | `profmer.bin` | Bridge/glass merchant |
| 37–40 | `proftnk.bin` | Bridge/glass tanker |
| 41–44 | `profubt.bin` | Bridge/glass U-boat, 64px. The glasses use this same sheet. |
| 45 | `p32esc.bin` | 32px escort profiles, 16 aspects |
| 46 | `p32mer.bin` | 32px merchant profiles |
| 47 | `p32tnk.bin` | 32px tanker profiles |
| 48 | `p32ubt.bin` | 32px U-boat profiles |
| 49 | `p16prof.bin` | 16px profiles, all four kinds |
| 50 | `p8prof.bin` | 8px profiles, all four kinds |
| 51–62 | free | 96 KB. Nothing loads here. |
| 63 | `phase.bin` | Title, credits, and the match loader. 853 bytes. |

The names are `BANK_ESCORT` through `BANK_P8` in `src/platform.c`. A 64-pixel profile frame is `bank = BANK_PROF + kind * 4 + (frame >> 2)`, then 2,048 bytes at offset `(frame & 3) * 2048` inside that bank. A 32-pixel frame is one bank per kind, 512 bytes at `frame * 512`. The 16-pixel and 8-pixel files pack all four kinds, 16 frames each. The visible frame is still copied into that ship's 2 KB slot, so two ships can be two sizes at once.

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

Gaps that are real and unused: about 128 bytes after the scope art, 256 bytes after the marks, 384 bytes before the hull slots, 992 bytes after the bow tiles (`$17C20`–`$17FFF`), and about 512 bytes before the text map. None of those is large enough for another 64×64 sheet. A new sheet belongs in banks 45–62, with one frame copied into a hull slot, or it replaces something already in VRAM. Bank 63 is the phase code.

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

- Banked RAM in banks 45–62.
- The PRG file cap (36,032 of 36,864).
- The sim's ship array.

What is short is the 184 bytes between the last global and the stack. The draw code and the sprite cache both live in the 64 KB the CPU can see without a bank switch. The view art does not: it is in the banks before the first frame.
