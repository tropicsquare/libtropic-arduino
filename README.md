# libtropic-arduino
This repository implements the LibtropicArduino library, which serves as an example of how to use our official SDK, [Libtropic](https://github.com/tropicsquare/libtropic), on Arduino-based platforms.

The LibtropicArduino library is implemented in a way to be compliant with the requirements of the **Arduino Library Manager** and **PlatformIO Registry**, but was not published in neither of these yet.

> [!WARNING]
> The LibtropicArduino library was tested using PlatformIO only!

> [!WARNING]
> The LibtropicArduino library depends on MbedTLS with PSA Crypto library.
>
> If you are using ESP32 boards, PlatformIO will most likely use the [ESP32 Arduino core](https://github.com/espressif/arduino-esp32), which includes MbedTLS with PSA Crypto library by default, so the dependency does not have to be handled manually.
>
> If you are using other boards, you will have to handle the MbedTLS dependency yourself.

## Repository structure
- `examples/`: Basic examples, showing how to use the Arduino C++ wrapper for Libtropic.
- `libtropic/`: Libtropic SDK as a submodule.
- `src/`: Implementation of the Arduino C++ wrapper for Libtropic.
- `extra_build_script.py`: Python script to help PlatformIO build Libtropic using CMake.
- `CMakeLists.txt`: Passes HAL and CAL paths to `extra_build_script.py`.

## Using LibtropicArduino Inside PlatformIO
> [!IMPORTANT]
> We provide a C++ wrapper for Libtropic for convenience. However, the implementation of this wrapper is not complete – this is your chance to try to implement some of the missing functionality and contribute to the library! If you would like to contribute, read the [contributing guidelines](./CONTRIBUTING.md) and send us a Pull Request!

1. Create a new PlatformIO project.
2. Select your board and the Arduino framework.
3. Inside the project's `platformio.ini`, add a dependency on this repository to the target for your board:
    ```ini
    lib_deps = 
        https://github.com/tropicsquare/libtropic-arduino.git
    ```
4. You can add additional flags for the libtropic build (see [Supported Libtropic Build Flags](#supported-libtropic-build-flags)) to the `build_flags` variable - for example, configure the logging:
    ```ini
    build_flags =
        -DLT_LOG_LVL=Debug  ; Sets Libtropic's logging verbosity to debug
    ```
5. In your source file (e.g. `src/main.cpp`), do the following:
    1. Include the needed headers:
        ```cpp
        #include <LibtropicArduino.h>
        #include "psa/crypto.h"  // MbedTLS's PSA Crypto library.
        ```
    2. Declare a global instance of `Tropic01` by calling its constructor (see `examples/` or the tiny example below). The number of parameters depends on the [LT_USE_INT_PIN](https://tropicsquare.github.io/libtropic/latest/get_started/integrating_libtropic/how_to_configure/#lt_use_int_pin) and [LT_SEPARATE_L3_BUFF](https://tropicsquare.github.io/libtropic/latest/get_started/integrating_libtropic/how_to_configure/#lt_separate_l3_buff) CMake options. These are the parameters of `Tropic01` constructor:
        - `spi_cs_pin`: GPIO pin where TROPIC01's CS pin is connected.
        - `int_gpio_pin` (exists only if [LT_USE_INT_PIN](https://tropicsquare.github.io/libtropic/latest/get_started/integrating_libtropic/how_to_configure/#lt_use_int_pin)=1): GPIO pin where TROPIC01's interrupt pin is connected.
        - `l3_buff` (exists only if [LT_SEPARATE_L3_BUFF](https://tropicsquare.github.io/libtropic/latest/get_started/integrating_libtropic/how_to_configure/#lt_separate_l3_buff)=1): User-defined L3 buffer.
        - `l3_buff_len` (exists only if [LT_SEPARATE_L3_BUFF](https://tropicsquare.github.io/libtropic/latest/get_started/integrating_libtropic/how_to_configure/#lt_separate_l3_buff)=1): Length of `lf_buff`.
        - `rng_seed`: Seed for the PRNG (defaults to random()).
        - `spi`: Instance of SPIClass to use, defaults to the default SPI instance set by `<SPI.h>`.
        - `spi_settings`: SPI settings, defaults to tested values. If you want to change them, keep `SPISettings.dataOrder=MSBFIRST` and `SPISettings.dataMode=SPI_MODE0` (required by TROPIC01).

        For the purpose of the tiny example below, consider:
        1. [LT_USE_INT_PIN](https://tropicsquare.github.io/libtropic/latest/get_started/integrating_libtropic/how_to_configure/#lt_use_int_pin)=0,
        2. [LT_SEPARATE_L3_BUFF](https://tropicsquare.github.io/libtropic/latest/get_started/integrating_libtropic/how_to_configure/#lt_separate_l3_buff)=0,
        3. using default values for the `Tropic01()` parameters that have a default value,
        4. `TROPIC01_CS_PIN` is a pin on your board where TROPIC01's Chip Select pin is connected.
        
        The global instance of `Tropic01` would then be declared as:
        ```cpp
        Tropic01 tropic01(TROPIC01_CS_PIN);
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
        3. Initialize `tropic01` using its `begin()` method:
            ```cpp
            lt_ret_t ret = tropic01.begin();
            if (ret != LT_OK) {
                // Your error handling.
            }
            ```
    4. Implement the rest of your program. Refer to this repository's `examples/` folder for examples on how to use the LibtropicArduino library to communicate with TROPIC01.
    5. At the end of the program, do the following:
       1. Deinitialize `tropic01` using its `end()` method:
            ```cpp
            lt_ret_t ret = tropic01.end();
            if (ret != LT_OK) {
                // Your error handling.
            }
            ```
       2. Free MbedTLS's PSA Crypto resources (could be ommitted if you know what you are doing):
            ```cpp
            mbedtls_psa_crypto_free();
            ```

## Using Libtropic directly Inside PlatformIO (Advanced)
It is also possible to use the C Libtropic API directly. However, this requires more advanced approach compared to using the C++ wrapper. But the entire Libtropic API will be available to you.

Repeat the steps 1, 2, 3, 4 from [Using LibtropicArduino Inside PlatformIO](#using-libtropicarduino-inside-platformio) and then do the following:
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

## FAQ
If you encounter any issues, please check the [FAQ](./FAQ.md) before filing an issue or reaching out to our [support](https://support.desk.tropicsquare.com/).

## Contributing
Contributors, please follow the [contribution guidelines](CONTRIBUTING.md).

## License
See the [LICENSE.md](LICENSE.md) file in the root of this repository or consult license information at [Tropic Square website](http:/tropicsquare.com/license).