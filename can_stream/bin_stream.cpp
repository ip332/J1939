//
// Created by igor.prilepov on 3/15/22.
//

#include "bin_stream.h"

bool BinStream::put(const J1939_frame & frame ) const{
    CanFrame binary;
    memset(&binary, 0, sizeof(binary));
    binary.time_ns_ = frame.time_ns_;
    binary.can_id_ = frame.canID();
    binary.data_len_ = frame.dlc_;
    memcpy(binary.data_, frame.buffer_, frame.dlc_);

    return fwrite(&binary, sizeof(binary), 1, file_) > 0;
}

bool BinStream::get(J1939_frame *frame) {
    CanFrame binary;
    memset(&binary, 0, sizeof(binary));
    if (!fread(&binary, sizeof(binary), 1, file_)) {
        return false;
    }
    frame->time_ns_ = binary.time_ns_;
    frame->setFrom(binary.can_id_, binary.data_, binary.data_len_);
    return true;
}
