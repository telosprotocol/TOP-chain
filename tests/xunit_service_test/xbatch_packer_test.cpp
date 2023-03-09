#include "gtest/gtest.h"
#include "xunit_service/xbatch_packer.h"
#include "xunit_service/xpreproposal_packer.h"
#include "xblockmaker/xblockmaker_face.h"
#include "tests/mock/xdatamock_table.hpp"
#include "tests/mock/xvchain_creator.hpp"

using namespace top::blockmaker;
using namespace top::mock;

namespace top {
using namespace xunit_service;

class xbatch_packer_test : public testing::Test {
protected:
    void SetUp() override {}

    void TearDown() override {}

public:
};

TEST_F(xbatch_packer_test, pack_strategy) {
    uint32_t high_tps_num = 200;
    uint32_t middle_tps_num = 100;
    uint32_t low_tps_num = 50;
    xpack_strategy_t pack_strategy;

    #define TRY_HIGH_TPS_TIME_WINDOW (200)
    #define TRY_MIDDLE_AND_HIGH_TPS_TIME_WINDOW (500)
    #define TRY_LOW_MIDDLE_AND_HIGH_TPS_TIME_WINDOW (1000)

    ASSERT_EQ(pack_strategy.get_timer_interval(), 50);

    for (uint32_t i = 0; i < 5; i++) {
        pack_strategy.clear();
        uint32_t tx_num = pack_strategy.get_tx_num_threshold_first_time(1000);
        ASSERT_EQ(tx_num, high_tps_num);

        uint32_t r = (rand()%250) + 1;

        tx_num = pack_strategy.get_tx_num_threshold(1000 + 1);
        ASSERT_EQ(tx_num, high_tps_num);
        tx_num = pack_strategy.get_tx_num_threshold(1000 + r);
        ASSERT_EQ(tx_num, high_tps_num);
        tx_num = pack_strategy.get_tx_num_threshold(1000 + TRY_HIGH_TPS_TIME_WINDOW);
        ASSERT_EQ(tx_num, high_tps_num);

        tx_num = pack_strategy.get_tx_num_threshold(1000 + TRY_HIGH_TPS_TIME_WINDOW + 1);
        ASSERT_EQ(tx_num, middle_tps_num);
        tx_num = pack_strategy.get_tx_num_threshold(1000 + TRY_HIGH_TPS_TIME_WINDOW + r);
        ASSERT_EQ(tx_num, middle_tps_num);
        tx_num = pack_strategy.get_tx_num_threshold(1000 + TRY_MIDDLE_AND_HIGH_TPS_TIME_WINDOW);
        ASSERT_EQ(tx_num, middle_tps_num);

        tx_num = pack_strategy.get_tx_num_threshold(1000 + TRY_MIDDLE_AND_HIGH_TPS_TIME_WINDOW + 1);
        ASSERT_EQ(tx_num, low_tps_num);
        tx_num = pack_strategy.get_tx_num_threshold(1000 + TRY_MIDDLE_AND_HIGH_TPS_TIME_WINDOW + r);
        ASSERT_EQ(tx_num, low_tps_num);
        tx_num = pack_strategy.get_tx_num_threshold(1000 + TRY_LOW_MIDDLE_AND_HIGH_TPS_TIME_WINDOW);
        ASSERT_EQ(tx_num, low_tps_num);

        tx_num = pack_strategy.get_tx_num_threshold(1000 + TRY_LOW_MIDDLE_AND_HIGH_TPS_TIME_WINDOW + 1);
        ASSERT_EQ(tx_num, 0);
        r = rand();
        tx_num = pack_strategy.get_tx_num_threshold(1000 + TRY_LOW_MIDDLE_AND_HIGH_TPS_TIME_WINDOW + r);
        ASSERT_EQ(tx_num, 0);
    }
}

TEST_F(xbatch_packer_test, preproposal) {
    xvchain_creator creator;
    base::xvblockstore_t * blockstore = creator.get_blockstore();
    mock::xdatamock_table mocktable(1, 2);
    std::string table_addr = mocktable.get_account();
    std::vector<std::string> unit_addrs = mocktable.get_unit_accounts();
    std::string from_addr = unit_addrs[0];
    std::string to_addr = unit_addrs[1];

    mocktable.store_genesis_units(blockstore);

    std::vector<xcons_transaction_ptr_t> send_txs = mocktable.create_send_txs(from_addr, to_addr, 2);
    xblock_consensus_para_t proposal_para = mocktable.init_consensus_para();

    xpreproposal_msg_t preproposal_msg(proposal_para, send_txs, {});
    std::string msg_str;
    preproposal_msg.serialize_to_string(msg_str);

    xpreproposal_msg_t preproposal_msg2;
    preproposal_msg2.serialize_from_string(msg_str);

    xassert(preproposal_msg.get_last_block_hash() == preproposal_msg2.get_last_block_hash());
    xassert(preproposal_msg.get_justify_cert_hash() == preproposal_msg2.get_justify_cert_hash());
    xassert(preproposal_msg.get_gmtime() == preproposal_msg2.get_gmtime());
    xassert(preproposal_msg.get_drand_height() == preproposal_msg2.get_drand_height());
    xassert(preproposal_msg.get_total_lock_tgas_token_height() == preproposal_msg2.get_total_lock_tgas_token_height());
    xassert(preproposal_msg.get_input_txs().size() == preproposal_msg2.get_input_txs().size());
    xassert(preproposal_msg.get_receiptid_state_proves() == preproposal_msg2.get_receiptid_state_proves());
    xassert(preproposal_msg.get_auditor_xip().high_addr == preproposal_msg2.get_auditor_xip().high_addr);
    xassert(preproposal_msg.get_auditor_xip().low_addr == preproposal_msg2.get_auditor_xip().low_addr);
    xassert(preproposal_msg.get_validator_xip().high_addr == preproposal_msg2.get_validator_xip().high_addr);
    xassert(preproposal_msg.get_validator_xip().low_addr == preproposal_msg2.get_validator_xip().low_addr);
}

}  // namespace top
