//
// Created by igor.prilepov on 3/15/22.
//

#include "trc_stream.h"

bool TrcStream::get(J1939_frame *frame) {
    auto fields_cnt = get_fields();
    if (fields_cnt == 0) {
        return false;
    }

    static double start_time = 0.;
    if (*fields_[0] == ';') {
        if (fields_cnt < 4) {
            return false;
        }
        // Comment line, we are looking for the following:
        //;   Start time: 5/29/2019 14:21:28.023.0
        if (!strcmp(fields_[1], "Start") && !strcmp(fields_[2], "time:")) {
            struct tm tm{};
            memset(&tm, 0, sizeof(tm));
            if ((strptime(fields_[3], "%m/%d/%Y", &tm) != nullptr) &&
                (strptime(fields_[4], "%H:%M:%S", &tm) != nullptr)) {
                start_time = mktime(&tm);
            }
        }
        return false;
    }
    // Expected format:
    //     1)         0.1  Rx     10FF6527  8  FF FF FF CF FF FF FF FF
    // Fields:
    //      0           1   2            3  4  5
    float offset = 0; // Offset in ms
    sscanf(fields_[1], "%f", & offset);
    frame->time_ns_ = (start_time + offset / 1000.) * 1E9;

    uint32_t can_id = 0;
    sscanf(fields_[3],"%8X", & can_id);

    uint32_t data_len = std::stoul(fields_[4]);
    uint8_t data[data_len];
    for (uint32_t i = 0; i < data_len; i++) {
        uint32_t tmp = 0;
        sscanf(fields_[5 + i], "%2X ", &tmp);
        data[i] = tmp & 0xFF;
    }
    return frame->setFrom(can_id | extended_, data, data_len);
}