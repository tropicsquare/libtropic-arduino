# Documentation

Various notes.

## Using Libtropic directly Inside PlatformIO (Advanced)
It is also possible to use the C Libtropic API directly. However, this requires more advanced approach compared to using the C++ wrapper. But the entire Libtropic API will be available to you.

Repeat the steps 1, 2, 3 from [Using LibtropicArduino Inside PlatformIO](../README.md#using-libtropicarduino-inside-platformio) and then do the following:
1. In your source file (e.g. `src/main.cpp`), do the following:
    1. Include the required headers:
        ```cpp
        #include "libtropic.h"
        #include "libtropic_common.h"
        #include "libtropic_mbedtls_v4.h"
        #include "libtropic_port_arduino.h"
        #include "psa/crypto.h"  // MbedTLS's PSA Crypto library.
        ```
    2. Declare the following global variables:
        ```cpp
        lt_dev_arduino_t my_device;
        lt_ctx_mbedtls_v4_t my_crypto_ctx;
        lt_handle_t my_handle;
        #if LT_SEPARATE_L3_BUFF
        // It is possible to define user's own buffer for L3 Layer data.
        // This is handy when using multiple instances of lt_handle_t - only one buffer for all instances will be used.
        uint8_t my_l3_buffer[LT_SIZE_OF_L3_BUFF] __attribute__((aligned(16))) = {0};
        #endif
        ```
    3. In your `setup()` function, do the following:
        1. Initialize MbedTLS's PSA Crypto library:
            ```cpp
            psa_status_t mbedtlsInitStatus = psa_crypto_init();
            if (mbedtlsInitStatus != PSA_SUCCESS) {
                // Your error handling.
            }
            ```
        2. Initialize `Serial` (needed only if you configured logging using the [LT_LOG_LVL](https://tropicsquare.github.io/libtropic/latest/get_started/integrating_libtropic/how_to_configure/) CMake option, as the Libtropic Arduino HAL uses `Serial` for logging):
            ```cpp
            Serial.begin(your_baudrate);
            ```
        3. Initialize `my_device`:
            ```cpp
            my_device.spi_cs_pin = <YOUR_VALUE>; // Platform's pin number where TROPIC01's SPI Chip Select pin is connected.
            #if LT_USE_INT_PIN
            my_device.int_gpio_pin = <YOUR_VALUE>; // Platform's pin number where TROPIC01's interrupt pin is connected. Is necessary only when -DLT_USE_INT_PIN=1 was set in build_flags.
            #endif
            my_device.spi_settings = SPISettings(10000000, MSBFIRST, SPI_MODE0); // Frequency can be changed.
            my_device.rng_seed = <YOUR_VALUE>; // Libtropic uses RNG for generating Host's ephemeral keys.
            my_device.spi = ::SPI; // Using default SPI instance, but can be changed.
            ```
        4. Initialize `my_handle`:
            ```cpp
            my_handle.l2.device = &my_device;
            my_handle.l3.crypto_ctx = &my_crypto_ctx;
            #if LT_SEPARATE_L3_BUFF
            my_handle.l3.buff = my_l3_buffer;
            my_handle.l3.buff_len = sizeof(my_l3_buffer);
            #endif
            ```
        5. Initialize Libtropic:
            ```cpp
            lt_ret_t ret_init = lt_init(&my_handle);
            if (ret_init != LT_OK) {
                // Your error handling.
            }
            ```
    4. Implement the rest of your program. Refer to [Examples](https://tropicsquare.github.io/libtropic/latest/get_started/examples/) in the [Libtropic documentation](https://tropicsquare.github.io/libtropic/latest/) for examples on how to use Libtropic to communicate with TROPIC01.
    5. At the end of the program, do the following:
        1. Deinitialize Libtropic:
            ```cpp
            lt_ret_t ret_deinit = lt_deinit(&my_handle);
            if (ret_deinit != LT_OK) {
                // Your error handling.
            }
            ```
        2. Free MbedTLS's PSA Crypto resources (could be ommitted if you know what you are doing):
            ```cpp
            mbedtls_psa_crypto_free();
            ```

## Supported Libtropic Build Flags
Except [LT_BUILD_TESTS](https://tropicsquare.github.io/libtropic/latest/get_started/integrating_libtropic/how_to_configure/#lt_build_tests), all Libtropic's [Available CMake Options](https://tropicsquare.github.io/libtropic/latest/get_started/integrating_libtropic/how_to_configure/#available-cmake-options) are supported.

> [!IMPORTANT]
> If you are using the [LT_LOG_LVL](https://tropicsquare.github.io/libtropic/latest/get_started/integrating_libtropic/how_to_configure/#lt_log_lvl) CMake option, do not forget to call `Serial.begin(your_baudrate)` in your `setup()` function, as the logging in the Arduino HAL is done via the `Serial` class.

## Contributing
Contributors, please follow the [contribution guidelines](../CONTRIBUTING.md).
