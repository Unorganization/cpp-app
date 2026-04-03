// =============================================================================
// main.cpp – demonstrates each supported library with a minimal working sample.
//
// Sections:
//   1. CLI11                  – command-line argument parsing
//   2. nlohmann/json          – parse, inspect, and serialize JSON
//   3. Asio (standalone)      – async timer (event-loop / io_context pattern)
//   4. cpp-httplib            – synchronous HTTP GET over plain TCP
//   5. filesystem        – enumerate files in the working directory
//   6. regex             – search for an IP address pattern in a string
//   7. thread / future   – background task with async
//   8. FP (transform /   – map, filter, reduce over a vector
//          copy_if /
//          reduce)
//   9. Embedded resource      – binary file linked into the executable at build
//                               time via llvm-objcopy; accessed through linker
//                               symbols with no file I/O at runtime
//  10. spdlog                 – structured logging with coloured stdout sink
//                               and a size-based rotating file sink; oldest
//                               files deleted automatically when limit is reached
//  11. ada-url                – parse, inspect, and mutate URIs
//  12. stduuid                – generate random (v4) and name-based (v5) UUIDs
//  13. reproc                 – launch a child process, capture its stdout
//  14. cpptrace               – capture and print the current call stack
//  15. Howard Hinnant's Date  – calendar date arithmetic on top of <chrono>
//                               (core library only; no IANA timezone data needed)
//  16. Vince's CSV Parser     – parse CSV data, access fields by column name
// =============================================================================

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <future>
#include <iostream>
#include <numeric>
#include <regex>
#include <string>
#include <thread>
#include <vector>

#include <asio.hpp>
#include <nlohmann/json.hpp>
#include <CLI/CLI.hpp>
#include <ada.h>
#include <uuid.h>
#include <date/date.h>
#include <csv.hpp>

// These libraries require OS capabilities absent from the browser sandbox.
#ifndef __EMSCRIPTEN__
#include <httplib.h>
#include <reproc++/reproc.hpp>
#include <reproc++/drain.hpp>
#include <cpptrace/cpptrace.hpp>
#endif

#include "math_utils.h"
#include "embedded_resource.h"
#include "logger.h"

using namespace std;

namespace fs  = filesystem;
namespace net = asio;
using     tcp = net::ip::tcp;
using     json = nlohmann::json;

// -----------------------------------------------------------------------------
// 1. CLI11 – command-line argument parsing
// -----------------------------------------------------------------------------
static void parse_options(int argc, char* argv[], string& host, int& port) {
    CLI::App app{"cpp_app"};
    app.add_option("--host", host, "server hostname")->default_val("localhost");
    app.add_option("--port,-p", port, "server port")->default_val(8080);

    try {
        app.parse(argc, argv);
    } catch (const CLI::ParseError& e) {
        exit(app.exit(e));  // --help exits 0; other errors exit non-zero
    }
}

// -----------------------------------------------------------------------------
// 2. nlohmann/json – parse, inspect, and serialize JSON
// -----------------------------------------------------------------------------
static void demo_json() {
    cout << "\n--- nlohmann/json ---\n";

    // Parse
    const string raw = R"({
        "service": "example-api",
        "version": 3,
        "features": ["async", "json", "http"]
    })";
    const json doc = json::parse(raw);

    cout << "service : " << doc["service"].get<string>() << "\n";
    cout << "version : " << doc["version"].get<int>()    << "\n";
    cout << "features: ";
    for (const auto& f : doc["features"]) {
        cout << f.get<string>() << " ";
    }
    cout << "\n";

    // Serialize
    json response;
    response["status"] = "ok";
    response["result"] = add(6, 7);     // calls production code from math_utils
    cout << "serialized: " << response.dump() << "\n";
}

// -----------------------------------------------------------------------------
// 3. Asio (standalone) – async timer
// -----------------------------------------------------------------------------
static void demo_asio_timer() {
    cout << "\n--- Asio (standalone): async timer ---\n";

    net::io_context ioc;
    net::steady_timer timer(ioc, chrono::milliseconds(200));

    timer.async_wait([](const asio::error_code& ec) {
        if (!ec) {
            cout << "timer fired (200 ms)\n";
        }
    });

    // Run the event loop until all async work is complete
    ioc.run();
}

// -----------------------------------------------------------------------------
// 4. cpp-httplib – synchronous HTTP GET  (skipped on WebAssembly: no raw TCP)
// -----------------------------------------------------------------------------
#ifndef __EMSCRIPTEN__
static void demo_http() {
    cout << "\n--- cpp-httplib: HTTP GET example.com ---\n";

    httplib::Client cli("http://example.com");
    cli.set_connection_timeout(5);
    cli.set_read_timeout(5);

    const auto res = cli.Get("/");
    if (res) {
        cout << "status : " << res->status << "\n";
        cout << "body   : " << res->body.size() << " bytes\n";
    } else {
        cout << "skipped (no network): "
             << httplib::to_string(res.error()) << "\n";
    }
}
#endif // !__EMSCRIPTEN__

