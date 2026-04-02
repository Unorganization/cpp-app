// =============================================================================
// main.cpp – demonstrates each supported library with a minimal working sample.
//
// Sections:
//   1. Boost.ProgramOptions  – command-line argument parsing
//   2. Boost.JSON            – parse, inspect, and serialize JSON
//   3. Boost.Asio            – async timer (event-loop / io_context pattern)
//   4. Boost.Beast           – synchronous HTTP GET over plain TCP
//   5. std::filesystem       – enumerate files in the working directory
//   6. std::regex            – search for an IP address pattern in a string
//   7. std::thread / future  – background task with std::async
//   8. FP (std::transform /  – map, filter, reduce over a vector
//          std::copy_if /
//          std::reduce)
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
#include <boost/beast.hpp>
#include <boost/beast/http.hpp>
#include <boost/json.hpp>
#include <boost/program_options.hpp>

#include "math_utils.h"

namespace fs  = std::filesystem;
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
        ("host",    po::value<std::string>()->default_value("localhost"),
                    "server hostname")
        ("port,p",  po::value<int>()->default_value(8080),
                    "server port");

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.count("help")) {
        std::cout << desc << "\n";
        std::exit(0);
    }
    return vm;
}

// -----------------------------------------------------------------------------
// 2. Boost.JSON
// -----------------------------------------------------------------------------
static void demo_json() {
    std::cout << "\n--- Boost.JSON ---\n";

    // Parse
    const std::string raw = R"({
        "service": "example-api",
        "version": 3,
        "features": ["async", "json", "http"]
    })";
    json::value doc = json::parse(raw);
    const json::object& obj = doc.as_object();

    std::cout << "service : " << obj.at("service").as_string() << "\n";
    std::cout << "version : " << obj.at("version").as_int64()  << "\n";
    std::cout << "features: ";
    for (const auto& f : obj.at("features").as_array()) {
        std::cout << f.as_string() << " ";
    }
    std::cout << "\n";

    // Serialize
    json::object response;
    response["status"] = "ok";
    response["result"] = add(6, 7);     // calls production code from math_utils
    std::cout << "serialized: " << json::serialize(response) << "\n";
}

// -----------------------------------------------------------------------------
// 3. Boost.Asio – async timer
// -----------------------------------------------------------------------------
static void demo_asio_timer() {
    std::cout << "\n--- Boost.Asio: async timer ---\n";

    net::io_context ioc;
    net::steady_timer timer(ioc, std::chrono::milliseconds(200));

    timer.async_wait([](const boost::system::error_code& ec) {
        if (!ec) {
            std::cout << "timer fired (200 ms)\n";
        }
    });

    // Run the event loop until all async work is complete
    ioc.run();
}

// -----------------------------------------------------------------------------
// 4. Boost.Beast – synchronous HTTP GET
// -----------------------------------------------------------------------------
static void demo_beast_http() {
    std::cout << "\n--- Boost.Beast: HTTP GET example.com ---\n";
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

        std::cout << "status : " << res.result_int() << " " << res.reason() << "\n";
        std::cout << "body   : "
                  << beast::buffers_to_string(res.body().data()).size()
                  << " bytes\n";

        beast::error_code ec;
        stream.socket().shutdown(tcp::socket::shutdown_both, ec);

    } catch (const std::exception& e) {
        std::cout << "skipped (no network or DNS): " << e.what() << "\n";
    }
}

// -----------------------------------------------------------------------------
// 5. std::filesystem
// -----------------------------------------------------------------------------
static void demo_filesystem() {
    std::cout << "\n--- std::filesystem ---\n";

    const fs::path cwd = fs::current_path();
    std::cout << "working directory: " << cwd << "\n";
    std::cout << "contents:\n";

    std::error_code ec;
    for (const auto& entry : fs::directory_iterator(cwd, ec)) {
        const char* kind = entry.is_directory() ? "DIR " : "FILE";
        std::cout << "  [" << kind << "] "
                  << entry.path().filename().string() << "\n";
    }
}

