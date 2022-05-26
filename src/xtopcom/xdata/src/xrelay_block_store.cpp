// Copyright (c) 2018-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xdata/xrelay_block.h"
#include "xbase/xns_macro.h"
#include "xpbase/base/top_utils.h"
#include "xbase/xutl.h"
#include "trezor-crypto/sha3.h"
#include <secp256k1/secp256k1.h>
#include <secp256k1/secp256k1_recovery.h>
#include "xevm_common/xtriehash.h"
#include "xevm_common/rlp.h"
#include "xvledger/xmerkle.hpp"
#include "xcrypto/xckey.h"
#include "xcrypto/xcrypto_util.h"
#include "xevm_common/xtriecommon.h"
#include "../xtopcom/xdepends/include/trezor-crypto/ed25519-donna/ed25519.h"
#include <openssl/sha.h>
#include "xdata/xrelay_block_store.h"

NS_BEG2(top, data)

using namespace top::evm_common;

//min 2^ > v
int min2Log(int v)
{
    float f = (float)(v - 1);  
    return 1 << ((*(unsigned int*)(&f) >> 23) - 126);
}


uint64_t xrelay_block_store::get_chain_height()
{
    return m_block_height;
}

void xrelay_block_store::save_chain_height(uint64_t height)
{
    if (m_block_height <= height) {
        m_block_height = height;
    }
}

top::evm_common::h256 xrelay_block_store::get_last_block_hash(){
    top::evm_common::h256 block_hash;
    if (m_block_height_to_hash_map.get(m_block_height, block_hash)) {
        return block_hash;
    }
    return h256{0};       
}

xPartialMerkleTree  xrelay_block_store::get_block_merkle_tree(top::evm_common::h256 block_hash)
{
    xPartialMerkleTree merkle_tree;
    if (m_block_merkle_tree_map.get(block_hash, merkle_tree)) {
        return merkle_tree;
    }
    return {};
}

void xrelay_block_store::save_block_merkle_tree(top::evm_common::h256 block_hash, xPartialMerkleTree merkle_tree)
{
    m_block_merkle_tree_map.put(block_hash, merkle_tree);
}

    void xrelay_block_store::save_block_hash(uint64_t height, top::evm_common::h256 header_hash)
{
    m_block_height_to_hash_map.put(height, header_hash);
}

    top::evm_common::h256 xrelay_block_store::get_block_hash(uint64_t height )
{
    top::evm_common::h256 block_hash;
    if (m_block_height_to_hash_map.get(height, block_hash)) {
        return block_hash;
    }
    return h256{0};
}

void  xrelay_block_store::save_block_data(top::evm_common::h256 block_hash,xrelay_block block)
{
    m_blocks_map.put(block_hash, block);
}

xrelay_block  xrelay_block_store::get_block_data(top::evm_common::h256 block_hash)
{
    xrelay_block block;
    if(m_blocks_map.get(block_hash, block)) {
        return block;
    }
    return {};
}

void  xrelay_block_store::save_block_ordinal_block_hash(uint64_t block_ordinal, top::evm_common::h256 block_hash )
{
    m_block_ordinal_to_hash_map.put(block_ordinal, block_hash);
}

top::evm_common::h256  xrelay_block_store::get_block_ordinal_block_hash(uint64_t block_ordinal)
{
    h256 block_hash;
    if(m_block_ordinal_to_hash_map.get(block_ordinal, block_hash)) {
        return block_hash;
    }
    return h256{0};
}
    
xPartialMerkleTree xrelay_block_store::get_block_merkle_tree_from_ordinal(uint64_t block_ordinal)
{
    h256 block_hash;
    if (m_block_ordinal_to_hash_map.get(block_ordinal, block_hash)) {
            xPartialMerkleTree merkle_tree;
            if (m_block_merkle_tree_map.get(block_hash, merkle_tree)) {
                return merkle_tree;
            }
    }
    return {};
}

