# Convoy — game plan

A 1v1 Atlantic convoy action for the Commander X16. One player commands up to four escorts. The other commands one U-boat. The picture is a single screen. Ships move inside it, and the water scrolls by the convoy's ordered course and speed, so a merchant on station stays put while a boat lying still drifts down the plot as the convoy steams onto her.

Both machines will run the same simulation and exchange orders. This tree's first program is the plot on one machine, with both stations on one keyboard. The link is the next slice. The rules the program follows today are in [mechanics.md](mechanics.md). The memory map, and why new code hits the stack, is in [banking.md](banking.md). Choosing a side loads that side's views into banks before the first frame. Changing view after that does not read the disk.

## Display

Output is 640×480, scale 128 (one VERA pixel per VGA pixel). Art is 4 bits per pixel.

A 640×480 4bpp framebuffer is 150KB. VERA has 128KB, so the picture is two tile layers plus sprites. The master palette is 256 colors. Each tile and sprite picks its own 16-color bank.

| Use | Layout | VRAM in this build |
|---|---|---|
| Sea | Layer 0, 16×16 tiles, 4bpp, map 64×64. One light-blue tile, two sparse wave breaks | Tiles at `$00000` (512 bytes). Map filled by the program at `$00400` (8KB). |
| Map and hub | Layer 1, KERNAL 80×60 text (`$1B000` / charset `$1F000`) | The chart, the glasses, and the control room. The bridge switches this layer to the bow picture instead. |
| Hulls and wakes | 64×64 4bpp, 16 aspects, between the layers | One slot per visible body at `$05000` (2KB each). Sheets live in banked RAM. |
| Eel and burst | 16×16 | `$04000` |

The program is about 35KB, inside a 36KB cap. The map is the left 480×480. The view stays on the ship that has the conn (the selected escort, or the boat). Other hulls move around her. `Z` zooms that chart out through 32, 16, and 8 pixel aspects, then back to 64. Latitude and longitude are grid numbers that travel along the left edge and the top as the conn ship moves. The hub on the right is the chart only. It is drawn once and patched when a value changes, so it does not flicker. The bottom of the hub is the key list. The bridge and the conning tower drop that panel and put a picture on layer 1: the bow of the ship you are standing on (`assets/pictures/bowesc.x16pic.json` and `bowubt.x16pic.json`). It stays put while the sea turns under it.

`V` cycles three pictures. The map is overhead. The bridge is a bulkhead and a bow window: ships in the forward arc are head-on aspects. Binoculars (escort) and the periscope (U-boat) are that same window with a sky and a horizon. `N` and `M` slew the look. The U-boat starts ahead and to port, outside visual and sonar range.

Sixteen aspects, not a 16° step. The helm is 256 steps, and 16 divides it, so each drawing is 22.5°. A true 16° step does not divide 360 or 256. Frame 0 is bow-north, then clockwise. A moving ship draws a wake on the same aspect, set astern. `tools/gen_hulls.py` writes those sheets (`escort.bin`, `merchant.bin`, `tanker.bin`, `uboat.bin`, `wake.bin`). The four U-boat types still share one overhead hull. The type name is in the hub.

The bridge and the glasses share sixteen painted aspects per hull, bow-on then clockwise, the same step as the chart. Range picks a 64, 32, 16, or 8 pixel copy of that aspect. The waterline stays under the horizon, so a far ship is small instead of a full-size sprite slid up the sky. The glasses show the next size up and a narrower slice of bearings. The sheets live in RAM banks 29–50. The aspect in view is copied into that ship's slot.

Index 0 is black and index 1 is white so the text layer survives the palette load.

## Battle space

The window is about eight nautical miles across. Each tick the sim subtracts the convoy's course and speed from every body, then adds that same vector to the sea scroll.

- A merchant on station does not move on the screen.
- An escort at flank ahead walks up the plot.
- A U-boat with engines stopped drifts down the plot.
- A body that leaves the window is clamped just off the edge for now. Later it is off the plot until it steams back.

Depth is a number. The bands, when they exist, are surface, decks awash, periscope, attack depth, deep, and crush. Time runs 1:1 while anyone has a contact, and 4× when the plot is quiet. A passage should land around twenty minutes.

Fixed-point only. No floating point. The shared random seed comes from the lobby, once the link exists. Zigzag tables will be baked so both sims take the same turn.

Positions in this build are signed 16-bit, in sixteenths of a pixel. North is heading 0. The convoy steams north at half ahead (2 pixels a tick). Telegraph speeds are stop, slow, half, full, and flank (0 through 4 pixels a tick).

