#ifndef CONVOY_X16CFG_H
#define CONVOY_X16CFG_H

/* Fixed sprite slots, so the pool is left out. sprite_put writes the attributes.
 * The Zimodem link uses the library network-card driver.
 * The propeller drone is PSG voice 0;
 * sound-cli effects use voices 2 and 3 (assets/sounds/sfx.x16snd.json).
 * No wide text and no text UI. Options are music and sfx in boot/CONVOY.CFG
 * (fileio/file_save, fileio/config_load). There is no title theme yet, so music is only stored. */
#define X16_NO_SPRITE_POOL 1
#define X16LIB_NETCARD 1

#endif
