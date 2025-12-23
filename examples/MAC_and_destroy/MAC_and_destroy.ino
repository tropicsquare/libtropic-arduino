/**
 * @file MAC_and_destroy.ino
 * @brief TROPIC01 MAC-And-Destroy PIN verification example using the C++
 * wrapper for libtropic.
 * @copyright Copyright (c) 2020-2025 Tropic Square s.r.o.
 *
 * @license For the license see file LICENSE.txt file in the root directory of
 * this source tree.
 */

/***************************************************************************
 *
 * MAC-and-Destroy PIN Verification TROPIC01 Example
 *
 * This example demonstrates TROPIC01's MAC-and-Destroy PIN verification,
 * implementing secure PIN set and PIN verify functions.
 *
 * MAC-and-Destroy is a cryptographic protocol that:
 * 1. Uses slots in TROPIC01's flash memory (up to 128 slots, 0-127) together
 * with slots on host side (in non volatile memory)
 * 2. Each slot can be used once per PIN entry attempt
 * 3. Slots are "destroyed" (invalidated) with each failed attempt
 * 4. Correct PIN entry reinitializes all slots and returns the secret key
 *
 * The example includes:
 * - PIN setup with master secret generation
 * - PIN verification with limited attempts
 * - Non-volatile storage in R memory - adapt to your platform
 *
 * Based on libtropic/examples/lt_ex_macandd.c
 *
 * For more information, refer to:
 * 1. Tropic Square: https://tropicsquare.com/
 * 2. TROPIC01: https://tropicsquare.com/tropic01
 * 3. Application Note: ODN_TR01_app_002_pin_verif.pdf
 *
 ***************************************************************************/

// Arduino libraries.
#include <Arduino.h>
#include <SPI.h>
// LibtropicArduino library.
#include <LibtropicArduino.h>
// MbedTLS's PSA Crypto library.
#include "psa/crypto.h"

// -------------------------------------- Configuration ------------------------------------------------
//  Number of MAC-and-Destroy rounds (PIN entry attempts)
// Valid range: 1 to 128 (TR01_MACANDD_ROUNDS_MAX), adjust this value to set the number of
// allowed PIN attempts
#ifndef MACANDD_ROUNDS
#define MACANDD_ROUNDS 5
#endif

#if (MACANDD_ROUNDS > TR01_MACANDD_ROUNDS_MAX)
#error "MACANDD_ROUNDS must be <= TR01_MACANDD_ROUNDS_MAX (128)"
#endif

// R Memory slot for storing MAC-and-Destroy NVM data.
// MAC-And-Destroy needs non volatile storage for storing part of the data,
// which are used during the process of PIN verification. In this example we are
// using last slot in TROPIC01 r-memory area for this.
#define R_MEM_SLOT_MACANDD 511

// PIN configuration.
#define PIN_SIZE_MIN 4
#define PIN_SIZE_MAX 8

// Generate random master secret. When used in production, make sure you generate
// masterSecret with a secure random generator
uint8_t masterSecret[32] = {0xE7, 0xE0, 0x50, 0x35, 0x4A, 0xBC, 0xE5, 0xD4,
                            0xA9, 0xE0, 0x57, 0x68, 0xFE, 0x27, 0x74, 0x05,
                            0x27, 0xC9, 0x88, 0x7E, 0xE9, 0xEC, 0x6F, 0x40,
                            0x98, 0xDC, 0x1F, 0xDD, 0x9F, 0x27, 0x34, 0xBA};
// Define PIN (4 bytes)
uint8_t pin[4] = {1, 2, 3, 4};
// -----------------------------------------------------------------------------------------------------

// -------------------------------------- TROPIC01 related macros --------------------------------------
#define TROPIC01_CS_PIN                                                        \
  5 // Platform's pin number where TROPIC01's SPI Chip Select pin is connected.
#if LT_USE_INT_PIN
#define TROPIC01_INT_PIN                                                       \
  4 // Platform's pin number where TROPIC01's interrupt pin is connected.
    // Is necessary only when -DLT_USE_INT_PIN=1 was set in build_flags.
#endif

// Pairing Key macros for establishing a Secure Channel Session with TROPIC01.
// Using the default Pairing Key slot 0 of Production TROPIC01 chips.
#define PAIRING_KEY_PRIV sh0priv_prod0
#define PAIRING_KEY_PUB sh0pub_prod0
#define PAIRING_KEY_SLOT TR01_PAIRING_KEY_SLOT_INDEX_0
// -----------------------------------------------------------------------------------------------------

