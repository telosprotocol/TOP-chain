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


    class xrelay_block_store {

        public:
        xrelay_block_store();
        static xrelay_block_store &get_instance()
        {
            static xrelay_block_store m_instance_store;
            return m_instance_store;
        }

        public:
        
        top::evm_common::h256   compute_merkle_root(std::vector<top::evm_common::h256> hash_vector);
        void                    save_block_ordinal_block_hash(uint64_t block_ordinal, top::evm_common::h256 block_hash);
        void                    save_block_data(top::evm_common::h256 block_hash,xrelay_block block);
        void                    save_chain_height(uint64_t block_height);
        void                    save_block_hash(uint64_t height, top::evm_common::h256 header_hash);
        void                    save_block_merkle_tree(top::evm_common::h256 block_hash, xPartialMerkleTree tree);

        bool                    verify_path(top::evm_common::h256  rootHash, xMerklePath path ,  top::evm_common::h256 item);
        void                    xpartial_merklize(std::vector<top::evm_common::h256> arr_in, 
                                                  top::evm_common::h256  &hash_out, std::vector<xMerklePath> &path_out);
        top::evm_common::h256   reconstruct_merkle_tree_node(uint64_t index, uint64_t level, uint64_t counter, uint64_t tree_size,
                                                     std::unordered_map<std::string, top::evm_common::h256> &tree_nodes);
        top::evm_common::h256   compute_root_from_path(xMerklePath path, top::evm_common::h256 item_hash);
        top::evm_common::h256   compute_root_from_path_and_item(xMerklePath path, xMerklePathItem item );
     

    public:
        top::evm_common::h256       get_block_ordinal_block_hash(uint64_t block_ordinal);
        uint64_t                    get_chain_height();
        top::evm_common::h256       get_block_hash(uint64_t height);
        xrelay_block                get_block_data(top::evm_common::h256 block_hash);
        xMerklePath                 get_block_proof(top::evm_common::h256 block_hash, top::evm_common::h256 head_block_hash);
        xPartialMerkleTree          get_block_merkle_tree_from_ordinal(uint64_t block_ordinal);
        top::evm_common::h256       get_last_block_hash();
        xPartialMerkleTree          get_block_merkle_tree(top::evm_common::h256 block_hash);
        top::evm_common::h256       get_merkle_tree_node(uint64_t index, uint64_t level, uint64_t counter, uint64_t tree_size,
                                                        std::unordered_map<std::string, top::evm_common::h256> &tree_nodes);

        private:

            //cache withe block . block hash -> xrelay_block
            basic::xlru_cache<top::evm_common::h256, xrelay_block>  m_blocks_map{1000};

            // block -> block_merkle_tree
            basic::xlru_cache<top::evm_common::h256, xPartialMerkleTree> m_block_merkle_tree_map{1000};
            //  height -> block hash
             basic::xlru_cache<uint64_t,  top::evm_common::h256> m_block_height_to_hash_map{1000};
            //save ordinal -> block hash
            basic::xlru_cache<uint64_t, top::evm_common::h256> m_block_ordinal_to_hash_map{1000};

            //save block height
            uint64_t    m_block_height;
    };

}

}