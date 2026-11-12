/*
 * 640x480 plot. Layer 0 is 16x16 4bpp sea tiles. Layer 1 is the KERNAL text,
 * except on the bridge, where it is the bow picture. Sprites sit between the
 * layers, so the bow covers them. The link is the netcard UART (link.c).
 */
#ifndef CONVOY_SMOKE
#define CONVOY_SMOKE 0
#endif

#include <cx16.h>
#include <conio.h>
#include <cbm.h>
#include "sfx_sfx.h"
#include <x16lib/sfx.h>
#include <x16lib/audio.h>
#include <x16lib/display.h>
#include <x16lib/fileio.h>
#include <x16lib/bank.h>
#include <x16lib/layers.h>
#include <x16lib/vera.h>
#include <x16lib/vram.h>
#include <x16lib/tilemap.h>
#include <x16lib/sprites.h>
#include "game.h"
#include "link.h"
#include "platform.h"

#define VRAM_TILES  0x00000UL
#define VRAM_MAP    0x00400UL
#define VRAM_MARKS  0x04000UL
#define VRAM_HULL   0x05000UL
#define MAP_TILES   64
#define MAP_PX      480
#define MAP_MID     240
#define HULL_B      2048u
#define IDX_MARK    ((unsigned int)(VRAM_MARKS >> 5))
#define IDX_HULL0   ((unsigned int)(VRAM_HULL >> 5))
/* Packed sprite byte 7 from the library size codes. Palette stays in the low nibble. */
#define PX8         SPR_B7(SPR_SIZE_8, SPR_SIZE_8, 0)
#define PX16        SPR_B7(SPR_SIZE_16, SPR_SIZE_16, 0)
#define PX32        SPR_B7(SPR_SIZE_32, SPR_SIZE_32, 0)
#define PX64        SPR_B7(SPR_SIZE_64, SPR_SIZE_64, 0)
/* Between the layers, so an opaque hub cell covers a sprite and the
 * transparent map cells let the hulls show. */
#define SPR_Z       SPR_B6(SPR_Z_BETWEEN_L0_L1, 0, 0, 0)
#define VRAM_ZOOM   0x04200UL
/* Between the sea map ($02400) and the crosshair ($03800). */
#define VRAM_SCOPE  0x02400UL
#define IDX_BEAM    ((unsigned int)(VRAM_SCOPE >> 5))
#define IDX_FACE    ((unsigned int)((VRAM_SCOPE + 2048UL) >> 5))
#define IDX_BLIP    ((unsigned int)((VRAM_SCOPE + 4096UL) >> 5))
#define IDX_HOSTILE ((unsigned int)(IDX_BLIP + 4u))
#define IDX_FOAM    ((unsigned int)((VRAM_SCOPE + 4352UL) >> 5))
#define IDX_RING    ((unsigned int)(IDX_FOAM + 4u))
#define SPR_N       96
#define BANK_ESCORT 1
#define BANK_MERCH  5
#define BANK_TANKER 9
#define BANK_UBOAT  13
#define BANK_WAKE   17
#define BANK_MID    21
#define BANK_MIDWAKE 25
#define BANK_TINY   26
#define BANK_TINYWAKE 27
#define BANK_CHARGE 28
/* Sixteen 64px profile aspects per kind. Four frames to a bank, same as the hulls. */
#define BANK_PROF   29
#define HUB_BG      2
#define VIEW_MAP    0
#define VIEW_BRIDGE 1
#define VIEW_GLASS  2
#define VIEW_ROOM   3
/* Bow picture on layer 1. 128x64 map of 8x8 tiles, then one atlas.
 * The atlas must end before the weapons at $18000. */
#define VRAM_BOWMAP 0x10000UL
#define VRAM_BOW    0x14000UL
#define VRAM_WEAP   0x18000UL
#define VRAM_DC     0x19200UL
#define VRAM_GAUGE  0x19400UL
#define VRAM_CROSS  0x03800UL
#define VRAM_NUM    0x04C00UL
#define IDX_EEL     ((unsigned int)(VRAM_WEAP >> 5))
#define IDX_TWAKE   ((unsigned int)((VRAM_WEAP + 2048UL) >> 5))
#define IDX_COLUMN  ((unsigned int)((VRAM_WEAP + 4096UL) >> 5))
#define IDX_DC      ((unsigned int)(VRAM_DC >> 5))
#define IDX_DIAL    ((unsigned int)(VRAM_GAUGE >> 5))
#define IDX_NEEDLE  ((unsigned int)((VRAM_GAUGE + 2048UL) >> 5))
#define IDX_LAMP    ((unsigned int)((VRAM_GAUGE + 6144UL) >> 5))
#define IDX_CROSS   ((unsigned int)(VRAM_CROSS >> 5))
#define IDX_ZWAKE   ((unsigned int)((VRAM_ZOOM + 2048UL) >> 5))
#define IDX_NUM16   ((unsigned int)(VRAM_NUM >> 5))
#define IDX_NUM8    ((unsigned int)((VRAM_NUM + 512UL) >> 5))

#define ST_TITLE 0
#define ST_HOST  1
#define ST_DIAL  2
#define ST_PLAY  3
#define ST_OVER  4

static const unsigned int drone_f[8] = { 0, 180, 360, 620, 980, 120, 180, 280 };
static const char *tel_name[8] = {
    "STOP", "SLOW", "HALF", "FULL", "FLANK", "DEAD SLOW", "ASTERN", "FULL ASTERN"
};
static const char ord_ch[3] = { 'P', 'H', 'A' };

static unsigned char gfx_ok;
static unsigned char stage, net, role, pick;
static unsigned char seq, ready, echoed, desync, step;
static char host[24];
static unsigned char hlen;

#define load_vram(name, addr) ((unsigned char)(file_load((name), FILE_TO_VRAM(addr), (unsigned int)(addr)) == FILE_OK))

/* Pixels astern of a 16px torpedo, bow-north frame 0, then clockwise. */
static const signed char astern_x[16] = {
    0, -3, -5, -6, -7, -6, -5, -3, 0, 3, 5, 6, 7, 6, 5, 3
};
static const signed char astern_y[16] = {
    7, 6, 5, 3, 0, -3, -5, -6, -7, -6, -5, -3, 0, 3, 5, 6
};
static unsigned int dc_off[4];
static unsigned char cache[22];
static unsigned char cache_bank[22];
/* Last attribute written. A repeat is not sent, so a still sprite is not
 * rewritten while the raster is reading it. */
static unsigned int spr_idx[SPR_N];
static int spr_x[SPR_N], spr_y[SPR_N];
static unsigned char spr_b6[SPR_N], spr_b7[SPR_N];
static unsigned char view, zoom, look_ofs, chrome, bow_on, bow_up;
static unsigned char l1_reg[3];
static unsigned char hub_ready;
static void hub_forget(void);

static void sprite_commit(unsigned char id, unsigned int idx, int px, int py,
                          unsigned char b6, unsigned char b7)
{
    if (id >= SPR_N) return;
    if (spr_b7[id] != 0xFF && spr_idx[id] == idx && spr_x[id] == px && spr_y[id] == py
        && spr_b6[id] == b6 && spr_b7[id] == b7)
        return;
    spr_idx[id] = idx;
    spr_x[id] = px;
    spr_y[id] = py;
    spr_b6[id] = b6;
    spr_b7[id] = b7;
    if (!b6) sprite_hide(id);
    else sprite_put(id, idx, px, py, b6, b7);
}

static void write_sprite(unsigned char id, unsigned int idx, int px, int py,
                         unsigned char show, unsigned char size)
{
    unsigned char b6 = show ? (unsigned char)(SPR_Z | ((show & 2) ? 1 : 0)) : 0;
    sprite_commit(id, idx, px, py, b6, size);
}

