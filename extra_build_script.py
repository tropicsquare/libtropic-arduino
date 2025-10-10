from pathlib import Path
import subprocess
import shlex
import re

Import("env", "projenv")

LIB_NAME_ON_DISK = "LibtropicArduino"
BUILD_TARGET = "tropic"
PIO_LIBTROPIC_BUILD_FLAGS_OPT = "libtropic_build_flags"

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
external_root = library_dir / "libtropic"
build_dir = library_dir / "libtropic_build"
hal_dir = external_root / "hal" / "port" / "arduino"

# list HAL source files (if any)
hal_sources = []
for f in hal_dir.iterdir():
    if f.suffix in (".c", ".cpp", ".cc") and f.is_file():
        hal_sources.append(f)

# 4) Configure & build libtropic with CMake using PlatformIO toolchain
build_dir.mkdir(parents=True, exist_ok=True)

cc = env.get("CC")
cxx = env.get("CXX")
ar = env.get("AR")

# Base cmake args
cmake_args = [
    "cmake",
    "-S", str(external_root),
    "-B", str(build_dir),
]

# pass toolchain compilers if provided by PlatformIO env
if cc:
    cmake_args.append(f"-DCMAKE_C_COMPILER={cc}")
if cxx:
    cmake_args.append(f"-DCMAKE_CXX_COMPILER={cxx}")
if ar:
    cmake_args.append(f"-DCMAKE_AR={ar}")

# Read libtropic_build_flags from platformio.ini (per-env)
libtropic_build_flags = env.GetProjectOption(PIO_LIBTROPIC_BUILD_FLAGS_OPT)
try:
    extra_tokens = shlex.split(libtropic_build_flags)
except Exception:
    extra_tokens = libtropic_build_flags.split()
# Append as-is; expected tokens are -D... items (or other CMake options)
cmake_args.extend(extra_tokens)
print(f"Appending {PIO_LIBTROPIC_BUILD_FLAGS_OPT} to CMake args:", extra_tokens)

print("Running CMake configure:", " ".join(cmake_args))
subprocess.check_call(cmake_args)

print(f"Running CMake build (target '{BUILD_TARGET}')")
subprocess.check_call(["cmake", "--build", str(build_dir), "--target", BUILD_TARGET])

# Find produced .a
lib_static_path = build_dir / "libtropic.a"
if not lib_static_path.is_file():
    raise RuntimeError(f"Could not find produced static library in {build_dir}")

# Parse flags.make for target
flags_make_path = build_dir / "CMakeFiles" / f"{BUILD_TARGET}.dir" / "flags.make"
parsed_defines = []

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
cpppaths = []
cpppaths.append(str(external_root / "include"))
cpppaths.append(str(external_root / "src"))
cpppaths.append(str(hal_dir))

if libdeps_env_dir and libdeps_env_dir.is_dir():
    for libfolder in libdeps_env_dir.iterdir():
        libfolder_path = libfolder
        if not libfolder_path.is_dir():
            continue
        inc1 = libfolder_path / "include"
        inc2 = libfolder_path / "src"
        inc3 = libfolder_path / "src" / "include"
        if inc1.is_dir():
            cpppaths.append(str(inc1))
        if inc2.is_dir():
            cpppaths.append(str(inc2))
        if inc3.is_dir():
            cpppaths.append(str(inc3))
        cpppaths.append(str(libfolder_path))

# Dedupe while preserving order
seen = set()
cpppaths_filtered = []
for p in cpppaths:
    if p and Path(p).is_dir() and p not in seen:
        seen.add(p)
        cpppaths_filtered.append(p)

if cpppaths_filtered:
    env.Append(CPPPATH=cpppaths_filtered)
    print("Added CPPPATH entries for libtropic and libdeps (per-env):")
    for p in cpppaths_filtered:
        print("  ", p)

# 6) Add the built static library for linking (LIBPATH + LIBS with short name)
libfilename = lib_static_path.name
if libfilename.startswith("lib") and libfilename.endswith(".a"):
    lib_shortname = libfilename[3:-2]
else:
    lib_shortname = lib_static_path.stem

env.Append(LIBPATH=[str(build_dir)])
env.Append(LIBS=[lib_shortname])

print("Will link static lib:", str(lib_static_path), "as -l" + lib_shortname)

# 7) Compile HAL sources into project build (after CPPPATH and CPPDEFINES added)
build_dir_expanded = env.subst("$BUILD_DIR")
hal_target_dir = str(Path(build_dir_expanded) / ("lib_" + lib_shortname + "_hal"))

if not hal_sources:
    # keep the same failure semantics as before
    raise RuntimeError("No HAL sources found at", str(hal_dir))

print("Compiling HAL sources into project build from", str(hal_dir))
env.BuildSources(hal_target_dir, str(hal_dir))
