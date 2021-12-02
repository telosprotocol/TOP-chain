// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "base/utility.h"
#include "xchain_fork/xchain_upgrade_center.h"
#include "task/task_info.h"

namespace xChainSDK {

    uint64_t get_timestamp() {
        return utility::gmttime_ms() / 1000;
    }

    task_info::task_info(){
        use_transaction = false;

        auto fire_time = get_timestamp();
        top::chain_fork::xtop_chain_fork_config_center::init();
        if (top::chain_fork::xtop_chain_fork_config_center::is_tx_forked_by_timestamp(fire_time)) {
            trans_action = top::data::xtx_factory::create_tx(top::data::xtransaction_version_2);
        } else {
            trans_action = top::data::xtx_factory::create_tx(top::data::xtransaction_version_1);
        }
    }
}
