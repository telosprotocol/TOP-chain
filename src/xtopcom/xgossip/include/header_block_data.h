// Copyright (c) 2017-2019 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <unordered_map>

#include "xpbase/base/top_utils.h"

namespace top {

namespace base {
class TimerRepeated;
}

namespace gossip {

class HeaderBlockData {
public:
    HeaderBlockData() {}
    ~HeaderBlockData() {}
    virtual void AddData(const std::string& header_hash, const std::string& block) = 0;
    virtual void GetData(const std::string& header_hash, std::string& block) = 0;
    virtual bool HasData(const std::string& header_hash) = 0;
    virtual void RemoveData(const std::string& header_hash) = 0;

private:

    DISALLOW_COPY_AND_ASSIGN(HeaderBlockData);
};


class HeaderBlockDataCache : public HeaderBlockData {
public:
    HeaderBlockDataCache();
    ~HeaderBlockDataCache();
    void AddData(const std::string& header_hash, const std::string& block) override;
    void GetData(const std::string& header_hash, std::string& block) override;
    bool HasData(const std::string& header_hash) override;
    void RemoveData(const std::string& header_hash) override;

protected:
    void CheckRemoveData();

private:
    std::mutex block_map_mutex_;
    std::unordered_map<std::string, std::string> block_cache_map_;
    uint64_t block_cache_size_{0};
    std::shared_ptr<base::TimerRepeated> remove_data_timer_{nullptr};
    std::mutex block_header_map_mutex_;
    std::unordered_map<std::string, std::chrono::steady_clock::time_point> block_header_cache_map_;

    DISALLOW_COPY_AND_ASSIGN(HeaderBlockDataCache);
};


}  // namespace gossip

}  // namespace top
