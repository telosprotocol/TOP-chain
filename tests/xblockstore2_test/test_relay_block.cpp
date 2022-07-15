#include "gtest/gtest.h"
#include "xpbase/base/top_utils.h"
#include "xbase/xcontext.h"
#include "xbase/xdata.h"
#include "xbase/xobject.h"
#include "xbase/xmem.h"
#include "xbase/xcontext.h"
#include "xdata/xdatautil.h"
#include "xdata/xemptyblock.h"
#include "xdata/xblocktool.h"
#include "xdata/xlightunit.h"
#include "xdata/xnative_contract_address.h"
#include "xmetrics/xmetrics.h"
#include "xstore/xstore.h"
#include "xverifier/xtx_verifier.h"
#include "xblockstore/xblockstore_face.h"
#include "tests/mock/xvchain_creator.hpp"
#include "tests/mock/xdatamock_table.hpp"

using namespace top;
using namespace top::base;
using namespace top::mbus;
using namespace top::store;
using namespace top::data;
using namespace top::xverifier;
using namespace top::mock;
using namespace top::metrics;

class test_relay_block : public testing::Test {
protected:
    void SetUp() override {
    }

    void TearDown() override {
    }  

};

const std::string  test_address   {"0xbc9b5f068bc20a5b12030fcb72975d8bddc4e84c"};
const std::string  test_to_address{"0xaaaaaf068bc20a5b12030fcb72975d8bddc4e84c"};
const evm_common::u256    test_value{0x45567};
const evm_common::u256    test_value_gas{0x12346};

xeth_transaction_t xrelay_tx_create()
{
    common::xeth_address_t from_address = common::xtop_eth_address::build_from(test_address);
    common::xeth_address_t to_address = common::xtop_eth_address::build_from(test_to_address);
    std::string test_str = "000000000000000000000000000000000000000000000000000000000000000a000000000000000000000000a4ba11f3f36b12c71f2aef775583b306a3cf784a";
    std::string log_data = top::HexDecode(test_str);
    xbytes_t  data = xbytes_t(log_data.begin(), log_data.end());

    xeth_transaction_t tx(from_address, to_address, data, test_value, test_value_gas, test_value_gas);
    return tx;
}
#if 0
base::xauto_ptr<base::xvblock_t> create_new_relay_block(std::string& extra_data)
{
    data::xrelay_block *_relay_block = new data::xrelay_block();
        std::vector<xeth_transaction_t> tx_vector;
        for(int i = 0; i < 4; i ++) {
            tx_vector.push_back(xrelay_tx_create());
        }
        _relay_block->set_transactions(tx_vector);

    _relay_block->build_finish();

    xbytes_t rlp_genesis_block_header_data = _relay_block->encodeBytes(false);
    std::string data((char*)rlp_genesis_block_header_data.data(), rlp_genesis_block_header_data.size());
    data::xemptyblock_build_t bbuild(sys_contract_relay_block_addr);

    data::xtableheader_extra_t header_extra_src;
    header_extra_src.set_relay_block_data(data);
    std::string _extra_data;
    header_extra_src.serialize_to_string(_extra_data);

    xdbg("create_genesis_of_relay_account, %d", _extra_data.size());
    base::xauto_ptr<base::xvblock_t> _new_block = bbuild.build_new_block();
    _new_block->get_cert()->set_clock(RandomUint64()); 
    extra_data = _extra_data;
    return _new_block;   
}

