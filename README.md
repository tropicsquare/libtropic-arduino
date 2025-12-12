# libtropic-arduino
This library provides support for using the TROPIC01 secure element in the Arduino framework. It's built on top of our [Libtropic SDK](https://github.com/tropicsquare/libtropic) and **currently works with PlatformIO only**!

**Tested Boards:**
* Espressif ESP32-DevKitC V4

**Supported API calls:**
* Ping

## Using LibtropicArduino Inside PlatformIO

1. Create a new PlatformIO project.
2. Select supported board, as a framework choose "Arduino framework"
3. Update `platformio.ini` with link to libtropic-arduino and set also serial port for uploading and communications:

    ```
    [env:esp32dev]
    platform = espressif32
    board = esp32dev
    framework = arduino

    lib_deps =
        https://github.com/tropicsquare/libtropic-arduino.git
    # (Update port based on how your board enumerated within your system)
    upload_port = /dev/ttyUSB0
    ```


4. Now add example's code to your project: copy the whole content of `example/hello_world.ino` from this repository and paste it into `main.cpp` of **your project**
5. Build project by clicking on `PlatformIO: Build` in a bottom menu, you should see similar output in terminal:
    ```
    ...
    RAM:   [=         ]   8.7% (used 28572 bytes from 327680 bytes)
    Flash: [===       ]  29.2% (used 382757 bytes from 1310720 bytes)
    ==================== [SUCCESS] Took 1.13 seconds ======================
    ````
6. Upload code into your board by clicking on `PlatformIO: Upload` in a bottom menu, you should see similar output in terminal:
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

7. Now open your `Serial Monitor` and reset your board with `RESET BUTTON` next to USB connector, you should see that TROPC01 talks to you over encrypted secure channel!

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
> As of now we provide a C++ wrapper for Libtropic for convenience. However, the implementation of this wrapper is not feature complete – this is your chance to try to implement some of the missing functionality and contribute to the library! If you would like to contribute, read the [contributing guidelines](./CONTRIBUTING.md) and send us a Pull Request!

For more info check out [docs](docs/index.md) folder.

## FAQ
If you encounter any issues, please check the [FAQ](docs/FAQ.md) before filing an issue or reaching out to our [support](https://support.desk.tropicsquare.com/).

## License
See the [LICENSE.md](LICENSE.md) file in the root of this repository or consult license information at [Tropic Square website](http:/tropicsquare.com/license).