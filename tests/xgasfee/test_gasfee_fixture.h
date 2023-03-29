#pragma once

#include "xconfig/xpredefined_configurations.h"
#include "xcrypto/xckey.h"
#include "xdata/xnative_contract_address.h"
#include "xpbase/base/top_utils.h"

#include <gtest/gtest.h>

#include <string>

#define private public
#include "xconfig/xconfig_register.h"
#include "xdata/xblockbuild.h"
#include "xdata/xtable_bstate.h"
#include "xdata/xtransaction.h"
#include "xdata/xtransaction_v2.h"
#include "xdata/xtransaction_v3.h"
#include "xdata/xtx_factory.h"
#include "xgasfee/xgasfee.h"
#include "xstatectx/xstatectx_face.h"

namespace top {
namespace tests {

class xtest_gasfee_data_t {
public:
    void make_default() {
        make_bstate();
        make_unit_state(default_bstate);
        default_tx_v2 = make_tx_v2();
        default_tx_v3 = make_tx_v3();
        default_tx_v3_deploy = make_tx_v3_deploy();
        make_cons_tx();
    }

    void make_recv_default() {
        make_bstate();
        make_unit_state(default_bstate);
        default_tx_v2 = make_tx_v2();
        default_tx_v3 = make_tx_v3();
        make_cons_tx();
        default_cons_tx->set_current_used_tgas(default_free_tgas);
        default_cons_tx->set_current_used_deposit(default_used_deposit);
        make_recv_cons_tx();
    }

    void make_confirm_default() {
        make_confirm_bstate();
        make_unit_state(default_confirm_bstate);
        default_tx_v2 = make_tx_v2();
        default_tx_v3 = make_tx_v3();
        make_cons_tx();
        default_cons_tx->set_current_used_tgas(default_free_tgas);
        default_cons_tx->set_current_used_deposit(default_used_deposit);
        make_recv_cons_tx();
        default_recv_cons_tx->set_current_used_deposit(default_used_deposit);
        default_recv_cons_tx->set_current_recv_tx_use_send_tx_tgas(0);
        make_confirm_cons_tx();
    }

    void make_bstate() {
        auto bstate = make_object_ptr<base::xvbstate_t>(default_sender, (uint64_t)0, (uint64_t)0, std::string(), std::string(), (uint64_t)0, (uint32_t)0, (uint16_t)0);
        auto canvas = make_object_ptr<base::xvcanvas_t>();
        bstate->new_string_var(data::XPROPERTY_LAST_TX_HOUR_KEY, canvas.get());
        bstate->load_string_var(data::XPROPERTY_LAST_TX_HOUR_KEY)->reset(std::to_string(default_last_time), canvas.get());
        bstate->new_string_var(data::XPROPERTY_USED_TGAS_KEY, canvas.get());
        bstate->load_string_var(data::XPROPERTY_USED_TGAS_KEY)->reset(std::to_string(default_used_tgas), canvas.get());
        bstate->new_token_var(data::XPROPERTY_BALANCE_AVAILABLE, canvas.get());
        bstate->load_token_var(data::XPROPERTY_BALANCE_AVAILABLE)->deposit(base::vtoken_t(default_balance), canvas.get());
        bstate->new_token_var(data::XPROPERTY_BALANCE_PLEDGE_TGAS, canvas.get());
        bstate->load_token_var(data::XPROPERTY_BALANCE_PLEDGE_TGAS)->deposit(base::vtoken_t(default_tgas_balance), canvas.get());
        default_bstate = bstate;
    }

