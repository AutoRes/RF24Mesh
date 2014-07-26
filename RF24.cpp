/*
   Copyright (C) 2011 J. Coliz <maniacbug@ymail.com>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   version 2 as published by the Free Software Foundation.
   */

#include "nRF24L01.h"
#include "RF24_config.h"
#include "RF24.h"

RF24 rf24;

/****************************************************************************/

static void rf24_csn(int mode)
{
  // Minimum ideal SPI bus speed is 2x data rate
  // If we assume 2Mbs data rate and 16Mhz clock, a
  // divider of 4 is the minimum we want.
  // CLK:BUS 8Mhz:2Mhz, 16Mhz:4Mhz, or 20Mhz:5Mhz
#ifdef ARDUINO
  SPI.setBitOrder(MSBFIRST);
  SPI.setDataMode(SPI_MODE0);
  SPI.setClockDivider(SPI_CLOCK_DIV4);
#endif
  digitalWrite(rf24.csn_pin,mode);
}

/****************************************************************************/

static void rf24_ce(int level)
{
  digitalWrite(rf24.ce_pin,level);
}

/****************************************************************************/

static uint8_t rf24_read_register(uint8_t reg, uint8_t* buf, uint8_t len)
{
  uint8_t status;

  rf24_csn(LOW);
  status = SPI.transfer( R_REGISTER | ( REGISTER_MASK & reg ) );
  while ( len-- )
    *buf++ = SPI.transfer(0xff);

  rf24_csn(HIGH);

  return status;
}

/****************************************************************************/

static uint8_t rf24_read_register(uint8_t reg)
{
  rf24_csn(LOW);
  SPI.transfer( R_REGISTER | ( REGISTER_MASK & reg ) );
  uint8_t result = SPI.transfer(0xff);

  rf24_csn(HIGH);
  return result;
}

/****************************************************************************/

static uint8_t rf24_write_register(uint8_t reg, const uint8_t* buf, uint8_t len)
{
  uint8_t status;

  rf24_csn(LOW);
  status = SPI.transfer( W_REGISTER | ( REGISTER_MASK & reg ) );
  while ( len-- )
    SPI.transfer(*buf++);

  rf24_csn(HIGH);

  return status;
}

/****************************************************************************/

static uint8_t rf24_write_register(uint8_t reg, uint8_t value)
{
  uint8_t status;

  IF_SERIAL_DEBUG(printf_P(PSTR("rf24_write_register(%02x,%02x)\r\n"),reg,value));

  rf24_csn(LOW);
  status = SPI.transfer( W_REGISTER | ( REGISTER_MASK & reg ) );
  SPI.transfer(value);
  rf24_csn(HIGH);

  return status;
}

/****************************************************************************/

static uint8_t rf24_write_payload(const void* buf, uint8_t len)
{
  uint8_t status;

  const uint8_t* current = reinterpret_cast<const uint8_t*>(buf);

  uint8_t data_len = min(len,rf24.payload_size);
  uint8_t blank_len = rf24.dynamic_payloads_enabled ? 0 : rf24.payload_size - data_len;

  //printf("[Writing %u bytes %u blanks]",data_len,blank_len);

  rf24_csn(LOW);
  status = SPI.transfer( W_TX_PAYLOAD );
  while ( data_len-- )
    SPI.transfer(*current++);
  while ( blank_len-- )
    SPI.transfer(0);
  rf24_csn(HIGH);

  return status;
}

/****************************************************************************/

static uint8_t rf24_read_payload(void* buf, uint8_t len)
{
  uint8_t status;
  uint8_t* current = reinterpret_cast<uint8_t*>(buf);

  uint8_t data_len = min(len,rf24.payload_size);
  uint8_t blank_len = rf24.dynamic_payloads_enabled ? 0 : rf24.payload_size - data_len;

  //printf("[Reading %u bytes %u blanks]",data_len,blank_len);

  rf24_csn(LOW);
  status = SPI.transfer( R_RX_PAYLOAD );
  while ( data_len-- )
    *current++ = SPI.transfer(0xff);
  while ( blank_len-- )
    SPI.transfer(0xff);
  rf24_csn(HIGH);

  return status;
}

