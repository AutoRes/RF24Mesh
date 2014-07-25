#include "SPI.h"
#include "RF24.h"
#include "radio.h"
#include "printf.h"
#include "msg.h"

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
