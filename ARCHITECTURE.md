# Project Architecture & Design Rationale

This document explains the technical decisions, architectural patterns, and project specifications. It is intended for developers and AI agents who need to understand the "why" behind the structure or reconstruct the repository from scratch.

---

## 1. Project Structure & Manifest

The repository is organized into a modular structure that separates production logic, main entry points, and tests.

```text
cpp-app/
├── .github/workflows/
│   └── build.yml          # Multi-platform CI/CD (Windows, Linux, macOS)
├── src/
│   ├── main.cpp           # Demo entry point (8 library sections)
│   ├── math_utils.cpp     # Production logic (addition example)
│   └── math_utils.h
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
- **`boost-program-options`**: Command-line argument parsing.
- **`gtest`**: Google Test framework for unit testing.

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

## 6. Build Performance: Precompiled Headers (PCH)

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

## 8. CI/CD Pipeline Blueprint (GitHub Actions)

The workflow (`build.yml`) automates the following:
1.  **Multi-Platform Build Matrix**:
    - **Windows**: Uses `Visual Studio 17 2022` with the `ClangCL` toolset.
    - **Linux**: Uses `ubuntu-20.04` with `Ninja` and `clang++`.
    - **macOS (Intel & Silicon)**: Uses `macos-13` (x86_64) and `macos-14` (arm64) runners.
    - **Android**: Uses `ubuntu-latest` with the Android NDK to cross-compile for `arm64-v8a`.
2.  **Automated Testing**: Runs `ctest` on every push and pull request.
3.  **Packaging**: Invokes `cpack` to create `.zip` (Windows) and `.tar.gz` (Unix) archives.
4.  **Release Automation**: Automatically creates a GitHub Release and uploads all four archives when a `v*.*.*` tag is pushed.
