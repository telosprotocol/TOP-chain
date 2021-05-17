// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include "xpre_request_handler_mgr.h"

#include <json/value.h>

#include "xrpc/xrpc_method.h"
#include "xrpc/prerequest/xpre_request_data.h"
#include "xrpc/prerequest/xpre_request_handler.h"
#include "xrpc/xerror/xrpc_error.h"
#include "xrpc/xerror/xrpc_error_code.h"

NS_BEG2(top, xrpc)
using std::string;

xpre_request_handler_mgr::xpre_request_handler_mgr()
{
    using pre_request_server = pre_request_service<xpre_request_token_handler>;
    m_pre_request_service.reset(new pre_request_server(new xpre_request_token_handler()));
    m_pre_request_service->init();
}
void xpre_request_handler_mgr::execute(xpre_request_data_t& request, const string& request_data)
{
    m_pre_request_service->parse_request(request, request_data);
    m_pre_request_service->check_request(request);
    m_pre_request_service->call_method(request);
    if (request.m_finish) {
        request.m_response_json[RPC_ERRNO] = RPC_OK_CODE;
        request.m_response_json[RPC_ERRMSG] = RPC_OK_MSG;
        request.m_response_json[RPC_SEQUENCE_ID] = request.get_request_value(RPC_SEQUENCE_ID);
    }

}
NS_END2
