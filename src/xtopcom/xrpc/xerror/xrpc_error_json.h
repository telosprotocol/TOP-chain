// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include <string>
#include "xbasic/xns_macro.h"
#include "json/json.h"
NS_BEG2(top, xrpc)

class xrpc_error_json
{
public:
    xrpc_error_json(const int32_t error_code, const std::string& message, const std::string& sequence_id);
    std::string write();

public:
    xJson::FastWriter       m_writer;
    xJson::Value            m_json;
};

NS_END2
