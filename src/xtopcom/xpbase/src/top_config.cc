//
//  top_config.cc
//
//  Created by Charlie Xie on 01/15/2019.
//  Copyright (c) 2017-2019 Telos Foundation & contributors
//

#include "xpbase/base/top_config.h"

#include <stdio.h>
#include <string>
#include <utility>

#include "xpbase/base/top_log.h"
#include "xpbase/base/check_cast.h"
#include "xpbase/base/top_utils.h"

namespace top {

namespace base {

static const uint32_t kConfigMaxLen = 1024 * 1024;

Config::Config() {}
Config::~Config() {}

bool Config::Get(const std::string& field, const std::string& key, std::string& value)  const {
    auto iter = config_map_.find(field);
    if (iter == config_map_.end()) {
        TOP_WARN("invalid field[%s]", field.c_str());
        return false;
    }

    auto kv_iter = iter->second.find(key);
    if (kv_iter == iter->second.end()) {
        TOP_WARN("invalid field[%s] key[%s]", field.c_str(), key.c_str());
        return false;
    }

    value = kv_iter->second;
    return true;
}

bool Config::Get(const std::string& field, const std::string& key, bool& value)  const {
    std::string str_value;
    if (!Get(field, key, str_value)) {
        return false;
    }

    if (str_value == "1") {
        value = true;
        return true;
    }

    if (str_value == "true") {
        value = true;
        return true;
    }

    if (str_value == "0") {
        value = false;
        return true;
    }

    if (str_value == "false") {
        value = false;
        return true;
    }
    return false;
}

bool Config::Get(const std::string& field, const std::string& key, int16_t& value) const {
    std::string str_value;
    if (!Get(field, key, str_value)) {
        return false;
    }

    try {
        value = check_cast<int16_t>(str_value.c_str());
        return true;
    } catch (...) {
        return false;
    }
    return false;
}

bool Config::Get(const std::string& field, const std::string& key, uint16_t& value) const {
    std::string str_value;
    if (!Get(field, key, str_value)) {
        return false;
    }

    try {
        value = check_cast<uint16_t>(str_value.c_str());
        return true;
    } catch (...) {
        return false;
    }
    return false;
}

bool Config::Get(const std::string& field, const std::string& key, int32_t& value)  const {
    std::string str_value;
    if (!Get(field, key, str_value)) {
        return false;
    }

    try {
        value = check_cast<int32_t>(str_value.c_str());
        return true;
    } catch (...) {
        return false;
    }
    return false;
}

bool Config::Get(const std::string& field, const std::string& key, uint32_t& value) const {
    std::string str_value;
    if (!Get(field, key, str_value)) {
        return false;
    }

    try {
        value = check_cast<uint32_t>(str_value.c_str());
        return true;
    } catch (...) {
        return false;
    }
    return false;
}

bool Config::Get(const std::string& field, const std::string& key, int64_t& value) const {
    std::string str_value;
    if (!Get(field, key, str_value)) {
        return false;
    }

    try {
        value = check_cast<int64_t>(str_value.c_str());
        return true;
    } catch (...) {
        return false;
    }
    return false;
}

bool Config::Get(const std::string& field, const std::string& key, uint64_t& value) const {
    std::string str_value;
    if (!Get(field, key, str_value)) {
        return false;
    }

    try {
        value = check_cast<uint64_t>(str_value.c_str());
        return true;
    } catch (...) {
        return false;
    }
    return false;
}

bool Config::Get(const std::string& field, const std::string& key, float& value)  const {
    std::string str_value;
    if (!Get(field, key, str_value)) {
        return false;
    }

    try {
        value = check_cast<float>(str_value.c_str());
        return true;
    } catch (...) {
        return false;
    }
    return false;
}

bool Config::Get(const std::string& field, const std::string& key, double& value) const {
    std::string str_value;
    if (!Get(field, key, str_value)) {
        return false;
    }

    try {
        value = check_cast<double>(str_value.c_str());
        return true;
    } catch (...) {
        return false;
    }
    return false;
}

bool Config::Set(const std::string& field, const std::string& key, const char* value) {
    if (value == nullptr) {
        return false;
    }

    return Set(field, key, std::string(value));
}

bool Config::Set(const std::string& field, const std::string& key, std::string value) {
    auto iter = config_map_.find(field);
    if (iter == config_map_.end()) {
        auto ins_iter = config_map_.insert(std::make_pair(
                field, std::map<std::string, std::string>()));
        if (!ins_iter.second) {
            return false;
        }

        iter = config_map_.find(field);
    }

    iter->second[key] = value;
    return true;
}

bool Config::Set(const std::string& field, const std::string& key, bool value) {
    std::string val = check_cast<std::string>(value);
    return Set(field, key, val);
}

bool Config::Set(const std::string& field, const std::string& key, int16_t value) {
    std::string val = check_cast<std::string>(value);
    return Set(field, key, val);
}

bool Config::Set(const std::string& field, const std::string& key, uint16_t value) {
    std::string val = check_cast<std::string>(value);
    return Set(field, key, val);
}

bool Config::Set(const std::string& field, const std::string& key, int32_t value) {
    std::string val = check_cast<std::string>(value);
    return Set(field, key, val);
}

bool Config::Set(const std::string& field, const std::string& key, uint32_t value) {
    std::string val = check_cast<std::string>(value);
    return Set(field, key, val);
}

bool Config::Set(const std::string& field, const std::string& key, int64_t value) {
    std::string val = check_cast<std::string>(value);
    return Set(field, key, val);
}

bool Config::Set(const std::string& field, const std::string& key, uint64_t value) {
    std::string val = check_cast<std::string>(value);
    return Set(field, key, val);
}

bool Config::Set(const std::string& field, const std::string& key, float value) {
    std::string val = check_cast<std::string>(value);
    return Set(field, key, val);
}

bool Config::Set(const std::string& field, const std::string& key, double value) {
    std::string val = check_cast<std::string>(value);
    return Set(field, key, val);
}

bool Config::DumpConfig(const std::string& conf) {
    FILE* fd = fopen(conf.c_str(), "w");
    if (fd == NULL) {
        TOP_ERROR("open config file[%s] failed!", conf.c_str());
        return false;
    }

    bool res = true;
    for (auto iter = config_map_.begin(); iter != config_map_.end(); ++iter) {
        std::string filed = std::string("[") + iter->first + "]\n";
        size_t ws = fwrite(filed.c_str(), 1, filed.size(), fd);
        if (ws != filed.size()) {
            TOP_ERROR("write file failed!");
            res = false;
            break;
        }

        for (auto key_iter = iter->second.begin(); key_iter != iter->second.end(); ++key_iter) {
            std::string kv = key_iter->first + "=" + key_iter->second + "\n";
            size_t ws = fwrite(kv.c_str(), 1, kv.size(), fd);
            if (ws != kv.size()) {
                TOP_ERROR("write file failed!");
                res = false;
                break;
            }
        }

        if (!res) {
            break;
        }
    }
    fclose(fd);
    return res;
}

bool Config::Init(const std::string& conf) {
    FILE* fd = fopen(conf.c_str(), "r");
    if (fd == NULL) {
        TOP_ERROR("open config file[%s] failed!", conf.c_str());
        return false;
    }

    std::string filed;
    char* read_buf = new char[kConfigMaxLen];
    bool res = true;
    while (true) {
        char* read_res = fgets(read_buf, kConfigMaxLen, fd);
        if (read_res == NULL) {
            break;
        }

        std::string line(read_buf);
        if (line.size() >= kConfigMaxLen) {
            TOP_ERROR("line size exceeded %d", kConfigMaxLen);
            res = false;
            break;
        }

        if (line[0] == '#') {
            continue;
        }

        if (line.find(']') != std::string::npos) {
            if (!HandleFiled(line, filed)) {
                TOP_ERROR("handle field failed[%s][%d]", line.c_str(), line.find(']'));
                res = false;
                break;
            }
            continue;
        }

        if (line.find('=') != std::string::npos) {
            if (!HandleKeyValue(filed, line)) {
                TOP_ERROR("handle key value failed[%s]", line.c_str());
                res = false;
                break;
            }
            continue;
        }

        for (uint32_t i = 0; i < line.size(); ++i) {
            if (line[i] != ' ' && line[i] != ' ' && line[i] != '\n') {
                TOP_ERROR("line illegal[%s]", line.c_str());
                res = false;
                break;
            }
        }

        if (!res) {
            break;
        }
    }

    fclose(fd);
    delete []read_buf;
    return res;
}

bool Config::HandleKeyValue(const std::string& filed, const std::string& key_value) {
    size_t eq_pos = key_value.find('=');
    int key_start_pos = -1;
    std::string key("");
    for (size_t i = 0; i < eq_pos; ++i) {
        if (key_value[i] == '#') {
            break;
        }

        if (key_value[i] == '=' || key_value[i] == '+' || key_value[i] == '-' ||
                key_value[i] == '*' || key_value[i] == '/') {
            TOP_ERROR("invalid char[%c]", key_value[i]);
            return false;
        }

        if (key_value[i] == ' ' || key_value[i] == '\t') {
            if (key_start_pos == -1) {
                continue;
            }

            for (size_t j = i; j < eq_pos; ++j) {
                if (key_value[j] != ' ' && key_value[j] != '\t' && key_value[j] != '\n') {
                    TOP_ERROR("invalid char[ ][\\t][\\n]");
                    return false;
                }
            }

            key = std::string(key_value.begin() + key_start_pos, key_value.begin() +  i);
            break;
        }

        if (key_start_pos == -1) {
            key_start_pos = i;
        }
    }
    if (key_start_pos == -1 || static_cast<int>(eq_pos) <= key_start_pos) {
        TOP_ERROR("invalid key_start_pos[%d]", key_start_pos);
        return false;
    }
    if (key.empty()) {
        key = std::string(key_value.begin() + key_start_pos, key_value.begin() + eq_pos);
    }
    if (key.empty()) {
        TOP_ERROR("invalid key_start_pos[%d]", key_start_pos);
        return false;
    }

    int value_start_pos = eq_pos + 1;
    std::string value("");
    for (size_t i = eq_pos + 1; i < key_value.size(); ++i) {
        if (key_value[i] == '#') {
            if (i > check_cast<size_t>(value_start_pos)) {
                value = std::string(key_value.begin() + value_start_pos, key_value.begin() + i);
            }
            break;
        }

        if (key_value[i] == '=') {
            TOP_ERROR("invalid char[%c]", key_value[i]);
            return false;
        }

        if (key_value[i] == ' ') {
            continue;
        }

        if (key_value[i] == '\n') {
            if (value_start_pos == -1) {
                continue;
            }

            for (size_t j = i; j < key_value.size(); ++j) {
                if (key_value[j] == '#') {
                    break;
                }

                if (key_value[j] != ' ' && key_value[j] != '\t' && key_value[j] != '\n') {
                    TOP_ERROR("invalid char[ ][\\t][\\n]");
                    return false;
                }
            }

            value = std::string(key_value.begin() + value_start_pos, key_value.begin() + i);
            break;
        }

        if (value_start_pos == -1) {
            value_start_pos = i;
        }
    }
    if (value_start_pos == -1 || static_cast<int>(key_value.size()) <= value_start_pos) {
        TOP_ERROR("invalid value_start_pos[%d]", value_start_pos);
        return false;
    }
    if (value.empty()) {
        value = std::string(key_value.begin() + value_start_pos, key_value.end() - 1);
    }
    TrimString(value);
    return AddKey(filed, key, value);
}

bool Config::HandleFiled(const std::string& field, std::string& field_val) {
    int start_pos = -1;
    for (uint32_t i = 0; i < field.size(); ++i) {
        if (field[i] == '#') {
            break;
        }

        if (field[i] == '=' || field[i] == '+' || field[i] == '-' ||
                field[i] == '*' || field[i] == '/') {
            TOP_ERROR("unvalid char[%c]", field[i]);
            return false;
        }

        if (field[i] == '[') {
            start_pos = i + 1;
            continue;
        }

        if (field[i] == ']') {
            for (uint32_t j = i + 1; j < field.size(); ++j) {
                if (field[j] == '#') {
                    break;
                }

                if (field[j] != ' ' && field[j] != '\t' && field[j] != '\n') {
                    TOP_ERROR("unvalid char[ ][\\n]");
                    return false;
                }
            }

            std::string str_filed(field.begin() + start_pos, field.begin() + i);
            AddField(str_filed);
            field_val = str_filed;
            return true;
        }

        if (field[i] == ' ') {
            if (start_pos == -1) {
                continue;
            }

            TOP_ERROR("unvalid char[ ][\\n]");
            return false;
        }

        if (start_pos == -1) {
            TOP_ERROR("unvalid start_pos");
            return false;
        }
    }
    return false;
}

bool Config::AddField(const std::string& field) {
    auto iter = config_map_.find(field);
    if (iter != config_map_.end()) {
        return false;
    }

    auto ins_iter = config_map_.insert(std::make_pair(
            field,
            std::map<std::string, std::string>()));
    return ins_iter.second;
}

bool Config::AddKey(const std::string& field, const std::string& key, const std::string& value) {
    auto iter = config_map_.find(field);
    if (iter == config_map_.end()) {
        return false;
    }
    auto ins_iter = iter->second.insert(std::make_pair(key, value));
    return ins_iter.second;
}

}  // namespace base

}  // namespace top
