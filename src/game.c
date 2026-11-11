#include "game.h"

ship_t ships[N_SHIP];
unsigned char selected;
unsigned char uboat_type;
int scroll_x, scroll_y;

static unsigned char helm_e, helm_u;

/* Sine, 32 steps, 16 = 1.0. Index 0 is east-of-north, used as sin(heading). */
static const int sdir[32] = {
    0, 3, 6, 9, 12, 14, 15, 16,
    16, 16, 15, 14, 12, 9, 6, 3,
    0, -3, -6, -9, -12, -14, -15, -16,
    -16, -16, -15, -14, -12, -9, -6, -3
};

static const int tel_fp[5] = { 0, 1 * FP, 2 * FP, 3 * FP, 4 * FP };

#define CENTER_X (320 * FP)
#define CENTER_Y (200 * FP)

static const int st_x[N_ESCORT] = { 0, -72 * FP, 72 * FP, 0 };
static const int st_y[N_ESCORT] = { -88 * FP, 0, 0, 88 * FP };
static const int form_x[N_MERCH] = { -28 * FP, -28 * FP, -28 * FP, 28 * FP, 28 * FP, 28 * FP };
static const int form_y[N_MERCH] = { -36 * FP, 0, 36 * FP, -36 * FP, 0, 36 * FP };

typedef struct {
    const char *name;
    unsigned char sprite;
    unsigned char surface_kn;
    unsigned char submerged_kn;
    unsigned char bow;
    unsigned char stern;
    unsigned char gun;
} boat_info;

static const boat_info boats[BOAT_N] = {
    { "TYPE II", SPR_II,   13,  7, 3, 0, 0 },
    { "VIIC",    SPR_VIIC, 17,  7, 4, 1, 1 },
    { "IXC",     SPR_IXC,  18,  7, 4, 2, 1 },
    { "XXI",     SPR_XXI,  15, 17, 6, 0, 0 }
};

const char *boat_name(unsigned char type) { return boats[type].name; }
unsigned char boat_surface(unsigned char type) { return boats[type].surface_kn; }
unsigned char boat_submerged(unsigned char type) { return boats[type].submerged_kn; }
unsigned char boat_bow(unsigned char type) { return boats[type].bow; }
unsigned char boat_stern(unsigned char type) { return boats[type].stern; }
unsigned char boat_gun(unsigned char type) { return boats[type].gun; }
unsigned char boat_sprite(unsigned char type) { return boats[type].sprite; }

unsigned char ship_sprite(unsigned char i)
{
    if (ships[i].kind == SHIP_ESCORT) return SPR_ESCORT;
    if (ships[i].kind == SHIP_TANKER) return SPR_TANKER;
    if (ships[i].kind == SHIP_UBOAT) return boat_sprite(uboat_type);
    return SPR_MERCH;
}

void game_station(unsigned char escort, int *x, int *y)
{
    *x = CENTER_X + st_x[escort];
    *y = CENTER_Y + st_y[escort];
}

static void vel(unsigned char hdg, unsigned char tel, int *vx, int *vy)
{
    int s = tel_fp[tel];
    *vx = s * sdir[hdg >> 3] / 16;
    *vy = -(s * sdir[((hdg + 64) & 255) >> 3] / 16);
}

static unsigned char heading_to(int dx, int dy)
{
    int ax = dx < 0 ? -dx : dx;
    int ay = dy < 0 ? -dy : dy;
    if (ay * 2 < ax) return dx > 0 ? 64 : 192;
    if (ax * 2 < ay) return dy > 0 ? 128 : 0;
    if (dx >= 0 && dy < 0) return 32;
    if (dx >= 0 && dy >= 0) return 96;
    if (dx < 0 && dy >= 0) return 160;
    return 224;
}

static unsigned char turn_toward(unsigned char from, unsigned char to)
{
    int d = (int)to - (int)from;
    if (d > 128) d -= 256;
    if (d < -128) d += 256;
    if (d > 8) d = 8;
    if (d < -8) d = -8;
    return (unsigned char)(from + d);
}

static void clamp_ship(ship_t *s)
{
    if (s->x < -64 * FP) s->x = -64 * FP;
    if (s->x > 704 * FP) s->x = 704 * FP;
    if (s->y < -32 * FP) s->y = -32 * FP;
    if (s->y > 512 * FP) s->y = 512 * FP;
}

