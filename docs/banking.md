# Memory banking in Convoy

This is the current map. It describes what the program does today. The design notes in [game-plan.md](game-plan.md) are older in places; if they disagree with this file or with `build/CONVOY.map`, this file and the map win.

Related: [mechanics.md](mechanics.md) (what the sim and the views actually do).

Numbers below are from the current `build/CONVOY.map` and `build/CONVOY.PRG` (35,587 bytes). Rebuild and read the map again before treating a byte count as still true.

## Why the build says it is out of memory

The failure is not the PRG file, and it is not banked RAM.

The 65C02 can see 64 KB. This program is one resident image:

| Segment | Address | Size |
|---|---|---|
| CODE | `$082E`–`$8CE4` | 33,975 bytes |
| RODATA | `$8CE5`–`$9293` | 1,455 bytes |
| DATA | `$9294`–`$92D8` | 69 bytes |
| BSS | `$92DB`–`$9A61` | 1,927 bytes |
| C stack | `$9D00`–`$9EFF` | 512 bytes (`__STACKSIZE__=0x0200`) |
| Free before the stack | `$9A62`–`$9CFF` | **670 bytes** |
| PHASECODE | `$A000`–`$A51A` | 1,307 bytes, bank 63, file `PHASE.BIN` |

BSS is the globals. It is not stored in the PRG; the loader clears it. The linker still places it immediately after the code. Every new function, string, or library call slides BSS up. When BSS would pass `$9CFF`, the link fails with `Segment 'BSS' overflows memory area 'BSS'`. That is the message we keep hitting.

The PRG cap in `x16project.json` is 36,864 bytes. The file is about 1,500 bytes under that cap. Raising the cap would not help. The collision is in the running machine, between the globals and the stack.

Banked RAM is a different 8 KB window at `$A000`–`$BFFF`. The CPU sees one bank at a time by writing `RAM_BANK`. A 512 KB machine has banks 0–63. Bank 0 is the KERNAL. Banks 1–50 hold art once a side has been chosen. Bank 63 holds `phase.bin` (the title card, the credits card, and the match loader). Banks 51–62 are empty. The draw path still lives in main RAM, because it runs every frame.

VERA has its own 128 KB and does not bank. It is tight in a few places and loose in others. It is not what the linker is complaining about.

The stack stays at 512 bytes. It is the call stack, not spare room for features. Making it smaller to "free" BSS would make deep calls unsafe. Making it larger would move `$9D00` down and make the collision happen sooner.

## Phases

Disk is for a phase change. A view change is not a phase change.

| Phase | What is on the disk | What is in memory |
|---|---|---|
| Title | `title.bin`, a 2bpp 640×480 card (`assets/pictures/title.x16pic.json`) | Layer 0 is that card. The bottom line says `LOADING` while the card or a match is read. No hull sheets, no sea tiles. `O` is options (`CONVOY.CFG`). `C` or `Q` opens credits. |
| Credits | `credits.bin`, the same size and slot | Same layer. Any key returns to the title. |
| Match | The side you picked, loaded once before the first frame | Chart, bridge, glasses, zoom steps, and (U-boat) the control room. Already in banks or VRAM. `B`, `G`, `C`, and `Z` do not open a file. |

`1` and `2` are the moment the disk runs. The loader is `phase_run` in bank 63. It fills the hull banks, the profile banks and their smaller copies, the chart's 32-pixel and 16-pixel sheets, and the VRAM the views share (sea, scopes, weapons, zoom sprites, and that side's bow). A U-boat match also loads the periscope crosshair and the control-room gauges. An escort match does not. The other bow stays on disk.

The title card occupies `$00000`–`$12BFF`, which is the same VRAM the sea and the scopes use. Leaving the title and starting a match reloads that VRAM. The sheets in banks 1–44 are not in that range, so a second match does not read them from disk again.

There is no wake sprite. The chart, the bridge, and the glasses do not drop foam.

## What actually uses the 1,926 bytes of globals

| Object | BSS |
|---|---|
| `platform.c` | 1,440 bytes |
| `game.c` | 359 bytes |
| `link.c` | 125 bytes |
| cc65 cursor and error | 2 bytes |

