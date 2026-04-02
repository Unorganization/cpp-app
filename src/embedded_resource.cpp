#include "embedded_resource.h"

#include <cstddef>

using namespace std;

#ifdef __EMSCRIPTEN__

// ---------------------------------------------------------------------------
// WebAssembly / Emscripten build
//
// llvm-objcopy cannot produce a wasm-compatible object file, so the native
// linker-symbol approach is unavailable here.  Instead, CMakeLists.txt passes
//   --preload-file <src>/data@/data
// to the Emscripten linker, which packages the entire data/ directory into the
// generated .data bundle and mounts it at /data/ inside the virtual filesystem
// at startup.  Standard POSIX I/O can then read /data/sample.json normally.
// ---------------------------------------------------------------------------

#include <fstream>
#include <sstream>
#include <stdexcept>

string_view get_embedded_sample_json() {
    static const string cached = []() {
        ifstream f("/data/sample.json");
        if (!f) {
            throw runtime_error(
                "embedded resource: /data/sample.json not found in virtual FS; "
                "ensure --preload-file was passed to the Emscripten linker");
        }
        ostringstream ss;
        ss << f.rdbuf();
        return ss.str();
    }();
    return cached;
}

#else

// ---------------------------------------------------------------------------
// Native build (Linux, macOS, Windows, Android)
//
// llvm-objcopy --input-target binary generates three linker symbols from the
// source filename ("sample.json" → characters mapped to valid identifier chars):
//
//   _binary_sample_json_start   pointer to the first byte
//   _binary_sample_json_end     pointer one past the last byte
//   _binary_sample_json_size    linker symbol whose address equals the byte count
//                               (not a size_t; use pointer arithmetic instead)
//
// How the name appears in each object format:
//   ELF   (Linux, Android) : stored as "_binary_sample_json_start"; C extern
//                             declaration resolves to the same name directly.
//   Mach-O (macOS)         : llvm-objcopy adds the C-linkage '_' prefix, so the
//                             object file holds "__binary_sample_json_start".
//                             The compiler's own '_' prefix on the extern "C"
//                             declaration then resolves it correctly.
//   COFF x64 (Windows)     : 64-bit PE has no leading-underscore convention;
//                             symbol stored as "_binary_sample_json_start" and
//                             the declaration resolves to it directly.
//
// Result: the same extern "C" declaration compiles and links on all platforms.

extern "C" {
    extern const char _binary_sample_json_start[];
    extern const char _binary_sample_json_end[];
}

string_view get_embedded_sample_json() {
    return {
        _binary_sample_json_start,
        static_cast<size_t>(
            _binary_sample_json_end - _binary_sample_json_start)
    };
}

#endif
