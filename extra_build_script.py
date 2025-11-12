from pathlib import Path
import subprocess
import shlex
import re
import json

Import("env", "projenv")

# Ensure project compiles C++ with C++14 (affects PlatformIO compile, not the CMake subprocess)
env.Append(CXXFLAGS=["-std=gnu++14"])
projenv.Append(CXXFLAGS=["-std=gnu++14"])

LIB_NAME_ON_DISK = "LibtropicArduino"
BUILD_TARGET = "tropic"
LIBTROPIC_DEFAULT_BUILD_FLAGS = [
    "-DLT_CAL=mbedtls_v4"
]

# 1) Find PROJECT_LIBDEPS_DIR and current PIO environment name
libdeps_root = env.get("PROJECT_LIBDEPS_DIR")
pioenv = env.subst("$PIOENV")
libdeps_root_path = Path(libdeps_root)

# Prefer the per-env libdeps folder
libdeps_env_dir = (libdeps_root_path / pioenv) if pioenv else None
if libdeps_env_dir and not libdeps_env_dir.is_dir():
    libdeps_env_dir = libdeps_root_path

# 2) Locate the installed library folder inside libdeps
library_dir = None
if libdeps_env_dir:
    candidate = libdeps_env_dir / LIB_NAME_ON_DISK
    if candidate.is_dir():
        library_dir = candidate

if library_dir is None:
    # search recursively for a directory named LIB_NAME_ON_DISK under libdeps_root
    for p in libdeps_root_path.rglob(LIB_NAME_ON_DISK):
        if p.is_dir():
            library_dir = p
            break

if library_dir is None:
    raise RuntimeError(f"Could not find installed library '{LIB_NAME_ON_DISK}' under {libdeps_root!s}")

# 3) Paths
# library_dir is the installed library root (contains CMakeLists.txt that produces hal_cal_vars.json)
external_root = library_dir / "libtropic"  # path used for building the libtropic target
hal_cal_vars_build_dir = library_dir / "hal_cal_vars_build"
hal_cal_vars_json_path = hal_cal_vars_build_dir / "hal_cal_vars.json"
libtropic_build_dir = library_dir / "libtropic_build"

# 4) Configure & build libtropic with CMake using PlatformIO toolchain
libtropic_build_dir.mkdir(parents=True, exist_ok=True)

cc = env.get("CC")
cxx = env.get("CXX")
ar = env.get("AR")

# Get the compiler flags from the PlatformIO environment (may be empty)
c_flags = " ".join(env.get("CCFLAGS", [])) or ""
# Ensure C++14 is passed to CMake's CXX flags
cxx_flags = (c_flags + " -std=gnu++14").strip()

# Base cmake args (we will extend with extra tokens after we extract LT_CAL)
cmake_args = [
    "cmake",
    "-S", str(external_root),
    "-B", str(libtropic_build_dir),
    # Add this line to pass linker flags during the initial test
    "-DCMAKE_EXE_LINKER_FLAGS=--specs=nosys.specs",
    f"-DCMAKE_C_FLAGS={c_flags}",
    f"-DCMAKE_CXX_FLAGS={cxx_flags}"
]

# pass toolchain compilers if provided by PlatformIO env
if cc:
    cmake_args.append(f"-DCMAKE_C_COMPILER={cc}")
if cxx:
    cmake_args.append(f"-DCMAKE_CXX_COMPILER={cxx}")
if ar:
    cmake_args.append(f"-DCMAKE_AR={ar}")

# Get all flags starting with -D
try:
    flags_for_libtropic = [f for f in env['BUILD_FLAGS'] if f.startswith("-D")]
except:
    flags_for_libtropic = []

# Append default libtropic flags
flags_for_libtropic.extend(LIBTROPIC_DEFAULT_BUILD_FLAGS)

# Get the LT_CAL flag
cal_flag = None
for flag in flags_for_libtropic:
    if flag.startswith("-DLT_CAL="):
        cal_flag = flag
        break

# Append the extracted flags to CMake args
cmake_args.extend(flags_for_libtropic)

# Run libtropic-arduino's CMake to generate hal_cal_vars.json
hal_cal_vars_build_dir.mkdir(parents=True, exist_ok=True)

# Important: run the generator with the library root (not external_root) so top-level CMakeLists can write the JSON
subprocess.check_call(["cmake", "-S", str(library_dir), "-B", str(hal_cal_vars_build_dir), cal_flag])

# Ensure the JSON was produced
if not hal_cal_vars_json_path.is_file():
    print("hal_cal_vars_build_dir contents:", list(hal_cal_vars_build_dir.glob("*")))
    raise FileNotFoundError(f"hal_cal_vars.json not found at {hal_cal_vars_json_path}")

# Read generated JSON with HAL/CAL vars
with hal_cal_vars_json_path.open() as f:
    halcal_vars = json.load(f)

hal_sources = halcal_vars.get("LT_HAL_SRCS", [])
hal_inc_dirs = halcal_vars.get("LT_HAL_INC_DIRS", [])
cal_sources = halcal_vars.get("LT_CAL_SRCS", [])
cal_inc_dirs = halcal_vars.get("LT_CAL_INC_DIRS", [])

# Run the full libtropic CMake configure & build
print("Running CMake configure for libtropic build:", " ".join(cmake_args))
subprocess.check_call(cmake_args)

print(f"Running CMake build (target '{BUILD_TARGET}')")
subprocess.check_call(["cmake", "--build", str(libtropic_build_dir), "--target", BUILD_TARGET])

# Find produced .a
lib_static_path = libtropic_build_dir / "libtropic.a"
if not lib_static_path.is_file():
    raise RuntimeError(f"Could not find produced static library in {libtropic_build_dir}")

