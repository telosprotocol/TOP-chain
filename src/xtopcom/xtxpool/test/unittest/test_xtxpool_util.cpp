#include "test_xtxpool_util.h"

#include "gtest/gtest.h"
#include "xchain_timer/xchain_timer.h"
#include "xconfig/xconfig_register.h"
#include "xcrypto/xcrypto_util.h"
#include "xdata/tests/test_blockutl.hpp"
#include "xdata/xblocktool.h"
#include "xdata/xgenesis_data.h"
#include "xdata/xtransaction_maker.hpp"
#include "xloader/xconfig_onchain_loader.h"
#include "xtxpool/xtxpool_face.h"

using namespace top::xtxpool;
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
    xtransaction_ptr_t    tx = make_object_ptr<xtransaction_t>();
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
        tx->set_signature(signature);
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
    xtransaction_ptr_t    tx = make_object_ptr<xtransaction_t>();
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

xblock_t * test_xtxpool_util_t::create_unit_with_cons_txs(xstore_face_t * store, const std::string & address, const std::vector<xcons_transaction_ptr_t> & txs) {
    auto account = store->clone_account(address);
    base::xvblock_t* latest_block = nullptr;
    if (account == nullptr) {
        latest_block = data::xblocktool_t::create_genesis_empty_unit(address);
        store->set_vblock(latest_block);
        store->execute_block(latest_block);
        account = store->clone_account(address);
    } else {
        latest_block = store->get_block_by_height(address, account->get_chain_height());
    }

    int64_t amount = -100 * txs.size();

    xlightunit_block_para_t para;
    para.set_input_txs(txs);
    para.set_balance_change(amount);
    para.set_account_unconfirm_sendtx_num(account->get_unconfirm_sendtx_num());

    base::xvblock_t *          proposal_block = test_blocktuil::create_next_lightunit_with_consensus(para, latest_block);
    auto                       block = dynamic_cast<data::xblock_t *>(proposal_block);
    data::xlightunit_block_t * lightunit = dynamic_cast<data::xlightunit_block_t *>(block);
    xinfo("create_unit_with_cons_txs: lightunit->is_prev_sendtx_confirmed(): %d", lightunit->is_prev_sendtx_confirmed());

    std::vector<xcons_transaction_ptr_t> sendtx_receipts;
    std::vector<xcons_transaction_ptr_t> recvtx_receipts;
    lightunit->create_txreceipts(sendtx_receipts, recvtx_receipts);
    xinfo("create_unit_with_cons_txs: sendtx_receipts size:%d recvtx_receipts size:%d", sendtx_receipts.size(), recvtx_receipts.size());

    store->set_vblock(block);
    store->execute_block(block);

    return block;
}

xblock_t * test_xtxpool_util_t::create_unit_with_contract_create_tx(xstore_face_t * store, const std::string & address, const std::vector<xcons_transaction_ptr_t> & txs, const std::string & receiver) {
    auto account = store->clone_account(address);
    base::xvblock_t* latest_block = nullptr;
    if (account == nullptr) {
        latest_block = data::xblocktool_t::create_genesis_empty_unit(address);
        store->set_vblock(latest_block);
        store->execute_block(latest_block);
        account = store->clone_account(address);
    } else {
        latest_block = store->get_block_by_height(address, account->get_chain_height());
    }

    int64_t amount = -100 * txs.size();

    xtransaction_ptr_t contract_tx = account->make_transfer_tx(receiver, amount, txs[0]->get_transaction()->get_fire_timestamp(), 0, 1000000);
    xcons_transaction_ptr_t constx_contract_tx = make_object_ptr<xcons_transaction_t>(contract_tx.get());
    std::vector<xcons_transaction_ptr_t> contract_txs;
    contract_txs.push_back(constx_contract_tx);

    xlightunit_block_para_t para;
    para.set_input_txs(txs);
    para.set_balance_change(amount);
    para.set_account_unconfirm_sendtx_num(account->get_unconfirm_sendtx_num());
    para.set_contract_txs(contract_txs);

    base::xvblock_t *          proposal_block = test_blocktuil::create_next_lightunit_with_consensus(para, latest_block);
    auto                       block = dynamic_cast<data::xblock_t *>(proposal_block);

    store->set_vblock(block);
    store->execute_block(block);

    return block;
}

void test_xtxpool_util_t::create_genesis_account(xstore_face_t * store, const std::string & address) {
    auto account = store->clone_account(address);
    if (account == nullptr) {
        base::xvblock_t * genesis_block = data::xblocktool_t::create_genesis_lightunit(address, 1000000);
        store->set_vblock(genesis_block);
        // account = store->clone_account(address);
    }

    // std::vector<xcons_transaction_ptr_t> txs = create_cons_transfer_txs(address, address, 1);

    // xlightunit_block_para_t para;
    // para.set_input_txs(txs);
    // para.set_balance_change(100000000);
    // para.set_account_unconfirm_sendtx_num(account->get_unconfirm_sendtx_num());

    // base::xvblock_t* proposal_block = data::xlightunit_block_t::create_next_lightunit(para, account);
    // auto block = dynamic_cast<data::xblock_t*>(proposal_block);
    // store->set_vblock(block);
}

