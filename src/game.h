#ifndef GAME_H
#define GAME_H

/* Plot sim. Positions are signed 16-bit, in 1/16 of a pixel (FP).
 * The screen is the battle space. Each tick subtracts the convoy's
 * velocity, so a ship steaming with the convoy stays put and the sea scrolls.
 * cc65 int is 16-bit; every value in here is kept inside that range. */

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
#define N_SHIP   11          /* 4 escorts, 5 merchants, 1 tanker, 1 U-boat */

#define TEL_STOP  0
#define TEL_SLOW  1
#define TEL_HALF  2
#define TEL_FULL  3
#define TEL_FLANK 4

#define HELM_PORT 0
#define HELM_MID  1
#define HELM_STBD 2

#define FP 16

#define SPR_ESCORT 0
#define SPR_MERCH  1
#define SPR_TANKER 2
#define SPR_II     3
#define SPR_VIIC   4
#define SPR_IXC    5
#define SPR_XXI    6
#define SPR_SELECT 7

typedef struct {
    int x, y;
    unsigned char hdg;     /* 0 = north, clockwise, 256 steps */
    unsigned char tel;
    unsigned char kind;
} ship_t;

extern ship_t ships[N_SHIP];
extern unsigned char selected;     /* escort 0..3 under the conn */
extern unsigned char uboat_type;
extern int scroll_x, scroll_y;     /* sea scroll, pixels */

void game_init(unsigned char uboat);
void game_set_uboat(unsigned char uboat);
void game_select(unsigned char escort);
void game_helm(unsigned char helm);
void game_telegraph_delta(signed char d);
void game_uboat_helm(unsigned char helm);
void game_uboat_telegraph_delta(signed char d);
void game_tick(void);
void game_station(unsigned char escort, int *x, int *y);
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