/****************************************************************************/

static uint8_t rf24_flush_rx(void)
{
  uint8_t status;

  rf24_csn(LOW);
  status = SPI.transfer( FLUSH_RX );
  rf24_csn(HIGH);

  return status;
}

/****************************************************************************/

static uint8_t rf24_flush_tx(void)
{
  uint8_t status;

  rf24_csn(LOW);
  status = SPI.transfer( FLUSH_TX );
  rf24_csn(HIGH);

  return status;
}

/****************************************************************************/

static uint8_t rf24_get_status(void)
{
  uint8_t status;

  rf24_csn(LOW);
  status = SPI.transfer( NOP );
  rf24_csn(HIGH);

  return status;
}

/****************************************************************************/

static void rf24_print_status(uint8_t status)
{
  printf_P(PSTR("STATUS\t\t = 0x%02x RX_DR=%x TX_DS=%x MAX_RT=%x RX_P_NO=%x TX_FULL=%x\r\n"),
      status,
      (status & _BV(RX_DR))?1:0,
      (status & _BV(TX_DS))?1:0,
      (status & _BV(MAX_RT))?1:0,
      ((status >> RX_P_NO) & B111),
      (status & _BV(TX_FULL))?1:0
      );
}

/****************************************************************************/

static void rf24_print_observe_tx(uint8_t value)
{
  printf_P(PSTR("OBSERVE_TX=%02x: POLS_CNT=%x ARC_CNT=%x\r\n"),
      value,
      (value >> PLOS_CNT) & B1111,
      (value >> ARC_CNT) & B1111
      );
}

/****************************************************************************/

static void rf24_print_byte_register(const char* name, uint8_t reg, uint8_t qty = 1)
{
  char extra_tab = strlen_P(name) < 8 ? '\t' : 0;
  printf_P(PSTR(PRIPSTR"\t%c ="),name,extra_tab);
  while (qty--)
    printf_P(PSTR(" 0x%02x"),rf24_read_register(reg++));
  printf_P(PSTR("\r\n"));
}

/****************************************************************************/

static void rf24_print_address_register(const char* name, uint8_t reg, uint8_t qty = 1)
{
  char extra_tab = strlen_P(name) < 8 ? '\t' : 0;
  printf_P(PSTR(PRIPSTR"\t%c ="),name,extra_tab);

  while (qty--)
  {
    uint8_t buffer[5];
    rf24_read_register(reg++,buffer,sizeof buffer);

    printf_P(PSTR(" 0x"));
    uint8_t* bufptr = buffer + sizeof buffer;
    while( --bufptr >= buffer )
      printf_P(PSTR("%02x"),*bufptr);
  }

  printf_P(PSTR("\r\n"));
}

/****************************************************************************/

void rf24_init(uint8_t _cepin, uint8_t _cspin)
{
  rf24.ce_pin = _cepin;
  rf24.ce_pin = _cepin;
  rf24.csn_pin = _cspin;
  rf24.wide_band = true;
  rf24.p_variant = false;
  rf24.payload_size = 32;
  rf24.ack_payload_available = false;
  rf24.dynamic_payloads_enabled = false;
  rf24.pipe0_reading_address = 0;
}

/****************************************************************************/

void rf24_setChannel(uint8_t channel)
{
  // TODO: This method could take advantage of the 'rf24.wide_band' calculation
  // done in rf24_setChannel() to require certain channel spacing.

  const uint8_t max_channel = 127;
  rf24_write_register(RF_CH,min(channel,max_channel));
}

/****************************************************************************/

void rf24_setPayloadSize(uint8_t size)
{
  const uint8_t max_payload_size = 32;
  rf24.payload_size = min(size,max_payload_size);
}

