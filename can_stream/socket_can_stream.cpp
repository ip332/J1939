//
// Created by igor on 12/16/21.
//

#include "socket_can_stream.h"

#include <cstdio>
#include <cstring>
#include <chrono>

bool SocketCanStream::open(const std::string &port, bool /*write*/) {
    if (fd_ != -1) {
        api_->close(fd_);
    }
    real_port = true;
    fd_ = api_->socket();
    if (fd_ < 0) {
        perror("Socket failed: ");
        return false;
    }

    unsigned int ifindex = api_->ifNameToIndex(port);
    if (!ifindex) {
        perror("if_nametoindex failed:");
        api_->close(fd_);
        fd_ = -1;
        return false;
    }

    if (api_->bind(fd_, ifindex) < 0) {
        perror("Bind failed: ");
        api_->close(fd_);
        fd_ = -1;
        return false;
    }
    return true;
}

bool SocketCanStream::put(const struct J1939_frame &frame) const {
    if (fd_ == -1) {
        return false;
    }
    if (frame.dlc_ > 8) {
        return false;
    }
    canfd_frame canfd = {};
    canfd.can_id = frame.canID();
    canfd.len = frame.dlc_;
    memcpy(&canfd.data[0], frame.buffer_, canfd.len);

    int mtu = 16; // CAN ID + 4 padding bytes + 8 bytes of data.
    ssize_t ret = api_->write(fd_, canfd, mtu);
    if (ret != mtu) {
        perror("Write failed: ");
        return false;
    }
    return true;
}

bool SocketCanStream::get(J1939_frame *frame) {
    if (fd_ == -1) {
        return false;
    }
    struct canfd_frame canfd = {};
    ssize_t ret = api_->read(fd_, &canfd, sizeof(canfd));
    if (ret <= 0) {
        return false;
    }
    auto now = std::chrono::system_clock::now().time_since_epoch();
    frame->time_ns_ = std::chrono::duration_cast<std::chrono::nanoseconds>(now).count();
    frame->setFrom(canfd.can_id, canfd.data, canfd.len);
    return true;
}
