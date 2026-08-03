//
// Thin seam wrapping the raw CAN-socket syscalls SocketCanStream depends on (socket(),
// if_nametoindex(), bind(), write(), read(), close()). Lets tests substitute a mock
// instead of needing a real (or virtual) CAN interface and CAP_NET_ADMIN.
//

#ifndef J1939_CAN_SOCKET_API_H
#define J1939_CAN_SOCKET_API_H

#include <linux/can.h>
#include <string>
#include <sys/types.h>

class CanSocketApi {
public:
    virtual ~CanSocketApi() {}

    // Returns the new socket's fd, or -1 on failure.
    virtual int socket() = 0;
    // Returns the interface index for `name`, or 0 if it doesn't exist.
    virtual unsigned int ifNameToIndex(const std::string &name) = 0;
    // Binds `fd` to the interface `ifindex`. Returns 0 on success, <0 on failure.
    virtual int bind(int fd, unsigned int ifindex) = 0;
    virtual ssize_t write(int fd, const struct canfd_frame &frame, size_t len) = 0;
    virtual ssize_t read(int fd, struct canfd_frame *frame, size_t len) = 0;
    virtual void close(int fd) = 0;
};

// Default implementation, backed by the real POSIX/SocketCAN syscalls.
class RealCanSocketApi : public CanSocketApi {
public:
    int socket() override;
    unsigned int ifNameToIndex(const std::string &name) override;
    int bind(int fd, unsigned int ifindex) override;
    ssize_t write(int fd, const struct canfd_frame &frame, size_t len) override;
    ssize_t read(int fd, struct canfd_frame *frame, size_t len) override;
    void close(int fd) override;
};

#endif //J1939_CAN_SOCKET_API_H
