#include "gtest/gtest.h"
#include "xpbase/base/top_utils.h"
#include "xbase/xcontext.h"
#include "xbase/xdata.h"
#include "xbase/xobject.h"
#include "xbase/xmem.h"
#include "xbase/xcontext.h"
#include "xbasic/xhex.h"
#include "xdata/xdatautil.h"
#include "xdata/xemptyblock.h"
#include "xdata/xblocktool.h"
#include "xdata/xlightunit.h"
#include "xdata/xnative_contract_address.h"
#include "xdata/xblockextract.h"
#include "xmetrics/xmetrics.h"
#include "xdata/xverifier/xtx_verifier.h"
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

data::xrelay_block create_tx_relayblock(uint64_t height, uint64_t viewid, xeth_transaction_t const& tx) {
    evm_common::h256  prev_hash;
    uint64_t clock = 8676012;
    evm_common::u256 chain_bits = 1;
    std::vector<xeth_transaction_t> transactions;
    transactions.push_back(tx);
    xeth_receipt_t receipt;
    std::vector<xeth_receipt_t> receipts;
    receipts.push_back(receipt);
    data::xrelay_block relay_block = data::xrelay_block(prev_hash, height, base::clock_to_gmtime(clock), chain_bits, transactions, receipts);    
    relay_block.build_finish();
    relay_block.set_viewid(viewid);
    return relay_block;    
}

data::xrelay_block create_tx_relayblock(uint64_t height, uint64_t viewid) {
    xeth_transaction_t tx = xrelay_tx_create();
    return create_tx_relayblock(height, viewid, tx);    
}

xobject_ptr_t<base::xvblock_t> create_new_relay_block(uint64_t height, std::string& extra_data)
{
    evm_common::h256  prev_hash;
    uint64_t clock = 8676012;
    evm_common::u256 chain_bits = 1;
    xeth_transaction_t tx = xrelay_tx_create();
    std::vector<xeth_transaction_t> transactions;
    transactions.push_back(tx);
    xeth_receipt_t receipt;
    std::vector<xeth_receipt_t> receipts;
    receipts.push_back(receipt);
    data::xrelay_block relay_block = data::xrelay_block(prev_hash, height, base::clock_to_gmtime(clock), chain_bits, transactions, receipts);

    std::error_code ec;
    xobject_ptr_t<base::xvblock_t> wrapblock = data::xblockextract_t::pack_relayblock_to_wrapblock(relay_block, ec);
    xassert(wrapblock != nullptr);
    return wrapblock;   
}

TEST_F(test_relay_block, store_relay_block_2) {
    mock::xvchain_creator creator(true);
    base::xvblockstore_t* blockstore = creator.get_blockstore();
    base::xvaccount_t _table_addr(sys_contract_relay_block_addr);

    base::xvchain_t::instance().get_xtxstore()->update_node_type((uint32_t)(common::xnode_type_t::storage));

    std::error_code ec;
    xeth_transaction_t ethtx = xrelay_tx_create();
    data::xrelay_block rblock1 = create_tx_relayblock(100, 100, ethtx);
    uint256_t txhash = ethtx.get_tx_hash();
    std::cout << "txhash=" << top::to_hex(top::to_bytes(txhash)) << std::endl;

    std::string rblock_hash = top::to_string(rblock1.get_block_hash().to_bytes());
    std::cout << "rblockhash=" << top::to_hex(rblock1.get_block_hash().to_bytes()) << std::endl;

    xobject_ptr_t<base::xvblock_t> wrapblock1 = data::xblockextract_t::pack_relayblock_to_wrapblock(rblock1, ec);
    xassert(wrapblock1 != nullptr);

    ASSERT_TRUE(blockstore->store_block(_table_addr, wrapblock1.get()));    

    {
        base::enum_transaction_subtype type = base::enum_transaction_subtype_send;
        std::string txhash_str = top::to_string(top::to_bytes(txhash));
        base::xauto_ptr<base::xvtxindex_t> txindex = base::xvchain_t::instance().get_xtxstore()->load_relay_tx_idx(txhash_str, type);
        xassert(nullptr != txindex);

        base::xvaccount_t _vaddress(txindex->get_block_addr());
        auto _block = base::xvchain_t::instance().get_xblockstore()->load_block_object(_vaddress, txindex->get_block_height(), txindex->get_block_hash(), false);
        xassert(nullptr != _block);

        top::data::xrelay_block  extra_relay_block;
        data::xblockextract_t::unpack_relayblock_from_wrapblock(_block.get(), extra_relay_block, ec);    
        if (ec) {
            xerror("xrpc_loader_t:load_relay_tx_indx_detail decodeBytes decodeBytes error %s; err msg %s", ec.category().name(), ec.message().c_str());
        }
        bool findtx = false;
        for (auto & tx : extra_relay_block.get_all_transactions()) {
            if (tx.get_tx_hash() == txhash) {
                findtx = true;
                break;
            }
        }
        xassert(findtx);
    }

    {
        auto _wrapblock = base::xvchain_t::instance().get_xblockstore()->get_block_by_hash(rblock_hash);
        xassert(nullptr != _wrapblock);
        top::data::xrelay_block  relay_block2;
        data::xblockextract_t::unpack_relayblock_from_wrapblock(_wrapblock.get(), relay_block2, ec);    
        if (ec) {
            xassert(false);
        }
    }
}