// -----------------------------------------------------------------------------
// 5. filesystem
// -----------------------------------------------------------------------------
static void demo_filesystem() {
    cout << "\n--- filesystem ---\n";

    const fs::path cwd = fs::current_path();
    cout << "working directory: " << cwd << "\n";
    cout << "contents:\n";

    error_code ec;
    for (const auto& entry : fs::directory_iterator(cwd, ec)) {
        const char* kind = entry.is_directory() ? "DIR " : "FILE";
        cout << "  [" << kind << "] "
                  << entry.path().filename().string() << "\n";
    }
}

// -----------------------------------------------------------------------------
// 6. regex
// -----------------------------------------------------------------------------
static void demo_regex() {
    cout << "\n--- regex ---\n";

    const string text = "connect to 192.168.1.100 on port 8080";
    const regex  ip_pattern(R"(\d{1,3}(?:\.\d{1,3}){3})");

    smatch m;
    if (regex_search(text, m, ip_pattern)) {
        cout << "found IP address: " << m[0] << "\n";
    }

    // Iterate over all matches
    auto begin = sregex_iterator(text.begin(), text.end(),
                                      regex(R"(\d+)"));
    auto end   = sregex_iterator{};
    cout << "all numbers in text: ";
    for (auto it = begin; it != end; ++it) {
        cout << (*it)[0] << " ";
    }
    cout << "\n";
}

// -----------------------------------------------------------------------------
// 7. thread / future  (skipped on WebAssembly: no pthreads by default)
// -----------------------------------------------------------------------------
#ifndef __EMSCRIPTEN__
static void demo_thread_future() {
    cout << "\n--- thread / future ---\n";

    // async fires the lambda on a thread-pool thread
    auto fut = async(launch::async, []() -> int {
        this_thread::sleep_for(chrono::milliseconds(50));
        return add(100, 200);   // calls production code
    });

    // Do other work here while the async task runs ...
    cout << "waiting for async result...\n";

    cout << "async result: " << fut.get() << "\n";

    // Explicit thread example
    thread t([]() {
        cout << "background thread id: "
                  << this_thread::get_id() << "\n";
    });
    t.join();
}
#endif // !__EMSCRIPTEN__

// -----------------------------------------------------------------------------
// 8. FP – map / filter / reduce
// -----------------------------------------------------------------------------
static void demo_fp() {
    cout << "\n--- FP: map / filter / reduce ---\n";

    const vector<int> numbers = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

    // map: square every element
    vector<int> squared;
    squared.reserve(numbers.size());
    transform(numbers.begin(), numbers.end(),
                   back_inserter(squared),
                   [](int n) { return n * n; });

    cout << "squared: ";
    for (int n : squared) { cout << n << " "; }
    cout << "\n";

    // filter: keep only even numbers
    vector<int> evens;
    copy_if(numbers.begin(), numbers.end(),
                 back_inserter(evens),
                 [](int n) { return n % 2 == 0; });

    cout << "evens  : ";
    for (int n : evens) { cout << n << " "; }
    cout << "\n";

    // reduce: sum all elements (reduce is C++17, supports parallel policy)
    const int total = reduce(numbers.begin(), numbers.end(), 0);
    cout << "sum    : " << total << "\n";

    // chained: sum of squares of even numbers
    vector<int> even_squares;
    copy_if(squared.begin(), squared.end(),
                 back_inserter(even_squares),
                 [](int n) {
                     const int root = static_cast<int>(sqrt(n));
                     return root * root == n && root % 2 == 0;
                 });
    const int even_square_sum = reduce(even_squares.begin(),
                                            even_squares.end(), 0);
    cout << "sum of squares of evens: " << even_square_sum << "\n";
}

// -----------------------------------------------------------------------------
// 9. Embedded resource – binary file linked in at build time via llvm-objcopy
// -----------------------------------------------------------------------------
static void demo_embedded_resource() {
    cout << "\n--- Embedded resource (llvm-objcopy, no file I/O) ---\n";

    // get_embedded_sample_json() returns a string_view directly into the
    // executable's read-only data segment – no heap allocation, no file open.
    const string_view raw = get_embedded_sample_json();
    cout << "embedded size  : " << raw.size() << " bytes\n";

    // Parse the embedded bytes as JSON using nlohmann/json (already linked in).
    const json doc = json::parse(raw.begin(), raw.end());

    cout << "application    : " << doc["application"].get<string>() << "\n";
    cout << "description    : " << doc["description"].get<string>() << "\n";

    cout << "features       : ";
    for (const auto& f : doc["settings"]["features"]) {
        cout << f.get<string>() << " ";
    }
    cout << "\n";
}

