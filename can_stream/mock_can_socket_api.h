//
// GMock double for CanSocketApi, used to unit-test SocketCanStream's own logic (error
// handling, frame packing) without a real or virtual CAN interface.
//

#ifndef J1939_MOCK_CAN_SOCKET_API_H
#define J1939_MOCK_CAN_SOCKET_API_H

#include "can_socket_api.h"

#include <gmock/gmock.h>

class MockCanSocketApi : public CanSocketApi {
public:
    MOCK_METHOD(int, socket, (), (override));
    MOCK_METHOD(unsigned int, ifNameToIndex, (const std::string &name), (override));
    MOCK_METHOD(int, bind, (int fd, unsigned int ifindex), (override));
    MOCK_METHOD(ssize_t, write, (int fd, const struct canfd_frame &frame, size_t len), (override));
    MOCK_METHOD(ssize_t, read, (int fd, struct canfd_frame *frame, size_t len), (override));
    MOCK_METHOD(void, close, (int fd), (override));
};

#endif //J1939_MOCK_CAN_SOCKET_API_H
