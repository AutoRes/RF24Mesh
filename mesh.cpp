#include "mesh.h"
#include "TimerOne.h"

Mesh mesh;

void mesh_init(uint8_t self_addr)
{
	radio_init(self_addr);
	l2_init();
	l3_init();

	queue_head_init(&mesh.rx);

#define TICK_uS 50000
	Timer1.initialize(TICK_uS);
	Timer1.attachInterrupt(mesh_tick);
}

void mesh_tick(void)
{
	l2_tick();
	l3_tick();
}

/*
	Available types:
	- MSG_PL_BROADCAST
	- MSG_PL_MULTICAST
	- MSG_PL
*/
void mesh_send(msg_t *m, uint8_t to, uint8_t type)
{
	msg_header_t *mh = msg_get_header(m);
	mh->type = type;
	l3_send(m, to);
}

void mesh_recv_irq(msg_t *m)
{
	queue_put((queue_entry*)m,&mesh.rx);
	// app_irq
}

msg_t *mesh_recv(void)
{
	return (msg_t*)queue_get(&mesh.rx);
}
