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
     * @param[in] rngSeed      Seed for the PRNG (defaults to random())
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
             const unsigned int rngSeed = random(), SPIClass &spi = ::SPI,
             SPISettings spiSettings = SPISettings(10000000, MSBFIRST, SPI_MODE0));

    Tropic01() = delete;
    Tropic01(const Tropic01 &) = delete;
    Tropic01 &operator=(const Tropic01 &) = delete;
    Tropic01(Tropic01 &&) = delete;
    Tropic01 &operator=(Tropic01 &&) = delete;

    /**
     * @brief Initializes resources. Must be called before all other methods are called.
     *
     * @retval  LT_OK  Method executed successfully
     * @retval  other  Method did not execute successully, you might use lt_ret_verbose() to get verbose
     * encoding of returned value
     */
    lt_ret_t begin(void);

    /**
     * @brief Deinitialize resources. Should be called at the end of the program.
     *
     * @retval  LT_OK  Method executed successfully
     * @retval  other  Method did not execute successully, you might use lt_ret_verbose() to get verbose
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
     * @retval                other Method did not execute successully, you might use lt_ret_verbose() to get verbose
     * encoding of returned value
     */
    lt_ret_t secureSessionStart(const uint8_t shiPriv[], const uint8_t shiPub[], const lt_pkey_index_t pkeyIndex);

    /**
     * @brief Aborts Secure Channel Session with TROPIC01.
     *
     * @retval  LT_OK Method executed successfully
     * @retval  other Method did not execute successully, you might use lt_ret_verbose() to get verbose
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
     * @retval             other Function did not execute successully, you might use lt_ret_verbose() to get verbose
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
     * @retval           other Method did not execute successully, you might use lt_ret_verbose() to get verbose
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
     * @retval           other Method did not execute successully, you might use lt_ret_verbose() to get verbose
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
     * @retval                  other Method did not execute successully, you might use lt_ret_verbose() to get verbose
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
     * @retval          other Method did not execute successully, you might use lt_ret_verbose() to get verbose
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
     * @retval             other Method did not execute successully, you might use lt_ret_verbose() to get verbose
     * encoding of returned value
     */
    lt_ret_t ecdsaSign(const lt_ecc_slot_t slot, const uint8_t msg[], const uint32_t msgLen, uint8_t rs[]);

    /**
     * @brief Performs EdDSA signature of a message with a private ECC key stored in TROPIC01.
     *
     * @param slot[in]     Slot containing a private key (TR01_ECC_SLOT_0 - TR01_ECC_SLOT_31)
     * @param msg[in]      Buffer containing a message to sign (max length 4096 bytes)
     * @param msg_len[in]  Length of the message
     * @param rs[out]      Buffer for storing signature R and S bytes (must be 64 bytes)
     *
     * @retval             LT_OK Method executed successfully
     * @retval             other Method did not execute successully, you might use lt_ret_verbose() to get verbose
     * encoding of returned value
     */
    lt_ret_t eddsaSign(const lt_ecc_slot_t slot, const uint8_t *msg, const uint16_t msg_len, uint8_t *rs);

   private:
    lt_dev_arduino_t device;
    lt_ctx_mbedtls_v4_t cryptoCtx;
    lt_handle_t handle;
};

#endif  // LIBTROPIC_ARDUINO_H