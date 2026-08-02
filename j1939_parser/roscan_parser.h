//
// Created by igor on 12/8/21.
//

#ifndef ROSCAN_DRIVER_ROSCAN_PARSER_H
#define ROSCAN_DRIVER_ROSCAN_PARSER_H

#include "j1939_frame.h"
#include "dbc_parser.h"

#include <memory>

// This class parses a text file (ROS CAN message format) into internal J1939_frame structure.
// The idea is to use it inside can_listener application to have ability to send some messages to the bus.
class RosCanParser {
    std::vector<J1939_frame> frames_;
    // DbcParser needs to be shared class between CANEncoder and CANDecoder.
    std::shared_ptr<DbcParser> dbc_parser_ptr_;

    // Extracts value form a string like "abcd: 1.0"
    double value(const std::string &line);
    // Adds given frame to the frames_ vector.
    void addFrame(const J1939_frame &frame);
public:
    // Stores the DBC parser pointer.
    void setDbc(const std::shared_ptr<DbcParser> &dbc_parser_ptr) { dbc_parser_ptr_ = dbc_parser_ptr; }
    // Parses given file name into a vector of CAN frames and returns number of decoded messages.
    size_t parseFile(const std::string &path);
    // Retrieves a message by index.
    bool getMessage(size_t index, J1939_frame *frame) const;
};

#endif //ROSCAN_DRIVER_ROSCAN_PARSER_H
