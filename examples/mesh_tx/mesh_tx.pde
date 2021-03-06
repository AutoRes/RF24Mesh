#include "SPI.h"
#include "TimerOne.h"
#include "mesh.h"

void setup()
{
	Serial.begin(57600);
	// same pins as RF24 library.
	mesh_init(0x02);
}

void loop()
{
      msg_t *m = msg_new(sizeof(uint8_t));
      msg_get_pl(m)[0] = 0x01;
      mesh_send(m, 0x01);

      delay(100);
}
