// =============================================================================
// main.cpp – demonstrates each supported library with a minimal working sample.
//
// Sections:
//   1. Boost.ProgramOptions  – command-line argument parsing
//   2. Boost.JSON            – parse, inspect, and serialize JSON
//   3. Boost.Asio            – async timer (event-loop / io_context pattern)
//   4. Boost.Beast           – synchronous HTTP GET over plain TCP
//   5. filesystem       – enumerate files in the working directory
//   6. regex            – search for an IP address pattern in a string
//   7. thread / future  – background task with async
//   8. FP (transform /  – map, filter, reduce over a vector
//          copy_if /
//          reduce)
//   9. Embedded resource     – binary file linked into the executable at build
//                              time via llvm-objcopy; accessed through linker
//                              symbols with no file I/O at runtime
//  10. spdlog                 – structured logging with coloured stdout sink
//                              and a size-based rotating file sink; oldest
//                              files deleted automatically when limit is reached
//  11. Boost.URL              – parse, inspect, and mutate URIs
//  12. Boost.UUID             – generate random (v4) and name-based (v5) UUIDs
//  13. Boost.Process v2       – launch a child process, capture its stdout
//  14. Boost.Stacktrace       – capture and print the current call stack
//  15. Howard Hinnant's Date  – calendar date arithmetic on top of <chrono>
//                              (core library only; no IANA timezone data needed)
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

#include <boost/asio.hpp>
#include <boost/json.hpp>
#include <boost/program_options.hpp>
#include <boost/url.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <date/date.h>
#include <csv.hpp>

// These libraries require OS capabilities absent from the browser sandbox.
#ifndef __EMSCRIPTEN__
#include <boost/beast.hpp>
#include <boost/beast/http.hpp>
#include <boost/process/v2.hpp>
#include <boost/stacktrace.hpp>
#endif

#include "math_utils.h"
#include "embedded_resource.h"
#include "logger.h"

using namespace std;

namespace fs  = filesystem;
namespace po  = boost::program_options;
namespace net = boost::asio;
using     tcp = net::ip::tcp;
namespace beast = boost::beast;
namespace http  = beast::http;
namespace json  = boost::json;

// -----------------------------------------------------------------------------
// 1. Boost.ProgramOptions
// -----------------------------------------------------------------------------
static po::variables_map parse_options(int argc, char* argv[]) {
    po::options_description desc("Options");
    desc.add_options()
        ("help,h",  "show this help message")
        ("host",    po::value<string>()->default_value("localhost"),
                    "server hostname")
        ("port,p",  po::value<int>()->default_value(8080),
                    "server port");

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.count("help")) {
        cout << desc << "\n";
        exit(0);
    }
    return vm;
}

// -----------------------------------------------------------------------------
// 2. Boost.JSON
// -----------------------------------------------------------------------------
static void demo_json() {
    cout << "\n--- Boost.JSON ---\n";

    // Parse
    const string raw = R"({
        "service": "example-api",
        "version": 3,
        "features": ["async", "json", "http"]
    })";
    json::value doc = json::parse(raw);
    const json::object& obj = doc.as_object();

    cout << "service : " << obj.at("service").as_string() << "\n";
    cout << "version : " << obj.at("version").as_int64()  << "\n";
    cout << "features: ";
    for (const auto& f : obj.at("features").as_array()) {
        cout << f.as_string() << " ";
    }
    cout << "\n";

    // Serialize
    json::object response;
    response["status"] = "ok";
    response["result"] = add(6, 7);     // calls production code from math_utils
    cout << "serialized: " << json::serialize(response) << "\n";
}

// -----------------------------------------------------------------------------
// 3. Boost.Asio – async timer
// -----------------------------------------------------------------------------
static void demo_asio_timer() {
    cout << "\n--- Boost.Asio: async timer ---\n";

    net::io_context ioc;
    net::steady_timer timer(ioc, chrono::milliseconds(200));

    timer.async_wait([](const boost::system::error_code& ec) {
        if (!ec) {
            cout << "timer fired (200 ms)\n";
        }
    });

    // Run the event loop until all async work is complete
    ioc.run();
}

