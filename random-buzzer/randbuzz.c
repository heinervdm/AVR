#include <stdlib.h>
#include <stdint.h>
#include <avr/io.h>
#include <avr/interrupt.h>

#ifndef F_CPU
#define F_CPU           1000000                   // processor clock frequency
#warning kein F_CPU definiert
#endif

/************************************************************************/
/*                                                                      */
/*                      Debouncing 8 Keys                               */
/*                      Sampling 4 Times                                */
/*                      With Repeat Function                            */
/*                                                                      */
/*              Author: Peter Dannegger                                 */
/*                      danni@specs.de                                  */
/*                                                                      */
/************************************************************************/

#define KEY_DDR         DDRB
#define KEY_PORT        PORTB
#define KEY_PIN         PINB
#define KEY0            PB0
#define KEY1            PB1
#define KEY2            PB2
#define ALL_KEYS        (1<<KEY0 | 1<<KEY1 | 1<<KEY2)

#define REPEAT_MASK     (1<<KEY1 | 1<<KEY2)       // repeat: key1, key2
#define REPEAT_START    50                        // after 500ms
#define REPEAT_NEXT     20                        // every 200ms

volatile uint8_t key_state;                                // debounced and inverted key state:
// bit = 1: key pressed
volatile uint8_t key_press;                                // key press detect

volatile uint8_t key_rpt;                                  // key long press and repeat

///////////////////////////////////////////////////////////////////
//
// check if a key has been pressed. Each pressed key is reported
// only once
//
uint8_t get_key_press( uint8_t key_mask )
{
	cli();                                          // read and clear atomic !
	key_mask &= key_press;                          // read key(s)
	key_press ^= key_mask;                          // clear key(s)
	sei();
	return key_mask;
}

///////////////////////////////////////////////////////////////////
//
// check if a key has been pressed long enough such that the
// key repeat functionality kicks in. After a small setup delay
// the key is reported being pressed in subsequent calls
// to this function. This simulates the user repeatedly
// pressing and releasing the key.
//
uint8_t get_key_rpt( uint8_t key_mask )
{
	cli();                                          // read and clear atomic !
	key_mask &= key_rpt;                            // read key(s)
	key_rpt ^= key_mask;                            // clear key(s)
	sei();
	return key_mask;
}

///////////////////////////////////////////////////////////////////
//
// check if a key is pressed right now
//
uint8_t get_key_state( uint8_t key_mask )

{
	key_mask &= key_state;
	return key_mask;
}

///////////////////////////////////////////////////////////////////
//
uint8_t get_key_short( uint8_t key_mask )
{
	cli();                                          // read key state and key press atomic !
	return get_key_press( ~key_state & key_mask );
}

///////////////////////////////////////////////////////////////////
//
uint8_t get_key_long( uint8_t key_mask )
{
	return get_key_press( get_key_rpt( key_mask ));
}

///////////////////////////////////////////////////////////////////
// End debouncing functions
///////////////////////////////////////////////////////////////////

#define SOUND_DDR  DDRD
#define SOUND_PORT PORTD
#define SOUND_PIN  PIND
#define SOUND_OUT  PD5

#define STATUS_DDR  DDRD
#define STATUS_PORT PORTD
#define STATUS_PIN  PIND
#define STATUS_OUT  PD6

volatile uint32_t time = 0;
volatile uint16_t reactiontime = 0;
volatile uint8_t state = 0;
uint8_t debounecounter = 0;

uint8_t getRandom(uint8_t max) {
// 	return rand() / (RAND_MAX / max + 1);

	return 5;
}

void startSound(void) {
	SOUND_PORT |= (1<<SOUND_OUT);
	
	STATUS_PORT |= 1<<STATUS_OUT;
}

void stopSound(void) {
	SOUND_PORT &= ~(1<<SOUND_OUT);
	STATUS_PORT &= ~(1<<STATUS_OUT);
}

int main(void) {
	// Configure debouncing routines
	KEY_DDR &= ~ALL_KEYS;                // configure key port for input
	KEY_PORT |= ALL_KEYS;                // and turn on pull up resistors

	SOUND_DDR |= (1<<SOUND_OUT);
	STATUS_DDR |= (1<<STATUS_OUT);

	TCCR0A = (1<<WGM01); // CTC Modus
	TCCR0B |= (1<<CS00) | (1<<CS02); // Prescaler 1204
	OCR0A = F_CPU/1024/1000 + 1;
	TIMSK |= (1<<OCIE0A);
	sei();
	
	int8_t r = getRandom(250);
	while(1) {
		if (time/1000 > r) {
			startSound();
		}
		if( get_key_press( 1<<KEY2 ) || get_key_rpt( 1<<KEY2 )){
			stopSound();
			r = getRandom(250);
			time = 0;
		}
	}
}

ISR (TIMER0_COMPA_vect) {
	++time;
	++debounecounter;

	if (debounecounter >= 10) {
		debounecounter = 0;
		static uint8_t ct0, ct1, rpt;
		uint8_t i;

		i = key_state ^ ~KEY_PIN;                       // key changed ?
		ct0 = ~( ct0 & i );                             // reset or count ct0
		ct1 = ct0 ^ (ct1 & i);                          // reset or count ct1
		i &= ct0 & ct1;                                 // count until roll over ?
		key_state ^= i;                                 // then toggle debounced state
		key_press |= key_state & i;                     // 0->1: key press detect
		
		if( (key_state & REPEAT_MASK) == 0 )            // check repeat function
			rpt = REPEAT_START;                         // start delay
		if( --rpt == 0 ){
			rpt = REPEAT_NEXT;                          // repeat delay
			key_rpt |= key_state & REPEAT_MASK;
		}
	}
}



// 	while(1){
// 		if( get_key_short( 1<<KEY1 ))
// 			LED_PORT ^= 1<<LED1;
// 		
// 		if( get_key_long( 1<<KEY1 ))
// 			LED_PORT ^= 1<<LED2;
// 		
// 		// single press and repeat
// 		
// 		if( get_key_press( 1<<KEY2 ) || get_key_rpt( 1<<KEY2 )){
// 			uint8_t i = LED_PORT;
// 			
// 			i = (i & 0x07) | ((i << 1) & 0xF0);
// 			if( i < 0xF0 )
// 				i |= 0x08;
// 			LED_PORT = i;      
// 		}
// 	}
