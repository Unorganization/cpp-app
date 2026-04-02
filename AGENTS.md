# AI Agent Guidelines for cpp_app

This file exists specifically to inform AI coding assistants (Claude, Copilot, Cursor,
ChatGPT, etc.) of the non-negotiable design principles for this project.

## ⚠️ MANDATORY: Update Documentation With Every Change

**This is not optional. There are no exceptions.**

Any change to source code, `CMakeLists.txt`, `vcpkg.json`, GitHub Actions workflows,
or any other file in the repo triggers this rule. The following documentation files
must be checked and updated as needed:

- **`README.md`**
- **`AGENTS.md`**
- **`ARCHITECTURE.md`**

Do not wait to be asked. Do not skip this step. A commit that changes behaviour
without updating the relevant documentation is incomplete.

### Pre-commit documentation checklist

Before every commit, run through the following mentally:

- [ ] Does `README.md` still accurately describe the build steps, supported platforms,
      and minimum versions?
- [ ] Does `README.md` have accurate unit test instructions for every platform touched?
- [ ] Does `ARCHITECTURE.md` still accurately describe the libraries, CI jobs, and
      design decisions?
- [ ] Does `AGENTS.md` reflect any new conventions or constraints introduced by this
      change?

---

## Core Principle: Maximum Binary Self-Containment

The single most important rule in this codebase is that **every release binary must be
as self-contained as possible**. See [`ARCHITECTURE.md`](ARCHITECTURE.md) for the
detailed technical rationale behind this principle.

End-users should be able to download a release archive,
extract it, and run the executable without installing any additional software, libraries,
or runtimes.

**Never weaken this principle** when adding features, libraries, or build changes.

---

## What Self-Contained Means Per Platform

### Windows (x64, MSVC)

- The C runtime is linked **statically** (`/MT`). The `/MD` flag (dynamic CRT) must
  never be used in release builds. This means no dependency on `MSVCP140.dll`,
  `VCRUNTIME140.dll`, or any Visual C++ Redistributable.
- All third-party libraries (e.g. Boost) must be built with the matching
  `x64-windows-static` vcpkg triplet. Using the default `x64-windows` triplet produces
  DLL dependencies and breaks the self-containment guarantee.
- The workflow passes `-DVCPKG_TARGET_TRIPLET=x64-windows-static` explicitly — do not
  remove this flag.

### Linux (x64, GCC)

- `libgcc` and `libstdc++` are linked **statically** via `-static-libgcc
  -static-libstdc++`. This means the binary runs on any glibc-based distro
  (Ubuntu 20.04+, Debian 11+, equivalents) without the user needing to install those
  packages separately.
- `glibc` itself remains **dynamic**. Do not attempt to link it statically — a
  statically linked glibc is not safe to redistribute and the dynamic ABI is stable and
  backward-compatible across all distributions we target.
- Do not add new `-shared` or `-Wl,-rpath` link flags that would introduce runtime
  library search paths.

### macOS (x86_64 and arm64)

- The binary relies on **only system-provided dylibs** (`libSystem`, `libc++`). These
  are present on every supported macOS version and do not need to be bundled.
- Do not link against Homebrew-installed shared libraries (`/usr/local/lib` or
  `/opt/homebrew/lib`). Such libraries are not present on users' machines and will
  cause a `dyld` error at launch.
- `-dead_strip` is passed to the linker to remove unused code — keep this flag.
- The deployment target is set to `macOS 12.0` (`-DCMAKE_OSX_DEPLOYMENT_TARGET=12.0`).
  Do not lower this without careful testing, and do not raise it without updating the
  README's supported-platforms table.

---

## Dependency Management Rules

### Adding a new library

Before adding any dependency, ask: *can this be avoided with the C++17 standard library
or Boost components already present?*

If a new library is genuinely necessary:

1. **Prefer header-only libraries** — they introduce no additional linking complexity.
2. **Use vcpkg** to manage it (add to `vcpkg.json`). Do not use system package managers
   (`apt`, `brew`, `choco`) inside the build — those libraries are not present on
   users' machines when they download a release binary.
3. **Verify static availability per platform.** Check that the vcpkg port supports
   static linking on all three platforms. If it only provides a shared library, look
   for an alternative.
4. **On Windows**, use the `x64-windows-static` triplet for every new library. If the
   library does not support static linking on Windows, it cannot be added without
   breaking the self-containment guarantee — raise this as a design question before
   proceeding.
