//
//  args_parser.h
//
//  Created by @author on 01/15/2019.
//  Copyright (c) 2017-2019 Telos Foundation & contributors
//

#include "xpbase/base/args_parser.h"

#include <cassert>

#include <list>

#include "xbase/xutl.h"

namespace top {

ArgsParser::ArgsParser() {}

ArgsParser::~ArgsParser() {}

int ArgsParser::GetParam(const std::string& key, uint64_t& value) {
    auto iter = result_.find(key);
    if (iter == result_.end()) {
        return kadmlia::kKadFailed;
    }

    if (iter->second.empty()) {
        return kadmlia::kKadFailed;
    }

    try {
        value = base::xstring_utl::touint64(iter->second[0].c_str());
    } catch (...) {
        return kadmlia::kKadFailed;
    }

    return kadmlia::kKadSuccess;
}



int ArgsParser::GetParam(const std::string& key, int& value) {
    auto iter = result_.find(key);
    if (iter == result_.end()) {
        return kadmlia::kKadFailed;
    }

    if (iter->second.empty()) {
        return kadmlia::kKadFailed;
    }

    try {
        value = base::xstring_utl::toint32(iter->second[0].c_str());
    } catch (...) {
        return kadmlia::kKadFailed;
    }

    return kadmlia::kKadSuccess;
}

int ArgsParser::GetParam(const std::string& key, uint16_t& value) {
    int tmp_value = 0;
    if (GetParam(key, tmp_value) != 0) {
        return kadmlia::kKadFailed;
    }

    value = static_cast<uint16_t>(tmp_value);
    return kadmlia::kKadSuccess;
}

int ArgsParser::GetParam(const std::string& key, std::string& value) {
    if (key.empty()) {
        return kadmlia::kKadFailed;
    }

    auto iter = result_.find(key);
    auto find_iter = iter;
    if (iter != result_.end()) {
        find_iter = iter;
    } else {
        std::string long_name;
        std::string short_name;
        if (key.size() == 1) {
            short_name = key;
            // key is short_name
            if (!GetLongName(key, long_name)) {
                return kadmlia::kKadFailed;
            }
        } else {
            // key is long_name
            long_name = key;
            if (!GetShortName(key, short_name)) { // short_name maybe empty
                return kadmlia::kKadFailed;
            }
        } // end if (key.size

        iter = result_.find(long_name);
        if (iter != result_.end()) {
            find_iter = iter;
        } else {
            iter = result_.find(short_name);
            if (iter == result_.end()) {
                return kadmlia::kKadFailed;
            }
            find_iter = iter;
        } // end if (iter !=

    } // end if (iter != ) else

    if (find_iter->second.empty()) {
        return kadmlia::kKadFailed;
    }

    value = find_iter->second[0];
    return kadmlia::kKadSuccess;
}

bool ArgsParser::HasParam(const std::string& key) {
    if (key.empty()) {
        return false;
    }

    auto iter = result_.find(key);
    if (iter != result_.end()) {
        return true;
    }

    std::string long_name;
    std::string short_name;
    if (key.size() == 1) {
        short_name = key;
        // key is short_name
        if (!GetLongName(key, long_name)) {
            return false;
        }
    } else {
        // key is long_name
        long_name = key;
        if (!GetShortName(key, short_name)) { // short_name maybe empty
            return false;
        }
    }

    iter = result_.find(long_name);
    if (iter != result_.end()) {
        return true;
    }

    iter = result_.find(short_name);
    if (iter != result_.end()) {
        return true;
    }

    return false;
}

// key is short_name or long_name, return long_name
bool ArgsParser::GetLongName(const std::string& key, std::string& long_name) {
    if (key.size() != 1) {
        return false;
    }
    for (uint32_t i = 0; i < args_.size(); ++i) {
        char tmp_short_name =  args_[i].short_name;
        std::string tmp_long_name  = args_[i].long_name;

        if (key[0] == tmp_short_name) {
            long_name = tmp_long_name;
            return true;
        }
    }
    return false;
}

// key is short_name or long_name, return short_name
bool ArgsParser::GetShortName(const std::string& key, std::string& short_name) {
    if (key.size() <= 1) {
        return false;
    }
    for (uint32_t i = 0; i < args_.size(); ++i) {
        char tmp_short_name =  args_[i].short_name;
        std::string tmp_long_name  = args_[i].long_name;

        // high priority looking for long_name
        if (key.compare(tmp_long_name) == 0) {
            short_name = tmp_short_name; // maybe 0
            return true;
        }
    }
    return false;
}


// short_name maybe empty bug long_name must not empty
bool ArgsParser::AddArgType(char short_name, const char * long_name, KeyFlag flag) {
    assert(NULL != long_name); // for debug
    if (NULL == long_name) {
        return false;
    }
    Option tmp;
    tmp.long_name = long_name;
    tmp.short_name = short_name;
    tmp.flag = flag;
    args_.push_back(tmp);
    return true;
}

bool ArgsParser::CheckKeyRegistered(const std::string& key) {
    if (key[0] != '-') {
        // is value
        return true;
    }

    for (uint32_t i = 0; i < args_.size(); ++i) {
        std::string short_name = "-";
        std::string long_name = "--";
        short_name += args_[i].short_name;
        long_name += args_[i].long_name;
        if (0 == key.compare(short_name) || (0 == key.compare(long_name))) {
            return true;
        }
    }
    return false;
}

KeyFlag ArgsParser::GetKeyFlag(std::string &key) {
    for (uint32_t i = 0; i < args_.size(); ++i) {
        std::string short_name = "-";
        std::string long_name = "--";
        short_name += args_[i].short_name;
        long_name += args_[i].long_name;
        if (0 == key.compare(short_name) || (0 == key.compare(long_name))) {
            RemoveKeyFlag(key);
            return args_[i].flag;
        }
    }
    return kInvalidKey;
}

void ArgsParser::RemoveKeyFlag(std::string& word) {
    if (word.size() >= 2) {
        if (word[1] == '-') {
            word.erase(1, 1);
        }
        if (word[0] == '-') {
            word.erase(0, 1);
        }
    }
}

bool ArgsParser::GetWord(std::string& params, std::string& word) {
    size_t not_space_pos = params.find_first_not_of(' ', 0);
    if (not_space_pos == std::string::npos) {
        params.clear();
        word.clear();
        return true;
    }

    int length = params.size();
    std::list<char> special_char;
    for (int i = not_space_pos; i < length; i++) {
        char cur = params[i];
        bool is_ok = false;
        switch (cur) {
            case ' ': {
                if (special_char.empty()) {
                    if (i != (length - 1)) {
                        params = std::string(params, i + 1, length - i - 1);
                    } else {
                        params.clear();
                    }
                    is_ok = true;
                } else {
                    if (special_char.back() == '\\') {
                        special_char.pop_back();
                    }
                    word.append(1, cur);
                }
                break;
            }
            case '"': {
                if (special_char.empty()) {
                    special_char.push_back(cur);
                } else if (special_char.back() == cur) {
                    special_char.pop_back();
                } else if (special_char.back() == '\\') {
                    word.append(1, cur);
                    special_char.pop_back();
                } else {
                    word.clear();
                    return false;
                }
                break;
            }
            case '\\': {
                if (special_char.empty()) {
                    special_char.push_back(cur);
                } else if (special_char.back() == '"') {
                    if (i < (length - 1)) {
                        if ('"' == params[i + 1] || '\\' == params[i + 1]) {
                            special_char.push_back(cur);
                        } else {
                            word.append(1, cur);
                        }
                    } else {
                        word.clear();
                        return false;
                    }
                } else if ('\\' == special_char.back()) {
                    word.append(1, cur);
                    special_char.pop_back();
                } else {
                    word.clear();
                    return false;
                }
                break;
            }
            default: {
                word.append(1, params[i]);
                if (i == (length - 1)) {
                    is_ok = true;
                    params.clear();
                }
                break;
            }
        }
        if (is_ok) {
            return true;
        }
    }

    if (special_char.empty()) {
        params.clear();
        return true;
    }
    return false;
}

bool ArgsParser::IsDuplicateKey(const std::string& key) {
    if (result_.find(key) != result_.end()) {
        return true;
    }

    for (uint32_t i = 0; i < args_.size(); ++i) {
        if ((key.compare(args_[i].long_name) == 0 &&
                result_.find(std::string(1, args_[i].short_name)) != result_.end()) ||
                (key.compare(std::string(1, args_[i].short_name)) == 0 &&
                result_.find(args_[i].long_name) != result_.end())) {
            return true;
        }
    }
    return false;
}

int ArgsParser::Parse(const std::string& params, std::string& err_pos) {
    std::string tmp_string = params;
    KeyFlag key_flag = kInvalidKey;
    std::string sKey = "";
    bool finded_value = false;
    while (!tmp_string.empty()) {
        std::string word = "";
        bool ret = GetWord(tmp_string, word);
        if (ret == false) {
            err_pos = tmp_string;
            return kadmlia::kKadFailed;
        }

        if (!CheckKeyRegistered(word)) {
            err_pos = tmp_string;
            return kadmlia::kKadFailed;
        }
        KeyFlag tmp_flag = GetKeyFlag(word);
        if (IsDuplicateKey(word)) {
            err_pos = tmp_string;
            return kadmlia::kKadFailed;
        }
        if (tmp_flag != kInvalidKey) {
            if (tmp_flag == kMustValue && key_flag == kMustValue && !finded_value) {
                err_pos = tmp_string;
                return kadmlia::kKadFailed;
            }
            key_flag = tmp_flag;
            std::vector<std::string> tmp;
            result_[word] = tmp;
            sKey = word;
            finded_value = false;
        } else {
            switch (key_flag) {
            case kMaybeValue:
            case kMustValue: {
                auto it = result_.find(sKey);
                if (it != result_.end()) {
                    it->second.push_back(word);
                    finded_value = true;
                } else {
                    err_pos = tmp_string;
                    return kadmlia::kKadFailed;
                }
                break;
            }
            case kNoValue:
                err_pos = tmp_string;
                return kadmlia::kKadFailed;
            default:
                err_pos = tmp_string;
                return kadmlia::kKadFailed;
            }
        }

    }
    return kadmlia::kKadSuccess;
}

}  // namespace top
