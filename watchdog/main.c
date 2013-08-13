#include <avr/interrupt.h>
#include <avr/io.h>

#define INPIN PINB
#define INPORT PORTB
#define INDDR DDRB
#define PIN PB1

#define OUTPIN PINB
#define OUTPORT PORTB
#define OUTDDR DDRB
#define POUT PB0

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
	TIMSK |= (1<<OCIE0A);

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

#if defined(__AVR_ATtiny2313__) || defined(ATmega168__) || defined(__AVR_ATmega48__) || defined(__AVR_ATmega88__) || defined(__AVR_ATmega640__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega1281__) || defined(__AVR_ATmega2560__) || defined(__AVR_ATmega2561__) || defined(__AVR_ATmega324P__) || defined(__AVR_ATmega164P__) || defined(__AVR_ATmega644P__) || defined(__AVR_ATmega644__) || defined(__AVR_ATmega16HVA__) || defined(__AVR_ATtiny2313__) || defined(__AVR_ATtiny48__) || defined(__AVR_ATtiny261__) || defined(__AVR_ATtiny461__) || defined(__AVR_ATtiny861__) || defined(__AVR_AT90USB162__) || defined(__AVR_AT90USB82__) || defined(__AVR_AT90USB1287__) || defined(__AVR_AT90USB1286__) || defined(__AVR_AT90USB647__) || defined(__AVR_AT90USB646) 
ISR(TIMER0_COMPA_vect) {
#elif defined(__AVR_ATtiny24__) || defined(__AVR_ATtiny44__) || defined(__AVR_ATtiny84__) || defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny25__) || defined(__AVR_ATtiny85)
ISR(TIM0_COMPA_vect ) {
#else
ISR(TIMER0_COMP_vect ) {
#endif
	cli();
	++duration;
	sei();
}
