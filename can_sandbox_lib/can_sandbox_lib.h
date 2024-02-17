//
// Created by igor.prilepov on 6/17/22.
//

#ifndef J1939_CAN_SANDBOX_LIB_H
#define J1939_CAN_SANDBOX_LIB_H

#include "options.h"
#include "socket_can_stream.h"
#include "dbc_parser.h"
#include "j1939_parser.h"
#include "transport_protocol.h"

// This class defines all supported options and functionalities for reading/writing CAN stream.
class CanSandbox {
    OptionsManager options_;
    // Input/output streams could be defined as CAN interface name (can0) or a file name.
    std::string input_stream_;
    std::string output_stream_;
    // When commons library is available we can also define input/outputs as channel names from the bridge config.
    std::string input_channel_;
    std::string output_channel_;
    std::string bridge_config_file_;
    std::string filter_file_;
    // More than one DBC is supported therefore we'll store their names in a vector:
    std::vector<std::string> dbc_paths_;
    // Do not print every message
    bool quiet_ = false;
    // Treat all input messages as extended CAN Ids
    bool extended_ = false;
    // Print out fields even when all bits are set to 1 (undefined).
    bool show_unset_bits_ = false;
    // Automatically rewind the input file once EOF has been reached.
    bool rewind_ = false;

    std::unique_ptr<CanStream> input_;
    std::unique_ptr<CanStream> output_;
    DbcParser dbc_parser_;

    std::function<void(const J1939Parser & parser)> parser_callback_;

    void printMessage(const J1939Parser &parser, double time, bool show_unset);

public:
    explicit CanSandbox();

    // Application can add extra options
    void addOption( const std::string &flag, const std::string & info, std::string *var) {
        options_.addOption(flag, info, var);
    }
    void addOption( const std::string &flag, const std::string & info, std::vector<std::string> *var) {
        options_.addOption(flag, info, var);
    }
    void addOption( const std::string &flag, const std::string & info, bool *var) {
        options_.addOption(flag, info, var);
    }
    void addOption( const std::string &flag, const std::string & info, int *var) {
        options_.addOption(flag, info, var);
    }
    // Parses command line arguments, loads all necessary files/streams and return the result.
    bool parseOptions(int argc, const char *argv[]);

    // Defines extra processing for a parsed message
    void setParserCallback(std::function<void(const J1939Parser & parser)> cb) { parser_callback_ = cb; }

#ifdef COMMONS_AVAILABLE
    std::shared_ptr<DataBridge> bridge() { return bridge_; }
#endif
    // Start input stream handling.
    void startProcessing();
};


#endif //J1939_CAN_SANDBOX_LIB_H
