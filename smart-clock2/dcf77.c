/************************************************************************/
/*									                                    */
/*			Decode DCF77 Time Information			                    */
/*									                                    */
/*              Author: Peter Dannegger                                 */
/*                      danni@specs.de                                  */
/*                                                                      */
/************************************************************************/

#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>


#include "clock.h"
#include "timebase.h"
#include "dcf77.h"


  uint8_t PROGMEM BITNO[] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06,		// minute
    0xFF,						// parity
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15,			// hour
    0xFF,						// parity
    0x20, 0x21, 0x22, 0x23, 0x24, 0x25,			// day
    0x30, 0x31, 0x32,					// weekday
    0x40, 0x41, 0x42, 0x43, 0x44,			// month
    0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57,	// year
    0xFF };						// parity
  uint8_t PROGMEM BMASK[] = { 1, 2, 4, 8, 10, 20, 40, 80 };


struct time newtime;

uint8_t dcf77error = 0;
uint8_t synchronize = 0;				// successful recieved


void decode_dcf77( uint8_t pulse )
{
  static uint8_t parity = 0;
  uint8_t i;
  uint8_t *d;

  i = newtime.second - 21;
  if( i >= sizeof( BITNO ))			// only bit 21 ... 58
    return;
  parity ^= pulse;				// calculate parity
  i = __LPM_classic__(&BITNO[i]);
  if( i == 0xFF ){				// test parity
    if( parity )
      dcf77error = 1;
    parity = 0;
    return;
  }
  d = (uint8_t *)&newtime.minute + (i >> 4);		// byte address
  i &= 0x0F;					// bit number
  if( i == 0 )
    *d = 0;					// clear all, if lsb
  if( pulse )
    *d += __LPM_classic__(&BMASK[i]);			// set bit
}


void scan_dcf77( void )
{
  if( dcf77_pulse ){
    if( dcf77_pulse > 3 && dcf77_pulse < 8 ){
      decode_dcf77( 0 );
    }else{
      if( dcf77_pulse > 9 && dcf77_pulse < 15 ){
	decode_dcf77( 1 );
      }else{
	dcf77error = 1;
      }
    }
    dcf77_pulse = 0;
  }

  if( dcf77_period ){
    if( newtime.second < 60 )
      newtime.second++;
    if( dcf77_period > 120 && dcf77_period < 140 ){
      if( dcf77error == 0 && newtime.second == 59 ){
        synchronize = 0xFF;
	sync_sec();
	time.second = 0;
	time.minute = newtime.minute;
	time.hour = newtime.hour;
	time.wday = newtime.wday;
	time.day = newtime.day;
	time.month = newtime.month;
	time.year = newtime.year;
      }
      newtime.second = 0;
      dcf77error = 0;
    }else{
      if( dcf77_period < 60 || dcf77_period > 70 )
        dcf77error = 1;
    }
    dcf77_period = 0;
  }
}
