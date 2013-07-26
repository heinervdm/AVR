#include "ds1307.h"
#include "i2cmaster.h"

void ds1307_write(uint8_t adr,uint8_t data) {
	i2c_start(DS1307ADR+I2C_WRITE);
	i2c_write(adr);
	i2c_write(data);
	i2c_stop();
}

uint8_t ds1307_read(uint8_t adr) {
	uint8_t data;

	i2c_start(DS1307ADR+I2C_WRITE);
	i2c_write(adr);
	i2c_rep_start(DS1307ADR+I2C_READ);
	data=i2c_readNak();
	i2c_stop();
	return data;
}

struct time ds1307_gettime(void) {
	struct time ds1307_time;
	uint8_t tmp = ds1307_read(0);
	ds1307_time.second = (tmp & 0x0F) + ((tmp & 0b01110000) >> 4) * 10;
	tmp = ds1307_read(1);
	ds1307_time.minute = (tmp & 0x0F) + ((tmp & 0b01110000) >> 4) * 10;
	tmp = ds1307_read(2);
	ds1307_time.hour = (tmp & 0x0F) + ((tmp & 0b00110000) >> 4) * 10;
	ds1307_time.wday = ds1307_read(3);
	tmp = ds1307_read(4);
	ds1307_time.day = (tmp & 0x0F) + ((tmp & 0xF0) >> 4) * 10;
	tmp = ds1307_read(5);
	ds1307_time.month = (tmp & 0x0F) + ((tmp & 0xF0) >> 4) * 10;
	tmp = ds1307_read(6);
	ds1307_time.year = (tmp & 0x0F) + ((tmp & 0xF0) >> 4) * 10;

	ds1307_time.second &= ~128;
	return ds1307_time;
}

void ds1307_settime(struct time t) {
	ds1307_write(0,t.second);
	ds1307_write(1,t.minute);
	ds1307_write(2,t.hour);
	ds1307_write(3,t.wday);
	ds1307_write(4,t.day);
	ds1307_write(5,t.month);
	ds1307_write(6,t.year);
}
