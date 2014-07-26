#include "SPI.h"
#include "RF24.h"
#include "radio.h"
#include "msg.h"
#include "TimerOne.h"

void setup()
{
	Serial.begin(57600);
	// same pins as RF24 library.
	radio_init(0x02);
	l2_init();
        l3_init();
}

void loop()
{
      msg_t *m = msg_new(sizeof(uint8_t));
      msg_get_header(m)->type = MSG_PL;
      msg_get_pl(m)[0] = 0x01;
      l3_send(m, 0x01);

      delay(10);
}
