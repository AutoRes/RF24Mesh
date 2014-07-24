#include "SPI.h"
#include "RF24.h"
#include "radio.h"
#include "msg.h"

void l2_recv_irq(msg_t *m)
{
}

void _l3_died(uint8_t addr)
{
	Serial.println("died.\n");
}

void setup()
{
	Serial.begin(57600);
	// same pins as RF24 library.
	radio_init(0x02);
}

void loop()
{
	static uint8_t pl;
	size_t len = 1;
	for (int i=0; i<10; i++) {
		msg_t *m = msg_new(len, true);
		m->dst = 0x01;
		m->pl[0] = pl++;
		radio_send(m);
	}

	delay(1000);
}