// -----------------------------------------------------------------------------
// 10. spdlog – structured logging with multiple sinks and log rotation
// -----------------------------------------------------------------------------
static void demo_spdlog() {
    cout << "\n--- spdlog: structured logging + rotating file sink ---\n";

    // Write logs to a temp subdirectory so the demo works from any cwd.
    const fs::path log_dir = fs::temp_directory_path() / "cpp_app_demo_logs";
    auto log = setup_logger(log_dir);

    cout << "log directory  : " << log_dir.string() << "\n";
    cout << "rotation policy: 5 MB max per file, 3 files kept\n\n";

    // All four severity levels.  The pattern written to both sinks is:
    //   [2026-04-02 09:00:00.123] [INFO ]  message
    log->debug("debug: detailed diagnostic – usually disabled in production");
    log->info ("info:  server listening on port {}", 8080);
    log->warn ("warn:  disk usage at {}%, consider archiving old data", 87);
    log->error("error: failed to open config file, falling back to defaults");

    log->flush();
    spdlog::drop("app");  // deregister so the demo can be re-run in the same process
}

// -----------------------------------------------------------------------------
// 11. ada-url – parse, inspect, and mutate URIs (WHATWG-compliant)
// -----------------------------------------------------------------------------
static void demo_url() {
    cout << "\n--- ada-url ---\n";

    // Parse to a mutable url object
    auto result = ada::parse(
        "https://api.example.com:8443/v2/users?page=1&limit=50#results");
    if (!result) {
        cout << "parse failed\n";
        return;
    }

    // get_protocol() includes the trailing colon ("https:")
    // get_search()   includes the leading "?" ("?page=1&limit=50")
    // get_hash()     includes the leading "#" ("#results")
    cout << "scheme   : " << result->get_protocol() << "\n";
    cout << "host     : " << result->get_hostname()  << "\n";
    cout << "port     : " << result->get_port()      << "\n";
    cout << "path     : " << result->get_pathname()  << "\n";
    cout << "query    : " << result->get_search()    << "\n";
    cout << "fragment : " << result->get_hash()      << "\n";

    // Mutate in place
    result->set_pathname("/v3/users");
    result->set_search("page=2&limit=25");
    cout << "modified : " << result->get_href() << "\n";
}

// -----------------------------------------------------------------------------
// 12. stduuid – random (v4) and name-based (v5) UUIDs
// -----------------------------------------------------------------------------
static void demo_uuid() {
    cout << "\n--- stduuid ---\n";

    // v4: uses OS-native source (CoCreateGuid on Windows, uuid_generate on
    // Linux/macOS). This works reliably on all supported platforms including
    // Windows XP x64 and Vista.
    uuids::uuid_system_generator sys_gen;
    const auto id1 = sys_gen();
    const auto id2 = sys_gen();
    cout << "random uuid 1  : " << uuids::to_string(id1) << "\n";
    cout << "random uuid 2  : " << uuids::to_string(id2) << "\n";
    cout << "are equal      : " << (id1 == id2 ? "yes" : "no") << "\n";

    // v5: name-based (SHA-1) – identical input always yields identical UUID
    uuids::uuid_name_generator name_gen(uuids::uuid_namespace_url);
    const auto id3 = name_gen("https://example.com");
    const auto id4 = name_gen("https://example.com");
    cout << "name-based     : " << uuids::to_string(id3) << "\n";
    cout << "reproducible   : " << (id3 == id4 ? "yes" : "no") << "\n";

    // String round-trip
    const string str = uuids::to_string(id1);
    const auto   parsed = uuids::uuid::from_string(str);
    cout << "string form    : " << str << "\n";
    cout << "round-trip ok  : " << (parsed && *parsed == id1 ? "yes" : "no") << "\n";
}

// -----------------------------------------------------------------------------
// 13. reproc  (skipped on WebAssembly: no process spawning)
// -----------------------------------------------------------------------------
#ifndef __EMSCRIPTEN__
static void demo_process() {
    cout << "\n--- reproc: child process ---\n";

    // cmake is guaranteed to be on PATH on any machine that can build this
    // project, making it a reliable demo target.
    reproc::options options;
    options.redirect.out.type = reproc::redirect::pipe;

    reproc::process proc;
    auto ec = proc.start({"cmake", "--version"}, options);
    if (ec) {
        cout << "cmake not found in PATH; skipping (" << ec.message() << ")\n";
        return;
    }

    string output;
    reproc::drain(proc, reproc::sink::string(output), reproc::sink::null);

    auto [exit_code, wait_ec] = proc.wait(reproc::infinite);

    // Print only the first line ("cmake version X.Y.Z")
    const auto nl = output.find('\n');
    cout << "output         : "
         << output.substr(0, nl != string::npos ? nl : output.size()) << "\n";
    cout << "exit code      : " << exit_code << "\n";
}
#endif // !__EMSCRIPTEN__

