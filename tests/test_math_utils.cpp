#include <gtest/gtest.h>
#include "math_utils.h"

using namespace std;

TEST(AddTest, PositiveNumbers) {
    EXPECT_EQ(add(2, 3), 5);
}

TEST(AddTest, NegativeNumbers) {
    EXPECT_EQ(add(-4, -6), -10);
}

TEST(AddTest, MixedSignNumbers) {
    EXPECT_EQ(add(10, -3), 7);
}

TEST(AddTest, Zeroes) {
    EXPECT_EQ(add(0, 0), 0);
}
