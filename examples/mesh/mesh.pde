#include "SPI.h"
#include "TimerOne.h"
#include "mesh.h"

const uint8_t my_addr = 0x02;
const uint8_t nb_addr = 0x01;

void setup()
{
	Serial.begin(57600);
	mesh_init(my_addr, irq);
}

void loop()
{
      msg_t *m = msg_new(sizeof(uint8_t));
      msg_get_pl(m)[0] = my_addr;
      mesh_send(m, nb_addr);

      delay(100); // ms
}

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
