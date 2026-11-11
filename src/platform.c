/*
 * 640x480 plot. Layer 0 is 16x16 4bpp sea tiles. Layer 1 is the KERNAL text.
 * Sprites sit between the layers. The link is the netcard UART (link.c).
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
#include "link.h"
#include "platform.h"

#define VRAM_TILES  0x00000UL
#define VRAM_MAP    0x00400UL
#define VRAM_SPR    0x04000UL
#define MAP_TILES   64
#define SPR_BYTES   128
#define IDX_SPR0    ((unsigned int)(VRAM_SPR >> 5))
#define SPR_SIZE_16 0x50
#define SPR_Z       0x08

#define ST_TITLE 0
#define ST_HOST  1
#define ST_DIAL  2
#define ST_PLAY  3
#define ST_OVER  4

static const unsigned int drone_f[5] = { 0, 180, 360, 620, 980 };
static const char *tel_name[5] = { "STOP", "SLOW", "HALF", "FULL", "FLANK" };
static const char ord_ch[3] = { 'P', 'H', 'A' };

static unsigned char gfx_ok;
static unsigned char stage, net, role, pick;
static unsigned char seq, ready, echoed, desync, step;
static char host[24];
static unsigned char hlen;

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

static void draw_at(unsigned char id, unsigned char image, int x, int y)
{
    int px = x / FP - 8;
    int py = y / FP - 8;
    unsigned char show = (unsigned char)(px >= 0 && py >= 0 && px < 640 && py < 480);
    write_sprite(id, (unsigned int)(IDX_SPR0 + (unsigned int)image * (SPR_BYTES / 32)), px, py, show);
}

static void hide_from(unsigned char id)
{
    unsigned char i;
    for (i = id; i < 28; i++) write_sprite(i, IDX_SPR0, 0, 0, 0);
}

static void draw_world(void)
{
    unsigned char i;
    int x, y;
    unsigned char spr = 0;
    for (i = 0; i < N_SHIP; i++) {
        if (!game_show_ship(i)) continue;
        draw_at(spr++, ship_sprite(i), ships[i].x, ships[i].y);
    }
    if (game_player() == ROLE_ESCORT && game_alive(selected))
        draw_at(spr++, SPR_SELECT, ships[selected].x, ships[selected].y);
    if (game_show_contact(&x, &y))
        draw_at(spr++, SPR_BURST, x, y);
    for (i = 0; i < N_TORP; i++) {
        if (!game_torp_on(i)) continue;
        game_torp_xy(i, &x, &y);
        draw_at(spr++, SPR_EEL, x, y);
    }
    for (i = 0; i < N_DC; i++) {
        if (!game_dc_on(i)) continue;
        game_dc_xy(i, &x, &y);
        draw_at(spr++, SPR_BURST, x, y);
    }
    hide_from(spr);
}

static void line_clear(unsigned char y)
{
    cclearxy(0, y, 80);
}

static void hud_play(void)
{
    unsigned char i;
    unsigned int deg = (unsigned int)((unsigned long)ships[selected].hdg * 360ul / 256ul);
    unsigned int heard;
    textcolor(COLOR_WHITE);
    gotoxy(0, 0);
    for (i = 0; i < N_ESCORT; i++) {
        if (i == selected) cputc('*');
        else cputc(' ');
        cputc((char)('1' + i));
        cputc(game_alive(i) ? ord_ch[game_order_of(i)] : 'x');
    }
    cprintf("  %s %3u  %s", tel_name[ships[selected].tel], deg, boat_name(uboat_type));
    cclear(6);
    gotoxy(0, 1);
    if (game_hear(&heard)) cprintf("HEAR %3u  ", heard);
    else cputs("QUIET     ");
    cprintf("TUBES %u  HITS %u  TON %u/%u",
            (unsigned)game_tubes(), (unsigned)game_hits(),
            (unsigned)game_tonnage(), (unsigned)TONNAGE_WIN);
    if (desync) cputs("  DESYNC");
    cclear(4);
    gotoxy(0, 2);
    if (game_player() == ROLE_ESCORT)
        cputs("[] CYCLE  P PATROL  H HUNT  A ATTACK  S PING  D CHARGES  ARROWS");
    else
        cputs("U-BOAT   J L HELM   I K TELEGRAPH   F FIRE");
    cclear(4);
}

static void begin_match(unsigned char as, unsigned char linked)
{
    unsigned char y;
    role = as;
    net = linked;
    ready = (unsigned char)!linked;
    echoed = 0;
    desync = 0;
    seq = 0;
    step = 0;
    game_init(pick);
    game_set_player(as);
    for (y = 8; y < 28; y++) line_clear(y);
    stage = ST_PLAY;
}

static void title_screen(void)
{
    textcolor(COLOR_WHITE);
    cputsxy(28, 12, "CONVOY");
    cputsxy(16, 15, "1  ESCORT AGAINST THE BOAT");
    cputsxy(16, 16, "2  U-BOAT AGAINST THE ESCORT");
    cputsxy(16, 17, "3  LINK THE OTHER MACHINE");
    gotoxy(16, 19);
    cprintf("BOAT %s   T CHANGES IT", boat_name(pick));
    cclear(8);
    cputsxy(16, 21, "P PATROL   H HUNT   A ATTACK");
}

static void host_screen(void)
{
    textcolor(COLOR_WHITE);
    cputsxy(12, 14, "E  THIS SIDE IS THE ESCORT");
    cputsxy(12, 15, "U  THIS SIDE IS THE U-BOAT");
    gotoxy(12, 17);
    cprintf("ADDRESS %s", host);
    cclear(10);
    cputsxy(12, 19, "TYPE THE ADDRESS   ENTER DIALS   ESC BACK");
    cputsxy(12, 21, "ON THAT PC:  PYTHON TOOLS\\RELAY.PY");
    gotoxy(12, 22);
    cputs(role == ROLE_UBOAT ? "SIDE U-BOAT" : "SIDE ESCORT");
    cclear(8);
}

static void apply_play_key(unsigned char k)
{
    if (game_player() == ROLE_ESCORT) {
        if (k == '[' || k == ',') game_cycle(-1);
        else if (k == ']' || k == '.' || k == 9) game_cycle(1);
        else if (k >= '1' && k <= '4') game_select((unsigned char)(k - '1'));
        else if (k == 'p' || k == 'P' || k == 'o' || k == 'O') game_order(ORD_PATROL);
        else if (k == 'h' || k == 'H') game_order(ORD_HUNT);
        else if (k == 'a' || k == 'A') game_order(ORD_ATTACK);
        else if (k == 's' || k == 'S') game_ping();
        else if (k == 'd' || k == 'D') game_drop();
        else if (k == CH_CURS_UP) game_telegraph_delta(1);
        else if (k == CH_CURS_DOWN) game_telegraph_delta(-1);
    } else {
        if (k == 'i' || k == 'I') game_uboat_telegraph_delta(1);
        else if (k == 'k' || k == 'K') game_uboat_telegraph_delta(-1);
        else if (k == 'f' || k == 'F' || k == ' ') game_fire();
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
            gotoxy(0, 3);
            cputs(ready ? "WAITING FOR ORDERS" : "WAITING FOR THE OTHER SIDE");
            cclear(6);
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
    sfx_tick();
    if (game_winner()) stage = ST_OVER;
}

static void hud_over(void)
{
    hud_play();
    gotoxy(0, 3);
    if (game_winner() == WIN_ESCORT) cputs("BOAT SUNK.  ESCORTS WIN.   ANY KEY");
    else cputs("CONVOY BROKEN.  U-BOAT WINS.   ANY KEY");
    cclear(4);
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
    for (i = 0; i < 28; i++)
        write_sprite(i, IDX_SPR0, 0, 0, 0);
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
    cursor(0);
}

void plat_frame(void)
{
    unsigned char helm = HELM_MID, uhelm = HELM_MID;
    unsigned char k;
    waitvsync();
    while (kbhit()) {
        k = (unsigned char)cgetc();
        if (stage == ST_TITLE) {
            if (k == '1') begin_match(ROLE_ESCORT, 0);
            else if (k == '2') begin_match(ROLE_UBOAT, 0);
            else if (k == '3') { stage = ST_HOST; line_clear(12); line_clear(15); line_clear(16); line_clear(17); line_clear(19); line_clear(21); }
            else if (k == 't' || k == 'T') {
                pick = (unsigned char)((pick + 1) % BOAT_N);
                game_set_uboat(pick);
            }
        } else if (stage == ST_HOST) {
            if (k == 27 || k == 3) { stage = ST_TITLE; }
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
            if (game_player() == ROLE_ESCORT) {
                if (k == CH_CURS_LEFT) helm = HELM_PORT;
                else if (k == CH_CURS_RIGHT) helm = HELM_STBD;
            } else {
                if (k == 'j' || k == 'J') uhelm = HELM_PORT;
                else if (k == 'l' || k == 'L') uhelm = HELM_STBD;
            }
            apply_play_key(k);
        } else if (stage == ST_OVER) {
            stage = ST_TITLE;
            link_reset();
            game_init(pick);
        }
    }

    if (stage == ST_TITLE) {
        draw_world();
        title_screen();
        return;
    }
    if (stage == ST_HOST) {
        draw_world();
        host_screen();
        return;
    }
    if (stage == ST_DIAL) {
        unsigned char side, data[4];
        link_pump(&side, data);
        draw_world();
        gotoxy(20, 16);
        cputs(link_status());
        cclear(10);
        if (link_state() == LINK_UP) begin_match(role, 1);
        else if (link_state() == LINK_FAIL) {
            gotoxy(16, 18);
            cputs("NO CARRIER   ANY KEY");
            if (kbhit()) { cgetc(); stage = ST_HOST; }
        }
        return;
    }

    step++;
    draw_world();
    layer_scroll(0, (unsigned int)scroll_x, (unsigned int)scroll_y);
    if (stage == ST_OVER) { hud_over(); return; }
    /* Same cadence solo and linked, so a stopped boat takes a few seconds
     * to drift into view instead of arriving on the first ping of the clock. */
    if ((step % 8) == 0) play_tick(helm, uhelm);
    else hud_play();
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
    cbm_k_chrout((unsigned char)('0' + (gfx_ok ? 1 : 0)));
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
