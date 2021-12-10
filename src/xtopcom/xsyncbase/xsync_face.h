// Copyright (c) 2017-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <unordered_map>
#include "xbase/xns_macro.h"

NS_BEG2(top, sync)

class xsync_face_t {
public:
    virtual std::string help() const = 0;
    virtual std::string status() const = 0;
    virtual std::map<std::string, std::vector<std::string>> get_neighbors() const = 0;
    virtual std::string auto_prune_data(const std::string& prune) const = 0;
};

NS_END2
