#include <avr/interrupt.h>
#include <avr/io.h>

#define INPIN PINB
#define INPORT PORTB
#define INDDR DDRB
#define PIN PB1

#define OUTPIN PINB
#define OUTPORT PORTB
#define OUTDDR DDRB
#define POUT PB2

volatile uint8_t firstedge = 0;
volatile uint8_t lastedge = 2;
volatile uint8_t secondlastedge = 2;
volatile uint8_t duration = 0;

void resetPulse(void) {
	firstedge = 0;
}

int main(void){
	OUTDDR |= (1<<POUT);
	OUTPORT &= ~(1 << POUT);
	INDDR &= ~(1<<POUT);

	OCRA0 = 100; 
	TCCR0A |= (1<<WGM01); // CTC Mode
	TCCR0B |= (1<<CS00) | (1<<CS02); // Prescaler = 1024

	sei();

	while (1) {
		uint8_t currentedge = INPIN & (1 << PIN);
		if (currentedge != lastedge && currentedge != secondlastedge) {
			firstedge = 1;
			duration = 0;
		}
		else if (firstedge && duration > 100) {
			resetPulse();
		}
		secondlastedge = lastedge;
		lastedge = currentedge;
	}
	return 0;
}

ISR(TIM0_COMPA) {
	cli();
	++duration;
	sei();
}
