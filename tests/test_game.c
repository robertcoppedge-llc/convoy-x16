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

    if (fails) {
        fprintf(stderr, "%d failed\n", fails);
        return 1;
    }
    printf("convoy tests ok\n");
    return 0;
}
