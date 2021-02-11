#include "gtest/gtest.h"
#include "test_xtxpool_util.h"
#include "xchain_timer/xchain_timer.h"
#include "xconfig/xconfig_register.h"
#include "xcrypto/xcrypto_util.h"
#include "xdata/xgenesis_data.h"
#include "xdata/xtransaction_maker.hpp"
#include "xloader/xconfig_onchain_loader.h"
#include "xstore/xstore_face.h"
#include "xtxpool/xtxpool_face.h"
#include "xblockstore/xblockstore_face.h"
#include "xtestca.hpp"
#include "xdata/xblocktool.h"
#include "xverifier/xverifier_utl.h"
#include "xtxpool/xtxpool_error.h"
#include "xcertauth/xcertauth_face.h"
#include "xelection/xvnode_house.h"

using namespace top::xtxpool;
using namespace top::store;
using namespace top::data;
using namespace top;
using namespace top::base;
using namespace std;
using namespace top::test;
using namespace top::xverifier;

class test_xtxpool : public testing::Test {
protected:
    void SetUp() override {
        // m_store = xstore_factory::create_store_with_memdb();
        // auto mbus = new mbus::xmessage_bus_t();
        // auto chain_timer = std::make_shared<time::xchain_timer_t>();
        // auto& config_center = top::config::xconfig_register_t::get_instance();

        // config::xconfig_loader_ptr_t loader = std::make_shared<loader::xconfig_onchain_loader_t>(m_store, mbus, chain_timer.get());
        // config_center.add_loader(loader);
        // config_center.load();

        // auto& config_register = top::config::xconfig_register_t::get_instance();
        // config_register.set(xcold_account_unknown_sync_time_configuration_t::name, 0);
    }

    void TearDown() override {
        // auto& config_register = top::config::xconfig_register_t::get_instance();
        // config_register.set(xcold_account_unknown_sync_time_configuration_t::name, xcold_account_unknown_sync_time_configuration_t::value);
    }

public:
    xobject_ptr_t<xstore_face_t> m_store;
};

TEST_F(test_xtxpool, push_send_tx) {
    auto     mbus = std::make_shared<top::mbus::xmessage_bus_t>();
    auto     store_ptr = xstore_factory::create_store_with_memdb(nullptr);
    xobject_ptr_t<base::xvblockstore_t> blockstore;
    blockstore.attach(store::xblockstorehub_t::instance().create_block_store(*store_ptr, ""));
    xobject_ptr_t<base::xvcertauth_t> cert_ptr;
// #ifdef MOCK_CA
    cert_ptr = make_object_ptr<xschnorrcert_t>((uint32_t)1);
// #else
//     common::xnode_id_t node_id { "T00000LgGPqEpiK6XLCKRj9gVPN8Ej1aMbyAb3Hu" };
//     std::string sign_key = "ONhWC2LJtgi9vLUyoa48MF3tiXxqWf7jmT9KtOg/Lwo=";
//     xobject_ptr_t<base::xvnodesrv_t> nodesvr_ptr = make_object_ptr<top::election::xvnode_house_t>(node_id, sign_key, blockstore, make_observer(mbus));
//     cert_ptr.attach(&auth::xauthcontext_t::instance(*nodesvr_ptr.get()));
// #endif
    auto     xtxpool = xtxpool_instance::create_xtxpool_inst(make_observer(store_ptr), blockstore, make_observer(mbus.get()), cert_ptr);
    uint64_t now = xverifier::xtx_utl::get_gmttime_s();
    uint256_t last_tx_hash = {};

    std::string account1 = test_xtxpool_util_t::get_account(0);
    std::string account2 = test_xtxpool_util_t::get_account(1);
    std::string account3 = test_xtxpool_util_t::get_account(2);

    for (uint64_t i = 0; i < 4; i++) {
        {
            xcons_transaction_ptr_t cons_tx = test_xtxpool_util_t::create_cons_transfer_tx(0 , 1, i, now + i, last_tx_hash);
            auto ret = xtxpool->push_send_tx(cons_tx);
            ASSERT_EQ(0, ret);
            last_tx_hash = cons_tx->get_transaction()->digest();
        }
    }

    ASSERT_NE(nullptr, xtxpool->query_tx(account1, last_tx_hash));

    last_tx_hash = {};
    for (uint64_t i = 0; i < 4; i++) {
        {
            xcons_transaction_ptr_t cons_tx = test_xtxpool_util_t::create_cons_transfer_tx(1 , 2, i, now + i, last_tx_hash);
            int32_t ret = xtxpool->push_send_tx(cons_tx);
            xassert(ret == 0);
            last_tx_hash = cons_tx->get_transaction()->digest();
        }
    }
    ASSERT_NE(nullptr, xtxpool->find_account(account1));
    ASSERT_NE(nullptr, xtxpool->find_account(account2));
    ASSERT_EQ(nullptr, xtxpool->find_account(account3));
    ASSERT_NE(nullptr, xtxpool->query_tx(account2, last_tx_hash));


    last_tx_hash = {};
    for (uint64_t i = 0; i < 4; i++) {
        {
            xcons_transaction_ptr_t cons_tx = test_xtxpool_util_t::create_cons_transfer_tx(1 , 2, i, now + i, last_tx_hash);
            xtxpool->pop_tx_by_hash(account2, cons_tx->get_transaction()->digest(), enum_transaction_subtype_send, 0);
            last_tx_hash = cons_tx->get_transaction()->digest();
        }
    }
    ASSERT_NE(true, xtxpool->find_account(account1)->empty());
    ASSERT_EQ(true, xtxpool->find_account(account2)->empty());
    ASSERT_EQ(nullptr, xtxpool->query_tx(account2, last_tx_hash));
}