// -----------------------------------------------------------------------------
// 6. std::regex
// -----------------------------------------------------------------------------
static void demo_regex() {
    std::cout << "\n--- std::regex ---\n";

    const std::string text = "connect to 192.168.1.100 on port 8080";
    const std::regex  ip_pattern(R"(\d{1,3}(?:\.\d{1,3}){3})");

    std::smatch m;
    if (std::regex_search(text, m, ip_pattern)) {
        std::cout << "found IP address: " << m[0] << "\n";
    }

    // Iterate over all matches
    auto begin = std::sregex_iterator(text.begin(), text.end(),
                                      std::regex(R"(\d+)"));
    auto end   = std::sregex_iterator{};
    std::cout << "all numbers in text: ";
    for (auto it = begin; it != end; ++it) {
        std::cout << (*it)[0] << " ";
    }
    std::cout << "\n";
}

// -----------------------------------------------------------------------------
// 7. std::thread / std::future
// -----------------------------------------------------------------------------
static void demo_thread_future() {
    std::cout << "\n--- std::thread / std::future ---\n";

    // std::async fires the lambda on a thread-pool thread
    auto fut = std::async(std::launch::async, []() -> int {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        return add(100, 200);   // calls production code
    });

    // Do other work here while the async task runs ...
    std::cout << "waiting for async result...\n";

    std::cout << "async result: " << fut.get() << "\n";

    // Explicit std::thread example
    std::thread t([]() {
        std::cout << "background thread id: "
                  << std::this_thread::get_id() << "\n";
    });
    t.join();
}

// -----------------------------------------------------------------------------
// 8. FP – map / filter / reduce
// -----------------------------------------------------------------------------
static void demo_fp() {
    std::cout << "\n--- FP: map / filter / reduce ---\n";

    const std::vector<int> numbers = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

    // map: square every element
    std::vector<int> squared;
    squared.reserve(numbers.size());
    std::transform(numbers.begin(), numbers.end(),
                   std::back_inserter(squared),
                   [](int n) { return n * n; });

    std::cout << "squared: ";
    for (int n : squared) { std::cout << n << " "; }
    std::cout << "\n";

    // filter: keep only even numbers
    std::vector<int> evens;
    std::copy_if(numbers.begin(), numbers.end(),
                 std::back_inserter(evens),
                 [](int n) { return n % 2 == 0; });

    std::cout << "evens  : ";
    for (int n : evens) { std::cout << n << " "; }
    std::cout << "\n";

    // reduce: sum all elements (std::reduce is C++17, supports parallel policy)
    const int total = std::reduce(numbers.begin(), numbers.end(), 0);
    std::cout << "sum    : " << total << "\n";

    // chained: sum of squares of even numbers
    std::vector<int> even_squares;
    std::copy_if(squared.begin(), squared.end(),
                 std::back_inserter(even_squares),
                 [](int n) {
                     const int root = static_cast<int>(std::sqrt(n));
                     return root * root == n && root % 2 == 0;
                 });
    const int even_square_sum = std::reduce(even_squares.begin(),
                                            even_squares.end(), 0);
    std::cout << "sum of squares of evens: " << even_square_sum << "\n";
}

// =============================================================================
// main
// =============================================================================
int main(int argc, char* argv[]) {
    std::cout << "=== Cross-platform C++ app ===\n";

#if defined(_WIN32)
    std::cout << "platform: Windows\n";
#elif defined(__APPLE__)
    std::cout << "platform: macOS\n";
#elif defined(__linux__)
    std::cout << "platform: Linux\n";
#else
    std::cout << "platform: unknown\n";
#endif

    const auto vm = parse_options(argc, argv);
    std::cout << "host: " << vm["host"].as<std::string>()
              << "  port: " << vm["port"].as<int>() << "\n";

    demo_json();
    demo_asio_timer();
    demo_beast_http();
    demo_filesystem();
    demo_regex();
    demo_thread_future();
    demo_fp();

    return 0;
}
