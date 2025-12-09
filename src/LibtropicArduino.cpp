/**
 * @file LibtropicArduino.cpp
 * @brief Implementation of the Libtropic C++ wrapper.
 * @copyright Copyright (c) 2020-2025 Tropic Square s.r.o.
 *
 * @license For the license see file LICENSE.txt file in the root directory of this source tree.
 */

#include "LibtropicArduino.h"

Tropic01::Tropic01(const uint16_t spi_cs_pin
#if LT_USE_INT_PIN
                   ,
                   const uint16_t int_gpio_pin
#endif
#if LT_SEPARATE_L3_BUFF
                   ,
                   uint8_t l3_buff[], const uint16_t l3_buff_len
#endif
                   ,
                   const unsigned int rng_seed, SPIClass &spi, SPISettings spi_settings)
{
    // Initialize device structure
    this->device.spi_cs_pin = spi_cs_pin;
#if LT_USE_INT_PIN
    this->device.int_gpio_pin = int_gpio_pin;
#endif
    this->device.spi_settings = spi_settings;
    this->device.rng_seed = rng_seed;
    this->device.spi = &spi;
    // Pass device structure to handle
    this->handle.l2.device = &this->device;

    // Initialize crypto context structure and pass to handle
    this->handle.l3.crypto_ctx = &this->crypto_ctx;

#if LT_SEPARATE_L3_BUFF
    this->handle.l3.buff = l3_buff;
    this->handle.l3.buff_len = l3_buff_len;
#endif
}

lt_ret_t Tropic01::begin(void) { return lt_init(&this->handle); }

lt_ret_t Tropic01::end(void)
{
    lt_ret_t ret_abort = LT_OK, ret_deinit = LT_OK;

    if (this->handle.l3.session_status == LT_SECURE_SESSION_ON) {
        ret_abort = this->secureSessionEnd();
    }

    ret_deinit = lt_deinit(&this->handle);

    if (ret_abort != LT_OK) {
        return ret_abort;
    }

    return ret_deinit;
}

lt_ret_t Tropic01::secureSessionStart(const uint8_t *shipriv, const uint8_t *shipub, const lt_pkey_index_t pkey_index)
{
    return lt_verify_chip_and_start_secure_session(&this->handle, shipriv, shipub, pkey_index);
}

lt_ret_t Tropic01::secureSessionEnd(void) { return lt_session_abort(&this->handle); }

lt_ret_t Tropic01::ping(const char msg_out[], char msg_in[], const uint16_t msg_len)
{
    return lt_ping(&this->handle, (uint8_t *)msg_out, (uint8_t *)msg_in, msg_len);
}