    void make_confirm_bstate() {
        auto bstate = make_object_ptr<base::xvbstate_t>(default_sender, (uint64_t)0, (uint64_t)0, std::string(), std::string(), (uint64_t)0, (uint32_t)0, (uint16_t)0);
        auto canvas = make_object_ptr<base::xvcanvas_t>();
        bstate->new_string_var(data::XPROPERTY_LAST_TX_HOUR_KEY, canvas.get());
        bstate->load_string_var(data::XPROPERTY_LAST_TX_HOUR_KEY)->reset(std::to_string(default_onchain_time), canvas.get());
        bstate->new_string_var(data::XPROPERTY_USED_TGAS_KEY, canvas.get());
        bstate->load_string_var(data::XPROPERTY_USED_TGAS_KEY)->reset(std::to_string(XGET_ONCHAIN_GOVERNANCE_PARAMETER(max_gas_account)), canvas.get());
        bstate->new_token_var(data::XPROPERTY_BALANCE_AVAILABLE, canvas.get());
        bstate->load_token_var(data::XPROPERTY_BALANCE_AVAILABLE)->deposit(base::vtoken_t(default_balance - default_deposit), canvas.get());
        bstate->new_token_var(data::XPROPERTY_BALANCE_PLEDGE_TGAS, canvas.get());
        bstate->load_token_var(data::XPROPERTY_BALANCE_PLEDGE_TGAS)->deposit(base::vtoken_t(default_tgas_balance), canvas.get());
        bstate->new_token_var(data::XPROPERTY_BALANCE_LOCK, canvas.get());
        bstate->load_token_var(data::XPROPERTY_BALANCE_LOCK)->deposit(base::vtoken_t(default_deposit), canvas.get());
        default_confirm_bstate = bstate;
    }

    void make_unit_state(xobject_ptr_t<base::xvbstate_t> bstate) {
        default_unit_state = std::make_shared<data::xunit_bstate_t>(bstate.get());
    }

    xobject_ptr_t<data::xtransaction_v2_t> make_tx_v2() {
        xobject_ptr_t<data::xtransaction_v2_t> tx{make_object_ptr<data::xtransaction_v2_t>()};
        data::xproperty_asset asset{data::XPROPERTY_ASSET_TOP, default_amount};
        tx->make_tx_transfer(asset);
        tx->set_last_trans_hash_and_nonce(uint256_t(), uint64_t(0));
        tx->set_different_source_target_address(default_sender, default_recver);
        tx->set_fire_timestamp(default_fire);
        tx->set_expire_duration(default_expire);
        tx->set_deposit(default_deposit);
        tx->set_digest();
        utl::xecprikey_t pri_key_obj((uint8_t *)(DecodePrivateString(default_sign_key).data()));
        utl::xecdsasig_t signature_obj = pri_key_obj.sign(tx->digest());
        auto signature = std::string(reinterpret_cast<char *>(signature_obj.get_compact_signature()), signature_obj.get_compact_signature_size());
        tx->set_authorization(signature);
        tx->set_len();
        return tx;
    }

    xobject_ptr_t<data::xtransaction_t> make_tx_v3() {
        std::string data;
        data::xtransaction_ptr_t tx = data::xtx_factory::create_ethcall_v3_tx(default_T6_sender, default_T6_recver, data, default_eth_value, default_evm_gas_limit, default_eth_per_gas);
        return tx;
    }

    xobject_ptr_t<data::xtransaction_t> make_tx_v3_deploy() {
        std::string data = "1111111"; // code not empty
        std::string to;  // to addr is empty
        data::xtransaction_ptr_t tx = data::xtx_factory::create_ethcall_v3_tx(default_T6_sender, to, data, default_eth_value, default_evm_gas_limit, default_eth_per_gas);
        return tx;
    }

    void make_cons_tx() {
        if (default_tx_version == data::xtransaction_version_3) {
            if (default_tx_type == data::xtransaction_type_transfer) {
                default_cons_tx = make_object_ptr<data::xcons_transaction_t>(default_tx_v3.get());
            } else {

                default_cons_tx = make_object_ptr<data::xcons_transaction_t>();
                auto tx = default_tx_v3_deploy.get();
                tx->add_ref();
                default_cons_tx->m_tx.attach(tx);
                default_cons_tx->m_subtype = data::enum_transaction_subtype_self;
            }
            return;
        }
        default_cons_tx = make_object_ptr<data::xcons_transaction_t>(default_tx_v2.get());
    }

    void make_recv_cons_tx() {
        base::xvaction_t srctx_action = data::xblockaction_build_t::make_tx_action(default_cons_tx);
        base::xtx_receipt_ptr_t txreceipt = make_object_ptr<base::xtx_receipt_t>(srctx_action);
        default_recv_cons_tx = make_object_ptr<data::xcons_transaction_t>(default_cons_tx->get_transaction(), txreceipt);
    }

