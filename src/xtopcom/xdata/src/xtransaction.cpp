// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include "xdata/xtransaction.h"

namespace top { namespace data {

bool xtransaction_t::transaction_type_check() const {
    switch (get_tx_type()) {
#ifdef ENABLE_CREATE_USER  // debug use
        case xtransaction_type_create_user_account:
#endif
        case xtransaction_type_run_contract:
        case xtransaction_type_transfer:
        case xtransaction_type_vote:
        case xtransaction_type_abolish_vote:
        case xtransaction_type_pledge_token_tgas:
        case xtransaction_type_redeem_token_tgas:
        case xtransaction_type_pledge_token_vote:
        case xtransaction_type_redeem_token_vote:
            return true;
        default:
            return false;
    }
}

std::string xtransaction_t::transaction_type_to_string(uint16_t type) {
    switch (type) {
        case xtransaction_type_create_user_account: return "create_user";
        case xtransaction_type_run_contract:        return "run_contract";
        case xtransaction_type_transfer:            return "transfer";
        case xtransaction_type_vote:                return "vote";
        case xtransaction_type_abolish_vote:        return "abolist_vote";
        case xtransaction_type_pledge_token_tgas:   return "pldge_tgas";
        case xtransaction_type_redeem_token_tgas:   return "redeem_tgas";
        case xtransaction_type_pledge_token_vote:   return "pledge_vote";
        case xtransaction_type_redeem_token_vote:   return "redeem_vote";
        default:
            xassert(false);
            return "invalid";
    }
}

bool xtransaction_t::set_tx_by_serialized_data(xtransaction_ptr_t & tx_ptr, const std::string & data) {
    base::xauto_ptr<base::xdataunit_t> raw_tx = base::xdataunit_t::read_from(data);
    if(nullptr == raw_tx) {
        xerror("xtransaction_t::set_tx_by_serialized_data fail-tx content read from fail.");
        return false;
    }

    auto tx = dynamic_cast<xtransaction_t*>(raw_tx.get());
    tx->add_ref();
    tx_ptr.attach(tx);

    // if (raw_tx->get_obj_type() == base::xdataunit_t::enum_xdata_type_max - xdata_type_transaction_v2) {
    //     xdbg("wish cluster v2, %d", raw_tx->get_obj_type());
    //     auto tx = dynamic_cast<xtransaction_v2_t*>(raw_tx.get());
    //     tx->add_ref();
    //     tx_ptr.attach(tx);
    // } else {
    //     xdbg("wish cluster v1, %d", raw_tx->get_obj_type());
    //     auto tx = dynamic_cast<xtransaction_v1_t*>(raw_tx.get());
    //     tx->add_ref();
    //     tx_ptr.attach(tx);
    // }    
    return true;
}

}  // namespace data
}  // namespace top
