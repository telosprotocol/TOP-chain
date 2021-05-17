// Copyright (c) 2017-2019 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xgossip/include/header_block_data.h"

#include <functional>
#include <type_traits>
#include <utility>
#include <vector>

#include "xpbase/base/top_log.h"
#include "xpbase/base/top_timer.h"
#include "xpbase/base/top_timer2.h"

namespace top {

namespace gossip {

static const uint64_t BlockCacheMaxSize = 50 * 1024 * 1024;  // 50MB
static const uint32_t kExpirePeriod = 2 * 60 * 1000; // expire  after 2 * 60 s
static const uint64_t kCheckRemovePeriod = 10ll * 1000ll * 1000ll;   //5 seconds

// implementation of cache

HeaderBlockDataCache::HeaderBlockDataCache()
        : HeaderBlockData(),
        block_map_mutex_(),
        block_cache_map_(),
        block_header_map_mutex_(),
        block_header_cache_map_() {

    remove_data_timer_ = std::make_shared<base::TimerRepeated>(base::TimerManager::Instance(), "HeaderBlockDataCache::CheckRemoveData");
    remove_data_timer_->Start(
            500ll * 1000ll,
            kCheckRemovePeriod,
            std::bind(&HeaderBlockDataCache::CheckRemoveData, this));

}

HeaderBlockDataCache::~HeaderBlockDataCache() {
    remove_data_timer_->Join();
    remove_data_timer_ = nullptr;
}

void HeaderBlockDataCache::AddData(const std::string& header_hash, const std::string& block) {
    {
        std::unique_lock<std::mutex> lock(block_map_mutex_);
        block_cache_size_ += block.size();
        block_cache_map_[header_hash] = block;
        if (block_cache_size_ > BlockCacheMaxSize) {
            // TODO(smaug) maybe not very safe
            TOP_WARN("block_cache_map_ beyond max_size:%d, do clear", block_cache_map_.size());
            block_cache_map_.clear();
            block_cache_size_ = 0;
        }
        //TOP_DEBUG("Cache add header hash: %s", HexEncode(header_hash).c_str());
    }

    {
        std::unique_lock<std::mutex> lock_header(block_header_map_mutex_);
        block_header_cache_map_[header_hash] = std::chrono::steady_clock::now();
    }
}

void HeaderBlockDataCache::GetData(const std::string& header_hash, std::string& block) {
    std::unique_lock<std::mutex> lock(block_map_mutex_);
    auto ifind = block_cache_map_.find(header_hash);
    if (ifind != block_cache_map_.end()) {
        block = ifind->second;
    }
}

bool HeaderBlockDataCache::HasData(const std::string& header_hash) {
    {
        std::unique_lock<std::mutex> lock_header(block_header_map_mutex_);
        auto ifind = block_header_cache_map_.find(header_hash);
        if (ifind != block_header_cache_map_.end()) {
            //TOP_DEBUG("Cache has header hash: %s", HexEncode(header_hash).c_str());
            return true;
        }
        //TOP_DEBUG("Cache not has header hash: %s", HexEncode(header_hash).c_str());
        return false;
    }
}


void HeaderBlockDataCache::RemoveData(const std::string& header_hash) {
    std::unique_lock<std::mutex> lock(block_map_mutex_);
    auto ifind = block_cache_map_.find(header_hash);
    if (ifind != block_cache_map_.end()) {
        block_cache_map_.erase(ifind);
    }
    //TOP_DEBUG("Cache clear header hash: %s", HexEncode(header_hash).c_str());
}


void HeaderBlockDataCache::CheckRemoveData() {
    std::vector<std::string> expire_vec;
    {
        std::unique_lock<std::mutex> lock_header(block_header_map_mutex_);
        auto now = std::chrono::steady_clock::now();
        for (auto it = block_header_cache_map_.begin(); it != block_header_cache_map_.end(); ) {
            if (it->second + std::chrono::milliseconds(kExpirePeriod) < now) {
                // expire
                TOP_DEBUG("removeheader header_hash:%s", HexEncode(it->first).c_str());
                expire_vec.push_back(it->first);
                it = block_header_cache_map_.erase(it);
            } else {
                ++ it;
            }
        }
    }
    for (auto& item2 : expire_vec) {
        RemoveData(item2);
    }
}


// end implementation of HeaderBlockDataCache


}  // namespace gossip

}  // namespace top
