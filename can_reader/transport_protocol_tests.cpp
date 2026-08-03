//
// Tests for TransportProtocol's dispatch logic (routing frames to LongCanMessage vs.
// FastPacketMessage vs. neither, keyed by source address).
//

#include "transport_protocol.h"

#include <gtest/gtest.h>

namespace {

constexpr uint32_t kFastPgn = 129029;

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

} // namespace

TEST(TransportProtocolTest, RoutesFastPacketFramesAndReassembles) {
    TransportProtocol tp;
    const uint8_t first[8] = {0, 10, 'A', 'B', 'C', 'D', 'E', 'F'}; // seq=0, size=10
    ASSERT_FALSE(tp.handleCanFrame(makeFrame(kFastPgn, 0x30, first)));
    const uint8_t cont[8] = {1, 'G', 'H', 'I', 'J', 0, 0, 0}; // frame index 1, same seq
    auto result = tp.handleCanFrame(makeFrame(kFastPgn, 0x30, cont));
    ASSERT_TRUE(result);
    EXPECT_EQ(result->dlc_, 10);
    EXPECT_EQ(memcmp(result->buffer_, "ABCDEFGHIJ", 10), 0);
}

TEST(TransportProtocolTest, ReturnsNullForFramesThatAreNeitherTpmNorFastPacket) {
    TransportProtocol tp;
    const uint8_t data[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    auto result = tp.handleCanFrame(makeFrame(0x00FEF100, 0x30, data));
    EXPECT_FALSE(result);
    EXPECT_FALSE(TransportProtocol::longMessage(makeFrame(0x00FEF100, 0x30, data)));
}

TEST(TransportProtocolTest, TracksSeparateSourcesIndependently) {
    TransportProtocol tp;
    // Two sources each mid-way through their own BAM transfer must not interfere.
    const uint8_t bam[8] = {32, 10, 0, 2, 0xFF, 0xF1, 0xFE, 0x00};
    ASSERT_FALSE(tp.handleCanFrame(makeFrame(0xEC00, 0x10, bam)));
    ASSERT_FALSE(tp.handleCanFrame(makeFrame(0xEC00, 0x20, bam)));

    const uint8_t dt1[8] = {1, 'A', 'B', 'C', 'D', 'E', 'F', 'G'};
    const uint8_t dt2[8] = {2, 'H', 'I', 0, 0, 0, 0, 0};
    ASSERT_FALSE(tp.handleCanFrame(makeFrame(0xEB00, 0x10, dt1)));
    ASSERT_FALSE(tp.handleCanFrame(makeFrame(0xEB00, 0x20, dt1)));

    auto result10 = tp.handleCanFrame(makeFrame(0xEB00, 0x10, dt2));
    auto result20 = tp.handleCanFrame(makeFrame(0xEB00, 0x20, dt2));
    ASSERT_TRUE(result10);
    ASSERT_TRUE(result20);
    EXPECT_EQ(memcmp(result10->buffer_, "ABCDEFGHI", 9), 0);
    EXPECT_EQ(memcmp(result20->buffer_, "ABCDEFGHI", 9), 0);
}

TEST(TransportProtocolTest, LongMessageStaticHelperCoversBothProtocols) {
    const uint8_t bam[8] = {32, 10, 0, 2, 0xFF, 0xF1, 0xFE, 0x00};
    EXPECT_TRUE(TransportProtocol::longMessage(makeFrame(0xEC00, 0x10, bam)));
    const uint8_t fastData[8] = {0, 4, 'A', 'B', 'C', 'D', 0, 0};
    EXPECT_TRUE(TransportProtocol::longMessage(makeFrame(kFastPgn, 0x10, fastData)));
    const uint8_t plain[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    EXPECT_FALSE(TransportProtocol::longMessage(makeFrame(0x00FEF100, 0x10, plain)));
}
