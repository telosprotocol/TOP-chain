#include "gtest/gtest.h"
#include "xblockmaker/xunit_maker.h"
#include "xblockmaker/xblock_rules.h"
#include "test_common.hpp"
#include "xstore/xstore_face.h"
#include "xstore/xaccount_context.h"
#include "xblockstore/xblockstore_face.h"
#include "xdata/xtransaction_maker.hpp"
#include "tests/mock/xdatamock_table.hpp"
#include "tests/mock/xdatamock_tx.hpp"
#include "tests/mock/xcertauth_util.hpp"

using namespace top::blockmaker;
using namespace top::store;
using namespace top::base;
using namespace top::mock;

class test_block_rules : public testing::Test {
 protected:
    void SetUp() override {

    }

    void TearDown() override {
    }
 public:

};

TEST_F(test_block_rules, rules_txs_1) {
    std::string account1 = xblocktool_t::make_address_user_account("11111111111111111111");
    std::string account2 = xblocktool_t::make_address_user_account("22222222222222222222");
    base::xaccount_index_t account_index;
    data::xblock_consensus_para_t cs_para;

    {
        xblockmaker_resources_ptr_t resouces = test_xblockmaker_resources_t::create();
        xdatamock_tx datamock_tx(resouces, account1);

        xunit_maker_ptr_t unitmaker = make_object_ptr<xunit_maker_t>(account1, resouces);
        ASSERT_EQ(unitmaker->check_latest_state(account_index), xsuccess);

        auto origin_txs = datamock_tx.generate_transfer_tx(account2, 3);
        xassert(unitmaker->push_tx(cs_para, origin_txs[0]) == true);
        xassert(unitmaker->push_tx(cs_para, origin_txs[1]) == true);
        xassert(unitmaker->push_tx(cs_para, origin_txs[2]) == true);
    }

    {
        xblockmaker_resources_ptr_t resouces = test_xblockmaker_resources_t::create();
        xdatamock_tx datamock_tx(resouces, account1);

        xunit_maker_ptr_t unitmaker = make_object_ptr<xunit_maker_t>(account1, resouces);
        ASSERT_EQ(unitmaker->check_latest_state(account_index), xsuccess);

        auto origin_txs = datamock_tx.generate_transfer_tx(account2, 4);
        xassert(unitmaker->push_tx(cs_para, origin_txs[0]) == true);
        xassert(unitmaker->push_tx(cs_para, origin_txs[2]) == false);
    }
    // {
    //     xblockmaker_resources_ptr_t resouces = test_xblockmaker_resources_t::create();
    //     xdatamock_tx datamock_tx(resouces, account1);

    //     xunit_maker_ptr_t unitmaker = make_object_ptr<xunit_maker_t>(account1, resouces);
    //     ASSERT_EQ(unitmaker->check_latest_state(), xsuccess);

    //     auto origin_txs = datamock_tx.generate_transfer_tx(account2, 4);

    //     std::vector<xcons_transaction_ptr_t> unvalid_origin_txs;
    //     unvalid_origin_txs.push_back(origin_txs[1]);
    //     unvalid_origin_txs.push_back(origin_txs[2]);

    //     std::vector<xcons_transaction_ptr_t> valid_txs;
    //     std::vector<xcons_transaction_ptr_t> pop_txs;
    //     unitmaker->unit_rules_filter(unvalid_origin_txs, valid_txs, pop_txs);
    //     xassert(0 == valid_txs.size());
    //     xassert(pop_txs.size() == 0);
    // }
    // {
    //     xblockmaker_resources_ptr_t resouces = test_xblockmaker_resources_t::create();
    //     xdatamock_tx datamock_tx(resouces, account1);

    //     xunit_maker_ptr_t unitmaker = make_object_ptr<xunit_maker_t>(account1, resouces);
    //     ASSERT_EQ(unitmaker->check_latest_state(), xsuccess);

    //     auto origin_txs = datamock_tx.generate_transfer_tx(account2, 4);
    //     origin_txs[1]->get_transaction()->set_last_hash(1000);

    //     std::vector<xcons_transaction_ptr_t> valid_txs;
    //     std::vector<xcons_transaction_ptr_t> pop_txs;
    //     unitmaker->unit_rules_filter(origin_txs, valid_txs, pop_txs);
    //     xassert(1 == valid_txs.size());
    //     xassert(pop_txs.size() == 1);
    // }
}

// TEST_F(test_block_rules, rules_txs_2) {
//     std::string account1 = xblocktool_t::make_address_user_account("11111111111111111111");
//     std::string account2 = xblocktool_t::make_address_user_account("22222222222222222222");

//     {
//         xblockmaker_resources_ptr_t resouces = test_xblockmaker_resources_t::create();
//         xdatamock_tx datamock_tx(resouces, account1);

//         xunit_maker_ptr_t unitmaker = make_object_ptr<xunit_maker_t>(account1, resouces);
//         ASSERT_EQ(unitmaker->check_latest_state(), xsuccess);

//         auto origin_txs = datamock_tx.generate_transfer_tx(account2, 3);
//         std::vector<xcons_transaction_ptr_t> valid_txs;
//         std::vector<xcons_transaction_ptr_t> pop_txs;
//         unitmaker->unit_rules_filter(origin_txs, valid_txs, pop_txs);
//         xassert(origin_txs.size() == valid_txs.size());
//         xassert(pop_txs.size() == 0);
//     }

//     {
//         xblockmaker_resources_ptr_t resouces = test_xblockmaker_resources_t::create();
//         xdatamock_tx datamock_tx(resouces, account1);

//         xunit_maker_ptr_t unitmaker = make_object_ptr<xunit_maker_t>(account1, resouces);
//         ASSERT_EQ(unitmaker->check_latest_state(), xsuccess);

//         auto origin_txs = datamock_tx.generate_contract_tx(account2, 3);
//         std::vector<xcons_transaction_ptr_t> valid_txs;
//         std::vector<xcons_transaction_ptr_t> pop_txs;
//         unitmaker->unit_rules_filter(origin_txs, valid_txs, pop_txs);
//         xassert(valid_txs.size() == 1);
//         xassert(pop_txs.size() == 0);
//     }
// }
