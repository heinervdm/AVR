// #include <stdlib.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include "ds18b20.h"

int main(void) {
	double d = 0;

	DDRB |= (1<<PB1); // Port OC0A mit angeschlossener LED als Ausgang
	TCCR0A = (1<<WGM00) | (1<<WGM01) | (1<<COM0B1); // Fast PWM, toggle OC0A.
	TCCR0B = (1<<CS00); // Prescaler 64 = Enable counter
	OCR0B = 128; // Duty cycle 50% (Anm. ob 128 oder 127 bitte prüfen)

	//init interrupt
	sei();

	for (;;) {
		d = ds18b20_gettemp();

		if (d < 25) OCR0B = 0;
		else if (d > 50) OCR0B = 0xFF;
		else OCR0B = (d - 25) * 4;

		_delay_ms(500);
	}
	
	return 0;
}
