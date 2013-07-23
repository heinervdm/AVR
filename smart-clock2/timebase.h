/*
 * timebase.h
 *
 *              Author: Peter Dannegger
 *                      danni@specs.de
 */

#ifndef TIMEBASE_H_
#define TIMEBASE_H_

#define ONE_TICK    0
#define ONE_SECOND  1
#define ONE_MINUTE  2

extern uint8_t timeflags;
extern uint8_t dcf77_period;
extern uint8_t dcf77_pulse;
extern uint8_t ct_64Hz;


void timebase_init( void );
void sync_sec( void );

#endif /* TIMEBASE_H_ */
