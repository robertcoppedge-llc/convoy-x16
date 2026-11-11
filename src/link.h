#ifndef LINK_H
#define LINK_H

#define LINK_PORT "6416"

/* 0 idle, 1 dialing, 2 up, 3 failed. */
#define LINK_IDLE 0
#define LINK_BUSY 1
#define LINK_UP   2
#define LINK_FAIL 3

void link_reset(void);
/* role is 'E' or 'U'. host is a dotted address, no port. */
void link_begin(const char *host, unsigned char role);
unsigned char link_state(void);
const char *link_status(void);

/* Call once a frame. A finished packet is reported through the out pointers. */
unsigned char link_pump(unsigned char *side, unsigned char *data);

void link_send_packet(unsigned char side, const unsigned char *data);
void link_send_hello(unsigned char type);
unsigned char link_take_hello(unsigned char *type);

#endif
