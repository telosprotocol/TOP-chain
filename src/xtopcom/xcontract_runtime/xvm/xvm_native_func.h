// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xns_macro.h"

#include <string>
#include <functional>

NS_BEG3(top, contract_runtime, vm)
class xvm_context;

using native_handler = std::function<void(xvm_context*)>;
class xvm_native_func {
 public:
    xvm_native_func();
 public:
    std::unordered_map<std::string, native_handler> m_native_func_map;
};
NS_END3
