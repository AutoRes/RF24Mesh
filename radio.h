#ifndef __RADIO__
#define __RADIO__

#include "mesh_config.h"
#include "queue.h"
#include "msg.h"

struct Radio
{
	bool listening;

	uint32_t mesh_id;
	addr_t self_addr;
	addr_t last_dst_addr;

	queue_head tx;
};

extern Radio radio;

void radio_init(uint32_t mesh_id, addr_t self_addr);
void radio_irq(void);
void radio_send(msg_t *m);

#endif
