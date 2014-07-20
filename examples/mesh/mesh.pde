#include "SPI.h"
#include "RF24.h"
#include "radio.h"
#include "msg.h"

void setup()
{
	// same pins as RF24 library.
	mesh_init(0x02);
}

void loop()
{
	mesh_tick();

	delay(10000);
}