`platform.c` is large because it keeps a shadow of all 96 sprite slots so a frame can skip a sprite that did not move: an index, an x, a y, and two attribute bytes per slot. That table alone is 768 bytes. The hub text cache, the blit cache, and the bow-layer registers sit in the rest.

`game.c` is the sim: 11 ships, torpedoes, depth charges, and the solo-AI state. That part is small. The shortage is not the ship list.

`link.c` plus the x16-lib netcard and UART objects are in the main image even during a solo game. They are not the largest piece, but they are resident.

## Rules that follow from this

1. A new feature that adds code or a global in main RAM spends the 670 bytes. Prefer cutting or moving something in the same change. The title card, the credits card, the match loader, and the title menu text already live in bank 63.
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
| 17–20 | free | Was the overhead wake sheet. Nothing loads here. |
| 21–24 | `mid.bin` | 32-pixel hulls, loaded with the match |
| 25 | free | |
| 26 | `tiny.bin` | 16-pixel hulls, loaded with the match |
| 27 | free | |
| 28 | `charge.bin` | One depth-charge body and one splash, 256 bytes |
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
| 63 | `phase.bin` | Title, credits, and the match loader. 852 bytes. |

The names are `BANK_ESCORT` through `BANK_P8` in `src/platform.c`. A 64-pixel profile frame is `bank = BANK_PROF + kind * 4 + (frame >> 2)`, then 2,048 bytes at offset `(frame & 3) * 2048` inside that bank. A 32-pixel frame is one bank per kind, 512 bytes at `frame * 512`. The 16-pixel and 8-pixel files pack all four kinds, 16 frames each. The visible frame is still copied into that ship's 2 KB slot, so two ships can be two sizes at once.

Zoom art, scope faces, weapons, gauges, the bow, and the sea tiles do not use these banks. They load straight into VRAM.

## VRAM

128 KB, addresses `$00000`–`$1FFFF`. Layer 0 is the sea. Layer 1 is the KERNAL text layer, except on the bridge, where it is the bow picture. Sprites that must hide behind the mask or the bow use Z between the two layers. Sprites in front of layer 1 (the scopes, the gauges, the crosshair) draw over text.

| Address | Size | What |
|---|---|---|
| `$00000` | 768 | Sea tiles, six 16×16 images (deep, mid, chop, foam, sky, horizon). The sky and horizon tiles are not on layer 0 anymore. |
| `$00400` | 8,192 | Layer 0 map, 64×64. The program fills it. Ahead views are all sea. The chart is all sea. |
| `$02400` | 4,160 | Scope beam, face, and two 8-pixel blips |
| `$03800` | 2,048 | Periscope crosshair |
| `$04000` | 256 | Plot marks |
| `$04200` | 2,560 | Zoomed hulls |
| `$04C00` | 640 | Escort numbers |
| `$05000` | 45,056 | 22 slots of 2 KB. The visible hull or profile frame is copied here. Ends at `$10000`. |
| `$10000` | 16,384 | Bow picture map, 128×64 cells of 8×8. Sky and the horizon line are in this map. |
| `$14000` | 15,456 | Bow tiles for the side you are playing, plus a blank tile, a sky tile, and a horizon tile. The other bow stays on disk until the match starts. Ends at `$17C20`. |
| `$18000` | 4,608 | Torpedo, its streak, and the shell |
| `$19200` | — | Depth-charge blit target. The frames themselves are in bank 28. |
| `$19400` | 4,224 | Control-room needles and 8-pixel lamps. The faces are the scope sprite. |
| `$1B000` | KERNAL | Text map. Do not load art here. |
| `$1F000` | KERNAL | Charset, then palette, PSG, and sprite attributes through `$1FFFF` |

Gaps that are real and unused: about 128 bytes after the scope art, 256 bytes after the marks, 384 bytes before the hull slots, 992 bytes after the bow tiles (`$17C20`–`$17FFF`), and about 512 bytes before the text map. None of those is large enough for another 64×64 sheet. A new sheet belongs in banks 51–62, with one frame copied into a hull slot, or it replaces something already in VRAM. Bank 63 is the phase code.

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
| 32–39 | Unused |
| 40 | Crosshair |
| 41–52 | Control room. Three scope faces, four sprites each. |
| 53–55 | Control-room needles |
| 56–59 | Tube lamps |
| 59 | Shell |
| 60–67 | Battery lamps in the control room. On the chart, 60–63 are torpedo streaks and 64–67 are escort numbers. |
| 68–71 | Radar blips, four at most |
| 72–95 | Sonar, hydrophone, and the three scope faces. Chart only. |

