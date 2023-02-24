// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include "xdata/xblock.h"
#include "xcommon/xaccount_address.h"
#include "xevm_common/common.h"
#include "xdata/xtable_bstate.h"

NS_BEG2(top, data)

class xblock_consensus_para_t {
 public:
    xblock_consensus_para_t() = default;

    xblock_consensus_para_t(const std::string & _account, uint64_t _clock, uint64_t _viewid, uint32_t _viewtoken, uint64_t _proposal_height, uint64_t _gmtime);
    xblock_consensus_para_t(const xvip2_t & validator, base::xvblock_t* prev_block);

 public:
    void    set_xip(const xvip2_t & _validator_xip, const xvip2_t & _auditor_xip);
    void    set_drand_block(base::xvblock_t* _drand_block);
    void    set_latest_blocks(const base::xblock_mptrs & latest_blocks);
    void    set_latest_blocks(const xblock_ptr_t & certblock, const xblock_ptr_t & lock_block, const xblock_ptr_t & commit_block);
    void    set_validator(const xvip2_t & validator) {m_validator = validator;}
    void    set_common_consensus_para(uint64_t clock,
                                   const xvip2_t & validator,
                                   const xvip2_t & auditor,
                                   uint64_t viewid,
                                   uint32_t m_viewtoken,
                                   uint64_t drand_height);
    void    set_tableblock_consensus_para(uint64_t drand_height,
                                       const std::string & random_seed,
                                       uint64_t total_lock_tgas_token,
                                       uint64_t total_lock_tgas_token_property_height);
    void    set_justify_cert_hash(const std::string & justify_cert_hash) const {m_justify_cert_hash = justify_cert_hash;}
    void    set_parent_height(uint64_t height) const {m_parent_height = height;}
    void    set_timeofday_s(uint64_t now) {m_timeofday_s = now;}
    void    set_clock(uint64_t clock) {m_clock = clock;}
    void    set_viewid(uint64_t viewid) {m_viewid = viewid;}
    void    set_drand_height(uint64_t drand_height) {m_drand_height = drand_height;}
    void    set_ethheader(const std::string & ethheader) const {m_ethheader = ethheader;}
    void    set_coinbase(common::xaccount_address_t const& address) {m_coinbase = address;}
    void    set_block_gaslimit(uint64_t _gas_limit) {m_block_gas_limit = _gas_limit;}
    void    set_block_base_price(evm_common::u256 const& _price) {m_base_price = _price;}
    void    set_election_round(uint64_t election_round) {m_election_round = election_round;}
    void    set_table_state(xtablestate_ptr_t const& cert_state, xtablestate_ptr_t const& commit_state);
    void    set_need_relay_prove(bool is_need) const {m_need_relay_prove = is_need;}
    void    set_vote_extend_hash(const uint256_t & hash) const {m_vote_extend_hash = hash;}
    void    set_tgas_height(uint64_t tgas_height) {m_total_lock_tgas_token_property_height = tgas_height;}
    void    set_total_burn_gas(const uint64_t burn_gas) const;

 public:
    const std::string &     get_random_seed() const {return m_random_seed;}
    uint64_t                get_clock() const {return m_clock;}
    uint64_t                get_gmtime() const {return m_gmtime;}
    uint64_t                get_viewid() const {return m_viewid;}
    uint32_t                get_viewtoken() const {return m_viewtoken;}
    uint64_t                get_timestamp() const {return (uint64_t)(m_clock * 10) + base::TOP_BEGIN_GMTIME;}
    uint64_t                get_drand_height() const {return m_drand_height;}
    const xblock_ptr_t &    get_drand_block() const {return m_drand_block;}
    const xblock_ptr_t &    get_latest_cert_block() const {return m_latest_cert_block;}
    const xblock_ptr_t &    get_latest_locked_block() const {return m_latest_locked_block;}
    const xblock_ptr_t &    get_latest_committed_block() const {return m_latest_committed_block;}
    const xvip2_t &         get_validator() const {return m_validator;}
    const xvip2_t &         get_auditor() const {return m_auditor;}
    xvip2_t                 get_leader_xip() const;
    common::xaccount_address_t const&   get_coinbase() const {return m_coinbase;}
    uint64_t                get_total_lock_tgas_token() const {return m_total_lock_tgas_token;}
    uint64_t                get_tgas_height() const {return m_total_lock_tgas_token_property_height;}
    const std::string &     get_justify_cert_hash() const {return m_justify_cert_hash;}
    uint64_t                get_proposal_height() const {return m_proposal_height;}
    const std::string &     get_table_account() const {return m_account;}
    uint64_t                get_table_proposal_height() const {return m_proposal_height;}
    uint64_t                get_parent_height() const {return m_parent_height;}
    const std::string &     dump() const {return m_dump_str;}
    uint64_t                get_gettimeofday_s() const {return m_timeofday_s;}
    const std::string &     get_ethheader() const {return m_ethheader;}
    uint64_t                get_block_gaslimit() const {return m_block_gas_limit;}
    evm_common::u256 const& get_block_base_price() const {return m_base_price;}
    uint64_t                get_election_round() const {return m_election_round;}
    xtablestate_ptr_t const& get_cert_table_state() const {return m_cert_tablestate;}
    xtablestate_ptr_t const& get_commit_table_state() const {return m_commit_tablestate;}
    bool                    need_relay_prove() const {return m_need_relay_prove;}
    const uint256_t &       get_vote_extend_hash() const {return m_vote_extend_hash;}
   uint64_t                 get_total_burn_gas() const {return  m_total_burn_tgas_token;}

 private:
    std::string     m_account;
    uint64_t        m_clock{0};
    uint32_t        m_viewtoken{0};
    uint64_t        m_viewid{0};
    uint64_t        m_proposal_height{0};
    uint64_t        m_gmtime{0};
    xvip2_t         m_validator;
    xvip2_t         m_auditor;
    uint64_t        m_drand_height{0};
    std::string     m_random_seed;
    uint64_t        m_total_lock_tgas_token{0};
    uint64_t        m_total_lock_tgas_token_property_height{0};
    std::string     m_dump_str;
    xblock_ptr_t    m_drand_block{nullptr};
    xblock_ptr_t    m_latest_cert_block{nullptr};
    xblock_ptr_t    m_latest_locked_block{nullptr};
    xblock_ptr_t    m_latest_committed_block{nullptr};
    mutable std::string     m_justify_cert_hash;  // may changed by unit
    mutable uint64_t        m_parent_height{0};  // may changed by unit
    uint64_t        m_timeofday_s{0};
    mutable std::string     m_ethheader;
    common::xaccount_address_t  m_coinbase;
    uint64_t                    m_block_gas_limit{0};
    evm_common::u256            m_base_price;
    uint64_t        m_election_round;
    xtablestate_ptr_t         m_cert_tablestate{nullptr};
    xtablestate_ptr_t         m_commit_tablestate{nullptr};
    mutable bool            m_need_relay_prove{false};
    mutable uint256_t       m_vote_extend_hash;
    mutable uint64_t        m_total_burn_tgas_token{0};
};

using xblock_consensus_para_ptr_t = std::shared_ptr<xblock_consensus_para_t>;

NS_END2
