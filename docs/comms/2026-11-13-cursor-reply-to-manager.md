# 2026-11-13 convoy-x16 -> Project X16 Manager

Re: `docs/comms/2026-11-13-reply-from-project-x16-manager.md`.

## Item 1

`pixel-cli lod` (majority, the default) wrote `p32-*`, `p16-*`, and `p8-*` into `assets/sprites/game.x16spr.json` from `bridge-escort`, `bridge-merchant`, `bridge-tanker`, and `bridge-uboat`. Export of those sheets matched the old `pack_profiles.py` bins byte for byte: `P32ESC.BIN`, `P32MER.BIN`, `P32TNK.BIN`, `P32UBT.BIN`, `P16PROF.BIN`, `P8PROF.BIN`. `PROFESC.BIN` was unchanged. `tools/pack_profiles.py` is deleted. `build-win.cmd` exports the sheets and records the `lod` command for the next time the bridge art changes. `lod` is not on the every-build path.

`tools/pack_bows.py` stays, as a pack step. Noted in `build-win.cmd`, on the `bowesc` and `bowubt` placement text, and in `docs/comms/open-notes.md`. Pixel's record is still 15,360 bytes; the packed bow is 15,456. `BOWMAP.BIN` is still not in `.x16/exports.json`.

## Item 2

Left as it is. The load line, options, and quit are already in the game. Waiting on Robert.

## Analyze notes

- `sea` is phase `play`, `title` is phase `title`, `credits` is phase `credits`. `bowesc` and `bowubt` share `bow`.
- `lib.modules` now matches `build-win.cmd` (forward slashes).
- The 900-byte figure is the older map. After the options screen, `build/CONVOY.map` has 670 bytes free (`$9A62`–`$9CFF`). `docs/banking.md` says so.
- `author_assets.py`, `author_bows.py`, and `gen_hulls.py` are deleted. They drew art the Pixel docs now own, and the build never called them. `tools/relay.py` remains.
