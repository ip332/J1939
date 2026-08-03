//
// Direct tests for RealCanSocketApi. Most of its methods (socket()/bind()/write()/
// read()) only do anything meaningful against a real CAN socket, which needs a live
// vcan0 interface and CAP_NET_ADMIN -- see socket_can_stream_tests.cpp/vcan_tests for
// that. The two cases below are the exceptions: genuinely deterministic and
// privilege-free in any environment, so they're tested for real here instead.
//

#include "can_socket_api.h"

#include <gtest/gtest.h>

TEST(RealCanSocketApiTest, IfNameToIndexReturnsZeroForAnUnknownInterface) {
    RealCanSocketApi api;
    EXPECT_EQ(api.ifNameToIndex("definitely_not_a_real_iface_xyz"), 0u);
}

TEST(RealCanSocketApiTest, CloseOnAnInvalidFdDoesNotCrash) {
    RealCanSocketApi api;
    api.close(-1);
}
