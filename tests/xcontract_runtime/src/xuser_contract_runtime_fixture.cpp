#if 0
#include "tests/xcontract_runtime/xuser_contract_runtime_fixture.h"

#include "xbasic/xmemory.hpp"
#include "xdata/xblocktool.h"
#include "xcontract_common/xcontract_state.h"

NS_BEG3(top, tests, contract_runtime)

void contract_runtime_fixture::SetUp() {
    std::string address = top::data::xblocktool_t::make_address_user_account("11111111111111111111");
    bstate_.attach(new top::base::xvbstate_t(address, 1, std::vector<top::base::xvproperty_t *>()));
    property_access_control_ = std::make_shared<top::contract_common::properties::xproperty_access_control_t>(top::make_observer(bstate_.get()), top::contract_common::properties::xproperty_access_control_data_t{});
    contract_state_ = std::make_shared<top::contract_common::xcontract_state_t>(top::common::xaccount_address_t{top::data::xblocktool_t::make_address_user_account("2")}, top::make_observer(property_access_control_.get()));
    contract_runtime_ = top::make_unique<top::contract_runtime::user::xuser_contract_runtime_t>();
}

void contract_runtime_fixture::TearDown() {
    contract_runtime_.reset();
    contract_state_.reset();
    bstate_.reset();
}

NS_END3
#endif
