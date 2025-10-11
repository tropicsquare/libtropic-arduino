#include "LibtropicArduino.h"

lt_ret_t Tropic01::begin(uint16_t spi_cs_pin, unsigned int rng_seed
#ifdef LT_USE_INT_PIN
                         ,
                         uint16_t int_gpio_pin
#endif
                         ,
                         SPIClass &spi, SPISettings spi_settings)
{
    // Initialize device structure
    this->device.spi_cs_pin = spi_cs_pin;
#ifdef LT_USE_INT_PIN
    this->device.int_gpio_pin = int_gpio_pin;
#endif
    this->device.spi_settings = spi_settings;
    this->device.rng_seed = rng_seed;
    this->device.spi = &spi;
    // Initialize handle
    this->handle.l2.device = &this->device;

    return lt_init(&this->handle);
}

lt_ret_t Tropic01::end(void) { return lt_deinit(&this->handle); }

Tropic01::~Tropic01(void) { this->end(); }