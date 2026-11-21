# Open notes

## Code review 2026-11-18 (needs Robert's call)

- Checked: the names-only change (`0ed3715`, #2) builds a byte-identical `CONVOY.PRG` (35,791 B) to `eba3b07`. Host tests, smoke (`CONVOY SMOKE OK`) and `--project-check` (0 findings) pass. `show_card` still calls `layer_scroll(0, 0, 0)` after `layer_config_bitmap`. `AGENTS.md` is already on main (#3). No `RESMAP_SPRITES` table is needed; the list is optional.
- `feat/balance` (local and `origin`) is fully merged into main. Deleting it waits for Robert's go.
- Untracked `assets/sourced/` (model zips, mp3s, `torpedo-fire.wmv`) and `assets/tilemaps/sea.x16map.json` are still uncommitted. Robert decides whether they belong in the repo or in `.gitignore`.
- Each build still rewrites `.x16/exports.json` in pixel-cli key order. The fix belongs to Pixel and Project-X16; until then it is restored after builds.
## Done

- Line endings are on the X16 standard (Robert's go, 2026-11-14 t665). `.gitattributes` is the lib's: `* text=auto eol=lf`, CRLF only for `*.cmd`, `*.bat` and `*.sln`, plus the binary lines. Everything was already stored as LF, so the renormalize changed no files; the working copy was checked out again so files on disk are LF too. `tools/write-balance.ps1` now writes `src/balance.h` with LF and UTF-8 without a BOM. `tools/pack_bows.py` writes only binary files.
- Profile step-down is `pixel-cli lod` into `game.x16spr.json` (`p32-*`, `p16-*`, `p8-*`). The build only exports those sheets. `tools/pack_profiles.py` is gone. A check against the old bins matched byte for byte, including `PROFESC.BIN`. Run `lod` again only when the bridge art changes (the command is in `build-win.cmd`).
- Title load line, options, and quit are in the game. Music is stored in `boot/CONVOY.CFG` and nothing plays it yet. Robert still has the section 7 call, per the manager's reply.

## Pack step that stays

`tools/pack_bows.py` runs after `pixel-cli export-picture` for `bowesc` and `bowubt`. It only reshapes those exports: a blank tile, a sky tile, and a horizon tile, then the shared 128×64 map. Pixel writes 15,360 bytes; the file on disk is 15,456. `BOWMAP.BIN` (16,384 bytes) is not a Pixel export, so it is not in `.x16/exports.json`. The sky, the horizon, and the row-21 fill are this game's, so they stay out of `pixel-cli`. Documented on the two picture resources and in `build-win.cmd`.

## Generator scripts

`tools/author_assets.py`, `tools/author_bows.py`, and `tools/gen_hulls.py` drew pixels into docs or `boot/` and the build did not call them. The art they once wrote is now in the Pixel documents. Running them again would overwrite that art, so they are removed. `tools/relay.py` stays; it is the two-machine relay, not an art generator.

## 2026-11-21: changes passed over while on hold (Project X16 Manager, Robert t768)

Robert put this project on hold on 2026-11-20 (t718) while the refinement group (Snake, Star Trek, Aethelgard, Rage Rig, Enshit) took the lib and rule changes below. Pick them up when it comes off hold, each on Robert's go. Status: open.

- **Shared build scripts** (x16-lib #214, #216; lib `docs/x16-lib/11-build-tools.md`): `build-win.cmd` through `tools/x16tools.cmd`, `memmap.py --check` for stack and free RAM, the lib's `pal_h.py` / `sums_h.py` in place of local copies, and `resmap -Svg -Grid` bank grids (#218, #220) in place of local chart scripts.
- **Assembly twins, tier 1** (#221-#225, #227; lib `docs/x16-asm-and-vhook.md`): `vera_fill_asm`, `sprite_hide_asm`, `trig8_asm`, `frame_now`, `u16_fmt_asm`. `X16LIB_SEG_<UNIT>=nn` puts a twin's code in a bank. Adopt only the ones this game calls.
- **C/assembly set switch** (#229, plan section 2): call `x16mods.cmd` (or include `x16mods.mk`) once the module list is final, so `X16LIB_IMPL=c|asm` and `X16LIB_C_UNITS` work.
- **Standard game options** (#233; PROJECT-START section 7 item 3, `03-modules.md` 3.36): `x16lib/gameopts.h`. Music and SFX on/off with 1/4, 1/2, 3/4 or full volume over a hidden per-app floor, fog of war and minimap on/off and place where the game has them, and an in-play options window that pauses play with volume bars that fill in. The lib has the mechanics; the look is the game's.
- **PCM options** (#209, #212): the hook-safe PCM tick is opt-in (`X16LIB_PCM_HOOK`) and there is an opt-in 3-voice mixer (`X16LIB_PCM_MIX`). Only if the game uses `pcm_feed`.
- **Dropped-in assets and docs** (PROJECT-START section 9, Robert t766): source assets and docs that Robert or another agent drop into the repo are committed and pushed with the next change that goes out.
  Untracked now and covered by it: `assets/sourced/`, `assets/tilemaps/` and `docs/comms/2026-11-20-project-x16-manager-optimization.md`.
- 2026-11-21 (Project X16 Manager, lib #245): x16-lib now has `tilemap_stream_arr` (array source, asm twin `tilemap\tilemap_stream_arr_asm`). `src/platform.c` streams the sea map through the `sea_tile` callback; if that map can come from a byte array, switching saves a call per cell. Also new asm twins to consider at the next touch: `collide_asm` (aabb_overlap) and `vram\vram_copy_asm`. Not adopted while on hold.
