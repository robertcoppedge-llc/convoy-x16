#ifndef CONVOY_RESMAP_H
#define CONVOY_RESMAP_H
/* VRAM and RAM banks (x16-lib docs/x16-resmap.md). build-win.cmd runs resmap.cmd on this file.
 * Title and credits share the screen bitmap; a match uses the same low VRAM for the sea and the
 * sprites, so those live in different phases. Banks stay loaded for the match. */

/* X(id, name) */
#define RESMAP_PHASES(X) \
  X(P_TITLE,   "title") \
  X(P_CREDITS, "credits") \
  X(P_PLAY,    "play")

#define PH_MENU ((1u << P_TITLE) | (1u << P_CREDITS))

/* X(id, start, size, kind, group, phases, share, file, "what is in it") */
#define RESMAP_VRAM(X) \
  X(V_CARD,   0x00000, 0x12C00, RM_BITMAP, RM_PER_VIEW, PH_MENU, 0, "TITLE.BIN,CREDITS.BIN", "title or credits, 640x480 2bpp") \
  X(V_TILES,  0x00000, 0x00300, RM_TILES,  RM_PER_VIEW, (1u << P_PLAY), 0, "TILES.BIN",  "six 16x16 4bpp sea tiles") \
  X(V_SEAMAP, 0x00400, 0x02000, RM_MAP,    RM_PER_VIEW, (1u << P_PLAY), 0, RM_NOFILE,    "64x64 sea map, streamed") \
  X(V_SCOPE,  0x02400, 0x01040, RM_SPRITE, RM_PER_VIEW, (1u << P_PLAY), 0, "SCOPE.BIN",  "radar beam, scope face, two 8px blips") \
  X(V_CROSS,  0x03800, 0x00800, RM_SPRITE, RM_PER_VIEW, (1u << P_PLAY), 0, "CROSS.BIN",  "64px periscope crosshair") \
  X(V_MARKS,  0x04000, 0x00100, RM_SPRITE, RM_PER_VIEW, (1u << P_PLAY), 0, "MARKS.BIN",  "16px eel mark and burst") \
  X(V_ZOOM,   0x04200, 0x00800, RM_SPRITE, RM_PER_VIEW, (1u << P_PLAY), 0, "ZOOM.BIN",   "8px chart hulls, four kinds") \
  X(V_NUM,    0x04C00, 0x00280, RM_SPRITE, RM_PER_VIEW, (1u << P_PLAY), 0, "NUMBERS.BIN","16px and 8px digits") \
  X(V_HULL,   0x05000, 0x0B000, RM_SPRITE, RM_PER_VIEW, (1u << P_PLAY), 0, RM_NOFILE,    "22 live hull slots, 2 KB each, copied from banks") \
  X(V_BOWMAP, 0x10000, 0x04000, RM_MAP,    RM_PER_VIEW, (1u << P_PLAY), 0, "BOWMAP.BIN", "128x64 bow tile map") \
  X(V_BOW,    0x14000, 0x03C60, RM_TILES,  RM_PER_VIEW, (1u << P_PLAY), 1, "BOWESC.BIN,BOWUBT.BIN", "bow atlas; one side at a time") \
  X(V_WEAP,   0x18000, 0x01200, RM_SPRITE, RM_PER_VIEW, (1u << P_PLAY), 0, "WEAPONS.BIN","eel, torpedo wake, splash column") \
  X(V_DC,     0x19200, 0x00200, RM_SPRITE, RM_PER_VIEW, (1u << P_PLAY), 0, RM_NOFILE,    "four depth-charge slots, copied from the charge bank") \
  X(V_GAUGE,  0x19400, 0x01080, RM_SPRITE, RM_PER_VIEW, (1u << P_PLAY), 0, "GAUGES.BIN", "needles and 8px lamps") \
  X(V_TEXT,   0x1B000, 0x04000, RM_MAP,    RM_ALWAYS,   RM_ALL, 0, RM_NOFILE, "KERNAL 80x60 text") \
  X(V_FONT,   0x1F000, 0x00800, RM_TILES,  RM_ALWAYS,   RM_ALL, 0, RM_NOFILE, "KERNAL charset")

