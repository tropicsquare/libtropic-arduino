#ifndef LIBTROPIC_ARDUINO_H
#define LIBTROPIC_ARDUINO_H

/**
 * @file LibtropicArduino.h
 * @brief Declarations of the Libtropic C++ wrapper.
 * @copyright Copyright (c) 2020-2025 Tropic Square s.r.o.
 *
 * @license For the license see file LICENSE.txt file in the root directory of this source tree.
 */

#include <Arduino.h>
#include <SPI.h>

#include "libtropic.h"
#include "libtropic_common.h"
#include "libtropic_mbedtls_v4.h"
#include "libtropic_port_arduino.h"

/**
 * @brief Instance of this class is used to communicate with one TROPIC01 chip.
 *
 */
class Tropic01 {
   public:
    /**
     * @brief Tropic01 constructor, which initializes internal structures.
     * @note Number of arguments depends on some Libtropic's CMake options.
     *
     * @param[in] spiCSPin     GPIO pin where TROPIC01's CS pin is connected
     * @param[in] intGpioPin   GPIO pin where TROPIC01's interrupt pin is connected (is one of the parameters only if
     * LT_USE_INT_PIN=1)
     * @param[in] l3Buff       User-defined L3 buffer (is one of the parameters only if LT_SEPARATE_L3_BUFF=1)
     * @param[in] l3BuffLen    Length of `lf_buff` (is one of the parameters only if LT_SEPARATE_L3_BUFF=1)
     * @param[in] spi          Instance of `SPIClass` to use, defaults to default SPI instance set by `<SPI.h>`
     * @param[in] spiSettings  SPI settings, defaults to tested values. If you want to change them, keep
     * `SPISettings.dataOrder=MSBFIRST` and `SPISettings.dataMode=SPI_MODE0` (required by TROPIC01).
     */
    Tropic01(const uint16_t spiCSPin
#if LT_USE_INT_PIN
             ,
             const uint16_t intGpioPin
#endif
#if LT_SEPARATE_L3_BUFF
             ,
             uint8_t l3Buff[], const uint16_t l3BuffLen
#endif
             ,
             SPIClass &spi = ::SPI, SPISettings spiSettings = SPISettings(10000000, MSBFIRST, SPI_MODE0));

    Tropic01() = delete;
    Tropic01(const Tropic01 &) = delete;
    Tropic01 &operator=(const Tropic01 &) = delete;
    Tropic01(Tropic01 &&) = delete;
    Tropic01 &operator=(Tropic01 &&) = delete;

    /**
     * @brief Initializes resources. Must be called before all other methods are called.
     * @warning SPI.begin() is not called by this method, user must ensure SPI is initialized before calling this
     * method.
     *
     * @retval  LT_OK  Method executed successfully
     * @retval  other  Method did not execute successfully, you might use lt_ret_verbose() to get verbose
     * encoding of returned value
     */
    lt_ret_t begin(void);

    /**
     * @brief Deinitialize resources. Should be called at the end of the program.
     * @warning SPI.end() is not called by this method, user must ensure SPI is deinitialized after calling this
     * method.
     *
     * @retval  LT_OK  Method executed successfully
     * @retval  other  Method did not execute successfully, you might use lt_ret_verbose() to get verbose
     * encoding of returned value
     */
    lt_ret_t end(void);

    /**
     * @brief Establishes Secure Session Channel with TROPIC01.
     *
     * @param shiPriv[in]     Host's private pairing key for the slot `pkeyIndex`
     * @param shiPub[in]      Host's public pairing key for the slot `pkeyIndex`
     * @param pkeyIndex[in]   Pairing key index
     *
     * @retval                LT_OK Method executed successfully
     * @retval                other Method did not execute successfully, you might use lt_ret_verbose() to get verbose
     * encoding of returned value
     */
    lt_ret_t secureSessionStart(const uint8_t shiPriv[], const uint8_t shiPub[], const lt_pkey_index_t pkeyIndex);

    /**
     * @brief Aborts Secure Channel Session with TROPIC01.
     *
     * @retval  LT_OK Method executed successfully
     * @retval  other Method did not execute successfully, you might use lt_ret_verbose() to get verbose
     * encoding of returned value
     */
    lt_ret_t secureSessionEnd(void);

