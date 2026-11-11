#include "game.h"

ship_t ships[N_SHIP];
unsigned char selected;
unsigned char uboat_type;
int scroll_x, scroll_y;
unsigned char snd_ping, snd_boom;

static unsigned char helm_e, helm_u;
static unsigned char orders[N_ESCORT];
static unsigned char sunk[N_SHIP];
static unsigned char player;
static unsigned char conn_on;
static unsigned char tubes, hits, tonnage, winner;
static unsigned char ping_f, drop_f, fire_f;
static unsigned char cool_fire, cool_dc[N_ESCORT], cool_drop;
static unsigned char contact_age, hear_on, hear_hdg;
static unsigned int ticks;
static int contact_x, contact_y;

#define UB (N_SHIP - 1)

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

typedef struct { int x, y; unsigned char hdg, life, on; } shot_t;
static shot_t torp[N_TORP];
static shot_t charge[N_DC];

typedef struct {
    const char *name;
    unsigned char sprite, surface_kn, submerged_kn, bow, stern, gun;
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

unsigned char game_order_of(unsigned char escort) { return orders[escort]; }
unsigned char game_player(void) { return player; }
unsigned char game_alive(unsigned char i) { return (unsigned char)!sunk[i]; }
unsigned char game_tubes(void) { return tubes; }
unsigned char game_hits(void) { return hits; }
unsigned char game_tonnage(void) { return tonnage; }
unsigned char game_winner(void) { return winner; }
unsigned char game_torp_on(unsigned char i) { return torp[i].on; }
unsigned char game_dc_on(unsigned char i) { return charge[i].on; }
void game_torp_xy(unsigned char i, int *x, int *y) { *x = torp[i].x; *y = torp[i].y; }
void game_dc_xy(unsigned char i, int *x, int *y) { *x = charge[i].x; *y = charge[i].y; }

void game_station(unsigned char escort, int *x, int *y)
{
    *x = CENTER_X + st_x[escort];
    *y = CENTER_Y + st_y[escort];
}

static void vel(unsigned char hdg, int speed, int *vx, int *vy)
{
    *vx = speed * sdir[hdg >> 3] / 16;
    *vy = -(speed * sdir[((hdg + 64) & 255) >> 3] / 16);
}

static unsigned char heading_to(int dx, int dy)
{
    int ax = dx < 0 ? -dx : dx;
    int ay = dy < 0 ? -dy : dy;
    if (ax < FP && ay < FP) return 0;
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

static unsigned char near_px(int x0, int y0, int x1, int y1, int r)
{
    long dx = ((long)x0 - x1) / FP;
    long dy = ((long)y0 - y1) / FP;
    if (dx < 0) dx = -dx;
    if (dy < 0) dy = -dy;
    if (dx > r || dy > r) return 0;
    return (unsigned char)(dx * dx + dy * dy <= (long)r * r);
}

static void clamp_ship(ship_t *s)
{
    if (s->x < -64 * FP) s->x = -64 * FP;
    if (s->x > 704 * FP) s->x = 704 * FP;
    if (s->y < -64 * FP) s->y = -64 * FP;
    if (s->y > 512 * FP) s->y = 512 * FP;
}

static void mark_contact(void)
{
    contact_x = ships[UB].x;
    contact_y = ships[UB].y;
    contact_age = 1;
}

static void add_tonnage(unsigned char i)
{
    unsigned char pts = (ships[i].kind == SHIP_TANKER) ? 2 : 1;
    if (tonnage + pts < 255) tonnage = (unsigned char)(tonnage + pts);
}

void game_init(unsigned char uboat)
{
    unsigned char i;
    uboat_type = uboat < BOAT_N ? uboat : BOAT_VIIC;
    selected = 0;
    player = ROLE_ESCORT;
    conn_on = 1;
    helm_e = HELM_MID;
    helm_u = HELM_MID;
    ping_f = drop_f = fire_f = 0;
    cool_fire = cool_drop = 0;
    tubes = boats[uboat_type].bow;
    hits = tonnage = winner = 0;
    contact_age = hear_on = 0;
    ticks = 0;
    snd_ping = snd_boom = 0;
    scroll_x = scroll_y = 0;
    for (i = 0; i < N_ESCORT; i++) {
        orders[i] = ORD_PATROL;
        cool_dc[i] = 0;
        sunk[i] = 0;
        ships[i].kind = SHIP_ESCORT;
        ships[i].hdg = 0;
        ships[i].tel = TEL_HALF;
        game_station(i, &ships[i].x, &ships[i].y);
    }
    for (i = 0; i < N_MERCH; i++) {
        unsigned char s = (unsigned char)(N_ESCORT + i);
        sunk[s] = 0;
        ships[s].kind = (i == N_MERCH - 1) ? SHIP_TANKER : SHIP_MERCHANT;
        ships[s].hdg = 0;
        ships[s].tel = TEL_HALF;
        ships[s].x = CENTER_X + form_x[i];
        ships[s].y = CENTER_Y + form_y[i];
    }
    sunk[UB] = 0;
    ships[UB].kind = SHIP_UBOAT;
    ships[UB].hdg = 0;
    ships[UB].tel = TEL_STOP;
    ships[UB].x = CENTER_X;
    ships[UB].y = CENTER_Y - 200 * FP;
    for (i = 0; i < N_TORP; i++) torp[i].on = 0;
    for (i = 0; i < N_DC; i++) charge[i].on = 0;
}

void game_set_uboat(unsigned char uboat)
{
    uboat_type = uboat < BOAT_N ? uboat : BOAT_VIIC;
}

void game_set_player(unsigned char role)
{
    player = role == ROLE_UBOAT ? ROLE_UBOAT : ROLE_ESCORT;
    conn_on = (unsigned char)(player == ROLE_ESCORT);
}

void game_select(unsigned char escort)
{
    if (escort < N_ESCORT && !sunk[escort]) selected = escort;
}

void game_cycle(signed char dir)
{
    unsigned char n = selected;
    unsigned char k;
    for (k = 0; k < N_ESCORT; k++) {
        if (dir < 0) n = (unsigned char)((n + N_ESCORT - 1) % N_ESCORT);
        else n = (unsigned char)((n + 1) % N_ESCORT);
        if (!sunk[n]) { selected = n; return; }
    }
}

void game_order(unsigned char ord)
{
    if (ord > ORD_ATTACK) return;
    orders[selected] = ord;
}

void game_helm(unsigned char helm) { helm_e = helm; }
void game_uboat_helm(unsigned char helm) { helm_u = helm; }
void game_uboat_set_tel(unsigned char tel)
{
    if (tel > TEL_FLANK) tel = TEL_FLANK;
    ships[UB].tel = tel;
}

static void bump_tel(unsigned char *tel, signed char d)
{
    int t = (int)*tel + d;
    if (t < TEL_STOP) t = TEL_STOP;
    if (t > TEL_FLANK) t = TEL_FLANK;
    *tel = (unsigned char)t;
}

void game_telegraph_delta(signed char d) { bump_tel(&ships[selected].tel, d); }
void game_uboat_telegraph_delta(signed char d) { bump_tel(&ships[UB].tel, d); }
void game_ping(void) { ping_f = 1; }
void game_drop(void) { drop_f = 1; }
void game_fire(void) { fire_f = 1; }

static void steer_to(unsigned char i, int tx, int ty, unsigned char fast, unsigned char hold_convoy)
{
    int dx = tx - ships[i].x;
    int dy = ty - ships[i].y;
    if (dx > -8 * FP && dx < 8 * FP && dy > -8 * FP && dy < 8 * FP) {
        if (hold_convoy) {
            ships[i].hdg = 0;
            ships[i].tel = TEL_HALF;
        } else {
            ships[i].tel = TEL_SLOW;
        }
    } else {
        ships[i].hdg = turn_toward(ships[i].hdg, heading_to(dx, dy));
        ships[i].tel = fast;
    }
}

void game_ai_escort(void)
{
    unsigned char i, best = 0;
    long best_d = 999999L;
    game_helm(HELM_MID);
    if (!contact_age) {
        for (i = 0; i < N_ESCORT; i++) orders[i] = ORD_PATROL;
        return;
    }
    for (i = 0; i < N_ESCORT; i++) {
        long dx, dy, d;
        orders[i] = ORD_PATROL;
        if (sunk[i]) continue;
        dx = ((long)contact_x - ships[i].x) / FP;
        dy = ((long)contact_y - ships[i].y) / FP;
        d = dx * dx + dy * dy;
        if (d < best_d) { best_d = d; best = i; }
    }
    if (!sunk[best]) orders[best] = ORD_ATTACK;
}

void game_ai_uboat(void)
{
    unsigned char i;
    long best = 999999L;
    int bx = ships[N_ESCORT].x, by = ships[N_ESCORT].y;
    unsigned char danger = 0;
    game_uboat_helm(HELM_MID);
    if (sunk[UB] || winner) return;
    for (i = 0; i < N_ESCORT; i++) {
        if (!sunk[i] && near_px(ships[i].x, ships[i].y, ships[UB].x, ships[UB].y, 42))
            danger = 1;
    }
    if (danger) {
        ships[UB].hdg = 64;
        ships[UB].tel = TEL_FLANK;
        return;
    }
    for (i = N_ESCORT; i < UB; i++) {
        long dx, dy, d;
        if (sunk[i]) continue;
        dx = ((long)ships[i].x - ships[UB].x) / FP;
        dy = ((long)ships[i].y - ships[UB].y) / FP;
        d = dx * dx + dy * dy;
        if (d < best) { best = d; bx = ships[i].x; by = ships[i].y; }
    }
    ships[UB].hdg = heading_to(bx - ships[UB].x, by - ships[UB].y);
    if (best < 36L * 36L) {
        ships[UB].tel = TEL_SLOW;
        game_fire();
    } else if (by + 20 * FP < ships[UB].y) {
        ships[UB].tel = TEL_FULL;
    } else {
        ships[UB].tel = TEL_STOP;
    }
}

static void spawn_torp(void)
{
    unsigned char i;
    if (!tubes || cool_fire || sunk[UB]) return;
    for (i = 0; i < N_TORP; i++) {
        if (torp[i].on) continue;
        torp[i].on = 1;
        torp[i].x = ships[UB].x;
        torp[i].y = ships[UB].y;
        torp[i].hdg = ships[UB].hdg;
        torp[i].life = 80;
        tubes--;
        cool_fire = 20;
        return;
    }
}

static void spawn_dc(unsigned char who)
{
    unsigned char i;
    if (sunk[who] || cool_dc[who]) return;
    for (i = 0; i < N_DC; i++) {
        if (charge[i].on) continue;
        charge[i].on = 1;
        charge[i].x = ships[who].x;
        charge[i].y = ships[who].y;
        charge[i].life = 18;
        cool_dc[who] = 36;
        return;
    }
}

static void sensors(void)
{
    unsigned char i;
    unsigned char refreshed = 0;
    hear_on = 0;
    if (!sunk[UB]) {
        for (i = 0; i < N_ESCORT; i++) {
            if (sunk[i]) continue;
            if (near_px(ships[i].x, ships[i].y, ships[UB].x, ships[UB].y, 56)) {
                mark_contact();
                refreshed = 1;
            }
        }
        if (ping_f && !sunk[selected]) {
            int dx = ships[UB].x - ships[selected].x;
            int dy = ships[UB].y - ships[selected].y;
            unsigned char want = heading_to(dx, dy);
            int diff = (int)ships[selected].hdg - (int)want;
            snd_ping = 1;
            if (diff < 0) diff = -diff;
            if (diff > 128) diff = 256 - diff;
            if (diff < 28 && near_px(ships[selected].x, ships[selected].y, ships[UB].x, ships[UB].y, 130)) {
                mark_contact();
                refreshed = 1;
            }
        }
        if (ships[UB].tel != TEL_STOP && !sunk[selected] &&
            near_px(ships[selected].x, ships[selected].y, ships[UB].x, ships[UB].y, 180)) {
            hear_on = 1;
            hear_hdg = heading_to(ships[UB].x - ships[selected].x, ships[UB].y - ships[selected].y);
        }
    }
    if (player == ROLE_UBOAT) {
        long best = 999999L;
        hear_on = 0;
        for (i = 0; i < UB; i++) {
            long dx, dy, d;
            if (sunk[i] || ships[i].tel == TEL_STOP) continue;
            dx = ((long)ships[i].x - ships[UB].x) / FP;
            dy = ((long)ships[i].y - ships[UB].y) / FP;
            d = dx * dx + dy * dy;
            if (d < 180L * 180L && d < best) {
                best = d;
                hear_on = 1;
                hear_hdg = heading_to(ships[i].x - ships[UB].x, ships[i].y - ships[UB].y);
            }
        }
    }
    ping_f = 0;
    if (contact_age && !refreshed) {
        if (contact_age < 150) contact_age++;
        else contact_age = 0;
    }
}

unsigned char game_hear(unsigned int *deg)
{
    if (!hear_on) return 0;
    *deg = (unsigned int)((unsigned long)hear_hdg * 360ul / 256ul);
    return 1;
}

unsigned char game_show_uboat(void)
{
    if (sunk[UB]) return 0;
    if (player == ROLE_UBOAT) return 1;
    return near_px(ships[selected].x, ships[selected].y, ships[UB].x, ships[UB].y, 56);
}

unsigned char game_show_ship(unsigned char i)
{
    if (sunk[i]) return 0;
    if (i == UB) return game_show_uboat();
    if (player == ROLE_ESCORT) return 1;
    if (i < N_ESCORT || ships[i].kind == SHIP_MERCHANT || ships[i].kind == SHIP_TANKER)
        return near_px(ships[UB].x, ships[UB].y, ships[i].x, ships[i].y, 110);
    return 1;
}

unsigned char game_show_contact(int *x, int *y)
{
    if (player != ROLE_ESCORT || !contact_age || game_show_uboat()) return 0;
    *x = contact_x;
    *y = contact_y;
    return 1;
}

static void sink(unsigned char i)
{
    if (sunk[i]) return;
    sunk[i] = 1;
    ships[i].tel = TEL_STOP;
    snd_boom = 1;
    if (i == UB) winner = WIN_ESCORT;
    else {
        add_tonnage(i);
        if (i < N_ESCORT) {
            unsigned char k, any = 0;
            for (k = 0; k < N_ESCORT; k++) if (!sunk[k]) any = 1;
            if (!any) winner = WIN_UBOAT;
        } else if (tonnage >= TONNAGE_WIN) {
            winner = WIN_UBOAT;
        }
    }
}

void game_tick(void)
{
    unsigned char i;
    int cvx, cvy, vx, vy;
    if (winner) return;
    if (conn_on && sunk[selected]) game_cycle(1);
    if (conn_on && !sunk[selected]) {
        if (helm_e == HELM_PORT) ships[selected].hdg = (unsigned char)(ships[selected].hdg - 4);
        else if (helm_e == HELM_STBD) ships[selected].hdg = (unsigned char)(ships[selected].hdg + 4);
    }
    if (!sunk[UB]) {
        if (helm_u == HELM_PORT) ships[UB].hdg = (unsigned char)(ships[UB].hdg - 4);
        else if (helm_u == HELM_STBD) ships[UB].hdg = (unsigned char)(ships[UB].hdg + 4);
    }
    helm_e = HELM_MID;
    helm_u = HELM_MID;
    sensors();

    for (i = 0; i < N_ESCORT; i++) {
        int tx, ty;
        if (sunk[i]) { ships[i].tel = TEL_STOP; continue; }
        if (conn_on && i == selected) continue;
        if (orders[i] == ORD_ATTACK && contact_age) {
            steer_to(i, contact_x, contact_y, TEL_FLANK, 0);
            if (near_px(ships[i].x, ships[i].y, contact_x, contact_y, 22))
                spawn_dc(i);
        } else if (orders[i] == ORD_HUNT && contact_age) {
            steer_to(i, contact_x, contact_y, TEL_FULL, 0);
        } else if (orders[i] == ORD_HUNT || orders[i] == ORD_ATTACK) {
            tx = CENTER_X + ((ticks & 64) ? 90 * FP : -90 * FP);
            ty = CENTER_Y - 130 * FP;
            steer_to(i, tx, ty, orders[i] == ORD_ATTACK ? TEL_FLANK : TEL_FULL, 0);
        } else {
            game_station(i, &tx, &ty);
            steer_to(i, tx, ty, TEL_FULL, 1);
        }
    }
    for (i = N_ESCORT; i < UB; i++) {
        if (sunk[i]) ships[i].tel = TEL_STOP;
        else { ships[i].hdg = 0; ships[i].tel = TEL_HALF; }
    }
    if (drop_f && conn_on && !cool_drop) {
        spawn_dc(selected);
        cool_drop = 28;
    }
    drop_f = 0;
    if (fire_f) spawn_torp();
    fire_f = 0;

    vel(0, tel_fp[TEL_HALF], &cvx, &cvy);
    scroll_x += cvx / FP;
    scroll_y += cvy / FP;
    if (contact_age) {
        contact_x -= cvx;
        contact_y -= cvy;
    }
    for (i = 0; i < N_SHIP; i++) {
        vel(ships[i].hdg, tel_fp[ships[i].tel], &vx, &vy);
        ships[i].x += vx - cvx;
        ships[i].y += vy - cvy;
        clamp_ship(&ships[i]);
    }
    for (i = 0; i < N_TORP; i++) {
        unsigned char t;
        if (!torp[i].on) continue;
        vel(torp[i].hdg, 6 * FP, &vx, &vy);
        torp[i].x += vx - cvx;
        torp[i].y += vy - cvy;
        if (torp[i].life) torp[i].life--;
        if (!torp[i].life) { torp[i].on = 0; continue; }
        for (t = 0; t < UB; t++) {
            if (!sunk[t] && near_px(torp[i].x, torp[i].y, ships[t].x, ships[t].y, 12)) {
                torp[i].on = 0;
                sink(t);
                break;
            }
        }
    }
    for (i = 0; i < N_DC; i++) {
        if (!charge[i].on) continue;
        charge[i].x -= cvx;
        charge[i].y -= cvy;
        if (charge[i].life) charge[i].life--;
        if (charge[i].life) continue;
        charge[i].on = 0;
        snd_boom = 1;
        if (!sunk[UB] && near_px(charge[i].x, charge[i].y, ships[UB].x, ships[UB].y, 16)) {
            if (hits < 255) hits++;
            if (hits >= 3) sink(UB);
        }
    }
    if (cool_fire) cool_fire--;
    if (cool_drop) cool_drop--;
    for (i = 0; i < N_ESCORT; i++) if (cool_dc[i]) cool_dc[i]--;
    ticks++;
}

static unsigned char pack_orders(void)
{
    return (unsigned char)(orders[0] | (orders[1] << 2) | (orders[2] << 4) | (orders[3] << 6));
}

static void unpack_orders(unsigned char v)
{
    unsigned char i;
    for (i = 0; i < N_ESCORT; i++) orders[i] = (unsigned char)((v >> (i * 2)) & 3);
}

unsigned char game_export_escort(unsigned char seq, unsigned char *b)
{
    b[0] = (unsigned char)((helm_e & 3) | ((ships[selected].tel & 7) << 2) |
                           ((selected & 3) << 5) | (ping_f ? 0x80 : 0));
    b[1] = pack_orders();
    b[2] = (unsigned char)((drop_f ? 1 : 0) | ((seq & 15) << 4));
    b[3] = (unsigned char)game_checksum();
    return b[2];
}

unsigned char game_export_uboat(unsigned char seq, unsigned char *b)
{
    b[0] = (unsigned char)((helm_u & 3) | ((ships[UB].tel & 7) << 2) | (fire_f ? 0x80 : 0));
    b[1] = 0;
    b[2] = (unsigned char)((seq & 15) << 4);
    b[3] = (unsigned char)game_checksum();
    return b[2];
}

unsigned char game_import_escort(const unsigned char *b)
{
    unsigned char ok = (unsigned char)(b[3] == (unsigned char)game_checksum());
    helm_e = (unsigned char)(b[0] & 3);
    selected = (unsigned char)((b[0] >> 5) & 3);
    if (selected >= N_ESCORT) selected = 0;
    ships[selected].tel = (unsigned char)((b[0] >> 2) & 7);
    if (ships[selected].tel > TEL_FLANK) ships[selected].tel = TEL_FLANK;
    unpack_orders(b[1]);
    if (b[0] & 0x80) ping_f = 1;
    if (b[2] & 1) drop_f = 1;
    return ok;
}

unsigned char game_import_uboat(const unsigned char *b)
{
    unsigned char ok = (unsigned char)(b[3] == (unsigned char)game_checksum());
    helm_u = (unsigned char)(b[0] & 3);
    ships[UB].tel = (unsigned char)((b[0] >> 2) & 7);
    if (ships[UB].tel > TEL_FLANK) ships[UB].tel = TEL_FLANK;
    if (b[0] & 0x80) fire_f = 1;
    return ok;
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
        s = s * 33u + sunk[i];
    }
    for (i = 0; i < N_ESCORT; i++) s = s * 33u + orders[i];
    s = s * 33u + tubes;
    s = s * 33u + hits;
    s = s * 33u + tonnage;
    s = s * 33u + (unsigned int)scroll_x;
    s = s * 33u + (unsigned int)scroll_y;
    s = s * 33u + contact_age;
    return s;
}
