#include "xconfig/xpredefined_configurations.h"
#include "xcrypto/xckey.h"
#include "xdata/xtransaction_v2.h"
#include "xgasfee/xgas_operator.h"
#include "xpbase/base/top_utils.h"
#include "xbase/xobject_ptr.h"

#include <gtest/gtest.h>

namespace top {

static std::string sender{"T00000LWZ2K7Be3iMZwkLTZpi2saSmdp9AyWsCBc"};
static std::string recver{"T00000LfxdAPxPUrbYvDCkpijvicSQCXTBT7J7WW"};

TEST(test_gas_state_operator, test_burn) {
    auto bstate = make_object_ptr<base::xvbstate_t>(sender, 1, 0, "", "", 0, 0, 0);
    auto canvas = make_object_ptr<base::xvcanvas_t>();
    bstate->new_token_var(data::XPROPERTY_BALANCE_AVAILABLE, canvas.get());
    bstate->load_token_var(data::XPROPERTY_BALANCE_AVAILABLE)->deposit(top::base::vtoken_t(100), canvas.get());

    auto unit_state = std::make_shared<data::xunit_bstate_t>(bstate.get());
    top::gasfee::xgas_state_operator_t op(unit_state);
    std::error_code ec;
    op.burn(base::vtoken_t(90), ec);
    EXPECT_EQ(ec.value(), 0);

    EXPECT_EQ(bstate->find_property(data::XPROPERTY_BALANCE_AVAILABLE), true);
    EXPECT_EQ(bstate->find_property(data::XPROPERTY_BALANCE_BURN), true);
    EXPECT_EQ(bstate->load_token_var(data::XPROPERTY_BALANCE_AVAILABLE)->get_balance(), 10);
    EXPECT_EQ(bstate->load_token_var(data::XPROPERTY_BALANCE_BURN)->get_balance(), 90);
}

TEST(test_gas_state_operator, test_lock) {
    auto bstate = make_object_ptr<base::xvbstate_t>(sender, 1, 0, "", "", 0, 0, 0);
    auto canvas = make_object_ptr<base::xvcanvas_t>();
    bstate->new_token_var(data::XPROPERTY_BALANCE_AVAILABLE, canvas.get());
    bstate->load_token_var(data::XPROPERTY_BALANCE_AVAILABLE)->deposit(top::base::vtoken_t(100), canvas.get());

    auto unit_state = std::make_shared<data::xunit_bstate_t>(bstate.get());
    top::gasfee::xgas_state_operator_t op(unit_state);
    std::error_code ec;
    op.lock(base::vtoken_t(90), ec);
    EXPECT_EQ(ec.value(), 0);

    EXPECT_EQ(bstate->find_property(data::XPROPERTY_BALANCE_AVAILABLE), true);
    EXPECT_EQ(bstate->find_property(data::XPROPERTY_BALANCE_LOCK), true);
    EXPECT_EQ(bstate->load_token_var(data::XPROPERTY_BALANCE_AVAILABLE)->get_balance(), 10);
    EXPECT_EQ(bstate->load_token_var(data::XPROPERTY_BALANCE_LOCK)->get_balance(), 90);
}

TEST(test_gas_state_operator, test_unlock) {
    auto bstate = make_object_ptr<base::xvbstate_t>(sender, 1, 0, "", "", 0, 0, 0);
    auto canvas = make_object_ptr<base::xvcanvas_t>();
    bstate->new_token_var(data::XPROPERTY_BALANCE_AVAILABLE, canvas.get());
    bstate->load_token_var(data::XPROPERTY_BALANCE_AVAILABLE)->deposit(top::base::vtoken_t(100), canvas.get());
    bstate->new_token_var(data::XPROPERTY_BALANCE_LOCK, canvas.get());
    bstate->load_token_var(data::XPROPERTY_BALANCE_LOCK)->deposit(top::base::vtoken_t(100), canvas.get());

    auto unit_state = std::make_shared<data::xunit_bstate_t>(bstate.get());
    top::gasfee::xgas_state_operator_t op(unit_state);
    std::error_code ec;
    op.unlock(base::vtoken_t(90), ec);
    EXPECT_EQ(ec.value(), 0);

    EXPECT_EQ(bstate->find_property(data::XPROPERTY_BALANCE_AVAILABLE), true);
    EXPECT_EQ(bstate->find_property(data::XPROPERTY_BALANCE_LOCK), true);
    EXPECT_EQ(bstate->load_token_var(data::XPROPERTY_BALANCE_AVAILABLE)->get_balance(), 190);
    EXPECT_EQ(bstate->load_token_var(data::XPROPERTY_BALANCE_LOCK)->get_balance(), 10);
}

TEST(test_gas_state_operator, test_state_set_used_tgas) {
    auto bstate = make_object_ptr<base::xvbstate_t>(sender, 1, 0, "", "", 0, 0, 0);
    auto canvas = make_object_ptr<base::xvcanvas_t>();

    auto unit_state = std::make_shared<data::xunit_bstate_t>(bstate.get());
    top::gasfee::xgas_state_operator_t op(unit_state);
    std::error_code ec;
    op.state_set_used_tgas(100, ec);
    EXPECT_EQ(ec.value(), 0);

    EXPECT_EQ(bstate->find_property(data::XPROPERTY_USED_TGAS_KEY), true);
    EXPECT_EQ(bstate->load_string_var(data::XPROPERTY_USED_TGAS_KEY)->query(), std::to_string(100));
}

TEST(test_gas_state_operator, test_state_set_last_time) {
    auto bstate = make_object_ptr<base::xvbstate_t>(sender, 1, 0, "", "", 0, 0, 0);
    auto canvas = make_object_ptr<base::xvcanvas_t>();

    auto unit_state = std::make_shared<data::xunit_bstate_t>(bstate.get());
    top::gasfee::xgas_state_operator_t op(unit_state);
    std::error_code ec;
    op.state_set_last_time(10000000, ec);
    EXPECT_EQ(ec.value(), 0);

    EXPECT_EQ(bstate->find_property(data::XPROPERTY_LAST_TX_HOUR_KEY), true);
    EXPECT_EQ(bstate->load_string_var(data::XPROPERTY_LAST_TX_HOUR_KEY)->query(), std::to_string(10000000));
}

// TEST(test_gas, test1) {
//     // data
//     std::string sender{"T00000LWZ2K7Be3iMZwkLTZpi2saSmdp9AyWsCBc"};
//     std::string recver{"T00000LfxdAPxPUrbYvDCkpijvicSQCXTBT7J7WW"};
//     struct timeval val;
//     base::xtime_utl::gettimeofday(&val);
//     const uint64_t fire = (uint64_t)val.tv_sec;
//     const uint64_t expire = 600;
//     const uint64_t amount = 100;
//     const uint64_t deposit = ASSET_uTOP(100000);
//     const std::string sign_key{"NzjQLs3K3stpskP8j1VG5DKwZF2vvBJNLDaHAvxsFQA="};

//     // state
//     xobject_ptr_t<base::xvbstate_t> bstate{make_object_ptr<base::xvbstate_t>(sender, (uint64_t)0, (uint64_t)0, std::string(), std::string(), (uint64_t)0, (uint32_t)0, (uint16_t)0)};
//     std::shared_ptr<data::xunit_bstate_t> unit_state{std::make_shared<data::xunit_bstate_t>(bstate.get())};

//     // tx
//     xobject_ptr_t<data::xtransaction_v2_t> tx{make_object_ptr<data::xtransaction_v2_t>()};
//     data::xproperty_asset asset{data::XPROPERTY_ASSET_TOP, amount};
//     tx->make_tx_transfer(asset);
//     tx->set_last_trans_hash_and_nonce(uint256_t(), uint64_t(0));
//     tx->set_different_source_target_address(sender, recver);
//     tx->set_fire_timestamp(fire);
//     tx->set_expire_duration(expire);
//     tx->set_deposit(deposit);
//     tx->set_digest();
//     utl::xecprikey_t pri_key_obj((uint8_t *)(DecodePrivateString(sign_key).data()));
//     utl::xecdsasig_t signature_obj = pri_key_obj.sign(tx->digest());
//     auto signature = std::string(reinterpret_cast<char *>(signature_obj.get_compact_signature()), signature_obj.get_compact_signature_size());
//     tx->set_authorization(signature);
//     tx->set_len();
//     xobject_ptr_t<data::xcons_transaction_t> cons_tx{make_object_ptr<data::xcons_transaction_t>(tx.get())};

//     gasfee::xtop_gas_operator op{unit_state, cons_tx, 10000000, ASSET_TOP(1000000000)};
//     std::error_code ec;
//     // op.process_three_stage<data::enum_xtransaction_type::xtransaction_type_transfer>(ec);
//     // op.process_recv_stage<data::enum_xtransaction_type::xtransaction_type_transfer>(ec);
//     // op.process_send_stage<data::enum_xtransaction_type::xtransaction_type_run_contract>(ec);
//     // op.process_recv_stage<data::enum_xtransaction_type::xtransaction_type_run_contract>(ec);
// }

class test_tgas : public testing::Test {
public:
    void SetUp() override {
    }

