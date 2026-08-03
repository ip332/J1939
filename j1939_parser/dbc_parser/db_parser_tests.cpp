//
// Created by Igor on 2021-09-10.
//

#include "dbc_parser.h"
#include "j1939_parser.h"

#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <fstream>
#include <unistd.h>
#include <gtest/gtest.h>

const char *msg_value = "BO_ 1234567 ABCD: 8 ABS";
const char *simple_signal = " SG_ Engine_Requested_SpeedSpeed_Limit : 8|16@1+ (0.125,0) [0|8191] \"rpm\" Vector__XXX";

const char *signal_value = " SG_ SystemStatus : 0|2@1+ (1,0) [0|3] \"\" Vector__XXX";
const char *val_record = "VAL_ 1234567 SystemStatus 2 \"Error\" 0 \"Off\" 1 \"On\" 3 \"Not-Available\" ;";
const char *expected_value_spn = "\t\t{\"SystemStatus\", 0, 2, false, true, 1, 0, 0, 3, {\n"
                                 "\t\t\t{0, \"Off\"}, {1, \"On\"}, {2, \"Error\"}, {3, \"Not-Available\"}, }},\n";
const char *expected_value_pgn = "{ \"ABCD\", 8, {\n"
                                 "\t\t{\"SystemStatus\", 0, 2, false, true, 1, 0, 0, 3, {\n"
                                 "\t\t\t{0, \"Off\"}, {1, \"On\"}, {2, \"Error\"}, {3, \"Not-Available\"}, }},\n"
                                 "\t}}";

const char *tsc1 = "BO_ 2348875520 TSC1: 8 ECM";
const char *tsc1_fields[] = {
        " SG_ Engine_Requested_Torque_Fraction : 40|4@1+ (0.125,0) [0|1] \"%\" Vector__XXX",
        " SG_ Engine_Override_Control_Mode : 0|2@1+ (1,0) [0|3] \"bit\" Vector__XXX",
        " SG_ Engine_Requested_SpeedSpeed_Limi : 8|16@1+ (0.125,0) [0|8191] \"rpm\" Vector__XXX",
        " SG_ Engine_Requested_Speed_Control_C : 2|2@1+ (1,0) [0|3] \"bit\" Vector__XXX",
        " SG_ TSC1_Control_Purpose : 35|5@1+ (1,0) [0|31] \"bit\" Vector__XXX",
        " SG_ Override_Control_Mode_Priority : 4|2@1+ (1,0) [0|3] \"bit\" Vector__XXX",
        " SG_ TSC1_Transmission_Rate : 32|3@1+ (1,0) [0|7] \"bit\" Vector__XXX",
        " SG_ Engine_Requested_TorqueTorque_Li : 24|8@1+ (1,-125) [-125|380] \"%\" Vector__XXX"
};

const char *expected_tsc1_pgn = "{ \"TSC1\", 8, {\n"
        "\t\t{\"Engine_Override_Control_Mode\", 0, 2, false, true, 1, 0, 0, 3, {}},\n"
        "\t\t{\"Engine_Requested_Speed_Control_C\", 2, 2, false, true, 1, 0, 0, 3, {}},\n"
        "\t\t{\"Override_Control_Mode_Priority\", 4, 2, false, true, 1, 0, 0, 3, {}},\n"
        "\t\t{\"Engine_Requested_SpeedSpeed_Limi\", 8, 16, false, true, 0.125, 0, 0, 8191, {}},\n"
        "\t\t{\"Engine_Requested_TorqueTorque_Li\", 24, 8, false, true, 1, -125, -125, 380, {}},\n"
        "\t\t{\"TSC1_Transmission_Rate\", 32, 3, false, true, 1, 0, 0, 7, {}},\n"
        "\t\t{\"TSC1_Control_Purpose\", 35, 5, false, true, 1, 0, 0, 31, {}},\n"
        "\t\t{\"Engine_Requested_Torque_Fraction\", 40, 4, false, true, 0.125, 0, 0, 1, {}},\n"
        "\t}}";

class DbcParserTest : public testing::Test {
protected:
    void SetUp() override {
    }

    void TearDown() override {
    }

};

TEST_F(DbcParserTest, SimpleSignalFormatTest) {
    Signal signal(simple_signal);

    EXPECT_TRUE(signal.valid());
    std::string str = signal.toSpnString();
    EXPECT_EQ(str,"\t\t{\"Engine_Requested_SpeedSpeed_Limit\", 8, 16, false, true, 0.125, 0, 0, 8191, {}},\n");
}

