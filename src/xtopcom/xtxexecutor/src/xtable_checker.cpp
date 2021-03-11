// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <string>
#include "xtxexecutor/xtable_checker.h"

NS_BEG2(top, txexecutor)

// base::xauto_ptr<base::xvblock_t> xtable_checker_t::get_latest_block(const std::string &account) {
//     xassert(false);
//     return nullptr;
// }

// base::xvblock_t * xtable_checker_t::make_block(const std::string &account, const xunit_service::xblock_maker_para_t & para, const xvip2_t& leader_xip) {
//     xassert(false);
//     return nullptr;
// }

// base::xvblock_t * xtable_checker_t::make_block(const std::string &account, uint64_t clock, uint64_t viewid, uint16_t threshold, const xvip2_t& leader_xip) {
//     xassert(false);
//     return nullptr;
// }

// int xtable_checker_t::verify_block(base::xvblock_t *proposal_block) {
//     xassert(false);
//     return -1;
// }

// int xtable_checker_t::verify_block(base::xvblock_t *proposal_block, base::xvqcert_t * bind_clock_cert, base::xvqcert_t * bind_drand_cert,
//                                       xtxpool::xtxpool_table_face_t* txpool_table, uint64_t committed_height, const xvip2_t& xip) {
//     xassert(false);
//     return -1;
// }

// xtxpool::xtxpool_table_face_t*  xtable_checker_t::get_txpool_table(const std::string & table_account) {
//     xassert(false);
//     return nullptr;
// }

int xtable_checker_t::verify_proposal(base::xvblock_t * proposal_block, base::xvqcert_t * bind_clock_cert) {
    return xsuccess;
}

NS_END2
