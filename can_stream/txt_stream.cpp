//
// Created by igor.prilepov on 3/15/22.
//

#include "txt_stream.h"

bool TxtStream::get(J1939_frame *frame) {
    auto fields_cnt = get_fields();
    if (fields_cnt == 0) {
        return false;
    }

    // There is no time field today therefore fake it, assuming 1ms per record.
    static double fake_time = 0.;
    frame->time_ns_ = fake_time * 1E9;
    fake_time += 0.1;

    // Expected format:
    //  can0  19F21200   [8]  20 0B 01 01 00 64 FF FF
    //      Field index:
    //  0       1         2   3  ...
    // or (if candump supports -ta option):
    //  (1657126380.578519)  can0  01A   [8]  11 22 33 44 AA BB CC DD
    //      Field index:
    //  0                    1     2     3    4 ...
    // Safety check:
    if (fields_.size() < 4) {
        // Weird format, ignore it.
        return false;
    }
    // Check if the timestamp is present
    int idx = 1;
    if (*fields_[0] == '(') {
        auto buf = fields_[0] + 1;
        char *time_end = strchr(buf, ')');
        if (time_end == nullptr) {
            return false;
        }
        *time_end = 0;
        double time;
        sscanf(buf, "%lf", &time);
        frame->time_ns_ = time * 1E9;
        idx = 2;
    }
    uint32_t can_id = 0;
    sscanf(fields_[idx],"%8X", & can_id);
    size_t data_len = fields_.size() - (idx + 2);
    uint8_t data[data_len];
    for (size_t i = 0; i < data_len; i++) {
        uint32_t tmp = 0;
        sscanf(fields_[idx + 2 + i], "%2X ", &tmp);
        data[i] = tmp & 0xFF;
    }
    return frame->setFrom(can_id | extended_, data, data_len);
}

bool TxtStream::put(const J1939_frame & frame) const {
    if (frame.time_ns_ != 0) {
        fprintf(file_, " (%17.6f)",frame.time_ns_ / 1E9);
    }
    fprintf(file_, "  can0  %.8X   [%d]  ", frame.canID(), frame.dlc_);
    for(int i = 0; i < frame.dlc_; i++) {
        fprintf(file_, "%.2X ", frame.buffer_[i]);
    }
    fprintf(file_,"\n");
    // TODO: add error checking
    return true;
}