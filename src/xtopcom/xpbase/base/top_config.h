//
//  top_config.h
//
//  Created by Charlie Xie on 01/15/2019.
//  Copyright (c) 2017-2019 Telos Foundation & contributors
//

#pragma once

#include <stdint.h>
#include <string>
#include <map>

#include "xpbase/base/top_utils.h"

namespace top {

namespace base {

class Config {
public:
    Config();
    ~Config();

    Config(const Config& other) {
        config_map_ = other.config_map_;
    }

    Config& operator=(const Config& other) {
        if (this != &other) {
            config_map_ = other.config_map_;
        }
        return *this;
    }

    bool Init(const std::string& conf);
    bool DumpConfig(const std::string& conf);
    bool Get(const std::string& field, const std::string& key, std::string& value) const;
    bool Get(const std::string& field, const std::string& key, bool& value) const;
    bool Get(const std::string& field, const std::string& key, int16_t& value) const;
    bool Get(const std::string& field, const std::string& key, uint16_t& value) const;
    bool Get(const std::string& field, const std::string& key, int32_t& value) const;
    bool Get(const std::string& field, const std::string& key, uint32_t& value) const;
    bool Get(const std::string& field, const std::string& key, int64_t& value) const;
    bool Get(const std::string& field, const std::string& key, uint64_t& value) const;
    bool Get(const std::string& field, const std::string& key, float& value) const;
    bool Get(const std::string& field, const std::string& key, double& value) const;

    bool Set(const std::string& field, const std::string& key, const char* value);
    bool Set(const std::string& field, const std::string& key, std::string value);
    bool Set(const std::string& field, const std::string& key, int16_t value);
    bool Set(const std::string& field, const std::string& key, uint16_t value);
    bool Set(const std::string& field, const std::string& key, bool value);
    bool Set(const std::string& field, const std::string& key, int32_t value);
    bool Set(const std::string& field, const std::string& key, uint32_t value);
    bool Set(const std::string& field, const std::string& key, int64_t value);
    bool Set(const std::string& field, const std::string& key, uint64_t value);
    bool Set(const std::string& field, const std::string& key, float value);
    bool Set(const std::string& field, const std::string& key, double value);

private:
    bool AddField(const std::string& field);
    bool AddKey(const std::string& field, const std::string& key, const std::string& value);
    bool HandleFiled(const std::string& field, std::string& field_val);
    bool HandleKeyValue(const std::string& filed, const std::string& key_value);

    std::map<std::string, std::map<std::string, std::string>> config_map_;
};

}  // namespace base

}  // namespace top
