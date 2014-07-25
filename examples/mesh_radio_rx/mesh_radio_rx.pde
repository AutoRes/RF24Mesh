#include "SPI.h"
#include "RF24.h"
#include "radio.h"
#include "printf.h"
#include "msg.h"

void l2_recv_irq(msg_t *m)
{
	Serial.print(m->pl[0], HEX);
	Serial.print(" - ");
	Serial.println(m->dst, HEX);
	free(m);
}

void setup()
{
	Serial.begin(57600);
	// same pins as RF24 library.
	radio_init(0x01);
	Serial.println("debug-setup");
}

void loop()
{
}