top::evm_common::h256 xrelay_block_store::get_merkle_tree_node(
    uint64_t index,
    uint64_t level,
    uint64_t counter,
    uint64_t tree_size,
    std::unordered_map<std::string, top::evm_common::h256> &tree_nodes)
{
    h64 h64_index = h64(index);
    h64 h64_level = h64( level);
    h128 h128_index;
    top::evm_common::h256 hash_result;
    for (size_t i = 0; i < 8; i++) {
        h128_index[i] = h64_index[i];
        h128_index[i + 8] = h64_level[i];
    }

    std::string hash_index = h128_index.hex();

    auto iter  = tree_nodes.find(hash_index);
    if (iter != tree_nodes.end()) {
        return iter->second;
    }

    if (level == 0) {
        if (index >= tree_size) {
            hash_result = h256 { 0 };
        } else {
            h256 block_hash;
            if (m_block_ordinal_to_hash_map.get(index, block_hash)) {
                // error
                assert(0);
            }else {
                hash_result = block_hash;
            }
            tree_nodes.insert({hash_index, hash_result});
        }
    } else {

        auto cur_tree_size = (index + 1) * counter;
        if (cur_tree_size > tree_size) {
            if (index * counter <= tree_size) {
                auto left_hash = get_merkle_tree_node(index * 2,
                    level - 1,
                    counter / 2,
                    tree_size,
                    tree_nodes);
                auto right_hash = reconstruct_merkle_tree_node(
                    index * 2 + 1,
                    level - 1,
                    counter / 2,
                    tree_size,
                    tree_nodes);
                hash_result = combine_hash(left_hash, right_hash);
            } else {
                hash_result = h256 { 0 };
            }
        } else {
            auto tree = get_block_merkle_tree_from_ordinal(cur_tree_size);
            if (tree.size() != 0) {
                hash_result = tree.get_path().back();
            } else {
                hash_result = h256 { 0 };
            }
        }

        tree_nodes.insert({hash_index, hash_result});
    }
    return hash_result;
}

top::evm_common::h256 xrelay_block_store::reconstruct_merkle_tree_node(
    uint64_t index,
    uint64_t level,
    uint64_t counter,
    uint64_t tree_size,
    std::unordered_map<std::string, top::evm_common::h256> &tree_nodes)
{
    h64 h64_index (index);
    h64 h64_level (level);
    h128 h128_index;
    top::evm_common::h256 hash_result;
    for (size_t i = 0; i < 8; i++) {
        h128_index[i] = h64_index[i];
        h128_index[i + 8] = h64_index[i];
    }

    std::string hash_index = h128_index.hex();

    auto iter =  tree_nodes.find(hash_index);
    if (iter != tree_nodes.end()) {
        return iter->second;
    }

    if (level == 0) {
        if (index >= tree_size) {
            hash_result = h256 { 0 };
        } else {
            if (!m_block_ordinal_to_hash_map.get(index, hash_result)) {
                assert(0);
            }
            
            tree_nodes.insert({hash_index, hash_result});
            return hash_result;
        }
    } else {
        auto left_hash = get_merkle_tree_node(index * 2,
            level - 1,
            counter / 2,
            tree_size,
            tree_nodes);

        auto right_hash = reconstruct_merkle_tree_node(index * 2 + 1,
            level - 1,
            counter / 2,
            tree_size,
            tree_nodes);

        hash_result = combine_hash(left_hash, right_hash);
        tree_nodes.insert({hash_index, hash_result});
    }
    return hash_result;
}

xMerklePath xrelay_block_store::get_block_proof(top::evm_common::h256 block_hash, top::evm_common::h256 head_block_hash)
{
    uint64_t leaf_index = 0;
    uint64_t tree_size = 0;
    xPartialMerkleTree block_tree{};
    xPartialMerkleTree head_block_tree{};

    
    m_block_merkle_tree_map.get(block_hash, block_tree);
    m_block_merkle_tree_map.get(head_block_hash, head_block_tree);

    leaf_index = block_tree.size();
    tree_size = head_block_tree.size();

    std::cout <<  "block_hash  " << block_hash  <<  "   head_block_hash" <<  head_block_hash  <<std::endl;
    std::cout <<  "leaf_index  " << leaf_index <<  "   tree_size" <<  tree_size  <<std::endl;
    if (leaf_index >= tree_size) {
        if (block_hash == head_block_hash) {
            return xMerklePath {};
        }
        // error
        assert(0);
        return xMerklePath {};
    }

    uint64_t level = 0;
    uint64_t counter = 1;
    uint64_t cur_index = leaf_index;
    std::vector<xMerklePathItem> path {};
    std::unordered_map<std::string, top::evm_common::h256> tree_nodes;
    uint64_t iter = tree_size;
    
    while (iter > 1) {
        h256 h256_result { 0 };
        if ((cur_index % 2) == 0) {
            cur_index += 1;
        } else {
            cur_index -= 1;
        }
        uint8_t direction = 0;
        if ((cur_index % 2) != 0) {
            direction = 1;
        }

        if (cur_index % 2 == 1) {
            h256_result = reconstruct_merkle_tree_node(cur_index, level, counter, tree_size, tree_nodes);
        } else {
            h256_result = get_merkle_tree_node(cur_index, level, counter, tree_size, tree_nodes);
        }

        path.push_back(xMerklePathItem{h256_result, direction});
        cur_index /= 2;
        iter = (iter + 1) / 2;
        level += 1;
        counter *= 2;
    }
    return path;
}

