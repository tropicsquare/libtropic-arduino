/**
 * @file monotonic_counter.ino
 * @brief TROPIC01 Monotonic Counter example using the C++ wrapper.
 * @copyright Copyright (c) 2020-2025 Tropic Square s.r.o.
 *
 * @license For the license see file LICENSE.txt file in the root directory of this source tree.
 */

/***************************************************************************
 *
 * Monotonic Counter TROPIC01 Example
 *
 * This example shows how to:
 * 1. Establish a Secure Channel Session with TROPIC01.
 * 2. Initialize a monotonic counter with an initial value.
 * 3. Read and update (decrement) the monotonic counter until it
 *    reaches zero.
 * 4. Check that the counter cannot be updated anymore once it is zero.
 *
 * TROPIC01 has 16 monotonic counters (indexes 0-15). Their value can only
 * decrease, so they can be used e.g. as anti-rollback or anti-replay
 * protection.
 *
 * WARNING: This example overwrites the value of the monotonic counter
 * with index MCOUNTER_INDEX.
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

// Monotonic counter used in this example and its initial value.
#define MCOUNTER_INDEX TR01_MCOUNTER_INDEX_0
#define MCOUNTER_INIT_VALUE 3
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

// Current value of the monotonic counter.
uint32_t mcounterValue;
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
    Serial.println("============= TROPIC01 Monotonic Counter Example ==============");
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
    Serial.print("Will use monotonic counter #");
    Serial.print(MCOUNTER_INDEX);
    Serial.println(" for the following operations.");
    Serial.println();

    // Initialize the monotonic counter.
    Serial.print("Initializing the counter to ");
    Serial.print(MCOUNTER_INIT_VALUE);
    Serial.println("...");
    returnVal = tropic01.mcounterInit(MCOUNTER_INDEX, MCOUNTER_INIT_VALUE);
    if (returnVal != LT_OK) {
        printLibtropicError("  Tropic01.mcounterInit() failed, returnVal=", returnVal);
        cleanResourcesAndLoopForever();
    }
    Serial.println("  OK");
    Serial.println();

    // Read and decrement the counter until it reaches zero.
    while (true) {
        returnVal = tropic01.mcounterGet(MCOUNTER_INDEX, mcounterValue);
        if (returnVal != LT_OK) {
            printLibtropicError("  Tropic01.mcounterGet() failed, returnVal=", returnVal);
            cleanResourcesAndLoopForever();
        }
        Serial.print("Counter value: ");
        Serial.println(mcounterValue);

        if (mcounterValue == 0) {
            break;
        }

        Serial.println("Updating (decrementing) the counter...");
        returnVal = tropic01.mcounterUpdate(MCOUNTER_INDEX);
        if (returnVal != LT_OK) {
            printLibtropicError("  Tropic01.mcounterUpdate() failed, returnVal=", returnVal);
            cleanResourcesAndLoopForever();
        }
        Serial.println("  OK");
    }
    Serial.println();

    // The counter is zero, so the update is expected to fail with LT_L3_UPDATE_ERR.
    Serial.println("Updating the counter again (expected to fail, the counter is zero)...");
    returnVal = tropic01.mcounterUpdate(MCOUNTER_INDEX);
    if (returnVal == LT_OK) {
        Serial.println("  Tropic01.mcounterUpdate() succeeded, but it was expected to fail!");
        cleanResourcesAndLoopForever();
    }
    else if (returnVal != LT_L3_UPDATE_ERR) {
        printLibtropicError("  Tropic01.mcounterUpdate() failed with unexpected returnVal=",
                            returnVal);
        cleanResourcesAndLoopForever();
    }
    printLibtropicError("  Failed as expected, returnVal=", returnVal);

    Serial.println();
    Serial.println("Success, entering an idle loop.");
    Serial.println("---------------------------------------------------------------");
    cleanResourcesAndLoopForever();
}
// ----------------------------------------------------------------------------------------------------
