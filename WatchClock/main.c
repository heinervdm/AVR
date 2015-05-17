#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>

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

#define BACKUPSTEPS 15

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
const uint8_t pins[N_POINTERS*2]        = {PD0,   PD1,   PD2,   PD3};
// FIXME: extend this to 6
// 2 pointers share one ADC
const uint8_t adcs[N_POINTERS/2]        = {0};
// FIXME: extend these to 12
const uint8_t stepsPerSec[N_POINTERS]   = { 1, 4};
uint8_t position[N_POINTERS]      = { 0, 0};
const uint8_t zeroSteps[N_POINTERS]     = {12,12};
const uint8_t stepsAfterZero[N_POINTERS]= { 5, 5};

volatile uint8_t duration = 0;
uint16_t lastpulse = 0; // bit = 0 -> lastpulse pin 0, bit = 1 -> lastpulse pin 1

uint8_t seconds = 0;
uint8_t minutes = 0;
uint8_t hours = 0;
uint8_t newtime = 0;
uint8_t running = 0;

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
			if (position[i] >= 60) position[i] = 0;
		}
	}
	duration = 1;
	TIMSK |= (1<<OCIE2);       // TC0 compare match A interrupt enable
}

void init(void) {
	uint16_t p = 0;
	uint8_t zerostepcount[N_POINTERS];
	uint8_t stepsToGo[N_POINTERS];
	uint8_t stepcount[N_POINTERS];
	uint8_t backupstepsgone[N_POINTERS/2];
	uint16_t zeroFound = 0;
	for (uint8_t i = 0; i < N_POINTERS; i++) {
		zerostepcount[i] = 0;
		stepsToGo[i] = stepsAfterZero[i];
		if (i<N_POINTERS/2) {
			backupstepsgone[i] = 0;
		}
	}
	uint8_t secondsHomed = 0;
	while (1) {
		p = 0;
		for (uint8_t i = secondsHomed; i < N_POINTERS; i+=2) {
			if (secondsHomed == 0 && stepcount[i] > 60) {
				// move minutes pointer away
				if (backupstepsgone[i/2] < BACKUPSTEPS) {
					backupstepsgone[i/2]++;
					p |= (1<<(i+1));
				} else {
					stepcount[i] = 0;
				}
			}
			if (secondsHomed == 1 || stepcount[i] <= 60) {
				ADMUX = (1<<REFS0) | (1<<REFS1) | (adcs[i/2] & 0x0F);
				ADCSRA = (1<<ADEN) | (1<<ADSC);
				while (ADCSRA & (1<<ADSC));
				// 0.1V*1024/2.56V = 40
				if (ADC < 40) zerostepcount[i]++;
				else zerostepcount[i] = 0;
				// check for 0 position
				if (zerostepcount[i] >= zeroSteps[i/2]) {
					zeroFound |= (1<<i);
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
// 			while(duration > 0); // wait for pulse to finish, can go to sleep mode in this time
			ACSR |= (1<<ACD);
			MCUCR = (1<<SE);
		} else {
			if (secondsHomed) break;
			else secondsHomed = 1;
		}
	}
}

int main(void) {
	DDRB = 0; // All inputs
	PORTB = 0; // All PullUps off

	// interrupt for motor pulses
	OCR2  = (F_CPU/(1024*20))/4; // number to count up to 50ms/4
	TCCR2 = (1<<WGM21);          // Clear Timer on Compare Match (CTC) mode
	TCCR2 = (1<<CS20)|(1<<CS21)|(1<<CS22); // clock source CLK/1024

	// interrupt every second, for clock
	TCCR1B = (1<<WGM12) | (1<<CS12) | (1<<CS10);
	OCR1A   = F_CPU/1024;
	TIMSK |= ~(1<<OCIE1A);

	sei();

	init();

	while(1) {
		if (newtime) {
			running = 1;
			// set position to reach.
		}
		if (running) {
			uint16_t p;
			for (uint8_t i = 0; i < 60; i++) {
				p = (1<<CLOCK_0_SEC);
			}
			pulse(p);
// 			while(duration > 0); // wait for pulse to finish, can go to sleep mode in this time
		}
		ACSR |= (1<<ACD);
		MCUCR = (1<<SE);
// 		_delay_ms(255);
	}
}

ISR(TIMER2_COMP_vect ) {
	cli();
	++duration;
	if (duration > 4) {
		// all pulses stop
		DDRB = 0;
		PORTB = 0;
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
