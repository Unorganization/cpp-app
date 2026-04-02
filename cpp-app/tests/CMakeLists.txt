find_package(GTest REQUIRED)

add_executable(${PROJECT_NAME}_tests)

# ---------------------------------------------------------------------------
# Test source files
# Add one line per new test .cpp file you create.
# ---------------------------------------------------------------------------
target_sources(${PROJECT_NAME}_tests PRIVATE
    test_math_utils.cpp
    # test_my_module.cpp   ← add new test files here
)

# Link against the production library (inherits its include paths) and
# GTest::gtest_main (provides main() so you don't have to write one).
target_link_libraries(${PROJECT_NAME}_tests PRIVATE
    ${PROJECT_NAME}_lib
    GTest::gtest_main
)

# Registers each TEST() with CTest so `ctest` can run them individually.
include(GoogleTest)
gtest_discover_tests(${PROJECT_NAME}_tests)
