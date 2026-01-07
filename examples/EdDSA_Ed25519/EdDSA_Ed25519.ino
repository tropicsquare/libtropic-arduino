/**
 * @file EdDSA_Ed25519.ino
 * @brief TROPIC01 EdDSA Ed25519 Sign and Verify example using the C++ wrapper for libtropic.
 * @copyright Copyright (c) 2020-2025 Tropic Square s.r.o.
 *
 * @license For the license see file LICENSE.txt file in the root directory of this source tree.
 */

/***************************************************************************
 *
 * EdDSA Ed25519 Sign and Verify TROPIC01 Example
 *
 * This example demonstrates how to:
 * 1. Establish a Secure Channel Session with TROPIC01.
 * 2. Generate an Ed25519 ECC key pair.
 * 3. Read the public key from TROPIC01.
 * 4. Sign a message using EdDSA (Ed25519 curve).
 * 5. Verify the signature on the host side using MbedTLS PSA Crypto.
 * 6. Erase the key from TROPIC01.
 *
 * The example uses slot 1 for Ed25519 (EdDSA) operations.
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

// ECC Key slot definition.
#define ECC_SLOT_ED25519 TR01_ECC_SLOT_1  // Slot for Ed25519 key
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

// Message to sign.
const char message[] = "Hello TROPIC01! This message will be signed with EdDSA Ed25519.";
const uint16_t messageLen = sizeof(message) - 1;  // Exclude null terminator

// Buffers for Ed25519 operations.
uint8_t ed25519PubKey[TR01_CURVE_ED25519_PUBKEY_LEN];
uint8_t ed25519Signature[TR01_ECDSA_EDDSA_SIGNATURE_LENGTH];

// Variables for key info.
lt_ecc_curve_type_t curveType;
lt_ecc_key_origin_t keyOrigin;
// -----------------------------------------------------------------------------------------------------

// ------------------------------------------ Other variables ------------------------------------------
// Used when initializing MbedTLS's PSA Crypto.
psa_status_t psaStatus;
// -----------------------------------------------------------------------------------------------------

// ---------------------------------------- Static local functions ------------------------------------------
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
static void printHex(const char *label, const uint8_t *data, size_t len)
{
    Serial.print(label);
    Serial.print(": ");
    for (size_t i = 0; i < len; i++) {
        if (data[i] < 0x10) Serial.print("0");
        Serial.print(data[i], HEX);
        if ((i + 1) % 32 == 0 && (i + 1) < len) {
            Serial.println();
            Serial.print("  ");
        }
    }
    Serial.println();
}

// Verify EdDSA signature using MbedTLS PSA Crypto.
// static bool verifyEdDSA(const uint8_t *pubKey, const uint8_t *message, size_t messageLen, const uint8_t *signature)
//{
//    // TBD
//}
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
    Serial.println("========== TROPIC01 EdDSA Ed25519 Sign & Verify ===============");
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
// -----------------------------------------------------------------------------------------------------

// ------------------------------------------ Loop function --------------------------------------------
void loop()
{
    Serial.print("Will use Ed25519 key slot #");
    Serial.print(ECC_SLOT_ED25519);
    Serial.println(" for the following EdDSA operations.");
    Serial.println();

    // Erase the slot before writing so the write does not fail if the slot was already written.
    Serial.println("Erasing the Ed25519 key slot...");
    returnVal = tropic01.eccKeyErase(ECC_SLOT_ED25519);
    if (returnVal != LT_OK) {
        printLibtropicError("  Tropic01.eccKeyErase() failed, returnVal=", returnVal);
        cleanResourcesAndLoopForever();
    }
    Serial.println("  OK");

    // Generate Ed25519 key in the slot.
    Serial.println("Generating Ed25519 key in the slot...");
    returnVal = tropic01.eccKeyGenerate(ECC_SLOT_ED25519, TR01_CURVE_ED25519);
    if (returnVal != LT_OK) {
        printLibtropicError("  Tropic01.eccKeyGenerate() failed, returnVal=", returnVal);
        cleanResourcesAndLoopForever();
    }
    Serial.println("  OK");

    // Read Ed25519 public key from the slot.
    Serial.println("Reading Ed25519 public key from the slot...");
    returnVal = tropic01.eccKeyRead(ECC_SLOT_ED25519, ed25519PubKey, sizeof(ed25519PubKey), curveType, keyOrigin);
    if (returnVal != LT_OK) {
        printLibtropicError("  Tropic01.eccKeyRead() failed, returnVal=", returnVal);
        returnVal = tropic01.eccKeyErase(ECC_SLOT_ED25519);
        if (returnVal != LT_OK) {
            printLibtropicError("  Additionally, failed to erase the Ed25519 key slot, returnVal=", returnVal);
        }
        cleanResourcesAndLoopForever();
    }
    Serial.print("  Curve type: ");
    Serial.println(curveType == TR01_CURVE_ED25519 ? "Ed25519" : "Unknown");
    Serial.print("  Key origin: ");
    Serial.println(keyOrigin == TR01_CURVE_GENERATED ? "Generated" : "Stored");
    printHex("  Public key", ed25519PubKey, sizeof(ed25519PubKey));

    // Display message to sign.
    Serial.println("Message to sign:");
    Serial.print("  \"");
    Serial.print(message);
    Serial.println("\"");
    Serial.print("  Length: ");
    Serial.print(messageLen);
    Serial.println(" bytes");
    Serial.println();

    // Sign message with Ed25519 key.
    Serial.println("Signing message with Ed25519 key (EdDSA)...");
    returnVal = tropic01.eddsaSign(ECC_SLOT_ED25519, (const uint8_t *)message, messageLen, ed25519Signature);
    if (returnVal != LT_OK) {
        printLibtropicError("  Tropic01.eddsaSign() failed, returnVal=", returnVal);
        returnVal = tropic01.eccKeyErase(ECC_SLOT_ED25519);
        if (returnVal != LT_OK) {
            printLibtropicError("  Additionally, failed to erase the Ed25519 key slot, returnVal=", returnVal);
        }
        cleanResourcesAndLoopForever();
    }
    printHex("  Signature", ed25519Signature, sizeof(ed25519Signature));

    Serial.println();

    // Erase Ed25519 key from the slot.
    Serial.println("Erasing Ed25519 key from the slot...");
    returnVal = tropic01.eccKeyErase(ECC_SLOT_ED25519);
    if (returnVal != LT_OK) {
        printLibtropicError("  Tropic01.eccKeyErase() failed, returnVal=", returnVal);
        cleanResourcesAndLoopForever();
    }
    Serial.println("  OK");

    Serial.println();
    Serial.println("Success, entering an idle loop.");
    Serial.println("---------------------------------------------------------------");
    cleanResourcesAndLoopForever();
}
// -----------------------------------------------------------------------------------------------------