/****************************************************************************/

uint8_t rf24_getPayloadSize(void)
{
  return rf24.payload_size;
}

/****************************************************************************/

static const char rf24_datarate_e_str_0[] PROGMEM = "1MBPS";
static const char rf24_datarate_e_str_1[] PROGMEM = "2MBPS";
static const char rf24_datarate_e_str_2[] PROGMEM = "250KBPS";
static const char * const rf24_datarate_e_str_P[] PROGMEM = {
  rf24_datarate_e_str_0,
  rf24_datarate_e_str_1,
  rf24_datarate_e_str_2,
};
static const char rf24_model_e_str_0[] PROGMEM = "nRF24L01";
static const char rf24_model_e_str_1[] PROGMEM = "nRF24L01+";
static const char * const rf24_model_e_str_P[] PROGMEM = {
  rf24_model_e_str_0,
  rf24_model_e_str_1,
};
static const char rf24_crclength_e_str_0[] PROGMEM = "Disabled";
static const char rf24_crclength_e_str_1[] PROGMEM = "8 bits";
static const char rf24_crclength_e_str_2[] PROGMEM = "16 bits" ;
static const char * const rf24_crclength_e_str_P[] PROGMEM = {
  rf24_crclength_e_str_0,
  rf24_crclength_e_str_1,
  rf24_crclength_e_str_2,
};
static const char rf24_pa_dbm_e_str_0[] PROGMEM = "PA_MIN";
static const char rf24_pa_dbm_e_str_1[] PROGMEM = "PA_LOW";
static const char rf24_pa_dbm_e_str_2[] PROGMEM = "LA_MED";
static const char rf24_pa_dbm_e_str_3[] PROGMEM = "PA_HIGH";
static const char * const rf24_pa_dbm_e_str_P[] PROGMEM = { 
  rf24_pa_dbm_e_str_0,
  rf24_pa_dbm_e_str_1,
  rf24_pa_dbm_e_str_2,
  rf24_pa_dbm_e_str_3,
};

void rf24_printDetails(void)
{
  rf24_print_status(rf24_get_status());

  rf24_print_address_register(PSTR("RX_ADDR_P0-1"),RX_ADDR_P0,2);
  rf24_print_byte_register(PSTR("RX_ADDR_P2-5"),RX_ADDR_P2,4);
  rf24_print_address_register(PSTR("TX_ADDR"),TX_ADDR);

  rf24_print_byte_register(PSTR("RX_PW_P0-6"),RX_PW_P0,6);
  rf24_print_byte_register(PSTR("EN_AA"),EN_AA);
  rf24_print_byte_register(PSTR("EN_RXADDR"),EN_RXADDR);
  rf24_print_byte_register(PSTR("RF_CH"),RF_CH);
  rf24_print_byte_register(PSTR("RF_SETUP"),RF_SETUP);
  rf24_print_byte_register(PSTR("CONFIG"),CONFIG);
  rf24_print_byte_register(PSTR("DYNPD/FEATURE"),DYNPD,2);

  printf_P(PSTR("Data Rate\t = %S\r\n"),pgm_read_word(&rf24_datarate_e_str_P[rf24_getDataRate()]));
  printf_P(PSTR("Model\t\t = %S\r\n"),pgm_read_word(&rf24_model_e_str_P[rf24_isPVariant()]));
  printf_P(PSTR("CRC Length\t = %S\r\n"),pgm_read_word(&rf24_crclength_e_str_P[rf24_getCRCLength()]));
  printf_P(PSTR("PA Power\t = %S\r\n"),pgm_read_word(&rf24_pa_dbm_e_str_P[rf24_getPALevel()]));
}

/****************************************************************************/

