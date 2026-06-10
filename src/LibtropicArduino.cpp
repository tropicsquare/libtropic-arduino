/**
 * @file LibtropicArduino.cpp
 * @brief Implementation of the Libtropic C++ wrapper.
 * @copyright Copyright (c) 2020-2025 Tropic Square s.r.o.
 *
 * @license For the license see file LICENSE.txt file in the root directory of this source tree.
 */

#include "LibtropicArduino.h"

Tropic01::Tropic01(
#if LT_USE_INT_PIN
    , const uint16_t intGpioPin
#endif
#if LT_SEPARATE_L3_BUFF
    ,
    uint8_t l3Buff[], const uint16_t l3BuffLen
#endif
)
{
    this->handle = {0};
    // Initialize device structure
    this->device.cs_pin = SPI_CS_PIN;
#if LT_USE_INT_PIN
    this->device.int_gpio_pin = intGpioPin;
#endif
    this->device.pin_miso = SPI_MISO_PIN;
    this->device.pin_mosi = SPI_MOSI_PIN;
    this->device.pin_sck = SPI_SCK_PIN;
    this->device.spi_baudrate = SPI_BAUDRATE;
    this->device.spi_instance = LT_SPI_PORT;
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

//************************************************************************************ */
//************************** Additional functions for rpi-pico  *************************
//************************************************************************************ */   

// chip_id
lt_ret_t Tropic01::getChipID(lt_chip_id_t &chipId) { return lt_get_info_chip_id(&this->handle, &chipId); }

lt_ret_t Tropic01::printChipID(const lt_chip_id_t &chip_id, int (*print_func)(const char *format, ...)) {
    return lt_print_chip_id(&chip_id, print_func);
}

// bootloader version
lt_ret_t Tropic01::getBootloaderFWVersion(uint8_t *fw_ver)
{
    lt_ret_t response = LT_OK;
    lt_ret_t ret;

    // 1. Save current mode to be able to restore it later
    lt_tr01_mode_t original_mode;
    ret = lt_get_tr01_mode(&this->handle, &original_mode);
    if (ret != LT_OK) {
        return ret;
    }

    // 2. Reboot the device in maintenance mode to be able to read bootloader version
    ret = lt_reboot(&this->handle, TR01_MAINTENANCE_REBOOT);
    if (ret != LT_OK) {
        return ret;
    }

    // 3. Get info RISC-V FW version
    response = lt_get_info_riscv_fw_ver(&this->handle, fw_ver);
    if (response != LT_OK) {
        return response;
    }

    // 4. Restore original mode (if it was application mode, reboot to application mode, if it was maintenance mode, reboot to maintenance mode)
    lt_ret_t reboot_ret;

    if (original_mode == LT_TR01_APPLICATION) {
        reboot_ret = lt_reboot(&this->handle, TR01_REBOOT);
    }

    return response;
}

lt_ret_t Tropic01::printBootloaderVersion(uint8_t *fw_ver, const lt_bank_id_t bank_id,
                            int (*print_func)(const char *format, ...))
{
    return lt_print_fw_header(fw_ver, bank_id, print_func);
}

String Tropic01::get_headers_v1()
{
    String response = "";

    uint8_t header[TR01_L2_GET_INFO_FW_HEADER_SIZE] = {0};
    uint16_t header_read_size = 0;

    // Read header from FW_BANK_FW1
    if (lt_get_info_fw_bank(&this->handle, TR01_FW_BANK_FW1, header, sizeof(header), &header_read_size) == LT_OK) {
        response = header_boot_v1_0_1(header, TR01_FW_BANK_FW1);
    }
    else {
        response = "ERR:FAILED_TO_GET_FW_BANK1;\n";
        return response;
    }

    // Read header from FW_BANK_FW2
    memset(header, 0, sizeof(header));
    if (lt_get_info_fw_bank(&this->handle, TR01_FW_BANK_FW2, header, sizeof(header), &header_read_size) == LT_OK) {
        response += header_boot_v1_0_1(header, TR01_FW_BANK_FW2);
    }
    else {
        response = "ERR:FAILED_TO_GET_FW_BANK2;\n";
        return response;
    }

    // Read header from FW_BANK_SPECT1
    memset(header, 0, sizeof(header));
    if (lt_get_info_fw_bank(&this->handle, TR01_FW_BANK_SPECT1, header, sizeof(header), &header_read_size) == LT_OK) {
        response += header_boot_v1_0_1(header, TR01_FW_BANK_SPECT1);
    }
    else {
        response = "ERR:FAILED_TO_GET_SPECT_BANK1;\n";
        return response;
    }

    // Read header from FW_BANK_SPECT2
    memset(header, 0, sizeof(header));
    if (lt_get_info_fw_bank(&this->handle, TR01_FW_BANK_SPECT2, header, sizeof(header), &header_read_size) == LT_OK) {
        response += header_boot_v1_0_1(header, TR01_FW_BANK_SPECT2);
    }
    else {
        response = "ERR:FAILED_TO_GET_SPECT_BANK2;\n";
        return response;
    }

    return response;
}

String Tropic01::header_boot_v1_0_1(uint8_t *data, lt_bank_id_t bank_id)
{
    String response = "";

    struct lt_header_boot_v1_t *p_h = (struct lt_header_boot_v1_t *)data;

    switch (bank_id) {
        case TR01_FW_BANK_FW1:
            response = "Firmware bank 1 header=";
            break;
        case TR01_FW_BANK_FW2:
            response = "Firmware bank 2 header=";
            break;
        case TR01_FW_BANK_SPECT1:
            response = "SPECT bank 1 header=";
            break;
        case TR01_FW_BANK_SPECT2:
            response = "SPECT bank 2 header=";
            break;
        default:
            response = "Unknown bank ID=" + (int)bank_id;
            return response;
    }

    char buff_2X[3];
    sprintf(buff_2X, "%02X", p_h->type[3]);
    String ph_type_3 = String(buff_2X);
    sprintf(buff_2X, "%02X", p_h->type[2]);
    String ph_type_2 = String(buff_2X);
    sprintf(buff_2X, "%02X", p_h->type[1]);
    String ph_type_1 = String(buff_2X);
    sprintf(buff_2X, "%02X", p_h->type[0]);
    String ph_type_0 = String(buff_2X);

    response += "Type= " + ph_type_3 + ph_type_2 + ph_type_1 + ph_type_3 + ":";

    sprintf(buff_2X, "%02X", p_h->version[3]);
    String ph_version_3 = String(buff_2X);
    sprintf(buff_2X, "%02X", p_h->version[2]);
    String ph_version_2 = String(buff_2X);
    sprintf(buff_2X, "%02X", p_h->version[1]);
    String ph_version_1 = String(buff_2X);
    sprintf(buff_2X, "%02X", p_h->version[0]);
    String ph_version_0 = String(buff_2X);

    response += "Version= " + ph_version_3 + ph_version_2 + ph_version_1 + ph_version_0 + ":";

    sprintf(buff_2X, "%02X", p_h->size[3]);
    String ph_size_3 = String(buff_2X);
    sprintf(buff_2X, "%02X", p_h->size[2]);
    String ph_size_2 = String(buff_2X);
    sprintf(buff_2X, "%02X", p_h->size[1]);
    String ph_size_1 = String(buff_2X);
    sprintf(buff_2X, "%02X", p_h->size[0]);
    String ph_size_0 = String(buff_2X);

    response += "Size= " + ph_size_3 + ph_size_2 + ph_size_1 + ph_size_0 + ":";

    sprintf(buff_2X, "%02X", p_h->git_hash[3]);
    String ph_git_hash_3 = String(buff_2X);
    sprintf(buff_2X, "%02X", p_h->git_hash[2]);
    String ph_git_hash_2 = String(buff_2X);
    sprintf(buff_2X, "%02X", p_h->git_hash[1]);
    String ph_git_hash_1 = String(buff_2X);
    sprintf(buff_2X, "%02X", p_h->git_hash[0]);
    String ph_git_hash_0 = String(buff_2X);

    response += "Git hash= " + ph_git_hash_3 + ph_git_hash_2 + ph_git_hash_1 + ph_git_hash_0 + ":";

    sprintf(buff_2X, "%02X", p_h->hash[3]);
    String ph_hash_3 = String(buff_2X);
    sprintf(buff_2X, "%02X", p_h->hash[2]);
    String ph_hash_2 = String(buff_2X);
    sprintf(buff_2X, "%02X", p_h->hash[1]);
    String ph_hash_1 = String(buff_2X);
    sprintf(buff_2X, "%02X", p_h->hash[0]);
    String ph_hash_0 = String(buff_2X);

    response += "FW hash= " + ph_hash_3 + ph_hash_2 + ph_hash_1 + ph_hash_0 + ":";

    return response;
}

// ----------------------------

String Tropic01::get_headers_v2()
{
    String response = "";

    uint8_t header[TR01_L2_GET_INFO_FW_HEADER_SIZE] = {0};
    uint16_t header_read_size = 0;

    // Read header from FW_BANK_FW1
    if (lt_get_info_fw_bank(&this->handle, TR01_FW_BANK_FW1, header, sizeof(header), &header_read_size) == LT_OK) {
        header_boot_v2_0_1(header, TR01_FW_BANK_FW1);
    }
    else {
        response = "ERR:FAILED_TO_GET_FW_BANK_1;";
        return response;
    }

    // Read header from FW_BANK_FW2
    memset(header, 0, sizeof(header));
    if (lt_get_info_fw_bank(&this->handle, TR01_FW_BANK_FW2, header, sizeof(header), &header_read_size) == LT_OK) {
        header_boot_v2_0_1(header, TR01_FW_BANK_FW2);
    }
    else {
        response = "ERR:FAILED_TO_GET_FW_BANK_2;";
        return response;
    }

    // Read header from FW_BANK_SPECT1
    memset(header, 0, sizeof(header));
    if (lt_get_info_fw_bank(&this->handle, TR01_FW_BANK_SPECT1, header, sizeof(header), &header_read_size) == LT_OK) {
        header_boot_v2_0_1(header, TR01_FW_BANK_SPECT1);
    }
    else {
        response = "ERR:FAILED_TO_GET_SPECT_BANK_1;";
        return response;
    }

    // Read header from FW_BANK_SPECT2
    memset(header, 0, sizeof(header));
    if (lt_get_info_fw_bank(&this->handle, TR01_FW_BANK_SPECT2, header, sizeof(header), &header_read_size) == LT_OK) {
        header_boot_v2_0_1(header, TR01_FW_BANK_SPECT2);
    }
    else {
        response = "ERR:FAILED_TO_GET_SPECT_BANK_2;";
        return response;
    }

    return response;
}

// This function prints the header in the new format used in bootloader version 2.0.1
String Tropic01::header_boot_v2_0_1(uint8_t *data, lt_bank_id_t bank_id)
{
    String response = "";

    struct lt_header_boot_v2_t *p_h = (struct lt_header_boot_v2_t *)data;

    switch (bank_id) {
        case TR01_FW_BANK_FW1:
            response = "Firmware bank 1 header=";
            break;
        case TR01_FW_BANK_FW2:
            response = "Firmware bank 2 header=";
            break;
        case TR01_FW_BANK_SPECT1:
            response = "SPECT bank 1 header=";
            break;
        case TR01_FW_BANK_SPECT2:
            response = "SPECT bank 2 header=";
            break;
        default:
            response = "Unknown bank ID " + String((int)bank_id) + ";";
            return response;
    }

    char buff_4X[5];
    sprintf(buff_4X, "%04X", p_h->type);
    String ph_type = String(buff_4X);

    response += "Type= " + ph_type + ":";

    char buff_2X[3];
    sprintf(buff_2X, "%02X", p_h->padding);
    String ph_padding = String(buff_2X);

    response += "Padding= " + ph_padding + ":";

    sprintf(buff_2X, "%02X", p_h->header_version);
    String ph_header_version = String(buff_2X);

    response += "FW header version= " + ph_header_version + ":\n";

    char buff_8X[9];
    sprintf(buff_8X, "%08X", p_h->ver);
    String ph_ver = String(buff_8X);

    response += "Version= " + ph_ver + ":";

    sprintf(buff_8X, "%08X", p_h->size);
    String ph_size = String(buff_8X);

    response += "Size= " + ph_size + ":";

    sprintf(buff_8X, "%08X", p_h->git_hash);
    String ph_git_hash = String(buff_8X);

    response += "Git hash= " + ph_git_hash + ":";

    // Hash str has 32B
    char hash_str[32 * 2 + 1] = {0};
    for (int i = 0; i < 32; i++) {
        snprintf(hash_str + i * 2, sizeof(hash_str) - i * 2, "%02" PRIX8 "", p_h->hash[i]);
    }

    response += "Hash=" + String(hash_str) + ":";
    response += "Pair version=" + String(p_h->pair_version) + ":";

    return response;
}


//---------------

lt_ret_t Tropic01::getRiscvFWVersion(uint8_t *fw_ver)
{
    lt_ret_t ret = LT_OK;

    // 1. Save current mode to be able to restore it later
    lt_tr01_mode_t original_mode;
    ret = lt_get_tr01_mode(&this->handle, &original_mode);
    if (ret != LT_OK) {
        return ret;
    }

    // 2. Reboot in Application Mode
    ret = lt_reboot(&this->handle, TR01_REBOOT);
    if (ret != LT_OK) {
        return ret;
    }

    // 3. Read RISC-V application firmware version
    ret = lt_get_info_riscv_fw_ver(&this->handle, fw_ver);
    if (ret != LT_OK) {
        return ret;
    }

    // 4. Restore original mode (if it was application mode, reboot to application mode, if it was maintenance mode, reboot to maintenance mode)
    if (original_mode == LT_TR01_APPLICATION) {
        lt_reboot(&this->handle, TR01_REBOOT);
    }

    return ret;
}

//---------------

lt_ret_t Tropic01::getSpectFWVersion(uint8_t *fw_ver)
{
    lt_ret_t ret = LT_OK;

    // 1. Save current mode to be able to restore it later
    lt_tr01_mode_t original_mode;
    ret = lt_get_tr01_mode(&this->handle, &original_mode);
    if (ret != LT_OK) {
        return ret;
    }

    // 2. Reboot in Application Mode
    ret = lt_reboot(&this->handle, TR01_REBOOT);
    if (ret != LT_OK) {
        return ret;
    }

    // 3. Read RISC-V application firmware version
    ret = lt_get_info_spect_fw_ver(&this->handle, fw_ver);
    if (ret != LT_OK) {
        return ret;
    }

    // 4. Restore original mode (if it was application mode, reboot to application mode, if it was maintenance mode, reboot to maintenance mode)
    if (original_mode == LT_TR01_APPLICATION) {
        lt_reboot(&this->handle, TR01_REBOOT);
    }
    
    return ret;
}

//---------------

lt_ret_t Tropic01::getRandomValue(uint8_t *rand_buf, const uint16_t rand_len)
{
    lt_ret_t ret;

    ret = lt_random_value_get(&this->handle, rand_buf, rand_len);
    if (LT_OK != ret) {
        // lt_out__random_value_get failed, lt_ret_verbose(ret));
        return ret;
    }

    return ret;
}

//--------------

lt_ret_t Tropic01::hashMessage(const uint8_t *message, const uint32_t message_len, uint8_t *hash)
{
    lt_ret_t ret;

    ret = lt_sha256_init(&cryptoCtx);
    if (ret != LT_OK) {
        // lt_sha256_init failed, lt_ret_verbose(ret));
        return ret;
    }

    ret = lt_sha256_start(&cryptoCtx);
    if (ret != LT_OK) {
        // lt_sha256_init failed, lt_ret_verbose(ret));
        return ret;
    }

    ret = lt_sha256_update(&cryptoCtx, (uint8_t *)message, message_len);
    if (ret != LT_OK) {
        // lt_sha256_init failed, lt_ret_verbose(ret));
        return ret;
    }

    ret = lt_sha256_finish(&cryptoCtx, hash);
    if (ret != LT_OK) {
        // lt_sha256_init failed, lt_ret_verbose(ret));
        return ret;
    }

    return ret;
}

//--------------

lt_ret_t Tropic01::mcounterInit(const lt_mcounter_index_t index, const uint32_t value)
{
    lt_ret_t ret = lt_mcounter_init(&this->handle, (lt_mcounter_index_t)index, value);
    if (ret != LT_OK) {
        // return "ERR:ENCODE;";
        return ret;
    }

    return ret;
}

lt_ret_t Tropic01::mcounterGet(const lt_mcounter_index_t index, uint32_t &value)
{
    return lt_mcounter_get(&this->handle, (lt_mcounter_index_t)index, &value);
}

lt_ret_t Tropic01::mcounterUpdate(const lt_mcounter_index_t index)
{
    return lt_mcounter_update(&this->handle, (lt_mcounter_index_t)index);
}
