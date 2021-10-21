// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xns_macro.h"

#include <memory>
NS_BEG2(top, common)

class xtop_user_option_t {
public:
};
using xuser_option_t = xtop_user_option_t;
using xuser_option_ptr_t = std::shared_ptr<xtop_user_option_t>;

NS_END2