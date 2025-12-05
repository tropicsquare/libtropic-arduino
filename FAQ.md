# FAQ
This list might help you resolve some issues. We also have an [FAQ with common problems](https://tropicsquare.github.io/libtropic/latest/faq) in the Libtropic SDK documentation.

- [FAQ](#faq)
  - [I enabled logging via `LT_LOG_LVL` CMake option, but no output is visible](#i-enabled-logging-via-lt_log_lvl-cmake-option-but-no-output-is-visible)

None of the above helped?
- If you are a customer, contact Tropic Square via the [Support Portal](http://support.tropicsquare.com) or contact your business partners.  
- Otherwise, [open an issue](https://github.com/tropicsquare/libtropic-arduino/issues/new/choose).

## I enabled logging via `LT_LOG_LVL` CMake option, but no output is visible
The Arduino HAL uses the `Serial` class for logging, so `Serial.begin(your_baudrate)` has to be called in your `setup()` function.