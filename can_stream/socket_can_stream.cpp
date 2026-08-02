//
// Created by igor on 12/16/21.
//

#include "socket_can_stream.h"

#include <cstdio>
#include <cstring>
#include <sys/socket.h>
#include <net/if.h>
#include <unistd.h>
#include <chrono>

bool SocketCanStream::open(const std::string &port, bool /*write*/) {
    if (fd_ != -1) {
        ::close(fd_);
    }
    real_port = true;
    // Create the socket
    fd_ = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (fd_ < 0) {
        perror("Socket failed: ");
        return false;
    }

    struct ifreq ifr;
    strncpy(ifr.ifr_name, port.c_str(), IFNAMSIZ - 1);
    ifr.ifr_name[IFNAMSIZ - 1] = '\0';
    ifr.ifr_ifindex = if_nametoindex(ifr.ifr_name);
    if (!ifr.ifr_ifindex) {
        perror("if_nametoindex failed:");
        ::close(fd_);
        fd_ = -1;
        return false;
    }

    struct sockaddr_can addr;
    memset(&addr, 0, sizeof(addr));
    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;
    if (bind(fd_, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
        perror("Bind failed: ");
        ::close(fd_);
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
    int ret = write(fd_, &canfd, mtu);
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
    int ret = read(fd_, & canfd, sizeof(canfd));
    if (ret <= 0) {
        return false;
    }
    auto now = std::chrono::system_clock::now().time_since_epoch();
    frame->time_ns_ = std::chrono::duration_cast<std::chrono::nanoseconds>(now).count();
    frame->setFrom(canfd.can_id, canfd.data, canfd.len);
    return true;
}
