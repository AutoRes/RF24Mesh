#include "SPI.h"
#include "TimerOne.h"
#include "mesh.h"

void irq(void)
{
	msg_t *m;
	
	while(m = mesh_recv())
	{
		uint8_t *pl = msg_get_pl(m);
		Serial.println(pl[0], HEX);
		msg_free(m);
	}
}

void setup()
{
	Serial.begin(57600);
	// same pins as RF24 library.
	mesh_init(0x01, irq);
	Serial.println("setup");
}

void loop()
{
}