// -------------------------------------- TROPIC01 related variables -----------------------------------
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
); // TROPIC01 instance.

lt_ret_t returnVal; // Used for return values of Tropic01's methods.
// -----------------------------------------------------------------------------------------------------

// ------------------------------------------ Other variables ------------------------------------------
// Used when initializing MbedTLS's PSA Crypto
psa_status_t psaStatus;

/**
 * @brief Structure holding MAC-and-Destroy non-volatile memory data.
 * This data must be stored persistently across power cycles.
 */
struct MacAndDestroyNVM {
  uint8_t attemptCount; // Number of remaining attempts
  uint8_t
      encryptedSecrets[MACANDD_ROUNDS * 32]; // Encrypted master secrets (c_i)
  uint8_t tag[32];                           // Verification tag (t)
} __attribute__((packed));
// -----------------------------------------------------------------------------------------------------

// ---------------------------------------- Local static functions -------------------------------------
static void errorHandler(void) {
  Serial.println("Starting cleanup...");
  tropic01.end(); // Aborts all communication with TROPIC01 and frees resources.
  mbedtls_psa_crypto_free(); // Frees MbedTLS's PSA Crypto resources.

  Serial.println("Cleanup finished, entering infinite loop...");
  while (true)
    ;
}

// Helper function to print hex buffer.
static void printHex(const char *label, const uint8_t *data, size_t len) {
  Serial.print(label);
  Serial.print(": ");
  for (size_t i = 0; i < len; i++) {
    if (data[i] < 0x10)
      Serial.print("0");
    Serial.print(data[i], HEX);
    if (i < len - 1 && (i + 1) % 32 != 0)
      Serial.print(" ");
    if ((i + 1) % 32 == 0 && (i + 1) < len) {
      Serial.println();
      Serial.print("  ");
    }
  }
  Serial.println();
}

// Simple XOR encryption/decryption
static void xorCrypt(const uint8_t *input, const uint8_t *key, uint8_t *output,
                     size_t len) {
  for (size_t i = 0; i < len; i++) {
    output[i] = input[i] ^ key[i];
  }
}

// HMAC-SHA256 wrapper using PSA Crypto.
static psa_status_t hmacSha256(const uint8_t *key, size_t keyLen,
                               const uint8_t *data, size_t dataLen,
                               uint8_t *output) {
  psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;
  psa_key_id_t keyId = 0;
  psa_status_t status;
  size_t outputLen;

  // Set key attributes for HMAC.
  psa_set_key_usage_flags(&attributes, PSA_KEY_USAGE_SIGN_HASH);
  psa_set_key_algorithm(&attributes, PSA_ALG_HMAC(PSA_ALG_SHA_256));
  psa_set_key_type(&attributes, PSA_KEY_TYPE_HMAC);

  // Import key.
  status = psa_import_key(&attributes, key, keyLen, &keyId);
  if (status != PSA_SUCCESS) {
    goto cleanup;
  }

  // Compute HMAC.
  status = psa_mac_compute(keyId, PSA_ALG_HMAC(PSA_ALG_SHA_256), data, dataLen,
                           output, 32, &outputLen);

cleanup:
  if (keyId != 0) {
    psa_destroy_key(keyId);
  }
  psa_reset_key_attributes(&attributes);
  return status;
}

// ------------------------------------ PIN Set and Verify Functions -----------------------------------
/**
 * @brief Set up a new PIN with MAC-and-Destroy.
 *
 * @param masterSecret[in]  32-byte random master secret
 * @param pin[in]           PIN bytes (4-8 bytes)
 * @param pinSize[in]       Length of PIN
 * @param finalKey[out]     32-byte final key derived from master secret
 *
 * @return LT_OK on success, error code otherwise
 */
