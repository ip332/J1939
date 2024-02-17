//
// Created by igor.prilepov on 3/14/22.
//

#ifndef J1939_IO_ADAPTER_H
#define J1939_IO_ADAPTER_H

#include <memory>
#include <iostream>
#include "j1939_frame.h"

// This is a base class to read/write a J1939 frame.
class CanStream {
protected:
    // Most of the streams are different file formats therefore we'll need a file pointer.
    FILE *file_;
    // This flag can be set in the derived class constructor
    bool writable_;
    // File will be read line by line so here is the buffer to store a single line.
    char buffer_[100];
    // That buffer should be represented as an array of fields.
    // Each field will be stored as a pointer to the actual buffer.
    std::vector<char *> fields_;
    // Some streams do not have extended bit encoded.
    // The following flag allows the caller to treat all IDs from that stream as extended ones.
    uint32_t extended_;
    // Writing to the real CAN bus must preserve the timing when files don't need it.
    bool real_port;
    // Reads line, parses it onto fields and returns number of items in the fields_ array.
    size_t get_fields();
public:
    CanStream() : file_(nullptr), writable_(false), extended_(false), real_port(false) {}

    bool writable() const { return writable_; }

    bool realPort() const { return real_port; }

    void setExtended(bool val) { extended_ = val ? 0x80000000 : 0;}

    virtual void close() {
        if (file_ != nullptr) {
            fclose(file_);
            file_ = nullptr;
        }
    }
    virtual ~CanStream() {
        close();
    }

    // Opens the stream
    virtual bool open(const std::string &input, bool write = false) {
        if (write && !writable_) {
            std::cerr << "Write operation is not supported for " << input << std::endl;
            return false;
        }
        file_= fopen(input.c_str(), write ? "wt" : "rt");
        if (file_== nullptr) {
            std::cerr << "File " << input << " not found" << std::endl;
            return false;
        }
        return true;
    }

    virtual void rewind() {
        if (!real_port && file_ != NULL) {
            ::rewind(file_);
        }
    }

    // Tells if the stream is closed or not.
    virtual bool dataAvailable() const { return file_!= nullptr && !feof(file_); }
    // In case of a writeable stream, we better to use a different name
    bool ready() const { return dataAvailable(); }

    // Writes a frame into the stream. This is stream specific operation.
    virtual bool put(const J1939_frame &frame) const = 0;
    // Reads a frame from the stream.  This is stream specific operation.
    virtual bool get(J1939_frame *frame) = 0;
};

std::string SupportedStreams();
std::unique_ptr<CanStream> CanStreamFor(const std::string &input);

#endif //J1939_IO_ADAPTER_H
