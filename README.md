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
> If you are using other boards, you will have to handle the dependency on MbedTLS yourself.

## Repository structure
- `examples/`: Basic examples, showing how to use the Arduino C++ wrapper for Libtropic.
- `libtropic/`: Libtropic SDK as a submodule.
- `src/`: Implementation of the Arduino C++ wrapper for Libtropic.
- `extra_build_script.py`: Python script to help PlatformIO build Libtropic using CMake.
- `CMakeLists.txt`: Passes HAL and CAL paths to `extra_build_script.py`.

## Using LibtropicArduino Inside PlatformIO
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
    2. Initialize MbedTLS's PSA Crypto library:
        ```cpp
        void setup() {
            psa_status_t mbedtlsInitStatus = psa_crypto_init();
            if (mbedtlsInitStatus != PSA_SUCCESS) {
                // Your error handling.
            }
        }
        ```
    3. Implement the rest of your program. Refer to this repository's `examples/` folder for examples on how to use the LibtropicArduino library to communicate with TROPIC01.
    4. At the end of the program, free MbedTLS's PSA Crypto resources:
        ```cpp
        mbedtls_psa_crypto_free();
        ```
        This step is recommended, but could be ommitted if you know what you are doing.

## Using Libtropic directly Inside PlatformIO
It is also possible to not use our C++ wrapper and use the C Libtropic API directly. To do that, repeat the steps 1, 2, 3 and 4 from [Using LibtropicArduino Inside PlatformIO](#using-libtropicarduino-inside-platformio) and then do the following:
1. In your source file (e.g. `src/main.cpp`), do the following:
    1. Include the needed headers:
        ```cpp
        #include "libtropic.h"
        #include "libtropic_common.h"
        #include "psa/crypto.h"  // MbedTLS's PSA Crypto library.
        ```
    2. Initialize MbedTLS's PSA Crypto library:
        ```cpp
        void setup() {
            psa_status_t mbedtlsInitStatus = psa_crypto_init();
            if (mbedtlsInitStatus != PSA_SUCCESS) {
                // Your error handling.
            }
        }
        ```
    3. Implement the rest of your program. Refer to [Examples](https://tropicsquare.github.io/libtropic/latest/get_started/examples/) in the [Libtropic documentation](https://tropicsquare.github.io/libtropic/latest/) for examples on how to use Libtropic to communicate with TROPIC01.
    4. At the end of the program, free MbedTLS's PSA Crypto resources:
        ```cpp
        mbedtls_psa_crypto_free();
        ```
        This step is recommended, but could be ommitted if you know what you are doing.

## Supported Libtropic Build Flags
Except [LT_BUILD_TESTS](https://tropicsquare.github.io/libtropic/latest/get_started/integrating_libtropic/how_to_configure/#lt_build_tests), all Libtropic's [Available CMake Options](https://tropicsquare.github.io/libtropic/latest/get_started/integrating_libtropic/how_to_configure/#available-cmake-options) are supported.

## Contributing
Contributors, please follow the [contribution guidelines](CONTRIBUTING.md).

## License
See the [LICENSE.md](LICENSE.md) file in the root of this repository or consult license information at [Tropic Square website](http:/tropicsquare.com/license).