//
// Created by igor on 12/16/21.
//

#ifndef J1939_LONG_CAN_MESSAGE_H
#define J1939_LONG_CAN_MESSAGE_H


#include "long_can_message_if.h"

// This class represents a single multi-frame message, i.e. when the payload size is more than 8 bytes and it is
// delivered using one of the Transport Protocols (BAM, RTS?CTS, etc. - see J1939-21).
class LongCanMessage : public LongCanMessageIf {
    // Total number of bytes expected.
    uint16_t expected_size_;
    // Total number of frames expected.
    uint8_t expected_frames_cnt_;
    // Next frame index.
    uint8_t expected_frame_;
    // Error flag. Set when an out of order packet is received.
    bool error_;

    void reset(const J1939_frame &frame) override;
public:
    std::unique_ptr<J1939_frame> handleDataFrame(const J1939_frame &frame) override;
};


#endif //J1939_LONG_CAN_MESSAGE_H
