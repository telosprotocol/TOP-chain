//
//  args_parser.h
//
//  Created by @author on 01/15/2019.
//  Copyright (c) 2017-2019 Telos Foundation & contributors
//

#pragma once

#include <stdint.h>
#include <map>
#include <vector>
#include <string>

#include "top_utils.h"
#include "error_code.h"

namespace top {

enum KeyFlag { kInvalidKey = -1, kNoValue, kMaybeValue, kMustValue };

class ArgsParser {
public:
    ArgsParser();
    ~ArgsParser();
    bool AddArgType(const char short_name, const char* long_name, KeyFlag flag);
    int Parse(const std::string& paras, std::string& err_pos);
    int GetParam(const std::string& key, int& value);
    int GetParam(const std::string& key, uint64_t& value);
    int GetParam(const std::string& key, uint16_t& value);
    int GetParam(const std::string& key, std::string& value);
    bool HasParam(const std::string& key);

private:
    KeyFlag GetKeyFlag(std::string &key);
    void RemoveKeyFlag(std::string & paras);
    bool GetWord(std::string & params, std::string & word);
    bool IsDuplicateKey(const std::string &key);
    bool GetLongName(const std::string& key, std::string& long_name);  // key is short_name or long_name, return long_name
    bool GetShortName(const std::string& key, std::string& short_name);  // key is short_name or long_name, return short_name
    bool CheckKeyRegistered(const std::string& key);

    struct Option {
        std::string long_name;
        char short_name;
        KeyFlag flag;
    };

    std::vector<Option> args_;
    std::map<std::string, std::vector<std::string>> result_;

    DISALLOW_COPY_AND_ASSIGN(ArgsParser);
};

}  // namespace top
