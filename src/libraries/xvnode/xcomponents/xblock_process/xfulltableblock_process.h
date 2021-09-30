// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xvnode/xcomponents/xblock_process/xblock_process_face.h"

#include "xdata/xfulltableblock_account_data.h"
#include "xdata/xfull_tableblock.h"
#include "xvledger/xvcnode.h"
#include "xvledger/xvledger.h"

NS_BEG3(top, vnode, components)

class xtop_fulltableblock_process: public xtop_block_process_face {
public:
    xtop_fulltableblock_process() =  default;
    ~xtop_fulltableblock_process() =  default;

protected:
    xtop_fulltableblock_process(xtop_fulltableblock_process const&) = delete;
    xtop_fulltableblock_process& operator=(xtop_fulltableblock_process const&) = delete;
    xtop_fulltableblock_process(xtop_fulltableblock_process&&) = delete;
    xtop_fulltableblock_process& operator=(xtop_fulltableblock_process&&) = delete;


public:
    virtual xvblock_class_t block_class(xobject_ptr_t<base::xvblock_t> const & vblock) override;
    virtual xvblock_level_t block_level(xobject_ptr_t<base::xvblock_t> const& vblock) override;
    virtual void process_block(xobject_ptr_t<base::xvblock_t> const & vblock) override;

    static data::xfulltableblock_statistic_accounts fulltableblock_statistic_accounts(data::xstatistics_data_t const& block_statistic_data, base::xvnodesrv_t * node_service);
};

using xfulltableblock_process_t = xtop_fulltableblock_process;
NS_END3