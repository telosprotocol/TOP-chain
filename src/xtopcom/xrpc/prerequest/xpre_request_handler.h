// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include <string>
#include <memory>

#include "xpre_request_data.h"
#include "xrpc/xrpc_method.h"
#include "xpre_request_handler_face.h"
#include "xbase/xns_macro.h"

namespace top {
namespace xrpc {
class xpre_request_data_t;
}  // namespace xrpc
}  // namespace top

NS_BEG2(top, xrpc)


class xpre_request_token_handler : public pre_request_face<xpre_request_token_handler> {
public:
    explicit xpre_request_token_handler();
    void requestToken(xpre_request_data_t & request);
    // void create_account(xpre_request_data_t & request);
    // void account(xpre_request_data_t & request);
};

NS_END2
