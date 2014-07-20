#include "radio.h"

#define BCAST_PIPE_N 1
#define SELF_PIPE_N  2

Radio radio;

static uint64_t addr2pipe(uint8_t addr)
{
	return 0xC2C2C2C200LL | addr;
}

static void adjust_acks()
{
	radio.rf24->openReadingPipe (BCAST_PIPE_N, addr2pipe(radio.bcast_addr));
	radio.rf24->openReadingPipe (SELF_PIPE_N, addr2pipe(radio.self_addr));
}

void radio_init(uint8_t _cepin, uint8_t _cspin,
	uint8_t bcast_addr, uint8_t self_addr)
{
	radio.rf24 = new RF24(_cepin, _cspin);

	radio.rf24->begin();
	radio.rf24->enableDynamicPayloads();

	radio.bcast_addr = bcast_addr;
	radio.self_addr = self_addr;
	adjust_acks();
	
	radio.rf24->startListening();
}

bool radio_send(bool bcast, uint8_t addr,
	uint8_t pl[], uint8_t len)
{
	radio.rf24->stopListening();

	if(bcast)
		radio.rf24->setAutoAck(false);

	radio.rf24->openWritingPipe(addr2pipe(addr));
	radio.rf24->write(pl, len);

	if(bcast)
		adjust_acks();
	
	radio.rf24->startListening();
}

uint8_t *radio_recv(uint8_t *_len)
{
	uint8_t *pl = NULL;

	if(radio.rf24->available())
	{
		uint8_t len = radio.rf24->getDynamicPayloadSize();
		pl = (uint8_t*)malloc(sizeof(uint8_t)*len);
		radio.rf24->read(pl, len);
		if (_len) *_len = len;
	}

	return pl;
}