void rf24_begin(void)
{
  // Initialize pins
  pinMode(rf24.ce_pin,OUTPUT);
  pinMode(rf24.csn_pin,OUTPUT);

  // Initialize SPI bus
  SPI.begin();

  rf24_ce(LOW);
  rf24_csn(HIGH);

  // Must allow the radio time to settle else configuration bits will not necessarily stick.
  // This is actually only required following power up but some settling time also appears to
  // be required after resets too. For full coverage, we'll always assume the worst.
  // Enabling 16b CRC is by far the most obvious case if the wrong timing is used - or skipped.
  // Technically we require 4.5ms + 14us as a worst case. We'll just call it 5ms for good measure.
  // WARNING: Delay is based on P-variant whereby non-P *may* require different timing.
  delay( 5 ) ;

  // Set 1500uS (minimum for 32B payload in ESB@250KBPS) timeouts, to make testing a little easier
  // WARNING: If this is ever lowered, either 250KBS mode with AA is broken or maximum packet
  // sizes must never be used. See documentation for a more complete explanation.
  rf24_write_register(SETUP_RETR,(B0100 << ARD) | (B1111 << ARC));

  // Restore our default PA level
  rf24_setPALevel( RF24_PA_MAX ) ;

  // Determine if this is a p or non-p RF24 module and then
  // reset our data rate back to default value. This works
  // because a non-P variant won't allow the data rate to
  // be set to 250Kbps.
  if( rf24_setDataRate( RF24_250KBPS ) )
  {
    rf24.p_variant = true ;
  }

  // Then set the data rate to the slowest (and most reliable) speed supported by all
  // hardware.
  rf24_setDataRate( RF24_1MBPS ) ;

  // Initialize CRC and request 2-byte (16bit) CRC
  rf24_setCRCLength( RF24_CRC_16 ) ;

  // Disable dynamic payloads, to match rf24.dynamic_payloads_enabled setting
  rf24_write_register(DYNPD,0);

  // Reset current status
  // Notice reset and flush is the last thing we do
  rf24_write_register(STATUS,_BV(RX_DR) | _BV(TX_DS) | _BV(MAX_RT) );

  // Set up default configuration.  Callers can always change it later.
  // This channel should be universally safe and not bleed over into adjacent
  // spectrum.
  rf24_setChannel(76);

  // Flush buffers
  rf24_flush_rx();
  rf24_flush_tx();
}

/****************************************************************************/

void rf24_startListening(void)
{
  rf24_write_register(CONFIG, rf24_read_register(CONFIG) | _BV(PWR_UP) | _BV(PRIM_RX));
  rf24_write_register(STATUS, _BV(RX_DR) | _BV(TX_DS) | _BV(MAX_RT) );

  // Restore the pipe0 adddress, if exists
  if (rf24.pipe0_reading_address)
    rf24_write_register(RX_ADDR_P0, reinterpret_cast<const uint8_t*>(&rf24.pipe0_reading_address), 5);

  // Flush buffers
  rf24_flush_rx();
  rf24_flush_tx();

  // Go!
  rf24_ce(HIGH);

  // wait for the radio to come up (130us actually only needed)
  delayMicroseconds(130);
}

/****************************************************************************/

void rf24_stopListening(void)
{
  rf24_ce(LOW);
  rf24_flush_tx();
  rf24_flush_rx();
}

/****************************************************************************/

void rf24_powerDown(void)
{
  rf24_write_register(CONFIG,rf24_read_register(CONFIG) & ~_BV(PWR_UP));
}

/****************************************************************************/

void rf24_powerUp(void)
{
  rf24_write_register(CONFIG,rf24_read_register(CONFIG) | _BV(PWR_UP));
}

/******************************************************************/

