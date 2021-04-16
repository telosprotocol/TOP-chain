// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <functional>
#include <memory>
#include "xbase/xns_macro.h"
#include "xbase/xlog.h"

NS_BEG2(top, xrpc)
using std::string;
using std::unordered_map;
using std::unordered_set;
using std::vector;
using std::pair;
using std::unique_ptr;
using std::shared_ptr;
using namespace std::placeholders;
#define ENABLE_RPC_SESSION 0
#define ENABLE_RPC_TOKEN 0

#define xdbg_rpc(fmt, ...) xdbg("[rpc] " fmt, ##__VA_ARGS__)
#define xinfo_rpc(fmt, ...) xinfo("[rpc] " fmt, ##__VA_ARGS__)
#define xkinfo_rpc(fmt, ...) xkinfo("[rpc] " fmt, ##__VA_ARGS__)
#define xerror_rpc(fmt, ...) xerror("[rpc] " fmt, ##__VA_ARGS__)

#define DELETE(p) if(p) { delete p; p = nullptr;}

enum class enum_xrpc_type : uint8_t
{
    enum_xrpc_http_type = 0,
    enum_xrpc_ws_type,
    enum_xrpc_error_type
};
enum class enum_xrpc_tx_type : uint8_t
{
    enum_xrpc_query_type = 0,
    enum_xrpc_tx_type
};
NS_END2

namespace std
{
    template<>
    struct hash<pair<string, string>>
    {
        size_t operator()(const pair<string, string>& obj)const
        {
            return std::hash<std::string>()(obj.first + obj.second);
        }
    };

    template <>
    struct hash<top::xrpc::enum_xrpc_type> final
    {
        std::size_t operator()(const top::xrpc::enum_xrpc_type type) const noexcept {
            return static_cast<std::size_t>(type);
        }
    };
}
