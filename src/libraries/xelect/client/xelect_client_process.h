// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.#pragma once

#pragma once

#include "xchain_timer/xchain_timer_face.h"
#include "xcommon/xip.h"
#include "xdata/xelection/xelection_result_store.h"
#include "xdata/xunit_bstate.h"
#include "xmbus/xbase_sync_event_monitor.hpp"

NS_BEG2(top, elect)

using elect_update_handler2 = std::function<void(data::election::xelection_result_store_t const &, common::xzone_id_t const &, std::uint64_t const, bool starup_flag)>;

class xelect_client_process : public mbus::xbase_sync_event_monitor_t {
public:
    xelect_client_process(common::xnetwork_id_t const & network_id,
                          observer_ptr<mbus::xmessage_bus_face_t> const & mb,
                          elect_update_handler2 cb2,
                          observer_ptr<time::xchain_time_face_t> const & xchain_timer);

protected:
    bool filter_event(const mbus::xevent_ptr_t & e) override;
    void process_event(const mbus::xevent_ptr_t & e) override;

protected:
    void process_timer(const mbus::xevent_ptr_t & e);
    // void process_elect(const mbus::xevent_ptr_t & e);
    void update_election_status(common::xlogic_time_t current_time);
    void process_election_block(data::xunitstate_ptr_t const& unitstate, common::xlogic_time_t const current_time);
    void process_election_contract(common::xaccount_address_t const & contract_address, common::xlogic_time_t const current_time, common::xlogic_time_t const update_interval);
    uint64_t get_new_election_height(data::xunitstate_ptr_t const& unitstate) const;

private:
    common::xnetwork_id_t m_network_id;
    elect_update_handler2 m_update_handler2{};
    observer_ptr<time::xchain_time_face_t> m_xchain_timer{nullptr};

    struct xtop_internal_election_status {
        uint64_t height{ 0 };
        common::xlogic_time_t last_update_time{ 0 };
    };
    using xinternal_election_status_t = xtop_internal_election_status;

    std::unordered_map<common::xaccount_address_t, xinternal_election_status_t> m_election_status;
};
NS_END2
