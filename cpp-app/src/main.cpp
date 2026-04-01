#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    std::cout << "Hello from the cross-platform C++ app!" << std::endl;
    std::cout << "Running on: ";

#if defined(_WIN32)
    std::cout << "Windows";
#elif defined(__APPLE__)
    std::cout << "macOS";
#elif defined(__linux__)
    std::cout << "Linux";
#else
    std::cout << "Unknown platform";
#endif

    std::cout << std::endl;

    if (argc > 1) {
        std::cout << "Arguments received:" << std::endl;
        for (int i = 1; i < argc; ++i) {
            std::cout << "  [" << i << "] " << argv[i] << std::endl;
        }
    }

    return 0;
}
