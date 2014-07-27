/* -------------------------------------------------------------------------- */

#include "RF24.h"
#include "TimerOne.h"

#include "radio.h"
#include "layer2.h"
#include "layer3.h"
#include "mesh.h"

#include "queue.h"
#include "msg.h"

Mesh mesh;

/* -------------------------------------------------------------------------- */

void mesh_init(uint8_t self_addr,
		irq_t app_irq,
		uint8_t irq_n,
		uint8_t cepin,
		uint8_t cspin,
		uint32_t mesh_id)
{
	rf24_init(cepin, cspin);
	radio_init(mesh_id, self_addr);
	l2_init();
	l3_init();

	mesh.app_irq = app_irq;
	queue_head_init(&mesh.rx);

	Timer1.initialize(TICK_uS);
	Timer1.attachInterrupt(mesh_tick);

	attachInterrupt(irq_n, radio_irq, FALLING);
}

void mesh_tick(void)
{
	l2_tick();
	l3_tick();
}

void mesh_send(msg_t *m, addr_t to)
{
	msg_header_t *mh = msg_get_header(m);
	mh->type = MSG_PL;
	l3_send(m, to);
}

void mesh_recv_irq(msg_t *m)
{
	queue_put((queue_entry*)m,&mesh.rx);

	if(mesh.app_irq)
		mesh.app_irq();
}

msg_t *mesh_recv(void)
{
	return (msg_t*)queue_get(&mesh.rx);
}

/* -------------------------------------------------------------------------- */