static lt_ret_t pinSetup(const uint8_t *masterSecret, const uint8_t *pin,
                         uint8_t pinSize, uint8_t *finalKey) {
  if (!masterSecret || !pin || pinSize < PIN_SIZE_MIN ||
      pinSize > PIN_SIZE_MAX || !finalKey) {
    return LT_PARAM_ERR;
  }

  MacAndDestroyNVM nvm = {0};
  uint8_t v[32] = {0};
  uint8_t w_i[32] = {0};
  uint8_t k_i[32] = {0};
  uint8_t u[32] = {0};
  uint8_t ignore[32] = {0};
  const uint8_t zeros[32] = {0};
  const uint8_t byte_00[1] = {0x00};
  const uint8_t byte_01[1] = {0x01};
  lt_ret_t ret;

  Serial.println("PIN Setup starting...");

  // Erase R memory slot.
  Serial.println("  Erasing R memory slot...");
  ret = tropic01.rMemErase(R_MEM_SLOT_MACANDD);
  if (ret != LT_OK) {
    Serial.print("    Failed, returnVal=");
    Serial.println(ret);
    return ret;
  }
  Serial.println("    OK");

  // Store number of attempts.
  nvm.attemptCount = MACANDD_ROUNDS;

  // Compute tag t = HMAC(masterSecret, 0x00).
  Serial.println("  Computing verification tag...");
  if (hmacSha256(masterSecret, 32, byte_00, 1, nvm.tag) != PSA_SUCCESS) {
    Serial.println("    Failed");
    return LT_FAIL;
  }
  Serial.println("    OK");

  // Compute u = HMAC(masterSecret, 0x01).
  if (hmacSha256(masterSecret, 32, byte_01, 1, u) != PSA_SUCCESS) {
    return LT_FAIL;
  }

  // Compute v = HMAC(zeros, PIN).
  if (hmacSha256(zeros, 32, pin, pinSize, v) != PSA_SUCCESS) {
    return LT_FAIL;
  }

  Serial.print("  Initializing ");
  Serial.print(MACANDD_ROUNDS);
  Serial.println(" MAC-and-Destroy slots...");

  for (int i = 0; i < MACANDD_ROUNDS; i++) {
    // Initialize slot with u.
    ret = tropic01.macAndDestroy((lt_mac_and_destroy_slot_t)i, u, ignore);
    if (ret != LT_OK) {
      Serial.print("    Slot ");
      Serial.print(i);
      Serial.print(" init failed, returnVal=");
      Serial.println(ret);
      return ret;
    }

    // Overwrite with v to get w_i.
    ret = tropic01.macAndDestroy((lt_mac_and_destroy_slot_t)i, v, w_i);
    if (ret != LT_OK) {
      return ret;
    }

    // Re-initialize with u.
    ret = tropic01.macAndDestroy((lt_mac_and_destroy_slot_t)i, u, ignore);
    if (ret != LT_OK) {
      return ret;
    }

    // Derive k_i = HMAC(w_i, PIN).
    if (hmacSha256(w_i, 32, pin, pinSize, k_i) != PSA_SUCCESS) {
      return LT_FAIL;
    }

    // Encrypt master secret with k_i.
    xorCrypt(masterSecret, k_i, &nvm.encryptedSecrets[i * 32], 32);

    if ((i + 1) % 10 == 0 || i == MACANDD_ROUNDS - 1) {
      Serial.print("    ");
      Serial.print(i + 1);
      Serial.print("/");
      Serial.print(MACANDD_ROUNDS);
      Serial.println(" slots initialized");
    }
  }

  // Store NVM data to R memory.
  Serial.println("  Saving NVM data to R memory...");
  ret = tropic01.rMemWrite(R_MEM_SLOT_MACANDD, (uint8_t *)&nvm, sizeof(nvm));
  if (ret != LT_OK) {
    Serial.print("    Failed, returnVal=");
    Serial.println(ret);
    return ret;
  }
  Serial.println("    OK");

  // Derive final key = HMAC(masterSecret, "2").
  if (hmacSha256(masterSecret, 32, (uint8_t *)"2", 1, finalKey) !=
      PSA_SUCCESS) {
    return LT_FAIL;
  }

  Serial.println("PIN Setup completed successfully!");

  // Clear sensitive data.
  memset(v, 0, sizeof(v));
  memset(w_i, 0, sizeof(w_i));
  memset(k_i, 0, sizeof(k_i));
  memset(u, 0, sizeof(u));

  return LT_OK;
}

/**
 * @brief Verify PIN with MAC-and-Destroy.
 *
 * @param pin[in]       PIN bytes to verify (4-8 bytes)
 * @param pinSize[in]   Length of PIN
 * @param finalKey[out] 32-byte final key (only valid if PIN correct)
 *
 * @return LT_OK on correct PIN, LT_FAIL on incorrect PIN or error
 */
