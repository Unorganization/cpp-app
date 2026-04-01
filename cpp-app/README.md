# cpp_app

A cross-platform C++ console application that runs on Windows, macOS, and Linux.

---

## Downloading a Release Binary

Pre-built binaries are published automatically on every tagged release.

1. Go to the [**Releases**](../../releases) page of this repository.
2. Expand the **Assets** section of the latest release.
3. Download the archive that matches your platform:

| Platform | File to download |
|---|---|
| Windows 64-bit | `cpp_app-<version>-win64.zip` |
| Linux 64-bit | `cpp_app-<version>-Linux.tar.gz` |
| macOS Intel (x86_64) | `cpp_app-<version>-Darwin-x86_64.tar.gz` |
| macOS Apple Silicon (arm64) | `cpp_app-<version>-Darwin-arm64.tar.gz` |

---

## Running the Application

### Windows

1. Extract the `.zip` archive (right-click → *Extract All…*).
2. Open a **Command Prompt** or **PowerShell** window.
3. Navigate to the extracted folder.
4. Run the application:
   ```cmd
   cpp_app.exe
   ```

### macOS

1. Open **Terminal**.
2. Extract the archive:
   ```bash
   tar -xzf cpp_app-<version>-Darwin-*.tar.gz
   ```
3. Navigate into the extracted folder and run:
   ```bash
   ./cpp_app
   ```
   > **First-time Gatekeeper prompt:** macOS may block unsigned binaries.  
   > If you see *"cannot be opened because the developer cannot be verified"*,  
   > right-click the binary in Finder and choose **Open**, then click **Open** in the dialog.  
   > Alternatively, from Terminal: `xattr -d com.apple.quarantine ./cpp_app`

### Linux

1. Open a terminal.
2. Extract the archive:
   ```bash
   tar -xzf cpp_app-<version>-Linux.tar.gz
   ```
3. Navigate into the extracted folder and run:
   ```bash
   ./cpp_app
   ```
   The binary is statically linked against `libgcc` and `libstdc++` so it runs on any reasonably modern glibc-based system (Ubuntu 20.04+, Debian 11+, and equivalents) without installing additional packages.

---

## Building from Source

**Prerequisites:**
- [CMake](https://cmake.org/download/) ≥ 3.20
- A C++17-capable compiler:
  - **Windows:** Visual Studio 2022 (MSVC) or MinGW-w64
  - **macOS:** Xcode Command Line Tools (`xcode-select --install`)
  - **Linux:** GCC ≥ 9 or Clang ≥ 10 (`sudo apt install build-essential`)

```bash
# Clone the repository
git clone https://github.com/<your-username>/<your-repo>.git
cd <your-repo>/cpp-app

# Configure and build
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel

# The binary is placed in build/bin/
./build/bin/cpp_app        # Linux / macOS
build\bin\cpp_app.exe      # Windows
```

---

## Creating a Release

To publish a new release and trigger the automated build + upload:

```bash
git tag v1.0.0
git push origin v1.0.0
```

GitHub Actions will:
1. Build the application on all four platform/architecture combinations.
2. Package each binary into an archive.
3. Create a GitHub Release named **Release v1.0.0** with all archives attached.

You can then edit the release notes on GitHub if desired.

---

## Supported Platforms

| OS | Architectures | Minimum version |
|---|---|---|
| Windows | x64 | Windows 10 / Server 2016 |
| macOS | x86_64 (Intel), arm64 (Apple Silicon) | macOS 12 Monterey |
| Linux | x86_64 | Ubuntu 20.04 / Debian 11 or equivalent glibc ≥ 2.31 |
