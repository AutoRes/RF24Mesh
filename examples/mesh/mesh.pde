#include "SPI.h"
#include "RF24.h"
#include "radio.h"

void setup() {
  // same pins as RF24 library.
  radio_init(9, 10, 0xFF, 0x01);

}

void loop() {
  uint8_t *ptr;

  if ((ptr = radio_recv(NULL)) != NULL) {
    char *s = "hello world";
    bool ok = radio_send(false, 0x02, (uint8_t *)s, sizeof(s));
    free(ptr);
  }

}
