//
// Created by igor on 12/16/21.
//

#ifndef J1939_SOCKET_CAN_STREAM_H
#define J1939_SOCKET_CAN_STREAM_H

#include "can_socket_api.h"
#include "can_stream.h"

#include <memory>

// This class defines and implements the interface to a CAN port.
class SocketCanStream : public CanStream {
    int fd_ = -1;
    std::unique_ptr<CanSocketApi> owned_api_;
    CanSocketApi *api_;
public:
    SocketCanStream() : owned_api_(std::make_unique<RealCanSocketApi>()), api_(owned_api_.get()) {}
    // Test-only seam: inject a fake/mock CanSocketApi instead of making real syscalls.
    // The pointer is not owned; the caller must keep it alive for this object's lifetime.
    explicit SocketCanStream(CanSocketApi *api) : api_(api) {}

    // Opens the port and initializes the fd_;
    bool open(const std::string &port, bool write = false) override;
    void close() override {
        if (fd_ != -1) {
            api_->close(fd_);
            fd_ = -1;
        }
    }

    bool dataAvailable() const override { return fd_ != -1; }

    bool put(const J1939_frame &frame) const override;
    bool get(J1939_frame *frame) override;

    int fd() const { return fd_; }
};


#endif //J1939_SOCKET_CAN_STREAM_H
