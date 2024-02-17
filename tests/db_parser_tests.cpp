//
// Created by Igor on 2021-09-10.
//

#include "dbc_parser.h"
#include "j1939_parser.h"

#include <cstddef>
#include <gtest/gtest.h>

const char *msg_value = "BO_ 1234567 ABCD: 8 ABS";
const char *simple_signal = " SG_ Engine_Requested_SpeedSpeed_Limit : 8|16@1+ (0.125,0) [0|8191] \"rpm\" Vector__XXX";

const char *signal_value = " SG_ SystemStatus : 0|2@1+ (1,0) [0|3] \"\" Vector__XXX";
const char *val_record = "VAL_ 1234567 SystemStatus 2 \"Error\" 0 \"Off\" 1 \"On\" 3 \"Not-Available\" ;";
const char *expected_value_spn = "\t\t{\"SystemStatus\", 0, 2, false, true, 1, 0, 0, 3, {\n"
                                 "\t\t\t{0, \"Off\"}, {1, \"On\"}, {2, \"Error\"}, {3, \"Not-Available\"}, }},\n";
const char *expected_value_pgn = "{ \"ABCD\", 8, {\n"
                                 "\t\t{\"SystemStatus\", 0, 2, false, true, 1, 0, 0, 3, {\n"
                                 "\t\t\t{0, \"Off\"}, {1, \"On\"}, {2, \"Error\"}, {3, \"Not-Available\"}, }},\n"
                                 "\t}}";

const char *tsc1 = "BO_ 2348875520 TSC1: 8 ECM";
const char *tsc1_fields[] = {
        " SG_ Engine_Requested_Torque_Fraction : 40|4@1+ (0.125,0) [0|1] \"%\" Vector__XXX",
        " SG_ Engine_Override_Control_Mode : 0|2@1+ (1,0) [0|3] \"bit\" Vector__XXX",
        " SG_ Engine_Requested_SpeedSpeed_Limi : 8|16@1+ (0.125,0) [0|8191] \"rpm\" Vector__XXX",
        " SG_ Engine_Requested_Speed_Control_C : 2|2@1+ (1,0) [0|3] \"bit\" Vector__XXX",
        " SG_ TSC1_Control_Purpose : 35|5@1+ (1,0) [0|31] \"bit\" Vector__XXX",
        " SG_ Override_Control_Mode_Priority : 4|2@1+ (1,0) [0|3] \"bit\" Vector__XXX",
        " SG_ TSC1_Transmission_Rate : 32|3@1+ (1,0) [0|7] \"bit\" Vector__XXX",
        " SG_ Engine_Requested_TorqueTorque_Li : 24|8@1+ (1,-125) [-125|380] \"%\" Vector__XXX"
};

const char *expected_tsc1_pgn = "{ \"TSC1\", 8, {\n"
        "\t\t{\"Engine_Override_Control_Mode\", 0, 2, false, true, 1, 0, 0, 3, {}},\n"
        "\t\t{\"Engine_Requested_Speed_Control_C\", 2, 2, false, true, 1, 0, 0, 3, {}},\n"
        "\t\t{\"Override_Control_Mode_Priority\", 4, 2, false, true, 1, 0, 0, 3, {}},\n"
        "\t\t{\"Engine_Requested_SpeedSpeed_Limi\", 8, 16, false, true, 0.125, 0, 0, 8191, {}},\n"
        "\t\t{\"Engine_Requested_TorqueTorque_Li\", 24, 8, false, true, 1, -125, -125, 380, {}},\n"
        "\t\t{\"TSC1_Transmission_Rate\", 32, 3, false, true, 1, 0, 0, 7, {}},\n"
        "\t\t{\"TSC1_Control_Purpose\", 35, 5, false, true, 1, 0, 0, 31, {}},\n"
        "\t\t{\"Engine_Requested_Torque_Fraction\", 40, 4, false, true, 0.125, 0, 0, 1, {}},\n"
        "\t}}";

class DbcParserTest : public testing::Test {
protected:
    void SetUp() override {
    }

    void TearDown() override {
    }

};

TEST_F(DbcParserTest, SimpleSignalFormatTest) {
    Signal signal(simple_signal);

    EXPECT_TRUE(signal.valid());
    std::string str = signal.toSpnString();
    EXPECT_EQ(str,"\t\t{\"Engine_Requested_SpeedSpeed_Limit\", 8, 16, false, true, 0.125, 0, 0, 8191, {}},\n");
}

TEST_F(DbcParserTest, SignalWithValueFormatTest) {
    Signal signal(signal_value);

    EXPECT_TRUE(signal.valid());
    std::vector<std::string> values = splitString(val_record, ' ');
    signal.addValue(values);

    std::string str = signal.toSpnString();
    EXPECT_EQ(str,expected_value_spn);
}

TEST_F(DbcParserTest, EmptyMessagePgnFormatTest) {
    Message msg(tsc1);

    EXPECT_EQ(msg.toPgnString(), "{ \"TSC1\", 8, {\n\t}}");
}

TEST_F(DbcParserTest, SimpleMessagePgnFormatTest) {
    Message msg(tsc1);

    for(const auto *ptr : tsc1_fields) {
        msg.addSignal(ptr);
    }
    EXPECT_EQ(msg.toPgnString(), expected_tsc1_pgn);
}


TEST_F(DbcParserTest, MessageValuePgnFormatTest) {
    Message msg(msg_value);

    msg.addSignal(signal_value);

    std::vector<std::string> fields = splitString(val_record, ' ');
    msg.addValues(fields);

    EXPECT_EQ(msg.toPgnString(), expected_value_pgn);
}

TEST_F(DbcParserTest, RuntimeMessageFindTest) {
    DbcParser parser;
    parser.setVerbose(false);
    ASSERT_TRUE(parser.parseFile(DBC_FOLDER"test_database.dbc"));
    parser.generateRunTimeFormat();
    auto dbc = parser.dbc();
    EXPECT_EQ(dbc.find(160), dbc.end());
    EXPECT_NE(dbc.find(16), dbc.end());
    EXPECT_NE(dbc.find(17), dbc.end());
    EXPECT_NE(dbc.find(18), dbc.end());
}