TEST_F(test_xtxpool, push_send_tx_ultralimit) {
    auto const send_tx_queue_max_num = XGET_CONFIG(account_send_queue_tx_max_num);
    top::config::config_register.set(config::xaccount_send_queue_tx_max_num_configuration_t::name, 10000);
    auto     mbus = std::make_shared<top::mbus::xmessage_bus_t>();
    auto     store_ptr = xstore_factory::create_store_with_memdb(nullptr);
    xobject_ptr_t<base::xvblockstore_t> blockstore;
    blockstore.attach(store::xblockstorehub_t::instance().create_block_store(*store_ptr, ""));
    xobject_ptr_t<base::xvcertauth_t> cert_ptr;
// #ifdef MOCK_CA
    cert_ptr = make_object_ptr<xschnorrcert_t>((uint32_t)1);
// #else
//     common::xnode_id_t node_id { "T00000LgGPqEpiK6XLCKRj9gVPN8Ej1aMbyAb3Hu" };
//     std::string sign_key = "ONhWC2LJtgi9vLUyoa48MF3tiXxqWf7jmT9KtOg/Lwo=";
//     xobject_ptr_t<base::xvnodesrv_t> nodesvr_ptr = make_object_ptr<top::election::xvnode_house_t>(node_id, sign_key, blockstore, make_observer(mbus));
//     cert_ptr.attach(&auth::xauthcontext_t::instance(*nodesvr_ptr.get()));
// #endif
    auto     xtxpool = xtxpool_instance::create_xtxpool_inst(make_observer(store_ptr), blockstore, make_observer(mbus.get()), cert_ptr);
    // std::vector<xcons_transaction_ptr_t> txs = test_xtxpool_util_t::create_cons_transfer_txs(0 , 1, 8194);

    std::vector<xcons_transaction_ptr_t> txs;
    uint64_t                             now = xverifier::xtx_utl::get_gmttime_s();
    uint256_t                            last_tx_hash = {};
    uint64_t                             last_tx_nonce = 0;
    for (uint32_t i = 0; i < 514; i++) {
        xcons_transaction_ptr_t tx = test_xtxpool_util_t::create_cons_transfer_tx(0, 2, last_tx_nonce, now + i/1000, last_tx_hash);
        last_tx_hash = tx->get_transaction()->digest();
        last_tx_nonce = tx->get_transaction()->get_tx_nonce();
        txs.push_back(tx);
    }

    int32_t ret = 0;
    int32_t table_max_send_tx_num = 8;
    for (int32_t i = 0; i < table_max_send_tx_num; i++) {
        ret = xtxpool->push_send_tx(txs[i]);
        ASSERT_EQ(ret, 0);
    }
    ret = xtxpool->push_send_tx(txs[table_max_send_tx_num]);
    ASSERT_EQ(ret, xtxpool_error_send_tx_reach_upper_limit);
    ret = xtxpool->push_send_tx(txs[table_max_send_tx_num+1]);
    ASSERT_EQ(ret, xtxpool_error_send_tx_reach_upper_limit);

    xtxpool->pop_tx_by_hash(txs[table_max_send_tx_num-1]->get_source_addr(), txs[table_max_send_tx_num-1]->get_transaction()->digest(), enum_transaction_subtype_send, 0);
    ret = xtxpool->push_send_tx(txs[table_max_send_tx_num-1]);
    ASSERT_EQ(ret, 0);
    ret = xtxpool->push_send_tx(txs[table_max_send_tx_num]);
    ASSERT_EQ(ret, xtxpool_error_send_tx_reach_upper_limit);
    ret = xtxpool->push_send_tx(txs[table_max_send_tx_num+1]);
    ASSERT_EQ(ret, xtxpool_error_send_tx_reach_upper_limit);

    top::config::config_register.set(config::xaccount_send_queue_tx_max_num_configuration_t::name, send_tx_queue_max_num);
}

