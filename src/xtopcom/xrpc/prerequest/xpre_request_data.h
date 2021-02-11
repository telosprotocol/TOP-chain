// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#ifndef ASIO_STANDALONE
#define ASIO_STANDALONE
#endif
#ifndef USE_STANDALONE_ASIO
#define USE_STANDALONE_ASIO
#endif
#include <string>
#include <memory>
#include <vector>
#include "xbasic/xns_macro.h"
#include "json/json.h"
#include "simplewebserver/utility.hpp"

NS_BEG2(top, xrpc)
using request_map_t = SimpleWeb::CaseInsensitiveMultimap;

class xpre_request_data_t
{
public:
    bool parse_request(const std::string& request_data);
    void check_request();
    std::string get_request_value(const std::string& key) const;
    std::string get_response();
public:
    request_map_t               m_request_map{};
    bool                        m_finish{false};
    xJson::FastWriter           m_writer;
    xJson::Value                m_response_json;
    static std::vector<std::string>  m_mandatory_fileds;
    static std::vector<std::string>  m_option_fileds;
};

NS_END2
