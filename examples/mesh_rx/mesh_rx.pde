#include "SPI.h"
#include "TimerOne.h"
#include "RF24.h"
#include "msg.h"
#include "mesh.h"

void setup()
{
	Serial.begin(57600);
	// same pins as RF24 library.
	mesh_init(0x01);
	Serial.println("setup");
}

static uint8_t i = 0;
void loop()
{
	msg_t *m;
	
	while(m = mesh_recv())
	{
		uint8_t *pl = msg_get_pl(m);
		Serial.println(pl[0], HEX);
		msg_free(m);
		i++;
	}

	delay(1);
}
