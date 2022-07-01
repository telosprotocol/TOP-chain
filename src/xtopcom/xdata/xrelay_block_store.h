// Copyright (c) 2018-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xevm_common/common_data.h"
#include "xdata/xpartial_merkle_tree.h"
#include "xdata/xrelay_block.h"
#include <iostream>
#include <limits>
#include <fstream>
#include "xbasic/xlru_cache.h"


namespace top {

namespace data {

    enum enum_block_cache_type {
        cache_tx_block,
        cache_poly_tx_block,
        cache_poly_election_block,
        cache_error_block,
    };

    struct xrelay_block_save_leaf {
        xrelay_block_save_leaf() = default;
        enum_block_cache_type   m_type;
        //uint64_t                m_chain_id;
       // evm_common::h256        m_merkle_root_hash;    //election key 
        evm_common::h256        m_block_hash;
    };

    class xrelay_block_store {

    public:
        xrelay_block_store():m_tx_block_map(100) {};
        static xrelay_block_store &get_instance()
        {
            static xrelay_block_store m_instance_store;
            return m_instance_store;
        }

    public:
        enum_block_cache_type   check_block_type(const xrelay_block &next_block);
        bool    set_block_merkle_root_from_store(xrelay_block &next_block);
        bool    get_all_poly_block_hash_list_from_cache(const xrelay_block &tx_block, std::vector<evm_common::h256> &leaf_hash_vector);
        bool    get_all_leaf_block_hash_list_from_cache(const xrelay_block &poly_block, std::vector<evm_common::h256> &leaf_hash_vector, bool include_self);
        bool    save_block_hash_to_store_cache(xrelay_block &next_block);
        bool    load_block_hash_from_cache(uint64_t load_height, xrelay_block_save_leaf &block_leaf);
        void    clear_cache();

    protected:
        bool    save_tx_block_hash_to_tx_map(xrelay_block &next_block, enum_block_cache_type block_typ);

    private:
        bool    check_tx_block_validity(const xrelay_block &next_block);
        bool    check_poly_block_validity(const xrelay_block &next_block);
       
    private:
         basic::xlru_cache<uint64_t, xrelay_block_save_leaf> m_tx_block_map;  
          
    };

}

}