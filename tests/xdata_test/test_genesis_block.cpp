#include "gtest/gtest.h"
#include "xdata/xtx_factory.h"
#include "xdata/xnative_contract_address.h"
#include "xdata/xblocktool.h"
#include "xconfig/xconfig_register.h"
#include "xbase/xmem.h"
#include "xcrypto/xckey.h"

using namespace top;
using namespace top::base;
using namespace top::data;

class test_genesis_block : public testing::Test {
protected:
    void SetUp() override {
    }

    void TearDown() override {
    }
};

TEST_F(test_genesis_block, genesis_tx_with_balance) {
    std::string address = "T00000LfVA4mibYtKsGqGpGRxf8VZYHmdwriuZNo";
    uint64_t top_balance = base::xstring_utl::touint64("1000000000");
    xtransaction_ptr_t tx = xtx_factory::create_genesis_tx_with_balance(address, top_balance);
    auto txhash = tx->get_digest_hex_str();
    std::cout << "address=" << address << " gtx=" << txhash << std::endl;
    ASSERT_EQ(txhash, "bcb38f4ea4e60e768cd0a6a5363c937e44b861616fbb36b54484a1d4d493e198");
}
