/*
 * Zimodem on the network card's UART B at $9FE8.
 * Crystal is 14.7456 MHz, divisor 8 is 115200 8N1.
 * Payload is ASCII hex so a byte never becomes '+', which would leave data mode.
 */
#include <cx16.h>
#include <cbm.h>
#include <x16lib/netcard.h>
#include "link.h"

static unsigned char state;
static unsigned char role;
static unsigned char phase;
static unsigned int timer;
static char host[24];
static char line[48];
static unsigned char llen;
static char status[28];
static unsigned char pkt_side;
static unsigned char pkt[4];
static unsigned char have;
static unsigned char hexn;
static unsigned char hexv;
static unsigned char pkt_i;
static unsigned char hello_on, hello_type;

static void set_status(const char *s)
{
    unsigned char i = 0;
    while (s[i] && i < 27) { status[i] = s[i]; i++; }
    status[i] = 0;
}

static void uart_putc(unsigned char c)
{
    netcard_putc(c);
}

static void uart_puts(const char *s)
{
    netcard_puts(s);
}

static int uart_getc(void)
{
    if (!netcard_rx_ready()) return -1;
    return netcard_getc();
}

static char hex_digit(unsigned char v)
{
    return (char)(v < 10 ? '0' + v : 'A' + (v - 10));
}

static int hex_val(unsigned char c)
{
    /* The wire is ASCII. cc65 stores 'A' as PETSCII, so compare the ASCII bytes. */
    if (c >= 0x30 && c <= 0x39) return c - 0x30;
    if (c >= 0x41 && c <= 0x46) return c - 0x41 + 10;
    if (c >= 0x61 && c <= 0x66) return c - 0x61 + 10;
    return -1;
}

void link_reset(void)
{
    state = LINK_IDLE;
    phase = 0;
    have = 0;
    hello_on = 0;
    llen = 0;
    set_status("OFFLINE");
}

unsigned char link_state(void) { return state; }
const char *link_status(void) { return status; }

void link_begin(const char *host_in, unsigned char want)
{
    unsigned char i = 0;
    link_reset();
    role = want;
    while (host_in[i] && i < 22) { host[i] = host_in[i]; i++; }
    host[i] = 0;
    netcard_open();
    waitvsync();
    uart_puts("ATE0");
    uart_putc(0x0D);
    state = LINK_BUSY;
    phase = 1;
    timer = 0;
    set_status("DIALING");
}

void link_send_packet(unsigned char side, const unsigned char *data)
{
    unsigned char i;
    if (state != LINK_UP) return;
    uart_putc(side);
    for (i = 0; i < 4; i++) {
        uart_putc((unsigned char)hex_digit((unsigned char)(data[i] >> 4)));
        uart_putc((unsigned char)hex_digit((unsigned char)(data[i] & 15)));
    }
    uart_putc('\n');
}

static void on_line(void)
{
    if (phase == 1) {
        if (line[0] == 0x4F && line[1] == 0x4B) {
            uart_puts("ATD");
            uart_puts(host);
            uart_puts(":" LINK_PORT);
            uart_putc(0x0D);
            phase = 2;
            set_status("DIALING");
        }
    } else if (phase == 2) {
        if (line[0] == 0x43) {
            phase = 3;
            state = LINK_UP;
            hexn = 0;
            set_status(role == 'U' ? "LINK UP U" : "LINK UP E");
        } else if (line[0] == 0x4E || line[0] == 0x45) {
            state = LINK_FAIL;
            phase = 0;
            set_status("NO CARRIER");
        }
    }
    llen = 0;
}

static void on_up_byte(unsigned char c)
{
    int v;
    if (hexn == 20) {
        v = hex_val(c);
        if (v >= 0) { hello_type = (unsigned char)v; hello_on = 1; }
        hexn = 0;
        return;
    }
    if (!hexn) {
        if (c == 0x48) { hexn = 20; return; }
        if (c == 0x45 || c == 0x55) {
            pkt_side = (unsigned char)(c == 0x45 ? 'E' : 'U');
            hexn = 1;
            pkt_i = 0;
            hexv = 0;
        }
        return;
    }
    if (c == 0x0A || c == 0x0D) {
        if (pkt_i == 4) have = 1;
        hexn = 0;
        return;
    }
    v = hex_val(c);
    if (v < 0) { hexn = 0; return; }
    if (hexn & 1) hexv = (unsigned char)v;
    else if (pkt_i < 4) pkt[pkt_i++] = (unsigned char)((hexv << 4) | v);
    hexn++;
}

void link_send_hello(unsigned char type)
{
    if (state != LINK_UP) return;
    uart_putc('H');
    uart_putc((unsigned char)hex_digit((unsigned char)(type & 15)));
    uart_putc('\n');
}

unsigned char link_take_hello(unsigned char *type)
{
    if (!hello_on) return 0;
    hello_on = 0;
    *type = hello_type;
    return 1;
}

unsigned char link_pump(unsigned char *side, unsigned char *data)
{
    int c;
    unsigned char n = 0;
    if (state == LINK_IDLE || state == LINK_FAIL) return 0;
    if (state == LINK_BUSY) {
        if (timer < 600) timer++;
        else {
            state = LINK_FAIL;
            set_status("NO CARRIER");
            return 0;
        }
    }
    while (n < 32 && (c = uart_getc()) >= 0) {
        n++;
        if (state == LINK_UP) {
            on_up_byte((unsigned char)c);
            continue;
        }
        if (c == 0x0D) continue;
        if (c == 0x0A) { line[llen] = 0; on_line(); continue; }
        if (llen < 46) line[llen++] = (char)c;
    }
    if (!have) return 0;
    have = 0;
    *side = pkt_side;
    data[0] = pkt[0];
    data[1] = pkt[1];
    data[2] = pkt[2];
    data[3] = pkt[3];
    return 1;
}
