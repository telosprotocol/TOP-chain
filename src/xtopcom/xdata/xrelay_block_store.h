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

    struct xrelay_block_save_leaf {
        xrelay_block_save_leaf() = default;
        enum_block_cache_type m_type;
        evm_common::h256 m_block_hash;
    };

    class xrelay_block_store {

    public:
        xrelay_block_store();
        static xrelay_block_store& get_instance()
        {
            static xrelay_block_store m_instance_store;
            return m_instance_store;
        }

    public:
        bool get_all_poly_block_hash_list_from_cache(const xrelay_block& tx_block, std::map<uint64_t, evm_common::h256>& block_hash_map);
        bool get_all_leaf_block_hash_list_from_cache(const xrelay_block& poly_block, std::vector<evm_common::h256>& leaf_hash_vector, bool include_self);
        bool load_block_hash_from_db(uint64_t load_height, xrelay_block_save_leaf& block_leaf);
    };

}

}