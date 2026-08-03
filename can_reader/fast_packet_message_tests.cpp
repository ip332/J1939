//
// Tests for FastPacketMessage (NMEA2000 fast-packet multi-frame reassembly),
// exercised directly rather than through TransportProtocol/CanSandbox.
//

#include "fast_packet_message.h"

#include <gtest/gtest.h>

namespace {

constexpr uint32_t kFastPgn = 129029; // a real entry in fast_packet_message.cpp's table

J1939_frame makeFrame(uint32_t pgn, uint8_t src, const uint8_t data[8]) {
    J1939_frame frame;
    frame.reset();
    frame.pgn_ = pgn;
    frame.dst_ = 0xFF;
    frame.src_ = src;
    frame.pri_ = 6;
    frame.dlc_ = 8;
    memcpy(frame.buffer_, data, 8);
    return frame;
}

// `seq` is the 3-bit fast-packet sequence counter (distinguishes concurrent messages
// of the same PGN), not a frame index.
J1939_frame firstFrame(uint32_t pgn, uint8_t src, uint8_t seq, uint8_t size, const char *payload) {
    uint8_t data[8] = {};
    data[0] = static_cast<uint8_t>(seq << 5);
    data[1] = size;
    memcpy(&data[2], payload, std::min<size_t>(6, strlen(payload)));
    return makeFrame(pgn, src, data);
}

J1939_frame continuationFrame(uint32_t pgn, uint8_t src, uint8_t seq, uint8_t frame_index, const char *payload) {
    uint8_t data[8] = {};
    data[0] = static_cast<uint8_t>((seq << 5) | frame_index);
    memcpy(&data[1], payload, std::min<size_t>(7, strlen(payload)));
    return makeFrame(pgn, src, data);
}

} // namespace

TEST(FastPacketMessageTest, DeliversSingleFrameMessageImmediately) {
    // Regression test: total size <= 6 fits entirely in the first frame, so there's no
    // continuation frame to ever complete it -- must be delivered right after reset().
    FastPacketMessage msg;
    auto result = msg.handleDataFrame(firstFrame(kFastPgn, 0x10, 0, 4, "ABCD"));
    ASSERT_TRUE(result);
    EXPECT_EQ(result->dlc_, 4);
    EXPECT_EQ(memcmp(result->buffer_, "ABCD", 4), 0);
}

TEST(FastPacketMessageTest, ReassemblesTwoFrameMessage) {
    FastPacketMessage msg;
    ASSERT_FALSE(msg.handleDataFrame(firstFrame(kFastPgn, 0x10, 0, 10, "ABCDEF")));
    auto result = msg.handleDataFrame(continuationFrame(kFastPgn, 0x10, 0, 1, "GHIJ"));
    ASSERT_TRUE(result);
    EXPECT_EQ(result->dlc_, 10);
    EXPECT_EQ(memcmp(result->buffer_, "ABCDEFGHIJ", 10), 0);
}

TEST(FastPacketMessageTest, ReassemblesThreeFrameMessage) {
    FastPacketMessage msg;
    ASSERT_FALSE(msg.handleDataFrame(firstFrame(kFastPgn, 0x10, 0, 18, "ABCDEF")));
    ASSERT_FALSE(msg.handleDataFrame(continuationFrame(kFastPgn, 0x10, 0, 1, "GHIJKLM")));
    auto result = msg.handleDataFrame(continuationFrame(kFastPgn, 0x10, 0, 2, "NOPQR"));
    ASSERT_TRUE(result);
    EXPECT_EQ(result->dlc_, 18);
    EXPECT_EQ(memcmp(result->buffer_, "ABCDEFGHIJKLMNOPQR", 18), 0);
}

TEST(FastPacketMessageTest, RejectsSequenceCounterMismatch) {
    FastPacketMessage msg;
    msg.handleDataFrame(firstFrame(kFastPgn, 0x10, 0, 10, "ABCDEF"));
    // Continuation claims a different sequence counter (2) than the first frame (0).
    auto result = msg.handleDataFrame(continuationFrame(kFastPgn, 0x10, 2, 1, "GHIJ"));
    EXPECT_FALSE(result);
}

TEST(FastPacketMessageTest, RejectsFrameFromWrongSource) {
    FastPacketMessage msg;
    msg.handleDataFrame(firstFrame(kFastPgn, 0x10, 0, 10, "ABCDEF"));
    auto result = msg.handleDataFrame(continuationFrame(kFastPgn, 0x22, 0, 1, "GHIJ"));
    EXPECT_FALSE(result);
}

TEST(FastPacketMessageTest, RejectsFrameWithWrongPgn) {
    FastPacketMessage msg;
    msg.handleDataFrame(firstFrame(kFastPgn, 0x10, 0, 10, "ABCDEF"));
    auto result = msg.handleDataFrame(continuationFrame(129038, 0x10, 0, 1, "GHIJ"));
    EXPECT_FALSE(result);
}

TEST(FastPacketMessageTest, StaysRejectingOnceErrored) {
    FastPacketMessage msg;
    msg.handleDataFrame(firstFrame(kFastPgn, 0x10, 0, 10, "ABCDEF"));
    ASSERT_FALSE(msg.handleDataFrame(continuationFrame(kFastPgn, 0x22, 0, 1, "GHIJ"))); // wrong source -> error_
    // Even a well-formed continuation from the right source is now rejected.
    auto result = msg.handleDataFrame(continuationFrame(kFastPgn, 0x10, 0, 1, "GHIJ"));
    EXPECT_FALSE(result);
}

TEST(LongCanMessageIfTest, IsFastPacketFrameRecognizesKnownPgn) {
    J1939_frame frame;
    frame.reset();
    frame.pgn_ = kFastPgn;
    EXPECT_TRUE(LongCanMessageIf::isFastPacketFrame(frame));
}

TEST(LongCanMessageIfTest, IsFastPacketFrameRejectsUnknownPgn) {
    J1939_frame frame;
    frame.reset();
    frame.pgn_ = 0x1234;
    EXPECT_FALSE(LongCanMessageIf::isFastPacketFrame(frame));
}