TEST_F(DbcParserTest, SignalWithValueFormatTest) {
    Signal signal(signal_value);

    EXPECT_TRUE(signal.valid());
    std::vector<std::string> values = splitString(val_record, ' ');
    signal.addValue(values);

    std::string str = signal.toSpnString();
    EXPECT_EQ(str,expected_value_spn);
}

TEST_F(DbcParserTest, EmptyMessagePgnFormatTest) {
    Message msg(tsc1);

    EXPECT_EQ(msg.toPgnString(), "{ \"TSC1\", 8, {\n\t}}");
}

TEST_F(DbcParserTest, SimpleMessagePgnFormatTest) {
    Message msg(tsc1);

    for(const auto *ptr : tsc1_fields) {
        msg.addSignal(ptr);
    }
    EXPECT_EQ(msg.toPgnString(), expected_tsc1_pgn);
}


TEST_F(DbcParserTest, MessageValuePgnFormatTest) {
    Message msg(msg_value);

    msg.addSignal(signal_value);

    std::vector<std::string> fields = splitString(val_record, ' ');
    msg.addValues(fields);

    EXPECT_EQ(msg.toPgnString(), expected_value_pgn);
}

TEST_F(DbcParserTest, RuntimeMessageFindTest) {
    DbcParser parser;
    parser.setVerbose(false);
    ASSERT_TRUE(parser.parseFile(DBC_FOLDER"test_database.dbc"));
    parser.generateRunTimeFormat();
    auto dbc = parser.dbc();
    EXPECT_EQ(dbc.find(160), dbc.end());
    EXPECT_NE(dbc.find(16), dbc.end());
    EXPECT_NE(dbc.find(17), dbc.end());
    EXPECT_NE(dbc.find(18), dbc.end());
}

// --- Tests needing real files, since DbcParser's internal state (raw_messages_,
// extended_ids_, etc.) is only populated by actually parsing a .dbc file. ---

class DbcParserFileTest : public testing::Test {
protected:
    void TearDown() override {
        for (const auto &path : temp_paths_) {
            remove(path.c_str());
        }
    }

    std::string writeTemp(const std::string &content) {
        std::string path = "/tmp/dbc_parser_test_XXXXXX.dbc";
        std::vector<char> buf(path.begin(), path.end());
        buf.push_back('\0');
        int fd = mkstemps(buf.data(), 4); // ".dbc"
        FILE *f = fdopen(fd, "w");
        fwrite(content.data(), 1, content.size(), f);
        fclose(f);
        std::string result(buf.data());
        temp_paths_.push_back(result);
        return result;
    }

    std::vector<std::string> temp_paths_;
};

TEST_F(DbcParserFileTest, ParseFileRejectsNonDbcExtension) {
    DbcParser parser;
    parser.setVerbose(false);
    EXPECT_FALSE(parser.parseFile("/tmp/not_a_dbc_file.txt"));
}

TEST_F(DbcParserFileTest, ParseFileRejectsNonexistentFile) {
    DbcParser parser;
    parser.setVerbose(false);
    EXPECT_FALSE(parser.parseFile("/nonexistent/path/does_not_exist.dbc"));
}

TEST_F(DbcParserFileTest, StoreLastMessageWarnsOnConflictingDuplicateCanId) {
    // Two BO_ blocks sharing the same (standard, non-extended) CAN ID but different
    // content -- exercises the "already used for a different message" warning path.
    // Only the first is kept; parsing still succeeds rather than failing hard.
    std::string content =
        "BO_ 100 FirstMessage: 8 ECM\n"
        " SG_ Sig1 : 0|8@1+ (1,0) [0|255] \"\" Vector__XXX\n"
        "\n"
        "BO_ 100 SecondMessage: 4 ECM\n"
        " SG_ Sig2 : 0|8@1+ (1,0) [0|255] \"\" Vector__XXX\n"
        "\n";
    std::string path = writeTemp(content);
    DbcParser parser;
    parser.setVerbose(false);
    ASSERT_TRUE(parser.parseFile(path));
    parser.generateRunTimeFormat();
    auto id = parser.getId("FirstMessage");
    ASSERT_TRUE(id);
    EXPECT_EQ(*id, 100u);
    EXPECT_FALSE(parser.getId("SecondMessage"));
}

