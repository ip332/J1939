//
// Tests for can_stream: confirms each text format still decodes real sample
// lines correctly, and that malformed/oversized declared lengths are rejected
// (return false) rather than causing a stack overflow, out-of-bounds read, or
// an uncaught exception.
//

#include "asc_stream.h"
#include "log_stream.h"
#include "out_stream.h"
#include "trc_stream.h"
#include "txt_stream.h"

#include <cstdio>
#include <cstring>
#include <gtest/gtest.h>

class CanStreamTest : public testing::Test {
protected:
    void TearDown() override {
        for (const auto &path : temp_paths_) {
            remove(path.c_str());
        }
    }

    // Writes `content` to a fresh temp file and returns its path.
    std::string writeTemp(const std::string &content) {
        char path[] = "/tmp/can_stream_test_XXXXXX";
        int fd = mkstemp(path);
        FILE *f = fdopen(fd, "w");
        fwrite(content.data(), 1, content.size(), f);
        fclose(f);
        temp_paths_.emplace_back(path);
        return path;
    }

    // Opens `content` with StreamT and returns the first successfully decoded frame.
    template <typename StreamT>
    bool decodeFirstDataLine(const std::string &content, J1939_frame *frame) {
        std::string path = writeTemp(content);
        StreamT stream;
        if (!stream.open(path)) {
            return false;
        }
        while (stream.dataAvailable()) {
            if (stream.get(frame)) {
                return true;
            }
        }
        return false;
    }

    std::vector<std::string> temp_paths_;
};

TEST_F(CanStreamTest, LogStreamDecodesRealSampleLine) {
    J1939_frame frame;
    bool ok = decodeFirstDataLine<LogStream>(
        "(1557265716.818982) can0 0CFF0686#F22700F2FFFFFF5F\n", &frame);
    EXPECT_TRUE(ok);
    EXPECT_EQ(frame.dlc_, 8);
    EXPECT_EQ(frame.buffer_[0], 0xF2);
    EXPECT_EQ(frame.buffer_[7], 0x5F);
}

TEST_F(CanStreamTest, LogStreamDecodesNonZeroPaddedCanId) {
    // LogStream::put always zero-pads the ID to 8 hex digits, but a real
    // candump-style log isn't guaranteed to; the parser must not assume a
    // fixed width for the ID field.
    J1939_frame frame;
    bool ok = decodeFirstDataLine<LogStream>(
        "(1557265716.818982) can0 1A#F22700F2FFFFFF5F\n", &frame);
    EXPECT_TRUE(ok);
    EXPECT_EQ(frame.dlc_, 8);
}

TEST_F(CanStreamTest, OutStreamDecodesRealSampleLine) {
    J1939_frame frame;
    bool ok = decodeFirstDataLine<OutStream>(
        "interface = can0, family = 29, type = 3, proto = 1\n"
        "<0x0cf00400> [8] ff ff ff 0c 1c ff f4 7d\n", &frame);
    EXPECT_TRUE(ok);
    EXPECT_EQ(frame.dlc_, 8);
    EXPECT_EQ(frame.buffer_[0], 0xff);
    EXPECT_EQ(frame.buffer_[3], 0x0c);
}

TEST_F(CanStreamTest, OutStreamRejectsAbsurdDeclaredLength) {
    J1939_frame frame;
    bool ok = decodeFirstDataLine<OutStream>("<0x0cf00400> [999999999] ff ff ff 0c\n", &frame);
    EXPECT_FALSE(ok);
}

TEST_F(CanStreamTest, TrcStreamDecodesRealSampleLine) {
    J1939_frame frame;
    bool ok = decodeFirstDataLine<TrcStream>(
        ";$FILEVERSION=1.1\n;$STARTTIME=43614.5999164005\n;\n;   trace2.trc\n;\n"
        ";   Start time: 5/29/2019 14:23:52.777.0\n;\n"
        "     1)      3099.8  Rx     1CFF6A27  8  FF FF 00 F7 FF FF FF FF \n", &frame);
    EXPECT_TRUE(ok);
    EXPECT_EQ(frame.dlc_, 8);
    EXPECT_EQ(frame.buffer_[0], 0xFF);
    EXPECT_EQ(frame.buffer_[2], 0x00);
}

