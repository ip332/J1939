//
// Created by igor on 12/8/21.
//

#include "roscan_parser.h"
#include "j1939_parser.h"

#include <iostream>

// Expected format (only mandatory fields are shown)
// ---
// id: 512
// name: "RadarConfiguration0"
// bus: "srdrsrcan"
// time:
//  secs: 0
//  nsecs:         0
// signals:
//  -
//    value: 1.0
//    name: "RadarCfg_MaxDistance_valid"
//    value: 90.0
//    name: "RadarCfg_MaxDistance"
size_t RosCanParser::parseFile(const std::string &path) {
    J1939_frame frame;
    frame.reset(0);

    if (! dbc_parser_ptr_) {
        std::cerr << "Error: DBC file was not loaded" << std::endl;
        return 0;
    }
    J1939Parser parser(frame, dbc_parser_ptr_->dbc());

    std::cout << "Loading ROS messages from text file " << path << std::endl;

    // Open file and start parsing.
    std::ifstream infile(path);
    if (!infile.is_open()) {
        std::cerr << "Error: could not open file " << path << std::endl;
        return 0;
    }
    std::string line;
    std::string value_str;
    while (std::getline(infile, line)) {
        if (line[line.size() - 1] == '\r') {
            line.pop_back();
        }
        if ((line.find("id: ") != std::string::npos) && (frame.canID() == 0)) {
            // Save last frame
            addFrame(frame);
            // New message start
            frame.reset(0);
            frame.setFrom(value(line));
            auto pgn = parser.getPgn();
            if (pgn != nullptr) {
                frame.dlc_ = pgn->dlc_;
            }
            continue;
        }
        if ((line.find("  value: ") != std::string::npos)) {
            value_str = line.substr(line.find(": ") + 2);
            continue;
        }
        if ((line.find("  name: ") != std::string::npos)) {
            auto tmp = line.substr(line.find("\"") + 1);
            auto name = tmp.substr(0, tmp.find("\""));
            parser.setSignalValue(name, std::stod(value_str));
            continue;
        }
    }
    addFrame(frame);
    return frames_.size();
}

void RosCanParser::addFrame(const J1939_frame &frame) {
    if ((frame.canID() != 0)) {
        frames_.push_back(frame);
    }
}
double RosCanParser::value(const std::string &line) {
    auto pos = line.find(": ");
    if (pos == std::string::npos) {
        return 0;
    }
    return std::stod(line.substr(pos + 2));
}

bool RosCanParser::getMessage(size_t index, J1939_frame *frame) const {
    if (index < frames_.size()) {
        *frame = frames_[index];
        return true;
    } else {
        return false;
    }
}