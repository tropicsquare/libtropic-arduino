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

lt_handle_t *Tropic01::getHandle() { return &this->handle; }

// chip_id
lt_ret_t Tropic01::getChipID(lt_chip_id_t &chipId) { return lt_get_info_chip_id(&this->handle, &chipId); }

String Tropic01::printChipID(lt_chip_id_t chip_id)
{
    String response = "";

    char print_bytes_buff[TR01_L2_GET_INFO_CHIP_ID_SIZE];

    //* get chip id version as string
    if (lt_print_bytes(chip_id.chip_id_ver, sizeof(chip_id.chip_id_ver), print_bytes_buff, sizeof(print_bytes_buff))
        != LT_OK) {
        response = "ERR:FAILED_TO_READ_CHIP_ID_VER\n";
        return response;
    }

    response = "OK:chip_id:ver = 0x" + String(print_bytes_buff) + " " + "(v" + chip_id.chip_id_ver[0] + "."
               + chip_id.chip_id_ver[1] + "." + chip_id.chip_id_ver[2] + "." + chip_id.chip_id_ver[3] + ")" + ":";

    //* get fl chip info as string
    if (lt_print_bytes(chip_id.fl_chip_info, sizeof(chip_id.fl_chip_info), print_bytes_buff, sizeof(print_bytes_buff))
        != LT_OK) {
        response = "ERR:FAILED_TO_READ_FL_PROD_DATA\n";
        return response;
    }

    String type_fl_prod_data;
    chip_id.fl_chip_info[0] == 0x01 ? type_fl_prod_data = "PASSED" : type_fl_prod_data = "N/A";

    response += "FL_PROD_DATA = 0x" + String(print_bytes_buff) + " " + "(" + type_fl_prod_data + ")" + ":";

    //* get man func test as string
    if (lt_print_bytes(chip_id.func_test_info, sizeof(chip_id.func_test_info), print_bytes_buff,
                       sizeof(print_bytes_buff))
        != LT_OK) {
        response = "ERR:FAILED_TO_READ_MAN_FUNC_TEST\n";
        return response;
    }

    String type_man_func_test;
    chip_id.func_test_info[0] == 0x01 ? type_man_func_test = "PASSED" : type_man_func_test = "N/A";

    response += "MAN_FUNC_TEST = 0x" + String(print_bytes_buff) + " " + "(" + type_man_func_test + ")" + ":";

    //* get silicon rev as string
    if (lt_print_bytes(chip_id.silicon_rev, sizeof(chip_id.silicon_rev), print_bytes_buff, sizeof(print_bytes_buff))
        != LT_OK) {
        response = "ERR:FAILED_TO_READ_SILICON_REV\n";
        return response;
    }

    response += "Silicon rev = 0x" + String(print_bytes_buff) + chip_id.silicon_rev[0] + chip_id.silicon_rev[1]
                + chip_id.silicon_rev[2] + chip_id.silicon_rev[3] + ":";

    //* get package type id as string
    uint16_t packg_type_id = ((uint16_t)chip_id.packg_type_id[0] << 8) | ((uint16_t)chip_id.packg_type_id[1]);
    if (lt_print_bytes(chip_id.packg_type_id, sizeof(chip_id.packg_type_id), print_bytes_buff, sizeof(print_bytes_buff))
        != LT_OK) {
        response = "ERR:FAILED_TO_READ_PACKAGE_TYPE_ID\n";
        return response;
    }

    char packg_type_id_str[17];
    switch (packg_type_id) {
        case TR01_CHIP_PKG_BARE_SILICON_ID:
            strcpy(packg_type_id_str, "Bare silicon die");
            break;

        case TR01_CHIP_PKG_QFN32_ID:
            strcpy(packg_type_id_str, "QFN32, 4x4mm");
            break;

        default:
            strcpy(packg_type_id_str, "N/A");
            break;
    }

    response += "Package ID = 0x" + String(print_bytes_buff) + " (" + String(packg_type_id_str) + ")" + ":";

    //* get prov info ver as string
    char buff_2X[3];
    sprintf(buff_2X, "%02X", chip_id.prov_ver_fab_id_pn[0]);
    String prov_ver_fab_id_str = String(buff_2X);

    response += "Prov info ver = 0x" + prov_ver_fab_id_str + "(v" + chip_id.prov_ver_fab_id_pn[0] + ")" + ":";

    //* get fab id as string
    uint16_t parsed_fab_id = ((chip_id.prov_ver_fab_id_pn[1] << 4) | (chip_id.prov_ver_fab_id_pn[2] >> 4)) & 0xfff;
    String fab_id = "";
    switch (parsed_fab_id) {
        case TR01_FAB_ID_TROPIC_SQUARE_LAB:
            fab_id = "Tropic Square Lab";
            break;

        case TR01_FAB_ID_EPS_BRNO:
            fab_id = "EPS Global - Brno";
            break;

        default:
            fab_id = "N/A";
            break;
    }
    char buff_3X[4];
    sprintf(buff_3X, "%03X", parsed_fab_id);
    String fab_id_str = String(buff_3X);

    response += "Fab ID = 0x" + fab_id_str + " (" + fab_id + ")" + ":";

    //* get P/N ID (short P/N) as string
    uint16_t parsed_short_pn = ((chip_id.prov_ver_fab_id_pn[2] << 8) | (chip_id.prov_ver_fab_id_pn[3])) & 0xfff;
    sprintf(buff_3X, "%03X", parsed_short_pn);
    String parsed_short_str = String(buff_3X);

    response += "P/N ID (short P/N) = 0x" + parsed_short_str + ":";

    //* get prov date as string
    if (lt_print_bytes(chip_id.provisioning_date, sizeof(chip_id.provisioning_date), print_bytes_buff,
                       sizeof(print_bytes_buff))
        != LT_OK) {
        response = "ERR:FAILED_TO_READ_PROVISIONING_DATE\n";
        return response;
    }

    response += "Prov date = 0x" + String(print_bytes_buff) + ":";

    //* get HSM HW/FW/SW ver as string
    if (lt_print_bytes(chip_id.hsm_ver, sizeof(chip_id.hsm_ver), print_bytes_buff, sizeof(print_bytes_buff)) != LT_OK) {
        response = "ERR:FAILED_TO_READ_HSM\n";
        return response;
    }

    response += "HSM HW/FW/SW ver = 0x" + String(print_bytes_buff) + ":";

    //* get programmer ver as string
    if (lt_print_bytes(chip_id.prog_ver, sizeof(chip_id.prog_ver), print_bytes_buff, sizeof(print_bytes_buff))
        != LT_OK) {
        response = "ERR:FAILED_TO_READ_PROGRAMMER_VER\n";
        return response;
    }

    response += "Programmer ver = 0x" + String(print_bytes_buff) + ":";

    //* get S/N as string
    if (lt_print_bytes((uint8_t *)&chip_id.ser_num, sizeof(chip_id.ser_num), print_bytes_buff, sizeof(print_bytes_buff))
        != LT_OK) {
        response = "ERR:FAILED_TO_READ_SER_NUM\n";
        return response;
    }

    response += "S/N = 0x" + String(print_bytes_buff) + ":";

    //* get P/N long as string
    uint8_t pn_len = chip_id.part_num_data[0];
    uint8_t pn_data[16];  // 15B for data, last byte for '\0'
    memcpy(pn_data, &chip_id.part_num_data[1], pn_len);
    pn_data[pn_len] = '\0';

    if (lt_print_bytes(chip_id.part_num_data, sizeof(chip_id.part_num_data), print_bytes_buff, sizeof(print_bytes_buff))
        != LT_OK) {
        response = "ERR:FAILED_TO_READ_P_N\n";
        return response;
    }

    response += "P/N (long) = 0x" + String(print_bytes_buff) + " (" + String((char *)pn_data) + ")" + ":";

    //* get prov template ver as string
    if (lt_print_bytes(chip_id.prov_templ_ver, sizeof(chip_id.prov_templ_ver), print_bytes_buff,
                       sizeof(print_bytes_buff))
        != LT_OK) {
        response = "ERR:FAILED_TO_READ_PROV_TEMPLATE_VER\n";
        return response;
    }

    response += "Prov template ver = 0x" + String(print_bytes_buff) + " (v" + chip_id.prov_templ_ver[0] + "."
                + chip_id.prov_templ_ver[1] + ")" + ":";

    //* get prov template tag as string
    if (lt_print_bytes(chip_id.prov_templ_tag, sizeof(chip_id.prov_templ_tag), print_bytes_buff,
                       sizeof(print_bytes_buff))
        != LT_OK) {
        response = "ERR:FAILED_TO_READ_PROV_TEMPLATE_TAG\n";
        return response;
    }

    response += "Prov template tag = 0x" + String(print_bytes_buff) + ":";

    //* get prov specification ver as string
    if (lt_print_bytes(chip_id.prov_spec_ver, sizeof(chip_id.prov_spec_ver), print_bytes_buff, sizeof(print_bytes_buff))
        != LT_OK) {
        response = "ERR:FAILED_TO_READ_PROV_SPECIFICATION_VER\n";
        return response;
    }

    response += "Prov specification ver = 0x" + String(print_bytes_buff) + ":";

    //* get prov specification tag as string
    if (lt_print_bytes(chip_id.prov_spec_tag, sizeof(chip_id.prov_spec_tag), print_bytes_buff, sizeof(print_bytes_buff))
        != LT_OK) {
        response = "ERR:FAILED_TO_READ_PROV_SPECIFICATION_TAG\n";
        return response;
    }

    response += "Prov specification tag = 0x" + String(print_bytes_buff) + ":";

    //* get batch ID as string
    if (lt_print_bytes(chip_id.batch_id, sizeof(chip_id.batch_id), print_bytes_buff, sizeof(print_bytes_buff))
        != LT_OK) {
        response = "ERR:FAILED_TO_READ_PROV_SPECIFICATION_TAG\n";
        return response;
    }

    response += "Batch ID = 0x" + String(print_bytes_buff) + ";\n";

    return response;
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

    // 2. Restart the device in maintenance mode to be able to read bootloader version
    ret = lt_reboot(&this->handle, TR01_MAINTENANCE_REBOOT);
    if (ret != LT_OK) {
        return ret;
    }

    // 3. Confirm that the device is in maintenance mode
    lt_tr01_mode_t mode = LT_TR01_MAINTENANCE;

    ret = lt_get_tr01_mode(&this->handle, &mode);
    if (ret == LT_OK) {
        response = lt_get_info_riscv_fw_ver(&this->handle, fw_ver);
        if (response != LT_OK) {
            return response;
        }
    }
    else {
        return ret;
    }

    // 4. Restore original mode (if it was application mode, reboot to application mode, if it was maintenance mode, reboot to maintenance mode)
    lt_ret_t reboot_ret;

    if (original_mode == LT_TR01_APPLICATION) {
        reboot_ret = lt_reboot(&this->handle, TR01_REBOOT);
    }
    else if (original_mode == LT_TR01_MAINTENANCE) {
        reboot_ret = lt_reboot(&this->handle, TR01_MAINTENANCE_REBOOT);
    }
    else {
        reboot_ret = LT_FAIL;
    }



    return response;
}

