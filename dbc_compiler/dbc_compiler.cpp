#include <iostream>
#include <memory>

#include "dbc_parser.h"
#include "options.h"

std::unique_ptr<std::ofstream> getFile(const std::string &path) {
    std::unique_ptr<std::ofstream> file;
    if (path.size() > 0) {
        file.reset(new std::ofstream());
        file->open(path.c_str());
        if (!file->is_open()) {
            std::cerr << "Error opening file " << path << std::endl;
            exit(2);
        }
    }
    return file;
}

int main(int argc, const char* argv[]) {
    OptionsManager options;

    std::vector<std::string> inputs;
    std::string pgn_path;
    std::string union_path;
    std::string var_name;

    options.addOption("-in", "Full path of the input DBC file (multiple input files are supported).", &inputs);
    options.addOption("-name", "Defines variable name in the output files. By default, the first input DBC name will be used.", &var_name);
    options.addOption("-pgn_path", "Full path of the output file to store the DBC content in PGN format.", &pgn_path);
    options.addOption("-union_path", "Full path of the output file to store the DBC content in the 'union' format.", &union_path);

    if (!options.processArgs(argc, argv)) {
        return 1;
    }

    if (inputs.size() == 0 || (union_path.empty() && pgn_path.empty())) {
        printf( "Usage: dbc_compiler <options>\n");
        printf( "   where <options> could be:\n");
        options.print();
        return 2;
    }
    // Open the output file(s).
    std::unique_ptr<std::ofstream> pgn_file = getFile(pgn_path);
    std::unique_ptr<std::ofstream> union_file = getFile(union_path);

    // Load all input files into RAM
    DbcParser parser;
    for (auto in : inputs) {
        if (!parser.parseFile(in)) {
            return -3;
        }
    }
    // TODO: add option to disable the following step.
    parser.removeDuplicates();

    // Optionally: set desired variable name.
    if (!var_name.empty()) {
        parser.setName(var_name);
    }
    parser.generatePgnOutput(pgn_file.get());
    parser.generateUnionOutput(union_file.get());

    if (union_file) {
        union_file->close();
    }
    if (pgn_file) {
        pgn_file->close();
    }
    return 0;
}