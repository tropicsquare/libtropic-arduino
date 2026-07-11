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
                   SPIClass &spi, SPISettings spiSettings)
{
    this->handle = {};
    // Initialize device structure
    this->device.spi_cs_pin = spiCSPin;
#if LT_USE_INT_PIN
    this->device.int_gpio_pin = intGpioPin;
#endif
    this->device.spi_settings = spiSettings;
    this->device.spi = &spi;
    // Pass device structure to handle
    this->handle.l2.device = &this->device;

    // Initialize crypto context structure and pass to handle
    this->handle.l3.crypto_ctx = &this->cryptoCtx;

#if LT_SEPARATE_L3_BUFF
    this->handle.l3.buff = l3Buff;
    this->handle.l3.buff_len = l3BuffLen;
#endif

    this->initialized = false;
}

lt_ret_t Tropic01::begin(void)
{
    if (this->initialized) {
        return LT_OK;
    }

    lt_ret_t ret = lt_init(&this->handle);
    if (ret == LT_OK) {
        this->initialized = true;
    }

    return ret;
}

lt_ret_t Tropic01::end(void)
{
    if (!this->initialized) {
        return LT_OK;
    }
    this->initialized = false;

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

lt_ret_t Tropic01::eccKeyStore(const lt_ecc_slot_t slot, const lt_ecc_curve_type_t curve, const uint8_t key[])
{
    return lt_ecc_key_store(&this->handle, slot, curve, key);
}

lt_ret_t Tropic01::eccKeyRead(const lt_ecc_slot_t slot, uint8_t key[], const uint8_t keyMaxSize,
                              lt_ecc_curve_type_t &curve, lt_ecc_key_origin_t &origin)
{
    return lt_ecc_key_read(&this->handle, slot, key, keyMaxSize, &curve, &origin);
}

lt_ret_t Tropic01::eccKeyErase(const lt_ecc_slot_t slot) { return lt_ecc_key_erase(&this->handle, slot); }

lt_ret_t Tropic01::ecdsaSign(const lt_ecc_slot_t slot, const uint8_t msg[], const uint32_t msgLen, uint8_t rs[])
{
    return lt_ecc_ecdsa_sign(&this->handle, slot, msg, msgLen, rs);
}

lt_ret_t Tropic01::eddsaSign(const lt_ecc_slot_t slot, const uint8_t msg[], const uint16_t msgLen, uint8_t rs[])
{
    return lt_ecc_eddsa_sign(&this->handle, slot, msg, msgLen, rs);
}

lt_ret_t Tropic01::rMemWrite(const uint16_t udataSlot, const uint8_t data[], const uint16_t dataSize)
{
    return lt_r_mem_data_write(&this->handle, udataSlot, data, dataSize);
}

lt_ret_t Tropic01::rMemRead(const uint16_t udataSlot, uint8_t data[], const uint16_t dataMaxSize,
                            uint16_t &dataReadSize)
{
    return lt_r_mem_data_read(&this->handle, udataSlot, data, dataMaxSize, &dataReadSize);
}

lt_ret_t Tropic01::rMemErase(const uint16_t udataSlot) { return lt_r_mem_data_erase(&this->handle, udataSlot); }

lt_ret_t Tropic01::macAndDestroy(const lt_mac_and_destroy_slot_t slot, const uint8_t dataOut[], uint8_t dataIn[])
{
    return lt_mac_and_destroy(&this->handle, slot, dataOut, dataIn);
}

// print chip id into a buffer
lt_ret_t Tropic01::getChipID(lt_chip_id_t &chipId) { 
    return lt_get_info_chip_id(&this->handle, &chipId); 
}

lt_ret_t Tropic01::printChipID(const lt_chip_id_t &chip_id, int (*print_func)(const char *format, ...)) {
    return lt_print_chip_id(&chip_id, print_func);
}

// print bootloader version into a buffer
lt_ret_t Tropic01::getBootloaderFWVersion(uint8_t fw_ver[])
{
    lt_ret_t ret = LT_OK;

    // 1. Save current mode to be able to restore it later
    lt_tr01_mode_t original_mode;
    ret = lt_get_tr01_mode(&this->handle, &original_mode);
    if (ret != LT_OK) {
        return ret;
    }

    // 2. Reboot the device in maintenance mode to be able to read bootloader version
    if (original_mode != LT_TR01_MAINTENANCE) {
        ret = lt_reboot(&this->handle, TR01_MAINTENANCE_REBOOT);
        if (ret != LT_OK) {
            return ret;
        }
    }

    // 3. Read Bootloader FW version
    ret = lt_get_info_riscv_fw_ver(&this->handle, fw_ver);
    if (ret != LT_OK) {
        return ret;
    }

    // 4. Reboot back is done only if the original mode was Application
    if (original_mode == LT_TR01_APPLICATION) {
        ret = lt_reboot(&this->handle, TR01_REBOOT);
    }

    return ret;
}

lt_ret_t Tropic01::printFWHeaders(int (*print_func)(const char *format, ...))
{
    // 1. Save current mode to be able to restore it later
    lt_tr01_mode_t original_mode;
    lt_ret_t ret;
    ret = lt_get_tr01_mode(&this->handle, &original_mode);
    if (ret != LT_OK) {
        return ret;
    }

    // 2. Reboot the device in maintenance mode to be able to read bootloader version
    if (original_mode != LT_TR01_MAINTENANCE) {
        ret = lt_reboot(&this->handle, TR01_MAINTENANCE_REBOOT);
        if (ret != LT_OK) {
            return ret;
        }
    }

    // 3. Print bootloader version using lt_print_fw_header, which reads the header from the chip itself via the handle.
    lt_print_fw_header(&this->handle, TR01_FW_BANK_FW1, print_func);
    lt_print_fw_header(&this->handle, TR01_FW_BANK_FW2, print_func);
    lt_print_fw_header(&this->handle, TR01_FW_BANK_SPECT1, print_func);
    lt_print_fw_header(&this->handle, TR01_FW_BANK_SPECT2, print_func);

    // 4. Restore original mode (if it was application mode, reboot to application mode, if it was maintenance mode, reboot to maintenance mode)
    if (original_mode == LT_TR01_APPLICATION) {
        ret = lt_reboot(&this->handle, TR01_REBOOT);
    }

    return ret;
}

//---------------

lt_ret_t Tropic01::getRiscvFWVersion(uint8_t fw_ver[])
{
    lt_ret_t ret = LT_OK;

    // 1. Save current mode to be able to restore it later
    lt_tr01_mode_t original_mode;
    ret = lt_get_tr01_mode(&this->handle, &original_mode);
    if (ret != LT_OK) {
        return ret;
    }

    // 2. Reboot in Application Mode
    if (original_mode == LT_TR01_MAINTENANCE) {
        ret = lt_reboot(&this->handle, TR01_REBOOT);
        if (ret != LT_OK) {
            return ret;
        }
    }

    // 3. Read RISC-V application firmware version
    ret = lt_get_info_riscv_fw_ver(&this->handle, fw_ver);
    if (ret != LT_OK) {
        return ret;
    }

    // 4. Reboot back is done only if the original mode was Maintenance
    if (original_mode == LT_TR01_MAINTENANCE) {
        ret = lt_reboot(&this->handle, TR01_MAINTENANCE_REBOOT);
    }

    return ret;
}

//---------------

lt_ret_t Tropic01::getSpectFWVersion(uint8_t fw_ver[])
{
    lt_ret_t ret = LT_OK;

    // 1. Save current mode to be able to restore it later
    lt_tr01_mode_t original_mode;
    ret = lt_get_tr01_mode(&this->handle, &original_mode);
    if (ret != LT_OK) {
        return ret;
    }

    // 2. Reboot in Application Mode
    if (original_mode == LT_TR01_MAINTENANCE) {
        ret = lt_reboot(&this->handle, TR01_REBOOT);
        if (ret != LT_OK) {
            return ret;
        }
    }

    // 3. Read RISC-V application firmware version
    ret = lt_get_info_spect_fw_ver(&this->handle, fw_ver);
    if (ret != LT_OK) {
        return ret;
    }

    // 4. Reboot back is done only if the original mode was Maintenance
    if (original_mode == LT_TR01_MAINTENANCE) {
        ret = lt_reboot(&this->handle, TR01_MAINTENANCE_REBOOT);
    }
    
    return ret;
}

//---------------

lt_ret_t Tropic01::randomValueGet(uint8_t rand_buf[], const uint16_t rand_len)
{
    return lt_random_value_get(&this->handle, rand_buf, rand_len);
}

//--------------

lt_ret_t Tropic01::mcounterInit(const lt_mcounter_index_t index, const uint32_t value)
{
    return lt_mcounter_init(&this->handle, index, value);
}

lt_ret_t Tropic01::mcounterGet(const lt_mcounter_index_t index, uint32_t &value)
{
    return lt_mcounter_get(&this->handle, index, &value);
}

lt_ret_t Tropic01::mcounterUpdate(const lt_mcounter_index_t index)
{
    return lt_mcounter_update(&this->handle, index);
}
