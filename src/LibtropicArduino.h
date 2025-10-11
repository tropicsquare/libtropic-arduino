#ifndef LIBTROPIC_ARDUINO_H
#define LIBTROPIC_ARDUINO_H

#include <Arduino.h>
#include <SPI.h>
#include <stdarg.h>

#include "libtropic.h"
#include "libtropic_common.h"
#include "libtropic_port_arduino.h"

class Tropic01 {
   public:
    ~Tropic01(void);
    lt_ret_t begin(uint16_t spi_cs_pin, unsigned int rng_seed
#ifdef LT_USE_INT_PIN
                   ,
                   uint16_t int_gpio_pin
#endif
                   ,
                   SPIClass &spi = ::SPI, SPISettings spi_settings = SPISettings(10000000, MSBFIRST, SPI_MODE0));
    lt_ret_t end(void);

   private:
    lt_dev_arduino_t device;
    lt_handle_t handle;
};

#endif  // LIBTROPIC_ARDUINO_H