TEST_F(DbcParserFileTest, StoreLastMessageIgnoresExactDuplicateCanId) {
    // Same CAN ID and identical content -- the "else" (no-conflict) branch of the
    // duplicate check, distinct from StoreLastMessageWarnsOnConflictingDuplicateCanId.
    std::string content =
        "BO_ 100 SameMessage: 8 ECM\n"
        " SG_ Sig1 : 0|8@1+ (1,0) [0|255] \"\" Vector__XXX\n"
        "\n"
        "BO_ 100 SameMessage: 8 ECM\n"
        " SG_ Sig1 : 0|8@1+ (1,0) [0|255] \"\" Vector__XXX\n"
        "\n";
    std::string path = writeTemp(content);
    DbcParser parser;
    parser.setVerbose(false);
    ASSERT_TRUE(parser.parseFile(path));
    parser.generateRunTimeFormat();
    EXPECT_TRUE(parser.getId("SameMessage"));
}

TEST_F(DbcParserFileTest, RemoveDuplicatesLeavesNoGeneralizedPgnEntryWhenNoMessageHasStandardDlc) {
    // When none of a PGN group's distinct messages have the "standard" 8-byte DLC, no
    // generalized copy is stored under the bare PGN key -- only the CAN-ID-keyed copies.
    std::string content =
        "BO_ 2164129809 StatusMsg_11: 4 ECM\n"
        " SG_ Sig1 : 0|8@1+ (1,0) [0|255] \"\" Vector__XXX\n\n"
        "BO_ 2164129826 StatusMsg_22: 2 ECM\n"
        " SG_ Sig1 : 0|8@1+ (1,0) [0|255] \"\" Vector__XXX\n\n";
    std::string path = writeTemp(content);
    DbcParser parser;
    parser.setVerbose(false);
    ASSERT_TRUE(parser.parseFile(path));
    parser.removeDuplicates();
    parser.generateRunTimeFormat();
    EXPECT_EQ(parser.dbc().find(65024), parser.dbc().end());
    EXPECT_NE(parser.dbc().find(2164129792), parser.dbc().end());
    EXPECT_NE(parser.dbc().find(2164129826), parser.dbc().end());
}

TEST_F(DbcParserFileTest, TopLevelParserHandlesValRecordThroughRealParse) {
    std::string content =
        "BO_ 100 StatusMessage: 1 ECM\n"
        " SG_ Mode : 0|2@1+ (1,0) [0|3] \"\" Vector__XXX\n"
        "\n"
        "VAL_ 100 Mode 1 \"On\" 0 \"Off\" ;\n";
    std::string path = writeTemp(content);
    DbcParser parser;
    parser.setVerbose(false);
    ASSERT_TRUE(parser.parseFile(path));
    parser.generateRunTimeFormat();
    const PGN *pgn = &parser.dbc().at(100);
    const SPN *mode = pgn->signalByName("Mode");
    ASSERT_NE(mode, nullptr);
    EXPECT_EQ(mode->enum_names_.size(), 2u);
}

TEST_F(DbcParserFileTest, VerboseModePrintsDiagnostics) {
    // Covers the three verbose_-gated std::cout sites: the path echoed at the top of
    // parseFile(), the "Loading dbc file" message once it's open, and the per-message
    // summary line printed from topLevelParser() for each BO_ record.
    std::string path = writeTemp("BO_ 100 Msg: 8 ECM\n\n");
    DbcParser parser;
    parser.setVerbose(true);
    testing::internal::CaptureStdout();
    bool ok = parser.parseFile(path);
    std::string output = testing::internal::GetCapturedStdout();
    EXPECT_TRUE(ok);
    EXPECT_NE(output.find(path), std::string::npos);
    EXPECT_NE(output.find("Loading dbc file"), std::string::npos);
    EXPECT_NE(output.find("Msg"), std::string::npos);
}

TEST_F(DbcParserFileTest, ParseFileHandlesPathWithNoDirectorySeparator) {
    // rfind("/") == npos on a bare filename takes the "no slash" branch of dbc_name_
    // extraction (uses the argument as-is instead of substr-ing out a directory
    // prefix) -- only reachable by opening a file via a relative, slash-free path.
    char cwd_buf[4096];
    ASSERT_NE(getcwd(cwd_buf, sizeof(cwd_buf)), nullptr);
    std::string original_cwd(cwd_buf);
    ASSERT_EQ(chdir("/tmp"), 0);
    std::string path = writeTemp("BO_ 100 Msg: 8 ECM\n\n"); // always under /tmp regardless of cwd
    std::string relative = path.substr(path.rfind('/') + 1);
    DbcParser parser;
    parser.setVerbose(false);
    bool ok = parser.parseFile(relative);
    ASSERT_EQ(chdir(original_cwd.c_str()), 0);
    EXPECT_TRUE(ok);
}