static void write_front(unsigned char id, unsigned int idx, int px, int py, unsigned char size)
{
    sprite_commit(id, idx, px, py, 0x0C, size);
}

static void cache_clear(void)
{
    unsigned char i;
    unsigned char d;
    for (i = 0; i < 22; i++) {
        cache[i] = 0xFF;
        cache_bank[i] = 0xFF;
    }
    for (d = 0; d < 4; d++) dc_off[d] = 0xFFFF;
}

static void blit_frame(unsigned char slot, unsigned char bank0, unsigned char frame)
{
    unsigned char old;
    unsigned char sub;
    unsigned int src;
    if (slot >= 22 || (cache[slot] == frame && cache_bank[slot] == bank0)) return;
    cache[slot] = frame;
    cache_bank[slot] = bank0;
    sub = (unsigned char)(frame & 3);
    src = (unsigned int)(X16_BANK_WIN + (unsigned int)sub * HULL_B);
    old = X16_RAM_BANK;
    X16_RAM_BANK = (unsigned char)(bank0 + (frame >> 2));
    vram_upload(VRAM_HULL + (unsigned long)slot * HULL_B, (const unsigned char *)src, HULL_B);
    X16_RAM_BANK = old;
}

/* One frame of a 32x32 or 16x16 sheet. The whole sheet fits in one bank. */
static void blit_span(unsigned char slot, unsigned char bank, unsigned char frame, unsigned int bytes)
{
    unsigned char old;
    unsigned int src;
    if (slot >= 22) return;
    if (cache[slot] == frame && cache_bank[slot] == bank) return;
    cache[slot] = frame;
    cache_bank[slot] = bank;
    src = (unsigned int)(X16_BANK_WIN + (unsigned int)frame * bytes);
    old = X16_RAM_BANK;
    X16_RAM_BANK = bank;
    vram_upload(VRAM_HULL + (unsigned long)slot * bytes, (const unsigned char *)src, bytes);
    X16_RAM_BANK = old;
}

static void blit_dc(unsigned char i, unsigned int off)
{
    unsigned char old;
    unsigned int src;
    if (i >= 4 || dc_off[i] == off) return;
    dc_off[i] = off;
    src = (unsigned int)(X16_BANK_WIN + off);
    old = X16_RAM_BANK;
    X16_RAM_BANK = BANK_CHARGE;
    vram_upload(VRAM_DC + (unsigned long)i * 128ul, (const unsigned char *)src, 128u);
    X16_RAM_BANK = old;
}

static unsigned char sea_tile(unsigned char x, unsigned char y)
{
    unsigned char h = (unsigned char)((x * 13 + y * 7) & 31);
    if (h == 0) return 1;
    if (h == 1) return 2;
    return 0;
}

static void fill_sea(void)
{
    tilemap_stream(VRAM_MAP, MAP_TILES, MAP_TILES, MAP_TILES, MAP_TILES, sea_tile, 0, 0, 0);
}

static void focus_ship(int *x, int *y)
{
    unsigned char i = (game_player() == ROLE_UBOAT) ? (unsigned char)(N_SHIP - 1) : selected;
    *x = ships[i].x;
    *y = ships[i].y;
}

static void place(unsigned char id, unsigned int idx, int wx, int wy, int camx, int camy,
                  int ox, int oy, unsigned char size)
{
    int sc = (view == VIEW_MAP) ? (1 << zoom) : 1;
    /* Palette lives in the low nibble. The gold escort sets that bit, and it
     * must not be read as a bigger sprite or her hull leaves her wake behind. */
    unsigned char dim = (unsigned char)(size & 0xF0);
    int half = (dim == (unsigned char)(PX16 & 0xF0)) ? 8
             : (dim == (unsigned char)(PX32 & 0xF0)) ? 16
             : (dim == (unsigned char)(PX64 & 0xF0)) ? 32 : 4;
    int px = (wx - camx) / FP / sc + MAP_MID - half + ox / sc;
    int py = (wy - camy) / FP / sc + MAP_MID - half + oy / sc;
    int edge = half * 2;
    unsigned char show = (unsigned char)(px >= -4 && py >= -4 &&
        px <= MAP_PX - edge && py <= MAP_PX - edge);
    write_sprite(id, idx, px, py, show, size);
}

static unsigned char bank_of(unsigned char kind)
{
    if (kind == SHIP_ESCORT) return BANK_ESCORT;
    if (kind == SHIP_TANKER) return BANK_TANKER;
    if (kind == SHIP_UBOAT) return BANK_UBOAT;
    return BANK_MERCH;
}

static unsigned char kind_i(unsigned char kind)
{
    if (kind == SHIP_ESCORT) return 0;
    if (kind == SHIP_MERCHANT) return 1;
    if (kind == SHIP_TANKER) return 2;
    return 3;
}

static void hide_sprites(void)
{
    unsigned char i;
    for (i = 0; i < SPR_N; i++)
        write_sprite(i, IDX_MARK, 0, 0, 0, PX16);
}

/* Last graticule marks. Rewritten only when a tick actually moves. */
static unsigned char mark_x[12], mark_y[12], mark_on[12];
static char mark_t[12][4];

static void wipe_mark(unsigned char n)
{
    unsigned char k;
    if (!mark_on[n]) return;
    bgcolor(COLOR_BLACK);
    for (k = 0; k < 4; k++)
        cputcxy((unsigned char)(mark_x[n] + k), mark_y[n], ' ');
    mark_on[n] = 0;
}

static void put_mark(unsigned char n, unsigned char x, unsigned char y, int v)
{
    char d[4];
    unsigned int u;
    unsigned char k;
    if (v < 0) { d[0] = '-'; v = -v; }
    else d[0] = ' ';
    if (v > 999) v = 999;
    u = (unsigned int)v;
    d[1] = (char)('0' + (u / 100) % 10);
    d[2] = (char)('0' + (u / 10) % 10);
    d[3] = (char)('0' + u % 10);
    if (mark_on[n] && mark_x[n] == x && mark_y[n] == y &&
        mark_t[n][0] == d[0] && mark_t[n][1] == d[1] &&
        mark_t[n][2] == d[2] && mark_t[n][3] == d[3])
        return;
    wipe_mark(n);
    bgcolor(COLOR_BLACK);
    textcolor(COLOR_WHITE);
    for (k = 0; k < 4; k++) {
        cputcxy((unsigned char)(x + k), y, d[k]);
        mark_t[n][k] = d[k];
    }
    mark_x[n] = x;
    mark_y[n] = y;
    mark_on[n] = 1;
}

static void graticule(int camx, int camy)
{
    unsigned char used;
    int scale = 1 << zoom;
    int step = 64 << zoom;
    int east0 = camx / FP + scroll_x + 4096;
    int north0 = -(camy / FP) - scroll_y + 4096;
    int base, k, g, s;

    used = 0;
    base = east0 - (east0 % step) - step * 3;
    for (k = 0; k < 8 && used < 4; k++) {
        g = base + k * step;
        s = MAP_MID + (g - east0) / scale;
        if (s < 8 || s > 400) continue;
        put_mark(used, (unsigned char)(s / 8), 0, g - 4096);
        used++;
    }
    while (used < 4) wipe_mark(used++);
    base = north0 - (north0 % step) - step * 3;
    for (k = 0; k < 8 && used < 8; k++) {
        g = base + k * step;
        s = MAP_MID + (north0 - g) / scale;
        if (s < 16 || s > 440) continue;
        put_mark(used, 0, (unsigned char)(s / 8), g - 4096);
        used++;
    }
    while (used < 12) wipe_mark(used++);
}

static void clear_graticule(void)
{
    unsigned char n;
    for (n = 0; n < 12; n++) wipe_mark(n);
}

