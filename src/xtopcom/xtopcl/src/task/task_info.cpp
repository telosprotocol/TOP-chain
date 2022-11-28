// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "base/utility.h"
#include "xchain_fork/xutility.h"
#include "xtopcl/include/task/task_info.h"

namespace xChainSDK {

    uint64_t get_timestamp() {
        return utility::gmttime_ms() / 1000;
    }

    task_info::task_info(){
        use_transaction = false;
        trans_action = top::data::xtx_factory::create_tx(top::data::xtransaction_version_2);
    }
}
