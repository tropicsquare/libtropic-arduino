# libtropic-arduino
This library provides support for using the TROPIC01 secure element in the Arduino framework. It's built on top of our [Libtropic SDK](https://github.com/tropicsquare/libtropic) and **currently works with PlatformIO only**!

**Tested Boards:**
* Espressif ESP32-DevKitC V4
* Raspberry Pi Pico (RP2040) - requires [additional setup](#using-libtropicarduino-with-raspberry-pi-pico)
* Raspberry Pi Pico 2 (RP2350) - requires [additional setup](#using-libtropicarduino-with-raspberry-pi-pico)

**Current API:**
* `secureSessionStart`
* `secureSessionEnd`
* `ping`
* `eccKeyGenerate`
* `eccKeyStore`
* `eccKeyRead`
* `eccKeyErase`
* `ecdsaSign`
* `eddsaSign`
* `rMemWrite`
* `rMemRead`
* `rMemErase`
* `macAndDestroy`
* `getChipID`
* `printChipID`
* `getBootloaderFWVersion`
* `getRiscvFWVersion`
* `getSpectFWVersion`
* `printFWHeaders`
* `randomValueGet`
* `mcounterInit`
* `mcounterGet`
* `mcounterUpdate`


## Using LibtropicArduino Inside PlatformIO

1. Create a new PlatformIO project.
2. Select supported board, as a framework choose "Arduino framework".
3. Inside your project's `platformio.ini` file, add a dependency on `libtropic-arduino` and set the serial port used for uploading and communication:

    ```
    [env:esp32dev]
    platform = espressif32
    board = esp32dev
    framework = arduino

    lib_deps =
        https://github.com/tropicsquare/libtropic-arduino.git
    # (Change to the port your board is mapped to in your system)
    upload_port = /dev/ttyUSB0
    ```


4. Inside your project's `src/` folder, create `main.cpp` and copy the whole contents of `examples/hello_world/hello_world.ino` into it.
5. Build the project: click on the **PlatformIO: Build** button in the bottom menu. In your terminal, you should see a similar output to this:
    ```
    ...
    RAM:   [=         ]   8.7% (used 28572 bytes from 327680 bytes)
    Flash: [===       ]  29.2% (used 382757 bytes from 1310720 bytes)
    ==================== [SUCCESS] Took 1.13 seconds ======================
    ```
6. Upload the code: click on the **PlatformIO: Upload** button in the bottom menu. In your terminal, you should see a similar output to this:
    ```
    ...
    Writing at 0x00057433... (85 %)
    Writing at 0x0005d744... (92 %)
    Writing at 0x00067ca2... (100 %)
    Wrote 383120 bytes (229316 compressed) at 0x00010000 in 5.8 seconds (effective 532.6 kbit/s)...
    Hash of data verified.
    
    Leaving...
    Hard resetting via RTS pin...
    ```

7. Observe the output from the example: open your **Serial Monitor** and reset your board using its reset button. In the Serial Monitor, you should see that TROPIC01 talks to you over encrypted secure channel!

    ```
    ---- Opened the serial port /dev/ttyUSB0 ----
    --
    Sending the following Ping message to TROPIC01: "Hello World!"
      OK
    Ping message received from TROPIC01: "Hello World!"
    --
    ```


## Using LibtropicArduino with Raspberry Pi Pico
The default MbedTLS configuration does not build for Raspberry Pi Pico (RP2040) and Pico 2 (RP2350), because it expects an operating system (time, filesystem, networking and platform entropy). The following steps use the [Arduino-Pico core](https://github.com/earlephilhower/arduino-pico) and the [MbedTLS library for PlatformIO](https://registry.platformio.org/libraries/baracodadailyhealthtech/mbedtls).

1. Create a new PlatformIO project and use the following `platformio.ini` (use `board = rpipico2` for Pico 2):

    ```
    [env:pico]
    platform = https://github.com/maxgerhardt/platform-raspberrypi.git
    board = rpipico
    framework = arduino
    board_build.core = earlephilhower

    lib_deps =
        https://github.com/tropicsquare/libtropic-arduino.git
        baracodadailyhealthtech/mbedtls

    build_flags =
        -I include
        ; Custom MbedTLS configuration, see step 2.
        -DMBEDTLS_USER_CONFIG_FILE=\"mbedtls_user_config.h\"
        ; Prevents CMake from linking a test executable when checking the compiler.
        -DCMAKE_TRY_COMPILE_TARGET_TYPE=STATIC_LIBRARY
        ; GCC 15+ no longer zeroes whole unions on "= {0}". MbedTLS < 3.6.4 relies on this.
        ; Using this flag restores the previous behavior.
        ; Remove if using GCC 14 or older.
        -fzero-init-padding-bits=unions
    ```

2. Inside your project's `include/` folder, create `mbedtls_user_config.h`. MbedTLS includes it after its default configuration:

    ```c
    // Disable features that require an operating system.
    #undef MBEDTLS_HAVE_TIME
    #undef MBEDTLS_HAVE_TIME_DATE
    #undef MBEDTLS_FS_IO
    #undef MBEDTLS_NET_C
    #undef MBEDTLS_TIMING_C
    #undef MBEDTLS_PSA_CRYPTO_STORAGE_C
    #undef MBEDTLS_PSA_ITS_FILE_C

    // Use the entropy source from step 3 instead of the platform one.
    #define MBEDTLS_NO_PLATFORM_ENTROPY
    #define MBEDTLS_ENTROPY_HARDWARE_ALT
    ```

3. Inside your project's `src/` folder, create `mbedtls_entropy.cpp`, which provides entropy to MbedTLS from the hardware random number generator of the Pico:

    ```cpp
    #include <string.h>

    #include "mbedtls/platform_util.h"
    #include "pico/rand.h"

    extern "C" int mbedtls_hardware_poll(void *data, unsigned char *output, size_t len, size_t *olen)
    {
        (void)data;
        size_t bytes_left = len;
        unsigned char *output_ptr = output;
        uint64_t random_data;
        *olen = 0;

        while (bytes_left) {
            random_data = get_rand_64();
            size_t cpy_cnt = bytes_left < sizeof(random_data) ? bytes_left : sizeof(random_data);
            memcpy(output_ptr, &random_data, cpy_cnt);
            bytes_left -= cpy_cnt;
            output_ptr += cpy_cnt;
            *olen += cpy_cnt;
        }

        mbedtls_platform_zeroize(&random_data, sizeof(random_data));
        return 0;
    }
    ```

4. Continue with steps 4 - 7 from [Using LibtropicArduino Inside PlatformIO](#using-libtropicarduino-inside-platformio). The default `SPI` instance uses pins GP16 (MISO), GP18 (SCK) and GP19 (MOSI). Set `TROPIC01_CS_PIN` in the example according to your wiring.


## Repository structure
- `docs/`: More detailed information.
- `examples/`: Basic examples, showing how to use the Arduino C++ wrapper for Libtropic.
- `libtropic/`: Libtropic SDK as a submodule.
- `src/`: Implementation of the Arduino C++ wrapper for Libtropic.
- `extra_build_script.py`: Python script to help PlatformIO build Libtropic using CMake.
- `CMakeLists.txt`: Passes HAL and CAL paths to `extra_build_script.py`.

## Next steps?

> [!IMPORTANT]
> This library uses an Arduino-like C++ wrapper around [Libtropic](https://github.com/tropicsquare/libtropic). However, the implementation of this wrapper is not feature complete – this is your chance to try to implement some of the missing functionality and contribute to the library! If you would like to contribute, read the [contributing guidelines](./CONTRIBUTING.md) and send us a Pull Request!

For more detailed information, refer to the [docs/](docs/index.md) folder.

## FAQ
If you encounter any issues, please check the [FAQ](docs/FAQ.md) before filing an issue or reaching out to our [support](https://support.desk.tropicsquare.com/).

## License
See the [LICENSE.md](LICENSE.md) file in the root of this repository or consult license information at [Tropic Square website](http:/tropicsquare.com/license).