// -----------------------------------------------------------------------------
// 4. Boost.Beast – synchronous HTTP GET  (skipped on WebAssembly: no raw TCP)
// -----------------------------------------------------------------------------
#ifndef __EMSCRIPTEN__
static void demo_beast_http() {
    cout << "\n--- Boost.Beast: HTTP GET example.com ---\n";
    try {
        net::io_context ioc;
        tcp::resolver   resolver(ioc);
        beast::tcp_stream stream(ioc);

        // Resolve and connect
        stream.connect(resolver.resolve("example.com", "80"));

        // Build the request
        http::request<http::string_body> req{http::verb::get, "/", 11};
        req.set(http::field::host,       "example.com");
        req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
        req.set(http::field::connection, "close");
        http::write(stream, req);

        // Read the response
        beast::flat_buffer                  buf;
        http::response<http::dynamic_body>  res;
        http::read(stream, buf, res);

        cout << "status : " << res.result_int() << " " << res.reason() << "\n";
        cout << "body   : "
                  << beast::buffers_to_string(res.body().data()).size()
                  << " bytes\n";

        beast::error_code ec;
        stream.socket().shutdown(tcp::socket::shutdown_both, ec);

    } catch (const exception& e) {
        cout << "skipped (no network or DNS): " << e.what() << "\n";
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

    // Parse the embedded bytes as JSON using Boost.JSON (already linked in).
    const json::value  doc = json::parse(raw);
    const json::object& obj = doc.as_object();

    cout << "application    : " << obj.at("application").as_string() << "\n";
    cout << "description    : " << obj.at("description").as_string() << "\n";

    const json::array& features =
        obj.at("settings").as_object().at("features").as_array();
    cout << "features       : ";
    for (const auto& f : features) {
        cout << f.as_string() << " ";
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
// 11. Boost.URL – parse, inspect, and mutate URIs
// -----------------------------------------------------------------------------
static void demo_boost_url() {
    cout << "\n--- Boost.URL ---\n";
    namespace urls = boost::urls;

    // Parse to an immutable view (zero-copy; input string must remain alive)
    const urls::url_view uv = urls::parse_uri(
        "https://api.example.com:8443/v2/users?page=1&limit=50#results").value();

    cout << "scheme   : " << uv.scheme()   << "\n";
    cout << "host     : " << uv.host()     << "\n";
    cout << "port     : " << uv.port()     << "\n";
    cout << "path     : " << uv.path()     << "\n";
    cout << "query    : " << uv.query()    << "\n";
    cout << "fragment : " << uv.fragment() << "\n";

    // Mutable url for building / modifying
    urls::url u(uv);
    u.set_path("/v3/users");
    u.set_query("page=2&limit=25");
    cout << "modified : " << u << "\n";
}

// -----------------------------------------------------------------------------
// 12. Boost.UUID – random (v4) and name-based (v5) UUIDs
// -----------------------------------------------------------------------------
static void demo_boost_uuid() {
    cout << "\n--- Boost.UUID ---\n";

    // v4: cryptographically random
    boost::uuids::random_generator rgen;
    const auto id1 = rgen();
    const auto id2 = rgen();
    cout << "random uuid 1  : " << id1 << "\n";
    cout << "random uuid 2  : " << id2 << "\n";
    cout << "are equal      : " << (id1 == id2 ? "yes" : "no") << "\n";

    // v5: name-based (SHA-1) – identical input always yields identical UUID
    boost::uuids::name_generator_sha1 ngen(boost::uuids::ns::url());
    const auto id3 = ngen("https://example.com");
    const auto id4 = ngen("https://example.com");
    cout << "name-based     : " << id3 << "\n";
    cout << "reproducible   : " << (id3 == id4 ? "yes" : "no") << "\n";

    // String round-trip
    const string str = boost::uuids::to_string(id1);
    const auto        id5 = boost::uuids::string_generator()(str);
    cout << "string form    : " << str << "\n";
    cout << "round-trip ok  : " << (id1 == id5 ? "yes" : "no") << "\n";
}

// -----------------------------------------------------------------------------
// 13. Boost.Process v2  (skipped on WebAssembly: no process spawning)
// -----------------------------------------------------------------------------
#ifndef __EMSCRIPTEN__
static void demo_boost_process() {
    cout << "\n--- Boost.Process v2 ---\n";
    namespace bp = boost::process::v2;

    // cmake must be installed on any machine that can build this project.
    const auto cmake_exe = bp::environment::find_executable("cmake");
    if (cmake_exe.empty()) {
        cout << "cmake not found in PATH; skipping\n";
        return;
    }
    cout << "cmake path     : " << cmake_exe.string() << "\n";

    net::io_context ioc;
    boost::asio::readable_pipe rp{ioc};

    // Redirect child stdout to the pipe; stdin and stderr are inherited.
    bp::process_stdio stdio;
    stdio.out = rp;

    bp::process proc(ioc, cmake_exe, {"--version"}, stdio);

    // Synchronous read until the child closes its end of the pipe (EOF).
    string output;
    boost::system::error_code ec;
    boost::asio::read(rp, boost::asio::dynamic_buffer(output), ec);
    // ec == boost::asio::error::eof here – expected, not an error.

    const int code = proc.wait();

    // Print only the first line ("cmake version X.Y.Z")
    const auto nl = output.find('\n');
    cout << "output         : "
              << output.substr(0, nl != string::npos ? nl : output.size())
              << "\n";
    cout << "exit code      : " << code << "\n";
}
#endif // !__EMSCRIPTEN__

// -----------------------------------------------------------------------------
// 14. Boost.Stacktrace  (skipped on WebAssembly: no stack unwinding support)
// -----------------------------------------------------------------------------
#ifndef __EMSCRIPTEN__
static void demo_stacktrace() {
    cout << "\n--- Boost.Stacktrace ---\n";

    // Captures the call stack at this point.
    // Built with BOOST_STACKTRACE_USE_BASIC so no debug-info files are needed;
    // each frame shows a raw address.  For human-readable names rebuild with:
    //   Linux  : Boost::stacktrace_addr2line + BOOST_STACKTRACE_USE_ADDR2LINE
    //   Windows: Boost::stacktrace_windbg    + BOOST_STACKTRACE_USE_WINDBG
    const boost::stacktrace::stacktrace st;
    cout << "frames captured: " << st.size() << "\n";

    const size_t show = min(st.size(), size_t{5});
    for (size_t i = 0; i < show; ++i) {
        cout << "  [" << i << "] " << st[i] << "\n";
    }
    if (st.size() > show) {
        cout << "  ... (" << (st.size() - show) << " more frames)\n";
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

    const auto vm = parse_options(argc, argv);
    cout << "host: " << vm["host"].as<string>()
              << "  port: " << vm["port"].as<int>() << "\n";

    demo_json();
    demo_asio_timer();
#ifndef __EMSCRIPTEN__
    demo_beast_http();
#endif
    demo_filesystem();
    demo_regex();
#ifndef __EMSCRIPTEN__
    demo_thread_future();
#endif
    demo_fp();
    demo_embedded_resource();
    demo_spdlog();
    demo_boost_url();
    demo_boost_uuid();
#ifndef __EMSCRIPTEN__
    demo_boost_process();
    demo_stacktrace();
#endif
    demo_date();
    demo_csv();

    return 0;
}
