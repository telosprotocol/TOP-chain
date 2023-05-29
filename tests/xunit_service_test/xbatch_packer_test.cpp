#include "gtest/gtest.h"
#include "xunit_service/xbatch_packer.h"
#include "xunit_service/xpreproposal_packer.h"
#include "xblockmaker/xblockmaker_face.h"
#include "tests/mock/xdatamock_table.hpp"
#include "tests/mock/xvchain_creator.hpp"
#include "xtxpool_v2/xtxpool_face.h"

using namespace top::blockmaker;
using namespace top::mock;
using namespace top::xtxpool_v2;

namespace top {
using namespace xunit_service;

class xbatch_packer_test : public testing::Test {
protected:
    void SetUp() override {}

    void TearDown() override {}

public:
};

class xdummy_txpool : public top::xtxpool_v2::xtxpool_face_t {
public:
    int32_t push_send_tx(const std::shared_ptr<xtx_entry> & tx) override {
        return 0;
    }
    int32_t push_receipt(const std::shared_ptr<xtx_entry> & tx, bool is_self_send, bool is_pulled) override {
        return 0;
    }
    const xcons_transaction_ptr_t pop_tx(const tx_info_t & txinfo) override {
        return nullptr;
    }
    xpack_resource get_pack_resource(const xtxs_pack_para_t & pack_para) override {
        return {};
    }
    data::xcons_transaction_ptr_t query_tx(const std::string & account, const uint256_t & hash) const override {
        return nullptr;
    }
    data::xcons_transaction_ptr_t query_tx(const std::string & account, const std::string & hash_hex) const override {
        return nullptr;
    }
    void updata_latest_nonce(const std::string & account_addr, uint64_t latest_nonce) override {
    }
    void subscribe_tables(uint8_t zone, uint16_t front_table_id, uint16_t back_table_id, common::xnode_type_t node_type) override {
    }
    void unsubscribe_tables(uint8_t zone, uint16_t front_table_id, uint16_t back_table_id, common::xnode_type_t node_type) override {
    }
    void on_block_confirmed(xblock_t * block) override {
    }
    bool on_block_confirmed(const std::string table_addr, base::enum_xvblock_class blk_class, uint64_t height) override {
        return true;
    }
    int32_t verify_txs(const std::string & account, const std::vector<xcons_transaction_ptr_t> & txs) override {
        return 0;
    }
    void refresh_table(uint8_t zone, uint16_t subaddr) override {
    }
    // void update_non_ready_accounts(uint8_t zone, uint16_t subaddr) override {}
    void update_table_state(const base::xvproperty_prove_ptr_t & property_prove_ptr, const data::xtablestate_ptr_t & table_state) override {
    }
    void build_recv_tx(base::xtable_shortid_t from_table_sid,
                       base::xtable_shortid_t to_table_sid,
                       std::vector<uint64_t> receiptids,
                       std::vector<xcons_transaction_ptr_t> & receipts) override {
    }
    void build_confirm_tx(base::xtable_shortid_t from_table_sid,
                          base::xtable_shortid_t to_table_sid,
                          std::vector<uint64_t> receiptids,
                          std::vector<xcons_transaction_ptr_t> & receipts) override {
    }
    const std::vector<xtxpool_table_lacking_receipt_ids_t> get_lacking_recv_tx_ids(uint8_t zone, uint16_t subaddr, uint32_t & total_num) const override {
        return {};
    }
    const std::vector<xtxpool_table_lacking_receipt_ids_t> get_lacking_confirm_tx_ids(uint8_t zone, uint16_t subaddr, uint32_t & total_num) const override {
        return {};
    }

    void print_statistic_values() const override{};
    void update_peer_receipt_id_state(const base::xvproperty_prove_ptr_t & property_prove_ptr, const base::xreceiptid_state_ptr_t & receiptid_state) override {
    }
    std::map<std::string, uint64_t> get_min_keep_heights() const override {
        return {};
    }
    xtransaction_ptr_t get_raw_tx(const std::string & account_addr, base::xtable_shortid_t peer_table_sid, uint64_t receipt_id) const override {
        return nullptr;
    }

    const std::set<base::xtable_shortid_t> & get_all_table_sids() const override {
        return m_all_table_sids;
    }

    uint32_t get_tx_cache_size(const std::string & table_addr) const override {
        return 0;
    }

    void update_uncommit_txs(base::xvblock_t * _lock_block, base::xvblock_t * _cert_block) override {}

    void add_tx_action_cache(base::xvblock_t * block, const std::shared_ptr<base::xinput_actions_cache_base> & txactions_cache) override {}
private:
    std::set<base::xtable_shortid_t> m_all_table_sids;
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

        uint32_t r = (rand()%200) + 1;

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
    xdummy_txpool txpool;

    mock::xdatamock_table mocktable(1, 2);
    std::string table_addr = mocktable.get_account();
    std::vector<std::string> unit_addrs = mocktable.get_unit_accounts();
    std::string from_addr = unit_addrs[0];
    std::string to_addr = unit_addrs[1];

    std::vector<xcons_transaction_ptr_t> send_txs = mocktable.create_send_txs(from_addr, to_addr, 2);
    xblock_consensus_para_t proposal_para = mocktable.init_consensus_para();

    xpreproposal_msg_t preproposal_msg(proposal_para, send_txs, {});
    std::string msg_str;
    preproposal_msg.serialize_to_string(msg_str);

    xpreproposal_msg_t preproposal_msg2;
    preproposal_msg2.serialize_from_string(msg_str, table_addr, &txpool);

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
