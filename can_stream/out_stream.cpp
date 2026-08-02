//
// Created by igor.prilepov on 3/15/22.
//

#include "out_stream.h"

#include <cstdlib>

bool OutStream::get(J1939_frame *frame) {
    auto fields_cnt = get_fields();
    if (fields_cnt == 0) {
        return false;
    }

    // There is no time field today therefore fake it, assuming 1ms per record.
    // Expected format:
    // interface = can0, family = 29, type = 3, proto = 1
    // <0x0cf00400> [8] ff ff ff 0c 1c ff f4 7d
    // Fields:   0   1  2 ....
    if (fields_cnt < 2) {
        // Weird format, ignore it.
        return false;
    }
    char *buf = fields_[0];
    if (*buf++ != '<') {
        // Invalid format (comment?)
        return false;
    }
    frame->time_ns_ = fake_time_ * 1E9;
    fake_time_ += 0.001;
    // Find CAN ID prefix
    char *can_id_str = strstr(buf,"0x");
    if (can_id_str == nullptr) {
        // Invalid format
        return false;
    }
    can_id_str += 2;
    char *can_id_end = strchr(can_id_str, '>');
    if (can_id_end == nullptr) {
        // Invalid format
        return false;
    }
    *can_id_end = 0;
    uint32_t can_id = 0;
    sscanf(can_id_str,"%8X", & can_id);

    char *ptr = fields_[1] + 1; // Skip '['
    char *end = strchr(ptr, ']');
    if (end == nullptr) {
        // Invalid format
        return false;
    }
    *end = 0;
    char *num_end = nullptr;
    unsigned long data_len = strtoul(ptr, &num_end, 10);
    if (num_end == ptr) {
        return false;
    }
    uint8_t data[J1939_frame::max_dlc_];
    if (!decodeFields(2, data_len, 16, data)) {
        return false;
    }
    return frame->setFrom(can_id | extended_, data, data_len);
}