static void save_text_layer(void)
{
    l1_reg[0] = VERA_L1_CONFIG;
    l1_reg[1] = VERA_L1_MAPBASE;
    l1_reg[2] = VERA_L1_TILEBASE;
}

/* The bridge bow is a 4bpp tile picture on layer 1. Colour 0 in the picture
 * is empty, so the sea and the ships show around the hull. */
static void show_bow(unsigned char on)
{
    if (on == bow_up) return;
    bow_up = on;
    if (on) {
        layer_config_tile(1, VRAM_BOWMAP, VRAM_BOW, 2, 1, 8, 8, LAYER_BPP_4);
        layer_scroll(1, 0, 0);
        return;
    }
    VERA_L1_CONFIG = l1_reg[0];
    VERA_L1_MAPBASE = l1_reg[1];
    VERA_L1_TILEBASE = l1_reg[2];
}

static void paint_chrome(void)
{
    unsigned char r, want;
    /* The conning tower only shows a bow while the boat is on the surface. */
    want = (unsigned char)(view == VIEW_BRIDGE &&
            !(game_player() == ROLE_UBOAT && game_depth() >= 8));
    if (chrome == view && bow_on == want) return;
    clear_graticule();
    if (view == VIEW_MAP) {
        bgcolor(COLOR_BLACK);
        for (r = 0; r < 60; r++)
            cclearxy(0, r, 60);
    } else if (view == VIEW_BRIDGE) {
        /* The sea shows through. The bow itself is the picture on layer 1. */
        bgcolor(COLOR_BLACK);
        for (r = 0; r < 60; r++) cclearxy(0, r, 80);
    } else if (view == VIEW_GLASS) {
        unsigned char col, row;
        for (row = 0; row < 60; row++) {
            for (col = 0; col < 80; col++) {
                int dx, dy, in;
                unsigned char bg;
                if (game_player() == ROLE_UBOAT) {
                    dx = (int)col - 40; dy = (int)row - 30;
                    in = dx * dx + dy * dy <= 24 * 24;
                } else {
                    dx = (int)col - 22; dy = (int)row - 30;
                    in = dx * dx + dy * dy <= 16 * 16;
                    dx = (int)col - 58;
                    in = in || (dx * dx + dy * dy <= 16 * 16);
                }
                /* Sky and the horizon stay on this layer. The sea below is
                 * clear so the scrolling water shows through the glass. */
                if (!in) bg = HUB_BG;
                else if (row < 24) bg = 8;
                else if (row == 24) bg = COLOR_WHITE;
                else bg = COLOR_BLACK;
                bgcolor(bg);
                cclearxy(col, row, 1);
            }
        }
        if (game_player() == ROLE_UBOAT) {
            bgcolor(8);
            textcolor(COLOR_WHITE);
            for (col = 20; col < 60; col += 2) cputcxy(col, 30, '-');
            for (row = 12; row < 48; row += 2) cputcxy(40, row, '+');
            cputcxy(40, 30, '+');
        }
    } else {
        bgcolor(HUB_BG);
        for (r = 0; r < 60; r++) cclearxy(0, r, 80);
        textcolor(COLOR_WHITE);
        cputsxy(6, 2, "BEARING");
        cputsxy(22, 2, "DEPTH");
        cputsxy(40, 2, "PLANES");
        cputsxy(6, 24, "TUBES");
        cputsxy(6, 36, "BATTERY");
        cputsxy(6, 48, "Y RISE   U DIVE");
    }
    bow_on = want;
    chrome = view;
    show_bow(want);
}

/* Westerly sea, in the same units as game_water. Waves run east and a little south. */
#define WAVE_X 10
#define WAVE_Y 4
static int sea_u, sea_v;
static unsigned int wind_p;

static void draw_map(int camx, int camy)
{
    unsigned char i;
    int x, y;
    int sc = 1 << zoom;
    int hs, vs;
    wind_p++;
    hs = camx / FP / sc - MAP_MID + scroll_x / sc - (int)(wind_p >> 3);
    vs = camy / FP / sc - MAP_MID + scroll_y / sc - (int)(wind_p >> 4);
    layer_scroll(0, (unsigned int)hs, (unsigned int)vs);
    for (i = 0; i < N_SHIP; i++) {
        unsigned char frame, kind;
        unsigned int idx;
        unsigned char sz;
        if (!game_show_ship(i)) {
            write_sprite(i, 0, 0, 0, 0, 0);
            write_sprite((unsigned char)(11 + i), 0, 0, 0, 0, 0);
            if (i < N_ESCORT) write_sprite((unsigned char)(64 + i), 0, 0, 0, 0, 0);
            continue;
        }
        frame = (unsigned char)(ships[i].hdg >> 4);
        kind = kind_i(ships[i].kind);
        if (zoom == 0) {
            blit_frame(i, bank_of(ships[i].kind), frame);
            idx = (unsigned int)(IDX_HULL0 + (unsigned int)i * 64u);
            sz = PX64;
        } else if (zoom == 1) {
            blit_span(i, (unsigned char)(BANK_MID + kind), frame, 512u);
            idx = (unsigned int)(IDX_HULL0 + (unsigned int)i * 16u);
            sz = PX32;
        } else if (zoom == 2) {
            blit_span(i, BANK_TINY, (unsigned char)(kind * 16u + frame), 128u);
            idx = (unsigned int)(IDX_HULL0 + (unsigned int)i * 4u);
            sz = PX16;
        } else {
            idx = (unsigned int)((VRAM_ZOOM >> 5) + (unsigned int)kind * 16u + frame);
            sz = PX8;
        }
        write_sprite((unsigned char)(11 + i), 0, 0, 0, 0, 0);
        if (game_player() == ROLE_ESCORT && i == selected && i < N_ESCORT)
            sz = (unsigned char)(sz | 1);
        place(i, idx, ships[i].x, ships[i].y, camx, camy, 0, 0, sz);
        if (i < N_ESCORT) {
            unsigned char nsz = (zoom >= 2) ? PX8 : PX16;
            unsigned int nidx = (zoom >= 2)
                ? (unsigned int)(IDX_NUM8 + i)
                : (unsigned int)(IDX_NUM16 + (unsigned int)i * 4u);
            place((unsigned char)(64 + i), nidx, ships[i].x, ships[i].y, camx, camy,
                  -28 * sc, -6 * sc, nsz);
        }
    }
    for (i = 0; i < 8; i++) {
        int fx, fy;
        if (game_wake_at(i, &fx, &fy))
            place((unsigned char)(32 + i), IDX_RING, fx, fy, camx, camy, 0, 0, PX32);
        else
            write_sprite((unsigned char)(32 + i), 0, 0, 0, 0, 0);
    }
    for (i = 0; i < N_TORP; i++) {
        unsigned char face;
        if (!game_torp_on(i)) {
            write_sprite((unsigned char)(22 + i), 0, 0, 0, 0, 0);
            write_sprite((unsigned char)(60 + i), 0, 0, 0, 0, 0);
            continue;
        }
        face = (unsigned char)(game_torp_hdg(i) >> 4);
        game_torp_xy(i, &x, &y);
        place((unsigned char)(22 + i),
              (unsigned int)(IDX_EEL + (unsigned int)face * 4u),
              x, y, camx, camy, 0, 0, PX16);
        place((unsigned char)(60 + i),
              (unsigned int)(IDX_TWAKE + (unsigned int)face * 4u),
              x, y, camx, camy,
              (int)astern_x[face] * 2, (int)astern_y[face] * 2, PX16);
    }
    for (i = 0; i < N_DC; i++) {
        unsigned char life, face;
        unsigned int idx;
        unsigned char sz;
        int oy;
        if (!game_dc_on(i)) {
            write_sprite((unsigned char)(26 + i), 0, 0, 0, 0, 0);
            continue;
        }
        life = game_dc_life(i);
        face = (unsigned char)(game_dc_hdg(i) >> 4);
        game_dc_xy(i, &x, &y);
        oy = 0;
        sz = PX16;
        if (life > 21) {
            blit_dc(i, (unsigned int)face * 128u);
            idx = (unsigned int)(IDX_DC + (unsigned int)i * 4u);
            oy = (int)(28 - life) * 5;
        } else if (life > 14) {
            blit_dc(i, 2048u + (unsigned int)face * 128u);
            idx = (unsigned int)(IDX_DC + (unsigned int)i * 4u);
        } else if (life > 8) {
            write_sprite((unsigned char)(26 + i), 0, 0, 0, 0, 0);
            continue;
        } else {
            idx = IDX_COLUMN;
            sz = PX32;
        }
        place((unsigned char)(26 + i), idx, x, y, camx, camy, 0, oy, sz);
    }
    if (game_show_contact(&x, &y))
        place(30, IDX_MARK + 4u, x, y, camx, camy, 0, 0, PX16);
    else
        write_sprite(30, IDX_MARK, 0, 0, 0, PX16);
    if (game_shell_on()) {
        game_shell_xy(&x, &y);
        place(59, IDX_HOSTILE, x, y, camx, camy, 0, 0, PX16);
    } else {
        write_sprite(59, 0, 0, 0, 0, 0);
    }
    graticule(camx, camy);
}

