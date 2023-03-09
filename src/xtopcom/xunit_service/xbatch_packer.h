// Copyright (c) 2017-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include <string>
#include <vector>
#include <map>
#include "xBFT/xconsaccount.h"
#include "xbase/xobject_ptr.h"
#include "xunit_service/xcons_face.h"
#include "xmbus/xmessage_bus.h"
#include "xtxpool_v2/xtxpool_face.h"
#include "xbase/xtimer.h"

NS_BEG2(top, xunit_service)
using xconsensus::xcsaccount_t;

class xpack_strategy_t {
public:
    void clear();
    uint32_t get_tx_num_threshold_first_time(uint64_t vc_time_ms);
    int32_t get_timer_interval() const;
    uint32_t get_tx_num_threshold(uint64_t cur_time) const;
private:
    uint64_t m_vc_time_ms{0};
};

// default block service entry
class xbatch_packer : public xcsaccount_t, public base::xtimersink_t {
public:
    explicit xbatch_packer(base::xtable_index_t &                          tableid,
                           const std::string &                             account_id,
                           std::shared_ptr<xcons_service_para_face> const &para,
                           std::shared_ptr<xblock_maker_face> const &      block_maker,
                           base::xcontext_t &                              _context,
                           uint32_t                                        target_thread_id);
    virtual ~xbatch_packer();

public:
    virtual bool close(bool force_async = true) override;  // must call close before release object,otherwise object never be cleanup
    virtual bool on_object_close() override;
    virtual bool on_timer_fire(const int32_t thread_id, const int64_t timer_id, const int64_t current_time_ms, const int32_t start_timeout_ms, int32_t & in_out_cur_interval_ms) override;

    virtual bool on_timer_start(const int32_t errorcode, const int32_t thread_id, const int64_t timer_id, const int64_t cur_time_ms, const int32_t timeout_ms, const int32_t timer_repeat_ms) override;

    virtual bool on_timer_stop(const int32_t errorcode, const int32_t thread_id, const int64_t timer_id, const int64_t cur_time_ms, const int32_t timeout_ms, const int32_t timer_repeat_ms) override;

    virtual base::xtable_index_t get_tableid();

    // recv_in packet from this object to child layers
    virtual bool recv_in(const xvip2_t &from_addr, const xvip2_t &to_addr, const base::xcspdu_t &packet, int32_t cur_thread_id, uint64_t timenow_ms);

    // send packet by network
    virtual bool send_out(const xvip2_t &from_addr, const xvip2_t &to_addr, const base::xcspdu_t &packet, int32_t cur_thread_id, uint64_t timenow_ms);

    // return specific error code(enum_xconsensus_result_code) to let caller know reason
    virtual int  verify_proposal(base::xvblock_t * proposal_block, base::xvqcert_t * bind_clock_cert, xcsobject_t * _from_child) override;

    virtual bool reset_xip_addr(const xvip2_t & new_addr);
    virtual bool set_fade_xip_addr(const xvip2_t & new_addr);
    virtual bool on_proposal_finish(const base::xvevent_t & event, xcsobject_t* from_child, const int32_t cur_thread_id, const uint64_t timenow_ms);
    virtual bool on_replicate_finish(const base::xvevent_t & event,xcsobject_t* from_child,const int32_t cur_thread_id,const uint64_t timenow_ms);
    virtual bool on_consensus_commit(const base::xvevent_t & event, xcsobject_t* from_child, const int32_t cur_thread_id, const uint64_t timenow_ms);
    virtual bool set_start_time(const common::xlogic_time_t& start_time);
protected:
    virtual bool on_view_fire(const base::xvevent_t &event, xcsobject_t *from_parent, const int32_t cur_thread_id, const uint64_t timenow_ms);

    // note: to return false may call parent'push_event_up,or stop further routing when return true
    virtual bool    on_pdu_event_up(const base::xvevent_t & event, xcsobject_t* from_child, const int32_t cur_thread_id, const uint64_t timenow_ms);
    // backup receive proposal
    // virtual bool on_proposal_start(const base::xvevent_t & event, xcsobject_t* from_parent, const int32_t cur_thread_id, const uint64_t timenow_ms);

protected:
    xvip2_t get_parent_xip(const xvip2_t & local_xip);
    xvip2_t get_child_xip(const xvip2_t & local_xip, const std::string & account);
    void set_xip(data::xblock_consensus_para_t & blockpara, const xvip2_t & leader);
    void    invoke_sync(const std::string & account, const std::string & reason);
    xresources_face * get_resources();
    std::shared_ptr<xproposal_maker_face> get_proposal_maker() const;
    bool    connect_to_checkpoint();

private:
    bool    start_proposal(uint32_t min_tx_num);
    bool    verify_proposal_viewid(const xvip2_t & from_addr, const xvip2_t & local_addr, uint64_t viewid);
    void    check_latest_cert_block(base::xvblock_t* _cert_block, const xconsensus::xcsview_fire* viewfire, std::error_code & ec);
    void    reset_leader_info();
    void    make_receipts_and_send(data::xblock_t * commit_block, data::xblock_t * cert_block);
    virtual uint32_t calculate_min_tx_num(bool first_packing, uint64_t time_ms);
    virtual int32_t set_vote_extend_data(base::xvblock_t * proposal_block, const uint256_t & hash, bool is_leader);
    virtual void clear_for_new_view();
    virtual void send_receipts(base::xvblock_t *vblock);
    virtual bool set_election_round(bool is_leader, data::xblock_consensus_para_t & proposal_para);
    bool    check_state_sync(base::xvblock_t * latest_committed_block);
    bool    do_state_sync(uint64_t sync_height);

    void send_preproposal(const data::xblock_consensus_para_t & cs_para,
                          const std::vector<data::xcons_transaction_ptr_t> & txs,
                          const std::vector<base::xvproperty_prove_ptr_t> & receiptid_state_proves);
    virtual xunit_service::xpreproposal_send_cb get_preproposal_send_cb();
    virtual bool process_msg(const xvip2_t & from_addr, const xvip2_t & to_addr, const base::xcspdu_t & packet, int32_t cur_thread_id, uint64_t timenow_ms);
    virtual int veriry_proposal_by_preproposal_block(base::xvblock_t * proposal_block);

private:
    base::xtable_index_t                     m_tableid;
    volatile uint64_t                        m_last_view_id;
    std::shared_ptr<xcons_service_para_face> m_para;
    std::shared_ptr<xproposal_maker_face>    m_proposal_maker;
    common::xlogic_time_t                    m_start_time;
    base::xtimer_t*                          m_raw_timer{nullptr};
    // m_is_leader to decide if timer need to do packing units and then start consensus
    bool                                     m_is_leader{false};
    data::xblock_consensus_para_ptr_t        m_leader_cs_para{nullptr};
    // m_leader_packed is used to avoid more than one block produced in one viewid
    bool                                     m_leader_packed{false};
    uint64_t                                 m_last_view_clock{0};

    // fade xip. fade version should not make new proposal
    xvip2_t                                  m_faded_xip2{};
    // record last xip in case of consensus success but leader xip changed.
    xvip2_t                                  m_last_xip2{};
    common::xtable_address_t                 m_table_addr;
    xpack_strategy_t                         m_pack_strategy;
};

using xbatch_packer_ptr_t = xobject_ptr_t<xbatch_packer>;
using xbatch_packers = std::vector<xbatch_packer_ptr_t>;
// batch_packer map for <tableid, packer>
using xbatch_paker_map = std::map<base::xtable_index_t, xbatch_packer_ptr_t, table_index_compare>;

NS_END2
