/**
 * @file ECDSA_P256.ino
 * @brief Libtropic ECDSA P-256 Sign and Verify example using the C++ wrapper.
 * @copyright Copyright (c) 2020-2025 Tropic Square s.r.o.
 *
 * @license For the license see file LICENSE.txt file in the root directory of this source tree.
 */

/***************************************************************************
 *
 * ECDSA P-256 Sign and Verify TROPIC01 Example
 *
 * This example demonstrates how to:
 * 1. Establish a Secure Channel Session with TROPIC01.
 * 2. Generate a P-256 ECC key pair.
 * 3. Read the public key from TROPIC01.
 * 4. Sign a message using ECDSA (P-256 curve with SHA-256).
 * 5. Verify the signature on the host side using MbedTLS PSA Crypto.
 * 6. Erase the key from TROPIC01.
 *
 * The example uses slot 1 for P-256 (ECDSA) operations.
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
#define ECC_SLOT_P256 TR01_ECC_SLOT_1  // Slot for P-256 key
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
const char message[] = "Hello TROPIC01! This message will be signed with ECDSA P-256.";
const uint32_t messageLen = sizeof(message) - 1;  // Exclude null terminator.

// Buffers for P-256 operations.
uint8_t p256PubKey[TR01_CURVE_P256_PUBKEY_LEN];
uint8_t p256Signature[TR01_ECDSA_EDDSA_SIGNATURE_LENGTH];

// Variables for key info.
lt_ecc_curve_type_t curveType;
lt_ecc_key_origin_t keyOrigin;
// -----------------------------------------------------------------------------------------------------

// ------------------------------------------ Other variables ------------------------------------------
// Used when initializing MbedTLS's PSA Crypto.
psa_status_t psaStatus;
// -----------------------------------------------------------------------------------------------------

// ---------------------------------------- Static local functions ------------------------------------------
// Used when some error occurs.
static void errorHandler(void)
{
    Serial.println("Starting cleanup...");
    tropic01.end();             // Aborts all communication with TROPIC01 and frees resources.
    mbedtls_psa_crypto_free();  // Frees MbedTLS's PSA Crypto resources.

    Serial.println("Cleanup finished, entering infinite loop...");
    while (true)
        ;
}

// Helper function to print hex buffer.
static void printHex(const char *label, const uint8_t *data, const size_t len)
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

// Verify ECDSA signature using MbedTLS PSA Crypto.
static bool verifyECDSA(const uint8_t *pubKey, const uint8_t *message, const size_t messageLen, const uint8_t *signature)
{
    psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;
    psa_key_id_t keyId = 0;
    psa_status_t status;
    bool result = false;
    uint8_t hash[32];  // SHA-256 hash
    size_t hashLen;

    // P-256 public key needs to be in uncompressed format (0x04 prefix + X + Y).
    uint8_t uncompressedPubKey[65];
    uncompressedPubKey[0] = 0x04;  // Uncompressed point indicator
    memcpy(&uncompressedPubKey[1], pubKey, TR01_CURVE_P256_PUBKEY_LEN);

    // Set key attributes for P-256.
    psa_set_key_usage_flags(&attributes, PSA_KEY_USAGE_VERIFY_HASH);
    psa_set_key_algorithm(&attributes, PSA_ALG_ECDSA(PSA_ALG_SHA_256));
    psa_set_key_type(&attributes, PSA_KEY_TYPE_ECC_PUBLIC_KEY(PSA_ECC_FAMILY_SECP_R1));
    psa_set_key_bits(&attributes, 256);

    // Import public key.
    status = psa_import_key(&attributes, uncompressedPubKey, sizeof(uncompressedPubKey), &keyId);
    if (status != PSA_SUCCESS) {
        Serial.print("  Failed to import P-256 public key, status=");
        Serial.println(status);
        goto cleanup;
    }

    // Hash the message with SHA-256.
    status = psa_hash_compute(PSA_ALG_SHA_256, message, messageLen, hash, sizeof(hash), &hashLen);
    if (status != PSA_SUCCESS) {
        Serial.print("  Failed to hash message, status=");
        Serial.println(status);
        goto cleanup;
    }

    // Verify signature.
    status = psa_verify_hash(keyId, PSA_ALG_ECDSA(PSA_ALG_SHA_256), hash, hashLen, signature,
                             TR01_ECDSA_EDDSA_SIGNATURE_LENGTH);
    if (status == PSA_SUCCESS) {
        result = true;
    }
    else {
        Serial.print("  ECDSA verification failed, status=");
        Serial.println(status);
    }

cleanup:
    if (keyId != 0) {
        psa_destroy_key(keyId);
    }
    psa_reset_key_attributes(&attributes);
    return result;
}
// -----------------------------------------------------------------------------------------------------

// ------------------------------------------ Setup function -------------------------------------------
void setup()
{
    Serial.begin(9600);
    while (!Serial)
        ;  // Wait for serial port to connect (useful for native USB)

    Serial.println("===============================================================");
    Serial.println("============ TROPIC01 ECDSA P-256 Sign & Verify ===============");
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

    // Generate P-256 key in slot 1.
    Serial.println("Generating P-256 key in slot 1...");
    returnVal = tropic01.eccKeyGenerate(ECC_SLOT_P256, TR01_CURVE_P256);
    if (returnVal != LT_OK) {
        Serial.print("  Tropic01.eccKeyGenerate() failed, returnVal=");
        Serial.println(returnVal);
        errorHandler();
    }
    Serial.println("  OK");

    // Read P-256 public key from slot 1.
    Serial.println("Reading P-256 public key from slot 1...");
    returnVal = tropic01.eccKeyRead(ECC_SLOT_P256, p256PubKey, sizeof(p256PubKey), &curveType, &keyOrigin);
    if (returnVal != LT_OK) {
        Serial.print("  eccKeyRead() failed, returnVal=");
        Serial.println(returnVal);
        errorHandler();
    }
    Serial.print("  Curve type: ");
    Serial.println(curveType == TR01_CURVE_P256 ? "P-256" : "Unknown");
    Serial.print("  Key origin: ");
    Serial.println(keyOrigin == TR01_CURVE_GENERATED ? "Generated" : "Stored");
    printHex("  Public key", p256PubKey, sizeof(p256PubKey));

    // Display message to sign.
    Serial.println("Message to sign:");
    Serial.print("  \"");
    Serial.print(message);
    Serial.println("\"");
    Serial.print("  Length: ");
    Serial.print(messageLen);
    Serial.println(" bytes");
    Serial.println();

    // Sign message with P-256 key.
    Serial.println("Signing message with P-256 key (ECDSA with SHA-256)...");
    returnVal = tropic01.ecdsaSign(ECC_SLOT_P256, (const uint8_t *)message, messageLen, p256Signature);
    if (returnVal != LT_OK) {
        Serial.print("  ecdsaSign() failed, returnVal=");
        Serial.println(returnVal);
        errorHandler();
    }
    printHex("  Signature", p256Signature, sizeof(p256Signature));

    // Verify P-256 signature.
    Serial.println("Verifying P-256 signature on host...");
    if (verifyECDSA(p256PubKey, (const uint8_t *)message, messageLen, p256Signature)) {
        Serial.println("  P-256 signature verification PASSED!");
    }
    else {
        Serial.println("  P-256 signature verification FAILED!");
    }
    Serial.println();

    // Erase P-256 key from slot 1.
    Serial.println("Erasing P-256 key from slot 1...");
    returnVal = tropic01.eccKeyErase(ECC_SLOT_P256);
    if (returnVal != LT_OK) {
        Serial.print("  eccKeyErase() failed, returnVal=");
        Serial.println(returnVal);
        errorHandler();
    }
    Serial.println("  OK");
    Serial.println("---------------------------------------------------------------");
}
// -----------------------------------------------------------------------------------------------------

// ------------------------------------------ Loop function --------------------------------------------
void loop()
{
    // Everything is done in setup, so just do nothing in loop.
    delay(1000);
}
// -----------------------------------------------------------------------------------------------------
