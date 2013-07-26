/*********************************
 * file name: temp_sensor.c
 *********************************/

#include <avr/io.h>
#include <stdint.h>
#define F_CPU 7372800UL
#include <util/delay.h>
#include "temp_sensor_util.h"
#include "temp_sensor.h"

//Master resets DS18B20
static void master_reset(void)
{
	//Master Tx reset pulse
	//Pull bus low 480us minimum
	MASTER_TX();
	BUS_LOW();
	_delay_us(480);
	
	//Master Rx 480us minimum
	//Waiting for DS18B20 presence pulse
	MASTER_RX();
	_delay_us(480);
}

//Master write one bit to DS18B20
static void master_write_bit(uint8_t bit_value)
{
	//Initiate write time slot
	MASTER_TX();
	BUS_LOW();
	_delay_us(1);
	
	//Pull bus high to write "1" to DS18B20
	//or leave bus low to write "0" to DS18B20
	if(bit_value != 0)
	{
		BUS_HIGH();
	}
	
	_delay_us(59); //delay to meet write time slot req.
	
	//1us recovery time
	MASTER_RX();
	_delay_us(1);
}

//Master reads one bit from DS18B20
static uint8_t master_read_bit(void)
{
	uint8_t bit_value;
	
	//Initiate write time slot
	MASTER_TX();
	BUS_LOW();
	_delay_us(1);
	
	//Master releasing bus
	MASTER_RX();
	_delay_us(10); //wait for DS18B20 output to stablize
	
	//Read in bit value
	if(BUS_LINE == 0)
	{
		bit_value = 0;
	}
	else
	{
		bit_value = 1;
	}
	
	//wait to meet minimum read time slot of 60us plus 1us recovery time
	_delay_us(50);
	
	return bit_value;
}

//Master sends a command to DS18B20
static void master_send_command(uint8_t command, commandType cmd_type)
{
	uint8_t bit_pos;
	
	//Need to reset for new round of access of DS18B20
	if(cmd_type == ROM_COMMAND_TYPE)
	{
		master_reset();
	}
	
	//Write each bit to DS18B20, LSB first
	for(bit_pos = 0; bit_pos != 8; ++bit_pos)
	{
		master_write_bit(command & (1<<bit_pos));
	}
}

//Master reads temperature word from DS18B20 scratchpad
static int16_t master_read_temp(void)
{
	int16_t temp_word = 0; //holds 16 bit signed temperature value
	uint16_t bit_value;
	uint8_t bit_pos;
	
	//initialize, send rom command "skip rom" and function command "read scratchpad"
	master_send_command(ROM_COMMAND_SKIP_ROM, ROM_COMMAND_TYPE);
	master_send_command(FUNCTION_COMMAND_READ_SCRATCHPAD, FUNC_COMMAND_TYPE);
	
	//read only the two temperature bytes
	for(bit_pos = 0; bit_pos != 16; ++bit_pos)
	{
		bit_value = (uint16_t)master_read_bit();
		temp_word |= bit_value<<bit_pos;
	}
	
	//stop reading the remaining scratchpad values
	master_reset();
	
	return temp_word;
}

//interface function to return temperature value in deg.C
double get_temp(void)
{
	int16_t temp_word;
	
	temp_word = master_read_temp();
	return temp_word * 0.0625;
}

//interface function to start temperature conversion
void start_conversion(void)
{
	//initialize, send rom command "skip rom" and function command "convert t"
	master_send_command(ROM_COMMAND_SKIP_ROM, ROM_COMMAND_TYPE);
	master_send_command(FUNCTION_COMMAND_CONVERT_T, FUNC_COMMAND_TYPE);
}

//interface function to check temperature conversion complete
//Boolean conversion_complete(void);
uint8_t conversion_complete(void)
{
	if(master_read_bit() == 0)
	{
		//return FALSE;
		return 0;
	}
	else
	{
		//return TRUE;
		return 1;
	}
} 