    void make_confirm_cons_tx() {
        base::xvaction_t recvtx_action = data::xblockaction_build_t::make_tx_action(default_recv_cons_tx);
        base::xtx_receipt_ptr_t txreceipt = make_object_ptr<base::xtx_receipt_t>(recvtx_action);
        default_confirm_cons_tx = make_object_ptr<data::xcons_transaction_t>(default_recv_cons_tx->get_transaction(), txreceipt);
    }

    gasfee::xtop_gasfee make_operator() {
        gasfee::xtop_gasfee op{default_unit_state, default_cons_tx, default_onchain_time, default_onchain_deposit_tgas};
        return op;
    }

    gasfee::xtop_gasfee make_recv_operator() {
        gasfee::xtop_gasfee op{default_unit_state, default_recv_cons_tx, default_onchain_time, default_onchain_deposit_tgas};
        return op;
    }

    gasfee::xtop_gasfee make_confirm_operator() {
        gasfee::xtop_gasfee op{default_unit_state, default_confirm_cons_tx, default_onchain_time, default_onchain_deposit_tgas};
        return op;
    }

    uint64_t legcy_get_available_tgas() {
        auto token_price = default_unit_state->get_token_price(default_onchain_deposit_tgas);
        return default_unit_state->get_available_tgas(default_onchain_time, token_price);
    }

    // default data
    std::string default_sender{"T00000LWZ2K7Be3iMZwkLTZpi2saSmdp9AyWsCBc"};
    std::string default_recver{"T00000LfxdAPxPUrbYvDCkpijvicSQCXTBT7J7WW"};
    std::string default_T6_sender{"T600042d3f291fac3dea7f5ed1da06ace4b50ebee76207"};
    std::string default_T6_recver{"T600043b85b55221b4fbc5ef999b40694bef221dd16f00"};
    std::string default_T8_sender{"T800000fe304a43d35321e39d2c9b10183c10ed941d687"};
    std::string default_T8_recver{"T80000b7f80a61dfe2418465e2a845724b54005886bfa8"};
    std::string default_sign_key{"NzjQLs3K3stpskP8j1VG5DKwZF2vvBJNLDaHAvxsFQA="};
    uint64_t default_balance{ASSET_TOP(1000)};
    uint64_t default_tgas_balance{ASSET_TOP(1000)};
    uint64_t default_last_time{9999000};
    uint64_t default_used_tgas{10000};
    uint64_t default_fire{10000000};
    uint64_t default_expire{600};
    uint64_t default_amount{100};
    uint64_t default_deposit{ASSET_uTOP(100000)};
    uint64_t default_v3_deposit{0};
    uint64_t default_used_deposit{ASSET_uTOP(50000)};
    uint64_t default_onchain_time{10000000};
    uint64_t default_onchain_deposit_tgas{ASSET_TOP(2000000000)};
    uint64_t default_free_tgas{991158};
    uint64_t default_available_tgas{996158};
    data::enum_xtransaction_version default_tx_version{data::xtransaction_version_2};
    data::enum_xtransaction_type default_tx_type{data::xtransaction_type_transfer};
    evm_common::u256 default_eth_per_gas{5000000000};
    evm_common::u256 default_evm_gas_limit{4};
    evm_common::u256 default_eth_value{1000};

    // data to build
    xobject_ptr_t<base::xvbstate_t> default_bstate;
    xobject_ptr_t<base::xvbstate_t> default_confirm_bstate;
    std::shared_ptr<data::xunit_bstate_t> default_unit_state;
    xobject_ptr_t<data::xtransaction_v2_t> default_tx_v2;
    xobject_ptr_t<data::xtransaction_t>    default_tx_v3;
    xobject_ptr_t<data::xtransaction_t>    default_tx_v3_deploy;
    xobject_ptr_t<data::xcons_transaction_t> default_cons_tx;
    xobject_ptr_t<data::xcons_transaction_t> default_recv_cons_tx;
    xobject_ptr_t<data::xcons_transaction_t> default_confirm_cons_tx;
};

class xtest_gasfee_fixture_t : public testing::Test, public xtest_gasfee_data_t {
public:
    xtest_gasfee_fixture_t() {
    }

