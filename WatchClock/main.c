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

/*
 * Index mapping:
 * pointer 0: pin 1 -> 0, pin 2 -> 1
 * pointer 1: pin 1 -> 2, pin 2 -> 3
 * ...
 * pointer i: pin1 -> i*2, pin 2 -> i*2 + 1
 */
// FIXME: extend these to 24
volatile uint8_t *ports[12] = {&PORTB,&PORTB,&PORTB,&PORTB,&PORTB,&PORTB,&PORTB,&PORTB,&PORTD,&PORTD,&PORTD,&PORTD};
volatile uint8_t *ddrs[12]  = {&DDRB, &DDRB, &DDRB, &DDRB, &DDRB, &DDRB, &DDRB, &DDRB, &DDRD, &DDRD, &DDRD, &DDRD};
volatile uint8_t pins[12]   = {PB0,   PB1,   PB2,   PB3,   PB4,   PB5,   PB6,   PB7,   PD0,   PD1,   PD2,   PD3};
volatile uint8_t stepsPerSec[12] = {1,4,1,4,1,4,1,4,1,4,1,4};

volatile uint8_t *port = &PORTB;
volatile uint8_t *ddr = &DDRB;

volatile uint8_t duration = 0;
volatile uint16_t lastpulse = 0; // bit = 0 -> lastpulse pin 0, bit = 1 -> lastpulse pin 1
volatile uint16_t topulse = 0;   // bit = 1 -> step this pointer
volatile uint8_t position[12] = {0,0,0,0,0,0,0,0,0,0,0,0};

void pulse(uint16_t pointers) {
	for (uint8_t i = 0; i < 6; i++) { // FIXME: extend to 12
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
	TIMSK |= (1<<OCIE0A);       // TC0 compare match A interrupt enable
}

void init(void) {
	uint16_t p = 0x0FFF;
	while (1) {
		for (uint8_t i = 0; i < 12; i++) {
			// check for 0 position
			// TODO
			p &= ~(1<<i);
		}
		if (p > 0) {
			pulse(p);
		} else {
			break;
		}
	}
}

int main(void) {
	DDRB = 0; // All inputs
	PORTB = 0; // All PullUps off

	OCR0A  = (F_CPU/(1024*20))/4;  // number to count up to 50ms/4
	TCCR0A |= (1<<WGM01);          // Clear Timer on Compare Match (CTC) mode
	TCCR0B |= (1<<CS00)|(1<<CS02); // clock source CLK/1024

	sei();

	init();

	while(1) {
		for (uint8_t i = 0; i < 60; i++) {
			uint16_t p = 1;//(1<<CLOCK_0_SEC);
			pulse(p);
			while(duration > 0); // wait for pulse to finish, can go to sleep mode in this time
		}
		_delay_ms(255);
	}
}

#if defined(__AVR_ATtiny2313__) || defined(ATmega168__) || defined(__AVR_ATmega48__) || defined(__AVR_ATmega88__) || defined(__AVR_ATmega640__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega1281__) || defined(__AVR_ATmega2560__) || defined(__AVR_ATmega2561__) || defined(__AVR_ATmega324P__) || defined(__AVR_ATmega164P__) || defined(__AVR_ATmega644P__) || defined(__AVR_ATmega644__) || defined(__AVR_ATmega16HVA__) || defined(__AVR_ATtiny2313__) || defined(__AVR_ATtiny48__) || defined(__AVR_ATtiny261__) || defined(__AVR_ATtiny461__) || defined(__AVR_ATtiny861__) || defined(__AVR_AT90USB162__) || defined(__AVR_AT90USB82__) || defined(__AVR_AT90USB1287__) || defined(__AVR_AT90USB1286__) || defined(__AVR_AT90USB647__) || defined(__AVR_AT90USB646) 
ISR(TIMER0_COMPA_vect) {
#elif defined(__AVR_ATtiny24__) || defined(__AVR_ATtiny44__) || defined(__AVR_ATtiny84__) || defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny25__) || defined(__AVR_ATtiny85)
ISR(TIM0_COMPA_vect ) {
#else
ISR(TIMER0_COMP_vect ) {
#endif
	cli();
	++duration;
	if (duration > 4) {
		// all pulses stop
		DDRB = 0;
		PORTB = 0;
		duration = 0;
		TIMSK &= ~(1<<OCIE0A);
	}
	sei();
}
