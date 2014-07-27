#ifndef __RADIO__
#define __RADIO__

#include "queue.h"
#include "msg.h"

#define BCAST_ADDR 0xFF
#define BCAST_PIPE 2
#define SELF_PIPE  1

#define RETRY_DELAY 4
#define RETRY_MAX   15

#define MAX_TX_PENDING 3

struct Radio
{
	bool listening;
	// TODO: sending timeout

	uint32_t mesh_id;
	uint8_t self_addr;
	uint8_t last_dst_addr;

	queue_head tx;
};

extern Radio radio;

void radio_init(uint32_t mesh_id, uint8_t self_addr);
void radio_irq(void);
void radio_send(msg_t *m);

#endif
