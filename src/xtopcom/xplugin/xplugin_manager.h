// Copyright (c) 2018-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include "xbase/xns_macro.h"
#include "xplugin/xplugin.h"

#include <memory>
#include <string>
#include <unordered_map>
NS_BEG2(top, data)
static const std::string AUDITX_PLUGIN = "auditxplugin";
class xplugin_manager {
public:
    xplugin_manager() = default;
    ~xplugin_manager() = default;

    void add(const std::string & plugin_name, std::shared_ptr<xplugin> plugin);
    void start();

    xplugin * get(const std::string & plugin_name);
    void remove(const std::string & plugin_name);

private:
    std::unordered_map<std::string, std::shared_ptr<xplugin>> m_xplugin_map;
};
using xplugin_manager_t = xplugin_manager;
NS_END2
