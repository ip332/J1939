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
#include "can_stream.h"
#include "mock_can_stream.h"

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

TEST_F(CanStreamTest, AscStreamParsesEveryMonthAbbreviation) {
    AscStream stream;
    std::string path = writeTemp(
        "date x Jun 1 01:01:01 am 2020\n"
        "date x Jul 1 01:01:01 am 2020\n"
        "date x Aug 1 01:01:01 am 2020\n"
        "date x Sep 1 01:01:01 am 2020\n"
        "date x Oct 1 01:01:01 am 2020\n"
        "date x Nov 1 01:01:01 pm 2020\n"
        "date x Dec 1 01:01:01 am 2020\n"
        "date x Xxx 1 01:01:01 am 2020\n"); // unrecognized month
    ASSERT_TRUE(stream.open(path));
    J1939_frame frame;
    // Every "date" line returns false (it only updates internal state); just confirm
    // none of them crash and all 8 lines get consumed.
    for (int i = 0; i < 8; i++) {
        EXPECT_FALSE(stream.get(&frame));
    }
}

TEST_F(CanStreamTest, AscStreamRejectsTruncatedDateLine) {
    J1939_frame frame;
    bool ok = decodeFirstDataLine<AscStream>("date x Jun 1\n", &frame);
    EXPECT_FALSE(ok);
}

TEST_F(CanStreamTest, AscStreamDecodesDecimalModeLine) {
    J1939_frame frame;
    bool ok = decodeFirstDataLine<AscStream>(
        "base dec  timestamps absolute\n"
        "7.000000 1  100  Rx   d 2 010 020\n", &frame);
    EXPECT_TRUE(ok);
    EXPECT_EQ(frame.dlc_, 2);
    EXPECT_EQ(frame.buffer_[0], 10);
    EXPECT_EQ(frame.buffer_[1], 20);
}

TEST_F(CanStreamTest, AscStreamAbortsOnSecondChannel) {
    J1939_frame frame;
    bool ok = decodeFirstDataLine<AscStream>(
        "7.000000 2  CFF2100x  Rx  d 8 00 00 32 00 32 00 00 0B\n", &frame);
    EXPECT_FALSE(ok);
}

