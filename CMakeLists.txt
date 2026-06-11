# --------------------------- IMPORTANT ----------------------------
# This file is not used to build anything, it just gets HAL and CAL
# file paths and hand them to the extra_build_script.py.
# 
# This file expects LT_CAL CMake variable.
# ------------------------------------------------------------------

cmake_minimum_required(VERSION 3.21.0)
project(HALCALFilesExtractor NONE)

# Function to convert a CMake list into a JSON array string
function(list_to_json_array list_var out_var)
    set(json "[")
    list(LENGTH ${list_var} list_len)
    math(EXPR last_index "${list_len} - 1")

    foreach(index RANGE ${last_index})
        list(GET ${list_var} ${index} item)
        string(APPEND json "\"${item}\"")
        if(NOT index EQUAL last_index)
            string(APPEND json ",")
        endif()
    endforeach()

    string(APPEND json "]")
    set(${out_var} "${json}" PARENT_SCOPE)
endfunction()


# Add the CAL directory
add_subdirectory("libtropic/cal/${LT_CAL}")

# Add the HAL directory.
# Select the HAL implementation based on the target platform. LT_USE_RPI_PICO /
# LT_PLATFORM are forwarded from PlatformIO (extra_build_script.py) as CMake
# cache vars. Without this the Raspberry Pi Pico HAL (hal/rpi-pico) is never
# picked up even when the project defines LT_USE_RPI_PICO, so its include dir is
# missing from the generated JSON and the project build fails with:
#   fatal error: libtropic_port_rpi_pico.h: No such file or directory
add_subdirectory("libtropic/hal/rpi-pico")

add_subdirectory("libtropic/hal/arduino")


# Convert lists from the CMake variables into JSON lists
list_to_json_array(LT_CAL_SRCS LT_CAL_SRCS_JSON)
list_to_json_array(LT_CAL_INC_DIRS LT_CAL_INC_DIRS_JSON)
list_to_json_array(LT_HAL_SRCS LT_HAL_SRCS_JSON)
list_to_json_array(LT_HAL_INC_DIRS LT_HAL_INC_DIRS_JSON)

# Write JSON file
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/hal_cal_vars.json"
    "{\n"
    "  \"LT_CAL_SRCS\": ${LT_CAL_SRCS_JSON},\n"
    "  \"LT_CAL_INC_DIRS\": ${LT_CAL_INC_DIRS_JSON},\n"
    "  \"LT_HAL_SRCS\": ${LT_HAL_SRCS_JSON},\n"
    "  \"LT_HAL_INC_DIRS\": ${LT_HAL_INC_DIRS_JSON}\n"
    "}\n"
)