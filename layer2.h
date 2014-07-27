#ifndef LAYER2_H
#define LAYER2_H

#include "mesh_config.h"
#include "msg.h"

struct Layer2
{
	struct
	{
		addr_t addr;
		uint8_t timer;
	} nb[NUM_NB_MAX];
	nb_iter_t nb_l;

	uint8_t hello_cnt;
};

extern Layer2 layer2;

void l2_init(void);
void l2_tick(void);

void l2_send(msg_t *m, addr_t to);
void l2_recv_irq(msg_t *m);

uint8_t l2_nb_find(addr_t addr);
bool l2_nb_add(addr_t addr);
void l2_nb_del(addr_t addr);

void l2_on_send_failure(addr_t addr);

#endif
