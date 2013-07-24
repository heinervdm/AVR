/*
 * timebase.c
 *
 *              Author: Peter Dannegger
 *                      danni@specs.de
 *
 */

#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>

#include "timebase.h"
#include "dcf77.h"
#include "dcf77_config.h"

// at 16MHz:
#define T0COUNT     (F_CPU / 1024 / 64)	                         // 244
#define T0SECERR    (F_CPU - 1024L * 64 * T0COUNT)	             // 9216
#define T0COUNTSEC  (T0COUNT + T0SECERR / 1024)	                 // 253
#define T0MINERR    (F_CPU - 1024 * (63 * T0COUNT + T0COUNTSEC)) // 0
#define T0COUNTMIN  (T0COUNTSEC + (T0MINERR * 60 + 512) / 1024)  // 254
// 234 = 12000000 Hz
// 233 = 12000017 Hz = 4s/month


uint8_t timeflags;
uint8_t dcf77_period;
uint8_t dcf77_pulse;
uint8_t ct_64Hz; // 64 Hz counter (4sec)


void
timebase_init(void)
{
#if defined (__AVR_ATmega328P__)
  TCCR0B = (1 << CS02) | (1 << CS00); // prescale = 1024
  TIMSK0 = 1 << TOIE0; // interrupt enable
#else
  TCCR0 = (1 << CS02) | (1 << CS00); // prescale = 1024
  TIMSK = 1 << TOIE0; // interrupt enable
#endif
}

SIGNAL (TIMER0_OVF_vect)
  {
    static uint8_t dcf77_time, old_dcf77;
    // DCF77 receive
    if( dcf77_time != 0xFF ) // stop on 0xFF
    dcf77_time++; // count ticks

#ifdef DCF77_INVERTED
    if( ~(DCF77_PIN ^ old_dcf77) & 1<<DCF77 )
#else
    if( (DCF77_PIN ^ old_dcf77) & 1<<DCF77 )
#endif

      { // pin changed ?
        old_dcf77 ^= 0xFF; // change old flag
        if( old_dcf77 & 1<<DCF77 )
          {
            dcf77_period = dcf77_time; // store ticks of period
            dcf77_time = 0; // count next period
          }
        else
          {
            dcf77_pulse = dcf77_time; // store ticks of pulse
          }
      }

    // time base 1 second
    TCNT0 = (uint16_t)(256 - T0COUNT); // reload per tick: -183
    if( ++ct_64Hz & 0x3F )
      { // 64 ticks = one second
        timeflags = 1<<ONE_TICK; // one tick over
        return;
      }
    TCNT0 = (uint16_t)(256 - T0COUNTSEC); // reload per second: -189
    if( timeflags & (1<< ONE_MINUTE ))
    TCNT0 = (uint16_t)(256 - T0COUNTMIN); // reload per minute: -234
    timeflags = 1<<ONE_SECOND^1<<ONE_TICK; // one tick, one second over
  }

void
sync_sec(void) // synchronize by DCF77
{
  TCNT0 = (uint16_t)(256 - T0COUNTMIN);
  ct_64Hz = 0;
  timeflags = 0;
}
