# convoy-x16

A one-screen Atlantic convoy fight for the Commander X16. One player cons up to four escorts. The other cons a U-boat (Type II, VIIC, IXC, or XXI). The screen is the battle space: ships move, and the sea scrolls by the convoy's own speed.

The design lives in [docs/game-plan.md](docs/game-plan.md). This tree is the first slice: the plot, helm, telegraph, station-keeping, and both boats on one keyboard.

## Build and run (Windows)

```text
cd D:\Vintage\src\convoy-x16
build-win.cmd          -> build\CONVOY.PRG (copied to boot\)
run-win.cmd            -> x16emu r49, host filesystem boot\
test-win.cmd           -> host unit tests (gcc)
smoke-win.cmd          -> scripted plot, prints "convoy smoke ok"
```

`make` wraps the same scripts. cc65, x16emu r49, pixel-cli, and sound-cli resolve the same way as the other X16 apps (`..\..\tools`, `..\pixel-x16`, `..\sound-x16`).

## Play

The title is how you smoke a side by hand. `1` is the escort against the boat. `2` is the U-boat against the escorts. `3` links a second machine. The map is the left 480×480 and stays on the ship that has the conn. `O` is the map. `Z` zooms out through 64, 32, 16, and 8 pixel sprites, then back to 64. `1` through `4` select an escort; her number sits on the hull, and the one you are steering is gold. `B` is the bridge, `G` is the binoculars or the periscope, and `C` is the U-boat control room. `N` and `M` steer on the bridge and in the control room, and only slew the look in the glasses. `Y` and `U` raise and dive the boat. The helm ticks every three blanks, which is the original pace, and the ships steam at half the first demo's speed. The columns sit far enough apart that the hulls do not cover each other. The hub has three scopes, each a 128 pixel face built from four 64 pixel sprites: a radar with its own rotating sweep, a sonar sector, and a hydrophone bearing. The grid numbers travel along the top and left. The hub on the right holds status at the top and the key list at the bottom, and it marks a one-machine game `SOLO`. `T` on the title cycles Type II, VIIC, IXC, and XXI. The boat starts ahead and to port, outside sight.

On `1` the boat slides onto a merchant column, fires one torpedo, then runs, and she flanks clear if an escort is on top of her. On `2` the escorts hold their stations and the leader pings up the bow. After a contact, the nearest attacks and the next hunts. The first ping waits, so the boat has time to leave that arc.

Escort: `[` `]` cycle the conn, `1`–`4` jump to a ship. `P` patrol, `H` hunt, `A` attack. The order sticks when you leave that ship. Arrows are helm and telegraph. `S` pings ahead. `D` drops a depth charge. `F` fires the deck gun, which can hit a boat that is still up. Driving an escort onto a surfaced boat rams her. A red diamond is the last plot mark after she dives, not an explosion. Three hits sink the boat. The key list on the right is only the side you are playing. Radar blips are white for the convoy and red for the boat.

U-boat: `J` `L` helm, `I` `K` telegraph, `F` fires the bow tubes. You see ships inside periscope range. Four points of tonnage wins. A tanker is two, a Liberty is one.

The boat starts ahead of the convoy, stopped, so she is quiet on the hydrophone until she moves. The lead escort does not see her until someone pings or she drifts into visual range.

## Two machines

`run-win.cmd` starts the emulator with `-netcard`. On one PC:

```text
python tools\relay.py
```

On both machines, choose `3`, `E` on one and `U` on the other, type that PC's LAN address, and press Enter. The relay listens on port 6416. The match uses the boat the escort picked on the title.

## Tools

Project-X16 reads `x16project.json`. Pixel owns `assets/palettes/main.x16pal.json`, `assets/tiles/sea.x16tile.json`, and `assets/sprites/plot.x16spr.json`. Sound owns `assets/sounds/sfx.x16snd.json`. The build exports them; it does not draw its own art.

Display is 640×480, 4bpp tiles and sprites. A full-screen 4bpp framebuffer is 150KB and does not fit in VERA's 128KB, so the sea is a tile layer and the ships are sprites. Index 0 is black and index 1 is white so the KERNAL text on layer 1 stays readable. The bridge and periscope use sixteen painted aspects. Those sheets sit in RAM banks 29–44, and the aspect in view is copied into a sprite slot. The chart uses sixteen as well.

