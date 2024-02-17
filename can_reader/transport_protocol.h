//
// Created by igor on 12/16/21.
//

#ifndef J1939_TRANSPORT_PROTOCOL_H
#define J1939_TRANSPORT_PROTOCOL_H

#include <map>
#include "long_can_message.h"
#include "fast_packet_message.h"

// This class implements the transport protocol (J1939-21)
class TransportProtocol {
    // More than one ECU can send long messages at the same time therefore we need a storage for incomplete messages.
    // Use input message source address as a key (only one long message could be transmitted from an ECU at a time.
    // This storage contains messages transferred using transport protocol (BAM or CTS/RTS)
    ::std::map<uint8_t, LongCanMessage> long_msgs_;
    // This storage contains messages transferred fast-packet protocol (NMEA2000)
    ::std::map<uint8_t, FastPacketMessage> fast_msgs_;

public:
    static bool longMessage(const J1939_frame &frame) {
        return LongCanMessageIf::isTpmFrame(frame) || LongCanMessageIf::isFastPacketFrame(frame);
    }

    // Returns a long message if the input was the last piece of the TP or fast-packet protocol.
    std::unique_ptr<J1939_frame> handleCanFrame(const J1939_frame &frame);
};

#endif //J1939_TRANSPORT_PROTOCOL_H
