// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xns_macro.h"
#include "xvledger/xvcnode.h"

NS_BEG3(top, tests, election)

class xtop_dummy_nodesvr : public base::xvnodesrv_t {
public:
    xtop_dummy_nodesvr() = default;
    xtop_dummy_nodesvr(xtop_dummy_nodesvr const &) = delete;
    xtop_dummy_nodesvr & operator=(xtop_dummy_nodesvr const &) = delete;
    xtop_dummy_nodesvr(xtop_dummy_nodesvr &&) = default;
    xtop_dummy_nodesvr & operator=(xtop_dummy_nodesvr &&) = default;
    ~xtop_dummy_nodesvr() override = default;

    base::xauto_ptr<base::xvnode_t> get_node(const xvip2_t & target_node) const override {
        return nullptr;
    }
    base::xauto_ptr<base::xvnodegroup_t> get_group(const xvip2_t & target_group) const override {
        return nullptr;
    }
    bool add_group(const base::xvnodegroup_t * group_ptr) override {
        return true;
    }
    bool remove_group(const xvip2_t & target_group) override {
        return true;
    }
};
using xdummy_nodesvr_t = xtop_dummy_nodesvr;

NS_END3
