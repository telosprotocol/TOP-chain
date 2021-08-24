// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

// TODO(jimmy) #include "xbase/xvledger.h"
#include "xstake/xstake_algorithm.h"

NS_BEG2(top, store)

class xtgas_singleton {
 private:
    xtgas_singleton() = default;
    enum {
        enum_property_height_behind_max = 1000,
    };

 public:
    static xtgas_singleton& get_instance() {
        static xtgas_singleton singleton;
        return singleton;
    }

    uint64_t get_cache_total_lock_tgas_token();
    bool    leader_get_total_lock_tgas_token(uint64_t timer_height, uint64_t & total_lock_tgas_token, uint64_t & property_height);
    bool    backup_get_total_lock_tgas_token(uint64_t timer_height, uint64_t property_height, uint64_t & total_lock_tgas_token);

 private:
    bool    get_latest_property(std::string & value, uint64_t & height);

    uint64_t m_last_update_time{0};
    uint64_t m_last_total_lock_tgas_token{0};
    uint64_t m_last_property_height{0};
    static constexpr uint16_t m_tgas_update_interval{10 * 6};  // total_tgas gets updated every 10 mins
    std::mutex m_mtx;
};

NS_END2
