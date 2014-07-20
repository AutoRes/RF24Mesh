/* -------------------------------------------------------------------------- */

#include "mesh.h"

Mesh mesh;

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
	}
	else mesh.hello_cnt++;

	if(mesh.ogm_cnt == OGM_TIMER)
	{
		mesh_l3_send_ogm();
	}
	else mesh.ogm_cnt++;

	for(uint8_t i = 0; i < mesh.nb_l; i++)
	{
		if(mesh.nb[i].timer == NODE_TIMEOUT)
		{
			mesh_l2_send_ping(mesh.nb[i].addr);
		}
		else if(mesh.nb[i].timer == NODE_TIMEOUT+PONG_TIMER)
		{
			// TODO: node died
		}
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

void mesh_l2_send_hello(void)
{
	msg_t *m = msg_new(0);

	msg_header_t *mh = msg_get_header(m);
	mh->type = MSG_L2_HELLO;

	mesh_l2_send(m, BCAST_ADDR);
}

void mesh_l2_send_ping(uint8_t to)
{
	msg_t *m = msg_new(0);

	msg_header_t *mh = msg_get_header(m);
	mh->type = MSG_L2_PING;

	mesh_l2_send(m, to);
}

void mesh_l2_send_pong(uint8_t to)
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

void mesh_l3_send(msg_t *m, uint8_t to)
{
	msg_header_t *mh = msg_get_header(m);
	mh->l3_src = radio.self_addr;
	mh->l3_dst = to;

	if(to == BCAST_ADDR)
		mh->seq = mesh.nodes[radio.self_addr].seq+1;

	mesh_l3_forward(m);
}

void mesh_l3_send_ogm()
{
	msg_t *m = msg_new(0);

	msg_header_t *mh = msg_get_header(m);
	mh->type = MSG_L3_OGM;

	mesh_l3_send(m, BCAST_ADDR);
}

void mesh_l3_send_rogm(uint8_t addr)
{
	msg_t *m = msg_new(sizeof(uint8_t));

	msg_header_t *mh = msg_get_header(m);
	mh->type = MSG_L3_ROGM;

	uint8_t *pl = msg_get_pl(m);
	*pl = addr;

	mesh_l3_send(m, BCAST_ADDR);
}

void mesh_l3_send_known(void)
{
	for(uint8_t i = 1; i <= NODES_MAX; i++)
	{
		if(mesh.nodes[i].hop)
		{
			msg_t *m = msg_new(sizeof(uint8_t));

			msg_header_t *mh = msg_get_header(m);
			mh->type = MSG_L3_KNOWN;

			uint8_t *pl = msg_get_pl(m);
			*pl = i;

			mesh_l3_send(m, BCAST_ADDR);
		}
	}
}

/* -------------------------------------------------------------------------- */
