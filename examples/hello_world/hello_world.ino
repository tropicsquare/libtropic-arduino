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
// Libtropic library.
#include <LibtropicArduino.h>
// MbedTLS's PSA Crypto library.
#include "psa/crypto.h"

// Platform's pin number where TROPIC01's Chip Select (CS) is connected to.
#define TROPIC01_CS_PIN 5

// Pairing Key macros for establishing a Secure Channel Session with TROPIC01.
// Using the default Pairing Key slot 0 of Production TROPIC01 chips.
#define PAIRING_KEY_PRIV sh0priv_prod0
#define PAIRING_KEY_PUB sh0pub_prod0
#define PAIRING_KEY_SLOT TR01_PAIRING_KEY_SLOT_INDEX_0

// TROPIC01 related variables.
Tropic01 tropic01;                                // TROPIC01 instance.
lt_ret_t returnVal;                               // Used for return values of Tropic01's methods.
uint8_t pingMsgToSend[] = "Hello World!";         // Ping message we will send to TROPIC01 via the Secure Channel.
uint8_t pingMsgToReceive[sizeof(pingMsgToSend)];  // Buffer for receiving the Ping message from TROPIC01.

// Used when initializing MbedTLS's PSA Crypto.
psa_status_t mbedtlsInitStatus;

// Used when some error occurs.
void errorHandler(void)
{
    Serial.println("Starting cleanup...");
    tropic01.end();             // Aborts all communication with TROPIC01 and frees resources.
    mbedtls_psa_crypto_free();  // Frees MbedTLS's PSA Crypto resources.

    Serial.println("Cleanup finished, entering infinite loop...");
    while (true);
}

void setup()
{
    Serial.begin(115200);

    // Init MbedTLS's PSA Crypto.
    psa_status_t mbedtlsInitStatus = psa_crypto_init();
    if (mbedtlsInitStatus != PSA_SUCCESS) {
        Serial.print("MbedTLS's PSA Crypto initialization failed, psa_status_t=");
        Serial.println(mbedtlsInitStatus);
        errorHandler();
    }

    // Init Tropic01 instance.
    returnVal = tropic01.begin(TROPIC01_CS_PIN);
    if (returnVal != LT_OK) {
        Serial.print("Tropic01.begin() failed, returnVal=");
        Serial.println(returnVal);
        errorHandler();
    }

    // Start Secure Channel Session with TROPIC01.
    returnVal = tropic01.secureSessionStart(PAIRING_KEY_PRIV, PAIRING_KEY_PUB, PAIRING_KEY_SLOT);
    if (returnVal != LT_OK) {
        Serial.print("Tropic01.secureSessionStart() failed, returnVal=");
        Serial.println(returnVal);
        errorHandler();
    }
}

void loop()
{
    // Print our Ping message we want to send.
    Serial.print("Sending the following Ping message to TROPIC01: \"");
    Serial.print((char*)pingMsgToSend);
    Serial.println("\"");

    // Ping TROPIC01 with our message.
    returnVal = tropic01.ping(pingMsgToSend, pingMsgToReceive, sizeof(pingMsgToSend));
    if (returnVal != LT_OK) {
        Serial.print("Tropic01.ping() failed, returnVal=");
        Serial.println(returnVal);
        errorHandler();
    }

    // Print received Ping message from TROPIC01.
    Serial.print("Ping message received from TROPIC01: \"");
    Serial.print((char*)pingMsgToReceive);
    Serial.println("\"");
    Serial.println();

    // Wait some time before the next Ping.
    delay(2000);
}