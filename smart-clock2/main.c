#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/io.h>
#include <avr/sleep.h>
#include "china_lcd.h"
#include "gfx.h"
#include "dcf77.h"
#include "ds18x20.h"
#include "ds1307.h"
#include "i2cmaster.h"

char timestr[] = "  :  ";
char datestr[] = "  .  .20  ";
char tempstr[] = "    C";

uint8_t lastmin = 255, lastday = 255, lastdcfday = 255;
int8_t lasttemp = 127;

void get_temp_str(int16_t curtemp) {
	if (curtemp < 0) tempstr[0] = '-';
	else tempstr[0] = ' ';
	tempstr[1] = (curtemp % 100 / 10) + '0';
	tempstr[2] = (curtemp % 10) + '0';
}

void get_time_str(struct time rtctime) {
	timestr[0] = (rtctime.hour / 10) + '0';
	timestr[1] = (rtctime.hour % 10) + '0';
	timestr[3] = (rtctime.minute / 10) + '0';
	timestr[4] = (rtctime.minute % 10) + '0';
}

void get_date_str(struct time rtctime) {
	datestr[0] = (rtctime.day / 10) + '0';
	datestr[1] = (rtctime.day % 10) + '0';
	datestr[3] = (rtctime.month / 10) + '0';
	datestr[4] = (rtctime.month % 10) + '0';
	datestr[8] = (rtctime.year % 100 / 10) + '0';
	datestr[9] = (rtctime.year % 10) + '0';
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

void ds18x20_get_temperature(int16_t *curtemp) {
	DS18X20_start_meas(DS18X20_POWER_EXTERN, NULL);
	while(DS18X20_conversion_in_progress());
	DS18X20_read_decicelsius_single(DS18B20_FAMILY_CODE, curtemp);
}

int main(void){
	struct time rtctime;
	int16_t curtemp = 0;
	DDRD |= (1<<PD4); // my test-pins
	DDRB |= (1<<PB1);
	PORTB &= ~(1 << PB1);

	// enable INT0
	EICRA |= (1<<ISC01) ^ (1<<ISC00); // interrupt on rising edge of INT0

	timebase_init();

	// init the 1.8 lcd display
	init();
	constructor(_width,_height);

	ds1307_init();

	sei();

	tempstr[3] = 246; // ° is not at right position in glcdfont.c
	ds18x20_get_temperature(&curtemp);
	ds1307_sqw(DS1307_SQW_1HZ);

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
			ds18x20_get_temperature(&curtemp);
		}

		if (lastmin != rtctime.minute) {
			fillScreen(ST7735_BLACK);
			setTextSize(5);
			setCursor(5,30);
			setRotation(1);
			get_time_str(rtctime);
			print(timestr);
			lastmin = rtctime.minute;

			setTextSize(1);
			setCursor(10,110);
			setRotation(1);
			get_date_str(rtctime);
			print(datestr);

			setTextSize(1);
			setCursor(110,110);
			setRotation(1);
			get_temp_str(curtemp);
			print(tempstr);
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
