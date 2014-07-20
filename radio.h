#ifndef __RADIO__
#define __RADIO__

#include "nRF24L01.h"
#include "RF24.h"
#include "queue.h"
#include "msg.h"
#include "layer2.h"
#include "layer3.h"

#define BCAST_ADDR 0xFF
#define BCAST_PIPE 1
#define SELF_PIPE  2

#define RETRY_DELAY 4
#define RETRY_MAX   15

#define MAX_TX_PENDING 3

#define DEFAULT_IRQ_N 0
#define DEFAULT_CEPIN 9
#define DEFAULT_CSPIN 10

struct Radio
{
	RF24 *rf24;
	bool listening;
	// TODO: sending timeout

	uint8_t self_addr;
	uint8_t last_dst_addr;

	queue_head tx;
	queue_head rx;
};

extern Radio radio;

void radio_init(uint8_t self_addr, uint8_t irq_n = DEFAULT_IRQ_N,
	uint8_t cepin = DEFAULT_CEPIN, uint8_t cspin = DEFAULT_CSPIN);


void radio_send(msg_t *m);

#endif
