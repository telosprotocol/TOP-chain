// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include <string>
#include <memory>
#include <mutex>

#include "xpre_request_data.h"
#include "xpre_request_handler.h"
#include "xbase/xns_macro.h"
#include "xrpc/prerequest/xpre_request_handler_server.h"
#include "xrpc/xrpc_define.h"

namespace top {
namespace xrpc {
class xpre_request_data_t;
}  // namespace xrpc
}  // namespace top


NS_BEG2(top, xrpc)
class xpre_request_handler_mgr
{
    public:
    xpre_request_handler_mgr();
    void execute(xpre_request_data_t& request, const std::string& request_data);
    public:
    unique_ptr<pre_request_service<>>    m_pre_request_service;

};
NS_END2