# Parse flags.make for target
flags_make_path = libtropic_build_dir / "CMakeFiles" / f"{BUILD_TARGET}.dir" / "flags.make"

# Get defines used for building libtropic
if flags_make_path and flags_make_path.is_file():
    print("Using flags.make:", flags_make_path)
    with flags_make_path.open("r", encoding="utf-8", errors="ignore") as f:
        content = f.read()

    # collect both C_DEFINES and CXX_DEFINES if present
    define_flags = []
    for name in ("C_DEFINES", "CXX_DEFINES"):
        m = re.search(r'^\s*' + re.escape(name) + r'\s*=\s*(.+)$', content, flags=re.MULTILINE)
        if m:
            define_flags.append(m.group(1).strip())

    if not define_flags:
        raise RuntimeError("No C_DEFINES/CXX_DEFINES found in flags.make")
else:
    raise RuntimeError("flags.make for target not found; skipping define export")

# Append parsed defines to env and projenv so subsequent compilation sees same macros
print("Injecting build flags into the main project environment:")
for flag in define_flags:
    print(f"  -> {flag}")
env.ProcessFlags(" ".join(define_flags))
projenv.ProcessFlags(" ".join(define_flags))

# 5) Prepare include paths: libtropic includes + per-env libdeps includes
def ensure_list(x):
    if x is None:
        return []
    if isinstance(x, (list, tuple)):
        return list(x)
    return [x]

def to_abs_include_paths(root: Path, src_list):
    """Convert CMake-provided include entries to absolute paths (root-relative if needed)."""
    out = []
    for s in ensure_list(src_list):
        p = Path(s)
        if not p.is_absolute():
            p = (root / p).resolve()
        else:
            p = p.resolve()
        out.append(str(p))
    return out

# Make absolute include dirs for HAL/CAL entries (relative to library root)
hal_inc_list = to_abs_include_paths(external_root, hal_inc_dirs)
cal_inc_list = to_abs_include_paths(external_root, cal_inc_dirs)

cpppaths = []
# public headers
cpppaths.append(str((external_root / "include").resolve()))
# internal src headers (add this so CAL impls can include internal headers)
cpppaths.append(str((external_root / "src").resolve()))

# include HAL/CAL include directories (already absolute)
cpppaths.extend(hal_inc_list)
cpppaths.extend(cal_inc_list)

# Add per-env libdeps include folders (if libdeps_env_dir exists)
if libdeps_env_dir and libdeps_env_dir.is_dir():
    for libfolder in libdeps_env_dir.iterdir():
        if not libfolder.is_dir():
            continue
        inc1 = libfolder / "include"
        inc2 = libfolder / "src"
        inc3 = libfolder / "src" / "include"
        if inc1.is_dir():
            cpppaths.append(str(inc1.resolve()))
        if inc2.is_dir():
            cpppaths.append(str(inc2.resolve()))
        if inc3.is_dir():
            cpppaths.append(str(inc3.resolve()))
        cpppaths.append(str(libfolder.resolve()))

# Dedupe while preserving order, only keep existing directories
seen = set()
cpppaths_filtered = []
for p in cpppaths:
    try:
        pp = Path(p)
    except TypeError:
        print("Skipping invalid CPPPATH entry:", repr(p))
        continue
    if pp.is_dir() and str(pp) not in seen:
        seen.add(str(pp))
        cpppaths_filtered.append(str(pp))

if cpppaths_filtered:
    env.Append(CPPPATH=cpppaths_filtered)
    print("Added CPPPATH entries for libtropic and libdeps (per-env):")
    for p in cpppaths_filtered:
        print("  ", p)
else:
    print("No CPPPATH entries found/added.")

# 6) Add the built static library for linking (LIBPATH + LIBS with short name)
libfilename = lib_static_path.name
if libfilename.startswith("lib") and libfilename.endswith(".a"):
    lib_shortname = libfilename[3:-2]
else:
    lib_shortname = lib_static_path.stem

env.Append(LIBPATH=[str(libtropic_build_dir.resolve())])
env.Append(LIBS=[lib_shortname])

print("Will link static lib:", str(lib_static_path), "as -l" + lib_shortname)

# 7) Compile HAL/CAL implementation sources into project build (after CPPPATH and CPPDEFINES added)
# Convert CMake-provided source lists to absolute paths (they may be relative to external_root)
def to_abs_paths(root: Path, src_list):
    out = []
    for s in ensure_list(src_list):
        p = Path(s)
        if not p.is_absolute():
            p = (root / p).resolve()
        else:
            p = p.resolve()
        out.append(p)
    return out

hal_source_paths = to_abs_paths(external_root, hal_sources)
cal_source_paths = to_abs_paths(external_root, cal_sources)

# Build the directories that contain those source files (unique, existing parents)
hal_dirs = sorted({ str(p.parent) for p in hal_source_paths if p.exists() })
cal_dirs = sorted({ str(p.parent) for p in cal_source_paths if p.exists() })

# IMPORTANT: add these implementation-parent directories to CPPPATH as well so headers next to .c files are found
for d in hal_dirs + cal_dirs:
    if d not in cpppaths_filtered:
        env.Append(CPPPATH=[d])
        cpppaths_filtered.append(d)
        print("Added impl-parent to CPPPATH:", d)

build_dir_expanded = env.subst("$BUILD_DIR")
hal_target_dir = str(Path(build_dir_expanded) / ("lib_" + lib_shortname + "_hal"))
cal_target_dir = str(Path(build_dir_expanded) / ("lib_" + lib_shortname + "_cal"))

for d in hal_dirs:
    print("Building HAL sources from dir:", d)
    env.BuildSources(hal_target_dir, d)

for d in cal_dirs:
    print("Building CAL sources from dir:", d)
    env.BuildSources(cal_target_dir, d)
