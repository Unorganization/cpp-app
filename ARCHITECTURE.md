# Project Architecture & Design Rationale

This document explains the technical decisions, architectural patterns, and project specifications. It is intended for developers and AI agents who need to understand the "why" behind the structure or reconstruct the repository from scratch.

---

## 1. Project Structure & Manifest

The repository is organized into a modular structure that separates production logic, main entry points, and tests.

```text
cpp-app/
├── .github/workflows/
│   └── build.yml          # Multi-platform CI/CD (Windows, Linux, macOS, Android)
├── data/
│   └── sample.json        # Embedded at link time via llvm-objcopy
├── src/
│   ├── main.cpp           # Demo entry point (16 library/feature sections)
│   ├── math_utils.cpp     # Production logic (addition example)
│   ├── math_utils.h
│   ├── logger.h           # setup_logger() – multi-sink spdlog utility
│   ├── embedded_resource.cpp  # Accesses linker symbols from the embedded object
│   └── embedded_resource.h   # get_embedded_sample_json() API
├── tests/
│   ├── CMakeLists.txt     # Test target configuration
│   └── test_math_utils.cpp # GTest cases for math_utils
├── CMakeLists.txt         # Root build configuration
├── vcpkg.json             # Dependency manifest (Boost, GTest)
├── README.md              # User documentation
├── AGENTS.md              # AI agent guidelines
└── ARCHITECTURE.md        # Technical specification (this file)
```

---

## 2. Dependency Manifest (`vcpkg.json`)

The following libraries are managed via vcpkg in manifest mode:
- **`boost-asio`**: Asynchronous networking and I/O.
- **`boost-beast`**: HTTP and WebSocket library built on Asio.
- **`boost-json`**: JSON parsing and serialization.
- **`boost-process`**: Cross-platform child-process management (Boost.Process v2).
- **`boost-program-options`**: Command-line argument parsing.
- **`boost-stacktrace`**: Call-stack capture at runtime (basic backend; no debug symbols required).
- **`boost-url`**: URI/URL parsing, inspection, and mutation.
- **`boost-uuid`**: UUID generation — random (v4) and name-based/SHA-1 (v5).
- **`csv-parser`**: Vince's CSV parser — reads CSV data from files or strings, accessed by column name.
- **`date`**: Howard Hinnant's calendar date library — fills the C++17 gap for date arithmetic (`year_month_day`, weekday, date math). Core library only; no IANA timezone database required.
- **`gtest`**: Google Test framework for unit testing.
- **`spdlog`**: Structured logging with multiple sinks (coloured stdout and rotating file). Used header-only via `spdlog::spdlog_header_only`.

---

## 3. Logic Isolation & Testability

### The Static Library Pattern
The project is split into two main targets:
1.  **`${PROJECT_NAME}_lib` (STATIC):** Contains all production code (logic, utilities, services).
2.  **`${PROJECT_NAME}` (EXECUTABLE):** A thin wrapper that only contains `main.cpp` and links against the library.

**Rationale:**
- **Unit Testing:** By placing logic in a library, we can link it into both the main executable and the test runner (`${PROJECT_NAME}_tests`). This ensures that the exact same code is being tested as is being shipped.
- **Build Performance:** Static libraries allow CMake to manage dependencies and include paths more efficiently across multiple targets.

---

## 4. Source Code Specifications

### `main.cpp` (Demonstration Suite)
The entry point serves as a comprehensive "kitchen sink" demo of the integrated libraries:
1.  **Boost.ProgramOptions**: Handles `--help`, `--host`, and `--port`.
2.  **Boost.JSON**: Parses raw strings, inspects objects, and serializes responses.
3.  **Boost.Asio**: Implements an asynchronous 200ms steady timer.
4.  **Boost.Beast**: Performs a synchronous HTTP GET request to `example.com`.
5.  **std::filesystem**: Enumerates and labels (DIR/FILE) the current working directory.
6.  **std::regex**: Searches for IP patterns and extracts numbers from text.
7.  **std::thread / std::future**: Executes `math_utils::add` asynchronously via `std::async`.
8.  **Functional Programming**: Demonstrates `std::transform`, `std::copy_if`, and `std::reduce` (C++17).
9.  **Embedded resource**: Reads `data/sample.json` that was baked into the executable at link time via `llvm-objcopy`. Accessed through linker symbols with no file I/O at runtime.
10. **spdlog**: Creates a multi-sink logger (coloured stdout + rotating file). Demonstrates all four severity levels (debug/info/warn/error) with a timestamped, level-tagged pattern. The rotating file sink caps each file at 5 MB and auto-deletes the oldest when more than 3 files accumulate.
11. **Boost.URL**: Parses a URI into components (scheme, host, port, path, query, fragment) via an immutable `url_view`, then mutates a copy using the `url` class.
12. **Boost.UUID**: Generates random (v4) UUIDs and reproducible name-based (v5/SHA-1) UUIDs; demonstrates string serialisation and round-trip parsing.
13. **Boost.Process v2**: Locates `cmake` via `find_executable`, spawns it with `--version`, captures its stdout through a `readable_pipe`, and reports the exit code.
14. **Boost.Stacktrace**: Captures the current call stack and prints up to five frames. Built with the basic backend (raw addresses); comments show how to switch to a symbolising backend per platform.
15. **Howard Hinnant's Date**: Converts the current `system_clock` instant to a `year_month_day`, performs calendar arithmetic (add 7 days, add 1 year), identifies the day of the week, and counts days to the next 1 January.
16. **Vince's CSV Parser**: Parses inline CSV data (name/age/city), enumerates column names, and iterates rows accessing fields by name with typed `get<T>()`.

