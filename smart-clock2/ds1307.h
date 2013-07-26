#ifndef DS1307_H_
#define DS1307_H_

#include <stdint.h>
#include "clock.h"


#define DS1307ADR 0b11010000

void ds1307_write(uint8_t adr, uint8_t data);
uint8_t ds1307_read(uint8_t adr);
struct time ds1307_gettime(void);
void ds1307_settime(struct time t);

#endif
