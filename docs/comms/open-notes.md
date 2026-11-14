# Open notes

## Done

- Profile step-down is `pixel-cli lod` into `game.x16spr.json` (`p32-*`, `p16-*`, `p8-*`). The build only exports those sheets. `tools/pack_profiles.py` is gone. A check against the old bins matched byte for byte, including `PROFESC.BIN`. Run `lod` again only when the bridge art changes (the command is in `build-win.cmd`).
- Title load line, options, and quit are in the game. Music is stored in `boot/CONVOY.CFG` and nothing plays it yet. Robert still has the section 7 call, per the manager's reply.

## Pack step that stays

`tools/pack_bows.py` runs after `pixel-cli export-picture` for `bowesc` and `bowubt`. It only reshapes those exports: a blank tile, a sky tile, and a horizon tile, then the shared 128×64 map. Pixel writes 15,360 bytes; the file on disk is 15,456. `BOWMAP.BIN` (16,384 bytes) is not a Pixel export, so it is not in `.x16/exports.json`. The sky, the horizon, and the row-21 fill are this game's, so they stay out of `pixel-cli`. Documented on the two picture resources and in `build-win.cmd`.

## Generator scripts

`tools/author_assets.py`, `tools/author_bows.py`, and `tools/gen_hulls.py` drew pixels into docs or `boot/` and the build did not call them. The art they once wrote is now in the Pixel documents. Running them again would overwrite that art, so they are removed. `tools/relay.py` stays; it is the two-machine relay, not an art generator.
