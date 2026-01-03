# FAQ
This list might help you resolve some issues. We also have an [FAQ with common problems](https://tropicsquare.github.io/libtropic/latest/faq) in the Libtropic SDK documentation.

- [FAQ](#faq)
  - [I enabled logging via `LT_LOG_LVL` CMake option, but no output is visible](#i-enabled-logging-via-lt_log_lvl-cmake-option-but-no-output-is-visible)
  - [`Tropic01.begin()` fails](#tropic01begin-fails)

None of the above helped?
- If you are a customer, contact Tropic Square via the [Support Portal](http://support.tropicsquare.com) or contact your business partners.
- Otherwise, [open an issue](https://github.com/tropicsquare/libtropic-arduino/issues/new/choose).

## I enabled logging via `LT_LOG_LVL` CMake option, but no output is visible
The Arduino HAL uses the `Serial` class for logging, so `Serial.begin(your_baudrate)` has to be called in your `setup()` function.

## `Tropic01.begin()` fails
If this method fails, do the following:
1. Save its return value (has a type `lt_ret_t`) to a variable (e.g. called `retVal`).
2. Call `Serial.println(lt_ret_verbose(retVal))` - this will print an error string.
3. If you see `LT_L1_CHIP_ALARM_MODE`, make sure that you called `SPI.begin()` in your `setup()` function. Note that the `SPI` instance has to be the same as the one you passed to the `Tropic01()` constructor. By default, the `Tropic01()` constructor uses the default `SPI` instance after including `<SPI.h>`.
4. If you see some other error string, check out our [FAQ with common problems](https://tropicsquare.github.io/libtropic/latest/faq) before reaching out to us.