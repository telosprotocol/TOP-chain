// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include <map>
#include "xdata/xtableblock.h"
#include "xtxexecutor/xtable_maker.h"
#include "xtxexecutor/xblock_maker_para.h"
#include "xtxexecutor/xtxexecutor_face.h"
#include "xunit_service/xcons_face.h"

NS_BEG2(top, txexecutor)

class xtable_checker_t {// : public xunit_service::xblock_maker_face
 public:
    xtable_checker_t(const std::string & account, const xblockmaker_resources_ptr_t & resources);

 public:
    // base::xauto_ptr<base::xvblock_t>    get_latest_block(const std::string &account) override;
    // base::xvblock_t *               make_block(const std::string &account, const xunit_service::xblock_maker_para_t & para, const xvip2_t& leader_xip) override;
    // base::xvblock_t *               make_block(const std::string &account, uint64_t clock, uint64_t viewid, uint16_t threshold, const xvip2_t& leader_xip) override {xassert(0);return nullptr;}
    // int                             verify_block(base::xvblock_t *proposal_block) override {return -1;}
    // int                             verify_block(base::xvblock_t *proposal_block, base::xvqcert_t * bind_clock_cert, base::xvqcert_t * bind_drand_cert, xtxpool::xtxpool_table_face_t* txpool_table, uint64_t committed_height, const xvip2_t& xip) override;
    // xtxpool::xtxpool_table_face_t*  get_txpool_table(const std::string & table_account) override;

    int verify_proposal(base::xvblock_t * proposal_block, base::xvqcert_t * bind_clock_cert);

 private:
    xtable_maker_ptr_t m_table_maker{nullptr};
};


class xtablechecker_factory {
 public:
    static std::shared_ptr<xunit_service::xblock_maker_face> create_table_maker(const observer_ptr<store::xstore_face_t> & store, base::xvblockstore_t* blockstore, xtxpool::xtxpool_face_t* txpool);
};


NS_END2
