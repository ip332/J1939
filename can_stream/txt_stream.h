//
// Created by igor.prilepov on 3/15/22.
//

#ifndef J1939_TXT_STREAM_H
#define J1939_TXT_STREAM_H

#include "can_stream.h"

class TxtStream : public CanStream {
public:
    TxtStream() : CanStream() { writable_ = true; }
    bool put(const J1939_frame & frame) const override;
    bool get(J1939_frame *frame) override;
};

#endif //J1939_TXT_STREAM_H
