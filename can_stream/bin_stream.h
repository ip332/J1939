//
// Created by igor.prilepov on 3/15/22.
//

#ifndef J1939_BIN_STREAM_H
#define J1939_BIN_STREAM_H

#include "can_stream.h"

// Defines a single CAN frame format in a binary format.
struct CanFrame {
    uint64_t time_ns_;
    uint32_t can_id_;
    uint8_t data_len_;
    uint8_t data_[8];
} __attribute__((packed));

// This class represents a binary format where each record has a fixed size which allows using random access for any file size.
class BinStream : public CanStream {
public:
    BinStream() : CanStream() { writable_ = true; }
    bool put(const J1939_frame & frame ) const override;
    bool get(J1939_frame *frame) override;
};

#endif //J1939_BIN_STREAM_H
