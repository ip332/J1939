//
// Created by igor.prilepov on 3/15/22.
//

#ifndef J1939_OUT_STREAM_H
#define J1939_OUT_STREAM_H

#include "can_stream.h"

class OutStream : public CanStream {
public:
    bool put(const J1939_frame & /*frame*/) const override { return false; };
    bool get(J1939_frame *frame) override;
};

#endif //J1939_OUT_STREAM_H
