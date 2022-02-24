// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include "xbase/xns_macro.h"
#include <system_error>
NS_BEG2(top, xvm)
const std::error_category& xvm_get_category();
NS_END2
