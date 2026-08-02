//
// Created by igor on 12/16/21.
//

#include "transport_protocol.h"

std::unique_ptr<J1939_frame> TransportProtocol::handleCanFrame(const J1939_frame &frame) {
    std::unique_ptr<J1939_frame> none;
    if (LongCanMessageIf::isTpmFrame(frame)) {
        std::map<uint8_t, LongCanMessage>::iterator it = long_msgs_.find(frame.src_);
        if (it == long_msgs_.end()) {
            auto result = long_msgs_.emplace(frame.src_, LongCanMessage());
            it = result.first;
        }
        return it->second.handleDataFrame(frame);
    } else if (LongCanMessageIf::isFastPacketFrame(frame)) {
        std::map<uint8_t, FastPacketMessage>::iterator it = fast_msgs_.find(frame.src_);
        if (it == fast_msgs_.end()) {
            auto result = fast_msgs_.emplace(frame.src_, FastPacketMessage());
            it = result.first;
        }
        return it->second.handleDataFrame(frame);
    }
    return none;
}

