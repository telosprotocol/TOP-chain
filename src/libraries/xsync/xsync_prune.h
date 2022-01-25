// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <atomic>
#include <mutex>
#include <map>
#include <set>
#include "xcommon/xnode_type.h"

NS_BEG2(top, sync)

enum enum_height_type {
    start,   
    mutable_checkpoint_height, 
    latest_state_height,
    confirm_height,
    end,
};

class xsync_prune_t {
public:
    xsync_prune_t(){
    }
    void add(const std::string address, const std::set<enum_height_type> types);
    void del(const std::string address, const std::set<enum_height_type> types);
    bool update(const std::string address, const enum_height_type height_type, const uint64_t height);
    bool get_height(const std::string address, uint64_t &min_height);
    bool empty(const std::string address);

private:
    std::map<std::string, std::map<enum_height_type, uint64_t>> m_prune_accounts;
    std::map<std::string, uint64_t> m_min_height_accounts;
    std::mutex m_lock;
};

class xsync_prune_sigleton_t {
public:
    static xsync_prune_t& instance() {
        static xsync_prune_t instance;
        return instance;
    }
};

NS_END2
