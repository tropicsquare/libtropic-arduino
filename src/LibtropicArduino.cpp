/**
 * @file LibtropicArduino.cpp
 * @brief Implementation of the Libtropic C++ wrapper.
 * @copyright Copyright (c) 2020-2025 Tropic Square s.r.o.
 *
 * @license For the license see file LICENSE.txt file in the root directory of this source tree.
 */

#include "LibtropicArduino.h"

Tropic01::Tropic01(const uint16_t spiCSPin
#if LT_USE_INT_PIN
                   ,
                   const uint16_t intGpioPin
#endif
#if LT_SEPARATE_L3_BUFF
                   ,
                   uint8_t l3Buff[], const uint16_t l3BuffLen
#endif
                   ,
                   const unsigned int rngSeed, SPIClass &spi, SPISettings spiSettings)
{
    // Initialize device structure
    this->device.spi_cs_pin = spiCSPin;
#if LT_USE_INT_PIN
    this->device.int_gpio_pin = intGpioPin;
#endif
    this->device.spi_settings = spiSettings;
    this->device.rng_seed = rngSeed;
    this->device.spi = &spi;
    // Pass device structure to handle
    this->handle.l2.device = &this->device;

    // Initialize crypto context structure and pass to handle
    this->handle.l3.crypto_ctx = &this->cryptoCtx;

#if LT_SEPARATE_L3_BUFF
    this->handle.l3.buff = l3Buff;
    this->handle.l3.buff_len = l3BuffLen;
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

lt_ret_t Tropic01::secureSessionStart(const uint8_t shiPriv[], const uint8_t shiPub[], const lt_pkey_index_t pkeyIndex)
{
    return lt_verify_chip_and_start_secure_session(&this->handle, shiPriv, shiPub, pkeyIndex);
}

lt_ret_t Tropic01::secureSessionEnd(void) { return lt_session_abort(&this->handle); }

lt_ret_t Tropic01::ping(const char msgOut[], char msgIn[], const uint16_t msgLen)
{
    return lt_ping(&this->handle, (uint8_t *)msgOut, (uint8_t *)msgIn, msgLen);
}

lt_ret_t Tropic01::eccKeyGenerate(const lt_ecc_slot_t slot, const lt_ecc_curve_type_t curve)
{
    return lt_ecc_key_generate(&this->handle, slot, curve);
}

lt_ret_t Tropic01::eccKeyStore(const lt_ecc_slot_t slot, const lt_ecc_curve_type_t curve, const uint8_t *key)
{
    return lt_ecc_key_store(&this->handle, slot, curve, key);
}

lt_ret_t Tropic01::eccKeyRead(const lt_ecc_slot_t slot, uint8_t *key, const uint8_t key_max_size,
                              lt_ecc_curve_type_t *curve, lt_ecc_key_origin_t *origin)
{
    return lt_ecc_key_read(&this->handle, slot, key, key_max_size, curve, origin);
}

lt_ret_t Tropic01::eccKeyErase(const lt_ecc_slot_t slot) { return lt_ecc_key_erase(&this->handle, slot); }

lt_ret_t Tropic01::ecdsaSign(const lt_ecc_slot_t slot, const uint8_t msg[], const uint32_t msgLen, uint8_t rs[])
{
    return lt_ecc_ecdsa_sign(&this->handle, slot, msg, msgLen, rs);
}

lt_ret_t Tropic01::eddsaSign(const lt_ecc_slot_t slot, const uint8_t *msg, const uint16_t msg_len, uint8_t *rs)
{
    return lt_ecc_eddsa_sign(&this->handle, slot, msg, msg_len, rs);
}
