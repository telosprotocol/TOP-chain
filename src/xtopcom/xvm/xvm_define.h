// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include <string>
#include <memory>
#include <vector>
#include <unordered_map>
#include <deque>
#include <chrono>
#include <ctime>
#include "xbase/xns_macro.h"
#include "xbase/xlog.h"

NS_BEG2(top, xvm)
using	std::string;
using	std::shared_ptr;
using	std::vector;
using	std::unordered_map;
using	std::deque;
using   std::make_shared;

//const uint8_t ARG_TYPE_INT64    = 0;
const uint8_t ARG_TYPE_UINT64   = 1;
const uint8_t ARG_TYPE_STRING   = 2;
const uint8_t ARG_TYPE_BOOL     = 3;


#define xinfo_lua(fmt, ...)     xinfo("[lua] "  fmt , ##__VA_ARGS__)
#define xkinfo_lua(fmt, ...)    xkinfo("[lua] "  fmt , ##__VA_ARGS__)
#define xwarn_lua(fmt, ...)     xwarn("[lua] "  fmt , ##__VA_ARGS__)
#define xerror_lua(fmt, ...)    xerror("[lua] "  fmt , ##__VA_ARGS__)

#define xinfo_vm(fmt, ...)      xinfo("[vm] "  fmt , ##__VA_ARGS__)
#define xkinfo_vm(fmt, ...)     xkinfo("[vm] "  fmt , ##__VA_ARGS__)
#define xwarn_vm(fmt, ...)      xwarn("[vm] "  fmt , ##__VA_ARGS__)
#define xerror_vm(fmt, ...)     xerror("[vm] "  fmt , ##__VA_ARGS__)

NS_END2
