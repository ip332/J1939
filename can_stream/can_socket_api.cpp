//
// Real syscall implementation of CanSocketApi. Pure straight-line passthrough to the
// underlying syscalls, with no branching/error-diagnostic logic of its own -- that
// lives in SocketCanStream (the caller), which is what all the failure-path branch
// coverage exercises via MockCanSocketApi. This keeps errno reads right next to the
// syscall that set it (no other syscall happens between the two) while leaving this
// class with nothing left to miss coverage-wise beyond exercising each call once.
//
// bind()/write()/read() only do anything meaningful against a real CAN socket bound
// to a live interface, so they're exercised by socket_can_stream_tests.cpp against
// vcan0 (needs CAP_NET_ADMIN -- see the vcan_tests CMake target) rather than here.
//

#include "can_socket_api.h"

#include <cstring>
#include <net/if.h>
#include <sys/socket.h>
#include <unistd.h>

int RealCanSocketApi::socket() {
    return ::socket(PF_CAN, SOCK_RAW, CAN_RAW);
}

unsigned int RealCanSocketApi::ifNameToIndex(const std::string &name) {
    struct ifreq ifr;
    strncpy(ifr.ifr_name, name.c_str(), IFNAMSIZ - 1);
    ifr.ifr_name[IFNAMSIZ - 1] = '\0';
    return if_nametoindex(ifr.ifr_name);
}

int RealCanSocketApi::bind(int fd, unsigned int ifindex) {
    struct sockaddr_can addr;
    memset(&addr, 0, sizeof(addr));
    addr.can_family = AF_CAN;
    addr.can_ifindex = ifindex;
    return ::bind(fd, (struct sockaddr *) &addr, sizeof(addr));
}

ssize_t RealCanSocketApi::write(int fd, const struct canfd_frame &frame, size_t len) {
    return ::write(fd, &frame, len);
}

ssize_t RealCanSocketApi::read(int fd, struct canfd_frame *frame, size_t len) {
    return ::read(fd, frame, len);
}

void RealCanSocketApi::close(int fd) {
    ::close(fd);
}
