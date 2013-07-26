#ifndef DS1307_H_
#define DS1307_H_

#include <stdint.h>
#include "clock.h"


#define DS1307ADR 0b11010000

enum DS1307SQW {
	DS1307_SQW_OFF   = 0b00000000,
	DS1307_SQW_1HZ   = 0b00010000,
	DS1307_SQW_4KHZ  = 0b00010001,
	DS1307_SQW_8KHZ  = 0b00010010,
	DS1307_SQW_32KHZ = 0b00010011,
};

void ds1307_init(void);
void ds1307_sqw(enum DS1307SQW);
void ds1307_write(uint8_t adr, uint8_t data);
uint8_t ds1307_read(uint8_t adr);
struct time ds1307_gettime(void);
void ds1307_settime(struct time t);

#endif
