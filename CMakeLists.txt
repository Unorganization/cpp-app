cmake_minimum_required(VERSION 3.20)

# ---------------------------------------------------------------------------
# vcpkg triplet – must be set before project() so the toolchain picks it up.
# On Windows we ask vcpkg for the "static" triplet so that Boost and the CRT
# are both linked statically (matching the /MT flag below).
# ---------------------------------------------------------------------------
if(WIN32)
    set(VCPKG_TARGET_TRIPLET "x64-windows-static" CACHE STRING "vcpkg triplet")
endif()

# ---------------------------------------------------------------------------
# PROJECT NAME
# Change the name here (first argument to project()) to rename the executable
# and all release archives. Every other reference in this file uses
# ${PROJECT_NAME} automatically – nothing else needs to be touched.
# ---------------------------------------------------------------------------
project(cpp_app
    VERSION 1.0.0
    DESCRIPTION "Cross-platform C++ console application"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Output the binary directly to the build root for easy packaging
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# ---------------------------------------------------------------------------
# Static CRT on Windows (/MD → /MT)
# Must be done before any add_library / add_executable calls so that the
# flag replacement applies to all targets, including the test runner.
# ---------------------------------------------------------------------------
if(WIN32 AND MSVC)
    foreach(flag_var
        CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
        CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO)
        string(REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
    endforeach()
endif()

# ---------------------------------------------------------------------------
# Production code library
#
# All application logic lives here so it is shared by both the main
# executable and the test runner. Only main.cpp stays in the executable.
#
# Add one line per .cpp file you create. Header files (.h / .hpp) need no
# CMake entry – just #include them in your .cpp files as normal.
# ---------------------------------------------------------------------------
add_library(${PROJECT_NAME}_lib STATIC)

target_sources(${PROJECT_NAME}_lib PRIVATE
    src/math_utils.cpp
    # src/my_module.cpp   ← add new .cpp files here
)

# PUBLIC so the executable and test targets inherit the include path
# automatically when they link against this library.
target_include_directories(${PROJECT_NAME}_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# ---------------------------------------------------------------------------
# Dependencies
#
# All third-party libraries are managed via vcpkg (see vcpkg.json).
#
# Header-only libraries (asio, CLI11, nlohmann/json, stduuid, spdlog) require
# no compiled object files; CMake propagates their include paths automatically.
#
# cpp-httplib, reproc, and cpptrace are not available for wasm32-emscripten
# (they require OS networking / process / stack-unwinding support).
#
# std::filesystem, std::regex, std::thread, and std::future are part of the
# C++17 standard – no find_package required.
# ---------------------------------------------------------------------------
find_package(asio          CONFIG REQUIRED)
find_package(CLI11         CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(ada           CONFIG REQUIRED)
find_package(stduuid       CONFIG REQUIRED)
find_package(Threads       REQUIRED)
find_package(spdlog        CONFIG REQUIRED)
find_package(date          CONFIG REQUIRED)
find_package(csv-parser    CONFIG REQUIRED)

target_link_libraries(${PROJECT_NAME}_lib PUBLIC
    asio::asio                   # standalone Asio (header-only; handles Winsock on Windows)
    CLI11::CLI11                 # header-only; command-line argument parsing
    nlohmann_json::nlohmann_json # header-only; JSON parse and serialize
    ada::ada                     # WHATWG-compliant URL parsing
    stduuid                      # header-only; UUID generation
    Threads::Threads             # pthreads on Linux, built-in on Windows/macOS
    spdlog::spdlog_header_only   # header-only; structured logging
    date::date                   # Howard Hinnant's date/calendar library (no TZ data)
    csv::csv                     # Vince's CSV parser
)

# Standalone Asio must not look for Boost headers.
target_compile_definitions(${PROJECT_NAME}_lib PUBLIC ASIO_STANDALONE)

# Libraries that require OS capabilities unavailable in the browser sandbox.
if(NOT EMSCRIPTEN)
    find_package(httplib   CONFIG REQUIRED)
    find_package(reproc++  CONFIG REQUIRED)
    find_package(cpptrace  CONFIG REQUIRED)
    target_link_libraries(${PROJECT_NAME}_lib PUBLIC
        httplib::httplib        # header-only; HTTP client (plain TCP)
        reproc++::reproc++      # child-process management
        cpptrace::cpptrace      # stack trace capture
        ${CMAKE_DL_LIBS}        # -ldl on Linux; needed by cpptrace
    )
endif()

if(WIN32)
    # Target Windows XP x64 / Server 2003 (0x0502) — the earliest AMD64 Windows.
    # This is intentionally conservative: if code ever calls a newer Win32 API,
    # the compiler will error, making the dependency explicit and opt-in per project.
    # NOMINMAX prevents <windows.h> from defining min/max macros that would shadow
    # std::min / std::max.  WIN32_LEAN_AND_MEAN speeds up compilation by excluding
    # rarely-used Win32 headers.
    target_compile_definitions(${PROJECT_NAME}_lib PUBLIC
        _WIN32_WINNT=0x0502
        WINVER=0x0502
        NOMINMAX
        WIN32_LEAN_AND_MEAN
    )
endif()

# ---------------------------------------------------------------------------
# Precompiled header (PCH)
# Listed headers are compiled once and reused by every .cpp in the library,
# which speeds up incremental builds when headers are large.
#
# Add any header that is included by most of your source files.
# ---------------------------------------------------------------------------
# httplib.h is large and not available for wasm32-emscripten.
set(_pch_headers
    <iostream>
    <string>
    <vector>
    <asio.hpp>
    <nlohmann/json.hpp>
    <spdlog/spdlog.h>
)
if(NOT EMSCRIPTEN)
    list(APPEND _pch_headers <httplib.h>)
endif()
target_precompile_headers(${PROJECT_NAME}_lib PRIVATE ${_pch_headers})
unset(_pch_headers)

# Compiler warnings
if(MSVC)
    target_compile_options(${PROJECT_NAME}_lib PRIVATE /W4)
else()
    target_compile_options(${PROJECT_NAME}_lib PRIVATE -Wall -Wextra -Wpedantic)
endif()

# ---------------------------------------------------------------------------
# Main executable
# Only main.cpp belongs here. All other production code belongs in
# ${PROJECT_NAME}_lib above so it can be reached by tests.
# ---------------------------------------------------------------------------
add_executable(${PROJECT_NAME})

target_sources(${PROJECT_NAME} PRIVATE
    src/main.cpp
)

target_link_libraries(${PROJECT_NAME} PRIVATE ${PROJECT_NAME}_lib)

# ---------------------------------------------------------------------------
# Embedded resource: data/sample.json
#
# Native targets (Linux, macOS, Windows, Android):
#   llvm-objcopy --input-target binary wraps the raw bytes in a native object
#   file and exports linker symbols _binary_sample_json_start/end. The same
#   extern "C" declaration in embedded_resource.cpp works on ELF, Mach-O, and
#   COFF x64 because llvm-objcopy follows each format's C-linkage convention.
#
# WebAssembly / Emscripten:
#   llvm-objcopy does not produce wasm-compatible objects. Instead, the entire
#   data/ directory is pre-packaged into Emscripten's virtual filesystem via
#   --preload-file at link time. embedded_resource.cpp has an #ifdef path that
#   reads /data/sample.json through standard POSIX I/O at runtime.
# ---------------------------------------------------------------------------
if(NOT EMSCRIPTEN)
    find_program(LLVM_OBJCOPY
        NAMES
            llvm-objcopy
            llvm-objcopy-18 llvm-objcopy-17 llvm-objcopy-16
            llvm-objcopy-15 llvm-objcopy-14 llvm-objcopy-13 llvm-objcopy-12
        HINTS
            /usr/local/opt/llvm/bin         # macOS Homebrew (Intel runner, macos-13)
            /opt/homebrew/opt/llvm/bin      # macOS Homebrew (ARM runner,   macos-14)
            "C:/Program Files/LLVM/bin"     # Windows LLVM install
            "$ENV{ANDROID_NDK_LATEST_HOME}/toolchains/llvm/prebuilt/linux-x86_64/bin"
        DOC "llvm-objcopy used to embed binary resources at link time"
    )

    if(NOT LLVM_OBJCOPY)
        message(FATAL_ERROR
            "llvm-objcopy not found. Install LLVM tools for your platform:\n"
            "  Linux          : sudo apt-get install llvm\n"
            "  macOS          : brew install llvm\n"
            "  Windows        : install Visual Studio 'C++ Clang tools for Windows'\n"
            "  Termux         : pkg install llvm\n"
            "  Android NDK CI : ensure ANDROID_NDK_LATEST_HOME points to a valid NDK")
    endif()

    if(WIN32)
        set(_EMBED_TARGET  "coff-x86-64")
        set(_EMBED_ARCH    "i386:x86-64")
        set(_EMBED_OBJ_EXT "obj")
    elseif(APPLE)
        if(CMAKE_OSX_ARCHITECTURES MATCHES "arm64" OR
           CMAKE_SYSTEM_PROCESSOR  MATCHES "arm64|aarch64")
            set(_EMBED_TARGET "mach-o-arm64")
            set(_EMBED_ARCH   "aarch64")
        else()
            set(_EMBED_TARGET "mach-o-x86-64")
            set(_EMBED_ARCH   "i386:x86-64")
        endif()
        set(_EMBED_OBJ_EXT "o")
    elseif(CMAKE_SYSTEM_NAME STREQUAL "Android")
        set(_EMBED_TARGET  "elf64-littleaarch64")
        set(_EMBED_ARCH    "aarch64")
        set(_EMBED_OBJ_EXT "o")
    else()  # Linux x86_64
        set(_EMBED_TARGET  "elf64-x86-64")
        set(_EMBED_ARCH    "i386:x86-64")
        set(_EMBED_OBJ_EXT "o")
    endif()

    set(_EMBED_OBJ "${CMAKE_CURRENT_BINARY_DIR}/sample_json.${_EMBED_OBJ_EXT}")

    add_custom_command(
        OUTPUT   "${_EMBED_OBJ}"
        COMMAND  "${LLVM_OBJCOPY}"
                 --input-target          binary
                 --output-target         "${_EMBED_TARGET}"
                 --binary-architecture   "${_EMBED_ARCH}"
                 "sample.json"
                 "${_EMBED_OBJ}"
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/data"
        DEPENDS   "${CMAKE_SOURCE_DIR}/data/sample.json"
        COMMENT   "Embedding data/sample.json (${_EMBED_TARGET}) via llvm-objcopy"
        VERBATIM
    )

    set_source_files_properties("${_EMBED_OBJ}" PROPERTIES
        EXTERNAL_OBJECT TRUE
        GENERATED       TRUE
    )

    target_sources(${PROJECT_NAME} PRIVATE
        src/embedded_resource.cpp
        "${_EMBED_OBJ}"
    )

else()  # EMSCRIPTEN
    # No linker-symbol embedding; the file is loaded from the virtual FS.
    target_sources(${PROJECT_NAME} PRIVATE src/embedded_resource.cpp)
endif()

# ---------------------------------------------------------------------------
# Platform-specific link flags to maximise self-containment
# ---------------------------------------------------------------------------
if(EMSCRIPTEN)
    # Produce a self-contained bundle: HTML page + JS glue + WASM binary.
    # Open cpp_app.html in any modern browser to run the application with a
    # simulated terminal provided by Emscripten's default shell page.
    set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX ".html")

    target_link_options(${PROJECT_NAME} PRIVATE
        -sASYNCIFY               # suspend/resume for blocking calls (e.g. ioc.run())
        -sALLOW_MEMORY_GROWTH=1  # heap grows on demand
        -sINITIAL_MEMORY=67108864 # 64 MB initial heap
        -sEXIT_RUNTIME=1         # flush I/O and run destructors on exit
        # Pre-package the data/ directory into the virtual filesystem so that
        # embedded_resource.cpp can read /data/sample.json at runtime without
        # any fetch/XHR – the bytes ship inside the generated .data bundle.
        "SHELL:--preload-file ${CMAKE_SOURCE_DIR}/data@/data"
    )

elseif(WIN32)
    # With MinGW we can pass -static to pull in libgcc and libstdc++.
    if(MINGW)
        target_link_options(${PROJECT_NAME} PRIVATE -static)
    endif()

elseif(APPLE)
    # macOS: rely on the OS-shipped libSystem / libc++ (present on every
    # supported macOS version). -dead_strip removes unused code.
    target_link_options(${PROJECT_NAME} PRIVATE "LINKER:-dead_strip")

elseif(UNIX)
    # Linux: statically link libgcc and libstdc++ so the binary runs on any
    # glibc-based distro from the last ~5 years. These flags work with both
    # Clang (which is required – see the workflow) and GCC.
    # glibc itself remains dynamic because its ABI is stable and a static
    # glibc is not safe to redistribute.
    target_link_options(${PROJECT_NAME} PRIVATE
        -static-libgcc
        -static-libstdc++
    )
endif()

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic)
endif()

# ---------------------------------------------------------------------------
# Tests
# Skipped for EMSCRIPTEN: the test runner is a native binary and cannot
# execute inside the WASM sandbox without a browser or Node.js host.
# enable_testing() must be called in the root CMakeLists.txt (not inside the
# subdirectory) for CTest to register and discover tests correctly.
# ---------------------------------------------------------------------------
if(NOT EMSCRIPTEN)
    enable_testing()
    add_subdirectory(tests)
endif()

# ---------------------------------------------------------------------------
# Install rules (used by CPack / GitHub Actions packaging)
# ---------------------------------------------------------------------------
if(EMSCRIPTEN)
    # Install the full WASM bundle: HTML shell, JS glue, WASM binary, and the
    # pre-packaged virtual-FS data file generated by --preload-file.
    install(DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/"
        DESTINATION .
        FILES_MATCHING
            PATTERN "*.html"
            PATTERN "*.js"
            PATTERN "*.wasm"
            PATTERN "*.data"
    )
else()
    install(TARGETS ${PROJECT_NAME}
        RUNTIME DESTINATION .
    )
endif()

# ---------------------------------------------------------------------------
# CPack – creates a simple ZIP/TGZ archive of the installed files
# ---------------------------------------------------------------------------
set(CPACK_PACKAGE_NAME "${PROJECT_NAME}")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${PROJECT_DESCRIPTION}")

if(NOT EMSCRIPTEN)
    set(CPACK_STRIP_FILES TRUE)  # strip native binaries; not applicable to WASM
endif()

if(WIN32)
    set(CPACK_GENERATOR "ZIP")
elseif(APPLE)
    set(CPACK_GENERATOR "TGZ")
else()
    set(CPACK_GENERATOR "TGZ")
endif()

include(CPack)
