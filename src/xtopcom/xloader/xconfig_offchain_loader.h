// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include <unordered_map>

#include <json/json.h>
#include "xconfig/xconfig_face.h"

NS_BEG2(top, loader)


class xconfig_offchain_loader_t : public config::xconfig_loader_face_t
{
public:
    explicit xconfig_offchain_loader_t(const std::string& conf_file, const std::string& config_extra_file = "");

    void start() override;
    void stop() override;
    virtual bool save_conf(const std::map<std::string, std::string>& map) override;
    virtual bool fetch_all(std::map<std::string, std::string>& map) override;

private:
    std::string m_config_file{};
    std::string m_config_extra_file{};

    std::string get_file_content(const std::string& filepath);
    void extract(const std::string& prefix, const Json::Value& arr, std::map<std::string, std::string>& params);
};

NS_END2
