// Copyright (c) 2018-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include "xbase/xns_macro.h"

#include <memory>
#include <string>

NS_BEG2(top, xtxpool_v2)
class xtx_entry;
class xtxpool_table_t;
NS_END2

NS_BEG2(top, data)
class xplugin {
public:
    xplugin() = default;
    xplugin(xplugin const &) = default;
    xplugin & operator=(xplugin const &) = default;
    xplugin(xplugin &&) = default;
    xplugin & operator=(xplugin &&) = default;
    virtual ~xplugin() = default;
    // load dynamic library
    virtual bool load();
    // The specific implementation of the plugin may be in another language, and if the plugin needs to maintain its running state,
    // it should be started in the plugin manager, and of course, the 'run' function can also be placed in the 'load' function
    virtual void run();
    // Freeing memory is necessary. When implementing plugins in other languages,
    // memory control in those languages must be handled by the languages themselves.
    // Therefore, a function must be provided to free the memory of the plugin itself
    virtual void free();
    // To send data to a plugin
    virtual bool send();
    // Async send data to a plugin
    virtual bool async_send(const std::shared_ptr<top::xtxpool_v2::xtx_entry> & tx, const std::shared_ptr<top::xtxpool_v2::xtxpool_table_t> & table);
    void set_plugin_name(std::string plugin_name);
    std::string plugin_name();

private:
    std::string m_plugin_name;
};

NS_END2