TEST_F(test_xtxpool, verify_send_tx) {
    auto     mbus = std::make_shared<top::mbus::xmessage_bus_t>();
    auto     store_ptr = xstore_factory::create_store_with_memdb(nullptr);
    xobject_ptr_t<base::xvblockstore_t> blockstore;
    blockstore.attach(store::xblockstorehub_t::instance().create_block_store(*store_ptr, ""));
    xobject_ptr_t<base::xvcertauth_t> cert_ptr;
// #ifdef MOCK_CA
    cert_ptr = make_object_ptr<xschnorrcert_t>((uint32_t)1);
// #else
//     common::xnode_id_t node_id { "T00000LgGPqEpiK6XLCKRj9gVPN8Ej1aMbyAb3Hu" };
//     std::string sign_key = "ONhWC2LJtgi9vLUyoa48MF3tiXxqWf7jmT9KtOg/Lwo=";
//     xobject_ptr_t<base::xvnodesrv_t> nodesvr_ptr = make_object_ptr<top::election::xvnode_house_t>(node_id, sign_key, blockstore, make_observer(mbus));
//     cert_ptr.attach(&auth::xauthcontext_t::instance(*nodesvr_ptr.get()));
// #endif
    auto     xtxpool = xtxpool_instance::create_xtxpool_inst(make_observer(store_ptr), blockstore, make_observer(mbus.get()), cert_ptr);
    uint64_t now = xverifier::xtx_utl::get_gmttime_s();
    uint256_t last_tx_hash = {};

    std::string account1 = test_xtxpool_util_t::get_account(0);
    std::string account2 = test_xtxpool_util_t::get_account(1);

    std::vector<xcons_transaction_ptr_t> txs = test_xtxpool_util_t::create_cons_transfer_txs(0 , 1, 1);
    auto ret = xtxpool->verify_txs(account1, 0, 0, txs, 0, {});
    ASSERT_EQ(ret, 0);
}

TEST_F(test_xtxpool, push_send_tx_invalid_tx_hash) {
    auto     mbus = std::make_shared<top::mbus::xmessage_bus_t>();
    auto     store_ptr = xstore_factory::create_store_with_memdb(nullptr);
    xobject_ptr_t<base::xvblockstore_t> blockstore;
    blockstore.attach(store::xblockstorehub_t::instance().create_block_store(*store_ptr, ""));
    xobject_ptr_t<base::xvcertauth_t> cert_ptr;
// #ifdef MOCK_CA
    cert_ptr = make_object_ptr<xschnorrcert_t>((uint32_t)1);
// #else
//     common::xnode_id_t node_id { "T00000LgGPqEpiK6XLCKRj9gVPN8Ej1aMbyAb3Hu" };
//     std::string sign_key = "ONhWC2LJtgi9vLUyoa48MF3tiXxqWf7jmT9KtOg/Lwo=";
//     xobject_ptr_t<base::xvnodesrv_t> nodesvr_ptr = make_object_ptr<top::election::xvnode_house_t>(node_id, sign_key, blockstore, make_observer(mbus));
//     cert_ptr.attach(&auth::xauthcontext_t::instance(*nodesvr_ptr.get()));
// #endif
    auto     xtxpool = xtxpool_instance::create_xtxpool_inst(make_observer(store_ptr), blockstore, make_observer(mbus.get()), cert_ptr);
    uint64_t now = xverifier::xtx_utl::get_gmttime_s();
    uint256_t last_tx_hash = {};

    std::string account1 = test_xtxpool_util_t::get_account(0);
    std::string account2 = test_xtxpool_util_t::get_account(1);

    xcons_transaction_ptr_t cons_tx = test_xtxpool_util_t::create_cons_transfer_tx(0 , 1, 0, now, last_tx_hash);
    cons_tx->get_transaction()->set_expire_duration(12345);
    auto ret = xtxpool->push_send_tx(cons_tx);
    ASSERT_EQ(xverifier::xverifier_error::xverifier_error_tx_hash_invalid, ret);
}

