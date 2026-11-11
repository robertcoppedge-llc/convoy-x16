/*
 * 640x480 plot. Layer 0 is 16x16 4bpp sea tiles. Layer 1 is the KERNAL
 * text screen (map at $1B000, left alone). Sprites sit between the layers.
 * Art and the bell come from pixel-cli / sound-cli exports in boot\.
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
#include <x16lib/layers.h>
#include <x16lib/vera.h>
#include "game.h"
#include "platform.h"

#define VRAM_TILES  0x00000UL
#define VRAM_MAP    0x00400UL
#define VRAM_SPR    0x04000UL
#define MAP_TILES   64
#define SPR_BYTES   128
#define IDX_SPR0    ((unsigned int)(VRAM_SPR >> 5))
#define SPR_SIZE_16 0x50
#define SPR_Z       0x08          /* between layer 0 and the text layer */

static const unsigned int drone_f[5] = { 0, 180, 360, 620, 980 };
static const char *tel_name[5] = { "STOP", "SLOW", "HALF", "FULL", "FLANK" };
static unsigned char gfx_ok;
static unsigned char prev_tel, prev_utel, prev_helm, prev_uhelm;

#define load_vram(name, addr) ((unsigned char)(file_load((name), FILE_TO_VRAM(addr), (unsigned int)(addr)) == FILE_OK))

static void write_sprite(unsigned char id, unsigned int idx, int px, int py, unsigned char show)
{
    VERA.control = 0;
    vera_set_addr16((unsigned int)(VERA_SPRATTR_ADDR + (unsigned int)id * 8u),
                    (unsigned char)(0x10 | (unsigned char)(VERA_SPRATTR_ADDR >> 16)));
    VERA.data0 = (unsigned char)(idx & 0xFF);
    VERA.data0 = (unsigned char)((idx >> 8) & 0x0F);
    VERA.data0 = (unsigned char)(px & 0xFF);
    VERA.data0 = (unsigned char)((px >> 8) & 0x03);
    VERA.data0 = (unsigned char)(py & 0xFF);
    VERA.data0 = (unsigned char)((py >> 8) & 0x03);
    VERA.data0 = show ? SPR_Z : 0;
    VERA.data0 = SPR_SIZE_16;
}

static void fill_sea(void)
{
    unsigned char x, y;
    vera_set_addr(VRAM_MAP);
    for (y = 0; y < MAP_TILES; y++) {
        for (x = 0; x < MAP_TILES; x++) {
            VERA.data0 = (unsigned char)((x + y) & 3);
            VERA.data0 = 0;
        }
    }
}

static void draw_ship(unsigned char id, unsigned char image, int x, int y)
{
    int px = x / FP - 8;
    int py = y / FP - 8;
    unsigned char show = (unsigned char)(px >= 0 && py >= 0 && px < 640 && py < 480);
    write_sprite(id, (unsigned int)(IDX_SPR0 + (unsigned int)image * (SPR_BYTES / 32)), px, py, show);
}

static void hud(void)
{
    unsigned int deg = (unsigned int)((unsigned long)ships[selected].hdg * 360ul / 256ul);
    unsigned int udeg = (unsigned int)((unsigned long)ships[N_SHIP - 1].hdg * 360ul / 256ul);
    textcolor(COLOR_WHITE);
    gotoxy(0, 0);
    cprintf("escort %u  %s  hdg %3u   %s  surf %u  sub %u",
            (unsigned)selected + 1, tel_name[ships[selected].tel], deg,
            boat_name(uboat_type), (unsigned)boat_surface(uboat_type),
            (unsigned)boat_submerged(uboat_type));
    cclear(8);
    gotoxy(0, 1);
    cprintf("u-boat %s  hdg %3u    1-4 conn  t type  arrows helm  ijkl boat",
            tel_name[ships[N_SHIP - 1].tel], udeg);
    cclear(8);
}

static void drone(void)
{
    unsigned int f = drone_f[ships[selected].tel];
    if (!f)
        psg_stop_voice(0);
    else
        psg_set_voice(0, f, 28, PSG_WAVE_TRIANGLE, 32);
}

