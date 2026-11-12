#ifndef CONVOY_X16CFG_H
#define CONVOY_X16CFG_H

/* Fixed sprite slots, so the pool is left out. sprite_put writes the attributes.
 * The Zimodem link uses the library network-card driver.
 * The propeller drone is PSG voice 0;
 * sound-cli effects use voices 2 and 3 (assets/sounds/sfx.x16snd.json). */
#define X16_NO_SPRITE_POOL 1
#define X16LIB_NETCARD 1

#endif
