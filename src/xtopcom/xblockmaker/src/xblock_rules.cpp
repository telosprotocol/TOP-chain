// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <string>
#include <cinttypes>
#include "xbasic/xmodule_type.h"
#include "xblockmaker/xblock_rules.h"

NS_BEG2(top, blockmaker)

bool xblock_rules::check_rule_txs_one_type(const std::vector<xcons_transaction_ptr_t> & txs) const {
    xassert(!txs.empty());
    enum_transaction_subtype first_tx_subtype = txs[0]->get_tx_subtype();
    if (first_tx_subtype == enum_transaction_subtype_self) {
        first_tx_subtype = enum_transaction_subtype_send;
    }

    for (auto & tx : txs) {
        uint8_t subtype = tx->get_tx_subtype();
        if (subtype == enum_transaction_subtype_self) {
            subtype = enum_transaction_subtype_send;
        }
        if (subtype != first_tx_subtype) {
            xerror("xblock_rules::check_rule_txs_one_type fail.subtype=%d,tx=%s", subtype, tx->dump().c_str());
            return false;
        }
    }
    return true;
}

bool xblock_rules::check_rule_sendtx_duplication(const xaccount_ptr_t & rules_end_state,
                                                 const std::vector<xcons_transaction_ptr_t> & txs,
                                                std::vector<xcons_transaction_ptr_t> & valid_txs,
                                                std::vector<xcons_transaction_ptr_t> & pop_txs) const {
    // uint64_t latest_nonce = rules_end_state->get_latest_send_trans_number();
    // uint256_t latest_hash = rules_end_state->get_latest_send_trans_hash();

    // for (auto & tx : txs) {
    //     if (tx->is_recv_tx() || tx->is_confirm_tx()) {
    //         valid_txs.push_back(tx);
    //         continue;
    //     }

    //     xtransaction_t* tx_ptr = tx->get_transaction();
    //     if (tx_ptr->get_last_nonce() < latest_nonce) {
    //         pop_txs.push_back(tx);
    //         xwarn("xblock_rules::check_rule_sendtx_duplication fail-tx nonce too old.account=%s,latest_nonce=%" PRIu64 ",tx=%s",
    //             rules_end_state->get_account().c_str(), latest_nonce, tx->dump().c_str());
    //     } else if (tx_ptr->get_last_nonce() == latest_nonce) {
    //         if (!tx_ptr->check_last_trans_hash(latest_hash)) {
    //             xwarn("xblock_rules::check_rule_sendtx_duplication fail-tx hash not match,account=%s,latest_nonce=%" PRIu64 ",tx=%s",
    //                 rules_end_state->get_account().c_str(), latest_nonce, tx->dump().c_str());
    //             pop_txs.push_back(tx);
    //             return false;
    //         } else {
    //             valid_txs.push_back(tx);
    //             latest_nonce = tx_ptr->get_tx_nonce();
    //             latest_hash = tx_ptr->digest();
    //         }
    //     } else {
    //         xwarn("xblock_rules::check_rule_sendtx_duplication fail-state not match,account=%s,origin_nonce=%" PRIu64 ",latest_nonce=%" PRIu64 ",tx:%s",
    //             rules_end_state->get_account().c_str(), rules_end_state->get_latest_send_trans_number(), latest_nonce, tx->dump().c_str());
    //         return false;
    //     }
    // }
    return true;
}

void xblock_rules::check_rule_batch_txs(const std::vector<xcons_transaction_ptr_t> & txs,
                                            std::vector<xcons_transaction_ptr_t> & valid_txs,
                                            std::vector<xcons_transaction_ptr_t> & pop_txs) const {
    xassert(valid_txs.empty());
    // TODO(jimmy) now, transfer txs and confirm txs can batch
    bool can_batch = true;
    for (auto & tx : txs) {
        if ( (tx->get_transaction()->get_tx_type() != xtransaction_type_transfer) && (!tx->is_confirm_tx()) ) {
            can_batch = false;
            break;
        }
    }
    // if can't batch, then take first tx, otherwise take all txs
    if (!can_batch) {
        valid_txs.push_back(txs[0]);
    } else {
        valid_txs = txs;
    }
}

bool xblock_rules::unit_rules_filter(const xblock_ptr_t & rules_end_block,
                                        const xaccount_ptr_t & rules_end_state,
                                        const std::vector<xcons_transaction_ptr_t> & origin_txs,
                                        std::vector<xcons_transaction_ptr_t> & valid_txs,
                                        std::vector<xcons_transaction_ptr_t> & pop_txs) {
    xassert(!origin_txs.empty());
    xassert(valid_txs.empty());
    xassert(pop_txs.empty());

    check_rule_sendtx_duplication(rules_end_state, origin_txs, valid_txs, pop_txs);

    // TODO(jimmy)  check_rule_tx_timestamp less than unit timestamp

    // valid_txs = valid_txs2;
    return true;
}


NS_END2
