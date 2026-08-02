//
// Created by igor on 9/14/21.
//

#include "dbc_message.h"

#include <gtest/gtest.h>

const char *led_msg = "BO_ 2365568042 ENGAGE_LED_CONTROL: 2 Vector__XXX\n";
const char *led_msg_fields[] = {
        " SG_ Counter : 8|8@1+ (1,0) [0|0] \"\" Vector__XXX\n",
        " SG_ Led1 : 1|1@1+ (1,0) [0|1] \"\" Vector__XXX\n",
        " SG_ Led0 : 0|1@1+ (1,0) [0|1] \"\" Vector__XXX\n"
};

const char *led_msg_union = "\n"
        "#define ENGAGE_LED_CONTROL_MESSAGE_ID\t0x8cffb42a\n"
        "#define ENGAGE_LED_CONTROL_PGN\t\t0xffb4\n"
        "#define ENGAGE_LED_CONTROL_MESSAGE_DLC\t2\n"
        "struct engage_led_control_fields_t {\n"
        "    uint64_t led0 : 1;\n"
        "    uint64_t led1 : 1;\n"
        "    uint64_t : 6;\n"
        "    uint64_t counter : 8;\n"
        "} __attribute__((packed));\n"
        "\n"
        "\n"
        "struct engage_led_control_message_t {\n"
        "    union {\n"
        "        uint8_t data[ENGAGE_LED_CONTROL_MESSAGE_DLC];\n"
        "        struct engage_led_control_fields_t fields;\n"
        "    };\n"
        "#ifdef __cplusplus\n"
        "\n"
        "\tuint8_t led0() const { return fields.led0; }\n"
        "\tvoid led0(uint8_t value) { fields.led0 = value; }\n"
        "\n"
        "\tuint8_t led1() const { return fields.led1; }\n"
        "\tvoid led1(uint8_t value) { fields.led1 = value; }\n"
        "\n"
        "\tuint8_t counter() const { return fields.counter; }\n"
        "\tvoid counter(uint8_t value) { fields.counter = value; }\n"
        "\n"
        "    void fillFrameWith(uint8_t value = 0xFF) { memset(data, value, sizeof(data)); }\n"
        "#endif\n"
        "\n"
        "};\n";

class UnionTest : public testing::Test {
protected:
    void SetUp() override {
    }

    void TearDown() override {
    }

};

// Tests that the fields, missing in the DBC, will be added to the packed structure list as unnamed bitfields
// and that the fields names will be converted into the snake case.
TEST_F(UnionTest, MissedFieldsMessageUnionTest) {
    Message msg(led_msg);

    for(const auto * ptr : led_msg_fields) {
        msg.addSignal(ptr);
    }
    EXPECT_EQ(msg.toUnion(), led_msg_union);
}


