/************************************************************************/
/*									                                    */
/*			Clock / Calendar				                            */
/*                                                                      */
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

struct time data;
struct time time;

uint8_t PROGMEM MDAYS[] =
  { 29, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

void
clock(void)
{
  uint8_t i;

  time.second++;
  if (time.second == 60)
    {
      time.second = 0;
      timeflags = 1 << ONE_MINUTE; // to correct deviation per minute
      time.minute++;
      if (time.minute == 60)
        {
          time.minute = 0;
          time.hour++;
          switch (time.hour)
            {
          case 24:
            time.hour = 0;
            time.day++;
            time.wday++;
            if (time.wday == 8)
              time.wday = 1;
            i = time.month;
            if (i == 2 && (time.year & 3) == 0) // leap year
              i = 0;
            if ( __LPM_classic__(MDAYS + i) == time.day)
              {
                time.day = 1;
                time.month++;
                if (time.month == 13)
                  {
                    time.month = 1;
                    time.year++;
                    if (time.year == 100)
                      time.year = 0; // next century
                  }
              }
            break;
            //	case 2:
            //	case 3: summertime(); break;
            }
        }
    }
}

