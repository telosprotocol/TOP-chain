#include "xcommon/xaccount_address_fwd.h"
#include "xdata/xpreprocess_data.h"
#include "xdata/xtransaction_v3.h"
#include "xdata/xtx_factory.h"
#include "xdata/xunit_bstate.h"
#include "xpreprocess/xpreprocess.h"

#include <gtest/gtest.h>

using namespace top;

// default data
std::string default_T6_sender{"T600043b85b55221b4fbc5ef999b40694bef221dd16f01"};
std::string default_T6_recver{"T600043b85b55221b4fbc5ef999b40694bef221dd16f02"};
evm_common::u256 default_eth_value{1000};
evm_common::u256 default_evm_gas_limit{4};
evm_common::u256 default_eth_per_gas{5000000000};

TEST(test_xpreprocess, send) {
    data::xtransaction_ptr_t tx = data::xtx_factory::create_ethcall_v3_tx(default_T6_sender, default_T6_recver, "", default_eth_value, default_evm_gas_limit, default_eth_per_gas);
    data::xcons_transaction_ptr_t ctx = make_object_ptr<data::xcons_transaction_t>(tx.get());
    common::xaccount_address_t account{default_T6_sender};
    data::xmessage_t msg{account, ctx};
    auto pre = &data::xpreprocess::instance();
    auto ret = pre->send(msg);
    EXPECT_EQ(ret, false);
}

TEST(test_xpreprocess, async_send) {
    data::xtransaction_ptr_t tx = data::xtx_factory::create_ethcall_v3_tx(default_T6_sender, default_T6_recver, "", default_eth_value, default_evm_gas_limit, default_eth_per_gas);
    data::xcons_transaction_ptr_t ctx = make_object_ptr<data::xcons_transaction_t>(tx.get());
    common::xaccount_address_t account{default_T6_sender};
    data::xmessage_t msg{account, ctx};
    auto pre = &data::xpreprocess::instance();
    auto ret = pre->send(msg);
    EXPECT_EQ(ret, false);
}