TEST_F(DbcParserFileTest, GetIdFindsAndMissesById) {
    std::string path = writeTemp("BO_ 100 KnownMessage: 8 ECM\n\n");
    DbcParser parser;
    parser.setVerbose(false);
    ASSERT_TRUE(parser.parseFile(path));
    parser.generateRunTimeFormat();
    EXPECT_TRUE(parser.getId("KnownMessage"));
    EXPECT_FALSE(parser.getId("UnknownMessage"));
}

TEST_F(DbcParserFileTest, GeneratePgnAndUnionOutputHandleNullptrGuard) {
    DbcParser parser;
    parser.setVerbose(false);
    // Both have an early-return guard for a null output stream; must not crash.
    parser.generatePgnOutput(nullptr);
    parser.generateUnionOutput(nullptr);
}

TEST_F(DbcParserFileTest, GeneratePgnOutputWritesExpectedContent) {
    std::string path = writeTemp("BO_ 100 SimpleMsg: 8 ECM\n\n");
    DbcParser parser;
    parser.setVerbose(false);
    parser.setName("test");
    ASSERT_TRUE(parser.parseFile(path));

    std::string out_path = "/tmp/dbc_parser_test_pgn_out.cpp";
    std::ofstream out(out_path);
    parser.generatePgnOutput(&out);
    out.close();

    std::ifstream in(out_path);
    std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    EXPECT_NE(content.find("std::map<uint32_t, PGN> test_dbc"), std::string::npos);
    EXPECT_NE(content.find("SimpleMsg"), std::string::npos);
    remove(out_path.c_str());
}

TEST_F(DbcParserFileTest, GenerateUnionOutputWritesExpectedContent) {
    std::string path = writeTemp("BO_ 100 SimpleMsg: 8 ECM\n\n");
    DbcParser parser;
    parser.setVerbose(false);
    ASSERT_TRUE(parser.parseFile(path));

    std::string out_path = "/tmp/dbc_parser_test_union_out.h";
    std::ofstream out(out_path);
    parser.generateUnionOutput(&out);
    out.close();

    std::ifstream in(out_path);
    std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    EXPECT_NE(content.find("#ifndef"), std::string::npos);
    EXPECT_NE(content.find("SimpleMsg_MESSAGE_ID"), std::string::npos);
    remove(out_path.c_str());
}

TEST_F(DbcParserFileTest, RemoveDuplicatesErasesZeroDlcMessages) {
    // 0x80fe0011: extended, PDU2 (pf=0xFE), PGN 0xFE00, DLC 0 -- not a real frame.
    std::string content = "BO_ 2164129809 NotARealFrame: 0 ECM\n\n";
    std::string path = writeTemp(content);
    DbcParser parser;
    parser.setVerbose(false);
    ASSERT_TRUE(parser.parseFile(path));
    parser.removeDuplicates();
    parser.generateRunTimeFormat();
    EXPECT_TRUE(parser.dbc().empty());
}

TEST_F(DbcParserFileTest, RemoveDuplicatesGeneralizesUniqueMessagePerPgn) {
    // 0x80fe0011: extended, PGN 0xFE00 (65024).
    std::string content =
        "BO_ 2164129809 StatusMsg_11: 8 ECM\n"
        " SG_ Sig1 : 0|8@1+ (1,0) [0|255] \"\" Vector__XXX\n\n";
    std::string path = writeTemp(content);
    DbcParser parser;
    parser.setVerbose(false);
    ASSERT_TRUE(parser.parseFile(path));
    parser.removeDuplicates();
    parser.generateRunTimeFormat();
    // Stored under the bare PGN key, generalized (SA-suffix stripped from the name).
    ASSERT_NE(parser.dbc().find(65024), parser.dbc().end());
    EXPECT_EQ(parser.dbc().at(65024).name_, "StatusMsg");
}

