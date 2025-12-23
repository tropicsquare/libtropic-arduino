/**
 * @file r_memory.ino
 * @brief TROPIC01 R Memory example using the C++ wrapper.
 * @copyright Copyright (c) 2020-2025 Tropic Square s.r.o.
 *
 * @license For the license see file LICENSE.txt file in the root directory of this source tree.
 */

/***************************************************************************
 *
 * R Memory TROPIC01 Example
 *
 * This example demonstrates how to:
 * 1. Establish a Secure Channel Session with TROPIC01.
 * 2. Write data to R memory slots.
 * 3. Read data from R memory slots.
 * 4. Erase R memory slots.
 *
 * R Memory is non-volatile storage in TROPIC01 that
 * can be used to store user data. It contains 512 slots (0-511) with
 * variable slot sizes.
 *
 * For more information, refer to:
 * 1. Tropic Square: https://tropicsquare.com/
 * 2. TROPIC01: https://tropicsquare.com/tropic01
 * 3. Libtropic: https://tropicsquare.github.io/libtropic
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

// R Memory slot definitions - using different slots for different data types.
#define R_MEM_SLOT_FOR_STRING 10  // Slot for string data
// -----------------------------------------------------------------------------------------------------

// ------------------------------------ TROPIC01 related variables -------------------------------------
#if LT_SEPARATE_L3_BUFF
// User's own buffer for L3 Layer data.
uint8_t l3_buffer[LT_SIZE_OF_L3_BUFF] __attribute__((aligned(16))) = {0};
#endif

Tropic01 tropic01(TROPIC01_CS_PIN
#if LT_USE_INT_PIN
                  ,
                  TROPIC01_INT_PIN
#endif
#if LT_SEPARATE_L3_BUFF
                  ,
                  l3_buffer, sizeof(l3_buffer)
#endif
);  // TROPIC01 instance.

lt_ret_t returnVal;  // Used for return values of Tropic01's methods.

// Buffers for R memory operations.
uint8_t writeBuffer[256];
uint8_t readBuffer[256];
uint16_t bytesRead;
// -----------------------------------------------------------------------------------------------------

// ------------------------------------------ Other variables ------------------------------------------
// Used when initializing MbedTLS's PSA Crypto.
psa_status_t psaStatus;
// -----------------------------------------------------------------------------------------------------

// ---------------------------------------- Utility functions ------------------------------------------
// Used when some error occurs.
static void errorHandler(void)
{
    Serial.println("Starting cleanup...");
    tropic01.end();             // Aborts all communication with TROPIC01 and frees resources.
    mbedtls_psa_crypto_free();  // Frees MbedTLS's PSA Crypto resources.

    Serial.println("Cleanup finished, entering infinite loop...");
    while (true);
}

// Helper function to print hex buffer.
static void printHex(const char *label, const uint8_t *data, const size_t len)
{
    Serial.print(label);
    Serial.print(": ");
    for (size_t i = 0; i < len; i++) {
        if (data[i] < 0x10) Serial.print("0");
        Serial.print(data[i], HEX);
        if (i < len - 1) Serial.print(" ");
        if ((i + 1) % 16 == 0 && (i + 1) < len) {
            Serial.println();
            Serial.print("  ");
        }
    }
    Serial.println();
}

// Helper function to compare buffers.
static bool compareBuffers(const uint8_t *buf1, const uint8_t *buf2, const size_t len)
{
    for (size_t i = 0; i < len; i++) {
        if (buf1[i] != buf2[i]) {
            return false;
        }
    }
    return true;
}
// -----------------------------------------------------------------------------------------------------

// ------------------------------------------ Setup function -------------------------------------------
void setup()
{
    Serial.begin(9600);
    while (!Serial);  // Wait for serial port to connect (useful for native USB)

    Serial.println("===============================================================");
    Serial.println("================ TROPIC01 R Memory Example ====================");
    Serial.println("===============================================================");
    Serial.println();

    Serial.println("---------------------------- Setup ----------------------------");

    // Init MbedTLS's PSA Crypto.
    Serial.println("Initializing MbedTLS PSA Crypto...");
    psaStatus = psa_crypto_init();
    if (psaStatus != PSA_SUCCESS) {
        Serial.print("  MbedTLS's PSA Crypto initialization failed, psa_status_t=");
        Serial.println(psaStatus);
        errorHandler();
    }
    Serial.println("  OK");

    // Init Tropic01 resources.
    Serial.println("Initializing Tropic01 resources...");
    returnVal = tropic01.begin();
    if (returnVal != LT_OK) {
        Serial.print("  Tropic01.begin() failed, returnVal=");
        Serial.println(returnVal);
        errorHandler();
    }
    Serial.println("  OK");

    // Start Secure Channel Session with TROPIC01.
    Serial.println("Starting Secure Channel Session with TROPIC01...");
    returnVal = tropic01.secureSessionStart(PAIRING_KEY_PRIV, PAIRING_KEY_PUB, PAIRING_KEY_SLOT);
    if (returnVal != LT_OK) {
        Serial.print("  Tropic01.secureSessionStart() failed, returnVal=");
        Serial.println(returnVal);
        errorHandler();
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
    // Prepare string data.
    const char *testString = "Hello TROPIC01 R Memory!";
    uint16_t stringLen = strlen(testString);
    memcpy(writeBuffer, testString, stringLen);

    // Erase the slot before writing so the write does not fail if the slot was already written.
    Serial.print("Erasing slot ");
    Serial.print(R_MEM_SLOT_FOR_STRING);
    Serial.println("...");
    returnVal = tropic01.rMemErase(R_MEM_SLOT_FOR_STRING);
    if (returnVal != LT_OK) {
        Serial.print("  Tropic01.rMemErase() failed, returnVal=");
        Serial.println(returnVal);
        errorHandler();
    }
    Serial.println("  OK - Slot erased successfully");
    Serial.println();

    Serial.print("Writing string to slot ");
    Serial.print(R_MEM_SLOT_FOR_STRING);
    Serial.println("...");
    Serial.print("  Data: \"");
    Serial.print(testString);
    Serial.println("\"");
    Serial.print("  Length: ");
    Serial.print(stringLen);
    Serial.println(" bytes");

    returnVal = tropic01.rMemWrite(R_MEM_SLOT_FOR_STRING, writeBuffer, stringLen);
    if (returnVal != LT_OK) {
        Serial.print("  Tropic01.rMemWrite() failed, returnVal=");
        Serial.println(returnVal);
        errorHandler();
    }
    Serial.println("  OK - Data written successfully");
    Serial.println();

    // Read string data back.
    Serial.print("Reading string from slot ");
    Serial.print(R_MEM_SLOT_FOR_STRING);
    Serial.println("...");
    memset(readBuffer, 0, sizeof(readBuffer));

    returnVal = tropic01.rMemRead(R_MEM_SLOT_FOR_STRING, readBuffer, sizeof(readBuffer), bytesRead);
    if (returnVal != LT_OK) {
        Serial.print("  Tropic01.rMemRead() failed, returnVal=");
        Serial.println(returnVal);
        errorHandler();
    }
    Serial.print("  Bytes read: ");
    Serial.println(bytesRead);
    Serial.print("  Data: \"");
    // Null-terminate for printing.
    readBuffer[bytesRead] = '\0';
    Serial.print((char *)readBuffer);
    Serial.println("\"");

    // Verify data.
    if (bytesRead == stringLen && compareBuffers(writeBuffer, readBuffer, stringLen)) {
        Serial.println("  String data verification PASSED!");
    }
    else {
        Serial.println("  String data verification FAILED!");
    }
    Serial.println();

    // Erase string data.
    Serial.print("Erasing slot ");
    Serial.print(R_MEM_SLOT_FOR_STRING);
    Serial.println("...");
    returnVal = tropic01.rMemErase(R_MEM_SLOT_FOR_STRING);
    if (returnVal != LT_OK) {
        Serial.print("  Tropic01.rMemErase() failed, returnVal=");
        Serial.println(returnVal);
        errorHandler();
    }
    Serial.println("  OK - Slot erased successfully");

    Serial.println();
    Serial.println("Success, entering an idle loop.");
    Serial.println("---------------------------------------------------------------");

    while (true);  // Do nothing, end of example.
}
// -----------------------------------------------------------------------------------------------------
