# AVR MS8607 Library

This is a library to read the sensor values of an [MS8607](https://www.adafruit.com/product/4716) PHT-sensor from AVR microcontrollers. Developed and tested on an ATmega32 in Microchip Studio.

## Requirements

This library requires an I2C-library in order to work. I recommend you use Peter Fleury's [I2C Master Interface](http://www.peterfleury.epizy.com/avr-software.html?i=1). You'll need `i2cmaster.h` and `i2cmaster.c`. Add them to your project and adjust the include statement in `ms8607.c`. 

This library requires the `_delay_ms()` function from `util/delay.h`. Make sure you have this library installed and that `F_CPU` is defined in your config file.

## Usage

```c
#include "ms8607.h"

int main(void)
{
    ms8607_init();

    float const temp  = temperature_in_C();
    float const press = pressure_in_mBar();
    float const humid = humidity_in_percentRH();
}
```




