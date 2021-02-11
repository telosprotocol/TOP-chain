// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include "xrpc_error_json.h"

NS_BEG2(top, xrpc)
using std::string;
xrpc_error_json::xrpc_error_json(const int32_t error_code, const string& message, const string& sequence_id)
{
    m_json["errno"] = error_code;
    m_json["sequence_id"] = sequence_id;
    m_json["errmsg"] = message;
}

std::string xrpc_error_json::write()
{
    return m_writer.write(m_json);
}

NS_END2
