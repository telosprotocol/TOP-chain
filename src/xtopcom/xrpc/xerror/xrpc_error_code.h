// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include <stdint.h>
#include <system_error>
#include <type_traits>

#include "xbase/xns_macro.h"

NS_BEG2(top, xrpc)

enum class enum_xrpc_error_code {
    rpc_param_json_parser_error = -32700,
    rpc_param_param_lack = -32602,
    rpc_param_param_error = 1,
    rpc_param_unkown_error = 2,
    rpc_shard_exec_error = 3,
    rpc_param_signature_error,
};

const uint32_t          RPC_OK_CODE         = 0;
constexpr const char*   RPC_OK_MSG          = "OK";
const uint32_t          RPC_ERROR_CODE      = 97;
constexpr const char*   RPC_ERROR_MSG       = "unknown error";
const uint32_t          RPC_EXCEPTION_CODE  = 98;
// const uint32_t          RPC_TIMEOUT_CODE    = 99;
constexpr const char*   RPC_TIMEOUT_MSG     = "request time out";
constexpr const char*   RPC_ERRNO           = "errno";
constexpr const char*   RPC_ERRMSG          = "errmsg";

std::error_code make_error_code(const enum_xrpc_error_code errc) noexcept;

NS_END2

NS_BEG1(std)

template <>
struct is_error_code_enum<top::xrpc::enum_xrpc_error_code> : std::true_type {};

NS_END1
