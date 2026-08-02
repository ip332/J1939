//
// Created by igor on 12/17/21.
//

#ifndef J1939_FAST_PACKET_MESSAGE_H
#define J1939_FAST_PACKET_MESSAGE_H

#include "long_can_message_if.h"

class FastPacketMessage : public LongCanMessageIf {
    // Total number of bytes expected (0-223)
    uint8_t expected_size_;
    // Next frame index.
    uint8_t expected_frame_;
    // Sequence counter
    uint8_t sequence_counter_;
    // Error flag. Set when an out-of-order packet is received.
    bool error_;

    void reset(const J1939_frame &frame) override;
public:
    std::unique_ptr<J1939_frame> handleDataFrame(const J1939_frame &frame) override;
};


#endif //J1939_FAST_PACKET_MESSAGE_H