### `math_utils`
Provides core production logic. Currently contains a basic `add(int, int)` function used to verify the static library linkage across production and test targets.

---

## 5. Toolchain: Clang Everywhere

**Decision:** Enforce Clang on all platforms (Windows: `clang-cl`, Linux: `clang++`, macOS: Apple Clang).

**Rationale:**
- **Consistency:** Reduces "works on my machine" issues where GCC or MSVC might accept code that Clang rejects.
- **Diagnostics:** Clang provides high-quality, readable error messages consistent across operating systems.
- **Cross-Platform Parity:** Ensures language features and optimizations behave similarly on all targets.

---

## 5b. C++ Standard: C++17

**Decision:** The project is built with C++17 (`set(CMAKE_CXX_STANDARD 17)`).

**Rationale:**

C++17 was chosen because it is the highest standard with **complete, verified compiler support across every current target platform**:

| Platform | Compiler | C++17 status | C++20 status |
|---|---|---|---|
| Windows | clang-cl (LLVM/MSVC) | ✅ Full | ⚠️ Mostly complete, some modules gaps |
| Linux | Clang 10+ | ✅ Full | ⚠️ Requires Clang 13+ for full support |
| macOS | Apple Clang (Xcode 10+) | ✅ Full | ⚠️ Requires Xcode 13+ / macOS 12+ |
| Android | NDK r21+ Clang | ✅ Full | ⚠️ Depends on NDK version |
| WebAssembly | Emscripten 2.x+ | ✅ Full | ⚠️ Partial |

**C++17 features actively used in this codebase:**
- `std::filesystem` — directory iteration, path manipulation (demo 5)
- `std::string_view` — zero-copy string references in the embedded resource API
- `std::optional` / `std::variant` — in Boost.URL result types
- Structured bindings (`auto [a, b] = ...`)
- `if constexpr` — compile-time branching
- `std::reduce` — parallel-ready accumulation (demo 8)
- Guaranteed copy elision (NRVO)

**The specific reason NOT to use C++20 yet:**

Howard Hinnant's `date` library (demo 15) fills the C++17 gap for calendar date arithmetic (`year_month_day`, `sys_days`, weekday arithmetic). C++20's `<chrono>` absorbs this functionality — but with **different type names and slightly different semantics**. Upgrading to C++20 would require either removing the `date` library dependency or carefully migrating the demo to `std::chrono` calendar types to avoid symbol conflicts from `using namespace date` + `using namespace std::chrono`.

**When to move to C++20:**

Move to C++20 when:
1. All CI compilers (clang-cl, Clang on Linux, Apple Clang, NDK Clang, Emscripten) fully support the C++20 features you intend to use.
2. The `date` library dependency is removed and demo 15 is rewritten using `std::chrono` calendar types (C++20).
3. Any other library dependency that conflicts with C++20 identifiers is resolved.

**`CMAKE_CXX_EXTENSIONS OFF`:** Compiler-specific extensions (e.g., `__int128` on MSVC) are disabled to maintain portability across all toolchains. Do not re-enable this without testing on all platforms.



**Decision:** Use `target_precompile_headers` for heavy libraries like Boost and STL.

**Rationale:**
- **Speed:** Compiling Boost headers (especially Asio and Beast) is computationally expensive. Precompiling them once per build significantly reduces incremental build times.

---

## 7. Platform-Specific Linker Choices

### Linux: glibc Compatibility
**Decision:** Target **Ubuntu 20.04** in CI and link `libgcc` and `libstdc++` statically.
- **Rationale:** Building on glibc 2.31 ensures the binary remains compatible with newer distributions while providing its own C++ standard library.

### macOS: dead_strip
**Decision:** Pass `LINKER:-dead_strip` and target **macOS 12.0**.
- **Rationale:** Removes unreachable code to minimize binary size. The resulting **x86_64 binary** is also compatible with **Darling**, enabling it to run on Linux environments that support the Darling translation layer.

### Windows: Static CRT (/MT)
**Decision:** Force `/MT` in Release builds and use the `x64-windows-static` vcpkg triplet.
- **Rationale:** Eliminates dependencies on Visual C++ Redistributables for true self-containment.