void game_init(unsigned char uboat)
{
    unsigned char i;
    uboat_type = uboat < BOAT_N ? uboat : BOAT_VIIC;
    selected = 0;
    helm_e = HELM_MID;
    helm_u = HELM_MID;
    scroll_x = 0;
    scroll_y = 0;
    for (i = 0; i < N_ESCORT; i++) {
        ships[i].kind = SHIP_ESCORT;
        ships[i].hdg = 0;
        ships[i].tel = TEL_HALF;
        game_station(i, &ships[i].x, &ships[i].y);
    }
    for (i = 0; i < N_MERCH; i++) {
        ships[N_ESCORT + i].kind = (i == N_MERCH - 1) ? SHIP_TANKER : SHIP_MERCHANT;
        ships[N_ESCORT + i].hdg = 0;
        ships[N_ESCORT + i].tel = TEL_HALF;
        ships[N_ESCORT + i].x = CENTER_X + form_x[i];
        ships[N_ESCORT + i].y = CENTER_Y + form_y[i];
    }
    ships[N_SHIP - 1].kind = SHIP_UBOAT;
    ships[N_SHIP - 1].hdg = 0;
    ships[N_SHIP - 1].tel = TEL_STOP;
    ships[N_SHIP - 1].x = CENTER_X;
    ships[N_SHIP - 1].y = CENTER_Y - 120 * FP;
}

void game_set_uboat(unsigned char uboat)
{
    uboat_type = uboat < BOAT_N ? uboat : BOAT_VIIC;
}

void game_select(unsigned char escort)
{
    if (escort < N_ESCORT) selected = escort;
}

void game_helm(unsigned char helm) { helm_e = helm; }
void game_uboat_helm(unsigned char helm) { helm_u = helm; }

static void bump_tel(unsigned char *tel, signed char d)
{
    int t = (int)*tel + d;
    if (t < TEL_STOP) t = TEL_STOP;
    if (t > TEL_FLANK) t = TEL_FLANK;
    *tel = (unsigned char)t;
}

void game_telegraph_delta(signed char d) { bump_tel(&ships[selected].tel, d); }
void game_uboat_telegraph_delta(signed char d) { bump_tel(&ships[N_SHIP - 1].tel, d); }

void game_tick(void)
{
    unsigned char i;
    int cvx, cvy, vx, vy;
    if (helm_e == HELM_PORT) ships[selected].hdg = (unsigned char)(ships[selected].hdg - 4);
    else if (helm_e == HELM_STBD) ships[selected].hdg = (unsigned char)(ships[selected].hdg + 4);
    if (helm_u == HELM_PORT) ships[N_SHIP - 1].hdg = (unsigned char)(ships[N_SHIP - 1].hdg - 4);
    else if (helm_u == HELM_STBD) ships[N_SHIP - 1].hdg = (unsigned char)(ships[N_SHIP - 1].hdg + 4);
    helm_e = HELM_MID;
    helm_u = HELM_MID;

    for (i = 0; i < N_ESCORT; i++) {
        int dx, dy, sx, sy;
        if (i == selected) continue;
        game_station(i, &sx, &sy);
        dx = sx - ships[i].x;
        dy = sy - ships[i].y;
        if (dx > -8 * FP && dx < 8 * FP && dy > -8 * FP && dy < 8 * FP) {
            ships[i].hdg = 0;
            ships[i].tel = TEL_HALF;
        } else {
            ships[i].hdg = turn_toward(ships[i].hdg, heading_to(dx, dy));
            ships[i].tel = TEL_FULL;
        }
    }
    for (i = 0; i < N_MERCH; i++) {
        ships[N_ESCORT + i].hdg = 0;
        ships[N_ESCORT + i].tel = TEL_HALF;
    }

    vel(0, TEL_HALF, &cvx, &cvy);
    scroll_x += cvx / FP;
    scroll_y += cvy / FP;
    for (i = 0; i < N_SHIP; i++) {
        vel(ships[i].hdg, ships[i].tel, &vx, &vy);
        ships[i].x += vx - cvx;
        ships[i].y += vy - cvy;
        clamp_ship(&ships[i]);
    }
}

unsigned int game_checksum(void)
{
    unsigned int s = uboat_type;
    unsigned char i;
    for (i = 0; i < N_SHIP; i++) {
        s = s * 33u + (unsigned int)ships[i].x;
        s = s * 33u + (unsigned int)ships[i].y;
        s = s * 33u + ships[i].hdg;
        s = s * 33u + ships[i].tel;
    }
    s = s * 33u + (unsigned int)scroll_x;
    s = s * 33u + (unsigned int)scroll_y;
    return s;
}