## What is not short

- Banked RAM in banks 51–62, and the unused wake banks 17–20, 25, and 27.
- The PRG file cap (35,587 of 36,864).
- The sim's ship array.

What is short is the 670 bytes between the last global and the stack. The draw code and the sprite cache both live in the 64 KB the CPU can see without a bank switch. The view art does not: it is in the banks before the first frame.

<!-- resmap:begin -->

_Generated by x16-lib `tools/resmap` from the app's resmap header; edit the header, not this section._

### Phases

| Phase | What |
|---|---|
| `P_TITLE` | title |
| `P_CREDITS` | credits |
| `P_PLAY` | play |

### VRAM

| Start | End | Size | Id | Kind | Group | Phases | Share | File | What is in it |
|---|---|---|---|---|---|---|---|---|---|
| $00000 | $12BFF | $12C00 | `V_CARD` | bitmap | per_view | P_TITLE, P_CREDITS |  | TITLE.BIN,CREDITS.BIN | title or credits, 640x480 2bpp |
| $00000 | $002FF | $0300 | `V_TILES` | tiles | per_view | P_PLAY |  | TILES.BIN | six 16x16 4bpp sea tiles |
| $00400 | $023FF | $2000 | `V_SEAMAP` | map | per_view | P_PLAY |  |  | 64x64 sea map, streamed |
| $02400 | $0343F | $1040 | `V_SCOPE` | sprite | per_view | P_PLAY |  | SCOPE.BIN | radar beam, scope face, two 8px blips |
| $03800 | $03FFF | $0800 | `V_CROSS` | sprite | per_view | P_PLAY |  | CROSS.BIN | 64px periscope crosshair |
| $04000 | $040FF | $0100 | `V_MARKS` | sprite | per_view | P_PLAY |  | MARKS.BIN | 16px eel mark and burst |
| $04200 | $049FF | $0800 | `V_ZOOM` | sprite | per_view | P_PLAY |  | ZOOM.BIN | 8px chart hulls, four kinds |
| $04C00 | $04E7F | $0280 | `V_NUM` | sprite | per_view | P_PLAY |  | NUMBERS.BIN | 16px and 8px digits |
| $05000 | $0FFFF | $B000 | `V_HULL` | sprite | per_view | P_PLAY |  |  | 22 live hull slots, 2 KB each, copied from banks |
| $10000 | $13FFF | $4000 | `V_BOWMAP` | map | per_view | P_PLAY |  | BOWMAP.BIN | 128x64 bow tile map |
| $14000 | $17C5F | $3C60 | `V_BOW` | tiles | per_view | P_PLAY | 1 | BOWESC.BIN,BOWUBT.BIN | bow atlas; one side at a time |
| $18000 | $191FF | $1200 | `V_WEAP` | sprite | per_view | P_PLAY |  | WEAPONS.BIN | eel, torpedo wake, splash column |
| $19200 | $193FF | $0200 | `V_DC` | sprite | per_view | P_PLAY |  |  | four depth-charge slots, copied from the charge bank |
| $19400 | $1A47F | $1080 | `V_GAUGE` | sprite | per_view | P_PLAY |  | GAUGES.BIN | needles and 8px lamps |
| $1B000 | $1EFFF | $4000 | `V_TEXT` | map | always | all |  |  | KERNAL 80x60 text |
| $1F000 | $1F7FF | $0800 | `V_FONT` | tiles | always | all |  |  | KERNAL charset |
| $1F9C0 | $1FFFF | $0640 | (lib) | reserved | | all | | | VERA PSG, palette, sprite attributes |

### VRAM slots

