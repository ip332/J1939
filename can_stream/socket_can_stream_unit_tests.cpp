//
// Unit tests for SocketCanStream's own logic (open()/put()/get()/close() branching and
// frame encoding), using MockCanSocketApi in place of real syscalls. Unlike
// socket_can_stream_tests.cpp (integration tests against a real vcan0 interface), these
// need no CAP_NET_ADMIN or virtual interface and run as part of all_tests.
//

#include "mock_can_socket_api.h"
#include "socket_can_stream.h"

#include <gtest/gtest.h>

using ::testing::_;
using ::testing::DoAll;
using ::testing::Return;
using ::testing::SaveArg;
using ::testing::SetArgPointee;

namespace {
constexpr uint32_t kExtendedCanId = 0x98FEF211; // CAN_EFF_FLAG set, PDU2 PGN 0xFEF2, SA 0x11
}

TEST(SocketCanStreamOpenTest, SucceedsAndBindsToTheRequestedInterface) {
    MockCanSocketApi api;
    EXPECT_CALL(api, socket()).WillOnce(Return(5));
    EXPECT_CALL(api, ifNameToIndex(std::string("vcan0"))).WillOnce(Return(7u));
    EXPECT_CALL(api, bind(5, 7u)).WillOnce(Return(0));

    SocketCanStream stream(&api);
    EXPECT_TRUE(stream.open("vcan0"));
}

TEST(SocketCanStreamOpenTest, FailsWhenSocketCreationFails) {
    MockCanSocketApi api;
    EXPECT_CALL(api, socket()).WillOnce(Return(-1));
    EXPECT_CALL(api, ifNameToIndex(_)).Times(0);
    EXPECT_CALL(api, bind(_, _)).Times(0);
    EXPECT_CALL(api, close(_)).Times(0);

    SocketCanStream stream(&api);
    EXPECT_FALSE(stream.open("vcan0"));
}

TEST(SocketCanStreamOpenTest, FailsAndClosesSocketWhenInterfaceIsUnknown) {
    MockCanSocketApi api;
    EXPECT_CALL(api, socket()).WillOnce(Return(5));
    EXPECT_CALL(api, ifNameToIndex(std::string("bad0"))).WillOnce(Return(0u));
    EXPECT_CALL(api, bind(_, _)).Times(0);
    EXPECT_CALL(api, close(5)).Times(1);

    SocketCanStream stream(&api);
    EXPECT_FALSE(stream.open("bad0"));
}

TEST(SocketCanStreamOpenTest, FailsAndClosesSocketWhenBindFails) {
    MockCanSocketApi api;
    EXPECT_CALL(api, socket()).WillOnce(Return(5));
    EXPECT_CALL(api, ifNameToIndex(std::string("vcan0"))).WillOnce(Return(7u));
    EXPECT_CALL(api, bind(5, 7u)).WillOnce(Return(-1));
    EXPECT_CALL(api, close(5)).Times(1);

    SocketCanStream stream(&api);
    EXPECT_FALSE(stream.open("vcan0"));
}

TEST(SocketCanStreamOpenTest, ClosesAnyPreviouslyOpenSocketBeforeReopening) {
    MockCanSocketApi api;
    {
        ::testing::InSequence seq;
        EXPECT_CALL(api, socket()).WillOnce(Return(5));
        EXPECT_CALL(api, ifNameToIndex(std::string("vcan0"))).WillOnce(Return(7u));
        EXPECT_CALL(api, bind(5, 7u)).WillOnce(Return(0));
        EXPECT_CALL(api, close(5)).Times(1);
        EXPECT_CALL(api, socket()).WillOnce(Return(9));
        EXPECT_CALL(api, ifNameToIndex(std::string("vcan1"))).WillOnce(Return(8u));
        EXPECT_CALL(api, bind(9, 8u)).WillOnce(Return(0));
    }

    SocketCanStream stream(&api);
    ASSERT_TRUE(stream.open("vcan0"));
    EXPECT_TRUE(stream.open("vcan1"));
}

TEST(SocketCanStreamTest, DataAvailableReflectsWhetherTheSocketIsOpen) {
    MockCanSocketApi api;
    EXPECT_CALL(api, socket()).WillOnce(Return(5));
    EXPECT_CALL(api, ifNameToIndex(_)).WillOnce(Return(7u));
    EXPECT_CALL(api, bind(_, _)).WillOnce(Return(0));

    SocketCanStream stream(&api);
    EXPECT_FALSE(stream.dataAvailable());
    ASSERT_TRUE(stream.open("vcan0"));
    EXPECT_TRUE(stream.dataAvailable());
}

TEST(SocketCanStreamPutTest, ReturnsFalseWhenNotOpen) {
    MockCanSocketApi api;
    EXPECT_CALL(api, write(_, _, _)).Times(0);

    SocketCanStream stream(&api);
    J1939_frame frame;
    frame.reset();
    frame.dlc_ = 8;
    EXPECT_FALSE(stream.put(frame));
}

