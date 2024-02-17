//
// Created by Igor on 2021-09-10.
//

#ifndef DBCPARSER_DBC_PARSER_H
#define DBCPARSER_DBC_PARSER_H

#include "dbc_message.h"

#include <fstream>
#include <memory>

// This class processes a single DBC file and generates the messages representation in several forms.
class DbcParser {
    // Main storage to load all messages from the DBC file and index them by their full CAN ID.
    std::map<uint32_t, Message> raw_messages_;
    // Contains all extended CAN ID.
    std::vector<uint32_t> extended_ids_;
    // The DBC file basename (for debugging purposes).
    std::string dbc_name_;
    // The variable name for the message container in the generated code.
    std::string name_;
    // Shows if parser expects to read a signal description or the top level records.
    bool top_level_;
    // Last message added to the storage. Is used to add signals.
    std::unique_ptr<Message> last_message_;

    // Stores the DBC content in the run-time optimized format so we could use DBC files directly, like libcandbc does.
    std::map<uint32_t, PGN> rt_messages_;
    // We might also need ability to find ID by message name:
    std::map <std::string, uint32_t> id_by_name_;

    // Enables/disables summary on every message loaded and file parsed (enabled by default).
    bool verbose_;

    // Set to true if all messages loaded from the source DBC have extended flag.
    bool extended_only_;

    // Parses given line.
    void topLevelParser(const std::string &line);

    void signalsParser(const std::string &line);

    void storeLastMessage();
public:
    DbcParser() : verbose_(false) {}

    // Parses the provided DBC file and stores its content in the raw_messages_ storage
    bool parseFile(const std::string &dbc_path);

    // Removes duplicated messages and updates the raw_messages_ container.
    void removeDuplicates();

    void setName(const std::string &name) { name_ = name; }

    // Outputs the raw_messages content into the provided stream.
    void generatePgnOutput(std::ofstream *out) const;

    // Outputs the raw_messages content into the provided stream as union of a packed structure and a bytes array.
    void generateUnionOutput(std::ofstream *out) const;

    // Generates run-time format into rt_messages_
    void generateRunTimeFormat();

    // Returns reference to the run-time messages.
    const std::map<uint32_t, PGN> & dbc() const { return rt_messages_; }

    // Returns CAN ID (or PGN) for given name from rt_messages_
    std::unique_ptr <uint32_t> getId(const std::string &name) const;

    std::string dbcName() const { return name_; }

    // Controls verbose mode.
    void setVerbose(bool value) { verbose_ = value; }

    // Returns the extended flag
    bool extendedFormatOnly() const { return extended_only_; }
};


#endif //DBCPARSER_DBC_PARSER_H
