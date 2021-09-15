#include "test_xtxpool_util.h"

#include "gtest/gtest.h"
#include "xchain_timer/xchain_timer.h"
#include "xconfig/xconfig_register.h"
#include "xcrypto/xckey.h"
#include "xcrypto/xcrypto_util.h"
#include "xdata/xblocktool.h"
#include "xdata/xgenesis_data.h"
#include "xdata/xtransaction_maker.hpp"
#include "xdata/xtransaction_v2.h"
#include "xloader/xconfig_onchain_loader.h"
#include "xtxpool_v2/xtxpool_face.h"
#include "xverifier/xverifier_utl.h"

using namespace top::xtxpool_v2;
using namespace top::store;
using namespace top::data;
using namespace top;
using namespace top::base;
using namespace std;


std::string test_xtxpool_util_t::get_account(uint32_t account_idx) {
    uint8_t private_keys[3][32] = {{27,18,108,127,186,103,109,218,68,69,90,233,151,194,76,151,88,204,139,220,154,238,167,25,107,91,35,49,72,28,79,22},
                                   {67,140,104,117,163,223,201,6,151,33,211,97,53,151,30,18,119,244,191,175,62,222,85,188,249,68,17,75,86,76,75,63},
                                   {219,79,108,49,38,203,159,34,218,86,203,107,191,126,132,121,100,100,65,100,178,64,144,148,205,69,2,8,236,166,0,72}};

    return top::utl::xcrypto_util::make_address_by_assigned_key(private_keys[account_idx], '0', 0) + "@0";
}
xcons_transaction_ptr_t test_xtxpool_util_t::create_cons_transfer_tx(uint32_t            sender_idx,
                                                                     uint32_t            receiver_idx,
                                                                     uint64_t            nonce,
                                                                     uint64_t            timestamp,
                                                                     uint256_t           last_hash,
                                                                     uint16_t            expire_duration,
                                                                     uint64_t            amount,
                                                                     bool                sign) {
    uint8_t private_keys[3][32] = {{27,18,108,127,186,103,109,218,68,69,90,233,151,194,76,151,88,204,139,220,154,238,167,25,107,91,35,49,72,28,79,22},
                                   {67,140,104,117,163,223,201,6,151,33,211,97,53,151,30,18,119,244,191,175,62,222,85,188,249,68,17,75,86,76,75,63},
                                   {219,79,108,49,38,203,159,34,218,86,203,107,191,126,132,121,100,100,65,100,178,64,144,148,205,69,2,8,236,166,0,72}};
    xtransaction_ptr_t    tx = make_object_ptr<xtransaction_v2_t>();
    data::xproperty_asset asset(amount);
    tx->make_tx_transfer(asset);
    std::string from = top::utl::xcrypto_util::make_address_by_assigned_key(private_keys[sender_idx], '0', 0) + "@0";
    std::string to = top::utl::xcrypto_util::make_address_by_assigned_key(private_keys[receiver_idx], '0', 0) + "@0";
    tx->set_different_source_target_address(from, to);
    tx->set_last_trans_hash_and_nonce(last_hash, nonce);
    tx->set_fire_timestamp(timestamp);
    tx->set_expire_duration(expire_duration);
    tx->set_digest();
    if (sign) {
        utl::xecprikey_t pri_key_obj(private_keys[sender_idx]);
        utl::xecdsasig_t signature_obj = pri_key_obj.sign(tx->digest());
        auto signature = std::string(reinterpret_cast<char *>(signature_obj.get_compact_signature()), signature_obj.get_compact_signature_size());
        tx->set_authorization(signature);
    }

    tx->set_len();

    xcons_transaction_ptr_t cons_tx = make_object_ptr<xcons_transaction_t>(tx.get());
    return cons_tx;
}

xcons_transaction_ptr_t test_xtxpool_util_t::create_cons_transfer_tx(const std::string & from,
                                                                     const std::string & to,
                                                                     uint64_t            nonce,
                                                                     uint64_t            timestamp,
                                                                     uint256_t           last_hash,
                                                                     uint16_t            expire_duration,
                                                                     uint64_t            amount) {
    xtransaction_ptr_t    tx = make_object_ptr<xtransaction_v2_t>();
    data::xproperty_asset asset(amount);
    tx->make_tx_transfer(asset);
    tx->set_different_source_target_address(from, to);
    tx->set_last_trans_hash_and_nonce(last_hash, nonce);
    tx->set_fire_timestamp(timestamp);
    tx->set_expire_duration(expire_duration);
    tx->set_digest();

    xcons_transaction_ptr_t cons_tx = make_object_ptr<xcons_transaction_t>(tx.get());
    return cons_tx;
}

std::vector<xcons_transaction_ptr_t> test_xtxpool_util_t::create_cons_transfer_txs(uint32_t sender_idx, uint32_t receiver_idx, uint32_t count) {
    std::vector<xcons_transaction_ptr_t> txs;
    uint64_t                             now = xverifier::xtx_utl::get_gmttime_s();
    uint256_t                            last_tx_hash = {};
    uint64_t                             last_tx_nonce = 0;
    for (uint32_t i = 0; i < count; i++) {
        xcons_transaction_ptr_t tx = create_cons_transfer_tx(sender_idx, receiver_idx, last_tx_nonce, now + 1 + i, last_tx_hash);
        last_tx_hash = tx->get_transaction()->digest();
        last_tx_nonce = tx->get_transaction()->get_tx_nonce();
        txs.push_back(tx);
    }
    return txs;
}
