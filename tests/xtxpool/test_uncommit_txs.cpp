#include "gtest/gtest.h"
#include "test_xtxpool_util.h"
#include "tests/mock/xdatamock_table.hpp"
#include "tests/mock/xvchain_creator.hpp"
#include "xdata/xlightunit.h"
#include "xtxpool_v2/xuncommit_txs.h"

using namespace top::xtxpool_v2;
using namespace top::data;
using namespace top;
using namespace top::base;
using namespace std;
using namespace top::utl;
using namespace top::mock;

class test_uncommit_txs : public testing::Test {
protected:
    void SetUp() override {
    }

    void TearDown() override {
    }
};

TEST_F(test_uncommit_txs, basic_test) {
    mock::xvchain_creator creator;
    base::xvblockstore_t * blockstore = creator.get_blockstore();

    mock::xdatamock_table mocktable(1, 2);
    auto & table_addr = mocktable.get_address();
    auto table_sid = mocktable.get_short_table_id();
    std::vector<std::string> unit_addrs = mocktable.get_unit_accounts(); 
    std::string sender = unit_addrs[0];
    std::string receiver = unit_addrs[1];

    uint32_t tx_num = 1;
    std::vector<xcons_transaction_ptr_t> send_txs1 = mocktable.create_send_txs(sender, receiver, tx_num);
    mocktable.push_txs(send_txs1);
    xblock_ptr_t _tableblock1 = mocktable.generate_one_table();

    xuncommit_txs_t uncommit_txs(table_addr);

    // case 1: uncommit_txs has no cache
    std::vector<xcons_transaction_ptr_t> recovered_send_txs;
    std::vector<xcons_transaction_ptr_t> recovered_receipts;
    auto ret = uncommit_txs.pop_recovered_block_txs(_tableblock1->get_height(), _tableblock1->get_block_hash() + "fork", _tableblock1->get_last_block_hash(), recovered_send_txs, recovered_receipts);
    xassert(ret == update_cert_and_lock);
    xassert(recovered_send_txs.empty());
    xassert(recovered_receipts.empty());

    std::map<std::string, xcons_transaction_ptr_t> send_txs_map1;
    for (auto & send_tx : send_txs1) {
        send_txs_map1[send_tx->get_tx_hash()] = send_tx;
    }
    std::map<std::string, xcons_transaction_ptr_t> receipts_map;
    uncommit_txs.update_block_txs(_tableblock1->get_height(), _tableblock1->get_block_hash() + "fork", send_txs_map1, receipts_map);

    auto tx_query = uncommit_txs.query_tx(send_txs1[0]->get_tx_hash());
    xassert(tx_query != nullptr);
    xassert(tx_query->get_tx_hash() == send_txs1[0]->get_tx_hash());

    // case 2: uncommit_txs has cert only, cert fork
    ret = uncommit_txs.pop_recovered_block_txs(_tableblock1->get_height(), _tableblock1->get_block_hash(), _tableblock1->get_last_block_hash(), recovered_send_txs, recovered_receipts);
    xassert(ret == update_cert_and_lock);
    xassert(recovered_send_txs.size() == send_txs1.size());
    for (uint32_t i = 0; i < recovered_send_txs.size(); i++) {
        xassert(recovered_send_txs[i]->get_tx_hash() == send_txs1[i]->get_tx_hash());
    }
    recovered_send_txs.clear();
    xassert(recovered_receipts.empty());

    uncommit_txs.update_block_txs(_tableblock1->get_height(), _tableblock1->get_block_hash(), send_txs_map1, receipts_map);

    // case 3: uncommit_txs has cert only
    tx_num = 2;
    std::vector<xcons_transaction_ptr_t> send_txs2 = mocktable.create_send_txs(sender, receiver, tx_num);
    mocktable.push_txs(send_txs2);
    xblock_ptr_t _tableblock2 = mocktable.generate_one_table();

    ret = uncommit_txs.pop_recovered_block_txs(_tableblock2->get_height(), _tableblock2->get_block_hash(), _tableblock2->get_last_block_hash(), recovered_send_txs, recovered_receipts);
    xassert(ret == update_cert_only);
    xassert(recovered_send_txs.empty());
    xassert(recovered_receipts.empty());

    std::map<std::string, xcons_transaction_ptr_t> send_txs_map2;
    for (auto & send_tx : send_txs2) {
        send_txs_map2[send_tx->get_tx_hash()] = send_tx;
    }
    uncommit_txs.update_block_txs(_tableblock2->get_height(), _tableblock2->get_block_hash(), send_txs_map2, receipts_map);

    // case 4: uncommit_txs has cert and lock
    tx_num = 3;
    std::vector<xcons_transaction_ptr_t> send_txs3 = mocktable.create_send_txs(sender, receiver, tx_num);
    mocktable.push_txs(send_txs3);
    xblock_ptr_t _tableblock3 = mocktable.generate_one_table();

    ret = uncommit_txs.pop_recovered_block_txs(_tableblock3->get_height(), _tableblock3->get_block_hash(), _tableblock3->get_last_block_hash(), recovered_send_txs, recovered_receipts);
    xassert(ret == update_cert_only);
    xassert(recovered_send_txs.empty());
    xassert(recovered_receipts.empty());

    std::map<std::string, xcons_transaction_ptr_t> send_txs_map3;
    for (auto & send_tx : send_txs3) {
        send_txs_map3[send_tx->get_tx_hash()] = send_tx;
    }
    uncommit_txs.update_block_txs(_tableblock3->get_height(), _tableblock3->get_block_hash(), send_txs_map3, receipts_map);

    // case 5: same cert
    ret = uncommit_txs.pop_recovered_block_txs(_tableblock3->get_height(), _tableblock3->get_block_hash(), _tableblock3->get_last_block_hash(), recovered_send_txs, recovered_receipts);
    xassert(ret == no_need_update);

    // case 6: cert height lower
    ret = uncommit_txs.pop_recovered_block_txs(_tableblock2->get_height(), _tableblock2->get_block_hash(), _tableblock2->get_last_block_hash(), recovered_send_txs, recovered_receipts);
    xassert(ret == no_need_update);

    // case 7:cert forked.
    ret = uncommit_txs.pop_recovered_block_txs(_tableblock3->get_height(), _tableblock3->get_block_hash() + "fork", _tableblock3->get_last_block_hash(), recovered_send_txs, recovered_receipts);
    xassert(ret == update_cert_only);
    xassert(recovered_send_txs.size() == send_txs3.size());
    // for (uint32_t i = 0; i < recovered_send_txs.size(); i++) {
    //     xassert(recovered_send_txs[i]->get_tx_hash() == send_txs3[i]->get_tx_hash());
    // }
    recovered_send_txs.clear();
    xassert(recovered_receipts.empty());

    std::vector<xcons_transaction_ptr_t> send_txs3_fork;
    send_txs3_fork.insert(send_txs3_fork.begin(), send_txs3.begin(), send_txs3.begin() + (send_txs3.size() - 1));
    xassert(send_txs3_fork.size() + 1 == send_txs3.size());

    std::map<std::string, xcons_transaction_ptr_t> send_txs_map3_fork;
    for (uint32_t i = 0; i < send_txs3.size() - 1; i++) {
        send_txs_map3_fork[send_txs3[i]->get_tx_hash()] = send_txs3[i];
    }
    uncommit_txs.update_block_txs(_tableblock3->get_height(), _tableblock3->get_block_hash() + "fork", send_txs_map3_fork, receipts_map);

    // case 8:cert update, lock forked
    tx_num = 4;
    std::vector<xcons_transaction_ptr_t> send_txs4 = mocktable.create_send_txs(sender, receiver, tx_num);
    mocktable.push_txs(send_txs4);
    xblock_ptr_t _tableblock4 = mocktable.generate_one_table();

    ret = uncommit_txs.pop_recovered_block_txs(_tableblock4->get_height(), _tableblock4->get_block_hash(), _tableblock4->get_last_block_hash(), recovered_send_txs, recovered_receipts);
    xassert(ret == update_cert_and_lock);
    xassert(recovered_send_txs.size() == send_txs3_fork.size());
    // for (uint32_t i = 0; i < recovered_send_txs.size(); i++) {
    //     xassert(recovered_send_txs[i]->get_tx_hash() == send_txs3_fork[i]->get_tx_hash());
    // }
    recovered_send_txs.clear();
    xassert(recovered_receipts.empty());

    std::map<std::string, xcons_transaction_ptr_t> send_txs_map4;
    for (auto & send_tx : send_txs4) {
        send_txs_map4[send_tx->get_tx_hash()] = send_tx;
    }
    uncommit_txs.update_block_txs(_tableblock3->get_height(), _tableblock3->get_block_hash(), send_txs_map3, receipts_map);
    uncommit_txs.update_block_txs(_tableblock4->get_height(), _tableblock4->get_block_hash(), send_txs_map4, receipts_map);

    // case 9:cert and lock forked
    ret = uncommit_txs.pop_recovered_block_txs(_tableblock4->get_height(), _tableblock4->get_block_hash() + "fork", _tableblock4->get_last_block_hash() + "fork", recovered_send_txs, recovered_receipts);
    xassert(ret == update_cert_and_lock);
    xassert(recovered_send_txs.size() == send_txs3.size() + send_txs4.size());
    // for (uint32_t i = 0; i < recovered_send_txs.size(); i++) {
    //     if (i < send_txs3.size()) {
    //         xassert(recovered_send_txs[i]->get_tx_hash() == send_txs3[i]->get_tx_hash());
    //     } else {
    //         xassert(recovered_send_txs[i]->get_tx_hash() == send_txs4[i - send_txs3.size()]->get_tx_hash());
    //     }
    // }
    recovered_send_txs.clear();
    xassert(recovered_receipts.empty());

    uncommit_txs.update_block_txs(_tableblock3->get_height(), _tableblock3->get_block_hash() + "fork", send_txs_map3, receipts_map);
    uncommit_txs.update_block_txs(_tableblock4->get_height(), _tableblock4->get_block_hash() + "fork", send_txs_map4, receipts_map);

    // case 10: cert height jump
    tx_num = 5;
    std::vector<xcons_transaction_ptr_t> send_txs5 = mocktable.create_send_txs(sender, receiver, tx_num);
    mocktable.push_txs(send_txs5);
    xblock_ptr_t _tableblock5 = mocktable.generate_one_table();
    tx_num = 6;
    std::vector<xcons_transaction_ptr_t> send_txs6 = mocktable.create_send_txs(sender, receiver, tx_num);
    mocktable.push_txs(send_txs6);
    xblock_ptr_t _tableblock6 = mocktable.generate_one_table();

    ret = uncommit_txs.pop_recovered_block_txs(_tableblock6->get_height(), _tableblock6->get_block_hash(), _tableblock6->get_last_block_hash(), recovered_send_txs, recovered_receipts);
    xassert(ret == update_cert_and_lock);
    xassert(recovered_send_txs.empty());
    xassert(recovered_receipts.empty());

    std::map<std::string, xcons_transaction_ptr_t> send_txs_map5;
    for (auto & send_tx : send_txs5) {
        send_txs_map5[send_tx->get_tx_hash()] = send_tx;
    }
    std::map<std::string, xcons_transaction_ptr_t> send_txs_map6;
    for (auto & send_tx : send_txs6) {
        send_txs_map6[send_tx->get_tx_hash()] = send_tx;
    }
    uncommit_txs.update_block_txs(_tableblock5->get_height(), _tableblock5->get_block_hash(), send_txs_map5, receipts_map);
    uncommit_txs.update_block_txs(_tableblock6->get_height(), _tableblock6->get_block_hash(), send_txs_map6, receipts_map);
}
