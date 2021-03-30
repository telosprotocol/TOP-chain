// Copyright (c) 2017-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include <string>
#include <map>
#include "xBFT/xconsevent.h"
#include "xbasic/xns_macro.h"

NS_BEG2(top, xunit_service)

// default block service entry
class xcons_unorder_cache final {
 public:
    xcons_unorder_cache() = default;
    ~xcons_unorder_cache();

 public:
    bool                        filter_event(uint64_t account_viewid, const xvip2_t &from_addr, const xvip2_t &to_addr, const base::xcspdu_t &packet);
    void                        on_view_fire(uint64_t account_viewid);
    xconsensus::xcspdu_fire*    get_proposal_event(uint64_t account_viewid);
    size_t                      get_unoder_cache_size() const {return m_unorder_events.size();}

 private:
    void                        set_unorder_event(const xvip2_t &from_addr, const xvip2_t &to_addr, const base::xcspdu_t &packet);
    void                        clear_old_unorder_event(uint64_t account_viewid);

 private:
    static constexpr int                             unorder_pdu_max_view_num{1};  // now only cache one viewid unorder pdu
    std::map<uint64_t, xconsensus::xcspdu_fire*>     m_unorder_events;
};

NS_END2
