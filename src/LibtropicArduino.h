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
     * @brief Writes bytes into a given slot of the User Partition in the R memory.
     *
     * @param udata_slot[in]  Memory slot to be written (0 - TR01_R_MEM_DATA_SLOT_MAX)
     * @param data[in]        Buffer of data to be written into R memory slot
     * @param data_size[in]   Size of data to be written into slot. Minimal size is TR01_R_MEM_DATA_SIZE_MIN, maximal
     * size depends on TROPIC01 internal firmware and is either 444 (TROPIC01 fw version <2.0.0) or 475 (TROPIC01 fw
     * version >=2.0.0)
     *
     * @retval                LT_OK Method executed successfully
     * @retval                other Method did not execute successully, you might use lt_ret_verbose() to get verbose
     * encoding of returned value
     */
    lt_ret_t rMemWrite(const uint16_t udata_slot, const uint8_t *data, const uint16_t data_size);

    /**
     * @brief Reads bytes from a given slot of the User Partition in the R memory.
     *
     * @param udata_slot[in]       Memory slot to be read (0 - TR01_R_MEM_DATA_SLOT_MAX)
     * @param data[out]            Buffer to read data into
     * @param data_max_size[in]    Size of the data buffer
     * @param data_read_size[out]  Number of bytes read from TROPIC01 slot into data buffer
     *
     * @retval                     LT_OK Method executed successfully
     * @retval                     other Method did not execute successully, you might use lt_ret_verbose() to get
     * verbose encoding of returned value
     */
    lt_ret_t rMemRead(const uint16_t udata_slot, uint8_t *data, const uint16_t data_max_size, uint16_t *data_read_size);

    /**
     * @brief Erases the given slot of the User Partition in the R memory.
     *
     * @param udata_slot[in]  Memory slot to be erased (0 - TR01_R_MEM_DATA_SLOT_MAX)
     *
     * @retval                LT_OK Method executed successfully
     * @retval                other Method did not execute successully, you might use lt_ret_verbose() to get verbose
     * encoding of returned value
     */
    lt_ret_t rMemErase(const uint16_t udata_slot);

   private:
    lt_dev_arduino_t device;
    lt_ctx_mbedtls_v4_t cryptoCtx;
    lt_handle_t handle;
};

#endif  // LIBTROPIC_ARDUINO_H