/* X(id, parent, offset, size, "what is in it") */
#define RESMAP_VSLOTS(X) \
  X(S_BEAM,   V_SCOPE, 0,    2048, "16 radar beams") \
  X(S_FACE,   V_SCOPE, 2048, 2048, "scope face") \
  X(S_BLIP,   V_SCOPE, 4096, 32,   "8px friendly blip") \
  X(S_HOSTILE,V_SCOPE, 4128, 32,   "8px hostile blip") \
  X(S_EEL,    V_WEAP,  0,    2048, "eel, 16 aspects") \
  X(S_TWAKE,  V_WEAP,  2048, 2048, "torpedo wake, 16 aspects") \
  X(S_COLUMN, V_WEAP,  4096, 512,  "splash column") \
  X(S_NEEDLE, V_GAUGE, 0,    4096, "eight needle aspects") \
  X(S_LAMP,   V_GAUGE, 4096, 128,  "on, off, tube full, tube empty") \
  X(S_NUM16,  V_NUM,   0,    512,  "four 16px digits") \
  X(S_NUM8,   V_NUM,   512,  128,  "four 8px digits")

/* X(id, first, count, group, phases, share, file, "what is in it") */
#define RESMAP_BANKS(X) \
  X(B_ESCORT,  1,  4, RM_DATA_GROUP, RM_ALL, 0, "ESCORT.BIN",  "64px chart escorts") \
  X(B_MERCH,   5,  4, RM_DATA_GROUP, RM_ALL, 0, "MERCHANT.BIN","64px chart merchants") \
  X(B_TANKER,  9,  4, RM_DATA_GROUP, RM_ALL, 0, "TANKER.BIN",  "64px chart tankers") \
  X(B_UBOAT,   13, 4, RM_DATA_GROUP, RM_ALL, 0, "UBOAT.BIN",   "64px chart U-boats") \
  X(B_MID,     21, 4, RM_DATA_GROUP, RM_ALL, 0, "MID.BIN",     "32px chart hulls") \
  X(B_TINY,    26, 1, RM_DATA_GROUP, RM_ALL, 0, "TINY.BIN",    "16px chart hulls") \
  X(B_CHARGE,  28, 1, RM_DATA_GROUP, RM_ALL, 0, "CHARGE.BIN",  "depth-charge body and splash") \
  X(B_PROFESC, 29, 4, RM_DATA_GROUP, RM_ALL, 0, "PROFESC.BIN", "64px escort profiles") \
  X(B_PROFMER, 33, 4, RM_DATA_GROUP, RM_ALL, 0, "PROFMER.BIN", "64px merchant profiles") \
  X(B_PROFTNK, 37, 4, RM_DATA_GROUP, RM_ALL, 0, "PROFTNK.BIN", "64px tanker profiles") \
  X(B_PROFUBT, 41, 4, RM_DATA_GROUP, RM_ALL, 0, "PROFUBT.BIN", "64px U-boat profiles") \
  X(B_P32ESC,  45, 1, RM_DATA_GROUP, RM_ALL, 0, "P32ESC.BIN",  "32px escort profiles") \
  X(B_P32MER,  46, 1, RM_DATA_GROUP, RM_ALL, 0, "P32MER.BIN",  "32px merchant profiles") \
  X(B_P32TNK,  47, 1, RM_DATA_GROUP, RM_ALL, 0, "P32TNK.BIN",  "32px tanker profiles") \
  X(B_P32UBT,  48, 1, RM_DATA_GROUP, RM_ALL, 0, "P32UBT.BIN",  "32px U-boat profiles") \
  X(B_P16,     49, 1, RM_DATA_GROUP, RM_ALL, 0, "P16PROF.BIN", "16px profiles, all kinds") \
  X(B_P8,      50, 1, RM_DATA_GROUP, RM_ALL, 0, "P8PROF.BIN",  "8px profiles, all kinds") \
  X(B_PHASE,   63, 1, RM_CODE_GROUP, RM_ALL, 0, "PHASE.BIN",   "title, credits, and match load")

/* X(id, row, rows, phases, share, file, "what is in it") */
#define RESMAP_PALETTE(X) \
  X(PAL_MAIN, 0, 2, RM_ALL, 0, "PAL.BIN", "two palette rows")

#include "x16lib/resmap.h"
#endif
