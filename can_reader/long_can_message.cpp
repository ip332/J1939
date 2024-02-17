//
// Created by igor on 12/16/21.
//

#include "long_can_message.h"

void LongCanMessage::reset(const J1939_frame &frame) {
    error_ = false;
    // Initialize the data storage.
    message_ = frame;

    // Message format:
    // Byte index:  0  1  2  3  4  5  6  7
    // Data:       10 73 00 11 11 DA FE 00
    //  0 - control byte (we handle only BAM and RTS)
    //  1 (LSB),2 (MSB) - number of bytes.
    //  3 - total number of data frames (control byte == RTS)
    //  4 - maximum number of data frames
    //  5,6,7 - PGN of the long message.
    // uint8_t control_byte = frame.buffer_[0];
    expected_frames_cnt_ = frame.buffer_[3];
    expected_frame_ = 1;
    message_.dlc_ = expected_size_ = frame.buffer_[1] + (frame.buffer_[2] << 8);
    message_.pgn_ = frame.buffer_[5] + (frame.buffer_[6] << 8) + (frame.buffer_[7] << 16);
}

std::unique_ptr<J1939_frame> LongCanMessage::handleDataFrame(const J1939_frame &frame) {
    std::unique_ptr<J1939_frame> result;
    if (frame.pgn_ == 0xEC00) {
        reset(frame);
        return result;
    }
    if (error_ || (frame.buffer_[0] != expected_frame_) ||
        (frame.src_ != message_.src_) ) {
        error_ = true;
        return result;
    }
    // Copy the payload into the correct location in the data buffer.
    memcpy(&message_.buffer_[(expected_frame_ - 1) * 7], &frame.buffer_[1], 7);
    if (expected_frame_ == expected_frames_cnt_) {
        result.reset(new J1939_frame());
        *result = message_;
    } else {
        expected_frame_++;
    }
    return result;
}
