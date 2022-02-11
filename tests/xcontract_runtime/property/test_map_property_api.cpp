#if 0

#define private public
#define protected public
#include "gtest/gtest.h"
#include "tests/mock/xvchain_creator.hpp"
#include "xcontract_common/xbasic_contract.h"
#include "xcontract_common/xproperties/xproperty_map.h"
#include "xcontract_runtime/xtop_action_generator.h"
#include "xdata/xblocktool.h"
#include "xdata/xnative_contract_address.h"
#include "xdata/xtransaction_v2.h"
#include "xsystem_contract_runtime/xsystem_contract_manager.h"
#include "xsystem_contracts/xbasic_system_contract.h"
#include "xvm/xsystem_contracts/xregistration/xrec_registration_contract_new.h"

NS_BEG3(top, tests, contract_property)

using namespace top::common;
using namespace top::base;
using namespace top::contract_common;
using namespace top::system_contracts;


common::xaccount_address_t contract_address{sys_contract_zec_slash_info_addr};
class contract_with_property : public system_contracts::xtop_basic_system_contract {
public:
    void contract_execution_context(observer_ptr<contract_common::xcontract_execution_context_t> const& exec_context);
    // xcontract_execution_result_t execute(observer_ptr<xcontract_execution_context_t> exec_ctx);

    contract_common::properties::xmap_property_t<std::string, std::string> m_slash_prop{xstake::XPROPERTY_CONTRACT_SLASH_INFO_KEY, this};

    BEGIN_CONTRACT_API()
        DECLARE_API(contract_with_property::setup);
    END_CONTRACT_API

    void setup() {
    }
};

void contract_with_property::contract_execution_context(observer_ptr<xcontract_execution_context_t> const& exec_context) {
    m_associated_execution_context = exec_context;
}


// xcontract_execution_result_t contract_with_property::execute(observer_ptr<xcontract_execution_context_t> exec_ctx) {
//     return xcontract_execution_result_t{};
// }

class test_property_api : public testing::Test {
public:
    std::unique_ptr<top::contract_runtime::system::xsystem_contract_manager_t> system_contract_manager_{};
    contract_with_property contract_;
protected:
    void SetUp() override;
    void TearDown() override;
};

void test_property_api::SetUp() {
    system_contract_manager_ = top::make_unique<top::contract_runtime::system::xsystem_contract_manager_t>();
}
void test_property_api::TearDown(){}

TEST_F(test_property_api, map_prop_api) {
    mock::xvchain_creator creator;
    base::xvblockstore_t* blockstore = creator.get_blockstore();

    system_contract_manager_->deploy_system_contract<contract_property::contract_with_property>(
        common::xaccount_address_t{contract_address.value()}, common::xnode_type_t::rec, {}, {}, {}, {}, make_observer(blockstore));

    auto latest_vblock = data::xblocktool_t::get_latest_connectted_state_changed_block(blockstore, contract_address.value());
    assert(latest_vblock);
    xobject_ptr_t<base::xvbstate_t> bstate_ = base::xvchain_t::instance().get_xstatestore()->get_blkstate_store()->get_block_state(latest_vblock.get(), metrics::statestore_access_from_application_isbeacon);
    assert(bstate_ != nullptr);
    auto state_accessor = std::make_shared<state_accessor::xstate_accessor_t>(top::make_observer(bstate_.get()), top::state_accessor::xstate_access_control_data_t{});
    auto contract_state =
        top::make_unique<contract_common::xcontract_state_t>(contract_address, top::make_observer(state_accessor.get()), contract_common::xcontract_execution_param_t{});

    auto transfer_tx = top::make_object_ptr<top::data::xtransaction_v2_t>();
    uint64_t amount = 1000;
    top::base::xstream_t param_stream(base::xcontext_t::instance());
    param_stream << amount;
    std::string param(reinterpret_cast<char *>(param_stream.data()), param_stream.size());
    transfer_tx->make_tx_run_contract("transfer", param);
    transfer_tx->set_different_source_target_address("T00000LS7SABDaqKaKfNDqbsyXB23F8dndquCeEu", contract_address.value());
    transfer_tx->set_digest();
    transfer_tx->set_len();

    data::xcons_transaction_ptr_t cons_tx = make_object_ptr<data::xcons_transaction_t>(transfer_tx.get());
    auto action = top::contract_runtime::xaction_generator_t::generate(cons_tx);
    auto contract_ctx= top::make_unique<contract_common::xcontract_execution_context_t>(std::move(action), make_observer(contract_state.get())); // action will be moved into xcontract_execution_context_t.
    contract_.contract_execution_context(contract_ctx);

    // map apis
    std::string key = "wens_test";
    std::string value = "wens";
    contract_.m_slash_prop.add(key, value);


    EXPECT_TRUE(contract_.m_slash_prop.exist(key));
    auto res = contract_.m_slash_prop.value();
    EXPECT_TRUE(!res.empty());
    EXPECT_EQ(res[key], value);
}

NS_END3

#endif
