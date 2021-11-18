#include "gtest/gtest.h"
#include "xdata/xtx_factory.h"
#include "xdata/xblocktool.h"
#include "xdata/xrootblock.h"

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

TEST_F(test_genesis_block, rootblock) {
    auto root_block = xrootblock_t::get_rootblock();    
    std::cout << root_block->dump() << std::endl;

    auto blockhash = base::xstring_utl::to_hex(root_block->get_block_hash());
    ASSERT_EQ(blockhash, "ddbeb302b1b000cdb403d8255fe4b49bbc0b9709f982e63ab5707267571c1c66");
}

TEST_F(test_genesis_block, genesis_tx_with_balance) {
    std::string address = "T00000LfVA4mibYtKsGqGpGRxf8VZYHmdwriuZNo";
    uint64_t top_balance = base::xstring_utl::touint64("1000000000");
    xtransaction_ptr_t tx = xtx_factory::create_genesis_tx_with_balance(address, top_balance);
    auto txhash = tx->get_digest_hex_str();
    std::cout << "address=" << address << " gtx=" << txhash << std::endl;
    ASSERT_EQ(txhash, "bcb38f4ea4e60e768cd0a6a5363c937e44b861616fbb36b54484a1d4d493e198");
}

TEST_F(test_genesis_block, genesis_block_with_balance) {
    std::string address = "T00000LfVA4mibYtKsGqGpGRxf8VZYHmdwriuZNo";
    uint64_t top_balance = base::xstring_utl::touint64("1000000000");
    base::xauto_ptr<base::xvblock_t> genesis_block = data::xblocktool_t::create_genesis_lightunit(address, top_balance);    
    std::cout << genesis_block->dump() << std::endl;

    auto blockhash = base::xstring_utl::to_hex(genesis_block->get_block_hash());
    ASSERT_EQ(blockhash, "f966559453e1a515cc620334def8d1ec567eaa382f823373b4ae8f9a2bc6607f");
    auto last_blockhash = base::xstring_utl::to_hex(genesis_block->get_last_block_hash());
    ASSERT_EQ(last_blockhash, "ddbeb302b1b000cdb403d8255fe4b49bbc0b9709f982e63ab5707267571c1c66");
}

