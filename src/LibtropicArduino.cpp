#include "LibtropicArduino.h"

Tropic01::~Tropic01(void) { this->end(); }

lt_ret_t Tropic01::begin(const uint16_t spi_cs_pin
#ifdef LT_USE_INT_PIN
                         ,
                         const uint16_t int_gpio_pin
#endif
                         ,
                         const unsigned int rng_seed, SPIClass& spi, SPISettings spi_settings)
{
    // Initialize device structure
    this->device.spi_cs_pin = spi_cs_pin;
#ifdef LT_USE_INT_PIN
    this->device.int_gpio_pin = int_gpio_pin;
#endif
    this->device.spi_settings = spi_settings;
    this->device.rng_seed = rng_seed;
    this->device.spi = &spi;
    // Pass device structure to handle
    this->handle.l2.device = &this->device;

    // Initialize crypto context structure and pass to handle
    this->handle.l3.crypto_ctx = &this->crypto_ctx;

    return lt_init(&this->handle);
}

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

lt_ret_t Tropic01::secureSessionStart(const uint8_t* shipriv, const uint8_t* shipub, const lt_pkey_index_t pkey_index)
{
    return lt_verify_chip_and_start_secure_session(&this->handle, shipriv, shipub, pkey_index);
}

lt_ret_t Tropic01::secureSessionEnd(void) { return lt_session_abort(&this->handle); }

lt_ret_t Tropic01::ping(const uint8_t* msg_out, uint8_t* msg_in, const uint16_t msg_len)
{
    return lt_ping(&this->handle, msg_out, msg_in, msg_len);
}