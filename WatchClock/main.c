#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/power.h>

#include "uart.h"

#ifndef F_CPU
	#define F_CPU 3686400
#endif

#define UART_BAUD_RATE 9600L

/*
 * Clock mapping
 * -----
 * |0 1|
 * |2 3|
 * |4 5|
 * -----
 */

/*
 * Pointer mapping:
 * 0: clock 0, sec pointer
 * 1: clock 0, min pointer
 * 2: clock 1, sec pointer
 * 3: clock 1, min pointer
 * ...
 */
#define CLOCK_0_SEC 0
#define CLOCK_0_MIN 1
#define CLOCK_1_SEC 2
#define CLOCK_1_MIN 3
#define CLOCK_2_SEC 4
#define CLOCK_2_MIN 5
#define CLOCK_3_SEC 6
#define CLOCK_3_MIN 7
#define CLOCK_4_SEC 8
#define CLOCK_4_MIN 9
#define CLOCK_5_SEC 10
#define CLOCK_5_MIN 11

#define BACKUPSTEPS 7

/*
 * Index mapping:
 * pointer 0: pin 1 -> 0, pin 2 -> 1
 * pointer 1: pin 1 -> 2, pin 2 -> 3
 * ...
 * pointer i: pin1 -> i*2, pin 2 -> i*2 + 1
 */
// FIXME: extend this to 12
#define N_POINTERS 2
// FIXME: extend these to 24
// first seconds pointer then minutes pointer of one clock
volatile uint8_t *ports[N_POINTERS*2]      = {&PORTD,&PORTD,&PORTD,&PORTD};
volatile uint8_t *ddrs[N_POINTERS*2]       = {&DDRD, &DDRD, &DDRD, &DDRD};
const uint8_t pins[N_POINTERS*2]           = {PD6,   PD7,   PD2,   PD3};
// FIXME: extend this to 6
// 2 pointers share one ADC
const uint8_t adcs[N_POINTERS/2]        = {0};
// FIXME: extend these to 12
const uint8_t stepsPerSec[N_POINTERS]   = { 1, 4};
uint8_t position[N_POINTERS]            = { 0, 0};
const uint8_t zeroSteps[N_POINTERS]     = { 5,14};
const uint8_t stepsAfterZero[N_POINTERS]= { 5, 7};

const uint8_t times[10][6][2] = {
	{{30,15*4},{45,30*4},{ 0,30*4},{ 0,30*4},{ 0,15*5},{ 0,45*4}}, // 0
	{{37,37*4},{30,30*4},{37,37*4},{ 0,30*4},{37,37*4},{ 0, 0*4}}, // 1
	{{15,15*4},{45,30*4},{15,30*4},{45, 0*4},{ 0,15*4},{45,45*4}}, // 2
	{{15,15*4},{45,30*4},{15,15*4},{45,30*4},{15,15*4},{45, 0*4}}, // 3
	{{30,30*4},{30,30*4},{ 0,15*4},{ 0,45*4},{37,37*4},{ 0, 0*4}}, // 4
	{{15,30*4},{45,45*4},{ 0,15*4},{45,30*4},{15,15*4},{45, 0*4}}, // 5
	{{15,30*4},{45,45*4},{30,15*4},{45,30*4},{ 0,15*4},{ 0,45*4}}, // 6
	{{15,15*4},{15,30*4},{37,37*4},{ 0,37*4},{ 7,37*4},{37,37*4}}, // 7
	{{30,15*4},{30,45*4},{30,15*4},{30,45*4},{ 0,15*4},{ 0,45*4}}, // 8
	{{30,15*4},{30,45*4},{ 0,15*4},{ 0,45*4},{15,15*4},{ 0,45*4}}, // 9
};

const uint8_t digit = 0;

volatile uint8_t duration = 0;
uint16_t lastpulse = 0; // bit = 0 -> lastpulse pin 0, bit = 1 -> lastpulse pin 1

uint8_t uart_data;

void pulse(uint16_t pointers) {
	for (uint8_t i = 0; i < N_POINTERS; i++) {
 		if (pointers & (1<<i)) {
			uint8_t index = i*2;
			uint8_t oindex = index;
			if (lastpulse & (1<<i)) {
				index += 1;
			} else {
				oindex += 1;
			}
			*ports[index] |= (1<<pins[index]);
			*ddrs[index]  |= (1<<pins[index]);
			*ddrs[oindex] |= (1<<pins[oindex]);
			lastpulse ^= (1<<i);
			position[i]++;
			if (position[i] >= 60*stepsPerSec[i]) position[i] = 0;
		}
	}
	duration = 1;
	TIMSK |= (1<<OCIE2);       // TC0 compare match A interrupt enable
}