// -----------------------------------------------------------------------------
// 14. cpptrace  (skipped on WebAssembly: stack unwinding not supported)
// -----------------------------------------------------------------------------
#ifndef __EMSCRIPTEN__
static void demo_stacktrace() {
    cout << "\n--- cpptrace ---\n";

    // Captures the current call stack. Symbol resolution depends on the
    // platform and build type:
    //   Debug builds   : full function names and line numbers
    //   Release builds : addresses only (or names if the binary is not stripped)
    const auto trace = cpptrace::generate_trace();
    cout << "frames captured: " << trace.frames.size() << "\n";

    const size_t show = min(trace.frames.size(), size_t{5});
    for (size_t i = 0; i < show; ++i) {
        cout << "  [" << i << "] " << trace.frames[i].symbol << "\n";
    }
    if (trace.frames.size() > show) {
        cout << "  ... (" << (trace.frames.size() - show) << " more frames)\n";
    }
}
#endif // !__EMSCRIPTEN__

// -----------------------------------------------------------------------------
// 15. Howard Hinnant's Date library – calendar arithmetic on top of <chrono>
//     (core library only; no IANA timezone database required)
// -----------------------------------------------------------------------------
static void demo_date() {
    cout << "\n--- Howard Hinnant's Date library ---\n";
    using namespace date;
    using namespace std::chrono;

    // today as a calendar date
    const auto today = floor<days>(system_clock::now());
    const auto ymd   = year_month_day{today};

    cout << "today          : " << ymd << "\n";
    cout << "year           : " << static_cast<int>(ymd.year())      << "\n";
    cout << "month          : " << static_cast<unsigned>(ymd.month()) << "\n";
    cout << "day            : " << static_cast<unsigned>(ymd.day())   << "\n";
    cout << "day of week    : " << weekday{today} << "\n";

    // arithmetic
    cout << "in 7 days      : " << year_month_day{sys_days{today} + days{7}}  << "\n";
    cout << "in 1 year      : " << (ymd.year() + years{1}) / ymd.month() / ymd.day() << "\n";

    // days remaining until 1 Jan next year
    const auto jan1_next  = (ymd.year() + years{1}) / January / 1;
    const auto days_left  = (sys_days{jan1_next} - sys_days{today}).count();
    cout << "days to Jan 1  : " << days_left << "\n";
}

// -----------------------------------------------------------------------------
// 16. Vince's CSV Parser – parse CSV, access fields by column name
// -----------------------------------------------------------------------------
static void demo_csv() {
    cout << "\n--- Vince's CSV Parser ---\n";

    // Inline data keeps the demo self-contained (no file path dependency).
    // To read a file instead: csv::CSVReader reader("path/to/file.csv");
    const string csv_data =
        "name,age,city\n"
        "Alice,30,New York\n"
        "Bob,25,London\n"
        "Charlie,35,Tokyo\n";

    auto reader = csv::parse(csv_data);

    cout << "columns        : ";
    for (const auto& col : reader.get_col_names()) {
        cout << col << "  ";
    }
    cout << "\n";

    for (auto& row : reader) {
        cout << "  "
                  << row["name"].get<string>() << ", "
                  << "age " << row["age"].get<int>() << ", "
                  << row["city"].get<string>()  << "\n";
    }
}

// =============================================================================
// main
// =============================================================================
int main(int argc, char* argv[]) {
    cout << "=== Cross-platform C++ app ===\n";

#if defined(_WIN32)
    cout << "platform: Windows\n";
#elif defined(__APPLE__)
    cout << "platform: macOS\n";
#elif defined(__linux__)
    cout << "platform: Linux\n";
#else
    cout << "platform: unknown\n";
#endif

    string host = "localhost";
    int    port = 8080;
    parse_options(argc, argv, host, port);
    cout << "host: " << host << "  port: " << port << "\n";

    demo_json();
    demo_asio_timer();
#ifndef __EMSCRIPTEN__
    demo_http();
#endif
    demo_filesystem();
    demo_regex();
#ifndef __EMSCRIPTEN__
    demo_thread_future();
#endif
    demo_fp();
    demo_embedded_resource();
    demo_spdlog();
    demo_url();
    demo_uuid();
#ifndef __EMSCRIPTEN__
    demo_process();
    demo_stacktrace();
#endif
    demo_date();
    demo_csv();

    return 0;
}
