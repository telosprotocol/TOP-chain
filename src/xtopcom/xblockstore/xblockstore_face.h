// Copyright (c) 2018-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xvledger/xvdbstore.h"
#include "xvledger/xvblockstore.h"
#include "xvledger/xvaccount.h"
#include "xbasic/xmodule_type.h"

namespace top
{
    namespace store
    {
        enum enum_blockstore_metrics_type
        {
            enum_blockstore_metrics_type_block_index        = 1,
            enum_blockstore_metrics_type_block_object       = 2,
            enum_blockstore_metrics_type_block_input_res    = 3,
            enum_blockstore_metrics_type_block_output_res   = 4,
        };
    
        base::xvblockstore_t*  get_vblockstore();
        base::xvblockstore_t*  create_vblockstore(base::xvdbstore_t* xvdb_ptr);
    
        bool                   install_block_recycler(base::xvdbstore_t* xvdb_ptr);
        bool                   enable_block_recycler(bool enable);//off as default
        bool                   refresh_block_recycler_rule(const chainbase::enum_xmodule_type mod_id, const base::xvaccount_t & account_obj, const uint64_t permit_prune_upper_boundary);
        bool                   watch_block_recycler(const chainbase::enum_xmodule_type mod_id, const base::xvaccount_t & account_obj);
        bool                   unwatch_block_recycler(const chainbase::enum_xmodule_type mod_id, const base::xvaccount_t & account_obj);
    };//end of namespace of xledger
};//end of namespace of top