/* sin of heading/16, 0 = north, clockwise, scale 127. */
static const signed char sin16[16] = {
    0, 49, 90, 117, 127, 117, 90, 49, 0, -49, -90, -117, -127, -117, -90, -49
};

static void look_axes(int dx, int dy, int s, int c, int *ahead, int *side)
{
    *ahead = (int)(((long)dx * s - (long)dy * c) >> 7);
    *side = (int)(((long)dx * c + (long)dy * s) >> 7);
}

static unsigned char project_ahead(int wx, int wy, int camx, int camy, int s, int c,
                                  int horizon, int reach, int mag, int hot, int *px, int *py);

static unsigned char draw_ahead_one(unsigned char i, int camx, int camy, int s, int c,
                                   int horizon, int reach, int mag, unsigned char look,
                                   unsigned char slot)
{
    int px, py;
    unsigned char face;
    unsigned int idx;
    if (slot >= 8 || !game_show_ship(i)) return slot;
    if ((game_player() == ROLE_UBOAT && i == N_SHIP - 1) ||
        (game_player() == ROLE_ESCORT && i == selected))
        return slot;
    if (!project_ahead(ships[i].x, ships[i].y, camx, camy, s, c, horizon, reach, mag, 32, &px, &py))
        return slot;
    /* One of sixteen painted aspects, copied out of banked RAM into this slot. */
    face = (unsigned char)(((ships[i].hdg + 128 - look) >> 4) & 15);
    blit_frame(slot, (unsigned char)(BANK_PROF + (unsigned char)(kind_i(ships[i].kind) * 4)), face);
    idx = (unsigned int)(IDX_HULL0 + (unsigned int)slot * 64u);
    sprite_commit(slot, idx, px, py, SPR_Z, PX64);
    return (unsigned char)(slot + 1);
}

static unsigned char project_ahead(int wx, int wy, int camx, int camy, int s, int c,
                                  int horizon, int reach, int mag, int hot,
                                  int *px, int *py)
{
    int dx = (wx - camx) >> 4;
    int dy = (wy - camy) >> 4;
    int ahead, side;
    look_axes(dx, dy, s, c, &ahead, &side);
    /* The ahead views use the full 640. The chart stays 480, with the hub. */
    if (ahead < -48 || ahead > reach || side > 320 || side < -320) return 0;
    if (ahead < 0) ahead = 0;
    if (mag > 1) side >>= 1;
    *px = 320 + side - hot;
    *py = horizon + ((reach - ahead) >> 3) - hot;
    if (*px < -16 || *px >= 640) return 0;
    return 1;
}

static void draw_ahead(int camx, int camy, unsigned char look)
{
    unsigned char i, slot;
    unsigned char li = (unsigned char)(look >> 4);
    unsigned char who = (game_player() == ROLE_UBOAT) ? (unsigned char)(N_SHIP - 1) : selected;
    int s = sin16[li];
    int c = sin16[(li + 4) & 15];
    int horizon = (view == VIEW_GLASS) ? 200 : 168;
    int reach = (view == VIEW_GLASS) ? 420 : 280;
    int mag = (view == VIEW_GLASS) ? 2 : 1;
    int mvx, mvy, along, across;
    /* Waves keep the wind's heading. Subtract the ship's own way, then
     * lay that on the glass: ahead comes down the window, abeam slides
     * across. The horizon is painted on the layer above, so this scroll
     * does not move it. */
    game_water(who, &mvx, &mvy);
    look_axes(WAVE_X - mvx, WAVE_Y - mvy, s, c, &along, &across);
    sea_u -= across;
    sea_v += along;
    layer_scroll(0, (unsigned int)(sea_u >> 6), (unsigned int)(sea_v >> 6));
    slot = 0;
    if (game_player() == ROLE_ESCORT)
        slot = draw_ahead_one((unsigned char)(N_SHIP - 1), camx, camy, s, c, horizon, reach, mag, look, slot);
    for (i = 0; i < N_SHIP && slot < 8; i++) {
        if (game_player() == ROLE_ESCORT && i == N_SHIP - 1) continue;
        slot = draw_ahead_one(i, camx, camy, s, c, horizon, reach, mag, look, slot);
    }
    while (slot < 8) {
        write_sprite(slot, 0, 0, 0, 0, 0);
        slot++;
    }
    for (i = 0; i < N_TORP; i++) {
        int x, y, px, py;
        unsigned char face;
        if (!game_torp_on(i)) {
            write_sprite((unsigned char)(8 + i), 0, 0, 0, 0, 0);
            continue;
        }
        face = (unsigned char)((game_torp_hdg(i) - look) >> 4);
        game_torp_xy(i, &x, &y);
        if (!project_ahead(x, y, camx, camy, s, c, horizon, reach, mag, 8, &px, &py)) {
            write_sprite((unsigned char)(8 + i), 0, 0, 0, 0, 0);
            continue;
        }
        sprite_commit((unsigned char)(8 + i), (unsigned int)(IDX_TWAKE + (unsigned int)face * 4u), px, py, SPR_Z, PX16);
    }
    if (view == VIEW_GLASS && game_player() == ROLE_UBOAT)
        write_front(40, IDX_CROSS, 292, 212, PX64);
    else
        write_sprite(40, IDX_CROSS, 0, 0, 0, PX64);
}

static void draw_room(void)
{
    unsigned char i, tubes, face;
    signed char pl;
    write_sprite(40, IDX_CROSS, 0, 0, 0, PX64);
    write_front(41, IDX_DIAL, 24, 32, PX64);
    write_front(42, IDX_DIAL, 168, 32, PX64);
    write_front(43, IDX_DIAL, 312, 32, PX64);
    face = (unsigned char)(ships[N_SHIP - 1].hdg >> 5);
    write_front(44, (unsigned int)(IDX_NEEDLE + (unsigned int)face * 16u), 40, 48, PX32);
    face = (unsigned char)((game_depth() * 7u) / 80u);
    write_front(45, (unsigned int)(IDX_NEEDLE + (unsigned int)face * 16u), 184, 48, PX32);
    pl = game_planes_of();
    face = (unsigned char)((pl + 2) * 2);
    if (face > 7) face = 7;
    write_front(46, (unsigned int)(IDX_NEEDLE + (unsigned int)face * 16u), 328, 48, PX32);
    tubes = game_tubes();
    for (i = 0; i < 4; i++)
        write_front((unsigned char)(47 + i),
                    (unsigned int)(IDX_LAMP + (i < tubes ? 8u : 12u)),
                    48 + (int)i * 40, 210, PX16);
    for (i = 0; i < 8; i++)
        write_front((unsigned char)(51 + i),
                    (unsigned int)(IDX_LAMP + (game_battery() > (unsigned char)(i * 5) ? 0u : 4u)),
                    48 + (int)i * 24, 310, PX16);
    bgcolor(HUB_BG);
    textcolor(COLOR_WHITE);
    cputcxy(23, 13, (char)('0' + (game_depth() / 10) % 10));
    cputcxy(24, 13, (char)('0' + game_depth() % 10));
    cputcxy(41, 13, (char)(pl < 0 ? '-' : '+'));
    cputcxy(42, 13, (char)('0' + (pl < 0 ? -pl : pl)));
}