TEST_F(CanStreamTest, LogStreamPutWritesAndReadsBackAFrame) {
    std::string path = writeTemp("");
    LogStream out;
    ASSERT_TRUE(out.open(path, true));
    J1939_frame sent;
    const uint8_t payload[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    ASSERT_TRUE(sent.setFrom(0x18FEF100, payload, 8));
    EXPECT_TRUE(out.put(sent));
    out.close();

    LogStream in;
    ASSERT_TRUE(in.open(path));
    J1939_frame received;
    ASSERT_TRUE(in.get(&received));
    EXPECT_EQ(received.dlc_, 8);
    EXPECT_EQ(received.buffer_[0], 1);
}

TEST_F(CanStreamTest, LogStreamRejectsLineWithoutOpeningParen) {
    J1939_frame frame;
    bool ok = decodeFirstDataLine<LogStream>("not a log line\n", &frame);
    EXPECT_FALSE(ok);
}

TEST_F(CanStreamTest, LogStreamRejectsLineWithoutClosingParen) {
    J1939_frame frame;
    bool ok = decodeFirstDataLine<LogStream>("(1557265716.818982 can0 0CFF0686#F2\n", &frame);
    EXPECT_FALSE(ok);
}

TEST_F(CanStreamTest, LogStreamRejectsLineWithoutHashSeparator) {
    J1939_frame frame;
    bool ok = decodeFirstDataLine<LogStream>("(1557265716.818982) can0 0CFF0686\n", &frame);
    EXPECT_FALSE(ok);
}

TEST_F(CanStreamTest, OutStreamRejectsTooFewFields) {
    J1939_frame frame;
    bool ok = decodeFirstDataLine<OutStream>("<\n", &frame);
    EXPECT_FALSE(ok);
}

TEST_F(CanStreamTest, OutStreamRejectsMissingHexPrefix) {
    J1939_frame frame;
    bool ok = decodeFirstDataLine<OutStream>("<0cf00400> [8] ff ff ff 0c 1c ff f4 7d\n", &frame);
    EXPECT_FALSE(ok);
}

TEST_F(CanStreamTest, OutStreamRejectsMissingClosingAngleBracket) {
    J1939_frame frame;
    bool ok = decodeFirstDataLine<OutStream>("<0x0cf00400 [8] ff ff ff 0c 1c ff f4 7d\n", &frame);
    EXPECT_FALSE(ok);
}

TEST_F(CanStreamTest, OutStreamRejectsMissingClosingBracket) {
    J1939_frame frame;
    bool ok = decodeFirstDataLine<OutStream>("<0x0cf00400> [8 ff ff ff 0c 1c ff f4 7d\n", &frame);
    EXPECT_FALSE(ok);
}

TEST_F(CanStreamTest, OutStreamRejectsNonNumericLength) {
    J1939_frame frame;
    bool ok = decodeFirstDataLine<OutStream>("<0x0cf00400> [abc] ff ff ff 0c\n", &frame);
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

TEST_F(CanStreamTest, TxtStreamDecodesLineWithTimestamp) {
    J1939_frame frame;
    bool ok = decodeFirstDataLine<TxtStream>(
        "(1657126380.578519)  can0  01A   [8]  11 22 33 44 AA BB CC DD\n", &frame);
    ASSERT_TRUE(ok);
    EXPECT_EQ(frame.dlc_, 8);
    EXPECT_EQ(frame.buffer_[0], 0x11);
    EXPECT_EQ(frame.time_ns_, static_cast<uint64_t>(1657126380.578519 * 1E9));
}

TEST_F(CanStreamTest, TxtStreamRejectsLineWithUnterminatedTimestamp) {
    J1939_frame frame;
    bool ok = decodeFirstDataLine<TxtStream>("(1657126380.578519  can0  01A [8] 11\n", &frame);
    EXPECT_FALSE(ok);
}

TEST_F(CanStreamTest, TxtStreamRejectsTooFewFields) {
    J1939_frame frame;
    bool ok = decodeFirstDataLine<TxtStream>("can0 01A\n", &frame);
    EXPECT_FALSE(ok);
}

TEST_F(CanStreamTest, TxtStreamRejectsUnparsableDataToken) {
    J1939_frame frame;
    bool ok = decodeFirstDataLine<TxtStream>("can0  19F21200   [8]  ZZ\n", &frame);
    EXPECT_FALSE(ok);
}

TEST_F(CanStreamTest, SetExtendedOrsTheExtendedBitIntoDecodedFrames) {
    std::string path = writeTemp("can0  19F21200   [8]  20 0B 01 01 00 64 FF FF\n");
    TxtStream stream;
    ASSERT_TRUE(stream.open(path));
    stream.setExtended(true);
    J1939_frame frame;
    ASSERT_TRUE(stream.get(&frame));
    EXPECT_TRUE(frame.extended_);
}

TEST_F(CanStreamTest, TxtStreamPutWritesAndReadsBackAFrame) {
    std::string path = writeTemp("");
    TxtStream out;
    ASSERT_TRUE(out.open(path, true));
    J1939_frame sent;
    const uint8_t payload[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    ASSERT_TRUE(sent.setFrom(0x18FEF100, payload, 8));
    sent.time_ns_ = 1234567890;
    EXPECT_TRUE(out.put(sent));
    out.close();

    TxtStream in;
    ASSERT_TRUE(in.open(path));
    J1939_frame received;
    ASSERT_TRUE(in.get(&received));
    EXPECT_EQ(received.dlc_, 8);
    EXPECT_EQ(received.buffer_[0], 1);
}

TEST_F(CanStreamTest, AscStreamPutWritesAndReadsBackAFrame) {
    std::string path = writeTemp("");
    AscStream out;
    ASSERT_TRUE(out.open(path, true));
    J1939_frame sent;
    const uint8_t payload[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    ASSERT_TRUE(sent.setFrom(0x18FEF100, payload, 8));
    sent.time_ns_ = 1234567890;
    EXPECT_TRUE(out.put(sent));
    out.close();

    AscStream in;
    ASSERT_TRUE(in.open(path));
    J1939_frame received;
    ASSERT_TRUE(in.get(&received));
    EXPECT_EQ(received.dlc_, 8);
    EXPECT_EQ(received.buffer_[0], 1);
    EXPECT_EQ(received.buffer_[7], 8);
}

TEST_F(CanStreamTest, OutStreamPutWritesAndReadsBackAFrame) {
    std::string path = writeTemp("");
    OutStream out;
    ASSERT_TRUE(out.open(path, true));
    J1939_frame sent;
    const uint8_t payload[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    ASSERT_TRUE(sent.setFrom(0x0CF00400, payload, 8));
    EXPECT_TRUE(out.put(sent));
    out.close();

    OutStream in;
    ASSERT_TRUE(in.open(path));
    J1939_frame received;
    ASSERT_TRUE(in.get(&received));
    EXPECT_EQ(received.dlc_, 8);
    EXPECT_EQ(received.buffer_[0], 1);
    EXPECT_EQ(received.buffer_[7], 8);
}

TEST_F(CanStreamTest, TrcStreamPutWritesAndReadsBackAFrame) {
    std::string path = writeTemp("");
    TrcStream out;
    ASSERT_TRUE(out.open(path, true));
    J1939_frame sent;
    const uint8_t payload[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    ASSERT_TRUE(sent.setFrom(0x1CFF6A27, payload, 8));
    sent.time_ns_ = 3099800000; // 3099.8 ms
    EXPECT_TRUE(out.put(sent));
    out.close();

    TrcStream in;
    ASSERT_TRUE(in.open(path));
    J1939_frame received;
    ASSERT_TRUE(in.get(&received));
    EXPECT_EQ(received.dlc_, 8);
    EXPECT_EQ(received.buffer_[0], 1);
    EXPECT_EQ(received.buffer_[7], 8);
}

TEST_F(CanStreamTest, TrcStreamRejectsDataLineWithTooFewFields) {
    J1939_frame frame;
    bool ok = decodeFirstDataLine<TrcStream>(
        ";   Start time: 5/29/2019 14:23:52.777.0\n"
        "     1)      3099.8  Rx\n", &frame);
    EXPECT_FALSE(ok);
}

TEST_F(CanStreamTest, TrcStreamRejectsUnparsableHexToken) {
    // Declared length matches the field count, but a data token itself isn't valid
    // hex -- a different rejection path than a too-large/too-short declared length.
    J1939_frame frame;
    bool ok = decodeFirstDataLine<TrcStream>(
        ";   Start time: 5/29/2019 14:23:52.777.0\n"
        "     1)      3099.8  Rx     1CFF6A27  2  ZZ ZZ\n", &frame);
    EXPECT_FALSE(ok);
}

TEST_F(CanStreamTest, LogStreamRejectsInvalidHexInDataString) {
    J1939_frame frame;
    bool ok = decodeFirstDataLine<LogStream>(
        "(1557265716.818982) can0 0CFF0686#F2ZZ\n", &frame);
    EXPECT_FALSE(ok);
}

TEST_F(CanStreamTest, LogStreamRejectsDataLongerThanMaxDlc) {
    // Constructs a data string with more than 2*max_dlc_ hex characters so
    // decodeHexString's count > max_dlc_ guard is the one that rejects it (not a
    // truncated/malformed line).
    std::string data(2 * (J1939_frame::max_dlc_ + 8), 'F');
    J1939_frame frame;
    bool ok = decodeFirstDataLine<LogStream>(
        "(1557265716.818982) can0 0CFF0686#" + data + "\n", &frame);
    EXPECT_FALSE(ok);
}

TEST_F(CanStreamTest, CanStreamForDispatchesByExtension) {
    EXPECT_NE(dynamic_cast<AscStream *>(CanStreamFor("sample.asc").get()), nullptr);
    EXPECT_NE(dynamic_cast<LogStream *>(CanStreamFor("sample.log").get()), nullptr);
    EXPECT_NE(dynamic_cast<OutStream *>(CanStreamFor("sample.out").get()), nullptr);
    EXPECT_NE(dynamic_cast<TrcStream *>(CanStreamFor("sample.trc").get()), nullptr);
    EXPECT_NE(dynamic_cast<TxtStream *>(CanStreamFor("sample.txt").get()), nullptr);
}

TEST_F(CanStreamTest, CanStreamForFallsBackToSocketCanForUnknownExtension) {
    auto stream = CanStreamFor("can0");
    ASSERT_TRUE(stream);
    // Not one of the file formats, and not a *file*-backed stream at all.
    EXPECT_EQ(dynamic_cast<AscStream *>(stream.get()), nullptr);
    EXPECT_EQ(dynamic_cast<LogStream *>(stream.get()), nullptr);
    // realPort() only flips true once open() runs (SocketCanStream sets it as the
    // first statement in open(), before attempting the actual socket/bind syscalls,
    // so this holds regardless of whether open() itself succeeds in this sandbox).
    stream->open("can0");
    EXPECT_TRUE(stream->realPort());
}

namespace {
// Minimal CanStream subclass that leaves writable_ at its default (false), used to
// exercise CanStream::open()'s write-gating in isolation now that every concrete
// file format supports put().
class NonWritableStream : public CanStream {
public:
    bool put(const J1939_frame & /*frame*/) const override { return false; }
    bool get(J1939_frame * /*frame*/) override { return false; }
};
}  // namespace

TEST_F(CanStreamTest, OpenRejectsWriteOnNonWritableFormat) {
    NonWritableStream stream;
    std::string path = writeTemp("");
    EXPECT_FALSE(stream.open(path, true));
}

TEST_F(CanStreamTest, OpenFailsForNonexistentFile) {
    LogStream stream;
    EXPECT_FALSE(stream.open("/nonexistent/path/does_not_exist.log"));
}

TEST_F(CanStreamTest, RewindReturnsToTheStartOfAFileBackedStream) {
    std::string path = writeTemp(
        "(1557265716.818982) can0 0CFF0686#F22700F2FFFFFF5F\n"
        "(1557265716.869006) can0 0CFF0686#F22700F2FFFFFF5F\n");
    LogStream stream;
    ASSERT_TRUE(stream.open(path));
    J1939_frame frame;
    ASSERT_TRUE(stream.get(&frame));
    ASSERT_TRUE(stream.get(&frame));
    // A trailing newline on the last line means feof() isn't set by the read that
    // returns it -- it takes one more (failing) read attempt to hit real EOF.
    ASSERT_FALSE(stream.get(&frame));
    ASSERT_FALSE(stream.dataAvailable());
    stream.rewind();
    EXPECT_TRUE(stream.get(&frame));
}

TEST_F(CanStreamTest, DataAvailableIsFalseAfterClose) {
    std::string path = writeTemp("(1557265716.818982) can0 0CFF0686#F22700F2FFFFFF5F\n");
    LogStream stream;
    ASSERT_TRUE(stream.open(path));
    EXPECT_TRUE(stream.ready());
    stream.close();
    EXPECT_FALSE(stream.dataAvailable());
}

// Exercises MockCanStream's own utility behaviors that no other test happens to hit.
TEST(MockCanStreamTest, SupportsFailureInjectionAndLifecycleMethods) {
    MockCanStream stream;

    stream.failNextOpen();
    EXPECT_FALSE(stream.open("whatever"));
    // The failure is one-shot: the next open() succeeds normally.
    EXPECT_TRUE(stream.open("whatever"));

    J1939_frame frame;
    frame.reset();
    EXPECT_FALSE(stream.get(&frame)); // empty queue

    stream.failPut(true);
    EXPECT_FALSE(stream.put(frame));
    stream.failPut(false);
    EXPECT_TRUE(stream.put(frame));
    ASSERT_EQ(stream.written().size(), 1u);

    EXPECT_EQ(stream.rewindCount(), 0u);
    stream.rewind();
    EXPECT_EQ(stream.rewindCount(), 1u);

    stream.close();
    EXPECT_FALSE(stream.dataAvailable());
}
