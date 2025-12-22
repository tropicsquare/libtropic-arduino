# libtropic-arduino
This library provides support for using the TROPIC01 secure element in the Arduino framework. It's built on top of our [Libtropic SDK](https://github.com/tropicsquare/libtropic) and **currently works with PlatformIO only**!

**Tested Boards:**
* Espressif ESP32-DevKitC V4

**Current API:**
* `secureSessionStart`
* `secureSessionEnd`
* `ping`
* `rMemWrite`
* `rMemRead`
* `rMemErase`

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


## Repository structure
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