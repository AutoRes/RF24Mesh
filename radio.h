#ifndef RADIO_H
#define RADIO_H

#include "mesh_config.h"
#include "queue.h"
#include "msg.h"

typedef struct
{
	bool listening;

	uint32_t mesh_id;
	addr_t self_addr;
	addr_t last_dst_addr;

	queue_head tx;
} Radio;

extern Radio radio;

void radio_init(uint32_t mesh_id, addr_t self_addr);
void radio_irq(void);
void radio_send(msg_t *m);

#endif
