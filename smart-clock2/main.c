#include <stdlib.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/io.h>
#include "china_lcd.h"
#include "gfx.h"
#include "dcf77.h"
#include "i2cmaster.h"

#define DS1307ADR 0b11010000

struct time oldtime, rtctime;

char newday[] = "  :  \n  .  .20  \n\0";

char* get_newday_time_str(char* timestr) {
	timestr[0] = (rtctime.hour % 10) + '0';
	timestr[1] = (rtctime.hour / 10) + '0';
	timestr[3] = (rtctime.minute % 10) + '0';
	timestr[4] = (rtctime.minute / 10) + '0';

	timestr[6] = (rtctime.day % 10) + '0';
	timestr[7] = (rtctime.day / 10) + '0';
	timestr[9] = (rtctime.month % 10) + '0';
	timestr[10] = (rtctime.month / 10) + '0';
	timestr[14] = (rtctime.year % 10) + '0';
	timestr[15] = (rtctime.year / 10) + '0';
	return timestr;
}

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

void ds1307_gettime(void) {
	rtctime.second = ds1307_read(0);
	rtctime.minute = ds1307_read(1);
	rtctime.hour = ds1307_read(2);
	rtctime.day = ds1307_read(3);
// 	date = ds1307_read(4);
	rtctime.month = ds1307_read(5);
	rtctime.year = ds1307_read(6);

	rtctime.second &= ~128;
}

int main(void){
	uint8_t flipper;
	uint8_t lastwait = 0;
	timebase_init();
	flipper = 0;

	// init the 1.8 lcd display
	init();
	constructor(_width,_height);

	i2c_init();

	sei();

	DDRD |= (1<<PD4); // my test-pins

	fillScreen(ST7735_BLACK);

	while(1){
		scan_dcf77(); // has to be called at least once every 100ms
		setRotation(0);
		setCursor(0,0);
 		setTextWrap(1);
#if 1
		if (DCF77_PIN & 1 << DCF77)
			PORTD &= ~(1 << PD4); // PD0 shows dcf77-pulse
		else
			PORTD |= 1 << PD4;
#endif
		if (timeflags & 1 << ONE_SECOND) {
			timeflags = 0;
			clock();
			if (!synchronize) {
				if (lastwait == 0) fillScreen(ST7735_BLACK);
				switch (flipper++) {
					case 0:
						print("Wait for Sync -\n");
						break;
					case 1:
						print("Wait for Sync \\\n");
						break;
					case 2:
						print("Wait for Sync |\n");
						break;
					case 3:
						print("Wait for Sync /\n");
						break;
				}
				if(flipper == 4) flipper=0;
				lastwait = 1;
			}
			else {
				if (lastwait == 1) fillScreen(ST7735_BLACK);
				get_newday_time_str((char *) newday);
				print(newday);
// 				if (time.day != oldtime.day) {
// 					get_newday_time_str((char *) newday);
// 					uputs((uint8_t *) newday);
// 					oldtime=time;
// 				} else if (time.hour != oldtime.hour) {
// 					get_newhour_time_str((char *) newhour);
// 					uputs((uint8_t *) newhour);
// 					oldtime=time;
// 				} else if (time.minute != oldtime.minute) {
// 					get_newminute_time_str((char *) newminute);
// 					uputs((uint8_t *) newminute);
// 					oldtime=time;
// 				}
				lastwait = 0;
			}
		}
		

// 		// COLORS AND 'T'
// 		fillScreen(Color565(255,0,0));
// 		fillRect(10,10,128-20,10,Color565(0,0,0));
// 		fillRect(64-5,10,10,140, Color565(0,0,0));
// 		myDelay(300);
// 
// 		fillScreen(Color565(0,255,0));
// 		fillRect(10,10,128-20,10,Color565(0,0,0));
// 		fillRect(64-5,10,10,140, Color565(0,0,0));
// 		myDelay(300);
// 
// 		fillScreen(Color565(0,0,255));
// 		fillRect(10,10,128-20,10,Color565(0,0,0));
// 		fillRect(64-5,10,10,140, Color565(0,0,0));
// 		myDelay(300);
// 
// 		// TEXT
// 		fillScreen(ST7735_BLACK);
// 		setCursor(0,0);
// 		setTextWrap(1);
// 		print("Hallo dies ist ein Test von Tobi!");
// 		myDelay(5000);
// 		setTextSize(2);
// 		setRotation(1);
// 		fillScreen(ST7735_BLACK);
// 		setCursor(0,0);
// 		print("Hallo dies ist ein Test von Tobi!");
// 		myDelay(5000);
	}

	return 0;
}