bool rf24_write( const void* buf, uint8_t len )
{
  bool result = false;

  // Begin the write
  rf24_startWrite(buf,len);

  // ------------
  // At this point we could return from a non-blocking write, and then call
  // the rest after an interrupt

  // Instead, we are going to block here until we get TX_DS (transmission completed and ack'd)
  // or MAX_RT (maximum retries, transmission failed).  Also, we'll timeout in case the radio
  // is flaky and we get neither.

  // IN the end, the send should be blocking.  It comes back in 60ms worst case, or much faster
  // if I tighted up the retry logic.  (Default settings will be 1500us.
  // Monitor the send
  uint8_t observe_tx;
  uint8_t status;
  uint32_t sent_at = millis();
  const uint32_t timeout = 500; //ms to wait for timeout
  do
  {
    status = rf24_read_register(OBSERVE_TX,&observe_tx,1);
    IF_SERIAL_DEBUG(Serial.print(observe_tx,HEX));
  }
  while( ! ( status & ( _BV(TX_DS) | _BV(MAX_RT) ) ) && ( millis() - sent_at < timeout ) );

  // The part above is what you could recreate with your own interrupt handler,
  // and then call this when you got an interrupt
  // ------------

  // Call this when you get an interrupt
  // The status tells us three things
  // * The send was successful (TX_DS)
  // * The send failed, too many retries (MAX_RT)
  // * There is an ack packet waiting (RX_DR)
  bool tx_ok, tx_fail;
  rf24_whatHappened(tx_ok,tx_fail,rf24.ack_payload_available);

  //printf("%u%u%u\r\n",tx_ok,tx_fail,rf24.ack_payload_available);

  result = tx_ok;
  IF_SERIAL_DEBUG(Serial.print(result?"...OK.":"...Failed"));

  // Handle the ack packet
  if ( rf24.ack_payload_available )
  {
    rf24.ack_payload_length = rf24_getDynamicPayloadSize();
    IF_SERIAL_DEBUG(Serial.print("[AckPacket]/"));
    IF_SERIAL_DEBUG(Serial.println(rf24.ack_payload_length,DEC));
  }

  // Yay, we are done.

  // Power down
  rf24_powerDown();

  // Flush buffers (Is this a relic of past experimentation, and not needed anymore??)
  rf24_flush_tx();

  return result;
}
/****************************************************************************/

void rf24_startWrite( const void* buf, uint8_t len )
{
  // Transmitter power-up
  rf24_write_register(CONFIG, ( rf24_read_register(CONFIG) | _BV(PWR_UP) ) & ~_BV(PRIM_RX) );
  delayMicroseconds(150);

  // Send the payload
  rf24_write_payload( buf, len );

  // Allons!
  rf24_ce(HIGH);
  delayMicroseconds(15);
  rf24_ce(LOW);
}

/****************************************************************************/

uint8_t rf24_getDynamicPayloadSize(void)
{
  uint8_t result = 0;

  rf24_csn(LOW);
  SPI.transfer( R_RX_PL_WID );
  result = SPI.transfer(0xff);
  rf24_csn(HIGH);

  return result;
}

/****************************************************************************/

bool rf24_available(void)
{
  return rf24_available(NULL);
}

/****************************************************************************/

bool rf24_available(uint8_t* pipe_num)
{
  uint8_t pipe = ( rf24_get_status() >> RX_P_NO ) & B111;

  if ( pipe_num )
    *pipe_num = pipe;

  return pipe != 7;
}

/****************************************************************************/

bool rf24_read( void* buf, uint8_t len )
{
  // Fetch the payload
  rf24_read_payload( buf, len );

  // was this the last of the data available?
  return rf24_read_register(FIFO_STATUS) & _BV(RX_EMPTY);
}

/****************************************************************************/

void rf24_whatHappened(bool& tx_ok,bool& tx_fail,bool& rx_ready)
{
  // Read the status & reset the status in one easy call
  // Or is that such a good idea?
  uint8_t status = rf24_write_register(STATUS,_BV(RX_DR) | _BV(TX_DS) | _BV(MAX_RT) );

  // Report to the user what happened
  tx_ok = status & _BV(TX_DS);
  tx_fail = status & _BV(MAX_RT);
  rx_ready = status & _BV(RX_DR);
}

/****************************************************************************/