TEST_F(DbcParserFileTest, RemoveDuplicatesKeepsDistinctMessagesUnderSamePgn) {
    // 0x80fe0011 and 0x80fe0022 share PGN 0xFE00 but differ in DLC (distinct content).
    std::string content =
        "BO_ 2164129809 StatusMsg_11: 8 ECM\n"
        " SG_ Sig1 : 0|8@1+ (1,0) [0|255] \"\" Vector__XXX\n\n"
        "BO_ 2164129826 StatusMsg_22: 4 ECM\n"
        " SG_ Sig1 : 0|8@1+ (1,0) [0|255] \"\" Vector__XXX\n\n";
    std::string path = writeTemp(content);
    DbcParser parser;
    parser.setVerbose(false);
    ASSERT_TRUE(parser.parseFile(path));
    parser.removeDuplicates();
    parser.generateRunTimeFormat();
    // Both kept individually: the first message in a new PGN group is stored via its
    // *generalized* (SA-reset) CAN ID (0x80fe0011 -> 0x80fe0000 = 2164129792), while
    // later messages added to an existing group keep their real CAN ID as-is.
    EXPECT_NE(parser.dbc().find(2164129792), parser.dbc().end());
    EXPECT_NE(parser.dbc().find(2164129826), parser.dbc().end());
    // ...and the DLC-8 one is also generalized under the bare PGN key.
    ASSERT_NE(parser.dbc().find(65024), parser.dbc().end());
    EXPECT_EQ(parser.dbc().at(65024).dlc_, 8);
}

TEST_F(DbcParserFileTest, RemoveDuplicatesDedupsIdenticalMessagesUnderSamePgn) {
    // Same PGN (0xFE00), same DLC and signal content, different CAN ID/name.
    std::string content =
        "BO_ 2164129809 StatusMsg_11: 8 ECM\n"
        " SG_ Sig1 : 0|8@1+ (1,0) [0|255] \"\" Vector__XXX\n\n"
        "BO_ 2164129826 StatusMsg_22: 8 ECM\n"
        " SG_ Sig1 : 0|8@1+ (1,0) [0|255] \"\" Vector__XXX\n\n";
    std::string path = writeTemp(content);
    DbcParser parser;
    parser.setVerbose(false);
    ASSERT_TRUE(parser.parseFile(path));
    parser.removeDuplicates();
    parser.generateRunTimeFormat();
    // Deduped to a single generalized entry under the bare PGN key.
    EXPECT_EQ(parser.dbc().size(), 1u);
    ASSERT_NE(parser.dbc().find(65024), parser.dbc().end());
}

// --- Message-level tests ---

TEST(MessageTest, ConstructorToleratesNameMissingTrailingColon) {
    Message msg("BO_ 100 MsgWithoutColon 8 ECM\n"); // malformed input missing the ':' after name
    EXPECT_EQ(msg.name(), "MsgWithoutColon");
}

TEST(MessageTest, EqualityComparesContentNotName) {
    Message a("BO_ 100 MsgA: 8 ECM\n");
    Message b("BO_ 200 MsgB: 8 ECM\n"); // different CAN ID/name, same PGN/DLC/no signals
    EXPECT_TRUE(a == b);
    EXPECT_FALSE(a != b);

    Message c("BO_ 300 MsgC: 4 ECM\n"); // different DLC
    EXPECT_TRUE(a != c);
}

TEST(MessageTest, EqualityDetectsDifferingSignalContent) {
    Message a("BO_ 100 MsgA: 8 ECM\n");
    a.addSignal(" SG_ Sig1 : 0|8@1+ (1,0) [0|255] \"\" Vector__XXX\n");
    Message b("BO_ 200 MsgB: 8 ECM\n");
    b.addSignal(" SG_ Sig1 : 8|8@1+ (1,0) [0|255] \"\" Vector__XXX\n"); // different start bit
    EXPECT_TRUE(a != b);
}

TEST(MessageTest, GeneralizedStripsValidTwoHexDigitSuffix) {
    Message msg("BO_ 2164129809 StatusMsg_0F: 8 ECM\n"); // "0F" is a valid hex suffix
    Message result = msg.generalized();
    EXPECT_EQ(result.name(), "StatusMsg");
}

TEST(MessageTest, GeneralizedLeavesNameWithoutValidSuffixUnchanged) {
    Message msg("BO_ 2164129809 StatusMessage: 8 ECM\n"); // "ge" is not hex
    Message result = msg.generalized();
    EXPECT_EQ(result.name(), "StatusMessage");
}

TEST(MessageTest, EqualityDetectsPgnMismatch) {
    Message a("BO_ 2164129809 MsgA: 8 ECM\n"); // PGN 0xFE00 (65024)
    Message b("BO_ 2163213585 MsgB: 8 ECM\n"); // PGN 0xF005 (61445) -- same DLC, no signals
    EXPECT_TRUE(a != b);
    EXPECT_FALSE(a == b);
}

