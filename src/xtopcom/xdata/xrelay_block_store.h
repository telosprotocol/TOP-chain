// Copyright (c) 2018-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xevm_common/common_data.h"
#include "xdata/xrelay_block.h"
#include <iostream>
#include <limits>



namespace top {

namespace data {
    class xrelay_block_store {

    public:
        static bool get_all_poly_block_hash_list_from_cache(const xrelay_block& tx_block, std::map<uint64_t, evm_common::h256>& block_hash_map);
        static bool get_all_leaf_block_hash_list_from_cache(const xrelay_block& poly_block, std::vector<evm_common::h256>& leaf_hash_vector, bool include_self);
        static bool load_block_hash_from_db(uint64_t load_height, top::data::xrelay_block  &db_relay_block);
    };

}

}