void rf24_openWritingPipe(uint64_t value)
{
  // Note that AVR 8-bit uC's store this LSB first, and the NRF24L01(+)
  // expects it LSB first too, so we're good.

  rf24_write_register(RX_ADDR_P0, reinterpret_cast<uint8_t*>(&value), 5);
  rf24_write_register(TX_ADDR, reinterpret_cast<uint8_t*>(&value), 5);

  const uint8_t max_payload_size = 32;
  rf24_write_register(RX_PW_P0,min(rf24.payload_size,max_payload_size));
}

/****************************************************************************/

static const uint8_t child_pipe[] PROGMEM =
{
  RX_ADDR_P0, RX_ADDR_P1, RX_ADDR_P2, RX_ADDR_P3, RX_ADDR_P4, RX_ADDR_P5
};
static const uint8_t child_payload_size[] PROGMEM =
{
  RX_PW_P0, RX_PW_P1, RX_PW_P2, RX_PW_P3, RX_PW_P4, RX_PW_P5
};
static const uint8_t child_pipe_enable[] PROGMEM =
{
  ERX_P0, ERX_P1, ERX_P2, ERX_P3, ERX_P4, ERX_P5
};

void rf24_openReadingPipe(uint8_t child, uint64_t address)
{
  // If this is pipe 0, cache the address.  This is needed because
  // rf24_openWritingPipe() will overwrite the pipe 0 address, so
  // rf24_startListening() will have to restore it.
  if (child == 0)
    rf24.pipe0_reading_address = address;

  if (child <= 6)
  {
    // For pipes 2-5, only write the LSB
    if ( child < 2 )
      rf24_write_register(pgm_read_byte(&child_pipe[child]), reinterpret_cast<const uint8_t*>(&address), 5);
    else
      rf24_write_register(pgm_read_byte(&child_pipe[child]), reinterpret_cast<const uint8_t*>(&address), 1);

    rf24_write_register(pgm_read_byte(&child_payload_size[child]),rf24.payload_size);

    // Note it would be more efficient to set all of the bits for all open
    // pipes at once.  However, I thought it would make the calling code
    // more simple to do it this way.
    rf24_write_register(EN_RXADDR,rf24_read_register(EN_RXADDR) | _BV(pgm_read_byte(&child_pipe_enable[child])));
  }
}

/****************************************************************************/

static void toggle_features(void)
{
  rf24_csn(LOW);
  SPI.transfer( ACTIVATE );
  SPI.transfer( 0x73 );
  rf24_csn(HIGH);
}

/****************************************************************************/

void rf24_enableDynamicPayloads(void)
{
  // Enable dynamic payload throughout the system
  rf24_write_register(FEATURE,rf24_read_register(FEATURE) | _BV(EN_DPL) );

  // If it didn't work, the features are not enabled
  if ( ! rf24_read_register(FEATURE) )
  {
    // So enable them and try again
    toggle_features();
    rf24_write_register(FEATURE,rf24_read_register(FEATURE) | _BV(EN_DPL) );
  }

  IF_SERIAL_DEBUG(printf("FEATURE=%i\r\n",rf24_read_register(FEATURE)));

  // Enable dynamic payload on all pipes
  //
  // Not sure the use case of only having dynamic payload on certain
  // pipes, so the library does not support it.
  rf24_write_register(DYNPD,rf24_read_register(DYNPD) | _BV(DPL_P5) | _BV(DPL_P4) | _BV(DPL_P3) | _BV(DPL_P2) | _BV(DPL_P1) | _BV(DPL_P0));

  rf24.dynamic_payloads_enabled = true;
}

/****************************************************************************/

