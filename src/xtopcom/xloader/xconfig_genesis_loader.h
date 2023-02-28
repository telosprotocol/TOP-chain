// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include <unordered_map>

#include "xconfig/xconfig_face.h"
#include "xdata/xrootblock.h"

NS_BEG2(top, loader)

class xconfig_genesis_loader_t : public config::xconfig_loader_face_t {
 public:
    explicit xconfig_genesis_loader_t(const std::string& conf_file);

    void start() override;
    void stop() override;
    virtual bool save_conf(const std::map<std::string, std::string>& map) override;
    virtual bool fetch_all(std::map<std::string, std::string>& map) override;
    bool         extract_genesis_para(data::xrootblock_para_t & para);

 private:
    bool extract_genesis_para_accounts(const Json::Value & json_root, data::xrootblock_para_t & para);
    bool extract_genesis_para_seedNodes(const Json::Value & json_root, data::xrootblock_para_t & para);
    bool extract_genesis_para_genesis_timestamp(const Json::Value & json_root, data::xrootblock_para_t & para);
    bool extract_genesis_para_ca_relation(const Json::Value& json_root, data::xrootblock_para_t& para);
    std::string extract_genesis_content(const std::string& filecontent);
    std::string get_file_content(const std::string& filepath);

 private:
    std::string m_genesis_info;
};

NS_END2
