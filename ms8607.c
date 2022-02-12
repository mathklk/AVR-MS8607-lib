/*
 * ms8607.c
 *
 * Author: Zciurus-Alt-Del
 */ 

#include "ms8607.h"
#include <util/delay.h>

/*
 * Adjust the following two includes to match your project structure
 */
#include "path/to/your/config.h"	// Location of your config file (where F_CPU is defined), required for the _delay_ms function
#include "path/to/your/i2cmaster.h"	// Location of i2cmaster.h (from Peter Fleury's I2C Master Interface)

// I2C-adresses in the 8-bit-format (already shifted left)
#define PT_ADDRESS 0xEC
#define RH_ADDRESS 0x80

uint16_t C1 =	0;
uint16_t C2 =	0;
uint16_t C3 =	0;
uint16_t C4 =	0;
uint16_t C5 =	0;
uint16_t C6 =	0;

int32_t _dt_buffer = 0;


uint16_t ptReadProm(uint8_t const commandCode)
{
	uint8_t lsb = 0, msb = 0;
	
	// Prom Read sequence
	i2c_start_wait(PT_ADDRESS + I2C_WRITE);
	i2c_write(commandCode);
	i2c_stop();

	i2c_start_wait(PT_ADDRESS + I2C_READ);
	msb = i2c_readAck();
	lsb = i2c_readNak();
	i2c_stop();
	
	return (msb << 8) + lsb;
}


void ms8607_init(void)
{
	i2c_init();
	
	// Reset Sequence (Only required for the PT-Sensor, not the H-Sensor)
	i2c_start_wait(PT_ADDRESS + I2C_WRITE);
	i2c_write(0x1E); // 0x1E = Reset
	i2c_stop();
	
	C1 = ptReadProm(0xA2);
	C2 = ptReadProm(0xA4);
	C3 = ptReadProm(0xA6);
	C4 = ptReadProm(0xA8);
	C5 = ptReadProm(0xAA);
	C6 = ptReadProm(0xAC);
}


float temperature_in_C(void)
{
	// D1 = Digital pressure value
	// D2 = Digital Temperature Value
	
	// Start the conversion, the command code specifies temperature/pressure and the resolution
	i2c_start_wait(PT_ADDRESS + I2C_WRITE);
	i2c_write(0x54); // figure 13	0x54 = D2 @ 1024
	i2c_stop();
	
	_delay_ms(3); // short delay during conversion, see datasheet-ms8607 page 3
	
	i2c_start_wait(PT_ADDRESS + I2C_WRITE);
	i2c_write(0x00); // figure 14
	i2c_stop();
	
	uint32_t adc2 = 0, adc1 = 0, adc0 = 0;
	i2c_start_wait(PT_ADDRESS + I2C_READ);
	adc2 = i2c_readAck();
	adc1 = i2c_readAck();
	adc0 = i2c_readNak();
	i2c_stop();
	
	// Converting the raw value to a float, this is specific to the temperature
	uint32_t const D2 = (adc2 << 16) + (adc1 << 8) + adc0; // right-adjusted
	
	uint16_t const two_pow_8 = 256;
	uint32_t const two_pow_23 = 8388608;
	int32_t dT = D2 - (int64_t)C5 * two_pow_8;
	_dt_buffer = dT;
	int32_t TEMP = 2000 + (int64_t)dT * (int64_t)C6 / two_pow_23;
	
	float tempFloat = TEMP;
	return tempFloat / 100;
}

float pressure_in_mBar(void) 
{
	// this first part is almost identical to reading the temperature
	i2c_start_wait(PT_ADDRESS + I2C_WRITE);
	i2c_write(0x42); // not identical to temperature
	i2c_stop();
	_delay_ms(3);
	i2c_start_wait(PT_ADDRESS + I2C_WRITE);
	i2c_write(0x00);
	i2c_stop();
	uint32_t adc2 = 0, adc1 = 0, adc0 = 0;
	i2c_start_wait(PT_ADDRESS + I2C_READ);
	adc2 = i2c_readAck();
	adc1 = i2c_readAck();
	adc0 = i2c_readNak();
	i2c_stop();
	
	// Converting the raw value to a float, this is specific to the pressure
	uint32_t const D1 = (adc2 << 16) + (adc1 << 8) + adc0; // right-adjusted
	
	int64_t const two_pow_6  = 64;
	int64_t const two_pow_7  = 128;
	int64_t const two_pow_15 = 32768;
	int64_t const two_pow_16 = 65536;
	int64_t const two_pow_17 = 131072;
	int64_t const two_pow_21 = 2097152;
	
	int64_t const OFF  = ((int64_t)C2 * two_pow_17) + ((int64_t)C4 * (int64_t)_dt_buffer) / two_pow_6;
	
	int64_t const SENS = ((int64_t)C1 * two_pow_16) + ((int64_t)C3 * (int64_t)_dt_buffer) / two_pow_7;
	int32_t const P = (D1 * SENS / two_pow_21 - OFF) / two_pow_15;
	//int64_t const P = ((((int64_t)D1 * (int64_t)SENS) / two_pow_21) - OFF) / two_pow_15;
	
	float floatP = P;
	floatP /= 100; // to mBar
	return floatP;
}

float humidity_in_percentRH(void)
{
	// Reading the humidity is different to temperature and pressure
	i2c_start_wait(RH_ADDRESS + I2C_WRITE);
	i2c_write(0xF5); // measure RH (no hold master), Datasheet-ms8607 page 6
	i2c_stop();
	
	_delay_ms(17); // Maximum measure time at highest resolution, Datasheet-ms8607 page 3
	
	uint32_t rh1 = 0, rh0 = 0;
	i2c_start_wait(RH_ADDRESS + I2C_READ);
	rh1 = i2c_readAck();
	rh0 = i2c_readAck();
	i2c_readNak(); // who cares about crc?
	i2c_stop();
	
	uint32_t two_pow_16 = 65536;
	
	uint32_t const D3 = (rh1 << 8) + (rh0 & ~(0b11));
	
	int16_t const RH = -600 + 12500 * D3 / two_pow_16; // Datasheet-ms8607 page 11
	
	return RH / 100.0f;
}