TEST_F(test_xtxpool, push_send_tx_no_tx_sign) {
    auto     mbus = std::make_shared<top::mbus::xmessage_bus_t>();
    auto     store_ptr = xstore_factory::create_store_with_memdb(nullptr);
    xobject_ptr_t<base::xvblockstore_t> blockstore;
    blockstore.attach(store::xblockstorehub_t::instance().create_block_store(*store_ptr, ""));
    xobject_ptr_t<base::xvcertauth_t> cert_ptr;
// #ifdef MOCK_CA
    cert_ptr = make_object_ptr<xschnorrcert_t>((uint32_t)1);
// #else
//     common::xnode_id_t node_id { "T00000LgGPqEpiK6XLCKRj9gVPN8Ej1aMbyAb3Hu" };
//     std::string sign_key = "ONhWC2LJtgi9vLUyoa48MF3tiXxqWf7jmT9KtOg/Lwo=";
//     xobject_ptr_t<base::xvnodesrv_t> nodesvr_ptr = make_object_ptr<top::election::xvnode_house_t>(node_id, sign_key, blockstore, make_observer(mbus));
//     cert_ptr.attach(&auth::xauthcontext_t::instance(*nodesvr_ptr.get()));
// #endif
    auto     xtxpool = xtxpool_instance::create_xtxpool_inst(make_observer(store_ptr), blockstore, make_observer(mbus.get()), cert_ptr);
    uint64_t now = xverifier::xtx_utl::get_gmttime_s();
    uint256_t last_tx_hash = {};

    std::string account1 = test_xtxpool_util_t::get_account(0);
    std::string account2 = test_xtxpool_util_t::get_account(1);

    xcons_transaction_ptr_t cons_tx = test_xtxpool_util_t::create_cons_transfer_tx(0 , 1, 0, now, last_tx_hash, 100, 100, false);
    auto ret = xtxpool->push_send_tx(cons_tx);
    ASSERT_EQ(xverifier::xverifier_error::xverifier_error_tx_signature_invalid, ret);
}

TEST_F(test_xtxpool, get_send_tx) {
    // auto mbus = std::make_shared<top::mbus::xmessage_bus_t>();
    // auto store_ptr = store::xstore_factory::create_store_with_memdb(nullptr);
    // uint64_t now = xverifier::xtx_utl::get_gmttime_s();
    // std::string sender = top::utl::xcrypto_util::make_address_by_random_key((uint8_t)enum_addr_type_t::alone_account, 0);
    // std::string receiver = top::utl::xcrypto_util::make_address_by_random_key((uint8_t)enum_addr_type_t::alone_account, 0);

    // ASSERT_TRUE(xblock_maker::create_genesis_account(store_ptr, sender, ASSET_TOP(10000)));

    // xcons_transaction_ptr_t cons_tx = test_xtxpool_util_t::create_cons_transfer_tx(sender, receiver, 0, now);

    // auto xtxpool = xtxpool_instance::create_xtxpool_inst(store_ptr, mbus);
    // xtxpool->push_send_tx(enum_xtxpool_table_type_shard, cons_tx);

    // auto tableid = data::account_map_to_table_id(sender);
    // uint32_t  target_table_id = tableid.table_id;
    // std::map<std::string, std::vector<xcons_transaction_ptr_t>> cons_txs = xtxpool->get_txs(enum_xtxpool_table_type_shard, target_table_id, 0);
    // ASSERT_EQ(cons_txs.size(), 1);

    // push_recv_tx(const xtransaction_ptr_t & tx, const data::xreceipt_unit_ptr_t & receipt);
}

