/* -------------------------------------------------------------------------- */

#include "layer2.h"

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

static void l2_send_pong(uint8_t to)
{
	msg_t *m = msg_new(0);

	msg_header_t *mh = msg_get_header(m);
	mh->type = MSG_L2_PONG;

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
		l3_found(addr);
	}

	// optimization
	layer3.nodes[addr].hop = addr;
}

static void l2_recv_ping(msg_t *m)
{
	msg_header_t *mh = msg_get_header(m);
	l2_send_pong(mh->l2_src);
	msg_free(m);
}

/* -------------------------------------------------------------------------- */

void l2_init(void)
{
	// TODO: attach timer interrupt
}

void l2_tick(void)
{
	if(layer2.hello_cnt == HELLO_TIMER)
	{
		l2_send_hello();
		layer2.hello_cnt = 0;
	}
	else layer2.hello_cnt++;

	for(uint8_t i = 0; i < layer2.nb_l; i++)
	{
		if(layer2.nb[i].timer == NODE_TIMEOUT)
			l2_send_ping(layer2.nb[i].addr);
		else if(layer2.nb[i].timer == NODE_TIMEOUT+PONG_TIMER)
			l3_died(layer2.nb[i].addr);

		layer2.nb[i].timer++;
	}
}

/* -------------------------------------------------------------------------- */

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

	// optimization
	l2_recv_pre(mh->l2_src);

	switch(mh->type)
	{
		case MSG_L2_PING:
			l2_recv_ping(m);
			break;

		case MSG_L3_OGM:
		case MSG_L3_ROGM:
		case MSG_L3_KNOWN:
		case MSG_PL_BROADCAST:
		case MSG_PL_MULTICAST:
		case MSG_PL:
			l3_recv_irq(m);
			break;

		default: // hello; pong;
			msg_free(m);
			break;
	}
}

/* -------------------------------------------------------------------------- */
