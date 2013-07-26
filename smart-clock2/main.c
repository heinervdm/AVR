#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/io.h>
#include <avr/sleep.h>
#include "china_lcd.h"
#include "gfx.h"
#include "dcf77.h"
#include "ds18x20.h"
#include "ds1307.h"

struct time rtctime;

char timestr[] = "  :  ";
char datestr[] = "  .  .20  ";
char tempstr[] = "   °C";

uint8_t lastmin = 255, lastday = 255, lastdcfday = 255;
int8_t lasttemp = 127, curtemp = 0;

char* get_temp_str(char* str) {
	if (curtemp < 0) str[0] = '-';
	else str[0] = ' ';
	str[1] = (curtemp % 100 / 10) + '0';
	str[2] = (curtemp % 10) + '0';
	return str;
}

char* get_time_str(char* str) {
	str[0] = (rtctime.hour / 10) + '0';
	str[1] = (rtctime.hour % 10) + '0';
	str[3] = (rtctime.minute / 10) + '0';
	str[4] = (rtctime.minute % 10) + '0';
	return str;
}

char* get_date_str(char* str) {
	str[0] = (rtctime.day / 10) + '0';
	str[1] = (rtctime.day % 10) + '0';
	str[3] = (rtctime.month / 10) + '0';
	str[4] = (rtctime.month % 10) + '0';
	str[8] = (rtctime.year % 100 / 10) + '0';
	str[9] = (rtctime.year % 10) + '0';
	return str;
}

void dcf77_sync(void) {
	scan_dcf77(); // has to be called at least once every 100ms
	#if 1
	if (DCF77_PIN & 1 << DCF77)
		PORTD &= ~(1 << PD4); // PD4 shows dcf77-pulse
	else
		PORTD |= 1 << PD4;
	#endif
	if (timeflags & 1 << ONE_SECOND) {
		timeflags = 0;
		clock();
	}
}

int main(void){
	DDRD |= (1<<PD4); // my test-pins
	DDRB |= (1<<PB1);
	PORTB &= ~(1 << PB1);

	timebase_init();

	// init the 1.8 lcd display
	init();
	constructor(_width,_height);

	i2c_init();

	sei();

	uint8_t tmp = ds1307_read(2);
	if (tmp & (1 << 6)) {
		ds1307_write(2, tmp & ~(1 << 6));
	}
	ds1307_write(7,(1 << 4)); // enable 1Hz of DS1307

	while (1) {
		rtctime = ds1307_gettime();

		if (lastdcfday != rtctime.day) {
			dcf77_sync();
			if (synchronize) {
				lastdcfday = rtctime.day;
				ds1307_settime(newtime);
			}
		}

		if (lastmin != rtctime.minute && synchronize) {
			// check temperature only once per minute and only if dcf77 is synchronized
			DS18X20_start_meas(DS18X20_POWER_EXTERN, NULL);
			while(DS18X20_conversion_in_progress());
			DS18X20_read_decicelsius_single(DS18B20_FAMILY_CODE,&curtemp);
		}

		if (lastmin != rtctime.minute) {
			setTextSize(5);
			setCursor(5,30);
			fillScreen(ST7735_BLACK);
			setRotation(1);
			print(get_time_str((char *) timestr));
			lastmin = rtctime.minute;

			setTextSize(1);
			setCursor(10,110);
			setRotation(1);
			print(get_date_str((char *) datestr));

			setTextSize(1);
			setCursor(110,110);
			setRotation(1);
			print(get_temp_str((char *) tempstr));
		}

		if (synchronize) {
			set_sleep_mode(SLEEP_MODE_PWR_DOWN);
			sleep_mode();
		}
	}

	return 0;
}

ISR(INT0_vect) {
	/* Interrupt Code */
}
