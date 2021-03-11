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
#include "xtxexecutor/xunit_maker.h"
#include "xtxexecutor/xblock_maker_para.h"
#include "xtxexecutor/xtxexecutor_face.h"
#include "xtxpool/xtxpool_face.h"
#include "xunit_service/xcons_face.h"

NS_BEG2(top, txexecutor)

using data::xblock_t;
using data::xblock_consensus_para_t;
using data::xtable_block_para_t;
using data::xfulltable_block_para_t;


class xtable_maker_t : public xblock_maker_t {
 public:
    explicit xtable_maker_t(const std::string & account, const xblockmaker_resources_ptr_t & resources);
    virtual ~xtable_maker_t() {}

 public:
    xblock_ptr_t            make_proposal(xtablemaker_para_t & table_para, const data::xblock_consensus_para_t & cs_para, xtablemaker_result_t & result);
    int32_t                 verify_proposal(base::xvblock_t* proposal_block, const xtablemaker_para_t & table_para, const data::xblock_consensus_para_t & cs_para);

    int32_t                 default_check_latest_state();
    virtual int32_t         check_latest_state(base::xvblock_t* latest_cert_block) override;  // check table latest block and state
    virtual xblock_ptr_t    make_next_block(const data::xblock_consensus_para_t & cs_para, int32_t & error_code) override;
    virtual xblock_ptr_t    make_next_block(const data::xblock_consensus_para_t & cs_para, xunitmaker_result_t & result) override {return nullptr;}
    virtual xblock_ptr_t    make_next_block(const data::xblock_consensus_para_t & cs_para, xtablemaker_result_t & result) override;
    virtual bool            can_make_next_block() const override;
    virtual bool            can_make_next_empty_block() const override;
    virtual bool            can_make_next_full_block() const override;
    virtual bool            can_make_next_light_block() const override;
    xunit_maker_ptr_t       create_unit_maker(const std::string & account);
    bool                    push_tx(const std::string & account, const std::vector<xcons_transaction_ptr_t> & txs);

 protected:
    xblock_ptr_t            make_light_table(const data::xblock_consensus_para_t & cs_para, xtablemaker_result_t & result, int32_t & error_code);
    xblock_ptr_t            make_full_table(const xblock_consensus_para_t & cs_para, int32_t & error_code);
    bool                    update_unit_makers();
    xblock_ptr_t            make_empty_table(base::xvblock_t* prev_block, const xblock_consensus_para_t & cs_para);
    xblock_ptr_t            make_full_table(base::xvblock_t* prev_block, const xfulltable_block_para_t & table_para, const xblock_consensus_para_t & cs_para);
    xblock_ptr_t            make_light_table(base::xvblock_t* prev_block, const xtable_block_para_t & table_para, const xblock_consensus_para_t & cs_para);

    xunit_maker_ptr_t       get_unit_maker(const std::string & account);
    int32_t                 update_full_state(const xblock_ptr_t & latest_committed_block);
    void                    update_uncommit_unit_makers();
    bool                    update_latest_blocks(const xblock_ptr_t & latest_block);
    int32_t                 set_table_latest_state(base::xvblock_t* latest_committed_block);
    bool                    unpack_table_and_update_units(const xblock_ptr_t & block);
    void                    clear_table_units(const xblock_ptr_t & block);
    bool                    verify_proposal_with_local(base::xvblock_t *proposal_block, base::xvblock_t *local_block) const;
    bool                    verify_proposal_class(base::xvblock_t *proposal_block) const;

 private:
    std::map<std::string, xunit_maker_ptr_t>    m_unit_makers;
    data::xtable_mbt_ptr_t                      m_last_full_table_mbt{nullptr};
    data::xtable_mbt_binlog_ptr_t               m_commit_table_mbt_binlog{nullptr};
    data::xblock_ptr_t                          m_last_full_block{nullptr};
    static constexpr uint32_t                   m_keep_latest_blocks_max{100};
    static constexpr uint32_t                   m_empty_block_max_num{2};
    static constexpr uint32_t                   m_full_table_interval_num{1000};
    mutable std::mutex                          m_lock;
};

using xtable_maker_ptr_t = xobject_ptr_t<xtable_maker_t>;

NS_END2
