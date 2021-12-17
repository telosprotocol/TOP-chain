// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include <vector>
#include "xdata/xblock.h"
#include "xblockmaker/xblock_maker_para.h"
#include "xblockmaker/xblockmaker_face.h"

NS_BEG2(top, blockmaker)

class xunit_maker_t : public xblock_maker_t {
 public:
    explicit xunit_maker_t(const std::string & account, const xblockmaker_resources_ptr_t & resources);
    virtual ~xunit_maker_t();

 public:
    int32_t                 check_latest_state(const data::xblock_consensus_para_t & cs_para, const base::xaccount_index_t & account_index, const base::xaccount_index_t & commit_account_index);  // check block and state is latest
    bool                    push_tx(const data::xblock_consensus_para_t & cs_para, const xcons_transaction_ptr_t & tx);
    void                    clear_tx();
    xblock_ptr_t            make_proposal(const xunitmaker_para_t & unit_para, const data::xblock_consensus_para_t & cs_para, xunitmaker_result_t & result);
    bool                    can_make_next_block() const;
    bool                    can_make_next_block_v2() const;
    bool                    can_make_next_empty_block() const;
    bool                    can_make_next_full_block(bool is_forked_unit_opt = false) const;
    bool                    must_make_next_full_block() const;

 protected:
    void                    make_light_block(xblock_ptr_t & proposal_unit, xblock_builder_face_ptr_t & blockbuilder, const xunitmaker_para_t & unit_para, const data::xblock_consensus_para_t & cs_para, xunitmaker_result_t & result);
    xblock_ptr_t            make_next_block(const xunitmaker_para_t & unit_para, const data::xblock_consensus_para_t & cs_para, xunitmaker_result_t & result);
    bool                    can_make_next_light_block() const;
    bool                    is_account_locked() const;
    std::string             dump() const;
    xblock_ptr_t            get_latest_block(const base::xaccount_index_t & account_index);
    void                    find_highest_send_tx(uint64_t & latest_nonce, uint256_t & latest_hash);
    bool                    is_match_account_fullunit_send_tx_limit(uint64_t current_lightunit_count) const;
    bool                    is_match_account_fullunit_recv_tx_limit(uint64_t current_lightunit_count) const;
    void                    try_sync_lacked_blocks(uint64_t from_height, uint64_t to_height, const std::string & reason, bool is_consensus, bool need_proof);
    uint64_t                get_current_lightunit_count_from_full() const;

 private:
    std::vector<xcons_transaction_ptr_t>        m_pending_txs;
    data::xblock_ptr_t                          m_proposal_unit{nullptr};
    static constexpr uint32_t                   m_keep_latest_blocks_max{2};  // only keep latest unit block after fork of remove empty unit block
    static constexpr uint32_t                   m_consecutive_empty_unit_max{2};
    uint32_t                                    m_fullunit_contain_of_unit_num_para;
    xblock_builder_face_ptr_t                   m_fullunit_builder;
    xblock_builder_face_ptr_t                   m_lightunit_builder;
    xblock_builder_face_ptr_t                   m_emptyunit_builder;
    xblock_builder_para_ptr_t                   m_default_builder_para;
    bool                                        m_check_state_success{false};
    base::xaccount_index_t                      m_latest_account_index;
};

using xunit_maker_ptr_t = xobject_ptr_t<xunit_maker_t>;

NS_END2