top::evm_common::h256 xrelay_block_store::compute_root_from_path(xMerklePath path, top::evm_common::h256 item_hash)
{
    auto res = item_hash;
    for (auto item :  path) {
        if (item.m_direction == 0) {
            res = combine_hash(item.m_hash, res);
        } else {
            res = combine_hash(res, item.m_hash);
        }
    }
    return res;
}

top::evm_common::h256  xrelay_block_store::compute_root_from_path_and_item(xMerklePath path, xMerklePathItem item )
{
    return compute_root_from_path(path, item.m_hash);
}

/// Verify merkle path for given item and corresponding path.
bool xrelay_block_store::verify_path(top::evm_common::h256  rootHash, xMerklePath path ,  top::evm_common::h256 item) {
    return (rootHash == compute_root_from_path(path,  item));
}

void xrelay_block_store::xpartial_merklize(std::vector<top::evm_common::h256> arr_in, top::evm_common::h256  &hash_out, std::vector<xMerklePath>& path_out) 
{
    if (arr_in.size() == 0)
    {
        hash_out = h256{0};
        path_out = std::vector<xMerklePath>{};
        return ;
    }
    auto len = min2Log(arr_in.size());
    if (len == 1) {
        hash_out = arr_in[0];
        path_out = std::vector<xMerklePath>{};
        return ;
    }
    
    int arr_len = arr_in.size();
    for (int i = 0; i < arr_len; i++) {
        if ((i%2) == 0) {
            if (i+1 < arr_len) {
                xMerklePath tmpPath;
                xMerklePathItem tmpPathItem{ arr_in[(i + 1)], 1};
                tmpPath.push_back(tmpPathItem);
                path_out.push_back(tmpPath);
            }else {
                path_out.push_back({});
            }
        } else {
            xMerklePath tmpPath;
            xMerklePathItem tmpPathItem{ arr_in[(i - 1)], 0};
            tmpPath.push_back(tmpPathItem);
            path_out.push_back(tmpPath);
        }
    }

    int counter = 1;
    while( len > 1) {
        len /= 2;
        counter *= 2;

        for (int i = 0; i < len; i++) {
            top::evm_common::h256  h256_hash;
            if (2*i >= arr_len) {
                continue;
            }else if((( 2*i) + 1) >= arr_len) {
                h256_hash = arr_in[2*i];
            } else {
                h256_hash =  combine_hash(arr_in[2 * i], arr_in[2 * i + 1]);
            }
            arr_in[i] = h256_hash;

            if (len > 1) {
                if ((i%2) == 0) {
                    for (int j = 0; j < counter; j++) {
                        auto index =  ((i + 1) * counter + j);
                        if (index < (int)arr_in.size()) {
                            path_out[index].push_back(xMerklePathItem { h256_hash, 0 });
                        }
                    }
                } else {
                    for (int j = 0; j < counter; j++) {
                        auto index =  ((i - 1) * counter + j);
                        if (index < (int)arr_in.size()) {
                            path_out[index].push_back(xMerklePathItem { h256_hash, 1 });
                        }
                    }
                }
            }
        }
        
        arr_len = (arr_len + 1) / 2;
        hash_out = arr_in[0];
    }

}

top::evm_common::h256  xrelay_block_store::compute_merkle_root(std::vector<top::evm_common::h256> hash_vector) {

    top::evm_common::h256 h256_result;
    if (hash_vector.size() == 0) {
        return h256{0};
    } else if (hash_vector.size() == 1) {
        return hash_vector[0];
    } else {
        auto len = hash_vector.size();
        auto subtree_len = (min2Log(len)/2);

        std::vector<top::evm_common::h256> left_vec(hash_vector.begin(), hash_vector.begin()+subtree_len);
        auto left_root = compute_merkle_root(left_vec);
        std::vector<top::evm_common::h256> right_vec(hash_vector.begin() + subtree_len, hash_vector.end());
        auto right_root = compute_merkle_root(right_vec);
        h256_result = combine_hash(left_root, right_root);
    }
    return h256_result;
}


NS_END2