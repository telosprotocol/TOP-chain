#define private public
#include "xbase/xobject_ptr.h"
#include "xbasic/xmemory.hpp"
#include "xcontract_common/xcontract_state.h"
#include "xcontract_common/xproperties/xproperty_access_control.h"
#include "xcontract_runtime/xaccount_vm.h"
#include "xcontract_runtime/xuser/xuser_action_runtime.h"
#include "xdata/xblocktool.h"
#include "xvledger/xvstate.h"

#include <gtest/gtest.h>

using namespace top;
using namespace top::base;
using namespace top::contract_runtime;
using namespace top::data;

#define DEMO_CONTRACT_ADDRESS "T2000138SJedyci3eaZN1XXC2wW79RYwgYh26n2cW"

class test_demo : public testing::Test {
public:
    xaccount_vm_t m_vm;

    void SetUp() {};
    void TearDown() {};
};

TEST_F(test_demo, test1) {
    auto raw_tx = make_object_ptr<xtransaction_t>();
    xaction_t source_action;
    xaction_t target_action;
    source_action.m_account_addr = DEMO_CONTRACT_ADDRESS;
    target_action.m_account_addr = DEMO_CONTRACT_ADDRESS;
    target_action.m_action_type = enum_xaction_type::xaction_type_run_contract;
    target_action.set_action_name("setup");
    raw_tx->set_source_action(source_action);
    raw_tx->set_target_action(target_action);
    raw_tx->set_tx_type(enum_xtransaction_type::xtransaction_type_run_contract);
    auto tx = make_object_ptr<xcons_transaction_t>(raw_tx.get());

    std::vector<xcons_transaction_ptr_t> txs{tx};
    auto vbstate = make_object_ptr<xvbstate_t>(std::string{DEMO_CONTRACT_ADDRESS}, 0, 0, "0", "0", 0, 0, 0);
    m_vm.execute(txs, vbstate);
} 