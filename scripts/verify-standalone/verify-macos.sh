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

# Use otool to get dependencies
DEPS=$(otool -L "$TARGET_BIN" | awk '{print $1}' | tail -n +2)

ALL_CLEAN=true

# List of allowed macOS system libraries
# libSystem.B.dylib, libc++.1.dylib are the main ones.
ALLOWED_REGEX="^(/usr/lib/libSystem\.B\.dylib|/usr/lib/libc\+\+\.1\.dylib)"

for lib in $DEPS; do
    # Check if lib is in the allowed list
    if ! [[ "$lib" =~ $ALLOWED_REGEX ]]; then
        # Check for Homebrew paths
        if [[ "$lib" =~ ^(/usr/local/lib|/opt/homebrew/lib) ]]; then
            echo -e "\033[0;31m[!] $lib - Non-standalone: Homebrew dependency detected!\033[0m"
            ALL_CLEAN=false
        else
            echo -e "\033[0;33m[-] $lib - Non-standard dependency\033[0m"
            ALL_CLEAN=false
        fi
    else
        echo -e "\033[0;32m[+] $lib - System OK\033[0m"
    fi
done

if [ "$ALL_CLEAN" = true ]; then
    echo -e "\n\033[0;32mSUCCESS: Binary only depends on core macOS system libraries.\033[0m"
else
    echo -e "\n\033[0;33mWARNING: Non-system dependencies detected.\033[0m"
fi