TEST_F(test_relay_block, store_relay_block) {
    std::string extra_data;
    base::xauto_ptr<base::xvblock_t> _new_block = create_new_relay_block(extra_data);

    data::xrelayblock_build_t relay_build(_new_block.get(),
                                          extra_data,
                                          _new_block->get_header()->get_comments(),
                                          std::string(1,0),
                                          _new_block->get_height() + 1);
    base::xauto_ptr<base::xvblock_t> relay_block_ptr = relay_build.build_new_block();
    relay_block_ptr->get_cert()->set_viewtoken(RandomUint64());
    relay_block_ptr->set_verify_signature(std::string(1, 0));
    relay_block_ptr->set_block_flag(base::enum_xvblock_flag_authenticated);

    mock::xvchain_creator creator(true);
    base::xvblockstore_t* blockstore = creator.get_blockstore();
    base::xvaccount_t _table_addr(sys_contract_relay_block_addr);
    ASSERT_TRUE(blockstore->store_block(_table_addr, relay_block_ptr.get()));

    {
        auto _block = blockstore->load_block_object(_table_addr, 1, base::enum_xvblock_flag_authenticated, false);
        ASSERT_NE(_block, nullptr);
    }
    {
        auto _block = blockstore->load_block_object(_table_addr, 2, base::enum_xvblock_flag_authenticated, false);
        ASSERT_EQ(_block, nullptr);
    }
}
TEST_F(test_relay_block, store_relay_block2)
{
    std::string extra_data;
    base::xauto_ptr<base::xvblock_t> _new_block = create_new_relay_block(extra_data);

    data::xrelayblock_build_t relay_build(_new_block.get(),
                                          extra_data,
                                          _new_block->get_header()->get_comments(),
                                          std::string(1,0),
                                          _new_block->get_height() + 1);
    base::xauto_ptr<base::xvblock_t> relay_block_ptr = relay_build.build_new_block();
    ASSERT_NE(relay_block_ptr, nullptr);
    relay_block_ptr->get_cert()->set_viewtoken(RandomUint64());
    relay_block_ptr->set_verify_signature(std::string(1, 0));
    relay_block_ptr->set_block_flag(base::enum_xvblock_flag_authenticated);

    mock::xvchain_creator creator(true);
    base::xvblockstore_t* blockstore = creator.get_blockstore();
    base::xvaccount_t _table_addr(sys_contract_relay_block_addr);
    ASSERT_TRUE(blockstore->store_block(_table_addr, relay_block_ptr.get()));

    for (int i=0;i< 10; i++){
        data::xrelayblock_build_t relay_build(_new_block.get(), extra_data, _new_block->get_header()->get_comments(), std::string(1, 0), i + 2);
        base::xauto_ptr<base::xvblock_t> block_ptr = relay_build.build_new_block();
        ASSERT_NE(block_ptr, nullptr);
        block_ptr->get_cert()->set_viewtoken(RandomUint64());
        block_ptr->set_verify_signature(std::string(1, 0));
        block_ptr->set_block_flag(base::enum_xvblock_flag_authenticated);

        ASSERT_TRUE(blockstore->store_block(_table_addr, block_ptr.get()));
    }

    for (int i = 0; i < 12; i++) {
        auto _block = blockstore->load_block_object(_table_addr, i, base::enum_xvblock_flag_authenticated, false);
        ASSERT_NE(_block, nullptr);
    }
    {
        auto _block = blockstore->load_block_object(_table_addr, 13, base::enum_xvblock_flag_authenticated, false);
        ASSERT_EQ(_block, nullptr);
    }
    {
        auto _block = blockstore->load_block_object(_table_addr, 14, base::enum_xvblock_flag_authenticated, false);
        ASSERT_EQ(_block, nullptr);
    }
}

