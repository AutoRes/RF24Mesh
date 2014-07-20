#include "SPI.h"
#include "RF24.h"
#include "radio.h"
#include "msg.h"

void setup()
{
	// same pins as RF24 library.
	radio_init(0x01);
	l2_init();
	l3_init();
}

void loop()
{
	l2_tick();
	l3_tick();

	delay(10000);
}
