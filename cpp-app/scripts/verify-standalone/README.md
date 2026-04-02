# Standalone Verification Scripts

These scripts allow developers to manually verify that a built executable is "self-contained" and only depends on core system libraries.

## Core Principle

This project aims for **maximum binary self-containment**. A binary is considered standalone if it:
1.  Links the C++ runtime and third-party libraries (like Boost) **statically**.
2.  Only depends on stable, standard libraries provided by the host operating system.

---

## Windows Verification (`verify-windows.ps1`)

This PowerShell script checks if any non-Windows DLLs are listed as dependencies.

### Prerequisites
- **`dumpbin.exe`**: Provided with Visual Studio.
- **Visual Studio Developer PowerShell**: Run the script from this shell to ensure `dumpbin` is in the PATH.

### Usage
```powershell
.\verify-windows.ps1 -targetExe "path\to\your\app.exe"
```

### What it checks
- Extracts DLL dependencies via `dumpbin /dependents`.
- Filters out "API-set" DLLs (`api-ms-win-*`).
- Verifies that each remaining DLL is located in `System32` or `SysWOW64`.
- Checks the DLL's metadata to ensure the `ProductName` matches Microsoft Windows.

---

## Linux Verification (`verify-linux.sh`)

This bash script uses `ldd` to inspect the shared library dependencies.

### Prerequisites
- **`ldd`**: Standard tool on most Linux distributions.
- **`awk`**: Standard text processing utility.

### Usage
```bash
chmod +x verify-linux.sh
./verify-linux.sh /path/to/your/binary
```

### What it checks
- Lists dependencies using `ldd`.
- Verifies that only core `glibc` libraries (e.g., `libc.so`, `libm.so`, `libpthread.so`) are linked dynamically.
- Flags `libgcc_s.so` or `libstdc++.so` as non-standalone if they appear (indicating they weren't linked statically).

---

## macOS Verification (`verify-macos.sh`)

This bash script uses `otool` to inspect the dynamic library dependencies.

### Prerequisites
- **Xcode Command Line Tools**: Provides the `otool` utility.

### Usage
```bash
chmod +x verify-macos.sh
./verify-macos.sh /path/to/your/binary
```

### What it checks
- Lists dependencies using `otool -L`.
- Verifies that only core system dylibs (`/usr/lib/libSystem.B.dylib`, `/usr/lib/libc++.1.dylib`) are used.
- Specifically flags dependencies on **Homebrew** paths (`/usr/local/lib` or `/opt/homebrew/lib`) as they break portability.

---

## Android (Termux) Verification

In Termux, you can use the same `verify-linux.sh` script.

### Prerequisites
- `pkg install ldd` (if not present)

### Note
Android binaries use **Bionic libc**. The script will flag non-Bionic dependencies. In a standalone build, only `libc.so`, `libm.so`, and `libdl.so` from the system should appear.
