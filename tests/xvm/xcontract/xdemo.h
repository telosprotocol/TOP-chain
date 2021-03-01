// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include "xvm/xcontract/xcontract_register.h"

NS_BEG3(top, xvm, xcontract)
class hello : public xcontract_base {
    using xbase_t = xcontract_base;
 public:
    XDECLARE_DELETED_COPY_DEFAULTED_MOVE_SEMANTICS(hello);
    XDECLARE_DEFAULTED_OVERRIDE_DESTRUCTOR(hello);

    explicit
    hello(common::xnetwork_id_t const & network_id = common::xtopchain_network_id);

    xcontract_base* clone() override {return new hello(network_id());}
    void hi();
    void hi2();

    BEGIN_CONTRACT
    CONTRACT_FUNCTION(hi);
    CONTRACT_FUNCTION(hi2);
    END_CONTRACT
};


NS_END3
