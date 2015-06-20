#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>

#include "uart.h"

#ifndef F_CPU
	#define F_CPU 3686400
#endif


#define UART_BAUD_RATE 9600L
// #define UBRR_VAL ((F_CPU+UART_BAUD_RATE*8)/(UART_BAUD_RATE*16)-1)

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
const uint8_t pins[N_POINTERS*2]        = {PD6,   PD7,   PD2,   PD3};
// FIXME: extend this to 6
// 2 pointers share one ADC
const uint8_t adcs[N_POINTERS/2]        = {0};
// FIXME: extend these to 12
const uint8_t stepsPerSec[N_POINTERS]   = { 1, 4};
uint8_t position[N_POINTERS]      = { 0, 0};
const uint8_t zeroSteps[N_POINTERS]     = { 5,14};
const uint8_t stepsAfterZero[N_POINTERS]= { 5, 7};

volatile uint8_t duration = 0;
uint16_t lastpulse = 0; // bit = 0 -> lastpulse pin 0, bit = 1 -> lastpulse pin 1

uint8_t seconds = 0;
uint8_t minutes = 0;
uint8_t hours = 0;
uint8_t newtime = 0;
uint8_t running = 0;

volatile uint8_t uart_flag;
volatile uint8_t uart_data;

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
// 			PORTD |= (1<<pins[index]);
// 			DDRD  |= (1<<pins[index]);
// 			DDRD  |= (1<<pins[oindex]);
			lastpulse ^= (1<<i);
			position[i]++;
			if (position[i] >= 60) position[i] = 0;
		}
	}
	duration = 1;
// 	_delay_ms(100);
	TIMSK |= (1<<OCIE2);       // TC0 compare match A interrupt enable
}

void init(void) {
	PORTB &= ~(1<<PB0);
	DDRB |= (1<<PB0);
	uint16_t p = 0;
	uint8_t zerostepcount[N_POINTERS];
	uint8_t stepsToGo[N_POINTERS];
	uint8_t stepcount[N_POINTERS];
	uint8_t backupstepsgone[N_POINTERS/2];
	uint16_t zeroFound = 0;
	for (uint8_t i = 0; i < N_POINTERS; i++) {
		zerostepcount[i] = 0;
		stepsToGo[i] = stepsAfterZero[i];
		stepcount[i]=0;
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
				uart_puts("Doing backup steps.\n");
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
// 				ADCSRA = (1<<ADEN) | (1<<ADSC);
// 				while (ADCSRA & (1<<ADSC));
// 				ares += ADCL;
// 				ares += (ADCH<<8);
// 				ares /= 2;
				// 0.1V*1024/2.56V = 40
				uart_puts("ADC: ");
				uart_putc('0'+ares/100);
				uart_putc('0'+(ares%100)/10);
				uart_putc('0'+ares%10);
				uart_putc('\n');
				if (ares < 150) {
					zerostepcount[i]++;
					PORTB |= (1<<PB0);
					uart_puts("LED off\n");
					uart_puts("zero step count: ");
					uart_putc('0'+zerostepcount[i]/10);
					uart_putc('0'+zerostepcount[i]%10);
					uart_putc('\n');
				}
				else {
					PORTB &= ~(1<<PB0);
					if (zerostepcount[i]>0) {
					uart_puts("LED on\n");
					uart_puts("zero step count: ");
					uart_putc('0'+zerostepcount[i]/10);
					uart_putc('0'+zerostepcount[i]%10);
					uart_putc('\n');
					}
					if (zerostepcount[i] == zeroSteps[i]) {
						zeroFound |= (1<<i);
						uart_puts("Zero found!\n");
					}
					zerostepcount[i] = 0;
					// check for 0 position
				}
				if (stepsToGo[i] > 0) {
					p |= (1<<i);
					if (zeroFound & (1<<i)) stepsToGo[i]--;
					stepcount[i]++;
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
}

void pulse2(void) {
	PORTD |= (1<<PD0);
	DDRD |= (1<<PD0) | (1<<PD1);
	_delay_ms(100);
	PORTD &= ~(1<<PD0);
	PORTD |= (1<<PD1);
	_delay_ms(100);
	PORTD &= ~(1<<PD1);
	DDRD &= ~(1<<PD0);
	DDRD &= ~(1<<PD1);
}

int main(void) {
	DDRD = 0; // All inputs
	PORTD = 0; // All PullUps off

	// interrupt for motor pulses
	OCR2  = (F_CPU/(1024*20))/4; // number to count up to 50ms/4
	TCCR2 = (1<<WGM21);          // Clear Timer on Compare Match (CTC) mode
	TCCR2 = (1<<CS20)|(1<<CS21)|(1<<CS22); // clock source CLK/1024

	uart_init( UART_BAUD_SELECT(UART_BAUD_RATE,F_CPU) );

// 	UBRRH = UBRR_VAL >> 8;
// 	UBRRL = UBRR_VAL & 0xFF;
// 	UCSRB = (1<<RXCIE) | (1<<RXEN);

	// interrupt every second, for clock
// 	TCCR1B = (1<<WGM12) | (1<<CS12) | (1<<CS10);
// 	OCR1A   = F_CPU/1024;
// 	TIMSK |= ~(1<<OCIE1A);

	sei();
	uart_puts("String stored in SRAM\n");
	init();

	while(1) {
// 		if (newtime) {
// 			running = 1;
// 			// set position to reach.
// 		}
// 		if (running) {
// 			uint16_t p;
// 			for (uint8_t i = 0; i < 60; i++) {
// 				p = (1<<CLOCK_0_SEC);
// 			}
// 			pulse(p);
// 			while(duration > 0); // wait for pulse to finish, can go to sleep mode in this time
// 		}
// 		set_sleep_mode(SLEEP_MODE_IDLE);
// 		sleep_mode();
		_delay_ms(100);
// 		DDRD = 0;
// 		PORTD = 0;
// 		if (uart_flag == 1) {
// 			uart_flag = 0;
// 			// new time;
// 		}
		ADMUX = (1<<REFS0) | (1<<REFS1) | (adcs[0] & 0x0F);
		ADCSRA = (1<<ADEN) | (1<<ADSC);
		while (ADCSRA & (1<<ADSC));
		uint16_t ares = ADCL;
		ares += (ADCH<<8);
		// 				ADCSRA = (1<<ADEN) | (1<<ADSC);
		// 				while (ADCSRA & (1<<ADSC));
		// 				ares += ADCL;
		// 				ares += (ADCH<<8);
		// 				ares /= 2;
		// 0.1V*1024/2.56V = 40
		uart_puts("ADC: ");
		uart_putc('0'+ares/100);
		uart_putc('0'+(ares%100)/10);
		uart_putc('0'+ares%10);
		uart_putc('\n');
		if (ares < 100) {
			PORTB |= (1<<PB0);
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

ISR(TIMER1_COMPA_vect ) {
	cli();
	seconds++;
	if (seconds >= 60) {
		seconds = 0;
		minutes++;
		if (minutes >= 60) {
			minutes = 0;
			hours++;
			if (hours >= 24) {
				hours = 0;
			}
		}
		newtime = 1;
	}
	sei();
}

// ISR(USART_RXC_vect) {
// 	uart_flag = 1;
// 	uart_data = UDR;
// }