void rf24_enableAckPayload(void)
{
  //
  // enable ack payload and dynamic payload features
  //

  rf24_write_register(FEATURE,rf24_read_register(FEATURE) | _BV(EN_ACK_PAY) | _BV(EN_DPL) );

  // If it didn't work, the features are not enabled
  if ( ! rf24_read_register(FEATURE) )
  {
    // So enable them and try again
    toggle_features();
    rf24_write_register(FEATURE,rf24_read_register(FEATURE) | _BV(EN_ACK_PAY) | _BV(EN_DPL) );
  }

  IF_SERIAL_DEBUG(printf("FEATURE=%i\r\n",rf24_read_register(FEATURE)));

  //
  // Enable dynamic payload on pipes 0 & 1
  //

  rf24_write_register(DYNPD,rf24_read_register(DYNPD) | _BV(DPL_P1) | _BV(DPL_P0));
}

/****************************************************************************/

void rf24_writeAckPayload(uint8_t pipe, const void* buf, uint8_t len)
{
  const uint8_t* current = reinterpret_cast<const uint8_t*>(buf);

  rf24_csn(LOW);
  SPI.transfer( W_ACK_PAYLOAD | ( pipe & B111 ) );
  const uint8_t max_payload_size = 32;
  uint8_t data_len = min(len,max_payload_size);
  while ( data_len-- )
    SPI.transfer(*current++);

  rf24_csn(HIGH);
}

/****************************************************************************/

bool rf24_isAckPayloadAvailable(void)
{
  bool result = rf24.ack_payload_available;
  rf24.ack_payload_available = false;
  return result;
}

/****************************************************************************/

bool rf24_isPVariant(void)
{
  return rf24.p_variant ;
}

/****************************************************************************/

void rf24_setAutoAck(bool enable)
{
  if ( enable )
    rf24_write_register(EN_AA, B111111);
  else
    rf24_write_register(EN_AA, 0);
}

/****************************************************************************/

void rf24_setAutoAck( uint8_t pipe, bool enable )
{
  if ( pipe <= 6 )
  {
    uint8_t en_aa = rf24_read_register( EN_AA ) ;
    if( enable )
    {
      en_aa |= _BV(pipe) ;
    }
    else
    {
      en_aa &= ~_BV(pipe) ;
    }
    rf24_write_register( EN_AA, en_aa ) ;
  }
}

/****************************************************************************/

bool rf24_testCarrier(void)
{
  return ( rf24_read_register(CD) & 1 );
}

/****************************************************************************/

bool rf24_testRPD(void)
{
  return ( rf24_read_register(RPD) & 1 ) ;
}

/****************************************************************************/

void rf24_setPALevel(rf24_pa_dbm_e level)
{
  uint8_t setup = rf24_read_register(RF_SETUP) ;
  setup &= ~(_BV(RF_PWR_LOW) | _BV(RF_PWR_HIGH)) ;

  // switch uses RAM (evil!)
  if ( level == RF24_PA_MAX )
  {
    setup |= (_BV(RF_PWR_LOW) | _BV(RF_PWR_HIGH)) ;
  }
  else if ( level == RF24_PA_HIGH )
  {
    setup |= _BV(RF_PWR_HIGH) ;
  }
  else if ( level == RF24_PA_LOW )
  {
    setup |= _BV(RF_PWR_LOW);
  }
  else if ( level == RF24_PA_MIN )
  {
    // nothing
  }
  else if ( level == RF24_PA_ERROR )
  {
    // On error, go to maximum PA
    setup |= (_BV(RF_PWR_LOW) | _BV(RF_PWR_HIGH)) ;
  }

  rf24_write_register( RF_SETUP, setup ) ;
}

/****************************************************************************/

rf24_pa_dbm_e rf24_getPALevel(void)
{
  rf24_pa_dbm_e result = RF24_PA_ERROR ;
  uint8_t power = rf24_read_register(RF_SETUP) & (_BV(RF_PWR_LOW) | _BV(RF_PWR_HIGH)) ;

  // switch uses RAM (evil!)
  if ( power == (_BV(RF_PWR_LOW) | _BV(RF_PWR_HIGH)) )
  {
    result = RF24_PA_MAX ;
  }
  else if ( power == _BV(RF_PWR_HIGH) )
  {
    result = RF24_PA_HIGH ;
  }
  else if ( power == _BV(RF_PWR_LOW) )
  {
    result = RF24_PA_LOW ;
  }
  else
  {
    result = RF24_PA_MIN ;
  }

  return result ;
}

