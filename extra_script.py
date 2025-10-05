import os
import subprocess
from SCons.Script import DefaultEnvironment

# We need the global env for this script to work correctly
Import("env")

# --- Path Configuration ---
# This script's directory is the root of the libtropic-arduino library
this_lib_dir = env.Dir('.').abspath

# The libtropic SDK is in a predictable path as a submodule
libtropic_src_dir = os.path.join(this_lib_dir, "third-party", "libtropic")

if not os.path.isdir(libtropic_src_dir):
    env.Exit(f"Error: Could not find 'libtropic' submodule at {libtropic_src_dir}")

# --- Helper for getting flags as strings ---
def get_cmake_flag(env_var_name):
    value = env.get(env_var_name)
    if not value: return ""
    # Correctly handle list of flags
    if isinstance(value, list):
        # Flatten list of lists if necessary
        flat_list = []
        for item in value:
            if isinstance(item, list):
                flat_list.extend(item)
            else:
                flat_list.append(item)
        return " ".join(env.subst(v) for v in flat_list)
    return env.subst(str(value))

# --- Build Configuration ---
pio_env_name = env.get("PIOENV")
# Use a build directory within the library to avoid project-level conflicts
build_dir = os.path.join(this_lib_dir, ".pio", "build", pio_env_name)
lib_path = os.path.join(build_dir, "libtropic-build", "libtropic.a")

os.makedirs(build_dir, exist_ok=True)

# --- CMake Configuration ---
cmake_args = [
    f"-DCMAKE_C_COMPILER={get_cmake_flag('CC')}",
    f"-DCMAKE_CXX_COMPILER={get_cmake_flag('CXX')}",
    f"-DCMAKE_C_FLAGS='{get_cmake_flag('CFLAGS')} {get_cmake_flag('CCFLAGS')}'",
    f"-DCMAKE_CXX_FLAGS='{get_cmake_flag('CXXFLAGS')}'",
    "-DCMAKE_SYSTEM_NAME=Generic",
    "-DCMAKE_TRY_COMPILE_TARGET_TYPE=STATIC_LIBRARY",
]

# Pass project build flags (e.g., -D defines) to CMake
custom_opts = env.GetProjectOption("build_flags", [])
for opt in custom_opts:
    if isinstance(opt, str) and opt.startswith("-D"):
        cmake_args.append(opt)

# --- Environment for subprocess ---
custom_env = os.environ.copy()
custom_env["PATH"] = env["ENV"]["PATH"]

# --- Run CMake to build the core static library ---
# Re-configure only if the static library doesn't exist to speed up builds
if not os.path.exists(lib_path):
    print("Configuring and building libtropic static library...")
    configure_cmd = ["cmake", "-S", this_lib_dir, "-B", build_dir] + cmake_args
    # REMOVE capture_output=True from here
    subprocess.run(configure_cmd, check=True, env=custom_env) 

    build_cmd = ["cmake", "--build", build_dir]
    # AND REMOVE it from here
    subprocess.run(build_cmd, check=True, env=custom_env)
    print("libtropic.a built successfully.")

# --- Link the core library and set include paths ---
# This part runs on every build

# Add path to the generated libtropic.a
env.Append(LIBPATH=[os.path.join(build_dir, "libtropic-build")])

# Link libtropic.a
env.Append(LIBS=["tropic"])

# Add include paths needed by the HAL files and the user's sketch
env.Append(CPPPATH=[
    os.path.join(libtropic_src_dir, "include"),
    os.path.join(libtropic_src_dir, "hal", "include"),
    # The HAL source files also need this path to find their own header
    os.path.join(libtropic_src_dir, "hal", "port", "arduino")
])

print("Successfully configured libtropic for PlatformIO.")