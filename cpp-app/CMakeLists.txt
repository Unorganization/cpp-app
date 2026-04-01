cmake_minimum_required(VERSION 3.20)

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

add_executable(cpp_app src/main.cpp)

# ---------------------------------------------------------------------------
# Platform-specific link flags to maximise self-containment
# ---------------------------------------------------------------------------

if(WIN32)
    # On Windows (MSVC or MinGW) the CRT is usually linked statically.
    # With MSVC: switch from /MD (DLL CRT) to /MT (static CRT).
    if(MSVC)
        foreach(flag_var
            CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
            CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO)
            string(REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
        endforeach()
    endif()

    # With MinGW (used by GitHub-hosted Windows runners via msys2 or the
    # native toolchain) we can pass -static to pull in libgcc and libstdc++.
    if(MINGW)
        target_link_options(cpp_app PRIVATE -static)
    endif()

elseif(APPLE)
    # macOS: the only safe shared libs to rely on are the OS-shipped dylibs
    # (libSystem, libc++).  Those ship with every supported macOS version and
    # do NOT need to be bundled.  There is nothing extra to do here.
    # We intentionally do NOT pass -static because static linking of libc++ on
    # Apple Silicon requires an additional toolchain setup that is fragile.
    target_link_options(cpp_app PRIVATE "LINKER:-dead_strip")

elseif(UNIX)
    # Linux: statically link libgcc and libstdc++ so the binary runs on any
    # glibc-based distro from the last ~5 years without requiring those
    # packages to be installed.  We do NOT statically link glibc itself
    # because a statically linked glibc is not safe to redistribute and the
    # dynamic glibc ABI is stable and backward compatible.
    target_link_options(cpp_app PRIVATE
        -static-libgcc
        -static-libstdc++
    )
endif()

# ---------------------------------------------------------------------------
# Compiler warnings
# ---------------------------------------------------------------------------
if(MSVC)
    target_compile_options(cpp_app PRIVATE /W4)
else()
    target_compile_options(cpp_app PRIVATE -Wall -Wextra -Wpedantic)
endif()

# ---------------------------------------------------------------------------
# Install rules (used by CPack / GitHub Actions packaging)
# ---------------------------------------------------------------------------
install(TARGETS cpp_app
    RUNTIME DESTINATION .
)

# ---------------------------------------------------------------------------
# CPack – creates a simple ZIP/TGZ archive of the installed files
# ---------------------------------------------------------------------------
set(CPACK_PACKAGE_NAME "cpp_app")
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
