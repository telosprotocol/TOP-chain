// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include "xpre_request_data.h"
#include "xrpc/xrpc_method.h"
#include "xrpc/xerror/xrpc_error.h"
#include "xrpc/xrpc_define.h"
NS_BEG2(top, xrpc)
using SimpleWeb::QueryString;
using std::vector;
using std::string;

vector<string> xpre_request_data_t::m_mandatory_fileds{RPC_VERSION, RPC_METHOD, RPC_SEQUENCE_ID};
vector<string> xpre_request_data_t::m_option_fileds{"target_account_addr"};


bool xpre_request_data_t::parse_request(const string &request_data) {
    m_request_map = std::move(QueryString::parse(request_data));
    for (auto iter = m_request_map.begin(); iter != m_request_map.end(); ++iter) {
        xinfo_rpc("key:%s,value:%s", iter->first.c_str(), iter->second.c_str());
    }
    return true;
}

void xpre_request_data_t::check_request() {
    for (const auto& field : m_mandatory_fileds) {
        const auto& field_value = get_request_value(field);
        if (field_value.empty()) {
            throw xrpc_error{enum_xrpc_error_code::rpc_param_param_lack, "miss param " + field + " or " + field + " is not valid"};
        }
        if (RPC_VERSION == field && RPC_DEFAULT_VERSION != field_value) {
            throw xrpc_error{enum_xrpc_error_code::rpc_param_param_error, "version must be 1.0 now"};
        }
    }
    const string& method = get_request_value(RPC_METHOD);
    for (const auto& field : m_option_fileds) {
        auto field_value = get_request_value(field);
        if (method != RPC_CREATE_ACCOUNT && method != RPC_ACCOUNT && field_value.empty()) {
            throw xrpc_error{enum_xrpc_error_code::rpc_param_param_lack, "miss param " + field};
        }
    }
}

std::string xpre_request_data_t::get_request_value(const string& key) const
{
    auto value_iter = m_request_map.find(key);
    if (value_iter != m_request_map.end()) {
        return value_iter->second;
    }
    return "";
}

std::string xpre_request_data_t::get_response()
{
    return m_writer.write(m_response_json);
}

NS_END2
