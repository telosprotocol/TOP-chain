// Copyright (c) 2017-2019 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xlru_cache_specialize.h"
#include "xpbase/base/top_utils.h"

namespace top {

namespace base {
class TimerRepeated;
}

namespace gossip {

class HeaderBlockDataCache {
public:
    HeaderBlockDataCache();
    ~HeaderBlockDataCache();
    void AddData(const std::string & header_hash, const std::string & block);
    void GetData(const std::string & header_hash, std::string & block);
    bool HasData(const std::string & header_hash);

private:
    static const uint32_t kMaxBlockCacheSize = 1000u;
    basic::xlru_cache_specialize<std::string, std::pair<std::string, std::chrono::steady_clock::time_point>> block_cache_{kMaxBlockCacheSize};

    DISALLOW_COPY_AND_ASSIGN(HeaderBlockDataCache);
};

}  // namespace gossip

}  // namespace top
