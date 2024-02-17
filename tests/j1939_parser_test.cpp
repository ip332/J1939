//
// Created by igor on 2/17/21.
//

#include "dbc_parser.h"
#include "j1939_parser.h"

#include <gtest/gtest.h>

// Test DBC fragment
const std::map<uint32_t, PGN> TEST_DBC{
        {1234, // PGN value
         {"ShortSignals", 8, // PGN name and the DLC
          {
                  {"Signal1", 0, 1, false, true, 1, 0, {}},
                  {"Signal2", 1, 8, false, true, 1, 0, {}},
                  {"Signal3", 9, 16, false, true, 1, 0, {}},
                  {"Signal4", 31, 32, false, true, 1, 0, {}},
          }
         },
        },
        {5678, // PGN value
         {"LongSignals", 64, // This is an example of a super long PGN with 64 bytes local_frame.
          {
                  {"LongSignal1", 0, 64, false, true, 1, 0, {}},
                  {"LongSignal2", 64, 64, false, true, 1, 0, {}},
                  {"LongSignal3", 128, 64, false, true, 1, 0, {}},
                  {"LongSignal4", 192, 64, false, true, 1, 0, {}},
                  {"LongSignal5", 256, 64, false, true, 1, 0, {}},
                  {"LongSignal6", 320, 64, false, true, 1, 0, {}},
                  {"LongSignal7", 384, 64, false, true, 1, 0, {}},
                  {"LongSignal8", 448, 64, false, true, 1, 0, {}},
          }
         },
        },
        {1112, // PGN value
                {"SignedSignals", 16, // PGN name and the DLC
                        {
                                {"Signed_8", 0, 8, true, true, 1, 0, {}},
                                {"Signal_16", 9, 16, true, true, 1, 0, {}},
                                {"Signal_32", 24, 32, true, true, 1, 0, {}},
                                {"Signal_64", 64, 64, true, true, 1, 0, {}},
                        }
                },
        },
};

class J1939ParserTest : public testing::Test {
protected:
    void SetUp() override {
        frame_.reset();
        parser_ = std::make_unique<J1939Parser>(frame_, TEST_DBC);
    }

    void TearDown() override {
    }

    void reset() {
        frame_.reset();
    }

    void setValue(uint64_t val, size_t index, size_t length) {
        frame_.write_bits(val, index, length, true);
    }

    uint64_t getValue(size_t index, size_t length) {
        return frame_.read_bits(index, length, true);
    }

    void testRawSignalsReading(uint16_t can_id) {
        auto it = TEST_DBC.find(can_id);

        frame_.reset();
        frame_.setFrom(can_id);
        frame_.dlc_ = it->second.dlc_;

        for (const auto & spn : TEST_DBC.at(can_id).signals_){
            // Reset the buffer
            memset(frame_.buffer_, 0xFF, sizeof(frame_.buffer_));
            // Set the signal value to 0.
            setValue(0, spn.start_bit_, spn.length_);
            // Now check that reading value by name gives the same result (0)
            auto result = parser_->signalRawValue(spn.name_);
            EXPECT_NE(result.get(), nullptr);
            if (result) {
                EXPECT_EQ(*result, 0);
            }
            // Set the value to 1
            uint64_t expected = make_mask(spn.length_);
            setValue(expected, spn.start_bit_, spn.length_);
            result = parser_->signalRawValue(spn.name_, true);
            EXPECT_EQ(*result, expected);
        }
    }

    void testRawSignalsWriting(uint16_t can_id) {
        auto it = TEST_DBC.find(can_id);

        frame_.reset();
        frame_.setFrom(can_id);
        frame_.dlc_ = it->second.dlc_;

        for (const auto & spn : TEST_DBC.at(can_id).signals_){
            // Reset the buffer
            memset(frame_.buffer_, 0xFF, sizeof(frame_.buffer_));
            // Set the signal value to 0.
            parser_->setSignalValue(spn.name_, 0);
            // Now check that the correct bits were set in the frame.
            uint64_t result = getValue(spn.start_bit_, spn.length_);
            EXPECT_EQ(result, 0);
            // Set the value to 1
            parser_->setSignalValue(spn.name_, double(0xFFFFFFFFFFFFFFFF));
            uint64_t expected = make_mask(spn.length_);
            result = getValue(spn.start_bit_, spn.length_);
            EXPECT_EQ(result, expected);
        }
    }
    std::unique_ptr<std::ofstream> file() {
        std::unique_ptr<std::ofstream> file(new std::ofstream());
        file->open("pgn_.cpp");
        return file;
    }