static void draw_world(void)
{
    int camx, camy;
    unsigned char look;
    focus_ship(&camx, &camy);
    if (view == VIEW_BRIDGE) look_ofs = 0;
    look = (unsigned char)(ships[(game_player() == ROLE_UBOAT) ? (N_SHIP - 1) : selected].hdg + look_ofs);
    paint_chrome();
    if (view == VIEW_MAP) draw_map(camx, camy);
    else if (view == VIEW_ROOM) draw_room();
    else draw_ahead(camx, camy, look);
}

/* Color 0 is transparent on a tile layer, so the text background lets
 * the sea and the ships show. The boot screen is opaque blue cells. */
static void clear_screen(void)
{
    show_bow(0);
    bgcolor(COLOR_BLACK);
    textcolor(COLOR_WHITE);
    clrscr();
    cursor(0);
    /* cc65 stores 'A' as screen code 0x41. ROM charset 2 is PET graphics,
     * where that code is a symbol. Charset 1 is ISO, where it is a letter.
     * $FF62 is screen_set_charset. */
    __asm__("lda #1\njsr $FF62");
    hub_forget();
}

static void hub_panel(void)
{
    unsigned char r;
    bgcolor(HUB_BG);
    textcolor(COLOR_WHITE);
    for (r = 0; r < 60; r++)
        cclearxy(60, r, 20);
}

static char hub_mem[8][20];

static void hub_forget(void)
{
    unsigned char r, c;
    for (r = 0; r < 8; r++)
        for (c = 0; c < 20; c++) hub_mem[r][c] = 0;
    hub_ready = 0;
    chrome = 0xFF;
    bow_on = 0xFF;
}

static void hub_row(unsigned char row)
{
    bgcolor(HUB_BG);
    textcolor(COLOR_WHITE);
    cclearxy(60, row, 20);
    gotoxy(60, row);
}

static void hub_set(unsigned char row, const char *s)
{
    unsigned char i;
    char buf[20];
    if (row >= 8) return;
    for (i = 0; i < 19 && s[i]; i++) buf[i] = s[i];
    for (; i < 19; i++) buf[i] = ' ';
    buf[19] = 0;
    for (i = 0; i < 20; i++)
        if (hub_mem[row][i] != buf[i]) break;
    if (i == 20) return;
    for (i = 0; i < 20; i++) hub_mem[row][i] = buf[i];
    hub_row(row);
    cputs(buf);
}

static void hub_instructions(void)
{
    if (hub_ready) return;
    hub_panel();
    gotoxy(77, 6); cputs("RAD");
    gotoxy(77, 22); cputs("SON");
    gotoxy(77, 38); cputs("HYD");
    if (game_player() == ROLE_UBOAT) {
        hub_row(54); cputs("O MAP  B G  C ROOM");
        hub_row(55); cputs("J L HELM  I K SPEED");
        hub_row(56); cputs("F FIRE  Y U DIVE");
        hub_row(57); cputs("N M LOOK IN GLASS");
        hub_row(59); cputs("4 TONS TO WIN");
    } else {
        hub_row(54); cputs("O B G   Z ZOOM");
        hub_row(55); cputs("1-4 []   P H A");
        hub_row(56); cputs("S PING  D CHARGE");
        hub_row(57); cputs("F DECK GUN");
        hub_row(58); cputs("ARROWS HELM SPEED");
        hub_row(59); cputs("N M STEER  LOOK G");
    }
    hub_ready = 1;
    bgcolor(COLOR_BLACK);
}

/* Each face is four 64px quadrants, flipped around the shared corner. Up is the bow. */
#define SCOPE_X 544
#define RADAR_Y 112
#define SONAR_Y 240
#define HYDRO_Y 368

static void scope_hide(unsigned char id)
{
    write_sprite(id, 0, 0, 0, 0, 0);
}

static unsigned char bearing16(int dx, int dy)
{
    unsigned char i, best = 0;
    long best_dot = -200000L;
    for (i = 0; i < 16; i++) {
        long dot = (long)dx * (long)sin16[i] - (long)dy * (long)sin16[(i + 4) & 15];
        if (dot > best_dot) { best_dot = dot; best = i; }
    }
    return best;
}

static int span_r(int dx, int dy, int limit, int radius)
{
    int ax = dx < 0 ? -dx : dx;
    int ay = dy < 0 ? -dy : dy;
    int hyp = (ax > ay) ? (ax + ay / 2) : (ay + ax / 2);
    if (hyp > limit) return -1;
    if (hyp < 4) hyp = 4;
    return (int)((long)hyp * radius / limit);
}

static void scope_at(unsigned char slot, unsigned int idx, unsigned char li,
                     int cx, int cy, int r, unsigned char sz, int hotspot)
{
    int x = cx + (int)sin16[li] * r / 127 - hotspot;
    int y = cy - (int)sin16[(li + 4) & 15] * r / 127 - hotspot;
    write_front(slot, idx, x, y, sz);
}

static void scope_ray(unsigned char slot, unsigned char li, int cx, int cy, int r)
{
    scope_at(slot, (unsigned int)(IDX_BEAM + (unsigned int)li * 4u), li, cx, cy, r, PX16, 8);
}

/* One quadrant, then the horizontal, vertical, and both-axis mirrors. */
static void scope_quad(unsigned char slot, int cx, int cy)
{
    sprite_commit(slot, IDX_FACE, cx - 64, cy - 64, 0x0C, PX64);
    sprite_commit((unsigned char)(slot + 1), IDX_FACE, cx, cy - 64, 0x0D, PX64);
    sprite_commit((unsigned char)(slot + 2), IDX_FACE, cx - 64, cy, 0x0E, PX64);
    sprite_commit((unsigned char)(slot + 3), IDX_FACE, cx, cy, 0x0F, PX64);
}

static void scope_blip(unsigned char slot, unsigned int idx, unsigned char li, int cx, int cy, int r)
{
    scope_at(slot, idx, li, cx, cy, r, PX16, 8);
}

