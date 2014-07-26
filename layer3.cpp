/* -------------------------------------------------------------------------- */

#include "layer3.h"

Layer3 layer3;

/* -------------------------------------------------------------------------- */

static void l3_forward(msg_t *m)
{
	msg_header_t *mh = msg_get_header(m);

	if(mh->l3_dst == BCAST_ADDR)
	{
		if(mh->type != MSG_PL_MULTICAST)
			layer3.nodes[mh->l3_src].seq = mh->seq;

		for(uint8_t i = 0; i < layer2.nb_l; i++)
		{
			msg_t *md = msg_dup(m);
			l2_send(md, layer2.nb[i].addr);
		}
		msg_free(m);
	}
	else
	{
		uint8_t hop = layer3.nodes[mh->l3_dst].hop;

		if(hop)
			l2_send(m, hop);
		else
			// TODO: send alert - node does not exist
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

static void l3_send_rogm(uint8_t addr)
{
	msg_t *m = msg_new(1);

	msg_header_t *mh = msg_get_header(m);
	mh->type = MSG_L3_ROGM;
	mh->pl[0] = addr;

	l3_send(m, BCAST_ADDR);
}

static void l3_send_known(void)
{
	for(uint8_t i = 1; i <= NODES_MAX; i++)
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

	msg_free(m);
	return false;
}

static void l3_recv_ogm(msg_t *m)
{
	if(l3_recv_broadcast_pre(m))
	{
		l3_forward(m);
	}
}

static void l3_recv_rogm(msg_t *m)
{
	if(l3_recv_broadcast_pre(m))
	{
		msg_header_t *mh = msg_get_header(m);
		uint8_t addr = mh->pl[0];

		l3_forward(m);

		if(addr != radio.self_addr)
		{
			l2_del_nb(addr);
			layer3.nodes[addr].hop = 0;
		}
		else l3_send_ogm();
	}
}

static void l3_recv_known(msg_t *m)
{
	if(l3_recv_broadcast_pre(m))
	{
		msg_header_t *mh = msg_get_header(m);
		uint8_t addr = mh->pl[0];

		if(!layer3.nodes[addr].hop && addr != radio.self_addr)
		{
			layer3.nodes[addr].hop = mh->l2_src;

			l3_forward(m);
		}
		else msg_free(m);
	}
}

static void l3_recv_pl_broadcast(msg_t *m)
{
	if(l3_recv_broadcast_pre(m))
	{
		l3_forward(msg_dup(m));
		mesh_recv_irq(m);
	}
}

static void l3_recv_pl_multicast(msg_t *m)
{
	mesh_recv_irq(m);
}

static void l3_recv_pl_unicast(msg_t *m)
{
	msg_header_t *mh = msg_get_header(m);

	if(mh->l3_dst == radio.self_addr)
		mesh_recv_irq(m);
	else
		l3_forward(m);
}

/* -------------------------------------------------------------------------- */

void l3_init(void)
{
}

void l3_tick(void)
{
	if(layer3.ogm_cnt == OGM_TIMER)
	{
		l3_send_ogm();
		layer3.ogm_cnt = 0;
	}
	else layer3.ogm_cnt++;
}

/* -------------------------------------------------------------------------- */

void l3_send(msg_t *m, uint8_t to)
{
	msg_header_t *mh = msg_get_header(m);
	mh->l3_src = radio.self_addr;
	mh->l3_dst = to;

	if(to == BCAST_ADDR && mh->type != MSG_PL_MULTICAST)
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

		case MSG_PL_BROADCAST:
			l3_recv_pl_broadcast(m);
			break;

		case MSG_PL_MULTICAST:
			l3_recv_pl_multicast(m);
			break;

		case MSG_PL_UNICAST:
			l3_recv_pl_unicast(m);
			break;

		default:
			msg_free(m);
			break;
	}
}

/* -------------------------------------------------------------------------- */

void l3_died(uint8_t addr)
{
	layer3.nodes[addr].hop = 0;
	l3_send_rogm(addr);
}

void l3_found(uint8_t addr)
{
	l3_send_known();
	layer3.nodes[addr].hop = addr;
}

/* -------------------------------------------------------------------------- */