    /**
     * @brief Executes the TROPIC01's Ping command. It is a dummy command to check the Secure Channel Session
     * is valid by exchanging a message with TROPIC01, which is echoed through the Secure Channel.
     *
     * @param msgOut[in]  Ping message going out
     * @param msgIn[out]  Ping message going in
     * @param msgLen[in]  Length of both messages (msgOut and msgIn)
     *
     * @retval             LT_OK Function executed successfully
     * @retval             other Function did not execute successfully, you might use lt_ret_verbose() to get verbose
     * encoding
     */
    lt_ret_t ping(const char msgOut[], char msgIn[], const uint16_t msgLen);

    /**
     * @brief Generates ECC key in the specified ECC key slot.
     *
     * @param slot[in]   Slot number (TR01_ECC_SLOT_0 - TR01_ECC_SLOT_31)
     * @param curve[in]  Type of ECC curve (TR01_CURVE_ED25519 or TR01_CURVE_P256)
     *
     * @retval           LT_OK Method executed successfully
     * @retval           other Method did not execute successfully, you might use lt_ret_verbose() to get verbose
     * encoding of returned value
     */
    lt_ret_t eccKeyGenerate(const lt_ecc_slot_t slot, const lt_ecc_curve_type_t curve);

    /**
     * @brief Stores ECC private key to the specified ECC key slot.
     *
     * @param slot[in]   Slot number (TR01_ECC_SLOT_0 - TR01_ECC_SLOT_31)
     * @param curve[in]  Type of ECC curve (TR01_CURVE_ED25519 or TR01_CURVE_P256)
     * @param key[in]    Private key to store (32 bytes)
     *
     * @retval           LT_OK Method executed successfully
     * @retval           other Method did not execute successfully, you might use lt_ret_verbose() to get verbose
     * encoding of returned value
     */
    lt_ret_t eccKeyStore(const lt_ecc_slot_t slot, const lt_ecc_curve_type_t curve, const uint8_t key[]);

    /**
     * @brief Reads ECC public key corresponding to a private key in the specified ECC key slot.
     *
     * @param slot[in]          Slot number (TR01_ECC_SLOT_0 - TR01_ECC_SLOT_31)
     * @param key[out]          Buffer for retrieving public key (32B for Ed25519, 64B for P256)
     * @param keyMaxSize[in]    Size of the key buffer
     * @param curve[out]        Type of elliptic curve public key (TR01_CURVE_ED25519 or TR01_CURVE_P256)
     * @param origin[out]       Origin of the public key (TR01_CURVE_GENERATED or TR01_CURVE_STORED)
     *
     * @retval                  LT_OK Method executed successfully
     * @retval                  other Method did not execute successfully, you might use lt_ret_verbose() to get verbose
     * encoding of returned value
     */
    lt_ret_t eccKeyRead(const lt_ecc_slot_t slot, uint8_t key[], const uint8_t keyMaxSize, lt_ecc_curve_type_t &curve,
                        lt_ecc_key_origin_t &origin);

    /**
     * @brief Erases ECC key from the specified ECC key slot.
     *
     * @param slot[in]  Slot number (TR01_ECC_SLOT_0 - TR01_ECC_SLOT_31)
     *
     * @retval          LT_OK Method executed successfully
     * @retval          other Method did not execute successfully, you might use lt_ret_verbose() to get verbose
     * encoding of returned value
     */
    lt_ret_t eccKeyErase(const lt_ecc_slot_t slot);

    /**
     * @brief Performs ECDSA signature of a message with a private ECC key stored in TROPIC01.
     *
     * @param slot[in]     Slot containing a private key (TR01_ECC_SLOT_0 - TR01_ECC_SLOT_31)
     * @param msg[in]      Buffer containing a message to sign
     * @param msgLen[in]   Length of the message
     * @param rs[out]      Buffer for storing signature R and S bytes (must be 64 bytes)
     *
     * @retval             LT_OK Method executed successfully
     * @retval             other Method did not execute successfully, you might use lt_ret_verbose() to get verbose
     * encoding of returned value
     */
    lt_ret_t ecdsaSign(const lt_ecc_slot_t slot, const uint8_t msg[], const uint32_t msgLen, uint8_t rs[]);

