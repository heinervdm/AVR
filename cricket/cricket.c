#include <stdlib.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

// enum {chirp, wait1, wait2, sleep1, sleep2};

volatile uint8_t clapped = 0;
// volatile uint8_t state = chirp;
volatile uint8_t count1 = 0;
volatile uint16_t count2 = 0, rndsleep = 0;

int main(void) {
	DDRB |= (1<<PB3);  // set PB3 as output

	PORTB |= (1<<PB4); // enable pull up for PB4
	GIMSK |= (1<<INT0) | (1<<PCIE); // enable pin change interrupt
	PCMSK |= (1<<PCINT4); // activate pin change interrupt on PB4

	TCCR1 |= (1<<CTC1) | (1<<PWM1A) | (1<<CS10); // no prescaler | (1<<CS11) | (1<<CS12) | (1<<CS13); // enable CTC, set prescaler = CK/16384
	TIMSK |= (1<<TOIE1); // enable output compare interrupt
	OCR1C = 100; // 10 kHz interrupt

	srand(10);

	while(1) {
		for (uint8_t s=1;s<4;s++) {
			for (uint8_t t=1;t<5;t++) {
				for (uint8_t i=1;i<25;i++) {
					if (clapped == 0) {
						PORTB |= (1<<PB3);
					}
					_delay_us(200);
					if (clapped == 0) {
						PORTB &= ~(1<<PB3);
					}
					_delay_us(200);
				}
				_delay_us(300);
			}
			if (clapped == 1) {
				clapped = 2;
				rndsleep = 10; // rand() & 0xFF;
				if (rndsleep < 10) {
					rndsleep = 10;
				}
				count1 = 0;
				count2 = 0;
			}
			_delay_ms(500);
		}
		_delay_ms(600);

// 		if (state == sleep1) {
// 			rndsleep = 10; // rand() & 0xFF;
// 			if (rndsleep < 10) {
// 				rndsleep = 10;
// 			}
// 			count1 = 0;
// 			count2 = 0;
// 			state = sleep2;
// 		}
	}
}

ISR(PCINT0_vect) { // called after clap
	clapped = 1;
// 	state = sleep1;
}

ISR(TIM1_OVF_vect) { // called every 100us
// 	if (state == chirp) {
// 		count1++;
// 		if (count1 %2) {
// 			PORTB ^= (1<<PB3);
// 		}
// 		if (count1 > 100) {
// 			count1 = 0;
// 			state = wait1;
// 		}
// 	} else if (state == wait1) {
// 		count1++;
// 		if (count1 > 3) {
// 			count1 = 0;
// 			count2++;
// 			state = chirp;
// 		}
// 		if (count2 > 5) {
// 			count2 = 0;
// 			state = wait2;
// 		}
// 	} else if (state == wait2) {
// 		count1++;
// 		if (count1 > 100) {
// 			count1 = 0;
// 			count2++;
// 			if (count2 > 1000) {
// 				count2 = 0;
// 				state = chirp;
// 			}
// 		}
// 	} else if (state == sleep2) {
		count1++;
		if (count1 > 100) {
			count1 = 0;
			count2++;
			if (count2 > 2000) {
				count2 = 0;
				rndsleep--;
				if (rndsleep < 1) {
// 					state = chirp;
					clapped = 0;
				}
			}
		}
// 	}
}
