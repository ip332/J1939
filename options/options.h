#pragma once

#include <functional>
#include <string>
#include <map>
#include <memory>
#include <iostream>
#include <sstream>
#include <vector>
#include <unordered_map>

// This base class holds all details of a single command line option, except the actual variable which is stored in the derived clas..
class Option {
protected:
    std::string flag_;
    std::string help_;
    bool argument_;
public:
    Option( const std::string & flag, const std::string & help, bool arg) : flag_(flag), help_(help), argument_(arg) {}
    virtual ~Option() {}

    const std::string &flag() const { return flag_; }

    bool argumentRequired() const { return argument_; }

    // Convert string into the native data type
    virtual void parseArgument(const char *str) = 0;

    // Print info about this option.
    virtual void print() const = 0;
};

class OptionStr : public Option {
    std::string *result_;
public:
    OptionStr(const std::string & flag, const std::string & help, std::string *variable) :
        Option(flag, help, true), result_(variable) {}
    void parseArgument(const char *str) override { result_->assign(str); }
    void print() const override {
        std::cout << "    " << flag_ << " <string> " << help_ << std::endl;
    }
};

class OptionInt : public Option {
    int *result_;
public:
    OptionInt(const std::string & flag, const std::string & help, int *variable) :
            Option(flag, help, true), result_(variable) {}
    void parseArgument(const char *str) override { *result_ = std::stoi(str); }
    void print() const override {
        std::cout << "    " << flag_ << " <integer> " << help_ << std::endl;
    }
};

class OptionBool : public Option {
    bool *result_;
public:
    OptionBool(const std::string & flag, const std::string & help, bool *variable) :
            Option(flag, help, false), result_(variable) {}
    void parseArgument(const char */*str*/) override { *result_ = true; }
    void print() const override {
        std::cout << "    " << flag_ << " " << help_ << std::endl;
    }
};

class OptionVectorStr : public Option {
    std::vector<std::string> *result_;
public:
    OptionVectorStr(const std::string & flag, const std::string & help, std::vector<std::string> *variable) :
            Option(flag, help, true), result_(variable) {}
    void parseArgument(const char *str) override { result_->push_back(std::string(str)); }
    void print() const override {
        std::cout << "    " << flag_ << " <string> " << help_ << std::endl;
    }
};

class OptionsManager {
    std::unordered_map<std::string, std::unique_ptr<Option>> options_;

    void checkFlag(const std::string &flag) {
        if (options_.count(flag)) {
            std::cerr << "Option " << flag << " was already added." << std::endl;
            exit(1);
        }
    }
public:
    void addOption( const std::string &flag, const std::string & info, std::string *var) {
        checkFlag(flag);
        options_.emplace(flag, std::make_unique<OptionStr>(flag, info, var));
    }
    void addOption( const std::string &flag, const std::string & info, std::vector<std::string> *var) {
        checkFlag(flag);
        options_.emplace(flag, std::make_unique<OptionVectorStr>(flag, info, var));
    }
    void addOption( const std::string &flag, const std::string & info, bool *var) {
        checkFlag(flag);
        options_.emplace(flag, std::make_unique<OptionBool>(flag, info, var));
    }
    void addOption( const std::string &flag, const std::string & info, int *var) {
        checkFlag(flag);
        options_.emplace(flag, std::make_unique<OptionInt>(flag, info, var));
    }

    void print() const {
        for ( const auto & opt : options_) {
            opt.second->print();
        }
    }

    bool processArgs(int argc, const char **argv) const {
        bool result = true;
        for (int i = 1; i < argc; i++) {
            const auto it = options_.find(argv[i]);
            if (it == options_.end()) {
                result = false;
                std::cout << "Option " << argv[i] << " is not defined" << std::endl;
                i++;
                continue;
            }
            if (it->second->argumentRequired()) {
                if (++i == argc) {
                    result  = false;
                    std::cout << "Option " << argv[i - 1] << " requires an argument" << std::endl;
                    break;
                }
            }
            it->second->parseArgument(argv[i]);
        }
        return result;
    }
};

// Helper class to pack several string options into a string
class PackedOptions {
    std::stringstream ss_;
    bool empty_ = true;
public:
    bool pack(const std::string & key, const std::string & value) {
        if (value.empty()) {
            return false;
        }
        if (!empty_) {
            ss_ << ',';
        }
        ss_ << key << ':' << value;
        empty_ = false;
        return true;
    }
    std::string str() const { return ss_.str(); }
    // On the receiver side, scan the packed options and run a callback for each of them
    static bool parse(const std::string &parameters, std::function<bool(const std::string & key, const std::string & value)> cb) {
        size_t start = 0;
        while(start < parameters.length()) {
            auto end = parameters.find(",", start);
            std::string parameter;
            if (end == std::string::npos) {
                parameter = parameters.substr(start);
                start = parameters.length();
            } else {
                parameter = parameters.substr(start, end - start);
                start = end + 1;
            }
            auto pos = parameter.find(":");
            if (pos == std::string::npos) {
                std::cout <<  "Invalid parameter: " << parameter;
                return false;
            }
            auto option = parameter.substr(0, pos);
            auto value = parameter.substr(pos + 1);
            if (!cb(option, value)) {
                return false;
            }
        }
        return true;
    }
};