## Who sees what

Truth lives in the sim. The picture is whatever that player's sensors have earned. This build draws the truth, so both sides can see every hull while the motion is being proven. Fog is the next slice.

| Source | Escort | U-boat |
|---|---|---|
| Own ships | Always | Own boat, plus battery, air, and depth |
| Merchants | Always | Masts in the periscope, or a hydrophone bearing |
| The other side | Surfaced boat by eye; submerged boat as a sonar return or a screw bearing | Masts and smoke inside periscope range; an active ping names the escort that made it |
| Weapons | Torpedo tracks when they porpoise, splashes, depth-charge plumes | Own eels, gun splashes, charges near the hull |

A contact is a bearing, a confidence, and sometimes a range. It becomes a ship icon only after someone has seen it. Hydrophones give a direction and a loudness. Own screws blank a sector aft. Active sonar is a cone ahead with a dead zone under the keel and directly astern, and every ping is audible to the boat. Rough seas and high speed degrade both.

## Views

Only one heavy view is resident at a time.

**Plot.** Overhead water, 64×64 aspects, wakes, and a zoomed 8×8 chart. The conn ship stays centered. Grid numbers slide along the margins.

**Bridge interior.** This build has one conn picture: a dark bulkhead, a bow window, and the ships ahead as head-on aspects. The fuller rooms are still to come.

- Escort conn: wheel, telegraph, compass repeater, revolution counter, damage lamps.
- Sonar shack: bearing ring, range drum, ping button.
- Depth-charge rails: depth setting, pattern (port, starboard, stern), ready lamps.
- U-boat control room: depth gauge, plane angle, battery, air, flooding, periscope-up lamp.

Damage darkens lamps, sticks needles, and cracks the painting. Fire and flood are a palette swap plus a sprite, so the background is not redrawn.

**Exterior, binoculars, periscope.** Wing Commander views. A hull is a stack of drawn aspects, head-on through stern-on. Sixteen angles when the sheet fits, eight for the first cut. Aspect is the target's heading minus the look bearing, quantized. Distance picks a near, mid, or far sheet and a slot between the horizon and the bow wave. Bow wave, funnel smoke, fire, and list are extra sprites.

Binoculars are that view through a dual-circle mask, narrower and longer-ranged. The periscope is a circular mask, a stadimeter, and a sky/sea split. Raised, it leaves a feather an escort can see when they are close and the sea is calm.

Overhead art is 16 aspects per hull, bow-north then clockwise, plus a wake. The compass heading is repeated in the hub.

## U-boat roster

Four hulls. A year/refit flag sits on the first three later (deck gun fitted or landed, snorkel, steam or electric eels, radar detector). The Type XXI is always late-war and has no gun. The numbers below are in the type table now. Movement on the plot still uses the shared telegraph; the knots drive the HUD and the tests, and they become the speed model when diving exists.

| | Type II | Type VIIC | Type IXC | Type XXI |
|---|---|---|---|---|
| Where | Coastal convoy | Atlantic default | Distant convoy | Late-war Atlantic |
| Tubes | 3 bow | 4 bow, 1 stern | 4 bow, 2 stern | 6 bow |
| Eels aboard | a handful, slow hand reload | about 14 | about 22 | a large ready store, hydraulic reload measured in minutes |
| Gun | none that matters | 88 mm until landed | 105 mm until landed | none |
| Surface | about 13 kn, short-legged | about 17 kn | about 18 kn, long range | about 15 kn, and a bad place to fight |
| Submerged | a few knots | about 7 kn briefly, creep near 4 | same, and she is large | sprint near 17 kn for a short burn, creep near 5 kn and quiet |
| Signature | tiny, dives fast | the baseline | loud, wide turning circle, slow dive | large sonar target while moving; very small while creeping |
| Ears | basic | GHG | GHG | Balkon array: tighter bearings, less masked by her own screws |
| Air | snorkel only if the scenario is late | late refit | late refit | snorkel, and she is meant to stay under |

Shared rules, once the dive model exists:

- Battery is one pool. Sprint, periscope depth, and the snorkel's diesels charge or drain it. The XXI can attack without surfacing, then has to creep or snorkel. A sprint makes her easy on active sonar.
- Test depth and crush depth scale with the hull. The II is shallow. The XXI is the deepest.
- Reload is per tube and per hull. A VIIC stern shot is a separate order from the bow salvo. The XXI fires six and can load another salvo inside the same attack.
- Deck-gun orders exist only while that boat still carries one.
- Overhead sheets: three sizes, eight headings, with the IX and the XXI each keeping their own outline. Exterior sheets load per hull. The XXI's sail, and the lack of a gun, have to read at a glance.

