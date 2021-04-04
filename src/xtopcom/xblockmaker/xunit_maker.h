// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include <map>
#include <vector>
#include "xdata/xblock.h"
#include "xdata/xtableblock.h"
#include "xdata/xfull_tableblock.h"
#include "xdata/xblockchain.h"
#include "xstore/xstore_face.h"
#include "xblockmaker/xblock_maker_para.h"
#include "xblockmaker/xblockmaker_face.h"
#include "xblockmaker/xblock_rules.h"

NS_BEG2(top, blockmaker)

class xunit_maker_t : public xblock_maker_t {
 public:
    explicit xunit_maker_t(const std::string & account, const xblockmaker_resources_ptr_t & resources, const store::xindexstore_face_ptr_t & indexstore = nullptr);
    virtual ~xunit_maker_t() {}

 public:
    int32_t                 check_latest_state(const xblock_ptr_t & committed_table_block = nullptr);  // check block and state is latest
    xblock_ptr_t            make_proposal(const xunit_proposal_input_t & proposal_input, const data::xblock_consensus_para_t & cs_para, xunitmaker_result_t & result);
    xblock_ptr_t            verify_proposal(const xunit_proposal_input_t & proposal_input, const data::xblock_consensus_para_t & cs_para, xunitmaker_result_t & result);
    bool                    can_make_next_block() const;
    bool                    unit_rules_filter(const std::vector<xcons_transaction_ptr_t> & origin_txs,
                                                std::vector<xcons_transaction_ptr_t> & valid_txs,
                                                std::vector<xcons_transaction_ptr_t> & pop_txs);
    bool                    can_make_next_empty_block() const;

 protected:
    virtual xblock_ptr_t    make_next_block(const data::xblock_consensus_para_t & cs_para, xunitmaker_result_t & result);

    bool                    can_make_next_full_block() const;
    bool                    can_make_next_light_block() const;

    bool                    backup_push_txs(const std::vector<xcons_transaction_ptr_t> & txs);
    bool                    leader_push_txs(const std::vector<xcons_transaction_ptr_t> & txs);
    void                    clear_tx();
    void                    init_unit_blocks(const base::xblock_mptrs & latest_blocks);
    bool                    is_account_locked() const;
    bool                    update_latest_blocks(const xblock_ptr_t & latest_block);
    std::string             dump() const;
    bool                    check_index_state(const xblock_ptr_t & committed_table_block, const base::xblock_mptrs & latest_blocks);

 private:
    std::vector<xcons_transaction_ptr_t>        m_pending_txs;
    data::xblock_ptr_t                          m_proposal_unit{nullptr};
    static constexpr uint32_t                   m_keep_latest_blocks_max{3};  // only keep commit/lock/highqc block
    static constexpr uint32_t                   m_consecutive_empty_unit_max{2};
    uint32_t                                    m_fullunit_contain_of_unit_num_para;
    xblock_rules_face_ptr_t                     m_block_rules;
    xblock_builder_face_ptr_t                   m_fullunit_builder;
    xblock_builder_face_ptr_t                   m_lightunit_builder;
    xblock_builder_face_ptr_t                   m_emptyunit_builder;
    xblock_builder_para_ptr_t                   m_default_builder_para;
    store::xindexstore_face_ptr_t         m_indexstore;
};

using xunit_maker_ptr_t = xobject_ptr_t<xunit_maker_t>;

NS_END2