static void draw_scopes(void)
{
    unsigned char i, nblip, focus, own, rel, li;
    unsigned char radar_on;
    int dx, dy, rr;
    static unsigned char sweep;
    if (stage != ST_PLAY && stage != ST_OVER) {
        for (i = 68; i < SPR_N; i++) scope_hide(i);
        return;
    }
    if (!gfx_ok) return;
    focus = (game_player() == ROLE_UBOAT) ? (unsigned char)(N_SHIP - 1) : selected;
    own = (unsigned char)(ships[focus].hdg >> 4);
    radar_on = (unsigned char)(game_player() == ROLE_ESCORT || game_depth() == 0);
    scope_quad(84, SCOPE_X, RADAR_Y);
    scope_quad(88, SCOPE_X, SONAR_Y);
    scope_quad(92, SCOPE_X, HYDRO_Y);
    if (radar_on) {
        sweep = (unsigned char)(sweep + 2);
        li = (unsigned char)(sweep >> 4);
        scope_ray(76, li, SCOPE_X, RADAR_Y, 8);
        scope_ray(77, li, SCOPE_X, RADAR_Y, 30);
        scope_ray(78, li, SCOPE_X, RADAR_Y, 52);
        nblip = 0;
        for (i = 0; i < N_SHIP && nblip < 8; i++) {
            if (i == focus || !game_alive(i)) continue;
            if (i == N_SHIP - 1 && game_depth() >= 8) continue;
            dx = (int)(((long)ships[i].x - ships[focus].x) / FP);
            dy = (int)(((long)ships[i].y - ships[focus].y) / FP);
            rr = span_r(dx, dy, 340, 52);
            if (rr < 0) continue;
            rel = (unsigned char)((bearing16(dx, dy) - own) & 15);
            scope_blip((unsigned char)(68 + nblip),
                       (i == (unsigned char)(N_SHIP - 1) || game_player() == ROLE_UBOAT) ? IDX_HOSTILE : IDX_BLIP,
                       rel, SCOPE_X, RADAR_Y, rr);
            nblip++;
        }
        while (nblip < 8) scope_hide((unsigned char)(68 + nblip++));
    } else {
        for (i = 68; i < 79; i++) scope_hide(i);
    }
    if (game_player() == ROLE_ESCORT) {
        int cx, cy;
        scope_ray(80, 2, SCOPE_X, SONAR_Y, 40);
        scope_ray(81, 14, SCOPE_X, SONAR_Y, 40);
        if (game_show_contact(&cx, &cy)) {
            dx = (int)(((long)cx - ships[focus].x) / FP);
            dy = (int)(((long)cy - ships[focus].y) / FP);
            rel = (unsigned char)((bearing16(dx, dy) - own) & 15);
            rr = span_r(dx, dy, 130, 52);
            if (rr >= 0 && (rel <= 2 || rel >= 14))
                scope_blip(79, IDX_HOSTILE, rel, SCOPE_X, SONAR_Y, rr);
            else
                scope_hide(79);
        } else scope_hide(79);
    } else {
        unsigned char who = game_ping_who();
        scope_hide(80);
        scope_hide(81);
        if (who < N_ESCORT) {
            dx = (int)(((long)ships[who].x - ships[focus].x) / FP);
            dy = (int)(((long)ships[who].y - ships[focus].y) / FP);
            rel = (unsigned char)((bearing16(dx, dy) - own) & 15);
            scope_blip(79, IDX_HOSTILE, rel, SCOPE_X, SONAR_Y, 48);
        } else scope_hide(79);
    }
    if (game_hear_xy(&dx, &dy)) {
        rel = (unsigned char)((bearing16(dx, dy) - own) & 15);
        scope_ray(82, rel, SCOPE_X, HYDRO_Y, 20);
        scope_ray(83, rel, SCOPE_X, HYDRO_Y, 48);
    } else {
        scope_hide(82);
        scope_hide(83);
    }
}

/* Sixteen compass points, 0 north then clockwise. One character fits; a
 * three-digit degree line does not. */
static char nyb(unsigned char n)
{
    return (char)(n < 10 ? '0' + n : 'A' + n - 10);
}

static void glass_bearing(void)
{
    unsigned char focus = (game_player() == ROLE_UBOAT) ? (unsigned char)(N_SHIP - 1) : selected;
    unsigned char h = (unsigned char)(ships[focus].hdg >> 4);
    bgcolor(HUB_BG);
    textcolor(COLOR_WHITE);
    cputcxy(2, 2, 'H');
    cputcxy(3, 2, nyb(h));
    cputcxy(5, 2, 'L');
    cputcxy(6, 2, nyb((unsigned char)((ships[focus].hdg + look_ofs) >> 4)));
    bgcolor(COLOR_BLACK);
}

static void hud_play(void)
{
    unsigned char focus;
    unsigned int deg, heard;
    hub_instructions();
    focus = (game_player() == ROLE_UBOAT) ? (unsigned char)(N_SHIP - 1) : selected;
    deg = (unsigned int)((ships[focus].hdg * 45u) >> 5);
    hub_set(0, desync ? "DESYNC" : (net ? "LINK" : "SOLO"));
    hub_set(1, game_player() == ROLE_UBOAT ? "U-BOAT" : "ESCORT");
    if (zoom == 1) hub_set(2, "MAP 32");
    else if (zoom == 2) hub_set(2, "MAP 16");
    else if (zoom == 3) hub_set(2, "MAP 8");
    else hub_set(2, "MAP");
    if (game_player() == ROLE_ESCORT) {
        unsigned char i;
        char line[20];
        unsigned char n = 0;
        for (i = 0; i < N_ESCORT; i++) {
            line[n++] = (char)(i == selected ? '*' : ' ');
            line[n++] = (char)('1' + i);
            line[n++] = (char)(game_alive(i) ? ord_ch[game_order_of(i)] : 'x');
        }
        line[n] = 0;
        hub_set(3, line);
    } else {
        hub_set(3, boat_name(uboat_type));
    }
    hub_set(4, tel_name[ships[focus].tel]);
    {
        char line[20];
        unsigned char d = game_depth();
        line[0] = (char)('0' + (deg / 100) % 10);
        line[1] = (char)('0' + (deg / 10) % 10);
        line[2] = (char)('0' + deg % 10);
        line[3] = ' ';
        line[4] = 'T';
        line[5] = (char)('0' + game_tubes());
        line[6] = ' ';
        line[7] = 'H';
        line[8] = (char)('0' + game_hits());
        line[9] = ' ';
        line[10] = (char)('0' + game_tonnage());
        line[11] = '/';
        line[12] = '4';
        if (game_player() == ROLE_UBOAT) {
            line[13] = ' ';
            line[14] = 'D';
            line[15] = (char)('0' + (d / 10) % 10);
            line[16] = (char)('0' + d % 10);
            line[17] = 0;
        } else if (game_watch(&heard)) {
            line[13] = ' ';
            line[14] = 'W';
            line[15] = 0;
        } else line[13] = 0;
        hub_set(5, line);
    }
    if (!gfx_ok) hub_set(0, "NO ART");
    bgcolor(COLOR_BLACK);
}

static void begin_match(unsigned char as, unsigned char linked)
{
    role = as;
    net = linked;
    ready = (unsigned char)!linked;
    echoed = 0;
    desync = 0;
    seq = 0;
    step = 0;
    game_init(pick);
    game_set_player(as);
    if (!load_vram(as == ROLE_UBOAT ? "bowubt.bin" : "bowesc.bin", VRAM_BOW))
        gfx_ok = 0;
    cache_clear();
    hub_forget();
    clear_screen();
    stage = ST_PLAY;
}

static void title_screen(void)
{
    hub_instructions();
    hub_set(0, "CONVOY");
    hub_set(1, "1 ESCORT");
    hub_set(2, "2 U-BOAT");
    hub_set(3, "3 LINK");
    hub_set(4, "T CYCLES THE BOAT");
    hub_set(5, boat_name(pick));
    hub_set(6, "O MAP B BRIDGE");
    hub_set(7, "G GLASS C ROOM");
}

static void host_screen(void)
{
    hub_instructions();
    hub_set(0, role == ROLE_UBOAT ? "SIDE U-BOAT" : "SIDE ESCORT");
    hub_set(1, "E ESCORT SIDE");
    hub_set(2, "U U-BOAT SIDE");
    hub_set(3, "ADDRESS");
    hub_set(4, host);
    hub_set(5, "ENTER DIALS");
    hub_set(6, "ESC BACK");
    hub_set(7, "RELAY PORT 6416");
}

static void set_view(unsigned char next)
{
    if (view == next && next != VIEW_MAP) return;
    view = next;
    if (view != VIEW_MAP) zoom = 0;
    cache_clear();
    hide_sprites();
    hub_forget();
}

