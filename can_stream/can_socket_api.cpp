//
// Real syscall implementation of CanSocketApi. Diagnostics via perror() live here
// (rather than in SocketCanStream) so errno is read immediately after the syscall
// that set it, right where it's still meaningful.
//

#include "can_socket_api.h"

#include <cstdio>
#include <cstring>
#include <net/if.h>
#include <sys/socket.h>
#include <unistd.h>

int RealCanSocketApi::socket() {
    int fd = ::socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (fd < 0) {
        perror("Socket failed: ");
    }
    return fd;
}

unsigned int RealCanSocketApi::ifNameToIndex(const std::string &name) {
    struct ifreq ifr;
    strncpy(ifr.ifr_name, name.c_str(), IFNAMSIZ - 1);
    ifr.ifr_name[IFNAMSIZ - 1] = '\0';
    unsigned int ifindex = if_nametoindex(ifr.ifr_name);
    if (!ifindex) {
        perror("if_nametoindex failed:");
    }
    return ifindex;
}

int RealCanSocketApi::bind(int fd, unsigned int ifindex) {
    struct sockaddr_can addr;
    memset(&addr, 0, sizeof(addr));
    addr.can_family = AF_CAN;
    addr.can_ifindex = ifindex;
    int ret = ::bind(fd, (struct sockaddr *) &addr, sizeof(addr));
    if (ret < 0) {
        perror("Bind failed: ");
    }
    return ret;
}

ssize_t RealCanSocketApi::write(int fd, const struct canfd_frame &frame, size_t len) {
    ssize_t ret = ::write(fd, &frame, len);
    if (ret != static_cast<ssize_t>(len)) {
        perror("Write failed: ");
    }
    return ret;
}

ssize_t RealCanSocketApi::read(int fd, struct canfd_frame *frame, size_t len) {
    return ::read(fd, frame, len);
}

void RealCanSocketApi::close(int fd) {
    ::close(fd);
}
