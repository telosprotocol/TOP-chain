// Copyright (c) 2017-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xBFT/xconsaccount.h"
#include "xunit_service/xcons_face.h"
#include "xmbus/xmessage_bus.h"

NS_BEG2(top, xunit_service)

class xtimer_picker_t : public xconsensus::xcsaccount_t {
protected:
    virtual ~xtimer_picker_t();

public:
    xtimer_picker_t(base::xcontext_t &                               _context,
                    const int32_t                                    target_thread_id,
                    std::shared_ptr<xcons_service_para_face> const & para,
                    std::shared_ptr<xblock_maker_face> const &       block_maker);

public:
    bool set_fade_xip_addr(const xvip2_t & new_addr);

protected:
    bool on_view_fire(const base::xvevent_t & event, xconsensus::xcsobject_t * from_parent, const int32_t cur_thread_id, const uint64_t timenow_ms) override;
    bool on_proposal_finish(const base::xvevent_t & event, xcsobject_t * from_child, const int32_t cur_thread_id, const uint64_t timenow_ms) override;
    bool on_pdu_event_up(const base::xvevent_t & event, xcsobject_t * from_child, const int32_t cur_thread_id, const uint64_t timenow_ms) override;
    bool on_create_block_event(const base::xvevent_t & event,xcsobject_t* from_parent,const int32_t cur_thread_id,const uint64_t timenow_ms) override;
    bool on_time_cert_event(const base::xvevent_t & event,xcsobject_t* from_parent,const int32_t cur_thread_id,const uint64_t timenow_ms) override;
    bool send_out(const xvip2_t & from_addr, const xvip2_t & to_addr, const base::xcspdu_t & packet, int32_t cur_thread_id, uint64_t timenow_ms);
    bool recv_in(const xvip2_t & from_addr, const xvip2_t & to_addr, const base::xcspdu_t & packet, int32_t cur_thread_id, uint64_t timenow_ms);

    virtual int  verify_proposal(base::xvblock_t * proposal_block,base::xvqcert_t * bind_clock_cert,xcsobject_t * _from_child) override;

protected:
    uint64_t                                 m_cur_view{};
    uint64_t                                 m_latest_cert_clock{};
    std::shared_ptr<xcons_service_para_face> m_params{};
    observer_ptr<xleader_election_face>      m_leader_selector{};
    std::shared_ptr<xblock_maker_face>       m_block_maker{};
    mbus::xmessage_bus_face_t               *m_bus{};    

    // fade xip. fade version should not make new proposal
    xvip2_t                                  m_faded_xip2{};
};

NS_END2
