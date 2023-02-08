// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xdata/xfulltableblock_account_data.h"
#include "xdata/xfull_tableblock.h"
#include "xvledger/xvcnode.h"

NS_BEG3(top, vnode, components)

class xtop_fulltableblock_process {
public:
    xtop_fulltableblock_process() = delete;
    xtop_fulltableblock_process(xtop_fulltableblock_process const&) = delete;
    xtop_fulltableblock_process& operator=(xtop_fulltableblock_process const&) = delete;
    xtop_fulltableblock_process(xtop_fulltableblock_process&&) = delete;
    xtop_fulltableblock_process& operator=(xtop_fulltableblock_process&&) = delete;
    ~xtop_fulltableblock_process() = delete;

    static data::xfulltableblock_statistic_accounts fulltableblock_statistic_accounts(data::xstatistics_data_t const& block_statistic_data, base::xvnodesrv_t * node_service);
    static data::xfulltableblock_statistic_accounts fulltableblock_statistic_cons_accounts(data::xstatistics_cons_data_t const& block_statistic_data, base::xvnodesrv_t* node_service);
};

using xfulltableblock_process_t = xtop_fulltableblock_process;
NS_END3