String Tropic01::printBootloaderVersion(uint8_t *fw_ver)
{
    String response = "";

    // Checking if bootloader version is 1.0.1
    if (((fw_ver[3] & 0x7f) == 1) && (fw_ver[2] == 0) && (fw_ver[1] == 1) && (fw_ver[0] == 0)) {
        char buff_2X[3];
        sprintf(buff_2X, "%02X", fw_ver[3] & 0x7f);
        String fw_ver_3 = String(buff_2X);
        sprintf(buff_2X, "%02X", fw_ver[2]);
        String fw_ver_2 = String(buff_2X);
        sprintf(buff_2X, "%02X", fw_ver[1]);
        String fw_ver_1 = String(buff_2X);
        sprintf(buff_2X, "%02X", fw_ver[0]);
        String fw_ver_0 = String(buff_2X);

        response = "OK:Bootloader version = " + fw_ver_3 + "." + fw_ver_2 + "." + fw_ver_1 + " (+ ." + fw_ver_0 + ")";

        response += get_headers_v1();
    }
    else {
        // Checking if bootloader version is 2.0.1
        if (((fw_ver[3] & 0x7f) == 2) && (fw_ver[2] == 0) && (fw_ver[1] == 1) && (fw_ver[0] == 0)) {
            char buff_2X[3];
            sprintf(buff_2X, "%02X", fw_ver[3] & 0x7f);
            String fw_ver_3 = String(buff_2X);
            sprintf(buff_2X, "%02X", fw_ver[2]);
            String fw_ver_2 = String(buff_2X);
            sprintf(buff_2X, "%02X", fw_ver[1]);
            String fw_ver_1 = String(buff_2X);
            sprintf(buff_2X, "%02X", fw_ver[0]);
            String fw_ver_0 = String(buff_2X);

            response
                = "OK:Bootloader version = " + fw_ver_3 + "." + fw_ver_2 + "." + fw_ver_1 + " (+ ." + fw_ver_0 + ")";

            response += get_headers_v2();
        }
        else {
            char buff_2X[3];
            sprintf(buff_2X, "%02X", fw_ver[3] & 0x7f);
            String fw_ver_3 = String(buff_2X);
            sprintf(buff_2X, "%02X", fw_ver[2]);
            String fw_ver_2 = String(buff_2X);
            sprintf(buff_2X, "%02X", fw_ver[1]);
            String fw_ver_1 = String(buff_2X);
            sprintf(buff_2X, "%02X", fw_ver[0]);
            String fw_ver_0 = String(buff_2X);

            response = "ERR:UNKNOWN_BOOTLOADER_VERSION=" + fw_ver_3 + "." + fw_ver_2 + "." + fw_ver_1 + " (+ ."
                       + fw_ver_0 + ");\n";

            lt_deinit(&this->handle);
            return response;
        }
    }

    return response;
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

lt_ret_t Tropic01::secureSessionON(const lt_pkey_index_t pkey_index, const uint8_t shipriv[], const uint8_t shipub[])
{
    lt_ret_t ret;

    // Read certificate store
    uint8_t cert_ese[TR01_L2_GET_INFO_REQ_CERT_SIZE_SINGLE] = {0};
    uint8_t cert_xxxx[TR01_L2_GET_INFO_REQ_CERT_SIZE_SINGLE] = {0};
    uint8_t cert_tr01[TR01_L2_GET_INFO_REQ_CERT_SIZE_SINGLE] = {0};
    uint8_t cert_root[TR01_L2_GET_INFO_REQ_CERT_SIZE_SINGLE] = {0};

    struct lt_cert_store_t cert_store
        = {.certs = {cert_ese, cert_xxxx, cert_tr01, cert_root},
           .buf_len = {TR01_L2_GET_INFO_REQ_CERT_SIZE_SINGLE, TR01_L2_GET_INFO_REQ_CERT_SIZE_SINGLE,
                       TR01_L2_GET_INFO_REQ_CERT_SIZE_SINGLE, TR01_L2_GET_INFO_REQ_CERT_SIZE_SINGLE},
           .cert_len = {0, 0, 0, 0}};

    ret = lt_get_info_cert_store(&this->handle, &cert_store);
    if (LT_OK != ret) {
        // Failed to get Certificate Store , lt_ret_verbose(ret)
        return ret;
    }

    // Extract STPub
    uint8_t stpub[TR01_STPUB_LEN] = {0};
    ret = lt_get_st_pub(&cert_store, stpub);
    if (LT_OK != ret) {
        // Failed to get stpub key, lt_ret_verbose(ret)
        return ret;
    }

    ret = lt_session_start(&this->handle, stpub, pkey_index, shipriv, shipub);
    if (ret != LT_OK) {
        return ret;
    }

    return ret;
}

lt_ret_t Tropic01::secureSessionOFF(void)
{
    lt_ret_t ret;

    // Aborting Secure Session
    ret = lt_session_abort(&this->handle);
    if (LT_OK != ret) {
        // Failed to abort Secure Session, lt_ret_verbose(ret)
        return ret;
    }

    return ret;
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

    // 2. Restart in Application Mode
    ret = lt_reboot(&this->handle, TR01_REBOOT);
    if (ret != LT_OK) {
        return ret;
    }

    // 3. Verify that the device is in application mode
    lt_tr01_mode_t mode = LT_TR01_APPLICATION;

    ret = lt_get_tr01_mode(&this->handle, &mode);
    if (ret == LT_OK) {
        return ret;
    }
    else {
        return ret;
    }

    // 4. Read RISC-V application firmware version
    ret = lt_get_info_riscv_fw_ver(&this->handle, fw_ver);
    if (ret != LT_OK) {
        return ret;
    }

    // 5. Restore original mode (if it was application mode, reboot to application mode, if it was maintenance mode, reboot to maintenance mode)
    if (original_mode == LT_TR01_APPLICATION) {
        lt_reboot(&this->handle, TR01_REBOOT);
    }
    else if (original_mode == LT_TR01_MAINTENANCE) {
        lt_reboot(&this->handle, TR01_MAINTENANCE_REBOOT);
    }



    return ret;
}

String Tropic01::printRiscvFWVersion(uint8_t *fw_ver)
{
    String response = "";

    char buff_2X[3];
    sprintf(buff_2X, "%02X", fw_ver[3]);
    String fw_ver_3 = String(buff_2X);
    sprintf(buff_2X, "%02X", fw_ver[2]);
    String fw_ver_2 = String(buff_2X);
    sprintf(buff_2X, "%02X", fw_ver[1]);
    String fw_ver_1 = String(buff_2X);
    sprintf(buff_2X, "%02X", fw_ver[0]);
    String fw_ver_0 = String(buff_2X);
    response = "OK:RISC-V application FW version = " + fw_ver_3 + "." + fw_ver_2 + "." + fw_ver_1 + " (+ ."
                + fw_ver_0 + "):";

    return response;
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

    // 2. Restart in Application Mode
    ret = lt_reboot(&this->handle, TR01_REBOOT);
    if (ret != LT_OK) {
        return ret;
    }

    // 3. Verify that the device is in application mode
    lt_tr01_mode_t mode = LT_TR01_APPLICATION;

    ret = lt_get_tr01_mode(&this->handle, &mode);
    if (ret == LT_OK) {
        return ret;
    }
    else {
        return ret;
    }

    // 4. Read RISC-V application firmware version
    ret = lt_get_info_spect_fw_ver(&this->handle, fw_ver);
    if (ret != LT_OK) {
        return ret;
    }

    // 5. Restore original mode (if it was application mode, reboot to application mode, if it was maintenance mode, reboot to maintenance mode)
    if (original_mode == LT_TR01_APPLICATION) {
        lt_reboot(&this->handle, TR01_REBOOT);
    }
    else if (original_mode == LT_TR01_MAINTENANCE) {
        lt_reboot(&this->handle, TR01_MAINTENANCE_REBOOT);
    }



    return ret;
}

String Tropic01::printSpectFWVersion(uint8_t *fw_ver)
{
    String response = "";

    char buff_2X[3];
    sprintf(buff_2X, "%02X", fw_ver[3]);
    String fw_ver_3 = String(buff_2X);
    sprintf(buff_2X, "%02X", fw_ver[2]);
    String fw_ver_2 = String(buff_2X);
    sprintf(buff_2X, "%02X", fw_ver[1]);
    String fw_ver_1 = String(buff_2X);
    sprintf(buff_2X, "%02X", fw_ver[0]);
    String fw_ver_0 = String(buff_2X);
    response+= "SPECT firmware version= " + fw_ver_3 + "." + fw_ver_2 + "." + fw_ver_1 + "  (+ ." + fw_ver_0 + ");\n";

    return response;
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

lt_ret_t Tropic01::mcounterGet(const lt_mcounter_index_t index, uint32_t *value)
{
    lt_ret_t ret = lt_mcounter_get(&this->handle, (lt_mcounter_index_t)index, value);
    if (ret != LT_OK) {
        // return "ERR:ENCODE;";
        return ret;
    }

    return ret;
}

lt_ret_t Tropic01::mcounterUpdate(const lt_mcounter_index_t index)
{
    lt_ret_t ret = lt_mcounter_update(&this->handle, (lt_mcounter_index_t)index);
    if (ret != LT_OK) {
        // return "ERR:ENCODE;";
        return ret;
    }

    return ret;
}
