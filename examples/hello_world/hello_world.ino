/**
 * @file hello_world.ino
 * @brief Libtropic Hello World example using the C++ wrapper.
 * @copyright Copyright (c) 2020-2025 Tropic Square s.r.o.
 *
 * @license For the license see file LICENSE.txt file in the root directory of this source tree.
 */

/***************************************************************************
 *
 * Hello World TROPIC01 Example
 *
 * This example shows how to:
 * 1. Establish a Secure Channel Session with TROPIC01.
 * 2. Execute the Ping command to send some message to TROPIC01.
 *
 * The Ping command can be used to verify that the Secure Channel Session
 * was successfully established.
 *
 * In the background, the LibtropicArduino.h library uses Libtropic,
 * a C-based Software Development Kit (SDK) developed by Tropic Square,
 * used to interface the TROPIC01 secure element.
 *
 * Because some cryptographic functionality is needed on the host side,
 * MbedTLS library is included below. Libtropic uses it to e.g. decrypt
 * incomming packets from TROPIC01.
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

// -------------------------------------- TROPIC01 related macros --------------------------------------
// GPIO pin definitions.
#define TROPIC01_CS_PIN 5  // Platform's pin number where TROPIC01's SPI Chip Select pin is connected.
#if LT_USE_INT_PIN
#define TROPIC01_INT_PIN \
    4  // Platform's pin number where TROPIC01's interrupt pin is connected.
       // Is necessary only when -DLT_USE_INT_PIN=1 was set in build_flags.
#endif

// Pairing Key macros for establishing a Secure Channel Session with TROPIC01.
// Using the default Pairing Key slot 0 of Production TROPIC01 chips.
#define PAIRING_KEY_PRIV sh0priv_prod0
#define PAIRING_KEY_PUB sh0pub_prod0
#define PAIRING_KEY_SLOT TR01_PAIRING_KEY_SLOT_INDEX_0
// -----------------------------------------------------------------------------------------------------

// ------------------------------------ TROPIC01 related variables -------------------------------------
#if LT_SEPARATE_L3_BUFF
// It is possible to define user's own buffer for L3 Layer data.
// This is handy when using multiple instances of lt_handle_t - only one buffer for all instances will be used.
uint8_t l3_buffer[LT_SIZE_OF_L3_BUFF] __attribute__((aligned(16))) = {0};
#endif

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
);                                             // TROPIC01 instance.
lt_ret_t returnVal;                            // Used for return values of Tropic01's methods.
char pingMsgToSend[] = "Hello World!";         // Ping message we will send to TROPIC01 via the Secure Channel.
char pingMsgToReceive[sizeof(pingMsgToSend)];  // Buffer for receiving the Ping message from TROPIC01.
// -----------------------------------------------------------------------------------------------------

// ------------------------------------------ Other variables ------------------------------------------
// Used when initializing MbedTLS's PSA Crypto.
psa_status_t mbedtlsInitStatus;
// -----------------------------------------------------------------------------------------------------

// ---------------------------------------- Utility functions ------------------------------------------
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
// -----------------------------------------------------------------------------------------------------

// ------------------------------------------ Setup function -------------------------------------------
void setup()
{
    // Initialize SPI (using the default SPI instance defined in <SPI.h>).
    // If you want to use non-default SPI instance, don't forget to pass it to the
    // Tropic01() constructor (otherwise it will use the default SPI instance).
    SPI.begin();

    Serial.begin(9600);
    while (!Serial);  // Wait for serial port to connect.

    Serial.println("===============================================================");
    Serial.println("================ TROPIC01 Hello World Example =================");
    Serial.println("===============================================================");
    Serial.println();

    Serial.println("---------------------------- Setup ----------------------------");

    // Init MbedTLS's PSA Crypto.
    Serial.println("Initializing MbedTLS PSA Crypto...");
    psa_status_t mbedtlsInitStatus = psa_crypto_init();
    if (mbedtlsInitStatus != PSA_SUCCESS) {
        Serial.print("MbedTLS's PSA Crypto initialization failed, psa_status_t=");
        Serial.println(mbedtlsInitStatus);
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
// -----------------------------------------------------------------------------------------------------

// ------------------------------------------ Loop function --------------------------------------------
void loop()
{
    Serial.println("--");
    // Print our Ping message we want to send.
    Serial.print("Sending the following Ping message to TROPIC01: \"");
    Serial.print(pingMsgToSend);
    Serial.println("\"");

    // Ping TROPIC01 with our message.
    returnVal = tropic01.ping(pingMsgToSend, pingMsgToReceive, sizeof(pingMsgToSend));
    if (returnVal != LT_OK) {
        printLibtropicError("  Tropic01.ping() failed, returnVal=", returnVal);
        cleanResourcesAndLoopForever();
    }
    Serial.println("  OK");

    // Print received Ping message from TROPIC01.
    Serial.print("Ping message received from TROPIC01: \"");
    Serial.print(pingMsgToReceive);
    Serial.println("\"");
    Serial.println("--");
    Serial.println();

    // Wait some time before the next Ping.
    delay(2000);
}
// -----------------------------------------------------------------------------------------------------