#!/bin/bash

TARGET_BIN="$1"

if [ -z "$TARGET_BIN" ]; then
    echo "Usage: $0 <path-to-binary>"
    exit 1
fi

if [ ! -f "$TARGET_BIN" ]; then
    echo "Error: Binary not found: $TARGET_BIN"
    exit 1
fi

echo "Verifying standalone status for: $TARGET_BIN"

# Use ldd to get dependencies
DEPS=$(ldd "$TARGET_BIN" | awk '{print $1}')

ALL_CLEAN=true

# List of allowed system/stable libraries
# glibc (libc.so, libm.so, libpthread.so, librt.so, libdl.so, ld-linux)
# These are considered stable ABIs across Linux distributions.
ALLOWED_REGEX="^(libc\.so|libm\.so|libpthread\.so|librt\.so|libdl\.so|ld-linux|/lib64/ld-linux|linux-vdso\.so)"

for lib in $DEPS; do
    # Skip vdso
    if [[ "$lib" =~ linux-vdso\.so ]]; then continue; fi
    
    # Check if lib is in the allowed list
    if ! [[ "$lib" =~ $ALLOWED_REGEX ]]; then
        # Check if it's libgcc or libstdc++ (which we should link statically)
        if [[ "$lib" =~ (libgcc_s\.so|libstdc\+\+\.so) ]]; then
            echo -e "\033[0;31m[!] $lib - Non-standalone: Should be linked statically!\033[0m"
            ALL_CLEAN=false
        else
            echo -e "\033[0;33m[-] $lib - Potential non-system dependency\033[0m"
            ALL_CLEAN=false
        fi
    else
        echo -e "\033[0;32m[+] $lib - System OK\033[0m"
    fi
done

if [ "$ALL_CLEAN" = true ]; then
    echo -e "\n\033[0;32mSUCCESS: Binary only depends on core system libraries (glibc).\033[0m"
else
    echo -e "\n\033[0;33mWARNING: Potential non-standalone dependencies detected.\033[0m"
fi
