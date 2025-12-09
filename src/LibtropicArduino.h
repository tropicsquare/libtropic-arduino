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
     * @param[in] spi_cs_pin    GPIO pin where TROPIC01's CS pin is connected
     * @param[in] int_gpio_pin  GPIO pin where TROPIC01's interrupt pin is connected (is one of the parameters only if
     * LT_USE_INT_PIN=1)
     * @param[in] l3_buff       User-defined L3 buffer (is one of the parameters only if LT_SEPARATE_L3_BUFF=1)
     * @param[in] l3_buff_len   Length of `lf_buff` (is one of the parameters only if LT_SEPARATE_L3_BUFF=1)
     * @param[in] rng_seed      Seed for the PRNG (defaults to random())
     * @param[in] spi           Instance of `SPIClass` to use, defaults to default SPI instance set by `<SPI.h>`
     * @param[in] spi_settings  SPI settings, defaults to tested values. If you want to change them, keep
     * `SPISettings.dataOrder=MSBFIRST` and `SPISettings.dataMode=SPI_MODE0` (required by TROPIC01).
     */
    Tropic01(const uint16_t spi_cs_pin
#if LT_USE_INT_PIN
             ,
             const uint16_t int_gpio_pin
#endif
#if LT_SEPARATE_L3_BUFF
             ,
             uint8_t l3_buff[], const uint16_t l3_buff_len
#endif
             ,
             const unsigned int rng_seed = random(), SPIClass &spi = ::SPI,
             SPISettings spi_settings = SPISettings(10000000, MSBFIRST, SPI_MODE0));

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
     * @param shipriv[in]     Host's private pairing key for the slot `pkey_index`
     * @param shipub[in]      Host's public pairing key for the slot `pkey_index`
     * @param pkey_index[in]  Pairing key index
     *
     * @retval                LT_OK Method executed successfully
     * @retval                other Method did not execute successully, you might use lt_ret_verbose() to get verbose
     * encoding of returned value
     */
    lt_ret_t secureSessionStart(const uint8_t *shipriv, const uint8_t *shipub, const lt_pkey_index_t pkey_index);

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
     * @param msg_out[in]  Ping message going out
     * @param msg_in[out]  Ping message going in
     * @param msg_len[in]  Length of both messages (msg_out and msg_in)
     *
     * @retval             LT_OK Function executed successfully
     * @retval             other Function did not execute successully, you might use lt_ret_verbose() to get verbose
     * encoding
     */
    lt_ret_t ping(const char msg_out[], char msg_in[], const uint16_t msg_len);

   private:
    lt_dev_arduino_t device;
    lt_ctx_mbedtls_v4_t crypto_ctx;
    lt_handle_t handle;
};

#endif  // LIBTROPIC_ARDUINO_H