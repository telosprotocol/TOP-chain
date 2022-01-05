// Copyright (c) 2017-2019 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xgossip/include/header_block_data.h"

#include "assert.h"

namespace top {

namespace gossip {

HeaderBlockDataCache::HeaderBlockDataCache() {
}

HeaderBlockDataCache::~HeaderBlockDataCache() {
}

void HeaderBlockDataCache::AddData(const std::string & header_hash, const std::string & block) {
    assert(!HasData(header_hash));
    block_cache_.put(header_hash, std::make_pair(block, std::chrono::steady_clock::now()));
    return;
}

void HeaderBlockDataCache::GetData(const std::string & header_hash, std::string & block) {
    std::pair<std::string, std::chrono::steady_clock::time_point> res{};
    block_cache_.get(header_hash, res);
    block = res.first;
    return;
}

bool HeaderBlockDataCache::HasData(const std::string & header_hash) {
    std::pair<std::string, std::chrono::steady_clock::time_point> res{};
    return block_cache_.get(header_hash, res);
}

}  // namespace gossip

}  // namespace top
