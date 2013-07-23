/*
 * clock.h
 *
 *              Author: Peter Dannegger
 *                      danni@specs.de
 *
 */


#ifndef CLOCK_H_
#define CLOCK_H_


struct time {
  uint8_t second;
  uint8_t minute;
  uint8_t hour;
  uint8_t day;
  uint8_t wday;
  uint8_t month;
  uint8_t year;
}TIME_T;

extern struct time time;

void clock( void );

#endif /* CLOCK_H_ */
