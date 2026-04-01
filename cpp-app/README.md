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

**Prerequisites:**
- [CMake](https://cmake.org/download/) ≥ 3.20
- [vcpkg](https://vcpkg.io) (see setup instructions below)
- A C++17-capable compiler:
  - **Windows:** Visual Studio 2022 (MSVC)
  - **macOS:** Xcode Command Line Tools (`xcode-select --install`)
  - **Linux:** GCC ≥ 9 (`sudo apt install build-essential`)

### 1 — Install vcpkg (one-time setup)

vcpkg is the package manager used by this project to provide Boost and any other C++ libraries. Install it once on your machine and point an environment variable at it.

**macOS / Linux:**
```bash
git clone https://github.com/microsoft/vcpkg.git ~/vcpkg
~/vcpkg/bootstrap-vcpkg.sh -disableMetrics
echo 'export VCPKG_ROOT="$HOME/vcpkg"' >> ~/.bashrc   # or ~/.zshrc on macOS
source ~/.bashrc
```

**Windows (PowerShell):**
```powershell
git clone https://github.com/microsoft/vcpkg.git C:\vcpkg
C:\vcpkg\bootstrap-vcpkg.bat -disableMetrics
[System.Environment]::SetEnvironmentVariable("VCPKG_ROOT", "C:\vcpkg", "User")
$env:VCPKG_ROOT = "C:\vcpkg"   # apply to the current session
```

### 2 — Configure and build

```bash
# Clone the repository
git clone https://github.com/<your-username>/<your-repo>.git
cd <your-repo>/cpp-app

# Configure – vcpkg will automatically download and build all dependencies
# listed in vcpkg.json before CMake configures the project.
cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_TOOLCHAIN_FILE="$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"

# Windows (PowerShell) – use the static triplet to match the static CRT:
# cmake -S . -B build -G "Visual Studio 17 2022" -A x64 `
#   -DCMAKE_BUILD_TYPE=Release `
#   -DCMAKE_TOOLCHAIN_FILE="$env:VCPKG_ROOT\scripts\buildsystems\vcpkg.cmake" `
#   -DVCPKG_TARGET_TRIPLET=x64-windows-static

cmake --build build --parallel

# The binary is placed in build/bin/
./build/bin/cpp_app        # Linux / macOS
build\bin\cpp_app.exe      # Windows
```

---

## Renaming the Project and Executable

Three files must be updated together. Replace every occurrence of `cpp_app` (underscores)
and `cpp-app` (hyphens) with your chosen name. vcpkg requires hyphens and lowercase;
CMake and the workflow use underscores or whatever casing you prefer.

### 1 — `CMakeLists.txt`

Change the `project()` name, the `add_executable()` target name, and the CPack package
name. Every subsequent CMake command that references the target (`target_link_libraries`,
`target_compile_options`, `target_link_options`, `install`) must use the same new name.

```cmake
# Line 1 – project name (also sets PROJECT_NAME used by CPack below)
project(my_app   ← change this
    VERSION 1.0.0
    ...
)

# Line 2 – executable target name; this becomes the binary filename on disk
add_executable(my_app src/main.cpp)   ← change this

# Lines 3+ – every command that references the target must match
target_link_libraries(my_app PRIVATE Boost::filesystem)   ← change this
target_compile_options(my_app PRIVATE ...)                 ← change this
target_link_options(my_app PRIVATE ...)                    ← change this
install(TARGETS my_app RUNTIME DESTINATION .)              ← change this

# CPack archive name
set(CPACK_PACKAGE_NAME "my_app")   ← change this
```

### 2 — `vcpkg.json`

The `"name"` field must be **all lowercase with hyphens** (no underscores — this is a
vcpkg requirement):

```json
{
  "name": "my-app",
  ...
}
```

### 3 — `.github/workflows/build.yml`

Update the artifact `name:` fields and the `path:` glob patterns so the workflow can
find the archives it just created. There are four build jobs; each has both a `name:`
and a `path:` that reference the old executable name.

```yaml
# In each of the four build jobs – change both fields:
- name: Upload artifact
  uses: actions/upload-artifact@v4
  with:
    name: my_app-windows-x64          ← change (all four jobs)
    path: build/my_app-*.zip          ← change Windows job
    path: build/my_app-*.tar.gz       ← change Linux / macOS jobs
```

After these three edits, rebuild from scratch (`cmake -S . -B build ...`) — CMake will
create a fresh configuration with the new name and the binary in `build/bin/` will have
your chosen filename.

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
