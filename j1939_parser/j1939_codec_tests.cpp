//
// Created by igor on 9/7/21.
//

#include "j1939_frame.h"

#include <gtest/gtest.h>

class J1939CodecTest : public testing::Test {
protected:
    void SetUp() override {
        frame_.dlc_ = 10;
    }

    void TearDown() override {
    }

    void fillFrameWith(uint8_t value, uint8_t dlc = 8) {
        frame_.reset(value);
        frame_.dlc_ = dlc;
    }

    void setValue(uint64_t val, size_t index, size_t length, bool little_endian) {
        frame_.write_bits(val, index, length, little_endian);
    }

    uint64_t getValue(size_t index, size_t length, bool little_endian) {
        return frame_.read_bits(index, length, little_endian);
    }

    void printFrame() const {
        std::cout << "\n    7 6 5 4 3 2 1 0" << '\n';
        for (int i = 0; i < frame_.dlc_; i++) {
            std::cout << i << "   ";
            for (int j = 7; j >= 0; j--) {
                std::cout << ((frame_.buffer_[i] >> j) & 1) << ' ';
            }
            std::cout << "\n";
        }
    }

    J1939_frame frame_;
};

// Checks that the set bits method works
TEST_F(J1939CodecTest, SetSingleBitsTest) {
    for (bool endian : { true, false }) {
        for (int i = 0; i < frame_.dlc_; i++) {
            uint8_t byte_idx = i;
            for (int bit = 0; bit < 8; bit++) {
                fillFrameWith(0);
                setValue(1, bit + i * 8, 1, endian);
                EXPECT_EQ(frame_.buffer_[byte_idx], 1 << bit) << " little_endian: " << endian << " at byte " << i;
            }
        }
        for (int i = 0; i < 8; i++) {
            uint8_t byte_idx = i;
            for (int bit = 0; bit < frame_.dlc_; bit++) {
                fillFrameWith(0xFF);
                setValue(0, bit + i * 8, 1, endian);
                EXPECT_EQ(frame_.buffer_[byte_idx], (~(1 << bit)) & 0xFF) << " little_endian: " << endian << " at byte " << i;
            }
        }
    }
}

// Checks that the get bits method works.
TEST_F(J1939CodecTest, GetSingleBitsTest) {
    for (bool endian : { true, false }) {
        for (int i = 0; i < frame_.dlc_; i++) {
            uint8_t byte_idx = i;
            for (int bit = 0; bit < 8; bit++) {
                fillFrameWith(0);
                frame_.buffer_[byte_idx] = 1 << bit;
                EXPECT_EQ(getValue(bit + i * 8, 1, endian), 1) << " little_endian: " << endian << " at byte " << i;
            }
        }
        for (int i = 0; i < frame_.dlc_; i++) {
            uint8_t byte_idx = i;
            for (int bit = 0; bit < 8; bit++) {
                fillFrameWith(0xFF);
                frame_.buffer_[byte_idx] = ~(1 << bit) & 0xFF;
                EXPECT_EQ(getValue(bit + i * 8, 1, endian), 0) << " little_endian: " << endian << " at byte " << i;
            }
        }
    }
}

// Checks that it can set any valid number of buts (1 - 63) from 0 position.
TEST_F(J1939CodecTest, SetMultipleBitsTest) {
    for (bool endian : { true, false }) {
        for (size_t i = 0; i < 64; i++) {
            fillFrameWith(0);
            uint64_t expected = (1ull << (i + 1)) - 1;
            setValue(expected, 0, i + 1, endian);
            EXPECT_EQ(expected, getValue(0, i + 1, endian)) << " little_endian: " << endian << " length: " << i;
        }
    }
}

// Checks that it can set any valid number of buts (1 - 63) from floating position.
TEST_F(J1939CodecTest, SetFloatingMultipleBitsTest) {
    for (bool endian : {true, false }) {
        for (size_t i = 0; i < 64; i++) {
            fillFrameWith(0);
            uint64_t expected = (1ull << (i + 1)) - 1;
            setValue(expected, 63 - i, i + 1, endian);
            EXPECT_EQ(expected, getValue(63 -  i, i + 1, endian)) << " little_endian: " << endian << " length: " << (i + 1) << " from " << (63 - i);
        }
    }
}

// Checks that it can clear  any valid number of buts (1 - 63) from 0 position.
TEST_F(J1939CodecTest, ResetMultipleBitsTest) {
    for (bool endian : { true, false }) {
        for (size_t i = 0; i < 64; i++) {
            fillFrameWith(0xFF);
            uint64_t expected = (~((1ull << (i + 1)) - 1)) & make_mask(i + 1);
            setValue(expected, 0, i + 1, endian);
            uint64_t result = getValue(0, i + 1, endian);
            EXPECT_EQ(expected, result) << " little_endian: " << endian << " length: " << i;
        }
    }
}

// Checks that it can clear  any valid number of buts (1 - 63) from 0 position.
TEST_F(J1939CodecTest, ResetFloatingMultipleBitsTest) {
    for (bool endian : { true, false }) {
        for (size_t i = 0; i < 64; i++) {
            fillFrameWith(0xFF);
            uint64_t expected = (~((1ull << (i + 1)) - 1)) & make_mask(i + 1);
            setValue(expected, 63 - i, i + 1, endian);
            uint64_t result = getValue(63 - i, i + 1, endian);
            EXPECT_EQ(expected, result) << " little_endian: " << endian << " length: " << i;
        }
    }
}

// Checks that an arbitrary value can be set/get if it spawns for two bytes.
TEST_F(J1939CodecTest, Set10BitsTest) {
    for (bool endian : { true, false }) {
        uint64_t expected = 0x2AA; // 10 bits: 10 1010 1010
        for (size_t i = 0; i < 54; i++) {
            fillFrameWith(0);
            setValue(expected, i, 10, endian);
            uint64_t result = getValue(i, 10, endian);
            EXPECT_EQ(expected, result) << " little_endian: " << endian << " start bit: " << i;
        }
        expected >>= 1; // 10 bits: 01 0101 0101
        for (size_t i = 0; i < 54; i++) {
            fillFrameWith(0);
            setValue(expected, i, 10, endian);
            uint64_t result = getValue(i, 10, endian);
            EXPECT_EQ(expected, result) << " little_endian: " << endian << " start bit: " << i;
        }
    }
}

// Checks that an arbitrary value can be set/get if it spawns for three bytes.
TEST_F(J1939CodecTest, Set20BitsTest) {
    for (bool endian : { true, false }) {
        uint64_t expected = 0xAAAAA; // 20 bits: 1010 1010 1010 1010 1010
        for (size_t i = 0; i < 44; i++) {
            fillFrameWith(0);
            setValue(expected, i, 20, endian);
            uint64_t result = getValue(i, 20, endian);
            EXPECT_EQ(expected, result) << " little_endian: " << endian << " start bit: " << i;
        }
        expected >>= 1; // 20 bits: 0x55555
        for (size_t i = 0; i < 44; i++) {
            fillFrameWith(0);
            setValue(expected, i, 20, endian);
            uint64_t result = getValue(i, 20, endian);
            EXPECT_EQ(expected, result) << " little_endian: " << endian << " start bit: " << i;
        }
    }
}

