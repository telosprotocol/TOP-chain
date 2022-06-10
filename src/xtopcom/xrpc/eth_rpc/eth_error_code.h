// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#pragma once
#include <map>
#include "json/json.h"

namespace eth {
enum enum_error_code {
    enum_invalid_argument_hex,
    enum_invalid_address,
};
enum enum_eth_rpc_code {
    enum_eth_rpc_execution_reverted = -32000,
    enum_eth_rpc_invalid_request = -32600,
    enum_eth_rpc_method_not_find = -32601,
    enum_eth_rpc_invalid_params = -32602,
    enum_eth_rpc_internal_error = -32603,

    enum_eth_rpc_parse_error = -32700,
    enum_eth_rpc_invalid_address = -32801,
};
enum enum_rpc_check_type {
    enum_rpc_type_block,
    enum_rpc_type_address,
    enum_rpc_type_hash,
    enum_rpc_type_data,
    enum_rpc_type_topic,
    enum_rpc_type_unknown = 99,
};
struct ErrorMessage {
    ErrorMessage(int32_t code, const std::string& message):m_code(code), m_message(message) {}
    ErrorMessage():m_code(0) {}
    int32_t m_code;
    std::string m_message;
};
class EthErrorCode {
public:
    static void deal_error(xJson::Value & js_rsp, eth::enum_eth_rpc_code error_id, const std::string& msg);
    static bool check_hex(const std::string& account, xJson::Value & js_rsp, uint32_t index, const enum_rpc_check_type type);
    static bool check_eth_address(const std::string& account, xJson::Value & js_rsp);
    static bool check_hash(const std::string& hash, xJson::Value & js_rsp);
    static bool check_req(const xJson::Value & js_req, xJson::Value & js_rsp, const uint32_t number);
};
}