The contact should hint at the hull. A small, fast dive reads as a II. A loud boat with a stern shot reads as an IX. A submerged contact that closes the convoy at escort speed is a XXI, and the sonar operator hears the difference between her sprint and her creep.

## Escorts and merchants

One escort hull (a Hunt-type) and two merchant hulls (Liberty and tanker). Six merchants in two columns of three. The starboard after ship is the tanker.

The escort who has the conn takes the wheel and the telegraph. `[` and `]` (or `1`–`4`) move the conn. Each escort also carries an order, set with `P`, `H`, or `A` while she has the conn. When you cycle off her, she carries that order out. The ship you are steering ignores her order until you leave.

`Z` zooms the map out through 64, 32, 16, and 8 pixel sprites, each with the same 16 headings, then returns to 64. `1` to `4` select an escort. The number is drawn on her hull, and the escort under the wheel uses the gold palette. Torpedoes, their wakes, and the depth-charge fall and splash are 16-way as well.

The bridge and the glasses fill the screen. The chart is the only view with the hub. On the bridge, and on the conning tower while the boat is up, layer 1 is a picture of your own bow: the escort's forecastle, or the boat's casing and tower. The pictures live in `assets/pictures` and can be repainted in Pixel. That bow does not turn. The sea and the other ships move past it, so a helm order shows up as the world swinging around the stem. Underwater the bow is gone. The glass mask still clips any hull that crosses the rim, and a line on the mask shows the ship's heading and the look as one of sixteen points (0 is north, then clockwise). The sea under the horizon keeps the wind's direction. A westerly runs the waves east and a little south, and your own way is taken off that, so the glass shows the water sliding ahead, astern, or across as you turn and as you change speed. A foam ring is dropped in the water as a ship moves, and the older rings stay put when she turns. In the binoculars and the periscope, `N` and `M` (and the helm keys) only slew the look. A torpedo shows its streak in that window, turned to the look. The bridge and the glasses share sixteen painted aspects, held in banked RAM, with a smaller copy for each range. The overhead map is sixteen as well. The bridge, for the escort and for the surfaced boat, is look-and-steer: `N` and `M` turn the ship and the view stays on the bow. Binoculars and the periscope only slew the look. Underwater, the control room is the boat's bridge: the arrow keys turn her, and `C` opens that room only for the U-boat. Telegraph steps are full astern, astern, stop, dead slow, slow, half, full, and flank. The helm ticks every three blanks. Ships steam at half the first demo's speed, and the columns are spaced so a 64 pixel hull does not sit on the next ship. A watch officer calls a surfaced boat inside 120 pixels that the lookouts have not yet marked. The hub draws three scopes, each a 128 pixel face made from four 64 pixel quadrants: radar with an automatic sweep (convoy contacts white, the boat red), sonar as a forward sector, and a hydrophone ray. `F` on an escort fires the deck gun at a boat that is still up, and driving onto her rams. A red diamond is the last plotted position after she dives. The key list is only the side you are playing.

- **Patrol.** Her screen station: ahead, port beam, starboard beam, or astern. On station she steams with the convoy.
- **Hunt.** Full ahead toward the last sonar or visual contact. With no contact she searches ahead of the convoy, port then starboard.
- **Attack.** Flank toward that contact, and she drops a depth charge when she reaches it.

## Wolfpack, later

The U-boat player is one boat in this demo. A later scenario lets that player command a pack the same way the escort assigns ships: one boat on the conn, the others on attack or shadow stations. Not in this build.

## Weapons

Not in this build.

**Torpedoes.** Gyro angle, running depth, spread of one to four. Steam eels leave a visible track. Electric eels do not, and they are shorter-ranged. They run until they hit, finish their run, or leave the plot. A circle-runner is a rare seeded failure.

**Deck guns.** Escort main gun, and the U-boat's gun only while surfaced and still fitted. A shot is a bearing plus an elevation. The shell lands after a flight time. List and sea state throw it off.

**Depth charges.** Pattern and fuse depth. The blast is a radius at a depth. A near miss rattles the boat. A close one floods a compartment. An empty pattern still makes a plume, which both players can see.

**Hits.** Compartments: bow, engine, steering, gun, sonar or hydrophone, wireless, and the charge racks. The boat: pressure hull, planes, periscope, batteries, motors. A dead engine changes the hydrophone signature. Flooding is a timer toward a sinking sprite: list, settle, bow-up or stern-up, then debris. Fire is a flame sprite and a darker hull, visible for miles at night.