static lt_ret_t pinVerify(const uint8_t *pin, uint8_t pinSize,
                          uint8_t *finalKey) {
  if (!pin || pinSize < PIN_SIZE_MIN || pinSize > PIN_SIZE_MAX || !finalKey) {
    return LT_PARAM_ERR;
  }

  MacAndDestroyNVM nvm = {0};
  uint8_t v_[32] = {0};
  uint8_t w_i[32] = {0};
  uint8_t k_i[32] = {0};
  uint8_t s_[32] = {0};
  uint8_t t_[32] = {0};
  uint8_t u[32] = {0};
  uint8_t ignore[32] = {0};
  const uint8_t zeros[32] = {0};
  const uint8_t byte_00[1] = {0x00};
  const uint8_t byte_01[1] = {0x01};
  uint16_t bytesRead;
  lt_ret_t ret;

  Serial.println("PIN Verification starting...");

  // Load NVM data from R memory.
  Serial.println("  Loading NVM data from R memory...");
  ret = tropic01.rMemRead(R_MEM_SLOT_MACANDD, (uint8_t *)&nvm, sizeof(nvm),
                          bytesRead);
  if (ret != LT_OK) {
    Serial.print("    Failed, returnVal=");
    Serial.println(ret);
    return ret;
  }
  Serial.println("    OK");

  // Check if attempts remaining.
  Serial.print("  Attempts remaining: ");
  Serial.println(nvm.attemptCount);
  if (nvm.attemptCount == 0) {
    Serial.println("  No attempts remaining!");
    return LT_FAIL;
  }

  // Decrement attempt count.
  nvm.attemptCount--;

  // Save decremented count immediately.
  Serial.println("  Updating attempt count...");
  ret = tropic01.rMemErase(R_MEM_SLOT_MACANDD);
  if (ret != LT_OK)
    return ret;
  ret = tropic01.rMemWrite(R_MEM_SLOT_MACANDD, (uint8_t *)&nvm, sizeof(nvm));
  if (ret != LT_OK)
    return ret;
  Serial.println("    OK");

  // Compute v' = HMAC(zeros, PIN).
  if (hmacSha256(zeros, 32, pin, pinSize, v_) != PSA_SUCCESS) {
    return LT_FAIL;
  }

  // Execute MAC-and-Destroy to get w'.
  Serial.println("  Executing MAC-and-Destroy...");
  ret = tropic01.macAndDestroy((lt_mac_and_destroy_slot_t)nvm.attemptCount, v_,
                               w_i);
  if (ret != LT_OK) {
    Serial.print("    Failed, returnVal=");
    Serial.println(ret);
    return ret;
  }
  Serial.println("    OK");

  // Compute k'_i = HMAC(w', PIN).
  if (hmacSha256(w_i, 32, pin, pinSize, k_i) != PSA_SUCCESS) {
    return LT_FAIL;
  }

  // Decrypt to get s'.
  xorCrypt(&nvm.encryptedSecrets[nvm.attemptCount * 32], k_i, s_, 32);

  // Compute t' = HMAC(s', 0x00).
  if (hmacSha256(s_, 32, byte_00, 1, t_) != PSA_SUCCESS) {
    return LT_FAIL;
  }

  // Verify tag.
  Serial.println("  Verifying tag...");
  if (memcmp(nvm.tag, t_, 32) != 0) {
    Serial.println("    Tag mismatch - Incorrect PIN!");
    memset(s_, 0, sizeof(s_));
    memset(k_i, 0, sizeof(k_i));
    return LT_FAIL;
  }
  Serial.println("    OK - PIN is correct!");

  // PIN is correct - reinitialize all slots.
  // Compute u = HMAC(s', 0x01).
  if (hmacSha256(s_, 32, byte_01, 1, u) != PSA_SUCCESS) {
    return LT_FAIL;
  }

  Serial.print("  Reinitializing ");
  Serial.print(MACANDD_ROUNDS - 1 - nvm.attemptCount);
  Serial.println(" slots...");

  for (int x = nvm.attemptCount; x < MACANDD_ROUNDS - 1; x++) {
    ret = tropic01.macAndDestroy((lt_mac_and_destroy_slot_t)x, u, ignore);
    if (ret != LT_OK) {
      return ret;
    }
  }
  Serial.println("    OK");

  // Reset attempt count.
  nvm.attemptCount = MACANDD_ROUNDS;

  // Save NVM data.
  Serial.println("  Saving updated NVM data...");
  ret = tropic01.rMemErase(R_MEM_SLOT_MACANDD);
  if (ret != LT_OK)
    return ret;
  ret = tropic01.rMemWrite(R_MEM_SLOT_MACANDD, (uint8_t *)&nvm, sizeof(nvm));
  if (ret != LT_OK)
    return ret;
  Serial.println("    OK");

  // Derive final key = HMAC(s', "2").
  if (hmacSha256(s_, 32, (uint8_t *)"2", 1, finalKey) != PSA_SUCCESS) {
    return LT_FAIL;
  }

  Serial.println("PIN Verification successful!");

  // Clear sensitive data.
  memset(v_, 0, sizeof(v_));
  memset(w_i, 0, sizeof(w_i));
  memset(k_i, 0, sizeof(k_i));
  memset(s_, 0, sizeof(s_));

  return LT_OK;
}
// -----------------------------------------------------------------------------------------------------

