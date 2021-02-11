// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include <vector>

#include "xdata/xcons_transaction.h"
#include "xtxpool/xtxpool_face.h"
#include "xstore/xaccount_context.h"
#include "xdata/xlightunit.h"
#include "xdata/xfullunit.h"
#include "xtxexecutor/xblock_maker_para.h"

NS_BEG2(top, txexecutor)
using store::xaccount_context_t;

class xunit_blockmaker_t {
 public:
    xunit_blockmaker_t(observer_ptr<store::xstore_face_t> store, base::xvblockstore_t* blockstore, xtxpool::xtxpool_face_t* txpool)
    : m_store(store), m_blockstore(blockstore), m_txpool(txpool) {}
    virtual ~xunit_blockmaker_t() {}

 public:
    virtual int make_block(const std::string & account,
                            const xblock_consensus_para_t & cs_para,
                            uint64_t committed_height,
                            xtxpool::xtxpool_table_face_t* txpool_table,
                            base::xvblock_t* & out_unit,
                            xunit_proposal_input_t & proposal_input);
    virtual int verify_block(const xunit_proposal_input_t & proposal_input,
                             const xblock_consensus_para_t & cs_para,
                             base::xvblock_t* & proposal_unit,
                             xtxpool::xtxpool_table_face_t* txpool_table,
                             uint64_t committed_height);

 protected:
    base::xvblock_t* create_lightunit(xaccount_context_t* context,
                                    base::xvblock_t* prev_block,
                                    const xblock_consensus_para_t & cs_para,
                                    const std::vector<xcons_transaction_ptr_t> & input_txs,
                                    int & error_code,
                                    std::vector<xcons_transaction_ptr_t> & proposal_txs);
    base::xvblock_t* create_fullunit(xaccount_context_t* context,
                                    base::xvblock_t* prev_block,
                                    const xblock_consensus_para_t & cs_para,
                                    int & error_code);
    base::xvblock_t* create_unit(xaccount_context_t* context,
                                    base::xvblock_t* prev_block,
                                    const xblock_consensus_para_t & cs_para,
                                    const std::vector<xcons_transaction_ptr_t> & input_txs,
                                    const std::string & justify_cert_hash,
                                    int & error_code,
                                    std::vector<xcons_transaction_ptr_t> & proposal_txs);
 private:
    bool                can_make_full_unit(xaccount_context_t * context, base::xvblock_t* prev_block);
    int                 make_lightunit_output(xaccount_context_t * context, const std::vector<xcons_transaction_ptr_t> & txs, data::xlightunit_block_para_t & lightunit_para);
    int                 make_fullunit_output(xaccount_context_t * context, xfullunit_block_para_t & para);
    int                 verify_proposal_block_class(xaccount_context_t * context, base::xvheader_t* unitheader, base::xvblock_t * latest_unit);
    int                 backup_check_state(const xunit_proposal_input_t & proposal_input, base::xvblock_t* prev_unit);
    int                 leader_check_state(const std::string & account, base::xvblock_t* latest_commit_block);
    void                invoke_sync(const std::string & account, const std::string & reason);
    bool                get_lock_block_sign_hash(base::xvblock_t* highqc_block, std::string & hash);

 private:
    observer_ptr<store::xstore_face_t> m_store;
    base::xvblockstore_t*              m_blockstore{nullptr};
    xtxpool::xtxpool_face_t*           m_txpool{nullptr};
};

NS_END2
