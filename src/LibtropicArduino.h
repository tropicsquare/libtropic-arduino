#ifndef LIBTROPIC_ARDUINO_H
#define LIBTROPIC_ARDUINO_H

#include <Arduino.h>
#include <SPI.h>

#include "libtropic.h"
#include "libtropic_common.h"
#include "libtropic_mbedtls_v4.h"
#include "libtropic_port_arduino.h"

class Tropic01 {
   public:
    ~Tropic01(void);
    lt_ret_t begin(const uint16_t spi_cs_pin
#if LT_USE_INT_PIN
                   ,
                   const uint16_t int_gpio_pin
#endif
                   ,
                   const unsigned int rng_seed = random(), SPIClass& spi = ::SPI,
                   SPISettings spi_settings = SPISettings(10000000, MSBFIRST, SPI_MODE0));
    lt_ret_t end(void);
    lt_ret_t secureSessionStart(const uint8_t* shipriv, const uint8_t* shipub, const lt_pkey_index_t pkey_index);
    lt_ret_t secureSessionEnd(void);
    lt_ret_t ping(const uint8_t* msg_out, uint8_t* msg_in, const uint16_t msg_len);

   private:
    lt_dev_arduino_t device;
    lt_ctx_mbedtls_v4_t crypto_ctx;
    lt_handle_t handle;
};

#endif  // LIBTROPIC_ARDUINO_H