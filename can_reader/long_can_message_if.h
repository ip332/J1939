//
// Created by igor on 12/17/21.
//

#ifndef J1939_LONG_CAN_MESSAGE_IF_H
#define J1939_LONG_CAN_MESSAGE_IF_H

#include "j1939_frame.h"

#include <memory>

// This class defines an interface to a class which implements a specific protocol to transport more than 8 bytes.
class LongCanMessageIf {
    // Resets the container
    virtual void reset(const J1939_frame &frame) = 0;

protected:
    // The data buffer and the message structure.
    J1939_frame message_;
public:
    // Checks if the provided frame is a part of the Transport Protocol.
    static bool isTpmFrame(const J1939_frame &frame) {
        if (frame.pgn_ == 0xEB00) return true;
        if (frame.pgn_ != 0xEC00) return false;
        if ((frame.buffer_[0] == 32) || // BAM
            (frame.buffer_[0] == 16) ) { // RTS
            return true;
        }
        return false;
    }

    // Checks if the provided frame represents fast-packet protocol (NMEA2000)
    static bool isFastPacketFrame(const J1939_frame &frame);

    // Handles a data frame and returns the complete message once all the pieces were collected.
    virtual std::unique_ptr<J1939_frame> handleDataFrame(const J1939_frame &frame) = 0;
};
#endif //J1939_LONG_CAN_MESSAGE_IF_H
