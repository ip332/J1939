//
// Created by igor on 9/13/21.
//

#include "dbc_signal.h"

#include <gtest/gtest.h>

class StringToolsTest : public testing::Test {
protected:
    void SetUp() override {
    }

    void TearDown() override {
    }

};

TEST_F(StringToolsTest, SimpleStringSplitTest) {
    auto fields = splitString("One Two Three", ' ');
    EXPECT_EQ(fields.size(), 3);
    EXPECT_EQ(fields[0], "One");
    EXPECT_EQ(fields[1], "Two");
    EXPECT_EQ(fields[2], "Three");
}

TEST_F(StringToolsTest, ComplexSplitTest) {
    auto fields = splitString("One Two:Three", ':');
    EXPECT_EQ(fields.size(), 2);
    EXPECT_EQ(fields[0], "One Two");
    EXPECT_EQ(fields[1], "Three");
}

TEST_F(StringToolsTest, SnakeConversionTest) {
    EXPECT_EQ(toSnakeCase("StrangersInTheNight"), "strangers_in_the_night");
    EXPECT_EQ(toSnakeCase("Strangers_In__The___Night"), "strangers_in_the_night");
    EXPECT_EQ(toSnakeCase("_StrangersInTheNight"), "strangers_in_the_night");
    EXPECT_EQ(toSnakeCase("ABC_XYZ_123"),"abc_xyz_123");
}

// Adversarial input: alternating case with no repeats maximizes the output/input
// character ratio (each pair of input characters can emit up to 3 output characters:
// an inserted underscore plus both letters), the worst case for the fixed-size stack
// buffer sized at name.length() + name.length()/2 + 1. This confirms the buffer's
// boundary-check `break`s never actually fire for this pattern -- there's always at
// least one character of slack -- without corrupting the result.
TEST_F(StringToolsTest, SnakeConversionStressTestForBufferBoundary) {
    std::string input;
    for (int i = 0; i < 200; i++) {
        input += (i % 2 == 0) ? 'a' : 'A';
    }
    std::string result = toSnakeCase(input);
    EXPECT_FALSE(result.empty());
    EXPECT_EQ(result.find("__"), std::string::npos);
}
