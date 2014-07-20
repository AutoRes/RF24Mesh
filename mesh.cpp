/* -------------------------------------------------------------------------- */

#include "mesh.h"

Mesh mesh;

/* -------------------------------------------------------------------------- */

static void mesh_l2_send_hello(void)
{
	msg_t *m = msg_new(0);

	msg_header_t *mh = msg_get_header(m);
	mh->type = MSG_L2_HELLO;

	mesh_l2_send(m, BCAST_ADDR);
}

static void mesh_l2_send_ping(uint8_t to)
{
	msg_t *m = msg_new(0);

	msg_header_t *mh = msg_get_header(m);
	mh->type = MSG_L2_PING;

	mesh_l2_send(m, to);
}

static void mesh_l2_send_pong(uint8_t to)
{
	msg_t *m = msg_new(0);

	msg_header_t *mh = msg_get_header(m);
	mh->type = MSG_L2_PONG;

	mesh_l2_send(m, to);
}

/* -------------------------------------------------------------------------- */

static void mesh_l3_forward(msg_t *m)
{
	msg_header_t *mh = msg_get_header(m);

	if(mh->l3_dst == BCAST_ADDR)
	{
		if(mh->type != MSG_PL_MULTICAST)
			mesh.nodes[mh->l3_src].seq = mh->seq;

		for(uint8_t i = 0; i < mesh.nb_l; i++)
		{
			msg_t *md = msg_dup(m);
			mesh_l2_send(md, mesh.nb[i].addr);
		}
		msg_free(m);
	}
	else
	{
		uint8_t hop = mesh.nodes[mh->l3_dst].hop;

		if(hop)
			mesh_l2_send(m, hop);
		else
			// node does not exist
			msg_free(m);
	}
}

static void mesh_l3_send_ogm()
{
	msg_t *m = msg_new(0);

	msg_header_t *mh = msg_get_header(m);
	mh->type = MSG_L3_OGM;

	mesh_l3_send(m, BCAST_ADDR);
}

static void mesh_l3_send_rogm(uint8_t addr)
{
	msg_t *m = msg_new(1);

	msg_header_t *mh = msg_get_header(m);
	mh->type = MSG_L3_ROGM;
	mh->pl[0] = addr;

	mesh_l3_send(m, BCAST_ADDR);
}

static void mesh_l3_send_known(void)
{
	for(uint8_t i = 1; i <= NODES_MAX; i++)
	{
		if(mesh.nodes[i].hop)
		{
			msg_t *m = msg_new(1);

			msg_header_t *mh = msg_get_header(m);
			mh->type = MSG_L3_KNOWN;
			mh->pl[0] = i;

			mesh_l3_send(m, BCAST_ADDR);
		}
	}
}

/* -------------------------------------------------------------------------- */

static void mesh_l2_recv_pre(uint8_t addr)
{
	uint8_t i;
	for(i = 0; i < mesh.nb_l; i++)
	{
		if(mesh.nb[i].addr == addr)
		{
			mesh.nb[i].timer = 0;
			break;
		}
	}
	if(i == mesh.nb_l)
	{
		mesh_node_found(addr);
	}

	mesh.nodes[addr].hop = addr;
}

static void mesh_l2_recv_ping(msg_t *m)
{
	msg_header_t *mh = msg_get_header(m);
	mesh_l2_send_pong(mh->l2_src);
	msg_free(m);
}

static bool mesh_l3_recv_broadcast_pre(msg_t *m)
{
	msg_header_t *mh = msg_get_header(m);

	if(mesh.nodes[mh->l3_src].seq != mh->seq)
	{
		mesh.nodes[mh->l3_src].hop = mh->l2_src;
		mesh.nodes[mh->l3_src].seq = mh->seq;
		return true;
	}

	msg_free(m);
	return false;
}

static void mesh_l3_recv_ogm(msg_t *m)
{
	if(mesh_l3_recv_broadcast_pre(m))
		mesh_l3_forward(m);
}