TEST(MessageTest, GeneralizedRejectsInvalidSuffixVariants) {
    // The "_XX" strip only applies when the suffix is exactly 2 hex digits -- exercise
    // each way it can fail short of that (wrong length, non-hex first/second char).
    EXPECT_EQ(Message("BO_ 100 Status_ABC: 8 ECM\n").generalized().name(), "Status_ABC"); // length 3
    EXPECT_EQ(Message("BO_ 100 Status_XY: 8 ECM\n").generalized().name(), "Status_XY");   // 'X' not hex
    EXPECT_EQ(Message("BO_ 100 Status_AZ: 8 ECM\n").generalized().name(), "Status_AZ");   // 'Z' not hex
}

TEST(MessageTest, ToPgnStringWithGenericNameFalseKeepsFullName) {
    Message msg("BO_ 100 Msg_03: 8 ECM\n");
    std::string str = msg.toPgnString(false);
    EXPECT_NE(str.find("Msg_03"), std::string::npos); // full name kept, no SA-suffix stripping
}

TEST(MessageTest, LittleEndianIsTrueForEmptySignals) {
    Message msg("BO_ 100 Msg: 8 ECM\n");
    EXPECT_TRUE(msg.little_endian());
}

TEST(MessageTest, LittleEndianReflectsUniformSignalEndianness) {
    Message msg("BO_ 100 Msg: 8 ECM\n");
    msg.addSignal(" SG_ Sig1 : 0|8@0+ (1,0) [0|255] \"\" Vector__XXX\n"); // @0 = big-endian
    EXPECT_FALSE(msg.little_endian());
}

TEST(MessageTest, DetailsShowsStandardFrameForNonExtendedMessage) {
    Message msg("BO_ 100 Msg: 8 ECM\n"); // 100 has no extended (0x80000000) bit set
    EXPECT_EQ(msg.details(), " /* Standard Frame */ ");
}

TEST(MessageTest, DetailsShowsPgnForExtendedMessage) {
    Message msg("BO_ 2164129809 Msg: 8 ECM\n");
    EXPECT_NE(msg.details().find("PGN:"), std::string::npos);
}

TEST(MessageTest, AddSignalRejectsInvalidSignalLine) {
    Message msg("BO_ 100 Msg: 8 ECM\n");
    EXPECT_FALSE(msg.addSignal(" SG_ TooFewFields\n"));
}

TEST(MessageTest, AddValuesIsANoOpForZeroDlcMessages) {
    Message msg("BO_ 2164129809 NotARealFrame: 0 ECM\n");
    std::vector<std::string> fields = splitString("VAL_ 2164129809 SomeSignal 1 \"On\" ;", ' ');
    EXPECT_TRUE(msg.addValues(fields)); // returns true without touching signals_
}

TEST(MessageTest, ToStringIncludesNameAndDlc) {
    Message msg("BO_ 100 Msg: 8 ECM\n");
    std::string str = msg.toString();
    EXPECT_NE(str.find("Msg"), std::string::npos);
    EXPECT_NE(str.find("DLC: 8"), std::string::npos);
}

// --- Signal-level tests ---

TEST(SignalTest, RejectsLineWithTooFewFields) {
    Signal signal(" SG_ Name : 0|8@1+\n"); // missing the (scale,offset)/[min|max]/unit fields
    EXPECT_FALSE(signal.valid());
}

TEST(SignalTest, ParsesMultiplexedSignalSuffix) {
    // "m0" between the name and ":" marks a multiplexed signal variant.
    Signal signal(" SG_ Name m0 : 0|8@1+ (1,0) [0|255] \"\" Vector__XXX\n");
    EXPECT_TRUE(signal.valid());
    EXPECT_EQ(signal.name(), "Name");
}

TEST(SignalTest, EqualityIgnoresName) {
    Signal a(" SG_ NameA : 0|8@1+ (1,0) [0|255] \"\" Vector__XXX\n");
    Signal b(" SG_ NameB : 0|8@1+ (1,0) [0|255] \"\" Vector__XXX\n");
    EXPECT_TRUE(a == b);
}

TEST(SignalTest, EqualityDetectsFieldMismatch) {
    Signal a(" SG_ Name : 0|8@1+ (1,0) [0|255] \"\" Vector__XXX\n");
    Signal b(" SG_ Name : 8|8@1+ (1,0) [0|255] \"\" Vector__XXX\n"); // different start bit
    EXPECT_FALSE(a == b);
}

