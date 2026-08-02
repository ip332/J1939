//
// Created by igor.prilepov on 3/15/22.
//

#include "log_stream.h"

bool LogStream::put(const J1939_frame & frame) const {
    fprintf(file_, "(%17.6f) can0 %.8X#",frame.time_ns_ / 1E9, frame.canID());
    for(int i = 0; i < frame.dlc_; i++) {
        fprintf(file_, "%.2X", frame.buffer_[i]);
    }
    fprintf(file_,"\n");
    // TODO: add error checking
    return true;
}

bool LogStream::get(J1939_frame *frame) {
    auto fields_cnt = get_fields();
    if (fields_cnt == 0) {
        return false;
    }
    // Expected format:
    // (1557265716.818982) can0 0CFF0686#F22700F2FFFFFF5F
    // Fields:
    //                  0   1   2
    char *buf = fields_[0];
    if (*buf++ != '(') {
        // Invalid format (comment?)
        return false;
    }
    char *time_end = strchr(buf, ')');
    if (time_end == nullptr) {
        return false;
    }
    *time_end = 0;
    double time;
    sscanf(buf, "%lf", &time);
    frame->time_ns_ = time * 1E9;
    buf = fields_[2];
    char *split = strchr(buf,'#');
    if (split == nullptr) {
        // Invalid format
        return false;
    }
    char *can_id_str = split - 8;
    char *data_str = split + 1;
    *split = 0;

    uint32_t can_id = 0;
    sscanf(can_id_str,"%8X", & can_id);

    size_t data_len = strlen(data_str) / 2;
    uint8_t data[data_len];
    for (uint32_t i = 0; i < data_len; i++) {
        uint32_t tmp = 0;
        sscanf(&data_str[2 * i], "%2X", &tmp);
        data[i] = tmp & 0xFF;
    }
    return frame->setFrom(can_id | extended_, data, data_len);
}
