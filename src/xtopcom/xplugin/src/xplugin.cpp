// Copyright (c) 2018-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include "xplugin/xplugin.h"

NS_BEG2(top, data)
void xplugin::set_plugin_name(std::string plugin_name) {
    m_plugin_name = plugin_name;
};
std::string xplugin::plugin_name() {
    return m_plugin_name;
}

bool xplugin::load() {
    return true;
}
void xplugin::run() {
    return;
}
void xplugin::free() {
    return;
}

bool xplugin::async_send(const std::shared_ptr<top::xtxpool_v2::xtx_entry> & tx, const std::shared_ptr<top::xtxpool_v2::xtxpool_table_t> & table) {
    return false;
}

bool xplugin::send() {
    return false;
}

NS_END2