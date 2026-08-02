//
// Created by igor.prilepov on 3/15/22.
//

#include "out_stream.h"

bool OutStream::get(J1939_frame *frame) {
    auto fields_cnt = get_fields();
    if (fields_cnt == 0) {
        return false;
    }

    // There is no time field today therefore fake it, assuming 1ms per record.
    static double fake_time = 0.;
    // Expected format:
    // interface = can0, family = 29, type = 3, proto = 1
    // <0x0cf00400> [8] ff ff ff 0c 1c ff f4 7d
    // Fields:   0   1  2 ....
    char *buf = fields_[0];
    if (*buf++ != '<') {
        // Invalid format (comment?)
        return false;
    }
    frame->time_ns_ = fake_time * 1E9;
    fake_time += 0.001;
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
    uint32_t data_len = std::stoul(ptr);
    uint8_t data[data_len];
    for (uint32_t i = 0; i < data_len; i++) {
        uint32_t tmp = 0;
        sscanf(fields_[2 + i], "%2X ", &tmp);
        data[i] = tmp & 0xFF;
    }
    return frame->setFrom(can_id | extended_, data, data_len);
}