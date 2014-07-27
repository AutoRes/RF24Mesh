#ifndef LAYER3_H
#define LAYER3_H

#include "mesh_config.h"
#include "msg.h"

struct Layer3
{
	struct
	{
		// assuming addr_t == uint8_t
		addr_t hop:7;
		addr_t seq:1;
	} nodes[NUM_NODES_MAX];
	addr_t nodes_n;

	uint8_t ogm_cnt;
};

extern Layer3 layer3;

void l3_init(void);
void l3_tick(void);

void l3_send(msg_t *m, addr_t to);
void l3_recv_irq(msg_t *m);

void l3_died(addr_t addr);
void l3_found(addr_t addr);

#endif