void init(void) {
// 	power_adc_enable();
	PORTB &= ~(1<<PB0);
	DDRB |= (1<<PB0);
	uint16_t p = 0;
	uint8_t zerostepcount[N_POINTERS];
	uint8_t stepsToGo[N_POINTERS];
	uint8_t backupstepsgone[N_POINTERS/2];
	uint16_t zeroFound = 0;
	for (uint8_t i = 0; i < N_POINTERS; i++) {
		zerostepcount[i] = 0;
		stepsToGo[i] = stepsAfterZero[i];
		if (i<N_POINTERS/2) {
			backupstepsgone[i] = 0;
		}
	}
	// home minutes first, because it's more likley to cover the lightbarrier with the minutes pointer
	uint8_t minutesHomed = 1;
	while (1) {
		p = 0;
		for (uint8_t i = minutesHomed; i < N_POINTERS; i+=2) {
			if (minutesHomed == 1 && zerostepcount[i] > 20) {
				// move minutes pointer away
				if (backupstepsgone[i/2] < BACKUPSTEPS) {
					backupstepsgone[i/2]++;
					p |= (1<<(i-1));
				} else {
					backupstepsgone[i/2] = 0;
					zerostepcount[i] = 0;
				}
			}
			if (minutesHomed == 0 || zerostepcount[i] <= 20) {
				ADMUX = (1<<REFS0) | (1<<REFS1) | (adcs[i/2] & 0x0F);
				ADCSRA = (1<<ADEN) | (1<<ADSC);
				while (ADCSRA & (1<<ADSC));
				uint16_t ares = ADCL;
				ares += (ADCH<<8);
				// 0.1V*1024/2.56V = 40
				uart_puts("ADC: ");
				uart_putc(ares/1000+'0');
				uart_putc(ares%1000/100+'0');
				uart_putc(ares%100/10+'0');
				uart_putc(ares%10+'0');
				uart_putc('\n');
				if (ares < 150) {
					// LED off
					zerostepcount[i]++;
					PORTB |= (1<<PB0);
				}
				else {
					// LED on
					PORTB &= ~(1<<PB0);
					if (zerostepcount[i] == zeroSteps[i]) {
						// LED was off for the searched time -> we found the zero position
						zeroFound |= (1<<i);
					}
					zerostepcount[i] = 0;
				}
				if (stepsToGo[i] > 0) {
					p |= (1<<i);
					// zero was found, go the remaining steps to zero position
					if (zeroFound & (1<<i)) stepsToGo[i]--;
				}
				ADCSRA = 0;
			}
		}
		if (p > 0) {
			pulse(p);
			while(duration > 0); // wait for pulse to finish, can go to sleep mode in this time
// 			set_sleep_mode(SLEEP_MODE_IDLE);
// 			sleep_mode();
		} else {
			if (minutesHomed == 0) break;
			else minutesHomed = 0;
		}
	}
// 	power_adc_disable();
}

int main(void) {
	uint8_t timeReached = 1;
// 	power_all_disable();
// 	power_timer2_enable();
// 	power_usart0_enable();
// 	wdt_disable();
	DDRD = 0; // All inputs
	PORTD = 0; // All PullUps off

	// interrupt for motor pulses
	OCR2  = (F_CPU/(1024*20))/4; // number to count up to 50ms/4
	TCCR2 = (1<<WGM21);          // Clear Timer on Compare Match (CTC) mode
	TCCR2 = (1<<CS20)|(1<<CS21)|(1<<CS22); // clock source CLK/1024

	uart_init( UART_BAUD_SELECT(UART_BAUD_RATE,F_CPU) ); 

	sei();
	init();
	for (uint8_t i = 0; i < N_POINTERS; i++) {
		position[i] = 0;
	}
	uart_puts("Start main loop.\n");
	while(1) {
		if (timeReached == 1) {
			set_sleep_mode(SLEEP_MODE_IDLE);
// 			power_timer2_disable();
// 			sleep_enable();
// 			sleep_bod_disable();
// 			sleep_cpu();
// 			sleep_disable();
			_delay_ms(100);
// 			power_timer2_enable();
		} else {
			uint16_t p = 0;
			uart_puts("Current Position: ");
			for (uint8_t i = 0; i < N_POINTERS; i++) {
				if (i > 0) uart_putc(',');
				uart_putc(i+'0');
				uart_puts(": ");
				uart_putc(position[i]/100+'0');
				uart_putc(position[i]%100/10+'0');
				uart_putc(position[i]%10+'0');
				uart_puts(" (");
				uart_putc(times[uart_data-'0'][i/2][i%2]/100+'0');
				uart_putc(times[uart_data-'0'][i/2][i%2]%100/10+'0');
				uart_putc(times[uart_data-'0'][i/2][i%2]%10+'0');
				uart_puts(")\n");
				if (position[i] != times[uart_data-'0'][i/2][i%2]) {
					p |= (1<<i);
				}
			}
			if (p > 0) {
				uart_puts("pulse\n");
				pulse(p);
				while(duration > 0); // wait for pulse to finish, can go to sleep mode in this time
			} else {
				uart_puts("time reached\n");
				timeReached = 1;
			}
		}
		_delay_ms(100);

		uint16_t c = uart_getc();
		if (! (c & UART_NO_DATA) ) {
			if (c >= '0' && c <= '9') {
				uart_data = c & 0xFF;
				timeReached = 0;
				uart_puts("New position: ");
				uart_putc(uart_data);
				uart_putc('\n');
			}
		}
	}
}

ISR(TIMER2_COMP_vect ) {
	cli();
	++duration;
	if (duration > 4) {
		// all pulses stop
		DDRD = 0;
		PORTD = 0;
		duration = 0;
		TIMSK &= ~(1<<OCIE2);
	}
	sei();
}