TEST(SignalTest, EqualityDetectsEachFieldMismatchIndividually) {
    // operator== short-circuits on the first mismatching field, so each field must be
    // the ONLY difference to independently exercise its comparison's false branch (a
    // single earlier-mismatch case, as in EqualityDetectsFieldMismatch, only ever
    // exercises the very first comparison in the chain).
    Signal base(" SG_ Name : 0|8@1+ (1,0) [0|255] \"\" Vector__XXX\n");
    EXPECT_FALSE(base == Signal(" SG_ Name : 0|16@1+ (1,0) [0|255] \"\" Vector__XXX\n")); // length_
    EXPECT_FALSE(base == Signal(" SG_ Name : 0|8@1+ (1,5) [0|255] \"\" Vector__XXX\n"));  // offset_
    EXPECT_FALSE(base == Signal(" SG_ Name : 0|8@1- (1,0) [0|255] \"\" Vector__XXX\n"));  // signed_
    EXPECT_FALSE(base == Signal(" SG_ Name : 0|8@1+ (2,0) [0|255] \"\" Vector__XXX\n"));  // scalar_
    EXPECT_FALSE(base == Signal(" SG_ Name : 0|8@1+ (1,0) [5|255] \"\" Vector__XXX\n"));  // min_
    EXPECT_FALSE(base == Signal(" SG_ Name : 0|8@1+ (1,0) [0|100] \"\" Vector__XXX\n"));  // max_
}

TEST(SignalTest, AddValueSkipsExactDuplicateEntry) {
    Signal signal(" SG_ Mode : 0|2@1+ (1,0) [0|3] \"\" Vector__XXX\n");
    auto fields = splitString("VAL_ 100 Mode 1 \"On\" ;", ' ');
    signal.addValue(fields);
    signal.addValue(fields); // exact duplicate (same value, same name) -- silently skipped
    std::string str = signal.toSpnString();
    // Only one occurrence of the enum entry, not two.
    EXPECT_EQ(str.find("{1, \"On\"}"), str.rfind("{1, \"On\"}"));
}

TEST(SignalTest, FieldTypeCoversAllBitWidthBuckets) {
    // fieldType() is private; its only caller is toGetSetMethod() (toPackedInt()
    // intentionally hardcodes a uniform uint64_t/int64_t bitfield base type instead,
    // for predictable packing across adjacent fields regardless of natural width).
    Signal u8(" SG_ S : 0|8@1+ (1,0) [0|255] \"\" Vector__XXX\n");
    Signal u16(" SG_ S : 0|16@1+ (1,0) [0|65535] \"\" Vector__XXX\n");
    Signal u32(" SG_ S : 0|32@1+ (1,0) [0|4294967295] \"\" Vector__XXX\n");
    Signal u64(" SG_ S : 0|64@1+ (1,0) [0|1] \"\" Vector__XXX\n");
    Signal i8(" SG_ S : 0|8@1- (1,0) [-128|127] \"\" Vector__XXX\n");
    Signal i16(" SG_ S : 0|16@1- (1,0) [-32768|32767] \"\" Vector__XXX\n");
    Signal i32(" SG_ S : 0|32@1- (1,0) [-2147483648|2147483647] \"\" Vector__XXX\n");
    Signal i64(" SG_ S : 0|64@1- (1,0) [0|1] \"\" Vector__XXX\n");
    EXPECT_NE(u8.toGetSetMethod().find("uint8_t"), std::string::npos);
    EXPECT_NE(u16.toGetSetMethod().find("uint16_t"), std::string::npos);
    EXPECT_NE(u32.toGetSetMethod().find("uint32_t"), std::string::npos);
    EXPECT_NE(u64.toGetSetMethod().find("uint64_t"), std::string::npos);
    EXPECT_NE(i8.toGetSetMethod().find("int8_t"), std::string::npos);
    EXPECT_NE(i16.toGetSetMethod().find("int16_t"), std::string::npos);
    EXPECT_NE(i32.toGetSetMethod().find("int32_t"), std::string::npos);
    EXPECT_NE(i64.toGetSetMethod().find("int64_t"), std::string::npos);
}

TEST(SignalTest, ToGetSetMethodUsesScaledAccessorWhenNotTrivial) {
    Signal signal(" SG_ Speed : 0|8@1+ (0.5,10) [0|255] \"\" Vector__XXX\n"); // scalar != 1
    std::string str = signal.toGetSetMethod();
    EXPECT_NE(str.find("double speed()"), std::string::npos);
}

