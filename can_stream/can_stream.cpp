//
// Created by igor.prilepov on 3/14/22.
//

#include "asc_stream.h"
#include "bin_stream.h"
#include "log_stream.h"
#include "out_stream.h"
#include "trc_stream.h"
#include "txt_stream.h"
#include "socket_can_stream.h"

size_t CanStream::get_fields() {
    buffer_[0] = 0;
    auto p = fgets(buffer_, sizeof(buffer_) - 1, file_);
    if (feof(file_) || (p == NULL)) {
        return 0;
    }
    // Find the first non whitespace character
    char *string = buffer_;
    while (isspace(*string) && *string != '\n' && *string != '\r') {
        string++;
    }
    char *field = string;
    fields_.clear();
    while (*string && *string != '\n' && *string != '\r') {
        if (isspace(*string)) {
            // Replace the first white space and move pointer to the next character.
            *string++ = 0;
            // Store the beginning of the field
            fields_.push_back(field);
            // Find the beginning of the next field.
            while (isspace(*string)) {
                string++;
            }
            field = string;
        } else {
            string++;
        }
    }
    *string = 0;
    if (strlen(field)) {
        fields_.push_back(field);
    }
    return fields_.size();
}

std::string SupportedStreams() {
    return "asc|bin|log|out|ros|trc|txt";
}

std::unique_ptr<CanStream> CanStreamFor(const std::string &input) {
    if (input.find(".asc") != std::string::npos) {
        return std::make_unique<AscStream>();
    }
    if (input.find(".bin") != std::string::npos) {
        return std::make_unique<BinStream>();
    }
    if (input.find(".log") != std::string::npos) {
        return std::make_unique<LogStream>();
    }
    if (input.find(".out") != std::string::npos) {
        return std::make_unique<OutStream>();
    }
    if (input.find(".trc") != std::string::npos) {
        return std::make_unique<TrcStream>();
    }
    if (input.find(".txt") != std::string::npos) {
        return std::make_unique<TxtStream>();
    }
    return std::make_unique<SocketCanStream>();
}

