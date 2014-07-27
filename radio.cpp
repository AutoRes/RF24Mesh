/* -------------------------------------------------------------------------- */

#include "RF24.h"

#include "radio.h"
#include "layer2.h"

#include "queue.h"
#include "msg.h"

Radio radio;

/* -------------------------------------------------------------------------- */

static uint64_t addr2pipe(uint8_t addr)
{
	return (((uint64_t)radio.mesh_id)<< 8) | addr;
}

static void adjust_pipes()
{
	rf24_openReadingPipe(BCAST_PIPE, addr2pipe(BCAST_ADDR));
	rf24_openReadingPipe(SELF_PIPE, addr2pipe(radio.self_addr));

	rf24_setAutoAck(BCAST_PIPE, false);
	rf24_setAutoAck(SELF_PIPE, true);
}

static void _radio_send(void)
{
	msg_t *m = (msg_t*)queue_get(&radio.tx);

	if(m)
	{
		radio.listening = false;
		rf24_stopListening();

		if(m->dst == BCAST_ADDR)
			rf24_setAutoAck(false);
		else
			rf24_setAutoAck(true);

		radio.last_dst_addr = m->dst;
		rf24_openWritingPipe(addr2pipe(m->dst));
		rf24_startWrite(m->pl, m->len);

		msg_free(m);
	}
	else
	{
		adjust_pipes();
		rf24_startListening();
		radio.listening = true;
	}
}

static void _radio_recv(void)
{
	msg_t *m;
	uint8_t pipe;

	if(rf24_available(&pipe))
	{
		uint8_t len = rf24_getDynamicPayloadSize();
		m = msg_new(len, true);

		if(pipe == SELF_PIPE)
			m->dst = radio.self_addr;
		else
			m->dst = BCAST_ADDR;

		rf24_read(m->pl, m->len);
		l2_recv_irq(m);
	}
}

/* -------------------------------------------------------------------------- */

void radio_init(uint32_t mesh_id, addr_t self_addr)
{
	rf24_begin();
	rf24_setRetries(SEND_RETRY_DELAY, SEND_NUM_RETRY_MAX);
	rf24_enableDynamicPayloads();

	radio.mesh_id = mesh_id;
	radio.self_addr = self_addr;
	adjust_pipes();
	
	rf24_startListening();
	radio.listening = true;

	queue_head_init(&radio.tx);
}

void radio_irq(void)
{
	bool tx_ok, tx_fail, rx_ready;
	rf24_whatHappened(tx_ok, tx_fail, rx_ready);

	if(tx_ok || tx_fail)
	{
		if(tx_fail)
		{
			l2_on_send_failure(radio.last_dst_addr);
		}

		_radio_send();
	}

	if(rx_ready)
	{
		_radio_recv();
	}
}

void radio_send(msg_t *m)
{
	queue_put((queue_entry*)m, &radio.tx);
	if(radio.listening)
		_radio_send();
}

/* -------------------------------------------------------------------------- */
