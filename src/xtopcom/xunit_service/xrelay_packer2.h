// Copyright (c) 2017-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include "xunit_service/xbatch_packer.h"

NS_BEG2(top, xunit_service)
// default block service entry
class xrelay_packer2 : public xbatch_packer {
public:
    explicit xrelay_packer2(observer_ptr<mbus::xmessage_bus_face_t> const & mb,
                            base::xtable_index_t & tableid,
                            const std::string & account_id,
                            std::shared_ptr<xcons_service_para_face> const & para,
                            std::shared_ptr<xblock_maker_face> const & block_maker,
                            base::xcontext_t & _context,
                            uint32_t target_thread_id);
    virtual ~xrelay_packer2();

public:
    virtual bool close(bool force_async = true) override;  // must call close before release object,otherwise object never be cleanup
    virtual bool on_timer_fire(const int32_t thread_id,
                               const int64_t timer_id,
                               const int64_t current_time_ms,
                               const int32_t start_timeout_ms,
                               int32_t & in_out_cur_interval_ms) override;

    virtual bool verify_vote_extend_data(base::xvblock_t * proposal_block, const xvip2_t & replica_xip, const std::string & vote_extend_data, std::string & result) override;
    virtual void add_vote_extend_data(base::xvblock_t * proposal_block, const xvip2_t & replica_xip, const std::string & vote_extend_data, const std::string & result) override;
    virtual bool proc_vote_complate(base::xvblock_t * proposal_block) override;
    virtual bool verify_commit_msg_extend_data(base::xvblock_t * block, const std::string & extend_data) override;

private:
    virtual int32_t set_vote_extend_data(base::xvblock_t * proposal_block, const uint256_t & hash, bool is_leader) override;
    virtual void clear_for_new_view() override;
    virtual void send_receipts(base::xvblock_t *vblock) override;
    virtual uint32_t calculate_min_tx_num(bool first_packing) override;
    virtual bool set_election_round(bool is_leader, data::xblock_consensus_para_t & proposal_para) override;
    bool    get_election_round(const xvip2_t & xip, uint64_t & election_round);
    void    get_elect_set(const xvip2_t & xip, xelection_cache_face::elect_set & elect_set);

private:
    std::map<std::string, std::pair<xvip2_t, std::string>> m_relay_multisign;  // key:pubkey string, value: first: xip, second: signature.
    uint256_t                               m_relay_hash{};
    uint64_t                                m_election_round{0};
    xelection_cache_face::elect_set         m_local_electset;
};

NS_END2