TEST(SocketCanStreamPutTest, RejectsOversizedFrame) {
    MockCanSocketApi api;
    EXPECT_CALL(api, socket()).WillOnce(Return(5));
    EXPECT_CALL(api, ifNameToIndex(_)).WillOnce(Return(7u));
    EXPECT_CALL(api, bind(_, _)).WillOnce(Return(0));
    EXPECT_CALL(api, write(_, _, _)).Times(0);

    SocketCanStream stream(&api);
    ASSERT_TRUE(stream.open("vcan0"));
    J1939_frame frame;
    frame.reset();
    frame.dlc_ = 20; // past this format's 8-byte limit
    EXPECT_FALSE(stream.put(frame));
}

TEST(SocketCanStreamPutTest, EncodesFrameAndWritesExpectedBytes) {
    MockCanSocketApi api;
    EXPECT_CALL(api, socket()).WillOnce(Return(5));
    EXPECT_CALL(api, ifNameToIndex(_)).WillOnce(Return(7u));
    EXPECT_CALL(api, bind(_, _)).WillOnce(Return(0));

    canfd_frame captured{};
    EXPECT_CALL(api, write(5, _, 16))
        .WillOnce(DoAll(SaveArg<1>(&captured), Return(16)));

    SocketCanStream stream(&api);
    ASSERT_TRUE(stream.open("vcan0"));

    uint8_t payload[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    J1939_frame frame;
    ASSERT_TRUE(frame.setFrom(kExtendedCanId, payload, 8));

    EXPECT_TRUE(stream.put(frame));
    EXPECT_EQ(captured.can_id, frame.canID());
    EXPECT_EQ(captured.len, frame.dlc_);
    EXPECT_EQ(memcmp(captured.data, frame.buffer_, frame.dlc_), 0);
}

TEST(SocketCanStreamPutTest, ReturnsFalseWhenWriteReturnsTheWrongByteCount) {
    MockCanSocketApi api;
    EXPECT_CALL(api, socket()).WillOnce(Return(5));
    EXPECT_CALL(api, ifNameToIndex(_)).WillOnce(Return(7u));
    EXPECT_CALL(api, bind(_, _)).WillOnce(Return(0));
    EXPECT_CALL(api, write(_, _, 16)).WillOnce(Return(8)); // short/partial write

    SocketCanStream stream(&api);
    ASSERT_TRUE(stream.open("vcan0"));
    uint8_t payload[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    J1939_frame frame;
    ASSERT_TRUE(frame.setFrom(kExtendedCanId, payload, 8));
    EXPECT_FALSE(stream.put(frame));
}

TEST(SocketCanStreamGetTest, ReturnsFalseWhenNotOpen) {
    MockCanSocketApi api;
    EXPECT_CALL(api, read(_, _, _)).Times(0);

    SocketCanStream stream(&api);
    J1939_frame frame;
    EXPECT_FALSE(stream.get(&frame));
}

TEST(SocketCanStreamGetTest, ReturnsFalseWhenReadFails) {
    MockCanSocketApi api;
    EXPECT_CALL(api, socket()).WillOnce(Return(5));
    EXPECT_CALL(api, ifNameToIndex(_)).WillOnce(Return(7u));
    EXPECT_CALL(api, bind(_, _)).WillOnce(Return(0));
    EXPECT_CALL(api, read(5, _, sizeof(canfd_frame))).WillOnce(Return(0));

    SocketCanStream stream(&api);
    ASSERT_TRUE(stream.open("vcan0"));
    J1939_frame frame;
    EXPECT_FALSE(stream.get(&frame));
}

TEST(SocketCanStreamGetTest, PopulatesFrameFromTheReadCanfdFrame) {
    MockCanSocketApi api;
    EXPECT_CALL(api, socket()).WillOnce(Return(5));
    EXPECT_CALL(api, ifNameToIndex(_)).WillOnce(Return(7u));
    EXPECT_CALL(api, bind(_, _)).WillOnce(Return(0));

    canfd_frame incoming{};
    incoming.can_id = kExtendedCanId;
    incoming.len = 3;
    incoming.data[0] = 0xAA;
    incoming.data[1] = 0xBB;
    incoming.data[2] = 0xCC;
    EXPECT_CALL(api, read(5, _, sizeof(canfd_frame)))
        .WillOnce(DoAll(SetArgPointee<1>(incoming), Return(16)));

    SocketCanStream stream(&api);
    ASSERT_TRUE(stream.open("vcan0"));
    J1939_frame frame;
    ASSERT_TRUE(stream.get(&frame));
    EXPECT_EQ(frame.canID(), incoming.can_id);
    EXPECT_EQ(frame.dlc_, incoming.len);
    EXPECT_EQ(memcmp(frame.buffer_, incoming.data, incoming.len), 0);
    EXPECT_GT(frame.time_ns_, 0u);
}

TEST(SocketCanStreamCloseTest, ClosesTheSocketOnceAndIsIdempotent) {
    MockCanSocketApi api;
    EXPECT_CALL(api, socket()).WillOnce(Return(5));
    EXPECT_CALL(api, ifNameToIndex(_)).WillOnce(Return(7u));
    EXPECT_CALL(api, bind(_, _)).WillOnce(Return(0));
    EXPECT_CALL(api, close(5)).Times(1); // exactly once, even though close() is called twice below

    SocketCanStream stream(&api);
    ASSERT_TRUE(stream.open("vcan0"));
    stream.close();
    stream.close(); // fd_ already -1, must not call the API again
}