TEST_F(CanStreamTest, TrcStreamRejectsNonNumericLengthToken) {
    // Must not throw (the original code used std::stoul, which throws).
    J1939_frame frame;
    bool ok = decodeFirstDataLine<TrcStream>(
        ";   Start time: 5/29/2019 14:23:52.777.0\n"
        "     1)      3099.8  Rx     1CFF6A27  notanumber  FF FF 00 F7\n", &frame);
    EXPECT_FALSE(ok);
}

TEST_F(CanStreamTest, TrcStreamRejectsAbsurdDeclaredLength) {
    J1939_frame frame;
    bool ok = decodeFirstDataLine<TrcStream>(
        ";   Start time: 5/29/2019 14:23:52.777.0\n"
        "     1)      3099.8  Rx     1CFF6A27  999999999  FF FF 00 F7\n", &frame);
    EXPECT_FALSE(ok);
}

TEST_F(CanStreamTest, TrcStreamRejectsLengthExceedingAvailableFields) {
    J1939_frame frame;
    bool ok = decodeFirstDataLine<TrcStream>(
        ";   Start time: 5/29/2019 14:23:52.777.0\n"
        "     1)      3099.8  Rx     1CFF6A27  8  FF FF\n", &frame);
    EXPECT_FALSE(ok);
}

TEST_F(CanStreamTest, TrcStreamRecoversAfterOverLongLine) {
    // A line longer than the internal line buffer must be skipped without
    // corrupting the parse of the next (valid) line.
    std::string huge_comment = ";" + std::string(20000, 'x') + "\n";
    std::string content =
        ";   Start time: 5/29/2019 14:23:52.777.0\n" +
        huge_comment +
        "     1)      3099.8  Rx     1CFF6A27  8  FF FF 00 F7 FF FF FF FF \n";
    J1939_frame frame;
    bool ok = decodeFirstDataLine<TrcStream>(content, &frame);
    EXPECT_TRUE(ok);
    EXPECT_EQ(frame.dlc_, 8);
    EXPECT_EQ(frame.buffer_[0], 0xFF);
    EXPECT_EQ(frame.buffer_[2], 0x00);
}

TEST_F(CanStreamTest, AscStreamDecodesRealSampleLine) {
    J1939_frame frame;
    bool ok = decodeFirstDataLine<AscStream>(
        "date Sun May 26 19:15:02 UTC 2019\n"
        "base hex  timestamps absolute\n"
        "00000.000000 Start of measurement\n"
        "00000.000000  \t 1   CFF0686x  Rx  d 8 F2 27 00 F2 FF FF FF 5F\n", &frame);
    EXPECT_TRUE(ok);
    EXPECT_EQ(frame.dlc_, 8);
    EXPECT_EQ(frame.buffer_[0], 0xF2);
    EXPECT_EQ(frame.buffer_[7], 0x5F);
}

TEST_F(CanStreamTest, AscStreamRejectsAbsurdDeclaredLength) {
    J1939_frame frame;
    bool ok = decodeFirstDataLine<AscStream>(
        "date Sun May 26 19:15:02 UTC 2019\n"
        "base hex  timestamps absolute\n"
        "00000.000000  \t 1   CFF0686x  Rx  d 999999999 F2 27 00 F2\n", &frame);
    EXPECT_FALSE(ok);
}

TEST_F(CanStreamTest, TxtStreamDecodesPlainCandumpLine) {
    J1939_frame frame;
    bool ok = decodeFirstDataLine<TxtStream>(
        "can0  19F21200   [8]  20 0B 01 01 00 64 FF FF\n", &frame);
    EXPECT_TRUE(ok);
    EXPECT_EQ(frame.dlc_, 8);
    EXPECT_EQ(frame.buffer_[0], 0x20);
    EXPECT_EQ(frame.buffer_[5], 0x64);
}