### Android: Bionic & NDK
**Decision:** Use the Android NDK for cross-compilation and link the C++ standard library statically (`-DANDROID_STL=c++_static`).
- **Rationale:** Targets the Android-native Bionic libc while ensuring all C++ logic is self-contained within the binary, making it portable across Termux environments on ARM64 devices.

---

## 8. Linker-Embedded Binary Resources

**Decision:** Use `llvm-objcopy --input-target binary` to convert arbitrary files (e.g. `data/sample.json`) into native object files at build time, then link them directly into the executable.

**How it works:**
1. CMake's `add_custom_command` invokes `llvm-objcopy` during the build step.
2. `llvm-objcopy` wraps the raw bytes in a platform-native object file (ELF / Mach-O / COFF) and synthesises three linker symbols derived from the source filename:
   - `_binary_sample_json_start` — pointer to the first byte
   - `_binary_sample_json_end` — pointer one past the last byte
   - `_binary_sample_json_size` — byte count as a linker symbol
3. `embedded_resource.cpp` declares these as `extern "C"` symbols and exposes them through `get_embedded_sample_json()`, which returns a `std::string_view` directly into the executable's read-only data segment.

**Why `llvm-objcopy` and not alternatives:**
- **Not source-code encoding** (e.g. hex arrays, base64): no round-trip conversion; the raw bytes are always in sync with the source file.
- **Not GNU ld `--format=binary`**: that is a GNU ld extension unsupported by Apple `ld` and MSVC `link.exe`.
- **Not assembly `.incbin`**: would require platform-specific assembly syntax and a separate `.S` file.
- **`llvm-objcopy` is cross-platform**: the same tool and the same `extern "C"` declaration work on ELF (Linux/Android), Mach-O (macOS), and COFF x64 (Windows) because `llvm-objcopy` follows each format's C-linkage naming convention automatically.

**Symbol placement:** `embedded_resource.cpp` and the generated object are added to the **executable** target directly (not to `${PROJECT_NAME}_lib`) so the test runner, which also links the library, does not encounter unresolved symbols for data it never uses.

**Tool availability per platform:**

| Platform | Source of `llvm-objcopy` |
|---|---|
| Linux | `sudo apt-get install llvm` |
| macOS | `brew install llvm` (pre-installed on GitHub-hosted runners) |
| Windows | Ships with Visual Studio "C++ Clang tools for Windows" |
| Android NDK CI | Bundled in the NDK at `$ANDROID_NDK_LATEST_HOME/toolchains/llvm/prebuilt/.../bin/` |
| Termux (on-device) | `pkg install llvm` |

### WebAssembly / Emscripten
**Decision:** Add a `wasm32-emscripten` build target using the Emscripten SDK, producing a self-contained `.html` + `.js` + `.wasm` bundle that runs in any modern browser.

**What works / what doesn't:**
- Demos 1–3, 5–6, 8–12, 15–16 run identically in the browser.
- Demos 4 (Beast HTTP), 7 (threads), 13 (Process v2), and 14 (Stacktrace) are disabled via `#ifndef __EMSCRIPTEN__` because the browser sandbox has no raw TCP sockets, no `fork`/`exec`, and no stack-unwinding support.
- The Boost.Asio timer (demo 3) works because `-sASYNCIFY` allows `ioc.run()` to suspend cooperatively instead of blocking the browser's event loop.
- The embedded resource (demo 9) uses Emscripten's `--preload-file` to package `data/sample.json` into the virtual filesystem instead of the llvm-objcopy linker-symbol approach, which doesn't apply to wasm objects.

**vcpkg:** `boost-beast`, `boost-process`, and `boost-stacktrace` are excluded from the `wasm32-emscripten` build via `"platform": "!wasm32"` in `vcpkg.json`.

**CI testing:** Emscripten output also runs under Node.js, so `node cpp_app.js` serves as the test step — no browser or emulator required.

---

## 9. CI/CD Pipeline Blueprint (GitHub Actions)

The workflow (`build.yml`) automates the following:
1.  **Multi-Platform Build Matrix**:
    - **Windows**: Uses `Visual Studio 17 2022` with the `ClangCL` toolset.
    - **Linux**: Uses `ubuntu-20.04` with `Ninja` and `clang++`.
    - **macOS (Intel & Silicon)**: Uses `macos-13` (x86_64) and `macos-14` (arm64) runners.
    - **Android**: Uses `ubuntu-latest` with the Android NDK to cross-compile for `arm64-v8a`.
    - **WebAssembly**: Uses `ubuntu-latest` with the Emscripten SDK to compile for `wasm32-emscripten`. Output is a `.html` + `.js` + `.wasm` bundle. Tested in CI via Node.js (no browser required).
2.  **Automated Testing**: Runs `ctest` on every push and pull request.
3.  **Packaging**: Invokes `cpack` to create `.zip` (Windows) and `.tar.gz` (Unix) archives.
4.  **Release Automation**: Automatically creates a GitHub Release and uploads all four archives when a `v*.*.*` tag is pushed.
