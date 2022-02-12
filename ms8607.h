/*
 * ms8607.h
 *
 * Author: Mathis Kölker
 */ 

#ifndef _MS8607
#define _MS8607

void ms8607_init(void);

float temperature_in_C(void); 
float pressure_in_mBar(void);
float humidity_in_percentRH(void);

#endif // _MS8607