//
// Tests for LongCanMessage (J1939-21 transport protocol multi-frame reassembly),
// exercised directly rather than through TransportProtocol/CanSandbox.
//

#include "long_can_message.h"

#include <gtest/gtest.h>

namespace {

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

J1939_frame bamFrame(uint8_t src, uint16_t size, uint8_t frames_cnt, uint32_t pgn) {
    uint8_t data[8] = {
        32, // BAM
        static_cast<uint8_t>(size & 0xFF), static_cast<uint8_t>((size >> 8) & 0xFF),
        frames_cnt, 0xFF,
        static_cast<uint8_t>(pgn & 0xFF), static_cast<uint8_t>((pgn >> 8) & 0xFF),
        static_cast<uint8_t>((pgn >> 16) & 0xFF),
    };
    return makeFrame(0xEC00, src, data);
}

J1939_frame dtFrame(uint8_t src, uint8_t seq, const uint8_t payload[7]) {
    uint8_t data[8];
    data[0] = seq;
    memcpy(&data[1], payload, 7);
    return makeFrame(0xEB00, src, data);
}

} // namespace

TEST(LongCanMessageTest, RejectsInvalidFrameCount) {
    LongCanMessage msg;
    auto result = msg.handleDataFrame(bamFrame(0x10, 10, /*frames_cnt=*/0, 0xFEF1));
    EXPECT_FALSE(result);
    // Once rejected, a subsequent data frame is also rejected (error_ latched).
    const uint8_t payload[7] = {1, 2, 3, 4, 5, 6, 7};
    result = msg.handleDataFrame(dtFrame(0x10, 1, payload));
    EXPECT_FALSE(result);
}

TEST(LongCanMessageTest, RejectsSizeLargerThanMaxDlc) {
    LongCanMessage msg;
    auto result = msg.handleDataFrame(bamFrame(0x10, J1939_frame::max_dlc_ + 1, 1, 0xFEF1));
    EXPECT_FALSE(result);
}

TEST(LongCanMessageTest, RejectsFrameCountThatWouldOverflowBuffer) {
    LongCanMessage msg;
    // A tiny declared size but a huge frame count -- the offset math
    // (frames_cnt-1)*7+7 must not be allowed to exceed max_dlc_.
    auto result = msg.handleDataFrame(bamFrame(0x10, 7, 255, 0xFEF1));
    EXPECT_FALSE(result);
}

TEST(LongCanMessageTest, RejectsOutOfSequenceFrame) {
    LongCanMessage msg;
    msg.handleDataFrame(bamFrame(0x10, 14, 2, 0xFEF1));
    const uint8_t payload[7] = {1, 2, 3, 4, 5, 6, 7};
    // Sends sequence 2 first instead of 1.
    auto result = msg.handleDataFrame(dtFrame(0x10, 2, payload));
    EXPECT_FALSE(result);
}

TEST(LongCanMessageTest, RejectsFrameFromWrongSource) {
    LongCanMessage msg;
    msg.handleDataFrame(bamFrame(0x10, 14, 2, 0xFEF1));
    const uint8_t payload[7] = {1, 2, 3, 4, 5, 6, 7};
    auto result = msg.handleDataFrame(dtFrame(0x22, 1, payload));
    EXPECT_FALSE(result);
}

TEST(LongCanMessageTest, ReassemblesThreeFrameTransfer) {
    LongCanMessage msg;
    ASSERT_FALSE(msg.handleDataFrame(bamFrame(0x10, 18, 3, 0xFEF1)));
    const uint8_t p1[7] = {'A', 'B', 'C', 'D', 'E', 'F', 'G'};
    const uint8_t p2[7] = {'H', 'I', 'J', 'K', 'L', 'M', 'N'};
    const uint8_t p3[7] = {'O', 'P', 0, 0, 0, 0, 0};
    ASSERT_FALSE(msg.handleDataFrame(dtFrame(0x10, 1, p1)));
    ASSERT_FALSE(msg.handleDataFrame(dtFrame(0x10, 2, p2)));
    auto result = msg.handleDataFrame(dtFrame(0x10, 3, p3));
    ASSERT_TRUE(result);
    EXPECT_EQ(result->dlc_, 18);
    EXPECT_EQ(result->pgn_, 0x00FEF1u);
    EXPECT_EQ(memcmp(result->buffer_, "ABCDEFGHIJKLMNOP", 16), 0);
}

TEST(LongCanMessageIfTest, IsTpmFrameRecognizesRts) {
    const uint8_t data[8] = {16, 10, 0, 2, 0xFF, 0xF1, 0xFE, 0x00}; // RTS
    EXPECT_TRUE(LongCanMessageIf::isTpmFrame(makeFrame(0xEC00, 0x10, data)));
}

TEST(LongCanMessageIfTest, IsTpmFrameRejectsOtherControlBytes) {
    const uint8_t data[8] = {17, 10, 0, 2, 0xFF, 0xF1, 0xFE, 0x00}; // CTS, not BAM/RTS
    EXPECT_FALSE(LongCanMessageIf::isTpmFrame(makeFrame(0xEC00, 0x10, data)));
}
