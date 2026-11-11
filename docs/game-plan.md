# Convoy — game plan

A 1v1 Atlantic convoy action for the Commander X16. One player commands up to four escorts. The other commands one U-boat. The picture is a single screen. Ships move inside it, and the water scrolls by the convoy's ordered course and speed, so a merchant on station stays put while a boat lying still drifts down the plot as the convoy steams onto her.

Both machines will run the same simulation and exchange orders. This tree's first program is the plot on one machine, with both stations on one keyboard. The link is the next slice.

## Display

Output is 640×480, scale 128 (one VERA pixel per VGA pixel). Art is 4 bits per pixel.

A 640×480 4bpp framebuffer is 150KB. VERA has 128KB, so the picture is two tile layers plus sprites. The master palette is 256 colors. Each tile and sprite picks its own 16-color bank.

| Use | Layout | VRAM in this build |
|---|---|---|
| Sea | Layer 0, 16×16 tiles, 4bpp, map 64×64 | Tiles at `$00000` (512 bytes, four tiles). Map filled by the program at `$00400` (8KB). |
| Instruments and text | Layer 1, KERNAL 80×60 text, left where the ROM put it (`$1B000` / charset `$1F000`) | Not overwritten |
| Plot ships | 16×16 4bpp sprites between the two layers | `$04000`, 1024 bytes, eight images |

Later exterior views use 64×64 sprites, several of them stacked for one hull, and those sheets load only when the glasses or the periscope are raised. The bridge is a 16×16 tile painting with sprite needles. A station change is a tileset swap.

The plot column can stay up while instrument tiles occupy the side, about 160 pixels, once the bridge art exists. This first build uses two text rows instead.

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

**Plot.** Overhead water, ship sprites, and later wakes, tracks, splashes, and charge circles. A mark on the escort that has the conn. Bearing tape and contact list in the margins.

**Bridge interior.** One room per role. Sprite needles and lamps over a tile painting.

- Escort conn: wheel, telegraph, compass repeater, revolution counter, damage lamps.
- Sonar shack: bearing ring, range drum, ping button.
- Depth-charge rails: depth setting, pattern (port, starboard, stern), ready lamps.
- U-boat control room: depth gauge, plane angle, battery, air, flooding, periscope-up lamp.

Damage darkens lamps, sticks needles, and cracks the painting. Fire and flood are a palette swap plus a sprite, so the background is not redrawn.

**Exterior, binoculars, periscope.** Wing Commander views. A hull is a stack of drawn aspects, head-on through stern-on. Sixteen angles when the sheet fits, eight for the first cut. Aspect is the target's heading minus the look bearing, quantized. Distance picks a near, mid, or far sheet and a slot between the horizon and the bow wave. Bow wave, funnel smoke, fire, and list are extra sprites.

Binoculars are that view through a dual-circle mask, narrower and longer-ranged. The periscope is a circular mask, a stadimeter, and a sky/sea split. Raised, it leaves a feather an escort can see when they are close and the sea is calm.

Overhead art in this build is one bow-up image per hull. Eight headings come with the next art pass. Until then the HUD prints the compass heading.

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

One escort hull (a Hunt-type) and two merchant hulls (Liberty and tanker). Six merchants in two columns of three. The starboard after ship is the tanker. The three escorts that do not have the conn hold station: ahead, port beam, starboard beam, and astern of a fixed convoy center. `1`–`4` moves the conn. The wing that just lost it steers back onto its station at full, then drops to the convoy's half ahead once it is inside eight pixels.

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
- The telegraph bell and a short helm tick are sound-cli effects on voices 2 and 3 (`assets/sounds/sfx.x16snd.json`).

Still to add: enemy screws (rhythm hints at the class), sonar ping and return, charge boom, shell splash, torpedo hit, alarm bell, hull creak under depth.

## Link

Lockstep inputs, not a streamed world. At 8 Hz each side sends one short order: helm, telegraph, selected ship, weapon key, look bearing, dive plane. Both sims apply both packets. Every couple of seconds they compare `game_checksum()`. On a miss, one side sends positions, depths, and compartment flags.

The emulator already has the port. `-netcard` puts a Zimodem UART at `$9FE0`, and `ATD host:port` opens TCP. A small relay accepts two calls and pairs them. The lobby picks a side, a seed, weather, and daylight. The relay forwards inputs only, so a modified picture cannot invent a hit the other sim did not compute.

Packets stay under 32 bytes in play. A snapshot is a few hundred bytes and travels only after a desync.

This build has the checksum and a deterministic tick. It does not open the UART yet.

## What this build does

`src/game.c` is the sim, with no VERA and no KERNAL, so `test-win.cmd` can link it on the host.

- Four escorts, five Libertys, one tanker, one U-boat, all visible.
- Sea scrolls by the convoy velocity. Merchants hold. A stopped boat drifts down-screen. Flank ahead pulls away.
- Conn on escorts 1–4. The others return to station.
- U-boat hull cycles with `T` (II, VIIC, IXC, XXI). Sprite and HUD knots follow the table.
- Both helms on one keyboard. See the README.
- Pixel docs for the palette, four sea tiles, and eight sprites. Sound doc for the bell and the helm tick. Project-X16 metadata in `x16project.json`.

Proven by the host tests: the XXI is the fast submerged boat and has no gun, the IXC has two stern tubes, merchants do not drift, the sea scroll matches the convoy, a wing escort walks back to station, and two inits with the same hull produce the same checksum.

## Build order from here

1. **Fog.** Hide the truth. Hydrophone bearings and visual range. Contacts as dots.
2. **Torpedoes.** Spread, tracks, hits, one sinking sequence.
3. **Sonar and depth charges.** Active ping cone, patterns, flood timers. First real match.
4. **Guns, bridges, exterior.** Gauges, then eight-angle billboards. Fire and list.
5. **Damage that changes the fight.** Lost steering, dead sonar, crushed periscope, flat batteries, crush depth.
6. **The link.** Orders over `-netcard`, checksum every couple of seconds, snapshot on a miss.

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