## Sound

VERA PSG and the YM2151 are mono, so bearing is drawn. Loudness is volume. An off-axis hydrophone contact is a tick whose rate falls as the bearing comes onto the nose.

This build:

- Voice 0 is a triangle drone whose pitch follows the conn's telegraph. Stop is silence.
- The telegraph bell, helm tick, sonar ping, and depth-charge boom are sound-cli effects on voices 2–5 (`assets/sounds/sfx.x16snd.json`).

Still to add: enemy screws (rhythm hints at the class), a sonar return, shell splash, alarm bell, hull creak under depth.

## Link

Lockstep inputs, not a streamed world. At 8 Hz each side sends one short order: helm, telegraph, selected ship, weapon key, look bearing, dive plane. Both sims apply both packets. Every couple of seconds they compare `game_checksum()`. On a miss, one side sends positions, depths, and compartment flags.

The emulator already has the port. `-netcard` puts a Zimodem UART at `$9FE0`, and `ATD host:port` opens TCP. A small relay accepts two calls and pairs them. The lobby picks a side, a seed, weather, and daylight. The relay forwards inputs only, so a modified picture cannot invent a hit the other sim did not compute.

Packets stay under 32 bytes in play. A snapshot is a few hundred bytes and travels only after a desync.

`tools/relay.py` is that relay. It listens on port 6416 and pipes the two sides together. The game dials with `ATD` on the netcard. Orders are ASCII hex so a payload byte is never `+`.

## What this build does

`src/game.c` is the sim, with no VERA and no KERNAL, so `test-win.cmd` can link it on the host.

This demo is a match. The title is escort versus the boat, U-boat versus the escorts, or a link through the relay.

- Four escorts, five Libertys, one tanker, one U-boat.
- The 480×480 map keeps the conn ship in the middle. The sea is light blue with a few wave breaks. Merchants hold in the convoy frame. A stopped boat drifts down the map.
- Conn cycles with `[` `]`. Patrol, hunt, and attack stick to the ship you leave.
- The boat is hidden until she is in visual range or a ping puts a contact on the plot. A moving boat gives a hydrophone bearing.
- Bow torpedoes and depth charges. Three charge hits sink the boat. Four points of tonnage wins for the boat. A tanker is two points.
- Solo is the one-machine match, for play and for a hand smoke of either side. The boat ambushes one column and the escorts screen, ping, and attack.
- Hulls, wakes, and the two small marks are generated by `tools/gen_hulls.py`. The palette and the sea tiles stay pixel docs. Sound doc for the bell, helm tick, ping, and boom.

Proven by the host tests: the XXI is the fast submerged boat and has no gun, merchants do not drift, the sea scroll matches the convoy, a wing escort walks back to a patrol station, an attack order closes on a contact, a torpedo sinks a merchant, a depth charge registers on the boat, an escort packet restores the conn and her order, the solo boat fires one fish, and the solo escorts attack and depth-charge. The emulator smoke checks the same plot and both opponents.

## Build order from here

1. **Guns, bridges, exterior.** Gauges, then the head-on aspects. A distinct overhead hull for the XXI. Fire and list.
2. **Damage that changes the fight.** Lost steering, dead sonar, crushed periscope, flat batteries, crush depth. Wolfpack orders for the U-boat player.
3. **A fuller link.** Checksum snapshot when the two sims diverge, instead of a DESYNC mark.

Victory for the escort is a sunk or fleeing boat, or the clock. Victory for the boat is a tonnage line, tankers worth more than Libertys.

## Kept for later

- Zigzag clocks the escort knows and the boat must guess.
- Night, moon, starshells, and searchlights. Burning ships become beacons.
- Fog and sea state. Fog blinds the periscope. A high sea wrecks sonar and the deck gun.
- Funnel smoke and a smoke float.
- Radar on the escort, and a detector on the boat that only says she is being painted.
- HF/DF as a one-shot radio bearing when the boat transmits.
- A merchant that wanders, and a torpedo that hits the wrong hull.
- A TDC: angle on the bow, a speed you dial in, and a gyro the attack solves.
- Battery versus air. Deep and quiet is safe until the air runs out. Surfacing to charge is the exposed moment.
- Ramming, as a last order for a destroyer that still has a bow.

The art rule for the rest of the game: plot sprites stay small, exterior sheets stream in only for the raised view, and every splash, plume, fire, and debris sprite comes from a pool. The 65C02 moves numbers and banks. It does not draw the ocean.
