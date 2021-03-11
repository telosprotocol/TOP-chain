// Copyright (c) 2017-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include <string>
#include <vector>
#include <map>
#include "xBFT/xconsaccount.h"
#include "xbasic/xobject_ptr.h"
#include "xunit_service/xcons_face.h"
#include "xunit_service/xcons_unorder_cache.h"
#include "xmbus/xmessage_bus.h"
#include "xtxpool/xtxpool_face.h"

NS_BEG2(top, xunit_service)
using xconsensus::xcsaccount_t;
// default block service entry
class xbatch_packer : public xcsaccount_t {
public:
    explicit xbatch_packer(observer_ptr<mbus::xmessage_bus_face_t> const   &mb,
                           uint16_t                                        tableid,
                           const std::string &                             account_id,
                           std::shared_ptr<xcons_service_para_face> const &para,
                           std::shared_ptr<xblock_maker_face> const &      block_maker,
                           base::xcontext_t &                              _context,
                           uint32_t                                        target_thread_id);
    virtual ~xbatch_packer();

public:
    virtual uint16_t get_tableid();

    // recv_in packet from this object to child layers
    virtual bool recv_in(const xvip2_t &from_addr, const xvip2_t &to_addr, const base::xcspdu_t &packet, int32_t cur_thread_id, uint64_t timenow_ms);

    // send packet by network
    virtual bool send_out(const xvip2_t &from_addr, const xvip2_t &to_addr, const base::xcspdu_t &packet, int32_t cur_thread_id, uint64_t timenow_ms);

    // return specific error code(enum_xconsensus_result_code) to let caller know reason
    virtual int  verify_proposal(base::xvblock_t * proposal_block, base::xvqcert_t * bind_clock_cert, xcsobject_t * _from_child) override;

    virtual bool reset_xip_addr(const xvip2_t & new_addr);
    virtual bool on_proposal_finish(const base::xvevent_t & event, xcsobject_t* from_child, const int32_t cur_thread_id, const uint64_t timenow_ms);
    virtual bool on_consensus_commit(const base::xvevent_t & event, xcsobject_t* from_child, const int32_t cur_thread_id, const uint64_t timenow_ms);

protected:
    virtual bool on_view_fire(const base::xvevent_t &event, xcsobject_t *from_parent, const int32_t cur_thread_id, const uint64_t timenow_ms);

    // note: to return false may call parent'push_event_up,or stop further routing when return true
    virtual bool    on_pdu_event_up(const base::xvevent_t & event, xcsobject_t* from_child, const int32_t cur_thread_id, const uint64_t timenow_ms);
    // backup receive proposal
    // virtual bool on_proposal_start(const base::xvevent_t & event, xcsobject_t* from_parent, const int32_t cur_thread_id, const uint64_t timenow_ms);

protected:
    base::xvqcert_t* get_bind_drand(base::xvblock_t *proposal_block);
    xvip2_t get_parent_xip(const xvip2_t & local_xip);
    xvip2_t get_child_xip(const xvip2_t & local_xip, const std::string & account);
    void    set_xip(xblock_maker_para_t & blockpara, const xvip2_t & leader);
    void    invoke_sync(const std::string & account, const std::string & reason);

private:
    observer_ptr<mbus::xmessage_bus_face_t>  m_mbus;
    uint16_t                                 m_tableid;
    volatile uint64_t                        m_last_view_id;
    std::shared_ptr<xcons_service_para_face> m_para;
    std::shared_ptr<xblock_maker_face>       m_block_maker;
    std::shared_ptr<xproposal_maker_face>    m_proposal_maker;
    uint64_t                                 m_cons_start_time_ms;
    xcons_unorder_cache                      m_unorder_cache;
    static constexpr uint32_t                m_empty_block_max_num{2};
    observer_ptr<xtxpool::xtxpool_table_face_t> m_txpool_table{nullptr};
    std::string                                 m_latest_cert_block_hash;
    bool                                        m_can_make_empty_block{false};
};

using xbatch_packer_ptr_t = xobject_ptr_t<xbatch_packer>;
using xbatch_packers = std::vector<xbatch_packer_ptr_t>;
// batch_packer map for <tableid, packer>
using xbatch_paker_map = std::map<uint16_t, xbatch_packer_ptr_t>;

NS_END2
