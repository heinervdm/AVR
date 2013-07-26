/*********************************
 * file name: temp_sensor_util.h
 *********************************/

#ifndef TEMP_SENSOR_UTIL_H
#define TEMP_SENSOR_UTIL_H



//Configure bus line to transmit
#define MASTER_TX()                 \
__asm__ __volatile__ (      \
"sbi %0, %1"                \
: /* no output */           \
: "I" (_SFR_IO_ADDR(DDRE)), \
"I" (DDE4)                \
)

//Configure bus line to receive
//internal pull-up disabled
#define MASTER_RX()                 \
__asm__ __volatile__ (      \
"cbi %0, %1" "\n\t"         \
"cbi %2, %3" "\n\t"         \
:                           \
: "I" (_SFR_IO_ADDR(DDRE)), \
"I" (DDE4),               \
"I" (_SFR_IO_ADDR(PORTE)),\
"I" (PE4)                 \
)

//Set bus line low
#define BUS_LOW()                   \
__asm__ __volatile__ (      \
"cbi %0, %1"                \
:                           \
: "I" (_SFR_IO_ADDR(PORTE)),\
"I" (PE4)                 \
)

//Set bus line high
#define BUS_HIGH()                  \
__asm__ __volatile__ (      \
"sbi %0, %1"                \
:                           \
: "I" (_SFR_IO_ADDR(PORTE)),\
"I" (PE4)                 \
)

//ROM commands
#define ROM_COMMAND_SEARCH_ROM      0xf0
#define ROM_COMMAND_READ_ROM        0x33
#define ROM_COMMAND_MATCH_ROM       0x55
#define ROM_COMMAND_SKIP_ROM        0xcc
#define ROM_COMMAND_ALARM_SEARCH    0xec

//Function commands
#define FUNCTION_COMMAND_CONVERT_T          0x44
#define FUNCTION_COMMAND_WRITE_SCRATCHPAD   0x4e
#define FUNCTION_COMMAND_READ_SCRATCHPAD    0xbe
#define FUNCTION_COMMAND_COPY_SCRATCHPAD    0x48
#define FUNCTION_COMMAND_READ_EE            0xb8
#define FUNCTION_COMMAND_READ_POWER_SUPPLY  0xb4

//Define pin number for bus line
#define BUS_LINE (PINE & 0b00010000)

//Define two different types of commands
#define ROM_COMMAND_TYPE 0
#define FUNC_COMMAND_TYPE 1

//define type to distinguish ROM or function command
typedef uint8_t commandType;

#endif 
