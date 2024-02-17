//
// Created by Igor on 2021-09-09.
//

#ifndef DBCPARSER_SIGNAL_H
#define DBCPARSER_SIGNAL_H

#include "j1939_frame.h"

#include <cassert>
#include <map>
#include <sstream>
#include <stdint.h>
#include <string>
#include <vector>

#define assertm(exp, msg) assert(((void)msg, exp))

// A helper function to split a string onto fields by a dlimeter
std::vector<std::string> splitString(const std::string &line, char delimiter);
// Transforms a camel case name into a snake case.
std::string toSnakeCase(const std::string &name);

// This class stores a single Signal format.
class Signal {
    // This name is used only to present the content in a human readable format.
    std::string name_;
    // Start bit in the local_frame.
    uint16_t start_bit_;
    // Number of bits reserved for this signal.
    uint16_t length_;
    // This bit tells if the MSB represents a sign or not.
    bool signed_;
    // Most of the signals are encoded in little-endian format but not all.
    bool little_endian_;
    // Scalar and offset will be used to convert the raw value into a human readable value (mph, degree, etc.)
    float scalar_;
    float offset_;
    double min_;
    double max_;
    // Not sure why do we need units here but just in case.
    std::string units_;

    // Stores list all enums values and their names.
    std::map<uint32_t, std::string> enum_map_;

    bool valid_;

    // Returns standard C type which is required to represent this signal.
    std::string fieldType() const;
public:
    // Parses SG_ record from a DBC file and set up validity flag:
    Signal(const std::string &line);

    bool valid() const { return valid_; }

    // Returns the value which defines signal's position in the CAN frame.
    uint16_t startBit() const { return start_bit_; }

    bool little_endian() const { return little_endian_; }

    std::string name() const { return name_; }

    // Compares two signals.
    bool operator==(const Signal & other) const {
        bool result = start_bit_ == other.start_bit_ &&
                      length_ == other.length_ && offset_ == other.offset_ && signed_ == other.signed_ &&
                      scalar_ == other.scalar_ && min_ == other.min_ && max_ == other.max_ &&
                      enum_map_.size() == other.enum_map_.size();
        if (result) {
            // Make sure that both enums have the same values
            return std::equal(enum_map_.begin(), enum_map_.end(),
                              other.enum_map_.begin());
        }
        return false;
    }

    // Prints out the initialisation list for the Signal class in the SPN format (see j1939_frame.h)
    std::string toSpnString() const;

    // Prints out the signal definition as a packed structure fields to be used in a union.
    // The first argument shows the expected start bit for this signal in the message.
    std::string toPackedInt(uint16_t *start_bit, bool snake_case_name = true) const;

    // Prints out the set/get methods.
    std::string toGetSetMethod() const;

    // Adds the list of values (a tokenized VAL_ record from the DBC file) to the signal.
    void addValue(const std::vector<std::string> &fields);

    // Creates a new instance of the SPN class and fills it out with *this parameters.
    SPN toSpn() const;

    // Print signal in a human-readable format.
    std::string toString() const;
};

#endif //DBCPARSER_SIGNAL_H