5. **On macOS**, if a port links against a Homebrew dependency under
   `/usr/local` or `/opt/homebrew`, it is not safe to use without bundling or replacing
   that dependency.

### Removing a library

When removing a library, remove it from **both** `vcpkg.json` and the `find_package` /
`target_link_libraries` lines in `CMakeLists.txt`. Leaving stale entries in either file
causes build failures.

---

## Build System Rules

- **CMake minimum version is 3.20.** Do not lower it.
- **C++ standard is C++17.** Do not lower it. Raising to C++20 or later is acceptable
  if all platforms support it, but verify the GitHub Actions compiler versions first.
- **`CMAKE_CXX_EXTENSIONS` is `OFF`** (disables compiler-specific extensions like
  `__int128` on MSVC). Keep it off to maintain portability.
- The vcpkg triplet for Windows (`VCPKG_TARGET_TRIPLET`) must be set **before** the
  `project()` call in `CMakeLists.txt` so the toolchain picks it up correctly. Do not
  move it below `project()`.
- CPack packaging is configured at the bottom of `CMakeLists.txt`. The workflow calls
  `cpack` directly (not via `cmake --build ... --target package` on all platforms) to
  produce the release archives. Do not change the archive format without updating the
  `upload-artifact` `path:` globs in the workflow.
- **`${PROJECT_NAME}` is the single source of truth for the executable name.** Every
  CMake command after `project()` — `add_executable`, `target_link_libraries`,
  `target_compile_options`, `target_link_options`, `install`, and
  `CPACK_PACKAGE_NAME` — uses `${PROJECT_NAME}`. Do not replace these references with
  a hardcoded string. The GitHub Actions workflow uses broad globs (`build/*.zip`,
  `build/*.tar.gz`) intentionally, so it stays correct after a rename without any
  workflow edits. Do not narrow those globs back to include a hardcoded name.

---

## Compiler Requirements

**Clang is required on all platforms.** Do not switch any platform back to a different
compiler without an explicit decision from the project owner.

| Platform | Compiler | How it is selected |
|---|---|---|
| Windows | clang-cl (LLVM frontend, MSVC-compatible) | `-T ClangCL` in the CMake configure step |
| Linux | Clang (`clang` / `clang++`) | `-DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++` |
| macOS | Apple Clang | Default toolchain on the GitHub runner — no flag needed |

On Windows, clang-cl uses the MSVC linker and runtime, so `/MT` (static CRT) and the
`x64-windows-static` vcpkg triplet continue to apply unchanged. In CMake, `MSVC` is
`TRUE` when clang-cl is active — this is expected and correct; the existing `/MT`
replacement logic relies on it.

On Linux, `build-essential` is still installed alongside `clang` because it provides
supporting tools (`ar`, `ranlib`, etc.) that the Clang toolchain uses. The
`-static-libgcc` and `-static-libstdc++` link flags work with Clang on Linux exactly
as they do with GCC.

---

## GitHub Actions Workflow Rules

- All four build jobs (Windows, Linux, macOS x86_64, macOS arm64) must remain in sync
  — if you add a CMake flag or vcpkg setting to one job, add it to all of them unless
  there is a specific platform reason not to.
- vcpkg is provided by the GitHub-hosted runner via `$VCPKG_INSTALLATION_ROOT`. The
  "Setup vcpkg" step in each job exposes this as `$VCPKG_ROOT` and passes it to CMake
  via `-DCMAKE_TOOLCHAIN_FILE`. Do not replace this with a `git clone` of vcpkg unless
  you have a specific version-pinning requirement — the pre-installed version is kept
  up to date by GitHub.
- The release job only runs on `v*.*.*` tags. Do not change the trigger condition
  without considering the impact on the release workflow.
- Do not add steps that upload debug symbols, intermediate object files, or build logs
  as release assets — only the final packaged archives should appear in a release.

---

## What Not to Do

- Do not add a runtime installer or an "install dependencies" step to the user-facing
  README — the whole point is that no such step is needed.
- Do not link against Qt, wxWidgets, or any other framework that requires the user to
  install a separate runtime.
- Do not use `find_package` with `REQUIRED` for system-installed packages (e.g.
  `find_package(OpenSSL REQUIRED)` without vcpkg) — system packages are not reliably
  present across all target OS versions.
- Do not change the deployment target or supported-platforms table without updating
  both the README and this file.
