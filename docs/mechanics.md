# Game mechanics

What the program does today. [game-plan.md](game-plan.md) is the design, including pieces that are not built. If this file and the plan disagree, the code wins: rules in `src/game.c`, pictures and input in `src/platform.c`.

Where those pieces sit in memory is [banking.md](banking.md).

## Phases

The title is a picture, `assets/pictures/title.x16pic.json`. While that picture or a match is loading, the bottom line says `LOADING`. `1` starts an escort match. `2` starts a U-boat match. `3` is the link. `T` cycles the boat type. `O` is options: music and effects on or off, saved in `boot/CONVOY.CFG`. There is no title theme yet, so the music flag is stored and nothing plays. Effects off also stops the escort's engine drone. `C` is the credits picture, `assets/pictures/credits.x16pic.json`. `Q` opens the credits too. Any key on the credits returns to the title. After a win, any key opens the credits.

`1` and `2` read that side's views from disk before the sea is drawn. The title picture stays up, with `LOADING` on it, until that read finishes. The chart, the bridge, the glasses, the zoom steps, and the U-boat's control room are then in memory. Switching among them does not read the disk. An escort match does not load the control-room gauges or the periscope crosshair. A U-boat match does not load the escort's bow. Both matches load every hull you can look at, because the other side is on the screen.

## Frame

`main` calls `plat_frame` once per vertical blank.

1. Read the keyboard.
2. Draw the current view.
3. On every third blank, run one sim tick and clear the helm keys. Ship speed stays at half the first demo. The tick rate is the original pace.
4. Patch the chart hub, or the heading line on the glasses.
5. Draw the three scopes, but only on the chart.

A held helm key is latched and then cleared after the tick. It does not turn the ship on the blanks in between.

There is no time compression. The plan's "4× when the plot is quiet" is not implemented. A tick is a tick.

## The plot

Positions are signed 16-bit, in sixteenths of a pixel. Heading is 0 north, clockwise, 256 steps. The screen is the battle space.

Each tick subtracts the convoy's own northward half-speed from every body, then adds that motion to the sea. A merchant who keeps station does not walk up the screen. An escort at flank ahead does. A U-boat with engines stopped drifts down the plot. Bodies are clamped to a box around the convoy, not deleted when they leave the window.

The telegraph is stop, slow, half, full, flank, dead slow, astern, and full astern. Half is the ordinary speed. The U-boat under depth 8 is capped at slow.

Eleven bodies: four escorts, six merchants (Liberty and tanker), one U-boat. The four boat types (II, VIIC, IXC, XXI) share one hull. The type name is text. Wolfpack orders and a second boat are not in this program. The comment in `game.h` is the reminder, not a hidden mode.

## Conn and orders

The escort who has the conn takes the wheel and the telegraph. The other escorts keep an order: patrol a station, hunt the last contact, or attack it. The order sticks when you leave that ship. The ship you are steering ignores her order until you leave.

`[` `]` cycle the conn. `1`–`4` jump. The selected escort is drawn in the gold palette. Her number is on the hull.

On the chart, the arrow keys are helm and telegraph for the escort. `N` and `M` do not steer there. On the bridge and in the control room, `N` and `M` and the arrows turn the ship, and the look stays on the bow. In the binoculars and the periscope, those same keys only slew the look. They do not change heading.

The U-boat's helm keys are `J` and `L`, and the arrows on her bridge and in the control room. `Y` and `U` raise and dive. The moment her depth leaves the surface, the view changes to the control room. `I` and `K` are her telegraph.

## Views

`O` chart, `B` bridge, `G` glasses, `C` control room (U-boat only). `Z` zooms the chart out through 64, 32, 16, and 8 pixel aspects, then back to 64. Zoom is the chart only.

The chart is the left 480 pixels. The hub and the three scopes occupy the right side. The other views use the full 640, and the scopes are hidden.

The bridge, for the escort and for the surfaced boat, shows a fixed picture of your own bow on layer 1. The escort picture and the boat picture are different files. The picture does not rotate. The sea and the other ships move past it. Under depth 8 the bow picture is taken off.