TEST_F(test_relay_block, store_relay_block) {
    mock::xvchain_creator creator(true);
    base::xvblockstore_t* blockstore = creator.get_blockstore();
    base::xvaccount_t _table_addr(sys_contract_relay_block_addr);

    std::error_code ec;

    data::xrelay_block rblock1 = create_tx_relayblock(100, 100);
    data::xrelay_block rblock1_2 = create_tx_relayblock(100, 100);
    data::xrelay_block rblock2 = create_tx_relayblock(100, 101);  // fork

    ASSERT_EQ(rblock1.get_viewid(), 100);
    ASSERT_EQ(rblock2.get_viewid(), 101);
    ASSERT_EQ(rblock1.get_block_height(), 100);
    ASSERT_EQ(rblock1.get_block_hash(), rblock1_2.get_block_hash());
    ASSERT_EQ(rblock1.get_block_hash(), rblock2.get_block_hash());
    ASSERT_EQ(rblock1.get_viewid(), rblock1_2.get_viewid());
    ASSERT_NE(rblock1.get_viewid(), rblock2.get_viewid());

    xobject_ptr_t<base::xvblock_t> wrapblock1 = data::xblockextract_t::pack_relayblock_to_wrapblock(rblock1, ec);
    xassert(wrapblock1 != nullptr);
    xobject_ptr_t<base::xvblock_t> wrapblock1_2 = data::xblockextract_t::pack_relayblock_to_wrapblock(rblock1_2, ec);
    xassert(wrapblock1_2 != nullptr);
    xobject_ptr_t<base::xvblock_t> wrapblock2 = data::xblockextract_t::pack_relayblock_to_wrapblock(rblock2, ec);
    xassert(wrapblock2 != nullptr);
    ASSERT_EQ(wrapblock1->get_block_hash(), wrapblock1_2->get_block_hash());
    ASSERT_NE(wrapblock1->get_block_hash(), wrapblock2->get_block_hash());
    ASSERT_EQ(wrapblock1->get_height(), wrapblock2->get_height());
    ASSERT_NE(wrapblock1->get_viewid(), wrapblock2->get_viewid());

    std::string rblock1_hash_str = top::to_string(rblock1.get_block_hash().to_bytes());
    ASSERT_NE(wrapblock1->get_block_hash(), rblock1_hash_str);

    ASSERT_TRUE(blockstore->store_block(_table_addr, wrapblock1.get()));    
    ASSERT_TRUE(blockstore->store_block(_table_addr, wrapblock2.get()));

    {
        auto _blocks = blockstore->load_block_object(_table_addr, 100);
        ASSERT_EQ(_blocks.get_vector().size(), 2);
    }

    {
        auto _block = blockstore->load_block_object(_table_addr, 100, base::enum_xvblock_flag_authenticated, false);
        ASSERT_NE(_block, nullptr);

        data::xrelay_block rblock_read;
        data::xblockextract_t::unpack_relayblock_from_wrapblock(_block.get(), rblock_read, ec);
        ASSERT_EQ(rblock_read.get_block_hash(), rblock2.get_block_hash());
        ASSERT_EQ(rblock_read.get_block_height(), 100);
        ASSERT_EQ(rblock_read.get_viewid(), 101);
    }
}

#if 0

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

    base::xtableheader_extra_t header_extra_src;
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



