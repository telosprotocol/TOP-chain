// Copyright (c) 2018-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xplugin/xplugin_manager.h"

#include "xbasic/xmemory.hpp"
#include "xplugin/xplugin.h"
#include "xplugin_audit/xaudit_plugin.h"

using namespace std;
using namespace top::data;
NS_BEG2(top, data)

void xplugin_manager::start() {
    std::shared_ptr<xaudit_pligin> plugin(&xaudit_pligin::instance());
    add(data::AUDITX_PLUGIN, plugin);
}

void xplugin_manager::add(const std::string & plugin_name, std::shared_ptr<xplugin> plugin) {
    auto it = m_xplugin_map.find(plugin_name);
    if (it != m_xplugin_map.end()) {
        xdbg("xplugin_manager::add audit-tx, %s exist already ", plugin_name.c_str());
        return;
    }
    if (true == plugin->load()) {
        plugin->run();
        m_xplugin_map[plugin_name] = std::move(plugin);
    }
}

void xplugin_manager::remove(const string & plugin_name) {
    auto it = m_xplugin_map.find(plugin_name);
    if (it != m_xplugin_map.end()) {
        xdbg("xplugin_manager::remove audit-tx ");
        get(plugin_name)->free();
        m_xplugin_map.erase(plugin_name);
    }
}

xplugin * xplugin_manager::get(const string & plugin_name) {
    auto it = m_xplugin_map.find(plugin_name);
    if (it == m_xplugin_map.end()) {
        return nullptr;
    }
    return it->second.get();
}

// template <typename T>
// T * xplugin_manager::plugin_instance(const string & plugin_name) {
//     return dynamic_cast<T *>(xplugin_manager::instance().get(plugin_name));
// }

NS_END2