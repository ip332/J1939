//
// Tests for CanSandbox's main processing loop, using MockCanStream to exercise it
// deterministically without real files or CAN hardware. Includes regression coverage
// for two bugs fixed in an earlier pass: frames could be silently dropped at EOF, and
// a real-port output reconnect used the wrong stream's name.
//

#include "can_sandbox_lib.h"
#include "mock_can_stream.h"

#include <gtest/gtest.h>

namespace {

J1939_frame makeFrame(uint32_t pgn, uint8_t src, const uint8_t data[8]) {
    J1939_frame frame;
    frame.reset();
    frame.pgn_ = pgn;
    frame.dst_ = 0xFF;
    frame.src_ = src;
    frame.pri_ = 6;
    frame.dlc_ = 8;
    memcpy(frame.buffer_, data, 8);
    return frame;
}

} // namespace

TEST(CanSandboxTest, RoundTripsFramesFromMockInput) {
    auto input = std::make_unique<MockCanStream>();
    const uint8_t data1[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    const uint8_t data2[8] = {8, 7, 6, 5, 4, 3, 2, 1};
    const uint8_t data3[8] = {0xAA, 0, 0, 0, 0, 0, 0, 0};
    input->enqueueFrame(makeFrame(0x00FEF100, 0x11, data1));
    input->enqueueFrame(makeFrame(0x00FEF100, 0x11, data2));
    input->enqueueFrame(makeFrame(0x00FEF100, 0x11, data3));

    CanSandbox sandbox;
    sandbox.setInputStream(std::move(input));

    std::vector<J1939_frame> received;
    sandbox.setParserCallback([&](const J1939Parser &parser) {
        received.push_back(parser.frame());
    });

    sandbox.startProcessing();

    ASSERT_EQ(received.size(), 3u);
    EXPECT_EQ(received[0].buffer_[0], 1);
    EXPECT_EQ(received[1].buffer_[0], 8);
    EXPECT_EQ(received[2].buffer_[0], 0xAA);
}

TEST(CanSandboxTest, ForwardsInputFramesToOutput) {
    auto input = std::make_unique<MockCanStream>();
    const uint8_t data[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    input->enqueueFrame(makeFrame(0x00FEF100, 0x11, data));

    auto output = std::make_unique<MockCanStream>();
    MockCanStream *output_ptr = output.get();

    CanSandbox sandbox;
    sandbox.setInputStream(std::move(input));
    sandbox.setOutputStream(std::move(output));

    sandbox.startProcessing();

    ASSERT_EQ(output_ptr->written().size(), 1u);
    EXPECT_EQ(output_ptr->written()[0].buffer_[0], 1);
}

TEST(CanSandboxTest, ReassemblesMultiFrameTransfer) {
    auto input = std::make_unique<MockCanStream>();
    // BAM: control=32, size=10 (LSB/MSB), 2 frames, reserved, PGN=0x00FEF1
    const uint8_t bam[8] = {32, 10, 0, 2, 0xFF, 0xF1, 0xFE, 0x00};
    const uint8_t dt1[8] = {1, 'H', 'e', 'l', 'l', 'o', ' ', 'J'};
    const uint8_t dt2[8] = {2, '1', '9', '3', '9', 0xFF, 0xFF, 0xFF};
    input->enqueueFrame(makeFrame(0xEC00, 0x22, bam));
    input->enqueueFrame(makeFrame(0xEB00, 0x22, dt1));
    input->enqueueFrame(makeFrame(0xEB00, 0x22, dt2));

    CanSandbox sandbox;
    sandbox.setInputStream(std::move(input));

    std::vector<J1939_frame> received;
    sandbox.setParserCallback([&](const J1939Parser &parser) {
        received.push_back(parser.frame());
    });

    sandbox.startProcessing();

    ASSERT_EQ(received.size(), 1u);
    EXPECT_EQ(received[0].dlc_, 10);
    EXPECT_EQ(received[0].pgn_, 0x00FEF1u);
    EXPECT_EQ(memcmp(received[0].buffer_, "Hello J", 7), 0);
    EXPECT_EQ(memcmp(&received[0].buffer_[7], "193", 3), 0);
}

TEST(CanSandboxTest, ReconnectsRealPortOutputUsingCorrectStreamName) {
    auto input = std::make_unique<MockCanStream>();
    const uint8_t data[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    input->enqueueFrame(makeFrame(0x00FEF100, 0x11, data));

    auto output = std::make_unique<MockCanStream>();
    output->setRealPort(true);
    output->setReady(false); // forces the reconnect path
    MockCanStream *output_ptr = output.get();

    CanSandbox sandbox;
    sandbox.setInputStream(std::move(input), "mock-input");
    sandbox.setOutputStream(std::move(output), "mock-output");

    sandbox.startProcessing();

    ASSERT_FALSE(output_ptr->openCalls().empty());
    for (const auto &name : output_ptr->openCalls()) {
        EXPECT_EQ(name, "mock-output");
    }
}
