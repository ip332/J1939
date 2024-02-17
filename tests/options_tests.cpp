//
// Created by igor.prilepov on 6/24/22.
//

#include "options.h"
#include <gtest/gtest.h>

class OptionsTest : public testing::Test {
protected:
    void SetUp() override {
        options_.addOption("-str", "String option", &str_);
        options_.addOption("-flag", "Boolean flag", &bool_);
        options_.addOption("-int", "Integer  value", &int_);
        options_.addOption("-list", "List of strings", &list_);
    }

    void TearDown() override {
    }

    OptionsManager options_;
    std::string str_;
    int int_ = 0;
    bool bool_ = false;
    std::vector<std::string> list_;
};

const char *argv[] = {
        "first argument is ignored",
        "-str",
        "string argument",
        "-int",
        "12345",
        "-flag",
        "-list",
        "first",
        "-list",
        "second",
        "-list",
        "third"
};

TEST_F(OptionsTest, StringOptions) {
    EXPECT_TRUE(str_.empty());
    options_.processArgs(3, argv);
    EXPECT_EQ(str_, "string argument");
}

TEST_F(OptionsTest, StrIntOptions) {
    EXPECT_EQ(int_,0);
    EXPECT_TRUE(str_.empty());
    options_.processArgs(5, argv);
    EXPECT_EQ(str_, "string argument");
    EXPECT_EQ(int_, 12345);
}

TEST_F(OptionsTest, StrIntBoolOptions) {
    EXPECT_EQ(int_,0);
    EXPECT_FALSE(bool_);
    EXPECT_TRUE(str_.empty());
    options_.processArgs(6, argv);
    EXPECT_EQ(str_, "string argument");
    EXPECT_EQ(int_, 12345);
    EXPECT_TRUE(bool_);
}

TEST_F(OptionsTest, AllOptions) {
    EXPECT_EQ(int_, 0);
    EXPECT_EQ(list_.size(), 0);
    EXPECT_FALSE(bool_);
    EXPECT_TRUE(str_.empty());
    options_.processArgs(12, argv);
    EXPECT_EQ(str_, "string argument");
    EXPECT_EQ(int_, 12345);
    EXPECT_TRUE(bool_);
    ASSERT_EQ(list_.size(), 3);
    ASSERT_EQ(list_[0],"first");
    ASSERT_EQ(list_[1],"second");
    ASSERT_EQ(list_[2],"third");
}
