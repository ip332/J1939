//
// Created by igor on 12/16/21.
//

#ifndef J1939_SOCKET_CAN_STREAM_H
#define J1939_SOCKET_CAN_STREAM_H

#include "can_stream.h"
#include <unistd.h>

// This class defines and implements the interface to a CAN port.
class SocketCanStream : public CanStream {
    int fd_ = -1;
public:
    // Opens the port and initializes the fd_;
    bool open(const std::string &port, bool write = false) override;
    void close() override {
        if (fd_ != -1) {
            ::close(fd_);
            fd_ = -1;
        }
    }

    bool dataAvailable() const override { return fd_ != -1; }

    bool put(const J1939_frame &frame) const override;
    bool get(J1939_frame *frame) override;

    int fd() const { return fd_; }
};


#endif //J1939_SOCKET_CAN_STREAM_H