    void TearDown() override {
    }
};

// TEST_F(test_tgas, test_burn) {
//     // std::string sender{"T00000LWZ2K7Be3iMZwkLTZpi2saSmdp9AyWsCBc"};
//     // xobject_ptr_t<base::xvbstate_t> bstate = make_object_ptr<base::xvbstate_t>(sender, (uint64_t)0, (uint64_t)0, std::string(), std::string(), (uint64_t)0, (uint32_t)0, (uint16_t)0);
//     // xobject_ptr_t<base::xvcanvas_t> canvas = make_object_ptr<base::xvcanvas_t>();
//     // auto propobj = bstate->new_uint64_var(data::XPROPERTY_ACCOUNT_CREATE_TIME, canvas.get());
//     //     propobj->set(base::TOP_BEGIN_GMTIME, canvas.get());
//     //     auto propobj_d = bstate->new_token_var(data::XPROPERTY_BALANCE_AVAILABLE, canvas.get());
//     //     auto balance = propobj_d->deposit(base::vtoken_t(100), canvas.get());
//     xobject_ptr_t<base::xvbstate_t> bstate = make_object_ptr<base::xvbstate_t>("T80000733b43e6a2542709dc918ef2209ae0fc6503c2f2", (uint64_t)1, (uint64_t)1, std::string(), std::string(), (uint64_t)0, (uint32_t)0, (uint16_t)0);    
//     base::xauto_ptr<base::xvcanvas_t> canvas = new base::xvcanvas_t();
//     base::xauto_ptr<base::xtokenvar_t> token = bstate->new_token_var("@1", canvas.get());
//     base::vtoken_t add_token = (base::vtoken_t)100;
//     base::vtoken_t value = token->deposit(add_token, canvas.get());
//     xassert(value == add_token);
// }

}