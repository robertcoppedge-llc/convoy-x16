#ifndef GAME_H
#define GAME_H

/* Plot sim. Positions are signed 16-bit, in 1/16 of a pixel (FP).
 * The screen is the battle space. Each tick subtracts the convoy's
 * velocity, so a ship steaming with the convoy stays put and the sea scrolls.
 * cc65 int is 16-bit; every value in here is kept inside that range.
 *
 * The escort who has the conn takes the wheel. The other escorts carry an
 * order: patrol a station, hunt the last contact, or attack it.
 * Wolfpack orders for several U-boats are a later scenario. One boat here. */

#define BOAT_II    0
#define BOAT_VIIC  1
#define BOAT_IXC   2
#define BOAT_XXI   3
#define BOAT_N     4

#define SHIP_ESCORT   0
#define SHIP_MERCHANT 1
#define SHIP_TANKER   2
#define SHIP_UBOAT    3

#define N_ESCORT 4
#define N_MERCH  6
#define N_SHIP   11

#define TEL_STOP  0
#define TEL_SLOW  1
#define TEL_HALF  2
#define TEL_FULL  3
#define TEL_FLANK 4
#define TEL_DEAD  5
#define TEL_ASTERN 6
#define TEL_FULL_ASTERN 7

#define HELM_PORT 0
#define HELM_MID  1
#define HELM_STBD 2

#define ORD_PATROL 0
#define ORD_HUNT   1
#define ORD_ATTACK 2

#define ROLE_ESCORT 0
#define ROLE_UBOAT  1

#define WIN_NONE   0
#define WIN_ESCORT 1
#define WIN_UBOAT  2

#define TONNAGE_WIN 4
#define N_TORP 4
#define N_DC   4

#define FP 16

#define SPR_ESCORT 0
#define SPR_MERCH  1
#define SPR_TANKER 2
#define SPR_II     3
#define SPR_VIIC   4
#define SPR_IXC    5
#define SPR_XXI    6
#define SPR_SELECT 7
#define SPR_EEL    8
#define SPR_BURST  9

typedef struct {
    int x, y;
    unsigned char hdg;     /* 0 = north, clockwise, 256 steps */
    unsigned char tel;
    unsigned char kind;
} ship_t;

extern ship_t ships[N_SHIP];
extern unsigned char selected;
extern unsigned char uboat_type;
extern int scroll_x, scroll_y;
extern unsigned char snd_ping, snd_boom, snd_launch;

void game_init(unsigned char uboat);
void game_set_uboat(unsigned char uboat);
void game_set_player(unsigned char role);
void game_select(unsigned char escort);
void game_cycle(signed char dir);
void game_order(unsigned char ord);
void game_helm(unsigned char helm);
void game_telegraph_delta(signed char d);
void game_uboat_helm(unsigned char helm);
void game_uboat_set_tel(unsigned char tel);
void game_uboat_telegraph_delta(signed char d);
void game_planes(signed char d);
void game_set_planes(signed char p);
unsigned char game_depth(void);
unsigned char game_battery(void);
signed char game_planes_of(void);
void game_ping(void);
void game_gun(void);
void game_drop(void);
void game_fire(void);
/* Solo opponents. Escort: hold station, ping up the bow, then the nearest
 * attacks and the next hunts. U-boat: slide onto a column, fire once, run. */
void game_ai_escort(void);
void game_ai_uboat(void);
void game_tick(void);

void game_station(unsigned char escort, int *x, int *y);
unsigned char game_order_of(unsigned char escort);
unsigned char game_player(void);
unsigned char game_alive(unsigned char i);
unsigned char game_tubes(void);
unsigned char game_hits(void);
unsigned char game_tonnage(void);
unsigned char game_winner(void);
unsigned char game_hear(unsigned int *deg);
unsigned char game_hear_xy(int *dx, int *dy);
unsigned char game_ping_who(void);
unsigned char game_show_uboat(void);
unsigned char game_show_ship(unsigned char i);
unsigned char game_show_contact(int *x, int *y);
void game_water(unsigned char i, int *vx, int *vy);
unsigned char game_wake_at(unsigned char n, int *x, int *y);
unsigned char game_torp_on(unsigned char i);
void game_torp_xy(unsigned char i, int *x, int *y);
unsigned char game_torp_hdg(unsigned char i);
unsigned char game_dc_on(unsigned char i);
void game_dc_xy(unsigned char i, int *x, int *y);
unsigned char game_dc_life(unsigned char i);
unsigned char game_dc_hdg(unsigned char i);
unsigned char game_shell_on(void);
void game_shell_xy(int *x, int *y);
unsigned char game_shell_hdg(void);
unsigned char game_watch(unsigned int *deg);

unsigned char game_export_escort(unsigned char seq, unsigned char *b);
unsigned char game_export_uboat(unsigned char seq, unsigned char *b);
unsigned char game_import_escort(const unsigned char *b);
unsigned char game_import_uboat(const unsigned char *b);

unsigned int game_checksum(void);

const char *boat_name(unsigned char type);
unsigned char boat_surface(unsigned char type);
unsigned char boat_submerged(unsigned char type);
unsigned char boat_bow(unsigned char type);
unsigned char boat_stern(unsigned char type);
unsigned char boat_gun(unsigned char type);
unsigned char boat_sprite(unsigned char type);
unsigned char ship_sprite(unsigned char i);

#endif
