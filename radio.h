#ifndef __RADIO__
#define __RADIO__

#include "nRF24L01.h"
#include "RF24.h"

typedef struct
{
	RF24 *rf24;

	uint8_t bcast_addr;
	uint8_t self_addr;
} Radio;

extern Radio radio;

void radio_init(uint8_t _cepin, uint8_t _cspin,
	uint8_t bcast_addr, uint8_t self_addr);


bool radio_send(bool bcast, uint8_t addr,
	uint8_t pl[], uint8_t len);


uint8_t *radio_recv(uint8_t *_len);

#endif