    bool setDbc(const char *dbc_file) {
        std::string path(DBC_FOLDER);
        dbc_parser_.setVerbose(false);
        if (!dbc_parser_.parseFile(path + dbc_file)) {
            return false;
        }
        dbc_parser_.generateRunTimeFormat();
        parser_ = std::make_unique<J1939Parser>(frame_, dbc_parser_.dbc());
        return true;
    }

    J1939_frame frame_;
    DbcParser dbc_parser_;
    std::unique_ptr<J1939Parser> parser_;
};

// Tests that signalRawValue returns the corresponding bits from the underlying frame.
TEST_F(J1939ParserTest, SimpleSignalsReadTest) {
    testRawSignalsReading(1234);
}

// Tests that signalRawValue works with large messages and long fields.
TEST_F(J1939ParserTest, LongSignalsReadTest) {
    testRawSignalsReading(5678);
}

// Tests that signalRawValue returns the corresponding bits from the underlying frame.
TEST_F(J1939ParserTest, SimpleSignalsWriteTest) {
    testRawSignalsReading(1234);
}

// Tests that signalRawValue works with large messages and long fields.
TEST_F(J1939ParserTest, LongSignalsWriteTest) {
    testRawSignalsReading(5678);
}

TEST_F(J1939ParserTest, SignedSignalsTest) {
    auto it = TEST_DBC.find(1112);

    frame_.reset();
    frame_.setFrom(1112);
    frame_.dlc_ = it->second.dlc_;

    for (const auto & spn : TEST_DBC.at(1112).signals_){
        double test_vector[] = {-100, -20, -10, -1, 0, 1, 10, 20, 100};
        for (auto x : test_vector) {
            // Reset the buffer
            memset(frame_.buffer_, 0xFF, sizeof(frame_.buffer_));
            parser_->setSignalValue(spn.name_, x);
            // Now check that reading value by name gives the same result (0)
            auto result = parser_->signalValue(spn.name_, true);
            EXPECT_NE(result.get(), nullptr);
            if (result) {
                EXPECT_EQ(*result, x);
            }
        }
    }
}


TEST_F(J1939ParserTest, ParserRuntimeMessageFindTest) {
    ASSERT_TRUE(setDbc("test_database.dbc"));

    frame_.setFrom(160);
    EXPECT_EQ(parser_->getPgn(), nullptr);

    frame_.setFrom(16);
    EXPECT_NE(parser_->getPgn(), nullptr);
    frame_.setFrom(17);
    EXPECT_NE(parser_->getPgn(), nullptr);
    frame_.setFrom(18);
    EXPECT_NE(parser_->getPgn(), nullptr);
}

TEST_F(J1939ParserTest, GetSetSingleSignalRuntimeTest) {
    ASSERT_TRUE(setDbc("test_database.dbc"));

    frame_.setFrom(16);
    frame_.dlc_ = parser_->getPgn()->dlc_;

    const auto * signal_ptr = parser_->signal("signal1");
    EXPECT_TRUE(signal_ptr->little_endian_);
    auto raw_ptr = parser_->signalRawValue32("signal1", true);
    EXPECT_TRUE(raw_ptr != nullptr);
    EXPECT_EQ(*raw_ptr, make_mask(signal_ptr->length_));
    EXPECT_TRUE(parser_->setSignalValue("signal1", -6770764));
    auto ptr = parser_->signalValue("signal1");
    EXPECT_TRUE(ptr != nullptr);
    EXPECT_EQ(*ptr, -6770764);

    EXPECT_TRUE(parser_->setSignalValue("signal_2", 26));
    ptr = parser_->signalValue("signal_2");
    EXPECT_EQ(*ptr, 26);

}

TEST_F(J1939ParserTest, GetSetMultipleSignalsRuntimeTest) {
    ASSERT_TRUE(setDbc("test_database.dbc"));

    frame_.setFrom(17);
    frame_.dlc_ = parser_->getPgn()->dlc_;

    EXPECT_TRUE(parser_->setSignalValue("signal_1", 0x44));
    EXPECT_TRUE(parser_->setSignalValue("signal_2", 516349));
    EXPECT_TRUE(parser_->setSignalValue("signal_3", 208.0 * 0.25));

    auto ptr = parser_->signalValue("signal_1");
    EXPECT_EQ(*ptr, 0x44);
    ptr = parser_->signalValue("signal_2");
    EXPECT_EQ(*ptr, 516349);
    ptr = parser_->signalValue("signal_3");
    EXPECT_EQ(*ptr, 208.0 * 0.25);
}

