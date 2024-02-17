//
// Created by igor.prilepov on 3/15/22.
//

#include "asc_stream.h"

uint8_t AscStream::month(const char *str) const {
    if (!strcmp(str, "Jan")) return 1;
    if (!strcmp(str, "Feb")) return 2;
    if (!strcmp(str, "Mar")) return 3;
    if (!strcmp(str, "Apr")) return 4;
    if (!strcmp(str, "May")) return 5;
    if (!strcmp(str, "Jun")) return 6;
    if (!strcmp(str, "Jul")) return 7;
    if (!strcmp(str, "Aug")) return 8;
    if (!strcmp(str, "Sep")) return 9;
    if (!strcmp(str, "Oct")) return 10;
    if (!strcmp(str, "Nov")) return 11;
    if (!strcmp(str, "Dec")) return 12;
    return 0;
}

bool AscStream::get(J1939_frame *msg) {
    auto fields_cnt = get_fields();
    if (fields_cnt == 0) {
        return false;
    }
    static double start_time = 0.;
    static bool decimal = false;
    // Expected format:
    // === Header ====
    // date Tue Nov 5 02:41:52.218 pm 2019
    // base hex  timestamps absolute
    //     OR
    // base dec  timestamps absolute
    // internal events logged
    // version 10.0.0
    // === To be ignored ====
    // 7.000839 CAN 2 Status:chip status error active
    // 7.003127    SV: 3 0 1 ::CANoe::UnknownSysVar = "::CANoe::Simulation::VirtualMemory"
    // 7.004379 1  Statistic: D 0 R 0 XD 314 XR 0 E 0 O 0 B 9.11%
    // === Data to parse ====
    // 7.006866 1  CFF2100x        Rx   d 8 00 00 32 00 32 00 00 0B  Length = 0 BitCount = 0 ID = 218046720x
    if (!strcmp(fields_[0], "base")) {
        if (!strncmp(fields_[1],"dec", 3)) {
            decimal = true;
        }
        return false;
    }
    if (!strcmp(fields_[0], "date")) {
        // Fields: date Tue Nov 5 02:41:52.218 pm 2019
        // Indices:   0   1   2 3            4  5    6
        struct tm tm{};
        memset(&tm, 0, sizeof(tm));
        uint8_t mon = month(fields_[2]);
        if (mon == 0) {
            return false;
        }
        tm.tm_mday = std::stoi(fields_[3]);
        if (strptime(fields_[4], "%T", &tm) != nullptr) {
            if (!strcmp(fields_[5],"pm")) {
                tm.tm_hour += 12;
                tm.tm_hour %= 12;
            }
            tm.tm_mon = mon;
            sscanf(fields_[6],"%d", & tm.tm_year);
            tm.tm_year -= 1900;
            start_time = mktime(&tm);
        }
        return false;
    }
    if ((fields_cnt < 3) || strcmp(fields_[3],"Rx")) {
        return false;
    }
    // === Data to parse ====
    // field:   0 1         2         3   4 5  6....
    //   7.006866 1  CFF2100x        Rx   d 8 00 00 32 00 32 00 00 0B  Length = 0 BitCount = 0 ID = 218046720x

    // Make sure there is only 1 channel
    if (atol(fields_[1]) != 1) {
        std::cerr << "Another channel found in the source stream, aborting operation." << std::endl;
        fclose(file_);
        file_ = nullptr;
        return false;
    }

    if (strstr(fields_[3], "Rx") == nullptr) {
        // Ignore comments and sent data
        return false;
    }
    float offset = 0;
    sscanf(fields_[0], "%f", & offset);
    msg->time_ns_ = (start_time + offset) * 1E9;

    bool extended = false;
    char *can_id_end = strchr(fields_[2], 'x');
    if (can_id_end != nullptr) {
        extended = true;
        *can_id_end = 0;
    }
    uint32_t can_id = 0;
    if (decimal) {
        sscanf(fields_[2],"%u", & can_id);
    } else {
        sscanf(fields_[2],"%X", & can_id);
    }

    uint32_t data_len = 0;
    sscanf(fields_[5],"%u", & data_len);

    uint8_t data[data_len];
    for (uint32_t i = 0; i < data_len; i++) {
        uint32_t tmp = 0;
        if (decimal) {
            sscanf(fields_[6 + i], "%3u", &tmp);
        } else {
            sscanf(fields_[6 + i], "%2X ", &tmp);
        }
        data[i] = tmp & 0xFF;
    }
    return msg->setFrom(can_id | extended | extended_, data, data_len);
}