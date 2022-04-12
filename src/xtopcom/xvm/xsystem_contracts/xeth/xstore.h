#pragma once

#include <map>
#include <unordered_map>
#include "xeth_header.h"

NS_BEG4(top, xvm, system_contracts, xeth)
using namespace top::evm_common;

class block_header_with_difficulty_t {
public:
    block_header_with_difficulty_t() {};
public:
    xeth_block_header_t m_header;
    u256 m_difficult_sum;
};

class store {
public:
    store() {};
    bool saveMainChain(uint64_t chain_id, int64_t height, h256 hash);
    bool saveBlock(xeth_block_header_t header, u256 difficult_sum, uint64_t chain_id);
    bool isBlockExist(uint64_t chain_id, h256 hash);
    bool rebuildMainChain(uint64_t chain_id, xeth_block_header_t& current_header, xeth_block_header_t& new_header);
    bool getBlockbyHash(uint64_t chain_id, h256 hash, xeth_block_header_t& block, u256& difficult_sum);
    bool getCurrentHeightOfMainChain(uint64_t chain_id, int64_t& height);
    bool getHashOfMainChainByHeight(uint64_t chain_id, int64_t height, h256 & hash);

private:
    std::map<uint64_t, std::unordered_map<h256, block_header_with_difficulty_t>> m_chains;
    std::map<uint64_t, std::unordered_map<int64_t, h256>> m_main_chains;
    // std::map<uint64_t, std::string> m_current_block_hash;
    std::map<uint64_t, int64_t> m_current_block_height;
};

NS_END4