The glasses are a mask: two circles for the escort, one for the periscope. Sky and a white horizon line are painted on that mask. The sea under the line scrolls. Hulls sit between the layers, so the mask clips them. A line on the mask shows heading and look as one of sixteen points, `0` north, then clockwise. That is not a three-digit degree readout. The periscope also draws a crosshair.

The control room is three copies of the chart's scope face, with a needle on each for heading, depth, and planes. Tube and battery lamps are 8-pixel sprites. It is not a window on the sea.

Sixteen aspects, 22.5° each, because 16 divides 256. Frame 0 is bow-on in the ahead view and bow-north on the chart, then clockwise. The bridge and the glasses use the same sixteen aspects. Range picks 64, 32, 16, or 8 pixels, and the waterline stays a short step under the horizon. A far ship is the small sprite on that line. The glasses use those same sheets one size larger, and the bearings spread further across the glass. The chart's `Z` sizes are a different set, overhead, and they stay. The ahead-view sheets are banks 29–44, with the smaller copies in banks 45–50. The chart sheets are banks 1–28. The frame you can see is copied into that ship's slot. The machine cannot scale a sprite; a 64-pixel image shown as 16 pixels would be a strip, so each size is its own sheet.

## Sea and wind

The waves run east and a little south. That is a constant, not weather.

On the chart the camera follows the conn ship, and the wave pattern also creeps southeast. Steaming with the sea slows the creep. Steaming against it makes the waves meet you faster.

On the bridge and in the glasses the ship's own course and speed are taken off the wind, then laid onto the window. Ahead comes down the glass toward the bow. A stern sea goes up toward the horizon. A beam sea slides across. Turning changes that direction. The horizon line does not scroll with the water. It is on the layer above.

## Detection and weapons

Radar, sonar, and hydrophone are three faces on the chart, each four 64-pixel sprites. The radar sweep turns on its own. It draws at most four contacts, each an 8-pixel blip. Convoy contacts are white. The boat is red. Radar does not show the boat at depth 8 or more. A red diamond on the chart is the last plotted spot after she dives, not an explosion and not a kill.

An escort ping looks up the bow. The first ping waits. After a contact, the hydrophone has a bearing. Visual range is what the bridge and the glasses use. A boat that is close but deep is not drawn there.

Escort `F` is the deck gun, and it only hurts a boat that is still up. Driving an escort onto a surfaced boat rams her. Three hits sink the boat. `D` drops a depth charge. The charge is one sprite. It does not turn with the ship. `S` pings.

U-boat `F` fires the bow tubes, up to four torpedoes. A tanker is two points of tonnage, a Liberty is one. Four points wins. The escort wins by sinking the boat.

Tubes, hits, depth, and tonnage are on the chart hub. The glasses show heading against look instead of that list.

## Solo opponents

Either side can be the player. The other side is a small script, not a second copy of the player's orders.

- Escort AI: hold station, ping up the bow, then the nearest attacks and the next hunts. It does not dive.
- U-boat AI: start ahead and to port, outside sight, stopped so she is quiet. Slide onto a merchant column, fire one torpedo, run. Flank clear if an escort is on top of her.

The link (`3` on the title, then a host) exchanges orders with a second machine. Solo does not need it. A desync is marked on the hub. The packet layout is part of `link.c` and is not something to change in passing.

## Not built

These are in the plan or in comments. They are not mechanics of this program.

- A second U-boat, or wolfpack orders.
- Named depth bands (decks awash, periscope depth, crush) beyond the single depth number and the "8 or deeper" tests.
- Quiet-plot time compression.
- A degree readout of heading and look. The glasses show the sixteen-point index.
- Wakes that grow as they age.
- Different overhead art for the four boat types.

## What costs memory when a mechanic grows

The ship list and the tick are cheap. They live in the 404 bytes of `game.c` globals.

The expensive part of a new view or a new weapon is the draw code and the sprite cache in main RAM, not the picture. A new sheet of art can go in banks 45–63. A new function in `platform.c` spends the 33 bytes under the stack. See [banking.md](banking.md).