TEST_F(J1939ParserTest, GetValuesLittleEndianTest) {
    ASSERT_TRUE(setDbc("test_database.dbc"));

    // first message
    uint8_t data[] = {0xb4, 0xaf, 0x98, 0x1a};
    frame_.setFrom(16, data, 4);

    auto ptr = parser_->signalValue("signal_2");
    EXPECT_EQ(*ptr, 26);
    ptr = parser_->signalValue("signal1");
    EXPECT_EQ(*ptr, -6770764);
}

TEST_F(J1939ParserTest, GetValuesBigEndianTest) {
    ASSERT_TRUE(setDbc("big_endian_database.dbc"));

    uint8_t data[7] = {0x3f, 0xff, 0xd4, 0x5a, 0x6a, 0x76, 0x69};
    frame_.setFrom(69, data, 7);

    auto ptr = parser_->signalValue("s1");
    EXPECT_EQ(*ptr, 1641);
    ptr = parser_->signalValue("s2");
    EXPECT_EQ(*ptr, -345);
    ptr = parser_->signalValue("s3");
    EXPECT_EQ(*ptr, 1365879257);
    ptr = parser_->signalValue("s4");
    EXPECT_EQ(*ptr, 0);
}

TEST_F(J1939ParserTest, PrintValuesBigEndianTest) {
    ASSERT_TRUE(setDbc("big_endian_database.dbc"));

    uint8_t data[7] = {0x3f, 0xff, 0xd4, 0x5a, 0x6a, 0x76, 0x69};
    frame_.setFrom(69, data, 7);

    std::stringstream stream;
    parser_->toString(stream);
    std::string expected("m1 CAN ID:0045(69) Data(7): 3FFFD45A6A7669 { s4: 0 s3: 1365879257 s2: -345 s1: 1641 }");
    EXPECT_EQ(stream.str(), expected);
}

TEST_F(J1939ParserTest, SetValuesBigEndianTest) {
    ASSERT_TRUE(setDbc("big_endian_database.dbc"));

    frame_.reset();
    frame_.setFrom(69);
    const auto *pgn = parser_->getPgn();
    ASSERT_NE(pgn, nullptr);
    frame_.dlc_ = pgn->dlc_;

    parser_->setSignalValue("s1",1641);
    parser_->setSignalValue("s2",-345);
    parser_->setSignalValue("s3",1365879257);
    parser_->setSignalValue("s4",0);

    uint8_t expected[7] = {0x3f, 0xff, 0xd4, 0x5a, 0x6a, 0x76, 0x69};
    EXPECT_EQ(memcmp(expected, frame_.buffer_, frame_.dlc_), 0);
}

TEST_F(J1939ParserTest, GetValuesRadarBigEndianTest) {
    ASSERT_TRUE(setDbc("big_endian_database.dbc"));

    uint8_t data[8] = {64, 12, 0, 0, 0, 200, 0, 0};
    frame_.setFrom(513, data, 8);

    auto ptr = parser_->signalValue("RadarState_MaxDistanceCfg");
    EXPECT_EQ(*ptr, 96);
    ptr = parser_->signalValue("RadarState_MotionRxState", true);
    EXPECT_EQ(*ptr, 3);
    ptr = parser_->signalValue("RadarState_OutputTypeCfg", true);
    EXPECT_EQ(*ptr, 2);
    ptr = parser_->signalValue("RadarState_NVMReadStatus", true);
    EXPECT_EQ(*ptr, 1);
}

TEST_F(J1939ParserTest, SetValuesRadarBigEndianTest) {
    ASSERT_TRUE(setDbc("big_endian_database.dbc"));

    frame_.reset(0);
    frame_.setFrom(768);
    const auto *pgn = parser_->getPgn();
    ASSERT_NE(pgn, nullptr);
    frame_.dlc_ = pgn->dlc_;

    parser_->setSignalValue("RadarDevice_SpeedDirection",2);
    parser_->setSignalValue("RadarDevice_Speed",0);

    uint8_t expected[2] = {0x80, 0};
    EXPECT_EQ(memcmp(expected, frame_.buffer_, frame_.dlc_), 0);
}

TEST_F(J1939ParserTest, SetValuesConfigBigEndianTest) {
    ASSERT_TRUE(setDbc("big_endian_database.dbc"));


    frame_.reset(0);
    frame_.setFrom(512);
    auto id = dbc_parser_.getId("RadarConfiguration_0");
    const auto *pgn = parser_->getPgn();
    ASSERT_NE(pgn, nullptr);
    frame_.dlc_ = pgn->dlc_;

    parser_->setSignalValue("RadarCfg_MaxDistance",50);
    EXPECT_EQ(frame_.buffer_[1], 6);
    EXPECT_EQ(frame_.buffer_[2], 64);
}
