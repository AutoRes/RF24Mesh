#include "SPI.h"
#include "TimerOne.h"
#include "mesh.h"

const addr_t my_addr = 0x01;

void setup()
{
	Serial.begin(57600);
	mesh_init(
		my_addr,
		irq,
		DEFAULT_IRQ_N,
		DEFAULT_CEPIN,
		DEFAULT_CSPIN,
		DEFAULT_MESH_ID
	);

}

void loop()
{
      msg_t *m = msg_new(sizeof(uint8_t), false);
      msg_get_pl(m)[0] = my_addr;
      mesh_send(m, BCAST_ADDR);

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
