#ifndef LAYER3_H
#define LAYER3_H

#include "msg.h"
#include "radio.h"
#include "layer2.h"
#include "mesh.h"

#define NODES_MAX 127

struct Layer3
{
	struct
	{
		uint8_t hop:7;
		uint8_t seq:1;
	} nodes[NODES_MAX];
	uint8_t nodes_n;

#define OGM_TIMER 100
	uint8_t ogm_cnt;
};

extern Layer3 layer3;

void l3_init(void);
void l3_tick(void);

void l3_send(msg_t *m, uint8_t to);
void l3_recv_irq(msg_t *m);

void l3_died(uint8_t addr);
void l3_found(uint8_t addr);

#endif
