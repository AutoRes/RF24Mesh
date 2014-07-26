#include "SPI.h"
#include "TimerOne.h"
#include "RF24.h"
#include "printf.h"
#include "msg.h"
#include "radio.h"
#include "layer3.h"

void mesh_recv_irq(msg_t *m)
{
	uint8_t *pl = msg_get_pl(m);	
	Serial.println(pl[0], HEX);
	msg_free(m);
}

void setup()
{
	Serial.begin(57600);
	// same pins as RF24 library.
	radio_init(0x01);
	l2_init();
	l3_init();
	Serial.println("setup");
}

void loop()
{
}
