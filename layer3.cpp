/* -------------------------------------------------------------------------- */

#include "radio.h"
#include "layer2.h"
#include "layer3.h"
#include "mesh.h"

#include "msg.h"

Layer3 layer3;

/* -------------------------------------------------------------------------- */

static void l3_forward(msg_t *m)
{
	msg_header_t *mh = msg_get_header(m);

	if(mh->l3_dst == BCAST_ADDR)
	{
		layer3.nodes[mh->l3_src].seq = mh->seq;

		for(nb_iter_t i = 0; i < layer2.nb_l; i++)
			l2_send(msg_dup(m), layer2.nb[i].addr);

		msg_free(m);
	}
	else
	{
		uint8_t hop = layer3.nodes[mh->l3_dst].hop;

		if(hop)
			l2_send(m, hop);
		else
			// TODO: node does not exist
			msg_free(m);
	}
}

static void l3_send_ogm()
{
	msg_t *m = msg_new(0);

	msg_header_t *mh = msg_get_header(m);
	mh->type = MSG_L3_OGM;

	l3_send(m, BCAST_ADDR);
}

static void l3_send_rogm(addr_t addr)
{
	msg_t *m = msg_new(1);

	msg_header_t *mh = msg_get_header(m);
	mh->type = MSG_L3_ROGM;
	mh->pl[0] = addr;

	l3_send(m, BCAST_ADDR);
}

static void l3_send_known(void)
{
	for(addr_t i = 1; i <= NUM_NODES_MAX; i++)
	{
		if(layer3.nodes[i].hop)
		{
			msg_t *m = msg_new(1);

			msg_header_t *mh = msg_get_header(m);
			mh->type = MSG_L3_KNOWN;
			mh->pl[0] = i;

			l3_send(m, BCAST_ADDR);
		}
	}
}

/* -------------------------------------------------------------------------- */

static bool l3_recv_broadcast_pre(msg_t *m)
{
	msg_header_t *mh = msg_get_header(m);

	if(layer3.nodes[mh->l3_src].seq != mh->seq)
	{
		layer3.nodes[mh->l3_src].hop = mh->l2_src;
		layer3.nodes[mh->l3_src].seq = mh->seq;
		return true;
	}

	return false;
}

static void l3_recv_ogm(msg_t *m)
{
	if(l3_recv_broadcast_pre(m))
	{
		l3_forward(m);
	}
	else msg_free(m);
}

static void l3_recv_rogm(msg_t *m)
{
	if(l3_recv_broadcast_pre(m))
	{
		msg_header_t *mh = msg_get_header(m);
		addr_t addr = mh->pl[0];

		l3_forward(m);

		if(addr != radio.self_addr)
		{
			l2_nb_del(addr);
			layer3.nodes[addr].hop = 0;
		}
		else l3_send_ogm();
	}
	else msg_free(m);
}

static void l3_recv_known(msg_t *m)
{
	if(l3_recv_broadcast_pre(m))
	{
		msg_header_t *mh = msg_get_header(m);
		addr_t addr = mh->pl[0];

		if(!layer3.nodes[addr].hop && addr != radio.self_addr)
		{
			layer3.nodes[addr].hop = mh->l2_src;

			l3_forward(m);
		}
		else msg_free(m);
	}
	else msg_free(m);
}

static void l3_recv_pl(msg_t *m)
{
	msg_header_t *mh = msg_get_header(m);

	if(mh->l3_dst == BCAST_ADDR)
	{
		if(l3_recv_broadcast_pre(m))
		{
			l3_forward(msg_dup(m));
			mesh_recv_irq(m);
		}
		else msg_free(m);
	}
	else // unicast
	{
		if(mh->l3_dst == radio.self_addr)
			mesh_recv_irq(m);
		else
			l3_forward(m);
	}
}

/* -------------------------------------------------------------------------- */

void l3_init(void)
{
}

void l3_tick(void)
{
	if(layer3.ogm_cnt == OGM_TIMER_MAX)
	{
		l3_send_ogm();
		layer3.ogm_cnt = 0;
	}
	else layer3.ogm_cnt++;
}

/* -------------------------------------------------------------------------- */

void l3_send(msg_t *m, addr_t to)
{
	msg_header_t *mh = msg_get_header(m);
	mh->l3_src = radio.self_addr;
	mh->l3_dst = to;

	if(to == BCAST_ADDR)
	{
		mh->seq = ++layer3.nodes[radio.self_addr].seq;
		layer3.ogm_cnt = 0;
	}

	l3_forward(m);
}

void l3_recv_irq(msg_t *m)
{
	msg_header_t *mh = msg_get_header(m);

	switch(mh->type)
	{
		case MSG_L3_OGM:
			l3_recv_ogm(m);
			break;

		case MSG_L3_ROGM:
			l3_recv_rogm(m);
			break;

		case MSG_L3_KNOWN:
			l3_recv_known(m);
			break;

		case MSG_PL:
			l3_recv_pl(m);
			break;

		default:
			msg_free(m);
			break;
	}
}

/* -------------------------------------------------------------------------- */

void l3_died(addr_t addr)
{
	layer3.nodes[addr].hop = 0;
	l3_send_rogm(addr);
}

void l3_found(addr_t addr)
{
	layer3.nodes[addr].hop = addr;
	l3_send_known();
}

/* -------------------------------------------------------------------------- */
