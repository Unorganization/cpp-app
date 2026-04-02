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
# Boost (managed via vcpkg – see vcpkg.json)
#
# To add more Boost libraries:
#   1. Add the component name to vcpkg.json  (e.g. "boost-regex")
#   2. Add the component name to the COMPONENTS list below (e.g. regex)
#   3. Add Boost::<component> to target_link_libraries below
# ---------------------------------------------------------------------------
find_package(Boost REQUIRED COMPONENTS filesystem)
target_link_libraries(${PROJECT_NAME}_lib PUBLIC Boost::filesystem)

# ---------------------------------------------------------------------------
# Precompiled header (PCH)
# Listed headers are compiled once and reused by every .cpp in the library,
# which speeds up incremental builds when headers are large (e.g. Boost).
#
# Add any header that is included by most of your source files.
# Angle-bracket headers (<...>) are system/library headers.
# Quoted headers ("...") are your own headers relative to the source tree.
# ---------------------------------------------------------------------------
target_precompile_headers(${PROJECT_NAME}_lib PRIVATE
    <iostream>
    <string>
    <boost/filesystem.hpp>
    # "src/my_header.h"   ← add your own frequently-used headers here
)

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
# Platform-specific link flags to maximise self-containment
# ---------------------------------------------------------------------------
if(WIN32)
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
# enable_testing() must be called here (the root CMakeLists.txt), not inside
# the subdirectory, for CTest to register and discover tests correctly.
# ---------------------------------------------------------------------------
enable_testing()
add_subdirectory(tests)

# ---------------------------------------------------------------------------
# Install rules (used by CPack / GitHub Actions packaging)
# ---------------------------------------------------------------------------
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION .
)

# ---------------------------------------------------------------------------
# CPack – creates a simple ZIP/TGZ archive of the installed files
# ---------------------------------------------------------------------------
set(CPACK_PACKAGE_NAME "${PROJECT_NAME}")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${PROJECT_DESCRIPTION}")
set(CPACK_STRIP_FILES TRUE)

if(WIN32)
    set(CPACK_GENERATOR "ZIP")
elseif(APPLE)
    set(CPACK_GENERATOR "TGZ")
else()
    set(CPACK_GENERATOR "TGZ")
endif()

include(CPack)
