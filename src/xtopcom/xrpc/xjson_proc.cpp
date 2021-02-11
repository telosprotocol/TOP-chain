// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include "xjson_proc.h"
#include <exception>
#include "xerror/xrpc_error.h"

NS_BEG2(top, xrpc)

void xjson_proc_t::parse_json(const xpre_request_data_t& pre_request_data) {
    try {
        if (!m_reader.parse(pre_request_data.get_request_value("body"), m_request_json)) {
            throw std::exception{};
        }
        m_request_json["version"] = pre_request_data.get_request_value("version");
        m_request_json["method"] = pre_request_data.get_request_value("method");
        m_request_json["sequence_id"] = pre_request_data.get_request_value("sequence_id");
        m_response_json["sequence_id"] = pre_request_data.get_request_value("sequence_id");
    }
    catch (const std::exception &e) {
        xkinfo_rpc("parse_json exception:%s,%s", e.what(), pre_request_data.get_request_value("body").c_str());
        throw xrpc_error{enum_xrpc_error_code::rpc_param_json_parser_error, "body json parse error"};
    }
}

void xjson_proc_t::parse_json(const string& content) {
    try {
        if (!m_reader.parse(content, m_request_json)) {
            throw std::exception{};
        }
    }
    catch (const std::exception &e) {
        xkinfo_rpc("parse_json exception:%s,%s", e.what(), content.c_str());
        throw xrpc_error{enum_xrpc_error_code::rpc_param_json_parser_error, "json parse error"};
    }
}


std::string xjson_proc_t::get_response()
{
    return m_writer.write(m_response_json);
}

std::string xjson_proc_t::get_request()
{
    return m_writer.write(m_request_json);
}


NS_END2