/****************************************************************************/

bool rf24_setDataRate(rf24_datarate_e speed)
{
  bool result = false;
  uint8_t setup = rf24_read_register(RF_SETUP) ;

  // HIGH and LOW '00' is 1Mbs - our default
  rf24.wide_band = false ;
  setup &= ~(_BV(RF_DR_LOW) | _BV(RF_DR_HIGH)) ;
  if( speed == RF24_250KBPS )
  {
    // Must set the RF_DR_LOW to 1; RF_DR_HIGH (used to be RF_DR) is already 0
    // Making it '10'.
    rf24.wide_band = false ;
    setup |= _BV( RF_DR_LOW ) ;
  }
  else
  {
    // Set 2Mbs, RF_DR (RF_DR_HIGH) is set 1
    // Making it '01'
    if ( speed == RF24_2MBPS )
    {
      rf24.wide_band = true ;
      setup |= _BV(RF_DR_HIGH);
    }
    else
    {
      // 1Mbs
      rf24.wide_band = false ;
    }
  }
  rf24_write_register(RF_SETUP,setup);

  // Verify our result
  if ( rf24_read_register(RF_SETUP) == setup )
  {
    result = true;
  }
  else
  {
    rf24.wide_band = false;
  }

  return result;
}

/****************************************************************************/

rf24_datarate_e rf24_getDataRate( void )
{
  rf24_datarate_e result ;
  uint8_t dr = rf24_read_register(RF_SETUP) & (_BV(RF_DR_LOW) | _BV(RF_DR_HIGH));

  // switch uses RAM (evil!)
  // Order matters in our case below
  if ( dr == _BV(RF_DR_LOW) )
  {
    // '10' = 250KBPS
    result = RF24_250KBPS ;
  }
  else if ( dr == _BV(RF_DR_HIGH) )
  {
    // '01' = 2MBPS
    result = RF24_2MBPS ;
  }
  else
  {
    // '00' = 1MBPS
    result = RF24_1MBPS ;
  }
  return result ;
}

/****************************************************************************/

void rf24_setCRCLength(rf24_crclength_e length)
{
  uint8_t config = rf24_read_register(CONFIG) & ~( _BV(CRCO) | _BV(EN_CRC)) ;

  // switch uses RAM (evil!)
  if ( length == RF24_CRC_DISABLED )
  {
    // Do nothing, we turned it off above. 
  }
  else if ( length == RF24_CRC_8 )
  {
    config |= _BV(EN_CRC);
  }
  else
  {
    config |= _BV(EN_CRC);
    config |= _BV( CRCO );
  }
  rf24_write_register( CONFIG, config ) ;
}

/****************************************************************************/

rf24_crclength_e rf24_getCRCLength(void)
{
  rf24_crclength_e result = RF24_CRC_DISABLED;
  uint8_t config = rf24_read_register(CONFIG) & ( _BV(CRCO) | _BV(EN_CRC)) ;

  if ( config & _BV(EN_CRC ) )
  {
    if ( config & _BV(CRCO) )
      result = RF24_CRC_16;
    else
      result = RF24_CRC_8;
  }

  return result;
}

/****************************************************************************/

void rf24_disableCRC( void )
{
  uint8_t disable = rf24_read_register(CONFIG) & ~_BV(EN_CRC) ;
  rf24_write_register( CONFIG, disable ) ;
}

/****************************************************************************/
void rf24_setRetries(uint8_t delay, uint8_t count)
{
  rf24_write_register(SETUP_RETR,(delay&0xf)<<ARD | (count&0xf)<<ARC);
}

// vim:ai:cin:sts=2 sw=2 ft=cpp

