#ifndef LAYER2_H
#define LAYER2_H

#include "msg.h"
#include "radio.h"
#include "layer3.h"

#define NB_MAX 32

struct Layer2
{
	struct
	{
		uint8_t addr;

#define NODE_MAX_TIMER 40
#define PONG_MAX_TIMER  1
		uint8_t timer;
	} nb[NB_MAX];
	uint8_t nb_l;

#define HELLO_MAX_TIMER 4
	uint8_t hello_cnt;
};

extern Layer2 layer2;

void l2_init(void);
void l2_tick(void);

void l2_send(msg_t *m, uint8_t to);
void l2_recv_irq(msg_t *m);

void l2_add_nb(uint8_t addr);
void l2_del_nb(uint8_t addr);

void l2_on_send_failure(uint8_t addr);

#endif
