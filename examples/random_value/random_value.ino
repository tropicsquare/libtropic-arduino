/**
 * @file random_value.ino
 * @brief TROPIC01 Random Value example using the C++ wrapper.
 * @copyright Copyright (c) 2020-2025 Tropic Square s.r.o.
 *
 * @license For the license see file LICENSE.txt file in the root directory of this source tree.
 */

/***************************************************************************
 *
 * Random Value TROPIC01 Example
 *
 * This example shows how to:
 * 1. Establish a Secure Channel Session with TROPIC01.
 * 2. Get random bytes from the True Random Number Generator (TRNG)
 *    of TROPIC01.
 *
 * Up to TR01_RANDOM_VALUE_GET_LEN_MAX (255) random bytes can be
 * requested in one call.
 *
 * For more information, refer to the following links:
 * 1. Tropic Square: https://tropicsquare.com/
 * 2. TROPIC01: https://tropicsquare.com/tropic01
 * 3. TROPIC01 resources: https://github.com/tropicsquare/tropic01
 * 4. Libtropic: https://tropicsquare.github.io/libtropic
 *
 ***************************************************************************/

// Arduino libraries.
#include <Arduino.h>
#include <SPI.h>
// LibtropicArduino library.
#include <LibtropicArduino.h>
// MbedTLS's PSA Crypto library.
#include "psa/crypto.h"

// ------------------------------------ TROPIC01 related macros --------------------------------------
// GPIO pin definitions.
// Platform's pin number where TROPIC01's SPI Chip Select pin is connected.
#define TROPIC01_CS_PIN 5
#if LT_USE_INT_PIN
// Platform's pin number where TROPIC01's interrupt pin is connected.
// Is necessary only when -DLT_USE_INT_PIN=1 was set in build_flags.
#define TROPIC01_INT_PIN 4
#endif

// Pairing Key macros for establishing a Secure Channel Session with TROPIC01.
// Using the default Pairing Key slot 0 of Production TROPIC01 chips.
#define PAIRING_KEY_PRIV lt_sh0priv_prod0
#define PAIRING_KEY_PUB lt_sh0pub_prod0
#define PAIRING_KEY_SLOT TR01_PAIRING_KEY_SLOT_INDEX_0

// Number of random bytes to get from TROPIC01 (max TR01_RANDOM_VALUE_GET_LEN_MAX).
#define RANDOM_BYTES_LEN 32
// ----------------------------------------------------------------------------------------------------

// ------------------------------------ TROPIC01 related variables ------------------------------------
#if LT_SEPARATE_L3_BUFF
// It is possible to define user's own buffer for L3 Layer data.
// This is handy when using multiple instances of lt_handle_t - only one buffer for all instances will
// be used.
uint8_t l3_buffer[LT_SIZE_OF_L3_BUFF] __attribute__((aligned(16))) = {0};
#endif

// TROPIC01 instance.
// Because Tropic01 constructor has different number of parameters depending on the used Libtropic
// CMake options, we are wrapping its call with the directives, so this example is functional with
// every supported Libtropic CMake option without making any changes to it.
// This is of course not necessary in your application, if you are not frequently changing the
// Libtropic CMake options that affect the Tropic01 constructor parameters.
Tropic01 tropic01(TROPIC01_CS_PIN
#if LT_USE_INT_PIN
                  ,
                  TROPIC01_INT_PIN
#endif
#if LT_SEPARATE_L3_BUFF
                  ,
                  l3_buffer, sizeof(l3_buffer)
#endif
);

// Used for return values of Tropic01's methods.
lt_ret_t returnVal;

// Buffer for the random bytes from TROPIC01.
uint8_t randomBytes[RANDOM_BYTES_LEN];
// ----------------------------------------------------------------------------------------------------

// ------------------------------------------ Other variables -----------------------------------------
// Used when initializing MbedTLS's PSA Crypto.
psa_status_t psaStatus;
// ----------------------------------------------------------------------------------------------------

