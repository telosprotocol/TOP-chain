// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <time.h>
#include <memory>
#include "xvnetwork/xaddress.h"
#include "xbasic/xlru_cache.h"

NS_BEG2(top, sync)

struct xsession_t {
    uint64_t start{};
    uint32_t count{};

    xsession_t() {
        start = ::time(nullptr);
        count = 1;
    }

    bool plus(uint32_t max_count, uint64_t time_interval) {
        uint64_t end = ::time(nullptr);
        if(end - start <= time_interval) {
            if(count < max_count) {
                ++count;
                return true;
            } else {
                return false;
            }
        }

        // if new round started
        start = ::time(nullptr);
        count = 1;
        return true;
    }
};

class xsession_manager_t {
public:
    xsession_manager_t(uint32_t size) {
        m_session.count = 0;
        cache.set_max_size(size);
    }
    virtual ~xsession_manager_t() {}

    bool plus(const vnetwork::xvnode_address_t& addr, uint32_t total_max_count, uint32_t max_count, uint64_t time_interval) {
        std::unique_lock<std::mutex> lock(m_lock);
        xsession_t* session{};
        if(cache.get(addr.account_address(), session)) {
            return m_session.plus(total_max_count, time_interval) && session->plus(max_count, time_interval);
        } else {
            session = new xsession_t{};
            cache.put(addr.account_address(), session);
            return m_session.plus(total_max_count, time_interval);
        }
    }

protected:
    std::mutex m_lock;
    xsession_t m_session; // for total
    basic::xlru_cache<common::xaccount_address_t, xsession_t*, std::default_delete<xsession_t>> cache{10000};
};

NS_END2
