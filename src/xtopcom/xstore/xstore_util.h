// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.#pragma once

#pragma once

#include "xmetrics/xmetrics.h"

class xstore_util {
public:
    static void metirc_key_value(std::string const& key, std::string const& value, bool add_or_minus = true);
private:
    static bool endwith(std::string const& str, std::string const& suffix);
};