    /**
     * @brief Performs EdDSA signature of a message with a private ECC key stored in TROPIC01.
     *
     * @param slot[in]     Slot containing a private key (TR01_ECC_SLOT_0 - TR01_ECC_SLOT_31)
     * @param msg[in]      Buffer containing a message to sign (max length 4096 bytes)
     * @param msgLen[in]   Length of the message
     * @param rs[out]      Buffer for storing signature R and S bytes (must be 64 bytes)
     *
     * @retval             LT_OK Method executed successfully
     * @retval             other Method did not execute successfully, you might use lt_ret_verbose() to get verbose
     * encoding of returned value
     */
    lt_ret_t eddsaSign(const lt_ecc_slot_t slot, const uint8_t msg[], const uint16_t msgLen, uint8_t rs[]);

    /**
     * @brief Writes bytes into a given slot of the User Partition in the R memory.
     *
     * @param udataSlot[in]  Memory slot to be written (0 - TR01_R_MEM_DATA_SLOT_MAX)
     * @param data[in]       Buffer of data to be written into R memory slot
     * @param dataSize[in]   Size of data to be written into slot. Minimal size is TR01_R_MEM_DATA_SIZE_MIN, maximal
     * size depends on TROPIC01 Application FW and is either 444B (TROPIC01 App FW version <2.0.0) or 475B (TROPIC01 App
     * FW version >=2.0.0)
     *
     * @retval               LT_OK Method executed successfully
     * @retval               other Method did not execute successfully, you might use lt_ret_verbose() to get verbose
     * encoding of returned value
     */
    lt_ret_t rMemWrite(const uint16_t udataSlot, const uint8_t data[], const uint16_t dataSize);

    /**
     * @brief Reads bytes from a given slot of the User Partition in the R memory.
     *
     * @param udataSlot[in]      Memory slot to be read (0 - TR01_R_MEM_DATA_SLOT_MAX)
     * @param data[out]          Buffer to read data into
     * @param dataMaxSize[in]    Size of the data buffer
     * @param dataReadSize[out]  Number of bytes read from TROPIC01 slot into data buffer
     *
     * @retval                   LT_OK Method executed successfully
     * @retval                   other Method did not execute successfully, you might use lt_ret_verbose() to get
     * verbose encoding of returned value
     */
    lt_ret_t rMemRead(const uint16_t udataSlot, uint8_t data[], const uint16_t dataMaxSize, uint16_t &dataReadSize);

    /**
     * @brief Erases the given slot of the User Partition in the R memory.
     *
     * @param udataSlot[in]  Memory slot to be erased (0 - TR01_R_MEM_DATA_SLOT_MAX)
     *
     * @retval                LT_OK Method executed successfully
     * @retval                other Method did not execute successfully, you might use lt_ret_verbose() to get verbose
     * encoding of returned value
     */
    lt_ret_t rMemErase(const uint16_t udataSlot);

    /**
     * @brief Executes the MAC-and-Destroy sequence.
     * @details This method is part of the MAC-and-Destroy PIN verification engine.
     *          For complete implementation examples, see the mac_and_destroy example.
     *          For more information, refer to the TROPIC01 Application Note on PIN verification.
     *
     * @param slot[in]      MAC-and-Destroy slot index (TR01_MAC_AND_DESTROY_SLOT_0 - TR01_MAC_AND_DESTROY_SLOT_127)
     * @param dataOut[in]   Data to be sent from host to TROPIC01 (32 bytes)
     * @param dataIn[out]   Data returned from TROPIC01 to host (32 bytes)
     *
     * @retval              LT_OK Method executed successfully
     * @retval              other Method did not execute successfully, you might use lt_ret_verbose() to get verbose
     * encoding of returned value
     */
    lt_ret_t macAndDestroy(const lt_mac_and_destroy_slot_t slot, const uint8_t dataOut[], uint8_t dataIn[]);

   private:
    lt_dev_arduino_t device;
    lt_ctx_mbedtls_v4_t cryptoCtx;
    lt_handle_t handle;
};

#endif  // LIBTROPIC_ARDUINO_H