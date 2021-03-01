// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include "xvm/xcontract/xcontract_register.h"

NS_BEG3(top, xvm, xcontract)
class hello_param : public xcontract_base {
    using xbase_t = xcontract_base;
 public:
    XDECLARE_DELETED_COPY_DEFAULTED_MOVE_SEMANTICS(hello_param);
    XDECLARE_DEFAULTED_OVERRIDE_DESTRUCTOR(hello_param);
    explicit
    hello_param(common::xnetwork_id_t const & network_id = common::xtopchain_network_id);

    xcontract_base* clone() override {return new hello_param(network_id());}
    void hi();
    void hi2(int32_t i);
    void hi3(int32_t i, std::string& str);
    void hi4(int32_t i, std::string& str, std::map<std::string,std::string> _map);
    BEGIN_CONTRACT_WITH_PARAM(hello_param)
    CONTRACT_FUNCTION_PARAM(hello_param, hi);
    CONTRACT_FUNCTION_PARAM(hello_param, hi2);
    CONTRACT_FUNCTION_PARAM(hello_param, hi3);
    CONTRACT_FUNCTION_PARAM(hello_param, hi4);
    END_CONTRACT_WITH_PARAM
};


NS_END3
