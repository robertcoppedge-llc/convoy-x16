#include <stdio.h>
#include <stdlib.h>
#include "game.h"

static int fails;

static void expect(int cond, const char *msg)
{
    if (!cond) {
        fprintf(stderr, "FAIL %s\n", msg);
        fails++;
    }
}

static int abs_i(int v) { return v < 0 ? -v : v; }

int main(void)
{
    int sx, sy, y0, my, uy, x1;
    unsigned int c0;
    unsigned char i;

    expect(boat_submerged(BOAT_XXI) > boat_submerged(BOAT_VIIC), "XXI is faster under");
    expect(boat_gun(BOAT_XXI) == 0, "XXI has no deck gun");
    expect(boat_gun(BOAT_VIIC) == 1 && boat_gun(BOAT_IXC) == 1, "VII and IX carry a gun");
    expect(boat_stern(BOAT_IXC) == 2 && boat_bow(BOAT_IXC) == 4, "IXC tubes");
    expect(boat_bow(BOAT_XXI) == 6 && boat_stern(BOAT_XXI) == 0, "XXI tubes");
    expect(boat_bow(BOAT_II) == 3 && boat_stern(BOAT_II) == 0, "Type II tubes");
    expect(boat_sprite(BOAT_XXI) == SPR_XXI, "XXI sprite");

    game_init(BOAT_VIIC);
    c0 = game_checksum();
    game_init(BOAT_VIIC);
    expect(game_checksum() == c0, "same seed, same checksum");

    y0 = ships[0].y;
    my = ships[4].y;
    uy = ships[N_SHIP - 1].y;
    game_telegraph_delta(2);                 /* half -> flank */
    for (i = 0; i < 40; i++) {
        game_helm(HELM_MID);
        game_tick();
    }
    expect(ships[0].y < y0, "flank escort pulls ahead");
    expect(ships[4].y == my, "merchant holds station in the convoy frame");
    expect(ships[9].kind == SHIP_TANKER, "last merchant is the tanker");
    expect(ships[9].y == my + 72 * FP, "tanker holds her row");
    expect(ships[N_SHIP - 1].y > uy, "stopped U-boat drifts down-screen");
    expect(scroll_y == -80, "sea scrolls with the convoy");
    expect(game_checksum() != c0, "checksum moves when the plot moves");

    game_init(BOAT_XXI);
    game_station(1, &sx, &sy);
    ships[1].x = sx + 40 * FP;
    x1 = ships[1].x;
    for (i = 0; i < 80; i++) {
        game_helm(HELM_MID);
        game_tick();
    }
    game_station(1, &sx, &sy);
    expect(abs_i(ships[1].x - sx) < 10 * FP, "wing escort returns to station");
    expect(abs_i(ships[1].y - sy) < 10 * FP, "wing escort station y");
    expect(ships[1].x != x1, "wing escort actually moved");

    game_init(BOAT_II);
    game_select(2);
    game_telegraph_delta(-2);
    expect(ships[2].tel == TEL_STOP, "selected escort takes the telegraph");
    expect(ships[0].tel == TEL_HALF, "the flagship telegraph stays");
    expect(game_tubes() == 3, "type II has three bow tubes");
    game_init(BOAT_XXI);
    expect(game_tubes() == 6, "XXI has six bow tubes");

    game_init(BOAT_VIIC);
    game_cycle(1);
    expect(selected == 1, "cycle moves the conn");
    game_order(ORD_HUNT);
    game_cycle(1);
    game_order(ORD_ATTACK);
    expect(game_order_of(1) == ORD_HUNT, "hunt stays on the ship that left the conn");
    expect(game_order_of(2) == ORD_ATTACK, "attack is on the conn ship");
    expect(selected == 2, "second cycle");

    game_init(BOAT_VIIC);
    game_select(1);
    game_order(ORD_ATTACK);
    game_select(0);
    ships[N_SHIP - 1].x = ships[1].x;
    ships[N_SHIP - 1].y = ships[1].y - 40 * FP;
    y0 = ships[1].y;
    for (i = 0; i < 20; i++) game_tick();
    expect(ships[1].y < y0, "attack order closes on the contact");
    expect(game_order_of(0) == ORD_PATROL, "the conn ship keeps her own order");

    game_init(BOAT_VIIC);
    ships[N_SHIP - 1].x = ships[4].x;
    ships[N_SHIP - 1].y = ships[4].y - 30 * FP;
    ships[N_SHIP - 1].hdg = 128;
    ships[N_SHIP - 1].tel = TEL_STOP;
    game_fire();
    for (i = 0; i < 16; i++) game_tick();
    expect(!game_alive(4), "torpedo sinks the merchant ahead of the bow");
    expect(game_tubes() == 3, "one eel left the bow tubes");
    expect(game_tonnage() >= 1, "tonnage is counted");

    game_init(BOAT_VIIC);
    ships[N_SHIP - 1].x = ships[0].x;
    ships[N_SHIP - 1].y = ships[0].y;
    game_drop();
    for (i = 0; i < 24; i++) game_tick();
    expect(game_hits() >= 1, "a depth charge on the boat registers");

    {
        unsigned char b[4];
        game_init(BOAT_VIIC);
        game_select(2);
        game_order(ORD_ATTACK);
        game_helm(HELM_STBD);
        game_export_escort(4, b);
        game_order(ORD_PATROL);
        game_select(0);
        game_import_escort(b);
        expect(selected == 2, "packet restores the conn");
        expect(game_order_of(2) == ORD_ATTACK, "packet restores the order");
    }

    if (fails) {
        fprintf(stderr, "%d failed\n", fails);
        return 1;
    }
    printf("convoy tests ok\n");
    return 0;
}
