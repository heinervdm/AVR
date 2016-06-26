#include <avr/io.h>
#include <util/delay.h>

int main(void) {
	DDRB=0xFF;// set port B for output

	while(1) {
		for (uint8_t s=1;s<4;s=s+1) {
			for (uint8_t t=1;t<5;t=t+1) {
				for (uint8_t i=1;i<25;i=i+1) {
					PORTB |= (1<<PB3);
					_delay_us(200);
					PORTB &= ~(1<<PB3);
					_delay_us(200);
				}
				_delay_us(300);
			}
			_delay_ms(500);
		}
		_delay_ms(600);
	}
	return 1;
}
