/**
 * @file identify_chip.ino
 * @brief TROPIC01 Identify Chip example using the C++ wrapper.
 * @copyright Copyright (c) 2020-2025 Tropic Square s.r.o.
 *
 * @license For the license see file LICENSE.txt file in the root directory of this source tree.
 */

/***************************************************************************
 *
 * Identify Chip TROPIC01 Example
 *
 * This example shows how to:
 * 1. Read and print the CHIP_ID of TROPIC01.
 * 2. Read the RISC-V and SPECT firmware versions.
 * 3. Read the RISC-V bootloader version.
 * 4. Print the headers of the firmware banks.
 *
 * None of these methods requires a Secure Channel Session.
 *
 * Note that getBootloaderFWVersion() and printFWHeaders() temporarily
 * reboot TROPIC01 into maintenance mode, which ends an active Secure
 * Channel Session. Start a new one with secureSessionStart() if needed.
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

// CHIP_ID read from TROPIC01.
lt_chip_id_t chipId;
// Buffers for the firmware versions read from TROPIC01.
uint8_t riscvFWVersion[TR01_L2_GET_INFO_RISCV_FW_SIZE];
uint8_t spectFWVersion[TR01_L2_GET_INFO_SPECT_FW_SIZE];
uint8_t bootloaderFWVersion[TR01_L2_GET_INFO_RISCV_FW_SIZE];
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

// Helper function to print a firmware version in the format major.minor.patch (.build).
// The version bytes are stored from the least significant (build) to the most significant (major).
static void printFWVersion(const char label[], const uint8_t fwVersion[])
{
    Serial.print(label);
    Serial.print(fwVersion[3], HEX);
    Serial.print(".");
    Serial.print(fwVersion[2], HEX);
    Serial.print(".");
    Serial.print(fwVersion[1], HEX);
    Serial.print(" (.");
    Serial.print(fwVersion[0], HEX);
    Serial.println(")");
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
    Serial.println("=============== TROPIC01 Identify Chip Example ================");
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

    Serial.println("---------------------------------------------------------------");
    Serial.println();
    Serial.println("---------------------------- Loop -----------------------------");
}
// ----------------------------------------------------------------------------------------------------

// ------------------------------------------ Loop function -------------------------------------------
void loop()
{
    // Read and print CHIP_ID.
    Serial.println("Reading CHIP_ID...");
    returnVal = tropic01.getChipID(chipId);
    if (returnVal != LT_OK) {
        printLibtropicError("  Tropic01.getChipID() failed, returnVal=", returnVal);
        cleanResourcesAndLoopForever();
    }
    Serial.println("  OK");

    Serial.println("CHIP_ID data:");
    returnVal = tropic01.printChipID(chipId);
    if (returnVal != LT_OK) {
        printLibtropicError("  Tropic01.printChipID() failed, returnVal=", returnVal);
        cleanResourcesAndLoopForever();
    }
    Serial.println();

    // Read firmware versions (TROPIC01 has to be in application mode, the method handles it).
    Serial.println("Reading RISC-V FW version...");
    returnVal = tropic01.getRiscvFWVersion(riscvFWVersion);
    if (returnVal != LT_OK) {
        printLibtropicError("  Tropic01.getRiscvFWVersion() failed, returnVal=", returnVal);
        cleanResourcesAndLoopForever();
    }
    printFWVersion("  RISC-V FW version: ", riscvFWVersion);

    Serial.println("Reading SPECT FW version...");
    returnVal = tropic01.getSpectFWVersion(spectFWVersion);
    if (returnVal != LT_OK) {
        printLibtropicError("  Tropic01.getSpectFWVersion() failed, returnVal=", returnVal);
        cleanResourcesAndLoopForever();
    }
    printFWVersion("  SPECT FW version: ", spectFWVersion);
    Serial.println();

    // Read bootloader version (TROPIC01 has to be in maintenance mode, the method handles it).
    Serial.println("Reading RISC-V bootloader version...");
    returnVal = tropic01.getBootloaderFWVersion(bootloaderFWVersion);
    if (returnVal != LT_OK) {
        printLibtropicError("  Tropic01.getBootloaderFWVersion() failed, returnVal=", returnVal);
        cleanResourcesAndLoopForever();
    }
    // The highest bit of the major version is set to mark the bootloader version, so mask it out.
    bootloaderFWVersion[3] &= 0x7F;
    printFWVersion("  RISC-V bootloader version: ", bootloaderFWVersion);
    Serial.println();

    // Print firmware bank headers (TROPIC01 has to be in maintenance mode, the method handles it).
    Serial.println("Firmware bank headers:");
    returnVal = tropic01.printFWHeaders();
    if (returnVal != LT_OK) {
        printLibtropicError("  Tropic01.printFWHeaders() failed, returnVal=", returnVal);
        cleanResourcesAndLoopForever();
    }

    Serial.println();
    Serial.println("Success, entering an idle loop.");
    Serial.println("---------------------------------------------------------------");
    cleanResourcesAndLoopForever();
}
// ----------------------------------------------------------------------------------------------------
