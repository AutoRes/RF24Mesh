/* -------------------------------------------------------------------------- */

#include "layer2.h"
#include "TimerOne.h"

Layer2 layer2;

/* -------------------------------------------------------------------------- */

static void l2_send_hello(void)
{
	msg_t *m = msg_new(0);

	msg_header_t *mh = msg_get_header(m);
	mh->type = MSG_L2_HELLO;

	l2_send(m, BCAST_ADDR);
}

static void l2_send_ping(uint8_t to)
{
	msg_t *m = msg_new(0);

	msg_header_t *mh = msg_get_header(m);
	mh->type = MSG_L2_PING;

	l2_send(m, to);
}

/* -------------------------------------------------------------------------- */

static void l2_recv_pre(uint8_t addr)
{
	uint8_t i;
	for(i = 0; i < layer2.nb_l; i++)
	{
		if(layer2.nb[i].addr == addr)
		{
			layer2.nb[i].timer = 0;
			break;
		}
	}
	if(i == layer2.nb_l)
	{
		l2_add_nb(addr);
		l3_found(addr);
	}
}

/* -------------------------------------------------------------------------- */

void l2_init(void)
{
}

void l2_tick(void)
{
	if(layer2.hello_cnt == HELLO_MAX_TIMER)
	{
		l2_send_hello();
		layer2.hello_cnt = 0;
	}
	else layer2.hello_cnt++;

	for(uint8_t i = 0; i < layer2.nb_l; i++)
	{
		if(layer2.nb[i].timer == NODE_MAX_TIMER)
		{
			l2_send_ping(layer2.nb[i].addr);
		}

		layer2.nb[i].timer++;
	}
}

/* -------------------------------------------------------------------------- */

void l2_add_nb(uint8_t addr)
{
	if(layer2.nb_l < NB_MAX)
	{
		layer2.nb[layer2.nb_l].addr = addr;
		layer2.nb[layer2.nb_l].timer = 0;
		layer2.nb_l++;
	}
}

void l2_del_nb(uint8_t addr)
{
	for(uint8_t i = 0; i < layer2.nb_l; i++)
	{
		if(layer2.nb[i].addr == addr)
		{
			layer2.nb_l--;
			layer2.nb[i].addr = layer2.nb[layer2.nb_l].addr;
			layer2.nb[i].timer = layer2.nb[layer2.nb_l].timer;
			return;
		}
	}
}

/* -------------------------------------------------------------------------- */

void l2_on_send_failure(uint8_t addr)
{
	l2_del_nb(addr);
	l3_died(addr);
}

void l2_send(msg_t *m, uint8_t to)
{
	msg_header_t *mh = msg_get_header(m);
	mh->l2_src = radio.self_addr;

	m->dst = to;
	radio_send(m);
}

void l2_recv_irq(msg_t *m)
{
	msg_header_t *mh = msg_get_header(m);

	l2_recv_pre(mh->l2_src);

	switch(mh->type)
	{
		case MSG_L3_OGM:
		case MSG_L3_ROGM:
		case MSG_L3_KNOWN:
		case MSG_PL_BROADCAST:
		case MSG_PL_MULTICAST:
		case MSG_PL:
			l3_recv_irq(m);
			break;

		default: // hello; ping;
			msg_free(m);
			break;
	}
}

/* -------------------------------------------------------------------------- */
