// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xns_macro.h"
#include "xbasic/xmemory.hpp"
#include "xmbus/xmessage_bus.h"
#include "xvledger/xvblock.h"
#include "xvledger/xvaccount.h"

#include <string>
#include <atomic>
#include <mutex>

NS_BEG2(top, datastat)

class xdatastat_t {
    enum {
       max_zone_index = base::enum_chain_zone_edge_index + 1,
       tps_calc_interval_sencond = 5*60,
    };
 public:
    explicit xdatastat_t(const observer_ptr<mbus::xmessage_bus_face_t> &bus);
    ~xdatastat_t();

 private:
    void try_calc_tps(uint64_t end_time);
    void on_block_to_db_event(mbus::xevent_ptr_t e);
    void do_tableblock_stat(base::xvblock_t* block);
    void do_unitblock_stat(base::xvblock_t* block);
    void do_tx_stat_with_unit(base::xvblock_t* block);

 private:
    uint64_t                                m_begin_gmtime{0};
    uint64_t                                m_tx_count[max_zone_index];
    uint64_t                                m_unit_count[max_zone_index];
    uint64_t                                m_tableblock_count[max_zone_index];
    uint64_t                                m_begin_tx_count[max_zone_index];
    uint64_t                                m_begin_unit_count[max_zone_index];
    uint64_t                                m_begin_tableblock_count[max_zone_index];
    uint64_t                                m_max_tx_tps[max_zone_index];
    uint64_t                                m_max_unit_tps[max_zone_index];
    uint64_t                                m_max_tableblock_tps[max_zone_index];

    std::mutex                              m_lock;
    uint32_t                                m_bus_listen_id;
    observer_ptr<mbus::xmessage_bus_face_t> m_bus;
};

NS_END2
