/* -------------------------------------------------------------------------- */

#include "radio.h"
#include "layer2.h"
#include "layer3.h"

#include "msg.h"

Layer2 layer2;

/* -------------------------------------------------------------------------- */

static void l2_send_hello(void)
{
	msg_t *m = msg_new(0);

	msg_header_t *mh = msg_get_header(m);
	mh->type = MSG_L2_HELLO;

	l2_send(m, BCAST_ADDR);
}

static void l2_send_ping(addr_t to)
{
	msg_t *m = msg_new(0);

	msg_header_t *mh = msg_get_header(m);
	mh->type = MSG_L2_PING;

	l2_send(m, to);
}

/* -------------------------------------------------------------------------- */

static void l2_recv_pre(addr_t addr)
{
	nb_iter_t nb_i;
	nb_i = l2_nb_find(addr);

	if(nb_i < 0) // not found
	{
		l2_nb_add(addr);
		l3_found(addr);
	}
	else layer2.nb[nb_i].timer = 0;
}

/* -------------------------------------------------------------------------- */

void l2_init(void)
{
}

void l2_tick(void)
{
	if(layer2.hello_cnt == HELLO_TIMER_MAX)
	{
		l2_send_hello();
		layer2.hello_cnt = 0;
	}
	else layer2.hello_cnt++;

	for(nb_iter_t i = 0; i < layer2.nb_l; i++)
	{
		if(layer2.nb[i].timer == NB_TIMER_MAX)
		{
			l2_send_ping(layer2.nb[i].addr);
		}

		layer2.nb[i].timer++;
	}
}

/* -------------------------------------------------------------------------- */

// TODO: binary search
uint8_t l2_nb_find(addr_t addr)
{
	for(nb_iter_t i = 0; i < layer2.nb_l; i++)
	{
		if(layer2.nb[i].addr == addr)
			return i;
	}
	return -1;
}

// TODO: sorted add
bool l2_nb_add(addr_t addr)
{
	if(layer2.nb_l < NUM_NB_MAX)
	{
		layer2.nb[layer2.nb_l].addr = addr;
		layer2.nb[layer2.nb_l].timer = 0;
		layer2.nb_l++;

		return true;
	}
	return false;
}

// TODO: sorted del
void l2_nb_del(addr_t addr)
{
	for(nb_iter_t i = 0; i < layer2.nb_l; i++)
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

void l2_on_send_failure(addr_t addr)
{
	l2_nb_del(addr);
	l3_died(addr);
}

void l2_send(msg_t *m, addr_t to)
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
		case MSG_PL_UNICAST:
			l3_recv_irq(m);
			break;

		default: // hello; ping;
			msg_free(m);
			break;
	}
}

/* -------------------------------------------------------------------------- */
