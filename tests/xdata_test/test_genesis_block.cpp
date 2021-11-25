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

TEST_F(test_genesis_block, merkle_root_and_path_compare) {
   
    std::vector<std::string> mpt_values;
    for(int i = 0; i < 10; ++i)  {
        std::string v = base::xstring_utl::tostring(i);
        mpt_values.push_back(v);
    }

    top::base::xmerkle_t<top::utl::xsha2_256_t, uint256_t> merkle;
    const std::string mpt_root = merkle.calc_root(mpt_values);
    auto mpt_root_hex =  base::xstring_utl::to_hex(mpt_root);

    ASSERT_EQ(mpt_root_hex,  "0f261760cb6cd3ed02cb371833ae61fe46525d3924d3ffcad1ac368879a1c087");

    xmerkle_path_256_t hash_path;
    xassert(merkle.calc_path(mpt_values, 1, hash_path.get_levels_for_write()));
    xassert(merkle.validate_path(mpt_values[1], mpt_root, hash_path.get_levels_for_write()));
    base::xstream_t _stream(base::xcontext_t::instance());
    hash_path.serialize_to(_stream);
    std::string _path_bin = std::string((char *)_stream.data(), _stream.size());
    auto _path_bin_hex =  base::xstring_utl::to_hex(_path_bin);
    ASSERT_EQ(_path_bin_hex, "0404015feceb66ffc86f38d952786c6d696c79c2dbc239dd4e91b46729d73a27fb57e90302a9f5b3ab61e28357cfcd14e2b42397f896aeea8d6998d19e6da85584e150d2b402020302c96f45abbeadb23878331a9ba406078bd0bd5dc202c102af7b9986249f010102948424863b58af636f888c8fce461912f6f6bf4ae8b29e76f91ca2db12e3ef5c");

}

