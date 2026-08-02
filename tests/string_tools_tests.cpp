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
