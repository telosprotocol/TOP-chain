// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include <string>

#include "xrpc_define.h"
#include "xbase/xns_macro.h"

NS_BEG2(top, xrpc)
class xtraffic_controller {
public:
    bool ip_flow_controll() { return true; }
    bool account_flow_controll() { return true; }
};
NS_END2