// -------------------------------------- Static local functions --------------------------------------
// Helper function to save some source code lines when printing Libtropic errors using Serial.
static void printLibtropicError(const char prefixMsg[], const lt_ret_t ret)
{
    Serial.print(prefixMsg);
    Serial.print(ret);
    Serial.print(" (");
    Serial.print(lt_ret_verbose(ret));
    Serial.println(")");
}

static void cleanResourcesAndLoopForever(void)
{
    tropic01.end();             // Aborts all communication with TROPIC01 and frees resources.
    mbedtls_psa_crypto_free();  // Frees MbedTLS's PSA Crypto resources.
    SPI.end();                  // Deinitialize SPI.

    while (true);
}

// Helper function to print hex buffer.
static void printHex(const char label[], const uint8_t data[], const size_t len)
{
    Serial.print(label);
    Serial.print(": ");
    for (size_t i = 0; i < len; i++) {
        if (data[i] < 0x10) {
            Serial.print("0");
        }
        Serial.print(data[i], HEX);
        if (i < len - 1) {
            Serial.print(" ");
        }
        if ((i + 1) % 16 == 0 && (i + 1) < len) {
            Serial.println();
            Serial.print("  ");
        }
    }
    Serial.println();
}
// ----------------------------------------------------------------------------------------------------

// ------------------------------------------ Setup function ------------------------------------------
void setup()
{
    // Initialize SPI (using the default SPI instance defined in <SPI.h>).
    // If you want to use non-default SPI instance, don't forget to pass it to the
    // Tropic01() constructor (otherwise it will use the default SPI instance).
    SPI.begin();

    Serial.begin(9600);
    while (!Serial);  // Wait for serial port to connect.

    Serial.println("===============================================================");
    Serial.println("=============== TROPIC01 Random Value Example =================");
    Serial.println("===============================================================");
    Serial.println();

    Serial.println("---------------------------- Setup ----------------------------");

    // Init MbedTLS's PSA Crypto.
    Serial.println("Initializing MbedTLS PSA Crypto...");
    psaStatus = psa_crypto_init();
    if (psaStatus != PSA_SUCCESS) {
        Serial.print("  MbedTLS's PSA Crypto initialization failed, psa_status_t=");
        Serial.println(psaStatus);
        cleanResourcesAndLoopForever();
    }
    Serial.println("  OK");

    // Init Tropic01 resources.
    Serial.println("Initializing Tropic01 resources...");
    returnVal = tropic01.begin();
    if (returnVal != LT_OK) {
        printLibtropicError("  Tropic01.begin() failed, returnVal=", returnVal);
        cleanResourcesAndLoopForever();
    }
    Serial.println("  OK");

    // Start Secure Channel Session with TROPIC01.
    Serial.println("Starting Secure Channel Session with TROPIC01...");
    returnVal = tropic01.secureSessionStart(PAIRING_KEY_PRIV, PAIRING_KEY_PUB, PAIRING_KEY_SLOT);
    if (returnVal != LT_OK) {
        printLibtropicError("  Tropic01.secureSessionStart() failed, returnVal=", returnVal);
        cleanResourcesAndLoopForever();
    }
    Serial.println("  OK");

    Serial.println("---------------------------------------------------------------");
    Serial.println();
    Serial.println("---------------------------- Loop -----------------------------");
}
// ----------------------------------------------------------------------------------------------------

// ------------------------------------------ Loop function -------------------------------------------
void loop()
{
    Serial.println("--");
    Serial.print("Getting ");
    Serial.print(RANDOM_BYTES_LEN);
    Serial.println(" random bytes from TROPIC01...");

    returnVal = tropic01.randomValueGet(randomBytes, sizeof(randomBytes));
    if (returnVal != LT_OK) {
        printLibtropicError("  Tropic01.randomValueGet() failed, returnVal=", returnVal);
        cleanResourcesAndLoopForever();
    }
    Serial.println("  OK");

    printHex("Random bytes", randomBytes, sizeof(randomBytes));
    Serial.println("--");
    Serial.println();

    // Wait some time before getting new random bytes.
    delay(2000);
}
// ----------------------------------------------------------------------------------------------------
