#include "xevm_common/xeth/xeth_store.h"

NS_BEG3(top, evm_common, eth)

bool store::saveMainChain(uint64_t chain_id, int64_t height, h256 hash) {
    auto chain_x = m_main_chains.find(chain_id);
    if (chain_x == m_main_chains.end()) {
      std::unordered_map<int64_t, h256> chain_y;
      chain_y[height]=hash;
      m_main_chains[chain_id] = chain_y;
    } else {
        chain_x->second[height] = hash;
    }

    m_current_block_height[chain_id] = height;
    
    return true;
}

bool store::saveBlock(xeth_block_header_t header, u256 difficult_sum, uint64_t chain_id) {
    block_header_with_difficulty_t headerWithDifficulty;
    headerWithDifficulty.m_header = header;
    headerWithDifficulty.m_difficult_sum = difficult_sum;
    auto chain_x = m_chains.find(chain_id);
    if (chain_x == m_chains.end()) {
        std::unordered_map<h256, block_header_with_difficulty_t> chain_y;
        chain_y[header.hash()] = headerWithDifficulty;
        m_chains[chain_id] = chain_y;
    } else {
        chain_x->second[header.hash()] = headerWithDifficulty;
    }
    return true;
}

bool store::isBlockExist(uint64_t chain_id, h256 hash) {
    auto chain_x = m_chains.find(chain_id);
    if (chain_x == m_chains.end()) {
        return false;
    }

    if (chain_x->second.find(hash) == chain_x->second.end()) {
        return false;
    }

    return true;
}

bool store::rebuildMainChain(uint64_t chain_id, xeth_block_header_t& current_header, xeth_block_header_t& new_header) {
    int64_t current_height = current_header.number();
    int64_t new_height = new_header.number();
    int64_t max_height = current_height;
    std::unordered_map<int64_t, h256> need_remove;
    std::unordered_map<int64_t, h256> need_add;
    auto chain_x = m_main_chains.find(chain_id);
    if (chain_x == m_main_chains.end()) {
        return false;
    }
    
    h256 new_hash = new_header.hash();
    h256 current_hash = current_header.hash();

    if (current_height > new_height) {
        for (int64_t height = current_height; height > new_height; height--) {
            auto chain_y = chain_x->second.find(height);
            if (chain_y == chain_x->second.end()) {
                return false;
            }
            need_remove[height] = chain_y->second;
        }

        if (!getHashOfMainChainByHeight(chain_id, new_height, current_hash)) {
            return false;
        }

        current_height--;
    } 

    while (new_height > current_height) {
        block_header_with_difficulty_t block;
        u256 difficult_sum;
        bool result = getBlockbyHash(chain_id, new_hash, block.m_header, difficult_sum);
        if (!result) {
            return false;
        }

        need_add[new_height] = new_hash;
        new_hash = block.m_header.parentHash();
        if (block.m_header.number() != new_height) {
            return false;
        }
        new_height--;
    }

    while (new_hash != current_hash) {
        block_header_with_difficulty_t block;
        bool result = getBlockbyHash(chain_id, new_hash, block.m_header, block.m_difficult_sum);
        if (!result) {
            return false;
        }

        need_add[new_height] = new_hash;

        new_hash = block.m_header.parentHash();
        if (!getHashOfMainChainByHeight(chain_id, new_height, current_hash)) {
            return false;
        }
        need_remove[new_height] = current_hash;
        new_height--;
    }

    for (auto it = need_remove.begin(); it != need_remove.end(); it++) {
        chain_x->second.erase(it->first);
    }

    for (auto it = need_add.begin(); it != need_add.end(); it++) {
        chain_x->second[it->first]= it->second;
    }

    m_current_block_height[chain_id] = new_header.number();

    return true;
}

bool store::getBlockbyHash(uint64_t chain_id, h256 hash, xeth_block_header_t& block, u256 &difficult_sum) {
    auto chain_x = m_chains.find(chain_id);
    if (chain_x == m_chains.end()) {
        return false;
    }

    if (chain_x->second.find(hash) == chain_x->second.end()) {
        return false;
    }

    block = chain_x->second[hash].m_header;
    difficult_sum = chain_x->second[hash].m_difficult_sum;

    return true;
}

bool store::getCurrentHeightOfMainChain(uint64_t chain_id, int64_t& height) {
    auto chain_x = m_current_block_height.find(chain_id);
    if (chain_x == m_current_block_height.end()){
        return false;
    }

    height = chain_x->second;
    return true;
}

bool store::getHashOfMainChainByHeight(uint64_t chain_id, int64_t height, h256& hash) {
    auto chain_x = m_main_chains.find(chain_id);
    if (chain_x == m_main_chains.end()) {
        return false;
    }

    if (chain_x->second.find(height) == chain_x->second.end()) {
        return false;
    }

    hash = chain_x->second[height];
    return true;        
}

NS_END3