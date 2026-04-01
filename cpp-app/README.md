# cpp_app

A cross-platform C++ console application that runs on Windows, macOS, and Linux.

> **For AI agents and contributors:** Read [`AGENTS.md`](AGENTS.md) before making any
> changes. It documents the self-containment design principles that all modifications
> must maintain.

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

This project uses **Clang** on all platforms and **vcpkg** for dependency management.
Follow the one-time setup for your platform, then the build steps are identical
everywhere.

---

### Windows

**One-time setup**

1. Install [Visual Studio 2022](https://visualstudio.microsoft.com/) with these
   workloads/components selected in the installer:
   - *Desktop development with C++*
   - *C++ Clang tools for Windows* (individual component — this installs `clang-cl`)

2. Install vcpkg and set the environment variable:
   ```powershell
   git clone https://github.com/microsoft/vcpkg.git C:\vcpkg
   C:\vcpkg\bootstrap-vcpkg.bat -disableMetrics
   [System.Environment]::SetEnvironmentVariable("VCPKG_ROOT", "C:\vcpkg", "User")
   $env:VCPKG_ROOT = "C:\vcpkg"   # apply to the current session
   ```

**Configure and build** (PowerShell)

```powershell
git clone https://github.com/<your-username>/<your-repo>.git
cd <your-repo>\cpp-app

cmake -S . -B build `
  -G "Visual Studio 17 2022" -A x64 -T ClangCL `
  -DCMAKE_BUILD_TYPE=Release `
  -DCMAKE_TOOLCHAIN_FILE="$env:VCPKG_ROOT\scripts\buildsystems\vcpkg.cmake" `
  -DVCPKG_TARGET_TRIPLET=x64-windows-static

cmake --build build --config Release --parallel
```

The binary is at `build\bin\cpp_app.exe`.

---

### macOS

**One-time setup**

1. Install Xcode Command Line Tools (provides Apple Clang):
   ```bash
   xcode-select --install
   ```

2. Install vcpkg and set the environment variable:
   ```bash
   git clone https://github.com/microsoft/vcpkg.git ~/vcpkg
   ~/vcpkg/bootstrap-vcpkg.sh -disableMetrics
   echo 'export VCPKG_ROOT="$HOME/vcpkg"' >> ~/.zshrc   # or ~/.bashrc
   source ~/.zshrc
   ```

**Configure and build**

```bash
git clone https://github.com/<your-username>/<your-repo>.git
cd <your-repo>/cpp-app

cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_OSX_DEPLOYMENT_TARGET=12.0 \
  -DCMAKE_TOOLCHAIN_FILE="$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"

cmake --build build --parallel
```

The binary is at `build/bin/cpp_app`.

---

### Linux (Ubuntu / Debian)

**One-time setup**

1. Install Clang and the supporting build tools:
   ```bash
   sudo apt-get update
   sudo apt-get install -y build-essential clang cmake ninja-build
   ```

2. Install vcpkg and set the environment variable:
   ```bash
   git clone https://github.com/microsoft/vcpkg.git ~/vcpkg
   ~/vcpkg/bootstrap-vcpkg.sh -disableMetrics
   echo 'export VCPKG_ROOT="$HOME/vcpkg"' >> ~/.bashrc
   source ~/.bashrc
   ```

**Configure and build**

```bash
git clone https://github.com/<your-username>/<your-repo>.git
cd <your-repo>/cpp-app

cmake -S . -B build \
  -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_C_COMPILER=clang \
  -DCMAKE_CXX_COMPILER=clang++ \
  -DCMAKE_TOOLCHAIN_FILE="$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"

cmake --build build --parallel
```

The binary is at `build/bin/cpp_app`.

> **vcpkg first run:** The first configure will download and compile all dependencies
> (including Boost) from source. This takes several minutes. Subsequent builds are
> fast because vcpkg caches the compiled packages.

---

## Renaming the Project and Executable

Change **one line** in **one file** — the first argument of `project()` in
`CMakeLists.txt`:

```cmake
project(my_app          ← change this to your chosen name
    VERSION 1.0.0
    DESCRIPTION "Cross-platform C++ console application"
    LANGUAGES CXX
)
```

That is the only edit required. CMake stores this name in the built-in variable
`${PROJECT_NAME}`, and every other command in `CMakeLists.txt` already uses that
variable — `add_executable`, `target_link_libraries`, `install`, and the CPack archive
name. The GitHub Actions workflow uses broad filename globs (`build/*.zip`,
`build/*.tar.gz`) so it picks up the renamed archives automatically.

After the edit, delete your `build/` folder and re-run CMake from scratch so the new
name is applied cleanly:

```bash
rm -rf build
cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_TOOLCHAIN_FILE="$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"
cmake --build build --parallel
# Binary is now at build/bin/my_app  (or my_app.exe on Windows)
```

> **`vcpkg.json` `"name"` field:** This is cosmetic metadata used only to identify
> the manifest to vcpkg. It has no effect on the binary name or build output. You can
> optionally update it to match, but it is not required. If you do update it, vcpkg
> names must be all-lowercase with hyphens (e.g. `"my-app"`).

---

## Adding Source Files (Modules)

CMake does not scan your source folder automatically. Every new `.cpp` file must be
registered in `CMakeLists.txt` or it will silently not be compiled. Header files
require no registration — create them anywhere in `src/` and `#include` them as normal.

### Step 1 — Create your files

Place them anywhere inside `src/`. A flat layout works fine for small projects; use
sub-folders for larger ones.

```
src/
├── main.cpp
├── my_module.cpp   ← new
└── my_module.h     ← new (no CMake entry needed)
```

### Step 2 — Register the `.cpp` file in `CMakeLists.txt`

Find the `target_sources` block (just below `add_executable`) and add one line:

```cmake
target_sources(${PROJECT_NAME} PRIVATE
    src/main.cpp
    src/my_module.cpp   ← add this line
)
```

Header files (`.h`, `.hpp`) are **not** listed here. CMake finds them automatically
when your code `#include`s them.

### Step 3 — Rebuild

No need to delete the build folder for this change — CMake detects the updated source
list on the next build:

```bash
cmake --build build --parallel
```

That's all. The new module will be compiled and linked into the executable.

---

## Adding Boost Libraries

Boost components are managed in two places: `vcpkg.json` (which packages to install) and `CMakeLists.txt` (how to link them). Both files must be updated together.

### Step 1 — Find the vcpkg package name

Each Boost library is its own vcpkg package named `boost-<library>`.  
Common examples:

| Boost library | vcpkg package name |
|---|---|
| Boost.Filesystem | `boost-filesystem` |
| Boost.Regex | `boost-regex` |
| Boost.Asio | `boost-asio` |
| Boost.Json | `boost-json` |
| Boost.Program_options | `boost-program-options` |
| Boost.Thread | `boost-thread` |
| Boost.Beast (HTTP/WebSocket) | `boost-beast` |

To search for others: `vcpkg search boost`

### Step 2 — Add the package to `vcpkg.json`

Open `vcpkg.json` at the root of the `cpp-app/` folder and add the package name to the `"dependencies"` array:

```json
{
  "name": "cpp-app",
  "version": "1.0.0",
  "dependencies": [
    "boost-filesystem",
    "boost-regex"
  ]
}
```

### Step 3 — Update `CMakeLists.txt`

Find the `find_package` line for Boost and add your new component to the `COMPONENTS` list, then add the corresponding `Boost::<component>` target to `target_link_libraries`.

**Before:**
```cmake
find_package(Boost REQUIRED COMPONENTS filesystem)
target_link_libraries(cpp_app PRIVATE Boost::filesystem)
```

**After:**
```cmake
find_package(Boost REQUIRED COMPONENTS filesystem regex)
target_link_libraries(cpp_app PRIVATE Boost::filesystem Boost::regex)
```

The CMake target name is always `Boost::` followed by the component name in lowercase (e.g. `Boost::regex`, `Boost::thread`, `Boost::program_options`).

### Step 4 — Include the header in your source file

```cpp
#include <boost/regex.hpp>   // example
```

### Step 5 — Re-run CMake

vcpkg reads `vcpkg.json` automatically during the CMake configure step. Just re-run configure and the new library will be downloaded, built, and linked:

```bash
cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_TOOLCHAIN_FILE="$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"
cmake --build build --parallel
```

**CI is handled automatically.** When you push the updated `vcpkg.json` and `CMakeLists.txt`, the GitHub Actions workflow installs the new library on all four platform/architecture runners before building.

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