    void SetUp() override {
        top::config::config_register.get_instance().set(config::xtx_deposit_gas_exchange_ratio_onchain_goverance_parameter_t::name, std::to_string(20));
        top::config::config_register.get_instance().set(config::xeth_gas_to_tgas_exchange_ratio_onchain_goverance_parameter_t::name, std::to_string(80));
        top::config::config_register.get_instance().set(config::xtop_eth_base_price_onchain_goverance_parameter_t::name, std::to_string(40000000000));
        top::config::config_register.get_instance().set(config::xeth_to_top_exchange_ratio_onchain_goverance_parameter_t::name, std::to_string(5000000));
        top::config::config_register.get_instance().set(config::xmin_tx_deposit_onchain_goverance_parameter_t::name, std::to_string(ASSET_uTOP(100000)));
        top::config::config_register.get_instance().set(config::xinitial_total_gas_deposit_onchain_goverance_parameter_t::name, std::to_string(ASSET_TOP(1000000000)));
        top::config::config_register.get_instance().set(config::xtotal_gas_shard_onchain_goverance_parameter_t::name, std::to_string(2160000000000));
        top::config::config_register.get_instance().set(config::xvalidator_group_count_configuration_t::name, std::to_string(4));
        top::config::config_register.get_instance().set(config::xusedgas_reset_interval_onchain_goverance_parameter_t::name, std::to_string(24 * 60 * 6));
        top::config::config_register.get_instance().set(config::xmin_free_gas_asset_onchain_goverance_parameter_t::name, std::to_string(ASSET_TOP(100)));
        top::config::config_register.get_instance().set(config::xfree_gas_onchain_goverance_parameter_t::name, std::to_string(25000));
        top::config::config_register.get_instance().set(config::xmax_gas_contract_onchain_goverance_parameter_t::name, std::to_string(50000000));
        top::config::config_register.get_instance().set(config::xmax_gas_account_onchain_goverance_parameter_t::name, std::to_string(1000000));
        top::config::config_register.get_instance().set(config::xbeacon_tx_fee_onchain_goverance_parameter_t::name, std::to_string(ASSET_TOP(100)));
    }

    void TearDown() override {
    }
};


class xmock_statectx_t : public statectx::xstatectx_face_t, public xtest_gasfee_data_t {
public:
    xmock_statectx_t() {
    }

    void build_default() {
        make_default();
        auto _sender = std::make_shared<data::xunit_bstate_t>(default_bstate.get(), default_bstate.get());
        base::xaccount_index_t accountindex;
        sender = std::make_shared<data::xaccount_state_t>(_sender, accountindex);
        recver_bstate = make_object_ptr<base::xvbstate_t>(default_recver, (uint64_t)0, (uint64_t)0, std::string(), std::string(), (uint64_t)0, (uint32_t)0, (uint16_t)0);
        auto _recver = std::make_shared<data::xunit_bstate_t>(recver_bstate.get(), recver_bstate.get());
        recver = std::make_shared<data::xaccount_state_t>(_recver, accountindex);
    }

    const data::xtablestate_ptr_t & get_table_state() const override {
        return table_state;
    }

    data::xunitstate_ptr_t load_unit_state(common::xaccount_address_t const& address) override {
        if (address.to_string() == default_recver) {
            return recver->get_unitstate();
        }
        return sender->get_unitstate();
    }
    data::xaccountstate_ptr_t           load_account_state(common::xaccount_address_t const& address) override {
        if (address.to_string() == default_recver) {
            return recver;
        }
        return sender;
    }
    bool do_rollback() override {
        return false;
    }

    size_t do_snapshot() override {
        return 0;
    }

    std::string get_table_address() const override {
        return table_address;
    }

    bool is_state_dirty() const override {
        return true;
    }

    std::map<std::string, statectx::xunitstate_ctx_ptr_t> const& get_modified_unit_ctx() const override {
        return m_changed_ctxs;
    }

    std::map<std::string, statectx::xunitstate_ctx_ptr_t> m_changed_ctxs;
    data::xtablestate_ptr_t table_state{nullptr};
    data::xaccountstate_ptr_t sender{nullptr};
    data::xaccountstate_ptr_t recver{nullptr};
    xobject_ptr_t<base::xvbstate_t> sender_bstate{nullptr};
    xobject_ptr_t<base::xvbstate_t> recver_bstate{nullptr};
    std::string table_address{common::eth_table_base_address.to_string()};
};

}  // namespace tests
}  // namespace top