TEST_F(test_xtxpool, receipt_resend_method) {
    uint64_t cert_time = xverifier::xtx_utl::get_gmttime_s();
    ASSERT_EQ(get_receipt_resend_time(cert_time, cert_time - 1), 0);
    ASSERT_EQ(get_receipt_resend_time(cert_time, cert_time), 0);
    ASSERT_EQ(get_receipt_resend_time(cert_time, cert_time + 1), 0);
    ASSERT_EQ(get_receipt_resend_time(cert_time, cert_time + send_tx_receipt_first_retry_timeout - 1), 0);
    ASSERT_EQ(get_receipt_resend_time(cert_time, cert_time + send_tx_receipt_first_retry_timeout), 1);
    ASSERT_EQ(get_receipt_resend_time(cert_time, cert_time + send_tx_receipt_first_retry_timeout + 1), 1);
    ASSERT_EQ(get_receipt_resend_time(cert_time, cert_time + send_tx_receipt_first_retry_timeout + send_tx_receipt_common_retry_timeout - 1), 1);
    ASSERT_EQ(get_receipt_resend_time(cert_time, cert_time + send_tx_receipt_first_retry_timeout + send_tx_receipt_common_retry_timeout), 2);
    ASSERT_EQ(get_receipt_resend_time(cert_time, cert_time + send_tx_receipt_first_retry_timeout + send_tx_receipt_common_retry_timeout + 1), 2);
    uint64_t delaycount = random()%1000;
    ASSERT_EQ(get_receipt_resend_time(cert_time, cert_time + send_tx_receipt_first_retry_timeout + delaycount * send_tx_receipt_common_retry_timeout - 1), delaycount);
    ASSERT_EQ(get_receipt_resend_time(cert_time, cert_time + send_tx_receipt_first_retry_timeout + delaycount * send_tx_receipt_common_retry_timeout), delaycount + 1);
    ASSERT_EQ(get_receipt_resend_time(cert_time, cert_time + send_tx_receipt_first_retry_timeout + delaycount * send_tx_receipt_common_retry_timeout + 1), delaycount + 1);

    
    ASSERT_EQ(get_next_retry_timestamp(cert_time, cert_time - 1), cert_time + send_tx_receipt_first_retry_timeout);
    ASSERT_EQ(get_next_retry_timestamp(cert_time, cert_time), cert_time + send_tx_receipt_first_retry_timeout);
    ASSERT_EQ(get_next_retry_timestamp(cert_time, cert_time + 1), cert_time + send_tx_receipt_first_retry_timeout);
    ASSERT_EQ(get_next_retry_timestamp(cert_time, cert_time + send_tx_receipt_first_retry_timeout - 1), cert_time + send_tx_receipt_first_retry_timeout);
    ASSERT_EQ(get_next_retry_timestamp(cert_time, cert_time + send_tx_receipt_first_retry_timeout), cert_time + send_tx_receipt_first_retry_timeout + send_tx_receipt_common_retry_timeout);
    ASSERT_EQ(get_next_retry_timestamp(cert_time, cert_time + send_tx_receipt_first_retry_timeout + 1), cert_time + send_tx_receipt_first_retry_timeout + send_tx_receipt_common_retry_timeout);
    ASSERT_EQ(get_next_retry_timestamp(cert_time, cert_time + send_tx_receipt_first_retry_timeout + send_tx_receipt_common_retry_timeout - 1), cert_time + send_tx_receipt_first_retry_timeout + send_tx_receipt_common_retry_timeout);
    ASSERT_EQ(get_next_retry_timestamp(cert_time, cert_time + send_tx_receipt_first_retry_timeout + send_tx_receipt_common_retry_timeout), cert_time + send_tx_receipt_first_retry_timeout + 2*send_tx_receipt_common_retry_timeout);
    ASSERT_EQ(get_next_retry_timestamp(cert_time, cert_time + send_tx_receipt_first_retry_timeout + send_tx_receipt_common_retry_timeout + 1), cert_time + send_tx_receipt_first_retry_timeout + 2*send_tx_receipt_common_retry_timeout);
    ASSERT_EQ(get_next_retry_timestamp(cert_time, cert_time + send_tx_receipt_first_retry_timeout + delaycount * send_tx_receipt_common_retry_timeout - 1), cert_time + send_tx_receipt_first_retry_timeout + delaycount*send_tx_receipt_common_retry_timeout);
    ASSERT_EQ(get_next_retry_timestamp(cert_time, cert_time + send_tx_receipt_first_retry_timeout + delaycount * send_tx_receipt_common_retry_timeout), cert_time + send_tx_receipt_first_retry_timeout + (delaycount + 1)*send_tx_receipt_common_retry_timeout);
    ASSERT_EQ(get_next_retry_timestamp(cert_time, cert_time + send_tx_receipt_first_retry_timeout + delaycount * send_tx_receipt_common_retry_timeout + 1), cert_time + send_tx_receipt_first_retry_timeout + (delaycount + 1)*send_tx_receipt_common_retry_timeout);
}
