# Project Architecture & Design Rationale

This document explains the technical decisions and architectural patterns used in this project. It is intended for developers and AI agents who need to understand the "why" behind the project's structure.

---

## 1. Logic Isolation & Testability

### The Static Library Pattern
The project is split into two main targets:
1.  **`${PROJECT_NAME}_lib` (STATIC):** Contains all production code (logic, utilities, services).
2.  **`${PROJECT_NAME}` (EXECUTABLE):** A thin wrapper that only contains `main.cpp` and links against the library.

**Rationale:**
- **Unit Testing:** By placing logic in a library, we can link it into both the main executable and the test runner (`${PROJECT_NAME}_tests`). This ensures that the exact same code is being tested as is being shipped.
- **Build Performance:** Static libraries allow CMake to manage dependencies and include paths more efficiently across multiple targets.

---

## 2. Toolchain: Clang Everywhere

**Decision:** Enforce Clang on all platforms (Windows: `clang-cl`, Linux: `clang++`, macOS: Apple Clang).

**Rationale:**
- **Consistency:** Reduces "works on my machine" issues where GCC or MSVC might accept code that Clang rejects (or vice versa).
- **Diagnostics:** Clang provides high-quality, readable error messages and warnings that are consistent across operating systems.
- **Cross-Platform Parity:** Using the same compiler frontend ensures that language features and optimizations behave similarly on all targets.

---

## 3. Dependency Management: vcpkg Manifest Mode

**Decision:** Use `vcpkg.json` (manifest mode) instead of manual library installation.

**Rationale:**
- **Reproducibility:** Every developer and CI runner uses the exact same versions of libraries.
- **Zero-Setup CI:** GitHub Actions can automatically restore cached dependencies, speeding up builds.
- **Static Linking:** vcpkg makes it easy to request static triplets (e.g., `x64-windows-static`), which is central to our [Self-Containment Principle](AGENTS.md).

---

## 4. Build Performance: Precompiled Headers (PCH)

**Decision:** Use `target_precompile_headers` for heavy libraries like Boost and STL.

**Rationale:**
- **Speed:** Compiling Boost headers (especially Asio and Beast) is computationally expensive. Precompiling them once per build significantly reduces incremental build times.
- **Developer Experience:** Faster builds lead to faster iteration cycles.

---

## 5. Platform-Specific Linker Choices

### Linux: glibc Compatibility
**Decision:** Target Ubuntu 20.04 in CI and link `libgcc` and `libstdc++` statically.

**Rationale:**
- **Forward Compatibility:** By building on an older glibc (2.31), the binary remains compatible with newer distributions.
- **Portability:** Statically linking the C++ standard library ensures the binary runs on distros that might have a different version of `libstdc++`.

### macOS: dead_strip
**Decision:** Pass `LINKER:-dead_strip`.

**Rationale:**
- **Binary Size:** Removes unreachable code from the final binary, keeping the distribution small.

### Windows: Static CRT (/MT)
**Decision:** Force `/MT` in Release builds.

**Rationale:**
- **Self-Containment:** Removes the dependency on `MSVCP140.dll` and other Visual C++ Redistributables, allowing the app to run on a "clean" Windows installation.

---

## 6. Testing Strategy: Google Test

**Decision:** Use GTest with `gtest_discover_tests`.

**Rationale:**
- **Integration:** `gtest_discover_tests` allows CTest to see individual test cases, enabling fine-grained test execution and reporting.
- **Main-less Tests:** Linking against `GTest::gtest_main` removes the need to write a boilerplate `main()` function for the test runner.