TEST_F(test_relay_block, relay_tx_test) {
    std::string extra_data;
    base::xauto_ptr<base::xvblock_t> _new_block = create_new_relay_block(extra_data);

    data::xrelayblock_build_t relay_build(_new_block.get(),
                                          extra_data,
                                          _new_block->get_header()->get_comments(),
                                          std::string(1,0),
                                          _new_block->get_height() + 1);
    base::xauto_ptr<base::xvblock_t> relay_block_ptr = relay_build.build_new_block();
    ASSERT_NE(relay_block_ptr, nullptr);
    relay_block_ptr->get_cert()->set_viewtoken(RandomUint64());
    relay_block_ptr->set_verify_signature(std::string(1, 0));
    relay_block_ptr->set_block_flag(base::enum_xvblock_flag_authenticated);

    mock::xvchain_creator creator(true);
    base::xvblockstore_t* blockstore = creator.get_blockstore();
    base::xvaccount_t _table_addr(sys_contract_relay_block_addr);
    ASSERT_TRUE(blockstore->store_block(_table_addr, relay_block_ptr.get()));

    for (int i=0;i< 10; i++){
        data::xrelayblock_build_t relay_build(_new_block.get(), extra_data,
            _new_block->get_header()->get_comments(), std::string(1, 0), i + 2);
        base::xauto_ptr<base::xvblock_t> block_ptr = relay_build.build_new_block();
        ASSERT_NE(block_ptr, nullptr);
        block_ptr->get_cert()->set_viewtoken(RandomUint64());
        block_ptr->set_verify_signature(std::string(1, 0));
        block_ptr->set_block_flag(base::enum_xvblock_flag_authenticated);

        ASSERT_TRUE(blockstore->store_block(_table_addr, block_ptr.get()));
    }

    for (int i = 0; i < 12; i++) {
        auto _block = blockstore->load_block_object(_table_addr, i, base::enum_xvblock_flag_authenticated, false);
        ASSERT_NE(_block, nullptr);
    }
    {
        auto _block = blockstore->load_block_object(_table_addr, 13, base::enum_xvblock_flag_authenticated, false);
        ASSERT_EQ(_block, nullptr);
    }
    {
        auto _block = blockstore->load_block_object(_table_addr, 14, base::enum_xvblock_flag_authenticated, false);
        ASSERT_EQ(_block, nullptr);
    }

}
base::xauto_ptr<base::xvblock_t> create_new_wrap_block(std::string& extra_data)
{
    data::xrelay_block *_relay_block = new data::xrelay_block();
        std::vector<xeth_transaction_t> tx_vector;
        for(int i = 0; i < 4; i ++) {
            tx_vector.push_back(xrelay_tx_create());
        }
        _relay_block->set_transactions(tx_vector);

    _relay_block->build_finish();

    xbytes_t rlp_genesis_block_header_data = _relay_block->encodeBytes(false);
    std::string data((char*)rlp_genesis_block_header_data.data(), rlp_genesis_block_header_data.size());
    data::xemptyblock_build_t bbuild(sys_contract_relay_table_block_addr);

    data::xtableheader_extra_t header_extra_src;
    header_extra_src.set_relay_block_data(data);
    std::string _extra_data;
    header_extra_src.serialize_to_string(_extra_data);

    xdbg("create_genesis_of_relay_account, %d", _extra_data.size());
    base::xauto_ptr<base::xvblock_t> _new_block = bbuild.build_new_block();
    _new_block->get_cert()->set_clock(RandomUint64()); 
    extra_data = _extra_data;
    return _new_block;   
}
TEST_F(test_relay_block, store_wrap_block)
{
    std::string extra_data;
    base::xauto_ptr<base::xvblock_t> _new_block = create_new_wrap_block(extra_data);

    data::xrelayblock_build_t relay_build(_new_block.get(),
                                          extra_data,
                                          _new_block->get_header()->get_comments(),
                                          std::string(1,0),
                                          _new_block->get_height() + 1);
    base::xauto_ptr<base::xvblock_t> relay_block_ptr = relay_build.build_new_block();
    ASSERT_NE(relay_block_ptr, nullptr);
    relay_block_ptr->get_cert()->set_viewtoken(RandomUint64());
    relay_block_ptr->set_verify_signature(std::string(1, 0));
    relay_block_ptr->set_block_flag(base::enum_xvblock_flag_authenticated);

    mock::xvchain_creator creator(true);
    base::xvblockstore_t* blockstore = creator.get_blockstore();
    base::xvaccount_t _table_addr(sys_contract_relay_block_addr);
    ASSERT_TRUE(blockstore->store_block(_table_addr, relay_block_ptr.get()));

    for (int i=0;i< 10; i++){
        data::xrelayblock_build_t relay_build(_new_block.get(), extra_data, _new_block->get_header()->get_comments(), std::string(1, 0), i + 2);
        base::xauto_ptr<base::xvblock_t> block_ptr = relay_build.build_new_block();
        ASSERT_NE(block_ptr, nullptr);
        block_ptr->get_cert()->set_viewtoken(RandomUint64());
        block_ptr->set_verify_signature(std::string(1, 0));
        block_ptr->set_block_flag(base::enum_xvblock_flag_authenticated);

        ASSERT_TRUE(blockstore->store_block(_table_addr, block_ptr.get()));
    }

    for (int i = 0; i < 12; i++) {
        auto _block = blockstore->load_block_object(_table_addr, i, base::enum_xvblock_flag_authenticated, false);
        ASSERT_NE(_block, nullptr);
    }
    {
        auto _block = blockstore->load_block_object(_table_addr, 13, base::enum_xvblock_flag_authenticated, false);
        ASSERT_EQ(_block, nullptr);
    }
    {
        auto _block = blockstore->load_block_object(_table_addr, 14, base::enum_xvblock_flag_authenticated, false);
        ASSERT_EQ(_block, nullptr);
    }
}
#endif