// ------------------------------------------ Setup function -------------------------------------------
void setup() {
  Serial.begin(9600);
  while (!Serial)
    ; // Wait for serial port to connect (useful for native USB)

  Serial.println("===============================================================");
  Serial.println("========== TROPIC01 MAC-and-Destroy PIN Verification ==========");
  Serial.println("===============================================================");
  Serial.println();
  Serial.print("Configuration: ");
  Serial.print(MACANDD_ROUNDS);
  Serial.println(" PIN attempts allowed");
  Serial.println();

  Serial.println(
      "---------------------------- Setup ----------------------------");

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
  returnVal = tropic01.secureSessionStart(PAIRING_KEY_PRIV, PAIRING_KEY_PUB,
                                          PAIRING_KEY_SLOT);
  if (returnVal != LT_OK) {
    Serial.print("  Tropic01.secureSessionStart() failed, returnVal=");
    Serial.println(returnVal);
    errorHandler();
  }
  Serial.println("  OK");

  Serial.println(
      "---------------------------------------------------------------");

  Serial.println(
      "==================== Step 1: PIN Setup ========================");
  Serial.println();

  printHex("Master Secret", masterSecret, 32);
  Serial.println();

  Serial.print("PIN: ");
  for (int i = 0; i < sizeof(pin); i++) {
    Serial.print(pin[i]);
    if (i < sizeof(pin) - 1)
      Serial.print(", ");
  }
  Serial.println();
  Serial.println();

  // Set up PIN.
  uint8_t finalKeySetup[32];
  returnVal = pinSetup(masterSecret, pin, sizeof(pin), finalKeySetup);
  if (returnVal != LT_OK) {
    Serial.print("PIN setup failed, returnVal=");
    Serial.println(returnVal);
    errorHandler();
  }
  printHex("Final Key", finalKeySetup, 32);

  Serial.println();
  Serial.println(
      "=========== Step 2: Testing Wrong PIN Attempts ================");
  Serial.println();

  uint8_t wrongPin[4] = {9, 9, 9, 9};
  Serial.print("Wrong PIN: ");
  for (int i = 0; i < sizeof(wrongPin); i++) {
    Serial.print(wrongPin[i]);
    if (i < sizeof(wrongPin) - 1)
      Serial.print(", ");
  }
  Serial.println();
  Serial.println();

  // Try wrong PIN several times.
  int wrongAttempts = MACANDD_ROUNDS - 1;
  Serial.print("Attempting ");
  Serial.print(wrongAttempts);
  Serial.println(" wrong PIN entries...");
  Serial.println();

  for (int i = 0; i < wrongAttempts; i++) {
    Serial.print("Attempt ");
    Serial.print(i + 1);
    Serial.print("/");
    Serial.println(wrongAttempts);

    uint8_t finalKeyWrong[32];
    returnVal = pinVerify(wrongPin, sizeof(wrongPin), finalKeyWrong);
    if (returnVal == LT_OK) {
      Serial.println("ERROR: Wrong PIN was accepted!");
      errorHandler();
    }
    Serial.println();
  }

  Serial.println(
      "========== Step 3: Correct PIN Verification ===================");
  Serial.println();

  Serial.print("Correct PIN: ");
  for (int i = 0; i < sizeof(pin); i++) {
    Serial.print(pin[i]);
    if (i < sizeof(pin) - 1)
      Serial.print(", ");
  }
  Serial.println();
  Serial.println();

  uint8_t finalKeyVerify[32];
  returnVal = pinVerify(pin, sizeof(pin), finalKeyVerify);
  if (returnVal != LT_OK) {
    Serial.print("PIN verification failed, returnVal=");
    Serial.println(returnVal);
    errorHandler();
  }
  printHex("Final Key", finalKeyVerify, 32);
  Serial.println();

  // Verify keys match.
  Serial.println("Comparing final keys...");
  if (memcmp(finalKeySetup, finalKeyVerify, 32) == 0) {
    Serial.println("  Final keys MATCH - PIN verification successful!");
  } else {
    Serial.println("  Final keys DO NOT MATCH - Error!");
    errorHandler();
  }
}
// -----------------------------------------------------------------------------------------------------

// ------------------------------------------ Loop function --------------------------------------------
void loop() {
  // Everything is done in setup, so just do nothing in loop.
  delay(1000);
}
// -----------------------------------------------------------------------------------------------------
