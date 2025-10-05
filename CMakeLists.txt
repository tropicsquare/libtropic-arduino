cmake_minimum_required(VERSION 3.21.0)

project(libtropic_arduino
        VERSION 1.0.0
        DESCRIPTION "Port of libtropic SDK to Arduino framework"
        LANGUAGES CXX)

# Add other options you want to control from PlatformIO later
set(LT_CRYPTO "trezor_crypto")
set(LT_LOG_LVL "Debug")

# Execute the main libtropic CMakeLists.txt to create the "tropic" target.
# The source directory now points to the submodule.
add_subdirectory(
    third-party/libtropic
    ${CMAKE_BINARY_DIR}/libtropic-build
)

# If your HAL port uses C++, ensure the linker uses the C++ compiler
set_target_properties(tropic PROPERTIES LINKER_LANGUAGE CXX)