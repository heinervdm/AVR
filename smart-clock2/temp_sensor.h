/******************************
 * file name: temp_sensor.h
 ******************************/
#ifndef TEMP_SENSOR_H
#define TEMP_SENSOR_H

//interface function to return temperature value in deg.C
double get_temp(void);
//interface function to start temperature conversion
void start_conversion(void);
//interface function to check temperature conversion complete
//Boolean conversion_complete(void);
uint8_t conversion_complete(void);

#endif 