xblock_t * test_xtxpool_util_t::create_tableblock_with_cons_txs(xstore_face_t *                              store,
                                                                const std::string &                          address,
                                                                base::xvblock_t *                            unit_genesis_block,
                                                                const std::vector<xcons_transaction_ptr_t> & txs,
                                                                uint64_t                                     tx_clock,
                                                                uint64_t                                     table_clock) {
    // base::xvblock_t* genesis_block = data::xblocktool_t::create_genesis_empty_unit(address);

    xlightunit_block_para_t para;
    para.set_input_txs(txs);
    xblock_t * unit = (xblock_t *)test_blocktuil::create_next_lightunit(para, unit_genesis_block, tx_clock);

    data::xlightunit_block_t *           lightunit = dynamic_cast<data::xlightunit_block_t *>(unit);
    std::vector<xcons_transaction_ptr_t> sendtx_receipts;
    std::vector<xcons_transaction_ptr_t> recvtx_receipts;
    lightunit->create_txreceipts(sendtx_receipts, recvtx_receipts);
    assert(!sendtx_receipts.empty());

    base::xvblock_t *       genesis_block = xblocktool_t::create_genesis_empty_unit(address);
    xlightunit_block_para_t para1;
    para1.set_input_txs(sendtx_receipts);
    xblock_t * unit1 = (xblock_t *)test_blocktuil::create_next_lightunit(para1, genesis_block);

    xtable_block_para_t table_para;
    table_para.add_unit(unit1);

    std::string       taccount1 = xblocktool_t::make_address_shard_table_account(100);
    base::xvblock_t * taccount1_genesis_block = xblocktool_t::create_genesis_empty_table(taccount1);
    // store->set_vblock(taccount1_genesis_block);
    xblock_t * taccount1_proposal_block = (xblock_t *)test_blocktuil::create_next_tableblock(table_para, taccount1_genesis_block, table_clock);
    assert(!taccount1_proposal_block->get_block_hash().empty());
    store->set_vblock(taccount1_proposal_block);
    return taccount1_proposal_block;
}

xblock_t * test_xtxpool_util_t::create_tableblock_with_cons_txs_with_next_two_emptyblock(xstore_face_t *                              store,
                                                                const std::string &                          address,
                                                                base::xvblock_t *                            unit_genesis_block,
                                                                const std::vector<xcons_transaction_ptr_t> & txs,
                                                                uint64_t                                     tx_clock,
                                                                uint64_t                                     table_clock) {
    // base::xvblock_t* genesis_block = data::xblocktool_t::create_genesis_empty_unit(address);

    xlightunit_block_para_t para;
    para.set_input_txs(txs);
    xblock_t * unit = (xblock_t *)test_blocktuil::create_next_lightunit_with_consensus(para, unit_genesis_block, tx_clock);

    data::xlightunit_block_t *           lightunit = dynamic_cast<data::xlightunit_block_t *>(unit);
    std::vector<xcons_transaction_ptr_t> sendtx_receipts;
    std::vector<xcons_transaction_ptr_t> recvtx_receipts;
    lightunit->create_txreceipts(sendtx_receipts, recvtx_receipts);
    assert(!sendtx_receipts.empty());
    for (auto iter : sendtx_receipts) {
        iter->set_commit_prove_with_parent_cert(lightunit->get_cert());
    }

    base::xvblock_t *       genesis_block = xblocktool_t::create_genesis_empty_unit(address);
    xlightunit_block_para_t para1;
    para1.set_input_txs(sendtx_receipts);
    xblock_t * unit1 = (xblock_t *)test_blocktuil::create_next_lightunit(para1, genesis_block);

    xtable_block_para_t table_para;
    table_para.add_unit(unit1);

    std::string       taccount1 = xblocktool_t::make_address_shard_table_account(100);
    base::xvblock_t * taccount1_genesis_block = xblocktool_t::create_genesis_empty_table(taccount1);
    // store->set_vblock(taccount1_genesis_block);
    xblock_t * taccount1_proposal_block = (xblock_t *)test_blocktuil::create_next_tableblock_with_next_two_emptyblock(table_para, taccount1_genesis_block, table_clock);
    assert(!taccount1_proposal_block->get_block_hash().empty());
    store->set_vblock(taccount1_proposal_block);
    return taccount1_proposal_block;
}

void test_xtxpool_util_t::table_block_set_to_store(xstore_face_t * store, xblock_t * table_block) {
    store->set_vblock(table_block);
}