static void apply_view_key(unsigned char k)
{
    if (k == 'o' || k == 'O') set_view(VIEW_MAP);
    else if (k == 'b' || k == 'B') set_view(VIEW_BRIDGE);
    else if (k == 'g' || k == 'G') set_view(VIEW_GLASS);
    else if ((k == 'c' || k == 'C') && game_player() == ROLE_UBOAT) set_view(VIEW_ROOM);
    else if (k == 'z' || k == 'Z') {
        view = VIEW_MAP;
        zoom = (unsigned char)((zoom + 1) & 3);
        cache_clear();
        hub_forget();
    } else if (k == 'n' || k == 'N' || k == 'm' || k == 'M') {
        if (stage == ST_PLAY && view != VIEW_GLASS)
            look_ofs = 0;
    }
}

static void apply_play_key(unsigned char k)
{
    apply_view_key(k);
    if (game_player() == ROLE_ESCORT) {
        if (k == '[' || k == ',') game_cycle(-1);
        else if (k == ']' || k == '.' || k == 9) game_cycle(1);
        else if (k >= '1' && k <= '4') game_select((unsigned char)(k - '1'));
        else if (k == 'p' || k == 'P') game_order(ORD_PATROL);
        else if (k == 'h' || k == 'H') game_order(ORD_HUNT);
        else if (k == 'a' || k == 'A') game_order(ORD_ATTACK);
        else if (k == 's' || k == 'S') game_ping();
        else if (k == 'd' || k == 'D') game_drop();
        else if (k == 'f' || k == 'F') game_gun();
        else if (k == CH_CURS_UP) game_telegraph_delta(1);
        else if (k == CH_CURS_DOWN) game_telegraph_delta(-1);
    } else {
        if (k == 'i' || k == 'I') game_uboat_telegraph_delta(1);
        else if (k == 'k' || k == 'K') game_uboat_telegraph_delta(-1);
        else if (k == 'f' || k == 'F' || k == ' ') game_fire();
        else if (k == 'y' || k == 'Y') game_planes(-1);
        else if (k == 'u' || k == 'U') game_planes(1);
    }
}

static void play_tick(unsigned char helm, unsigned char uhelm)
{
    unsigned char side, data[4], saw = 0;
    if (game_player() == ROLE_ESCORT) game_helm(helm);
    else game_uboat_helm(uhelm);

    if (net) {
        unsigned char hello, got[4];
        while (link_pump(&side, data)) {
            if (!ready) continue;
            if (((side == 'U' && role == ROLE_ESCORT) || (side == 'E' && role == ROLE_UBOAT)) &&
                ((data[2] >> 4) == (seq & 15))) {
                got[0] = data[0]; got[1] = data[1]; got[2] = data[2]; got[3] = data[3];
                saw = 1;
            }
        }
        if (link_take_hello(&hello)) {
            if (!ready) {
                pick = hello;
                game_init(pick);
                game_set_player(role);
                ready = 1;
                seq = 0;
                if (role == ROLE_UBOAT) echoed = 1;
            }
        }
        if (link_state() == LINK_UP && role == ROLE_ESCORT && !ready)
            link_send_hello(pick);
        if (echoed == 1) {
            link_send_hello(pick);
            echoed = 2;
        }
        if (ready) {
            if (role == ROLE_ESCORT) game_export_escort(seq, data);
            else game_export_uboat(seq, data);
            link_send_packet(role == ROLE_ESCORT ? 'E' : 'U', data);
        }
        if (!ready || !saw) {
            hud_play();
            hub_row(28);
            cputs(ready ? "WAITING ORDERS" : "WAITING SIDE");
            bgcolor(COLOR_BLACK);
            return;
        }
        if (role == ROLE_ESCORT) {
            if (!game_import_uboat(got)) desync = 1;
        } else if (!game_import_escort(got)) desync = 1;
        seq = (unsigned char)((seq + 1) & 15);
    } else if (role == ROLE_ESCORT) {
        game_ai_uboat();
    } else {
        game_ai_escort();
    }

    game_tick();
    if (snd_ping) { sfx_play(SFX_PING); snd_ping = 0; }
    if (snd_boom) { sfx_play(SFX_BOOM); snd_boom = 0; }
    if (snd_launch) { sfx_play(SFX_LAUNCH); snd_launch = 0; }
    sfx_tick();
    if (game_winner()) {
        stage = ST_OVER;
        set_view(VIEW_MAP);
    }
}

static void hud_over(void)
{
    hud_play();
    hub_set(4, game_winner() == WIN_ESCORT ? "BOAT SUNK" : "CONVOY BROKEN");
    hub_set(5, "ANY KEY");
    bgcolor(COLOR_BLACK);
}

void plat_init(void)
{
    unsigned char i;
    videomode(VIDEOMODE_80x60);
    clear_screen();
    display_init_640x480(0);
    layer_config_tile(0, VRAM_MAP, VRAM_TILES, 1, 1, 16, 16, LAYER_BPP_4);
    save_text_layer();
    gfx_ok = (unsigned char)(load_vram("tiles.bin", VRAM_TILES)
                          && load_vram("pal.bin", VERA_PALETTE_ADDR)
                          && load_vram("marks.bin", VRAM_MARKS));
    {
        unsigned int n;
        if (file_load_to_banks("escort.bin", BANK_ESCORT, BANK_ESCORT + 3, 32768u, &n) != FILE_OK ||
            file_load_to_banks("merchant.bin", BANK_MERCH, BANK_MERCH + 3, 32768u, &n) != FILE_OK ||
            file_load_to_banks("tanker.bin", BANK_TANKER, BANK_TANKER + 3, 32768u, &n) != FILE_OK ||
            file_load_to_banks("uboat.bin", BANK_UBOAT, BANK_UBOAT + 3, 32768u, &n) != FILE_OK ||
            file_load_to_banks("wake.bin", BANK_WAKE, BANK_WAKE + 3, 32768u, &n) != FILE_OK ||
            file_load_to_banks("mid.bin", BANK_MID, BANK_MID + 3, 32768u, &n) != FILE_OK ||
            file_load_to_banks("midwake.bin", BANK_MIDWAKE, BANK_MIDWAKE, 8192u, &n) != FILE_OK ||
            file_load_to_banks("tiny.bin", BANK_TINY, BANK_TINY, 8192u, &n) != FILE_OK ||
            file_load_to_banks("tinywake.bin", BANK_TINYWAKE, BANK_TINYWAKE, 2048u, &n) != FILE_OK ||
            file_load_to_banks("charge.bin", BANK_CHARGE, BANK_CHARGE, 4096u, &n) != FILE_OK ||
            !load_vram("zoom.bin", VRAM_ZOOM) ||
            !load_vram("numbers.bin", VRAM_NUM) ||
            !load_vram("weapons.bin", VRAM_WEAP) ||
            !load_vram("gauges.bin", VRAM_GAUGE) ||
            !load_vram("scope.bin", VRAM_SCOPE) ||
            !load_vram("cross.bin", VRAM_CROSS) ||
            !load_vram("bowmap.bin", VRAM_BOWMAP) ||
            !load_vram("bowesc.bin", VRAM_BOW))
            gfx_ok = 0;
        {
            static const char prof_nm[][12] = {
                "profesc.bin", "profmer.bin", "proftnk.bin", "profubt.bin"
            };
            for (i = 0; i < 4; i++) {
                X16_RAM_BANK = (unsigned char)(BANK_PROF + (unsigned char)(i << 2));
                if (file_load(prof_nm[i], FILE_TO_RAM, 0xA000) != FILE_OK) gfx_ok = 0;
            }
        }
    }
    fill_sea();
    layer_enable_in_dc(0, 1);
    layer_enable_in_dc(1, 1);
    sprite_global_enable(1);
    cache_clear();
    for (i = 0; i < SPR_N; i++) spr_b7[i] = 0xFF;
    hide_sprites();
    audio_init();
    sfx_off();
    link_reset();
    pick = BOAT_VIIC;
    role = ROLE_ESCORT;
    stage = ST_TITLE;
    host[0] = 0;
    hlen = 0;
    {
        const char *d = "192.168.1.20";
        while (d[hlen] && hlen < 22) { host[hlen] = d[hlen]; hlen++; }
        host[hlen] = 0;
    }
    game_init(pick);
    clear_screen();
}