static void apply_key(unsigned char k)
{
    if (k == CH_CURS_LEFT) game_helm(HELM_PORT);
    else if (k == CH_CURS_RIGHT) game_helm(HELM_STBD);
    else if (k == CH_CURS_UP) game_telegraph_delta(1);
    else if (k == CH_CURS_DOWN) game_telegraph_delta(-1);
    else if (k == 'j' || k == 'J') game_uboat_helm(HELM_PORT);
    else if (k == 'l' || k == 'L') game_uboat_helm(HELM_STBD);
    else if (k == 'i' || k == 'I') game_uboat_telegraph_delta(1);
    else if (k == 'k' || k == 'K') game_uboat_telegraph_delta(-1);
    else if (k >= '1' && k <= '4') game_select((unsigned char)(k - '1'));
    else if (k == 't' || k == 'T')
        game_set_uboat((unsigned char)((uboat_type + 1) % BOAT_N));
}

void plat_init(void)
{
    unsigned char i;
    display_init_640x480(6);
    layer_config_tile(0, VRAM_MAP, VRAM_TILES, 1, 1, 16, 16, LAYER_BPP_4);
    gfx_ok = (unsigned char)(load_vram("tiles.bin", VRAM_TILES)
                          && load_vram("sprites.bin", VRAM_SPR)
                          && load_vram("pal.bin", VERA_PALETTE_ADDR));
    fill_sea();
    layer_enable_in_dc(0, 1);
    layer_enable_in_dc(1, 1);
    VERA.control = 0;
    VERA_DC_VIDEO = (unsigned char)(VERA_DC_VIDEO | VERA_VIDEO_SPRITES);
    for (i = 0; i < 16; i++)
        write_sprite(i, IDX_SPR0, 0, 0, 0);
    audio_init();
    sfx_off();
    game_init(BOAT_VIIC);
    prev_tel = ships[0].tel;
    prev_utel = ships[N_SHIP - 1].tel;
    prev_helm = HELM_MID;
    prev_uhelm = HELM_MID;
    cursor(0);
    hud();
}

void plat_frame(void)
{
    unsigned char helm = HELM_MID, uhelm = HELM_MID;
    waitvsync();
    while (kbhit()) {
        unsigned char k = (unsigned char)cgetc();
        if (k == CH_CURS_LEFT) helm = HELM_PORT;
        else if (k == CH_CURS_RIGHT) helm = HELM_STBD;
        else if (k == 'j' || k == 'J') uhelm = HELM_PORT;
        else if (k == 'l' || k == 'L') uhelm = HELM_STBD;
        else apply_key(k);
    }
    game_helm(helm);
    game_uboat_helm(uhelm);
    if ((helm != HELM_MID && helm != prev_helm) || (uhelm != HELM_MID && uhelm != prev_uhelm))
        sfx_play(SFX_HELM);
    prev_helm = helm;
    prev_uhelm = uhelm;
    game_tick();
    if (ships[selected].tel != prev_tel || ships[N_SHIP - 1].tel != prev_utel)
        sfx_play(SFX_BELL);
    prev_tel = ships[selected].tel;
    prev_utel = ships[N_SHIP - 1].tel;
    sfx_tick();
    drone();
    {
        unsigned char i;
        for (i = 0; i < N_SHIP; i++)
            draw_ship(i, ship_sprite(i), ships[i].x, ships[i].y);
        draw_ship(N_SHIP, SPR_SELECT, ships[selected].x, ships[selected].y);
    }
    layer_scroll(0, (unsigned int)scroll_x, (unsigned int)scroll_y);
    hud();
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
    unsigned char i;
    unsigned char moved, hold, drift;
    game_telegraph_delta(2);
    for (i = 0; i < 40; i++) {
        game_helm(HELM_MID);
        game_tick();
    }
    moved = (unsigned char)(ships[0].y < y0);
    hold = (unsigned char)(ships[4].y == my);
    drift = (unsigned char)(ships[N_SHIP - 1].y > uy);
    smoke_puts("convoy smoke load=");
    cbm_k_chrout((unsigned char)('0' + gfx_ok));
    smoke_puts(" moved=");
    cbm_k_chrout((unsigned char)('0' + moved));
    smoke_puts(" hold=");
    cbm_k_chrout((unsigned char)('0' + hold));
    smoke_puts(" drift=");
    cbm_k_chrout((unsigned char)('0' + drift));
    cbm_k_chrout(13);
    if (gfx_ok && moved && hold && drift && scroll_y == -80)
        smoke_puts("convoy smoke ok\r");
    else
        smoke_puts("convoy smoke fail\r");
}
#endif
