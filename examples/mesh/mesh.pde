#include "SPI.h"
#include "RF24.h"
#include "radio.h"
#include "msg.h"

void setup() {
	// same pins as RF24 library.
	radio_init(0x01);
}

void loop() {
	msg_t *ptr;

	if ((ptr = radio_recv()) != NULL) {
		msg_t *m = msg_new(1);
		m->pl[0]= 10;
		m->dst = 0x02;

		radio_send(m);
		free(ptr);
	}
}