static void mesh_l3_recv_rogm(msg_t *m)
{
	if(mesh_l3_recv_broadcast_pre(m))
	{
		msg_header_t *mh = msg_get_header(m);
		uint8_t addr = mh->pl[0];

		mesh_l3_forward(m);

		if(addr = radio.self_addr)
			mesh_l3_send_ogm();
	}
}

static void mesh_l3_recv_known(msg_t *m)
{
	if(mesh_l3_recv_broadcast_pre(m))
	{
		msg_header_t *mh = msg_get_header(m);
		uint8_t addr = mh->pl[0];

		if(!mesh.nodes[addr].hop)
		{
			// TODO: add to nb
			mesh.nodes[addr].hop = mh->l2_src;

			mesh_l3_forward(m);
		}
		else msg_free(m);
	}
}

static void mesh_l3_recv_broadcast(msg_t *m)
{
	if(mesh_l3_recv_broadcast_pre(m))
	{
		// TODO: send to app
		mesh_l3_forward(msg_dup(m));
	}
}

/* -------------------------------------------------------------------------- */

void mesh_init(void)
{
	// TODO: attach timer interrupt
}

void mesh_tick(void)
{
	if(mesh.hello_cnt == HELLO_TIMER)
	{
		mesh_l2_send_hello();
		mesh.hello_cnt = 0;
	}
	else mesh.hello_cnt++;

	if(mesh.ogm_cnt == OGM_TIMER)
	{
		mesh_l3_send_ogm();
		mesh.ogm_cnt = 0;
	}
	else mesh.ogm_cnt++;

	for(uint8_t i = 0; i < mesh.nb_l; i++)
	{
		if(mesh.nb[i].timer == NODE_TIMEOUT)
			mesh_l2_send_ping(mesh.nb[i].addr);
		else if(mesh.nb[i].timer == NODE_TIMEOUT+PONG_TIMER)
			mesh_node_died(mesh.nb[i].addr);

		mesh.nb[i].timer++;
	}
}

/* -------------------------------------------------------------------------- */

void mesh_l2_send(msg_t *m, uint8_t to)
{
	msg_header_t *mh = msg_get_header(m);
	mh->l2_src = radio.self_addr;

	m->dst = to;
	radio_send(m);
}

void mesh_l3_send(msg_t *m, uint8_t to)
{
	msg_header_t *mh = msg_get_header(m);
	mh->l3_src = radio.self_addr;
	mh->l3_dst = to;

	if(to == BCAST_ADDR && mh->type != MSG_PL_MULTICAST)
	{
		mh->seq = ++mesh.nodes[radio.self_addr].seq;
		mesh.ogm_cnt = 0;
	}

	mesh_l3_forward(m);
}

/* -------------------------------------------------------------------------- */

void mesh_l2_recv_irq(void)
{
	msg_t *m = radio_recv();
	msg_header_t *mh = msg_get_header(m);

	// optimization
	mesh_l2_recv_pre(mh->l2_src);

	switch(mh->type)
	{
		case MSG_L2_PING:
			mesh_l2_recv_ping(m);
			break;

		case MSG_L3_OGM:
			mesh_l3_recv_ogm(m);
			break;

		case MSG_L3_ROGM:
			mesh_l3_recv_rogm(m);
			break;

		case MSG_L3_KNOWN:
			mesh_l3_recv_known(m);
			break;

		case MSG_PL_BROADCAST:
			mesh_l3_recv_broadcast(m);
			break;

		case MSG_PL_MULTICAST:
		case MSG_PL:
			// TODO: send to app
			break;

		default: // hello; pong;
			msg_free(m);
			break;
	}
}

/* -------------------------------------------------------------------------- */

void mesh_node_died(uint8_t addr)
{
	// TODO
}

void mesh_node_found(uint8_t addr)
{
	// TODO
}

/* -------------------------------------------------------------------------- */
