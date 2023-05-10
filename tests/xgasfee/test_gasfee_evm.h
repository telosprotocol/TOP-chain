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
#include "xgasfee/xgasfee_evm.h"
#include "xstatectx/xstatectx_face.h"
#include "xgasfee/xgas_estimate.h"

namespace top {
namespace tests {

class xtest_gasfee_evm_t {
public:
    void make_default() {
        make_bstate();
        make_unit_state(default_bstate);
        default_tx_v3 = make_tx_v3();
        default_tx_v3_deploy = make_tx_v3_deploy();
        make_cons_tx();
    }

    void make_bstate() {
        auto bstate = make_object_ptr<base::xvbstate_t>(default_sender, (uint64_t)0, (uint64_t)0, std::string(), std::string(), (uint64_t)0, (uint32_t)0, (uint16_t)0);
        auto canvas = make_object_ptr<base::xvcanvas_t>();
        bstate->new_string_var(data::XPROPERTY_LAST_TX_HOUR_KEY, canvas.get());
        bstate->load_string_var(data::XPROPERTY_LAST_TX_HOUR_KEY)->reset(std::to_string(default_last_time), canvas.get());
        bstate->new_token_var(data::XPROPERTY_BALANCE_AVAILABLE, canvas.get());
        bstate->load_token_var(data::XPROPERTY_BALANCE_AVAILABLE)->deposit(base::vtoken_t(default_balance), canvas.get());
        default_bstate = bstate;
    }


    void make_unit_state(xobject_ptr_t<base::xvbstate_t> bstate) {
        default_unit_state = std::make_shared<data::xunit_bstate_t>(bstate.get());
    }

    xobject_ptr_t<data::xtransaction_t> make_tx_v3() {
        std::string data;
        data::xtransaction_ptr_t tx = data::xtx_factory::create_ethcall_v3_tx(default_T6_sender, default_T6_recver, 
                                                                            data, default_eth_value, default_evm_gas_limit, 
                                                                            default_user_set_max_per_gas, default_user_set_max_priority_fee);
        return tx;
    }

    xobject_ptr_t<data::xtransaction_t> make_tx_v3_deploy() {
        std::string data = "1111111"; // code not empty
        std::string to;  // to addr is empty
        data::xtransaction_ptr_t tx = data::xtx_factory::create_ethcall_v3_tx(default_T6_sender, to, data, 
                                                                    default_eth_value, default_evm_gas_limit, 
                                                                    default_user_set_max_per_gas, default_user_set_max_priority_fee);
        return tx;
    }

    void make_cons_tx() {
        if (default_tx_version == data::xtransaction_version_3) {
            if (default_tx_type == data::xtransaction_type_transfer) {
                default_cons_tx = make_object_ptr<data::xcons_transaction_t>(default_tx_v3.get());
                default_cons_tx->set_inner_table_flag();
            } else {

                default_cons_tx = make_object_ptr<data::xcons_transaction_t>();
                auto tx = default_tx_v3_deploy.get();
                tx->add_ref();
                default_cons_tx->m_tx.attach(tx);
                default_cons_tx->m_subtype = data::enum_transaction_subtype_self;
            }
            return;
        }
    }


    gasfee::xtop_gasfee_evm make_operator_evm() {
        gasfee::xtop_gasfee_evm op{default_unit_state, default_cons_tx, default_onchain_time};
        return op;
    }

    uint64_t get_eth_utop() {
        evm_common::u256 config_base_price = gasfee::xgas_estimate::base_price();
        evm_common::u256 priority_fee_price = std::min(default_user_set_max_priority_fee, (default_user_set_max_per_gas - config_base_price));
        evm_common::u256 eth_gas_price = config_base_price + priority_fee_price;
        evm_common::u256 eth_fee_wei = default_evm_gas_used * eth_gas_price;
        evm_common::u256 eth_fee_utop = gasfee::xtop_gas_tx_operator::wei_to_utop(eth_fee_wei, true);
        return static_cast<uint64_t>(eth_fee_utop);
    }


    // default data
    std::string default_sender{"T00000LWZ2K7Be3iMZwkLTZpi2saSmdp9AyWsCBc"};
    std::string default_recver{"T00000LfxdAPxPUrbYvDCkpijvicSQCXTBT7J7WW"};
    std::string default_T6_sender{"T600042d3f291fac3dea7f5ed1da06ace4b50ebee76207"};
    std::string default_T6_recver{"T600043b85b55221b4fbc5ef999b40694bef221dd16f00"};
    std::string default_sign_key{"NzjQLs3K3stpskP8j1VG5DKwZF2vvBJNLDaHAvxsFQA="};
    uint64_t default_balance{ASSET_TOP(1000000000)};
    uint64_t default_last_time{9999000};
    uint64_t default_fire{10000000};
    uint64_t default_expire{600};
    uint64_t default_v3_deposit{0};
    uint64_t default_onchain_time{10000000};
    data::enum_xtransaction_version default_tx_version{data::xtransaction_version_3};
    data::enum_xtransaction_type default_tx_type{data::xtransaction_type_transfer};
    evm_common::u256 default_user_set_max_per_gas{0};
    evm_common::u256 default_user_set_max_priority_fee{0};
    evm_common::u256 default_evm_gas_limit{21000};
    evm_common::u256 default_evm_gas_used{21000};
    evm_common::u256 default_eth_value{0};

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

class xtest_gasfee_evm_fixture_t : public testing::Test, public xtest_gasfee_evm_t {
public:
    xtest_gasfee_evm_fixture_t() {
    }

    void SetUp() override {
        XSET_ONCHAIN_GOVERNANCE_PARAMETER(tx_deposit_gas_exchange_ratio, std::to_string(20));
        XSET_ONCHAIN_GOVERNANCE_PARAMETER(top_eth_base_price, std::to_string(40000000000));
        XSET_ONCHAIN_GOVERNANCE_PARAMETER(eth_to_top_exchange_ratio, std::to_string(5000000));
        XSET_ONCHAIN_GOVERNANCE_PARAMETER(eth_to_top_exchange_ratio, std::to_string(5000000));
        XSET_ONCHAIN_GOVERNANCE_PARAMETER(max_gas_contract, std::to_string(50000000));
    }

    void TearDown() override {
    }
};


class xmock_statectx_evm_t : public statectx::xstatectx_face_t, public xtest_gasfee_evm_t {
public:
    xmock_statectx_evm_t() {
    }

    void build_default() {
        make_default();
        auto _sender = std::make_shared<data::xunit_bstate_t>(default_bstate.get(), default_bstate.get());
        base::xaccount_index_t accountindex;
        sender = std::make_shared<data::xaccount_state_t>(_sender, accountindex);
        xobject_ptr_t<base::xvbstate_t> recver_bstate{nullptr};
        if (default_sender == default_recver) {
            recver_bstate = default_bstate;
        } else {
            recver_bstate = make_object_ptr<base::xvbstate_t>(default_recver, (uint64_t)0, (uint64_t)0, std::string(), std::string(), (uint64_t)0, (uint32_t)0, (uint16_t)0);
        }
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

    std::string table_address{common::eth_table_base_address.to_string()};
};

}  // namespace tests
}  // namespace top
