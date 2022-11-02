#include "gtest/gtest.h"

#include "xstore/xaccount_context.h"
#include "xdata/xaction_parse.h"
#include "xtxexecutor/xtransaction_context.h"
#include "xconfig/xconfig_register.h"
#include "xchain_timer/xchain_timer.h"
#include "xloader/xconfig_onchain_loader.h"
//#include "test_xtxexecutor_util.hpp"

using namespace top::txexecutor;
using namespace top::data;
using namespace top;
using namespace top::store;
/*
class test_vote : public testing::Test {
 protected:
    void SetUp() override {
        m_store = xstore_factory::create_store_with_memdb();

        m_source_context = std::make_shared<xaccount_context_t>(m_source_account, m_store.get());
        m_source_context->set_lock_token_sum(XGET_CONFIG(min_account_deposit));
        m_target_context = std::make_shared<xaccount_context_t>(m_target_account, m_store.get());
        xtransaction_ptr_t tx = make_object_ptr<xtransaction_t>();
        tx->set_different_source_target_address(m_source_account, m_target_account);
        tx->set_deposit(100000);
        tx->set_digest();
        m_trans = make_object_ptr<xcons_transaction_t>(tx.get());
    }

    void TearDown() override {
    }
 public:
    xobject_ptr_t<xstore_face_t> m_store;
    std::string m_source_account{"T-1-11111111111111111"};
    std::string m_target_account{"T-1-11111111111111111"};
    std::shared_ptr<xaccount_context_t> m_source_context;
    std::shared_ptr<xaccount_context_t> m_target_context;
    xcons_transaction_ptr_t             m_trans;
};

TEST_F(test_vote, pledge_vote) {
    m_source_context->get_blockchain()->set_balance(ASSET_TOP(10000));
    data::xaction_pledge_token_vote action;
    uint64_t vote_num = 1000;
    xaction_source_null::serialze_to(m_trans->get_transaction()->get_source_action());
    action.serialze_to(m_trans->get_transaction()->get_target_action(), vote_num, 30);

    xtransaction_pledge_token_vote pledge_token_vote(m_source_context.get(), m_trans);

    pledge_token_vote.parse();
    int32_t ret = pledge_token_vote.check();
    EXPECT_EQ(0, ret);
    ret = pledge_token_vote.source_fee_exec();
    EXPECT_EQ(0, ret);
    ret = pledge_token_vote.source_action_exec();
    EXPECT_EQ(0, ret);

    ret = pledge_token_vote.target_fee_exec();
    EXPECT_EQ(0, ret);
    ret = pledge_token_vote.target_action_exec();
    EXPECT_EQ(0, ret);
    EXPECT_EQ(m_source_context->get_balance_change(), -ASSET_TOP(vote_num) - m_trans->get_current_used_deposit());
    EXPECT_EQ(vote_num, m_source_context->get_unvote_num_change());
    EXPECT_EQ(ASSET_TOP(vote_num), m_source_context->get_vote_balance_change());
}

TEST_F(test_vote, pledge_minus_vote) {
    m_source_context->get_blockchain()->set_balance(ASSET_TOP(10000));
    data::xaction_pledge_token_vote action;
    uint64_t vote_num = -1;
    xaction_source_null::serialze_to(m_trans->get_transaction()->get_source_action());
    action.serialze_to(m_trans->get_transaction()->get_target_action(), vote_num, 30);
    m_trans->get_transaction()->get_source_action().set_account_addr(m_source_account);

    xtransaction_pledge_token_vote pledge_token_vote(m_source_context.get(), m_trans);
    auto ret = pledge_token_vote.parse();
    EXPECT_EQ(0, ret);
    ret = pledge_token_vote.source_fee_exec();
    EXPECT_EQ(xconsensus_service_error_balance_not_enough, ret);
}

TEST_F(test_vote, pledge_vote_days) {
    m_source_context->get_blockchain()->set_balance(ASSET_TOP(10000));
    data::xaction_pledge_token_vote action;
    uint64_t vote_num = 1040;
    xaction_source_null::serialze_to(m_trans->get_transaction()->get_source_action());
    action.serialze_to(m_trans->get_transaction()->get_target_action(), vote_num, 60);
    m_trans->get_transaction()->get_source_action().set_account_addr(m_source_account);

    xtransaction_pledge_token_vote pledge_token_vote(m_source_context.get(), m_trans);

    pledge_token_vote.parse();
    int32_t ret = pledge_token_vote.source_fee_exec();
    EXPECT_EQ(0, ret);
    ret = pledge_token_vote.target_action_exec();
    EXPECT_EQ(0, ret);
    auto top_num = m_source_context->get_top_by_vote(vote_num, 60);
    EXPECT_EQ(static_cast<uint64_t>(top_num), m_source_context->get_vote_balance_change());
    std::cout << std::setprecision(6) << std::fixed << "factor " << top_num
              << " vote num: " << vote_num
              << " vote balance: " << m_source_context->get_vote_balance_change() << std::endl;

    vote_num = 1000;
    top_num = m_source_context->get_top_by_vote(vote_num, 570);
    EXPECT_EQ(top_num, vote_num / 2 * TOP_UNIT);

    vote_num = 1000;
    top_num = m_source_context->get_top_by_vote(vote_num, 90);
    EXPECT_EQ(top_num, 924556213);

    // uint64 overflow cases
    vote_num = UINT64_MAX;
    top_num = m_source_context->get_top_by_vote(vote_num, 30);
    EXPECT_EQ(top_num, UINT64_MAX);

    // UINT64_MAX / 1e6
    vote_num = 18446744073709;
    top_num = m_source_context->get_top_by_vote(vote_num, 30);
    EXPECT_EQ(top_num, vote_num * TOP_UNIT);
}

// TODO(jimmy)
TEST_F(test_vote, DISABLED_pledge_too_little_vote) {
    m_source_context->get_blockchain()->set_balance(ASSET_TOP(10000));
    data::xaction_pledge_token_vote action;
    uint64_t vote_num = 888;
    xaction_source_null::serialze_to(m_trans->get_transaction()->get_source_action());
    action.serialze_to(m_trans->get_transaction()->get_target_action(), vote_num, 30);
    m_trans->get_transaction()->get_source_action().set_account_addr(m_source_account);

    xtransaction_pledge_token_vote pledge_token_vote(m_source_context.get(), m_trans);

    pledge_token_vote.parse();
    int32_t ret = pledge_token_vote.check();
    EXPECT_EQ(store::xtransaction_pledge_redeem_vote_err, ret);
}

TEST_F(test_vote, redeem_vote_no_expire) {
    m_source_context->get_blockchain()->set_balance(ASSET_TOP(1000));
    data::xaction_pledge_token_vote action;
    uint64_t pledge_vote(100);
    action.serialze_to(m_trans->get_transaction()->get_target_action(), pledge_vote, 30);
    m_source_context->insert_pledge_vote_property(action);
    m_source_context->get_blockchain()->set_unvote_num(pledge_vote);

    data::xaction_redeem_token_vote action_redeem_vote;
    action_redeem_vote.serialze_to(m_trans->get_transaction()->get_target_action(), 10);

    m_trans->get_transaction()->get_source_action().set_account_addr(m_source_account);
    m_trans->get_transaction()->get_target_action().set_account_addr(m_target_account);
    xtransaction_redeem_token_vote redeem_token_vote(m_source_context.get(), m_trans);
    redeem_token_vote.parse();
    redeem_token_vote.check();
    auto ret = redeem_token_vote.source_fee_exec();
    EXPECT_EQ(0, ret);
    ret = redeem_token_vote.target_action_exec();
    EXPECT_EQ(store::xtransaction_pledge_redeem_vote_err, ret);
}
// TODO(jimmy)
TEST_F(test_vote, DISABLED_redeem_vote) {
    m_source_context->get_blockchain()->set_balance(ASSET_TOP(1000));
    data::xaction_pledge_token_vote action;
    uint64_t pledge_vote(100);
    action.m_vote_num = pledge_vote;
    action.m_lock_duration = 30;
    m_source_context->insert_pledge_vote_property(action);
    m_source_context->get_blockchain()->set_unvote_num(pledge_vote);

    data::xproperty_asset asset_out{10};
    data::xaction_asset_out action_asset_out;
    action_asset_out.serialze_to(m_trans->get_transaction()->get_source_action(), asset_out);

    m_trans->get_transaction()->get_source_action().set_account_addr(m_source_account);
    m_trans->get_transaction()->get_target_action().set_account_addr(m_target_account);
    xtransaction_redeem_token_vote redeem_token_vote(m_source_context.get(), m_trans);
    redeem_token_vote.parse();
    redeem_token_vote.check();
    auto ret = redeem_token_vote.source_fee_exec();
    EXPECT_EQ(0, ret);
    ret = redeem_token_vote.source_action_exec();
    EXPECT_EQ(0, ret);
    EXPECT_EQ(m_source_context->get_balance_change(), ASSET_TOP(asset_out.m_amount) - m_trans->get_current_used_deposit());
    EXPECT_EQ(-asset_out.m_amount, m_source_context->get_unvote_num_change());
    EXPECT_EQ(-(ASSET_TOP(asset_out.m_amount)), m_source_context->get_vote_balance_change());

    asset_out.m_amount = pledge_vote + 1;
    action_asset_out.serialze_to(m_trans->get_transaction()->get_source_action(), asset_out);
    redeem_token_vote.parse();
    ret = redeem_token_vote.source_action_exec();
    EXPECT_EQ(store::xtransaction_pledge_redeem_vote_err, ret);

    ret = redeem_token_vote.target_action_exec();
    EXPECT_EQ(0, ret);
}

TEST_F(test_vote, merge_vote) {
    m_source_context->get_blockchain()->set_balance(ASSET_TOP(1000));
    data::xaction_pledge_token_vote action;
    uint64_t pledge_vote(100);
    action.m_vote_num = pledge_vote;
    action.m_lock_duration = 30;
    m_source_context->insert_pledge_vote_property(action);

    action.m_lock_duration = 60;
    m_source_context->insert_pledge_vote_property(action);

    action.m_lock_duration = 30;
    m_source_context->insert_pledge_vote_property(action);

    int32_t err;
    auto ds = m_source_context->deque_read_get(XPROPERTY_PLEDGE_VOTE_KEY, err).get()->get_deque();
    EXPECT_EQ(2, ds.size());
    uint64_t vote_num{0};
    uint16_t duration{0};
    uint64_t lock_time{0};
    for(auto str: ds){
        base::xstream_t stream{base::xcontext_t::instance(), (uint8_t*)str.data(), static_cast<uint32_t>(str.size())};
        stream >> vote_num;
        stream >> duration;
        stream >> lock_time;
        if(lock_time == 30){
            EXPECT_EQ(200, vote_num);
        } else if(lock_time == 60){
            EXPECT_EQ(100, vote_num);
        }
    }
}

TEST_F(test_vote, vote_129) {
    data::xaction_pledge_token_vote action;
    uint64_t pledge_vote(100);
    action.m_vote_num = pledge_vote;
    for(int i = 0; i < 128; ++i){
        action.m_lock_duration = i;
        auto ret = m_source_context->insert_pledge_vote_property(action);
        EXPECT_EQ(0, ret);
    }

    int32_t err{0};
    auto ds = m_source_context->deque_read_get(XPROPERTY_PLEDGE_VOTE_KEY, err).get()->get_deque();
    EXPECT_EQ(0, err);
    EXPECT_EQ(128, ds.size());

    action.m_lock_duration = 128;
    auto ret = m_source_context->insert_pledge_vote_property(action);
    EXPECT_EQ(ret, store::xtransaction_pledge_redeem_vote_err);
    ds = m_source_context->deque_read_get(XPROPERTY_PLEDGE_VOTE_KEY, err).get()->get_deque();
    EXPECT_EQ(128, ds.size());
}

// TODO(jimmy)
TEST_F(test_vote, DISABLED_vote_normal) {
    data::xproperty_asset asset_out{0};
    data::xaction_asset_out action_asset_out;
    action_asset_out.serialze_to(m_trans->get_transaction()->get_source_action(), asset_out);

    data::xaction_run_contract action;
    std::map<std::string, uint64_t> vote_infos;
    vote_infos["T-111"] = static_cast<uint64_t>(10);
    vote_infos["T-222"] = static_cast<uint64_t>(20);
    base::xstream_t stream{base::xcontext_t::instance()};
    stream << vote_infos;
    action.serialze_to(m_trans->get_transaction()->get_target_action(), "set_vote", std::string((char*)stream.data(), stream.size()));

    m_trans->get_transaction()->get_source_action().set_account_addr(m_source_account);
    m_trans->get_transaction()->get_target_action().set_account_addr(m_target_account);
    xtransaction_vote vote(m_source_context.get(), m_trans);
    m_source_context->get_blockchain()->set_unvote_num(100);

    auto ret = vote.parse();
    EXPECT_EQ(0, ret);
    ret = vote.check();
    EXPECT_EQ(ret, 0);

    ret = vote.source_fee_exec();
    EXPECT_EQ(ret, xtransaction_too_much_deposit);

    ret = vote.source_action_exec();
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(-30, m_source_context->get_unvote_num_change());

    ret = vote.target_action_exec();
    EXPECT_EQ(static_cast<int32_t>(xvm::enum_xvm_error_code::enum_vm_exception), ret);
}

// TODO(jimmy)
TEST_F(test_vote, DISABLED_vote_too_many) {
    data::xproperty_asset asset_out{0};
    data::xaction_asset_out action_asset_out;
    action_asset_out.serialze_to(m_trans->get_transaction()->get_source_action(), asset_out);

    data::xaction_run_contract action;
    std::map<std::string, uint64_t> vote_infos;
    vote_infos["T-111"] = static_cast<uint64_t>(10);
    vote_infos["T-222"] = static_cast<uint64_t>(20);
    base::xstream_t stream{base::xcontext_t::instance()};
    stream << vote_infos;
    action.serialze_to(m_trans->get_transaction()->get_target_action(), "set_vote", std::string((char*)stream.data(), stream.size()));

    xtransaction_vote vote(m_source_context.get(), m_trans);
    auto ret = vote.parse();
    EXPECT_EQ(0, ret);
    ret = vote.check();
    EXPECT_EQ(ret, 0);
    ret = vote.source_action_exec();
    EXPECT_EQ(ret, store::xtransaction_pledge_redeem_vote_err);
}

// TODO(jimmy)
TEST_F(test_vote, DISABLED_vote_exe_fail) {
    data::xproperty_asset asset_out{0};
    data::xaction_asset_out action_asset_out;
    action_asset_out.serialze_to(m_trans->get_transaction()->get_source_action(), asset_out);

    data::xaction_run_contract action;
    std::map<std::string, uint64_t> vote_infos;
    vote_infos["T-111"] = static_cast<uint64_t>(10);
    vote_infos["T-222"] = static_cast<uint64_t>(20);
    base::xstream_t stream{base::xcontext_t::instance()};
    stream << vote_infos;
    action.serialze_to(m_trans->get_transaction()->get_target_action(), "set_vote", std::string((char*)stream.data(), stream.size()));

    xtransaction_vote vote(m_source_context.get(), m_trans);
    m_source_context->get_blockchain()->set_unvote_num(100);

    auto ret = vote.parse();
    EXPECT_EQ(0, ret);
    ret = vote.check();
    EXPECT_EQ(ret, 0);

    ret = vote.source_action_exec();
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(-30, m_source_context->get_unvote_num_change());

    xtransaction_exec_state_t state;
    state.set_tx_exec_status(enum_xunit_tx_exec_status_fail);
    xcons_transaction_ptr_t confirmtx = test_xtxexecutor_util_t::create_mock_confirm_tx(m_trans->get_transaction(), state);

    xtransaction_vote vote_confirm(m_source_context.get(), confirmtx);
    ret = vote_confirm.parse();
    EXPECT_EQ(0, ret);
    ret = vote_confirm.source_confirm_action_exec();
    EXPECT_EQ(0, m_source_context->get_unvote_num_change());
}

// TODO(jimmy) abolish vote will add the unvote num of source account
TEST_F(test_vote, DISABLED_abolish_vote) {
    xobject_ptr_t<xstore_face_t> store = xstore_factory::create_store_with_memdb();
    std::string source_account{"T-1-11111111111111111"};
    std::string target_account{"T-1-11111111111111111"};
    auto source_context = std::make_shared<xaccount_context_t>(source_account, store.get());
    auto target_context = std::make_shared<xaccount_context_t>(target_account, store.get());

    xtransaction_ptr_t tx = make_object_ptr<xtransaction_t>();
    data::xproperty_asset asset_out{0};
    data::xaction_asset_out action_asset_out;
    xaction_asset_out::serialze_to(tx->get_source_action(), asset_out);
    data::xaction_run_contract action;
    std::map<std::string, uint64_t> vote_infos;
    vote_infos["T-111"] = static_cast<uint64_t>(10);
    vote_infos["T-222"] = static_cast<uint64_t>(20);
    base::xstream_t stream{base::xcontext_t::instance()};
    stream << vote_infos;
    action.serialze_to(tx->get_target_action(), "set_vote", std::string((char*)stream.data(), stream.size()));
    tx->set_tx_type(xtransaction_type_abolish_vote);
    tx->set_different_source_target_address(source_account, target_account);
    tx->set_deposit(100000);
    tx->set_digest();

    xcons_transaction_ptr_t trans = make_object_ptr<xcons_transaction_t>(tx.get());

    xtransaction_abolish_vote vote(source_context.get(), trans);
    auto ret = vote.parse();
    EXPECT_EQ(0, ret);
    ret = vote.check();
    EXPECT_EQ(ret, 0);

    ret = vote.source_fee_exec();
    EXPECT_EQ(xtransaction_too_much_deposit, ret);

    ret = vote.source_action_exec();
    EXPECT_EQ(0, ret);

    xtransaction_exec_state_t state;
    state.set_tx_exec_status(enum_xunit_tx_exec_status_success);
    xcons_transaction_ptr_t confirmtx = test_xtxexecutor_util_t::create_mock_confirm_tx(trans->get_transaction(), state);

    xtransaction_abolish_vote vote_confirm(source_context.get(), confirmtx);
    ret = vote_confirm.parse();
    EXPECT_EQ(0, ret);
    ret = vote_confirm.source_confirm_action_exec();
    EXPECT_EQ(30, source_context->get_unvote_num_change());
}
// TODO(jimmy)
TEST_F(test_vote, DISABLED_abolish_vote_illegal) {
    xobject_ptr_t<xstore_face_t> store = xstore_factory::create_store_with_memdb();
    std::string source_account{"T-1-11111111111111111"};
    std::string target_account{"T-1-11111111111111111"};
    auto source_context = std::make_shared<xaccount_context_t>(source_account, store.get());
    auto target_context = std::make_shared<xaccount_context_t>(target_account, store.get());

    xtransaction_ptr_t tx = make_object_ptr<xtransaction_t>();
    data::xproperty_asset asset_out{0};
    data::xaction_asset_out action_asset_out;
    xaction_asset_out::serialze_to(tx->get_source_action(), asset_out);
    data::xaction_run_contract action;
    std::map<std::string, uint64_t> vote_infos;
    vote_infos["T-111"] = static_cast<uint64_t>(10);
    vote_infos["T-222"] = static_cast<uint64_t>(20);
    base::xstream_t stream{base::xcontext_t::instance()};
    stream << vote_infos;
    action.serialze_to(tx->get_target_action(), "set_vote", std::string((char*)stream.data(), stream.size()));
    tx->set_tx_type(xtransaction_type_abolish_vote);
    tx->set_different_source_target_address(source_account, target_account);
    tx->set_deposit(100000);
    tx->set_digest();

    xcons_transaction_ptr_t trans = make_object_ptr<xcons_transaction_t>(tx.get());

    xtransaction_abolish_vote vote(source_context.get(), trans);
    auto ret = vote.parse();
    EXPECT_EQ(0, ret);
    ret = vote.check();
    EXPECT_EQ(ret, 0);

    ret = vote.target_action_exec();
    EXPECT_EQ(static_cast<int32_t>(xvm::enum_xvm_error_code::enum_vm_exception), ret);

    xtransaction_exec_state_t state;
    state.set_tx_exec_status(enum_xunit_tx_exec_status_fail);
    xcons_transaction_ptr_t confirmtx = test_xtxexecutor_util_t::create_mock_confirm_tx(trans->get_transaction(), state);

    xtransaction_abolish_vote vote_confirm(source_context.get(), confirmtx);
    ret = vote_confirm.parse();
    EXPECT_EQ(0, ret);
    ret = vote_confirm.source_confirm_action_exec();
    EXPECT_EQ(0, source_context->get_unvote_num_change());
}
*/
