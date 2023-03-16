#include "gtest/gtest.h"
#define private public
#define protected public
#include "test_xtxpool_util.h"
#include "tests/mock/xdatamock_table.hpp"
#include "tests/mock/xvchain_creator.hpp"
#include "xtxpool_v2/xtxpool.h"
#include "xtxpool_v2/xtxpool_para.h"
#include "xtxpool_v2/xtxpool_table.h"

using namespace top::xtxpool_v2;
using namespace top::data;
using namespace top;
using namespace top::base;
using namespace std;
using namespace top::utl;

class test_xtxpool : public testing::Test {
protected:
    void SetUp() override {
    }

    void TearDown() override {
    }
};

void * sub_unsub_thread(void * arg) {
    xtxpool_t * xtxpool = (xtxpool_t *)arg;
    for (uint32_t i = 0; i < 1000; i++) {
        xtxpool->unsubscribe_tables(2, 0, 0, common::xnode_type_t::consensus_auditor);
        xtxpool->unsubscribe_tables(1, 0, 0, common::xnode_type_t::consensus_auditor);
        xtxpool->unsubscribe_tables(0, 0, 0, common::xnode_type_t::consensus_auditor);
    }

    return nullptr;
}

TEST_F(test_xtxpool, sub_unsub) {
    mock::xvchain_creator creator;
    creator.create_blockstore_with_xstore();
    base::xvblockstore_t * blockstore = creator.get_blockstore();
    store::xstore_face_t * xstore = creator.get_xstore();
    auto para = std::make_shared<xtxpool_resources>(make_observer(blockstore), nullptr, nullptr);
    xtxpool_t xtxpool(para);

    pthread_t tid;
    pthread_create(&tid, NULL, sub_unsub_thread, &xtxpool);

    pthread_join(tid, NULL);
}

TEST_F(test_xtxpool, black_white_list) {
    mock::xvchain_creator creator;
    base::xvblockstore_t * blockstore = creator.get_blockstore();

    mock::xdatamock_table mocktable(1, 6);
    auto & table_addr = mocktable.get_address();
    auto table_sid = mocktable.get_short_table_id();
    std::vector<std::string> unit_addrs = mocktable.get_unit_accounts(); 
    std::string addr0 = unit_addrs[0];
    std::string addr1 = unit_addrs[1];
    std::string addr2 = unit_addrs[2];
    std::string addr3 = unit_addrs[3];
    std::string addr4 = unit_addrs[4];
    std::string addr5 = unit_addrs[5];

    std::vector<xcons_transaction_ptr_t> send_txs_0_1 = mocktable.create_send_txs(addr0, addr1, 1);
    std::vector<xcons_transaction_ptr_t> send_txs_1_2 = mocktable.create_send_txs(addr1, addr2, 1);
    std::vector<xcons_transaction_ptr_t> send_txs_2_3 = mocktable.create_send_txs(addr2, addr3, 1);
    std::vector<xcons_transaction_ptr_t> send_txs_3_4 = mocktable.create_send_txs(addr3, addr4, 1);
    std::vector<xcons_transaction_ptr_t> send_txs_4_5 = mocktable.create_send_txs(addr4, addr5, 1);

    std::vector<xcons_transaction_ptr_t> send_txs;
    send_txs.insert(send_txs.end(), send_txs_0_1.begin(), send_txs_0_1.end());
    send_txs.insert(send_txs.end(), send_txs_1_2.begin(), send_txs_1_2.end());
    send_txs.insert(send_txs.end(), send_txs_2_3.begin(), send_txs_2_3.end());
    send_txs.insert(send_txs.end(), send_txs_3_4.begin(), send_txs_3_4.end());
    send_txs.insert(send_txs.end(), send_txs_4_5.begin(), send_txs_4_5.end());

    std::string empty_config;
    XSET_ONCHAIN_GOVERNANCE_PARAMETER(whitelist, empty_config);
    XSET_CONFIG(local_whitelist, empty_config);
    XSET_ONCHAIN_GOVERNANCE_PARAMETER(toggle_whitelist, false);
    XSET_CONFIG(local_toggle_whitelist, false);
    XSET_ONCHAIN_GOVERNANCE_PARAMETER(blacklist, empty_config);
    XSET_CONFIG(local_blacklist, empty_config);

    xtxpool_role_info_t shard(0, 0, 0, common::xnode_type_t::consensus_auditor);
    xtxpool_statistic_t statistic;
    xtable_state_cache_t table_state_cache(nullptr, table_addr);
    xtxpool_table_info_t table_para(table_addr, &shard, &statistic, &table_state_cache);
    xtxpool_resources resource(nullptr, nullptr, nullptr);
    xtxpool_table_t txpool_table(&resource, table_addr, &shard, &statistic);

    // case 1: white list not enable, white list empty, black list empty
    std::vector<xcons_transaction_ptr_t> send_txs_test(send_txs);
    txpool_table.filter_txs_by_black_white_list(send_txs_test);
    xassert(send_txs_test.size() == send_txs.size());

    // case 2: white list enable, white list empty, black list empty
    XSET_ONCHAIN_GOVERNANCE_PARAMETER(toggle_whitelist, true);

    txpool_table.filter_txs_by_black_white_list(send_txs_test);
    xassert(send_txs_test.empty());

    // case 3: white list enable, white list not emtpy, black list empty
    XSET_ONCHAIN_GOVERNANCE_PARAMETER(whitelist, addr0);
    send_txs_test = send_txs;
    txpool_table.filter_txs_by_black_white_list(send_txs_test);
    xassert(send_txs_test.size() == 1);
    xassert(send_txs_test[0]->get_tx_hash_256() == send_txs_0_1[0]->get_tx_hash_256());

    // case 4: white list enable, white list not emtpy, black list not empty
    XSET_ONCHAIN_GOVERNANCE_PARAMETER(whitelist, addr0 + "," + addr1);
    XSET_ONCHAIN_GOVERNANCE_PARAMETER(blacklist, addr0);
    send_txs_test = send_txs;
    txpool_table.filter_txs_by_black_white_list(send_txs_test);
    xassert(send_txs_test.size() == 1);
    xassert(send_txs_test[0]->get_tx_hash_256() == send_txs_1_2[0]->get_tx_hash_256());

    // case 5: white list not enable, white list emtpy, black list not empty
    XSET_ONCHAIN_GOVERNANCE_PARAMETER(toggle_whitelist, false);
    XSET_ONCHAIN_GOVERNANCE_PARAMETER(whitelist, empty_config);
    XSET_ONCHAIN_GOVERNANCE_PARAMETER(blacklist, addr0);
    send_txs_test = send_txs;
    txpool_table.filter_txs_by_black_white_list(send_txs_test);
    xassert(send_txs_test.size() == send_txs.size() - 1);
    xassert(send_txs_test[0]->get_tx_hash_256() == send_txs_1_2[0]->get_tx_hash_256());
    xassert(send_txs_test[1]->get_tx_hash_256() == send_txs_2_3[0]->get_tx_hash_256());
    xassert(send_txs_test[2]->get_tx_hash_256() == send_txs_3_4[0]->get_tx_hash_256());
    xassert(send_txs_test[3]->get_tx_hash_256() == send_txs_4_5[0]->get_tx_hash_256());
}