void plat_frame(void)
{
    static unsigned char helm = HELM_MID, uhelm = HELM_MID;
    unsigned char k;
    waitvsync();
    while (kbhit()) {
        k = (unsigned char)cgetc();
        if (stage == ST_TITLE) {
            if (k == '1') begin_match(ROLE_ESCORT, 0);
            else if (k == '2') begin_match(ROLE_UBOAT, 0);
            else if (k == '3') { stage = ST_HOST; clear_screen(); }
            else if (k == 't' || k == 'T') {
                pick = (unsigned char)((pick + 1) % BOAT_N);
                game_set_uboat(pick);
            } else apply_view_key(k);
        } else if (stage == ST_HOST) {
            if (k == 27 || k == 3) { stage = ST_TITLE; clear_screen(); }
            else if (k == 'e' || k == 'E') role = ROLE_ESCORT;
            else if (k == 'u' || k == 'U') role = ROLE_UBOAT;
            else if (k == '\r' || k == '\n') {
                link_begin(host, role == ROLE_UBOAT ? 'U' : 'E');
                stage = ST_DIAL;
            } else if ((k == 20 || k == 8 || k == 127) && hlen) {
                host[--hlen] = 0;
            } else if (((k >= '0' && k <= '9') || k == '.') && hlen < 21) {
                host[hlen++] = (char)k;
                host[hlen] = 0;
            }
        } else if (stage == ST_PLAY) {
            if (view == VIEW_GLASS) {
                /* The glasses only slew the look. The ship's heading stays put. */
                if (k == 'n' || k == 'N' || k == CH_CURS_LEFT || k == 'j' || k == 'J')
                    look_ofs = (unsigned char)(look_ofs - 16);
                else if (k == 'm' || k == 'M' || k == CH_CURS_RIGHT || k == 'l' || k == 'L')
                    look_ofs = (unsigned char)(look_ofs + 16);
            } else if (game_player() == ROLE_ESCORT) {
                if (k == CH_CURS_LEFT) helm = HELM_PORT;
                else if (k == CH_CURS_RIGHT) helm = HELM_STBD;
                if (view == VIEW_BRIDGE && (k == 'n' || k == 'N')) helm = HELM_PORT;
                if (view == VIEW_BRIDGE && (k == 'm' || k == 'M')) helm = HELM_STBD;
            } else {
                if (k == 'j' || k == 'J') uhelm = HELM_PORT;
                else if (k == 'l' || k == 'L') uhelm = HELM_STBD;
                if (view == VIEW_ROOM || view == VIEW_BRIDGE) {
                    if (k == CH_CURS_LEFT || k == 'n' || k == 'N') uhelm = HELM_PORT;
                    else if (k == CH_CURS_RIGHT || k == 'm' || k == 'M') uhelm = HELM_STBD;
                }
            }
            apply_play_key(k);
        } else if (stage == ST_OVER) {
            stage = ST_TITLE;
            link_reset();
            game_init(pick);
            hub_forget();
            clear_screen();
        }
    }

    if (stage == ST_TITLE) {
        draw_world();
        title_screen();
        draw_scopes();
        return;
    }
    if (stage == ST_HOST) {
        draw_world();
        host_screen();
        draw_scopes();
        return;
    }
    if (stage == ST_DIAL) {
        unsigned char side, data[4];
        link_pump(&side, data);
        draw_world();
        hub_row(16);
        cputs(link_status());
        bgcolor(COLOR_BLACK);
        draw_scopes();
        if (link_state() == LINK_UP) begin_match(role, 1);
        else if (link_state() == LINK_FAIL) {
            hub_row(18);
            cputs("NO CARRIER");
            hub_row(19);
            cputs("ANY KEY");
            bgcolor(COLOR_BLACK);
            if (kbhit()) { cgetc(); stage = ST_HOST; }
        }
        return;
    }

    step++;
    draw_world();
    if (stage == ST_OVER) { hud_over(); draw_scopes(); return; }
    /* Original helm rate: one tick every three blanks. Ship speed stays at
     * half the first demo, so a faster tick does not restore the old knots. */
    if ((step % 3) == 0) {
        play_tick(helm, uhelm);
        helm = HELM_MID;
        uhelm = HELM_MID;
    }
    if (stage == ST_OVER) hud_over();
    else if (view == VIEW_MAP) hud_play();
    else if (view == VIEW_GLASS) glass_bearing();
    if (view == VIEW_MAP || stage == ST_OVER) draw_scopes();
    {
        unsigned int f = drone_f[ships[selected].tel];
        if (!f || game_player() != ROLE_ESCORT) psg_stop_voice(0);
        else psg_set_voice(0, f, 28, PSG_WAVE_TRIANGLE, 32);
    }
}

#if CONVOY_SMOKE
static void smoke_puts(const char *m)
{
    for (; *m; ++m)
        cbm_k_chrout((unsigned char)*m);
}

void plat_smoke(void)
{
    int y0 = ships[0].y;
    int my = ships[4].y;
    int uy = ships[N_SHIP - 1].y;
    unsigned char i, n;
    unsigned char moved, hold, drift, fired, attack;
    int sea;
    game_telegraph_delta(2);
    for (i = 0; i < 40; i++) {
        game_helm(HELM_MID);
        game_tick();
    }
    moved = (unsigned char)(ships[0].y < y0);
    hold = (unsigned char)(ships[4].y == my);
    drift = (unsigned char)(ships[N_SHIP - 1].y > uy);
    sea = scroll_y;

    /* Same opponents the title uses, so a scripted run has to see a fish
     * leave the tubes and an escort go to attack. */
    game_init(BOAT_VIIC);
    ships[N_SHIP - 1].x = ships[4].x + 20 * FP;
    ships[N_SHIP - 1].y = ships[4].y + 80 * FP;
    game_set_player(ROLE_ESCORT);
    for (i = 0; i < 160; i++) {
        game_ai_uboat();
        game_tick();
    }
    fired = (unsigned char)(game_tubes() < 4);

    game_init(BOAT_VIIC);
    ships[N_SHIP - 1].x = ships[0].x;
    ships[N_SHIP - 1].y = ships[0].y - 70 * FP;
    game_set_player(ROLE_UBOAT);
    attack = 0;
    for (i = 0; i < 80; i++) {
        game_ai_escort();
        game_tick();
        for (n = 0; n < N_ESCORT; n++)
            if (game_order_of(n) == ORD_ATTACK) attack = 1;
    }

    smoke_puts("convoy smoke load=");
    cbm_k_chrout((unsigned char)('0' + (gfx_ok ? 1 : 0)));
    smoke_puts(" moved=");
    cbm_k_chrout((unsigned char)('0' + moved));
    smoke_puts(" hold=");
    cbm_k_chrout((unsigned char)('0' + hold));
    smoke_puts(" drift=");
    cbm_k_chrout((unsigned char)('0' + drift));
    smoke_puts(" fired=");
    cbm_k_chrout((unsigned char)('0' + fired));
    smoke_puts(" attack=");
    cbm_k_chrout((unsigned char)('0' + attack));
    cbm_k_chrout(13);
    if (gfx_ok && moved && hold && drift && sea == -40 && fired && attack)
        smoke_puts("convoy smoke ok\r");
    else
        smoke_puts("convoy smoke fail\r");
}
#endif
