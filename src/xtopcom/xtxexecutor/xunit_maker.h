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
#include "xtxexecutor/xblock_maker_para.h"
#include "xtxexecutor/xtransaction_executor.h"
#include "xtxexecutor/xtxexecutor_face.h"
#include "xstore/xaccount_context.h"

NS_BEG2(top, txexecutor)

class xunit_maker_t : public xblock_maker_t {
 public:
    explicit xunit_maker_t(const std::string & account, const xblockmaker_resources_ptr_t & resources);
    virtual ~xunit_maker_t() {}

 public:
    int32_t                 default_check_latest_state();
    virtual int32_t         check_latest_state(base::xvblock_t* latest_cert_block = nullptr) override;  // check block and state is latest
    virtual xblock_ptr_t    make_next_block(const data::xblock_consensus_para_t & cs_para, int32_t & error_code) override;
    virtual xblock_ptr_t    make_next_block(const data::xblock_consensus_para_t & cs_para, xunitmaker_result_t & result) override;
    virtual xblock_ptr_t    make_next_block(const data::xblock_consensus_para_t & cs_para, xtablemaker_result_t & result) override {return nullptr;}
    virtual bool            can_make_next_block() const override;
    virtual bool            can_make_next_empty_block() const override;
    virtual bool            can_make_next_full_block() const override;
    virtual bool            can_make_next_light_block() const override;

    bool            push_tx(const std::vector<xcons_transaction_ptr_t> & txs);

 protected:
    xblock_ptr_t    make_full_unit(int32_t & error_code);
    xblock_ptr_t    make_light_unit(const xblock_consensus_para_t & cs_para, xbatch_txs_result_t & exec_result, int32_t & error_code);
    xblock_ptr_t    make_empty_unit(int32_t & error_code);

    int             make_fullunit_output(const data::xaccount_ptr_t & bstate, xfullunit_block_para_t & para);
    int             make_lightunit_output(store::xaccount_context_t * context,
                                            const std::vector<xcons_transaction_ptr_t> & txs,
                                            xbatch_txs_result_t & exec_result,
                                            data::xlightunit_block_para_t & lightunit_para);
    void            init_unit_blocks(const base::xblock_mptrs & latest_blocks);

 private:
    std::vector<xcons_transaction_ptr_t>        m_pending_txs;
    data::xblock_ptr_t                          m_proposal_unit{nullptr};
    static constexpr uint32_t                   m_keep_latest_blocks_max{3};  // only keep commit/lock/highqc block
    static constexpr uint32_t                   m_consecutive_empty_unit_max{2};
    uint32_t                                    m_fullunit_contain_of_unit_num_para;
};

using xunit_maker_ptr_t = xobject_ptr_t<xunit_maker_t>;

NS_END2