| Address | Size | Id | In | What is in it |
|---|---|---|---|---|
| $19400 | $1000 | `S_NEEDLE` | `V_GAUGE` +$0000 | eight needle aspects |
| $1A400 | $0080 | `S_LAMP` | `V_GAUGE` +$1000 | on, off, tube full, tube empty |
| $04C00 | $0200 | `S_NUM16` | `V_NUM` +$0000 | four 16px digits |
| $04E00 | $0080 | `S_NUM8` | `V_NUM` +$0200 | four 8px digits |
| $02400 | $0800 | `S_BEAM` | `V_SCOPE` +$0000 | 16 radar beams |
| $02C00 | $0800 | `S_FACE` | `V_SCOPE` +$0800 | scope face |
| $03400 | $0020 | `S_BLIP` | `V_SCOPE` +$1000 | 8px friendly blip |
| $03420 | $0020 | `S_HOSTILE` | `V_SCOPE` +$1020 | 8px hostile blip |
| $18000 | $0800 | `S_EEL` | `V_WEAP` +$0000 | eel, 16 aspects |
| $18800 | $0800 | `S_TWAKE` | `V_WEAP` +$0800 | torpedo wake, 16 aspects |
| $19000 | $0200 | `S_COLUMN` | `V_WEAP` +$1000 | splash column |

### RAM banks

| Banks | Count | Id | Group | Phases | Share | File | What is in it |
|---|---|---|---|---|---|---|---|
| 0 | 1 | (KERNAL) | reserved | all | | | KERNAL and system |
| 1-4 | 4 | `B_ESCORT` | data | all |  | ESCORT.BIN | 64px chart escorts |
| 5-8 | 4 | `B_MERCH` | data | all |  | MERCHANT.BIN | 64px chart merchants |
| 9-12 | 4 | `B_TANKER` | data | all |  | TANKER.BIN | 64px chart tankers |
| 13-16 | 4 | `B_UBOAT` | data | all |  | UBOAT.BIN | 64px chart U-boats |
| 21-24 | 4 | `B_MID` | data | all |  | MID.BIN | 32px chart hulls |
| 26 | 1 | `B_TINY` | data | all |  | TINY.BIN | 16px chart hulls |
| 28 | 1 | `B_CHARGE` | data | all |  | CHARGE.BIN | depth-charge body and splash |
| 29-32 | 4 | `B_PROFESC` | data | all |  | PROFESC.BIN | 64px escort profiles |
| 33-36 | 4 | `B_PROFMER` | data | all |  | PROFMER.BIN | 64px merchant profiles |
| 37-40 | 4 | `B_PROFTNK` | data | all |  | PROFTNK.BIN | 64px tanker profiles |
| 41-44 | 4 | `B_PROFUBT` | data | all |  | PROFUBT.BIN | 64px U-boat profiles |
| 45 | 1 | `B_P32ESC` | data | all |  | P32ESC.BIN | 32px escort profiles |
| 46 | 1 | `B_P32MER` | data | all |  | P32MER.BIN | 32px merchant profiles |
| 47 | 1 | `B_P32TNK` | data | all |  | P32TNK.BIN | 32px tanker profiles |
| 48 | 1 | `B_P32UBT` | data | all |  | P32UBT.BIN | 32px U-boat profiles |
| 49 | 1 | `B_P16` | data | all |  | P16PROF.BIN | 16px profiles, all kinds |
| 50 | 1 | `B_P8` | data | all |  | P8PROF.BIN | 8px profiles, all kinds |
| 63 | 1 | `B_PHASE` | code | all |  | PHASE.BIN | title, credits, and match load |

### Palette

| Rows | Address | Id | Phases | Share | File | What is in it |
|---|---|---|---|---|---|---|
| 0-1 | $1FA00 | `PAL_MAIN` | all |  | PAL.BIN | two palette rows |

### Free space

| Phase | VRAM free | Largest VRAM block | Free banks | Free VRAM blocks |
|---|---|---|---|---|
| P_TITLE | 34240 bytes | $8400 at $12C00 | 18 (17-20, 25, 27, 51-62) | $12C00+$8400, $1F800+$01C0 |
| P_CREDITS | 34240 bytes | $8400 at $12C00 | 18 (17-20, 25, 27, 51-62) | $12C00+$8400, $1F800+$01C0 |
| P_PLAY | 6688 bytes | $0B80 at $1A480 | 18 (17-20, 25, 27, 51-62) | $00300+$0100, $03440+$03C0, $04100+$0100, $04A00+$0200, $04E80+$0180, $17C60+$03A0, $1A480+$0B80, $1F800+$01C0 |

<!-- resmap:end -->
