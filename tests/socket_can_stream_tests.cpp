//
// Integration tests for SocketCanStream against a real (virtual) SocketCAN interface.
// Unlike the rest of the suite, these need a "vcan0" interface to exist and CAP_NET_ADMIN
// to have created it, so this binary is intentionally not part of all_tests/ctest — see
// README.md for how to run it (docker run --cap-add=NET_ADMIN ... after `ip link add dev
// vcan0 type vcan && ip link set up vcan0`).
//

#include "socket_can_stream.h"

#include <gtest/gtest.h>

namespace {
constexpr const char *kInterface = "vcan0";
}

class SocketCanStreamTest : public testing::Test {
protected:
    void SetUp() override {
        if (!sender_.open(kInterface) || !receiver_.open(kInterface)) {
            GTEST_SKIP() << "Could not open " << kInterface << " -- run this binary with "
                         << "CAP_NET_ADMIN after `ip link add dev " << kInterface
                         << " type vcan && ip link set up " << kInterface << "`.";
        }
    }

    SocketCanStream sender_;
    SocketCanStream receiver_;
};

TEST_F(SocketCanStreamTest, RoundTripsAFrameBetweenTwoSockets) {
    // Linux CAN_RAW sockets don't receive their own transmitted frames by default
    // (CAN_RAW_RECV_OWN_MSGS is off), so this needs two independent sockets on the
    // same interface -- exactly how real bus nodes see each other's traffic.
    J1939_frame sent;
    uint8_t payload[8] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
    ASSERT_TRUE(sent.setFrom(0x18FEF100, payload, 8));

    ASSERT_TRUE(sender_.put(sent));

    J1939_frame received;
    ASSERT_TRUE(receiver_.get(&received));
    EXPECT_EQ(received.canID(), sent.canID());
    EXPECT_EQ(received.src_, sent.src_);
    EXPECT_EQ(received.dlc_, sent.dlc_);
    EXPECT_EQ(memcmp(received.buffer_, sent.buffer_, sent.dlc_), 0);
}

TEST_F(SocketCanStreamTest, PutRejectsOversizedFrame) {
    J1939_frame frame;
    frame.setFrom(0x18FEF100, nullptr, 0);
    frame.dlc_ = 20; // past this format's 8-byte limit
    memset(frame.buffer_, 0xAB, frame.dlc_);
    EXPECT_FALSE(sender_.put(frame));
}

TEST(SocketCanStreamOpenTest, FailsCleanlyOnNonexistentInterface) {
    SocketCanStream stream;
    EXPECT_FALSE(stream.open("vcan9"));
}