TEST(SignalTest, ToStringIncludesNameAndFields) {
    Signal signal(" SG_ Speed : 0|8@1+ (1,0) [0|255] \"\" Vector__XXX\n");
    std::string str = signal.toString();
    EXPECT_NE(str.find("Speed"), std::string::npos);
}

TEST(SignalTest, ToStringIncludesEnumValues) {
    Signal signal(" SG_ Mode : 0|2@1+ (1,0) [0|3] \"\" Vector__XXX\n");
    signal.addValue(splitString("VAL_ 100 Mode 1 \"On\" 0 \"Off\" ;", ' '));
    std::string str = signal.toString();
    EXPECT_NE(str.find("On"), std::string::npos);
    EXPECT_NE(str.find("Off"), std::string::npos);
}

TEST(SignalTest, ToSpnStringWrapsLongEnumLists) {
    // Once the running output line exceeds 80 chars, toSpnString() wraps onto a new
    // "\n\t\t\t" line instead of growing one line unboundedly -- needs enough VAL_
    // entries in a single record to cross that threshold.
    Signal signal(" SG_ Mode : 0|8@1+ (1,0) [0|255] \"\" Vector__XXX\n");
    std::string val_line = "VAL_ 100 Mode";
    for (int i = 0; i < 10; i++) {
        val_line += " " + std::to_string(i) + " \"LongEnumValueName" + std::to_string(i) + "\"";
    }
    val_line += " ;";
    signal.addValue(splitString(val_line, ' '));
    std::string str = signal.toSpnString();
    EXPECT_GT(std::count(str.begin(), str.end(), '\n'), 1);
}

TEST(SignalTest, ToPackedIntUsesRawNameWhenSnakeCaseDisabled) {
    Signal signal(" SG_ MyValue : 0|8@1+ (1,0) [0|255] \"\" Vector__XXX\n");
    uint16_t start_bit = 0;
    std::string str = signal.toPackedInt(&start_bit, false);
    EXPECT_NE(str.find("MyValue"), std::string::npos); // raw name, not snake_case "my_value"
}

// --- Representative death tests for a couple of "should be structurally impossible"
// invariant checks (assertm/assert). Not exhaustive over every call site -- these
// guard internal consistency assumptions, not real external-input validation, so one
// representative case per class of check is enough to prove the pattern works. ---

TEST(SignalDeathTest, AddValueAssertsOnConflictingDuplicate) {
    Signal signal(" SG_ Mode : 0|2@1+ (1,0) [0|3] \"\" Vector__XXX\n");
    signal.addValue(splitString("VAL_ 100 Mode 1 \"On\" ;", ' '));
    EXPECT_DEATH(signal.addValue(splitString("VAL_ 100 Mode 1 \"Different\" ;", ' ')), "");
}

TEST(MessageDeathTest, AddValuesAssertsOnUnknownSignalName) {
    Message msg("BO_ 100 Msg: 8 ECM\n");
    msg.addSignal(" SG_ Sig1 : 0|8@1+ (1,0) [0|255] \"\" Vector__XXX\n");
    auto fields = splitString("VAL_ 100 NoSuchSignal 1 \"On\" ;", ' ');
    EXPECT_DEATH(msg.addValues(fields), "");
}

TEST(MessageDeathTest, LittleEndianAssertsOnMixedSignalEndianness) {
    // little_endian()'s header comment documents "aborts if not all signals have the
    // same value" -- exercises that contract directly.
    Message msg("BO_ 100 Msg: 8 ECM\n");
    msg.addSignal(" SG_ Sig1 : 0|8@1+ (1,0) [0|255] \"\" Vector__XXX\n"); // little-endian
    msg.addSignal(" SG_ Sig2 : 8|8@0+ (1,0) [0|255] \"\" Vector__XXX\n"); // big-endian -- mixed
    EXPECT_DEATH(msg.little_endian(), "");
}

TEST(SignalDeathTest, ToPackedIntAssertsWhenStartBitPrecedesPreviousField) {
    // toUnion() feeds signals in ascending/descending start-bit order so gap is never
    // negative in practice; forcing it here (as if two signals in a message overlapped)
    // exercises the "should be structurally impossible" diagnostic-then-assert path.
    Signal signal(" SG_ Name : 8|8@1+ (1,0) [0|255] \"\" Vector__XXX\n"); // little-endian, start_bit=8
    uint16_t start_bit = 20; // pretend the previous field already consumed through bit 20
    EXPECT_DEATH(signal.toPackedInt(&start_bit), "");
}
