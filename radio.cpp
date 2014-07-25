/* -------------------------------------------------------------------------- */

#include "radio.h"

Radio radio;

/* -------------------------------------------------------------------------- */

static uint64_t addr2pipe(uint8_t addr)
{
	return 0xC2C2C2C200LL | addr;
}

static void adjust_pipes()
{
	radio.rf24->openReadingPipe(BCAST_PIPE, addr2pipe(BCAST_ADDR));
	radio.rf24->openReadingPipe(SELF_PIPE, addr2pipe(radio.self_addr));

	radio.rf24->setAutoAck(BCAST_PIPE, false);
	radio.rf24->setAutoAck(SELF_PIPE, true);
}

static void _radio_send(void)
{
	msg_t *m = (msg_t*)queue_get(&radio.tx);

	if(m)
	{
		radio.listening = false;
		radio.rf24->stopListening();

		if(m->dst == BCAST_ADDR)
			radio.rf24->setAutoAck(false);
		else
			radio.rf24->setAutoAck(true);

		radio.last_dst_addr = m->dst;
		radio.rf24->openWritingPipe(addr2pipe(m->dst));
		radio.rf24->startWrite(m->pl, m->len);

		msg_free(m);
	}
	else
	{
		adjust_pipes();
		radio.rf24->startListening();
		radio.listening = true;
	}
}

static void _radio_recv(void)
{
	msg_t *m;
	uint8_t pipe;

	if(radio.rf24->available(&pipe))
	{
		uint8_t len = radio.rf24->getDynamicPayloadSize();
		m = msg_new(len, true);

		if(pipe == SELF_PIPE)
			m->dst = radio.self_addr;
		else
			m->dst = BCAST_ADDR;

		radio.rf24->read(m->pl, m->len);
		l2_recv_irq(m);
	}
}

static void radio_irq(void)
{
	bool tx_ok, tx_fail, rx_ready;
	radio.rf24->whatHappened(tx_ok, tx_fail, rx_ready);

	if(tx_ok || tx_fail)
	{
		if(tx_fail)
		{
			l3_died(radio.last_dst_addr);
		}

		_radio_send();
	}

	if(rx_ready)
	{
		_radio_recv();
	}
}

/* -------------------------------------------------------------------------- */

void radio_init(uint8_t self_addr, uint8_t irq_n, uint8_t cepin, uint8_t cspin)
{
	radio.rf24 = new RF24(cepin, cspin);
	attachInterrupt(irq_n, radio_irq, FALLING);

	radio.rf24->begin();
	radio.rf24->setRetries(RETRY_DELAY, RETRY_MAX);
	radio.rf24->enableDynamicPayloads();

	radio.self_addr = self_addr;
	adjust_pipes();
	
	radio.rf24->startListening();
	radio.listening = true;

	queue_head_init(&radio.tx);
}

void radio_send(msg_t *m)
{
	queue_put((queue_entry*)m, &radio.tx);
	if(radio.listening)
		_radio_send();
}

/* -------------------------------------------------------------------------- */
