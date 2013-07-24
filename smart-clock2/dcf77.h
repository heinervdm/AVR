/*
 * dcf77.h
 *
 *              Author: Peter Dannegger
 *                      danni@specs.de
 *
 */


#ifndef DCF77_H_
#define DCF77_H_

#include "clock.h"
#include "timebase.h"
#include "dcf77_config.h"

extern uint8_t dcf77error;
extern uint8_t synchronize;

struct time newtime;

void scan_dcf77( void );

#endif /* DCF77_H_ */
