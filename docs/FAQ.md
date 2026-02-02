# FAQ
This list might help you resolve some issues. We also have an [FAQ with common problems](https://tropicsquare.github.io/libtropic/latest/faq) in the Libtropic SDK documentation.

- [FAQ](#faq)
  - [I enabled logging via `LT_LOG_LVL` CMake option, but no output is visible](#i-enabled-logging-via-lt_log_lvl-cmake-option-but-no-output-is-visible)
  - [`Tropic01.begin()` fails](#tropic01begin-fails)
  - [`Tropic01.secureSessionStart()` fails](#tropic01securesessionstart-fails)

None of the above helped?
- If you are a customer, contact Tropic Square via the [Support Portal](http://support.tropicsquare.com) or contact your business partners.
- Otherwise, [open an issue](https://github.com/tropicsquare/libtropic-arduino/issues/new/choose).

## I enabled logging via `LT_LOG_LVL` CMake option, but no output is visible
The Arduino HAL uses the `Serial` class for logging, so `Serial.begin(your_baudrate)` has to be called in your `setup()` function.

## `Tropic01.begin()` fails
If this method fails, do the following (if you are running our examples, steps 1 and 2 are already done - just read the log from serial monitor):
1. Save its return value (has a type `lt_ret_t`) to a variable (e.g. called `retVal`).
2. Call `Serial.println(lt_ret_verbose(retVal))` - this will print an error string.
3. If you see `LT_L1_CHIP_ALARM_MODE`, make sure that you called `SPI.begin()` in your `setup()` function. Note that the `SPI` instance has to be the same as the one you passed to the `Tropic01()` constructor. By default, the `Tropic01()` constructor uses the default `SPI` instance after including `<SPI.h>`.
4. If you see some other error string, check out our [FAQ with common problems](https://tropicsquare.github.io/libtropic/latest/faq) before reaching out to us.

## `Tropic01.secureSessionStart()` fails
If this method fails, do the following (if you are running our examples, steps 1 and 2 are already done - just read the log from serial monitor):
1. Save its return value (has a type `lt_ret_t`) to a variable (e.g. called `retVal`).
2. Call `Serial.println(lt_ret_verbose(retVal))` - this will print an error string.
3. If you see `LT_CRYPTO_ERR`, you might be using a wrong pair of Pairing Keys:
    - If you are running our examples, which use production Pairing Keys by default, navigate to the top of the file and search for this part:
    ```cpp
    // Pairing Key macros for establishing a Secure Channel Session with TROPIC01.
    // Using the default Pairing Key slot 0 of Production TROPIC01 chips.
    #define PAIRING_KEY_PRIV sh0priv_prod0
    #define PAIRING_KEY_PUB sh0pub_prod0
    #define PAIRING_KEY_SLOT TR01_PAIRING_KEY_SLOT_INDEX_0
    ```
    To use engineering sample Pairing Keys, change the values of `PAIRING_KEY_PRIV` and `PAIRING_KEY_PUB` in the following way:
    ```cpp
    #define PAIRING_KEY_PRIV sh0priv_eng_sample
    #define PAIRING_KEY_PUB sh0pub_eng_sample
    #define PAIRING_KEY_SLOT TR01_PAIRING_KEY_SLOT_INDEX_0
    ```
    If you are using custom Pairing Keys, create two arrays for them and adjust these macros accordingly. 
    
    The mentioned macros are directly passed to `Tropic01.secureSessionStart()`, so if you are not running our examples, you will have to adjust the passed arguments directly.
4. If you see some other error string, check out our [FAQ with common problems](https://tropicsquare.github.io/libtropic/latest/faq) before reaching out to us.

> [!TIP]
> Refer to [Default Pairing Keys for a Secure Channel Handshake](https://tropicsquare.github.io/libtropic/latest/reference/default_pairing_keys/) in the Libtropic documentation for more information about the Pairing Keys. However, note that instructions from the section [Establishing Your First Secure Channel Session](https://tropicsquare.github.io/libtropic/latest/reference/default_pairing_keys/#establishing-your-first-secure-channel-session) are not compatible with the approach in this repository.