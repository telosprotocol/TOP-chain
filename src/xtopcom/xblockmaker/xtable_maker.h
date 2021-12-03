// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include <map>
#include <vector>
#include <mutex>
#include "xdata/xtableblock.h"
#include "xdata/xfull_tableblock.h"
#include "xblockmaker/xunit_maker.h"
#include "xblockmaker/xblock_maker_para.h"
#include "xblockmaker/xblockmaker_face.h"
#include "xunit_service/xcons_face.h"

NS_BEG2(top, blockmaker)

using data::xblock_t;
using data::xblock_consensus_para_t;
using data::xtable_block_para_t;
using data::xfulltable_block_para_t;


class xtable_maker_t : public xblock_maker_t {
 public:
    explicit xtable_maker_t(const std::string & account, const xblockmaker_resources_ptr_t & resources);
    virtual ~xtable_maker_t();

 public:
    int32_t                 default_check_latest_state();
    xblock_ptr_t            make_proposal(xtablemaker_para_t & table_para, const data::xblock_consensus_para_t & cs_para, xtablemaker_result_t & result);
    int32_t                 verify_proposal(base::xvblock_t* proposal_block, const xtablemaker_para_t & table_para, const data::xblock_consensus_para_t & cs_para);

 protected:
    int32_t                 check_latest_state(const xblock_ptr_t & latest_block); // check table latest block and state
    bool                    can_make_next_full_block() const;
    bool                    can_make_next_light_block(xtablemaker_para_t & table_para) const;
    xunit_maker_ptr_t       create_unit_maker(const std::string & account);

    xblock_ptr_t            leader_make_light_table(const xtablemaker_para_t & table_para, const data::xblock_consensus_para_t & cs_para, xtablemaker_result_t & table_result);
    xblock_ptr_t            backup_make_light_table(const xtablemaker_para_t & table_para, const data::xblock_consensus_para_t & cs_para, xtablemaker_result_t & table_result);
    xblock_ptr_t            make_full_table(const xtablemaker_para_t & table_para, const xblock_consensus_para_t & cs_para, int32_t & error_code);
    xblock_ptr_t            make_empty_table(const xtablemaker_para_t & table_para, const xblock_consensus_para_t & cs_para, int32_t & error_code);
    void                    clear_old_unit_makers();

    bool                    verify_proposal_with_local(base::xvblock_t *proposal_block, base::xvblock_t *local_block) const;
    bool                    verify_proposal_class(base::xvblock_t *proposal_block) const;
    std::string             dump() const;
    bool                    load_table_blocks_from_last_full(const xblock_ptr_t & prev_block, std::vector<xblock_ptr_t> & blocks);
    bool                    create_lightunit_makers(const xtablemaker_para_t & table_para, const data::xblock_consensus_para_t & cs_para, std::map<std::string, xunit_maker_ptr_t> & unitmakers);
    bool                    create_non_lightunit_makers(const xtablemaker_para_t & table_para, const data::xblock_consensus_para_t & cs_para, std::map<std::string, xunit_maker_ptr_t> & unitmakers);
    void                    get_unit_accounts(const xblock_ptr_t & block, std::set<std::string> & accounts) const;
    xblock_ptr_t            make_light_table(bool is_leader, const xtablemaker_para_t & table_para, const data::xblock_consensus_para_t & cs_para, xtablemaker_result_t & table_result);
    bool                    create_other_makers(const xtablemaker_para_t & table_para, const data::xblock_consensus_para_t & cs_para, std::map<std::string, xunit_maker_ptr_t> & unitmakers);
    void                    clear_all_pending_txs();
    void                    refresh_cache_unit_makers();
    void                    set_packtx_metrics(const xcons_transaction_ptr_t & tx, bool bsucc) const;
    bool                    can_make_next_empty_block() const;

 private:
    std::map<std::string, xunit_maker_ptr_t>    m_unit_makers;
    static constexpr uint32_t                   m_keep_latest_blocks_max{3}; // only keep commit/lock/highqc block
    static constexpr uint32_t                   m_empty_block_max_num{2};
    static constexpr uint32_t                   m_cache_unit_maker_parent_height{10};
    uint32_t                                    m_full_table_interval_num;
    xblock_builder_face_ptr_t                   m_fulltable_builder;
    xblock_builder_face_ptr_t                   m_lighttable_builder;
    xblock_builder_face_ptr_t                   m_emptytable_builder;
    xblock_builder_para_ptr_t                   m_default_builder_para;
    bool                                        m_check_state_success{false};
    mutable std::mutex                          m_lock;
};

using xtable_maker_ptr_t = xobject_ptr_t<xtable_maker_t>;

NS_END2
