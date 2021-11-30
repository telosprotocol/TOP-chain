// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include <string>
#include "xbase/xns_macro.h"

NS_BEG4(top, vnode, components, prune_data)

class xprune_data {
public:
    xprune_data(xprune_data const &) = delete;
    xprune_data & operator=(xprune_data const &) = delete;
    xprune_data(xprune_data &&) = default;
    xprune_data & operator=(xprune_data &&) = default;
    ~xprune_data() = default;
    xprune_data() {}

    bool  update_prune_config_file(std::string& prune_enable);
};

using xprune_data_t = xprune_data;

NS_END4