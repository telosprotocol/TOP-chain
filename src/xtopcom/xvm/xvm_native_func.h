// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include <string>
#include <chrono>
#include <cstdint>
#include <functional>
#include "xbase/xns_macro.h"
#include "xvm_define.h"

NS_BEG2(top, xvm)
class xvm_context;
using std::placeholders::_1;
using native_handler = std::function<void(xvm_context*)>;
class xvm_native_func {
 public:
    xvm_native_func();
 public:
    unordered_map<std::string, native_handler> m_native_func_map;
};
NS_END2
