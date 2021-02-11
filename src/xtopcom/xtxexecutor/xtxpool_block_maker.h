// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include <vector>

#include "xbasic/xns_macro.h"
#include "xstore/xstore_face.h"
#include "xblockstore/xblockstore_face.h"
#include "xtxpool/xtxpool_face.h"
#include "xunit_service/xcons_face.h"
#include "xtxexecutor/xunit_blockmaker.h"

NS_BEG2(top, txexecutor)

using xunit_service::xblock_maker_para_t;

class xtable_blockmaker_t : public xunit_service::xblock_maker_face {
 public:
    xtable_blockmaker_t(const observer_ptr<store::xstore_face_t> & store, base::xvblockstore_t* blockstore, xtxpool::xtxpool_face_t* txpool);
    virtual ~xtable_blockmaker_t();

 public:
    base::xauto_ptr<base::xvblock_t>    get_latest_block(const std::string &account) override;
    base::xvblock_t *               make_block(const std::string &account, const xblock_maker_para_t & para, const xvip2_t& leader_xip) override;
    base::xvblock_t *               make_block(const std::string &account, uint64_t clock, uint64_t viewid, uint16_t threshold, const xvip2_t& leader_xip) override {xassert(0);return nullptr;}
    int                             verify_block(base::xvblock_t *proposal_block) override {return -1;}
    int                             verify_block(base::xvblock_t *proposal_block, base::xvqcert_t * bind_clock_cert, base::xvqcert_t * bind_drand_cert, xtxpool::xtxpool_table_face_t* txpool_table, uint64_t committed_height, const xvip2_t& xip) override;
    xtxpool::xtxpool_table_face_t*  get_txpool_table(const std::string & table_account) override;

 private:
    int                 make_tableblock(const std::vector<std::string> &accounts,
                                        base::xvblock_t* latest_cert_block,
                                        const xblock_consensus_para_t & cs_para,
                                        uint64_t committed_height,
                                        xtxpool::xtxpool_table_face_t* txpool_table,
                                        base::xvblock_t* & proposal_block);
    // base::xvblock_t*    verify_tableblock(base::xvblock_t* proposal_block,
    //                                             const std::vector<xblock_t *> & units,
    //                                             base::xvblock_t* prev_block);
    int                 check_proposal_connect(base::xvblock_t *proposal_block, const xvip2_t& xip);
    int                 check_cert_connect(base::xvblock_t* latest_cert_block);
    int                 verify_out_tableblock(base::xvblock_t *proposal_block, base::xvblock_t *local_block);
    int                 try_verify_tableblock(base::xvblock_t *proposal_block, base::xvblock_t* latest_cert_block, const xblock_consensus_para_t &cs_para, const xvip2_t& xip, xtxpool::xtxpool_table_face_t* txpool_table, uint64_t committed_height);
    int                 validity_check_for_proposal(base::xvblock_t *proposal_block);
    void                invoke_sync(const std::string & account, const std::string & reason);
    std::string         calc_random_seed(base::xvblock_t* latest_cert_block, base::xvqcert_t* drand_cert, uint64_t viewtoken);
    bool                leader_set_consensus_para(base::xvblock_t* latest_cert_block, const xblock_maker_para_t & blockmaker_para, xblock_consensus_para_t & cs_para, bool is_empty_block);
    bool                backup_set_consensus_para(base::xvblock_t* latest_cert_block, base::xvblock_t* proposal, base::xvqcert_t * bind_drand_cert, xblock_consensus_para_t & cs_para);

    observer_ptr<store::xstore_face_t>  m_store;
    base::xvblockstore_t*               m_blockstore{nullptr};
    xtxpool::xtxpool_face_t*            m_txpool{nullptr};
    xunit_blockmaker_t*                 m_unit_blockmaker;
};

class xblockmaker_factory {
 public:
    static std::shared_ptr<xunit_service::xblock_maker_face> create_txpool_block_maker(const observer_ptr<store::xstore_face_t> & store, base::xvblockstore_t* blockstore, xtxpool::xtxpool_face_t* txpool);
};

NS_END2
