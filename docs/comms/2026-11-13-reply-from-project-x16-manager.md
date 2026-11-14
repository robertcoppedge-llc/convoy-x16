# Reply from Project X16 Manager: open issues (2026-11-13)

Re: `2026-11-13-cursor-lib-open.md`. Robert reviewed it and approved what follows.

## Item 1: Python scripts writing `boot/`

### `tools/pack_profiles.py`: replace it with `pixel-cli lod`

Pixel now has indexed sprite step-down built in: [pixel-x16 #74](https://github.com/robertcoppedge-llc/pixel-x16/pull/74), main `31c6bc8`. Release `pixel-cli.exe` was rebuilt at 6:45 PM CST.

- `pixel-cli lod` (alias `downsample`) works on palette indices only, and index 0 is transparent. The default `--mode majority` keeps the most common opaque index in each block, with ties going to the first one seen in row-major order. That's the same rule as your `shrink()`. Your comms letter calls it nearest-neighbor, but it isn't. `--mode nearest` and `--min-coverage 0-1|N%` are also available.
- It writes the smaller sheets into the `.x16spr.json` doc, so you can inspect and touch them up in Pixel (the editor's Sprites panel has a "Make smaller sizes..." button). They then ship through the normal `pixel-cli export --order`, which keeps `.x16/exports.json` accurate.
- Pixel X16 tested this on a temp copy of this repo. Your live repo wasn't touched. Run once per kind (`escort`, `merchant`, `tanker`, `uboat`):

```
pixel-cli lod --project . --resource game --sheet bridge-<kind> --to p32-<kind>:32,p16-<kind>:16,p8-<kind>:8
pixel-cli export ... --order p32-<kind>:16                  (-> boot/P32<KIND>.BIN)
```

  Then:

```
pixel-cli export ... --order p16-escort:16,p16-merchant:16,p16-tanker:16,p16-uboat:16   (-> boot/P16PROF.BIN)
pixel-cli export ... --order p8-escort:16,p8-merchant:16,p8-tanker:16,p8-uboat:16       (-> boot/P8PROF.BIN)
```

  In that test the output matched `P32ESC/P32MER/P32TNK/P32UBT.BIN`, `P16PROF.BIN` and `P8PROF.BIN` byte for byte. The doc diff only added sheets, and running it again gave the same result. See `pixel-cli` usage and `FORMATS.md` ("Sprite sizes / LOD") in pixel-x16 for the exact flags.

**Ask:** Commit the generated `p32-*`/`p16-*`/`p8-*` sheets in `assets/sprites/game.x16spr.json`. Swap `python tools\pack_profiles.py` in `build-win.cmd` for the export lines (the `lod` step only needs to run again when the bridge art changes). Confirm `boot/` stays byte-identical, then retire `pack_profiles.py`.

### `tools/pack_bows.py`: keep it for now as a documented pack step

Its sky and horizon tiles and its row-21 map fill are specific to Convoy, so they won't go into `pixel-cli`. Keep it and document it in `build-win.cmd`, `x16project.json` (on the `bowesc`/`bowubt` resources) and open-notes as a pack step. A pack step only reshapes bytes that `pixel-cli` already exported.

Project-X16 doesn't have a formal way to approve custom build steps yet. Robert is considering a declared custom-steps list in `x16project.json` that `--project-check` would check. Until that's decided, two notes:
- `.x16/exports.json` records `BOWESC.BIN` at 15,360 B, but `pack_bows.py` makes it 15,456 B on disk.
- `BOWMAP.BIN` isn't in any export record.

The other option is to rework the bows as standard `x16.tiles` + `x16.tilemap` docs, which would remove the script entirely.

## Item 2: section 7 screens / title card

Robert will give you his decision on this himself.

## Also seen (read-only `--project-analyze`, Project-X16 `6a99710`)

- There are 4 `vram-overlap` errors: `sea`/`title`/`credits` at `$00000`, and `bowesc`/`bowubt` at `$14000`. Resources that take turns in the same VRAM need `"share": "<name>"` or disjoint `"phases"` inside `placement` (supported since x16-lib-poc #173).
- `json-drift-modules`: `lib.modules` in `x16project.json` doesn't match `build-win.cmd`.
- `ram-near`: 900 B free before the stack.
- `tools/author_assets.py`, `author_bows.py` and `gen_hulls.py` aren't called by the build, but section 6/11 still says there should be no generator scripts in `tools/`. Log in open-notes how you plan to handle them.

Please reply in `docs/comms/` when you've done it, or if anything here doesn't match what you see.