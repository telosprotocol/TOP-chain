#include "gtest/gtest.h"

#include "xbase/xcontext.h"
#include "xbase/xdata.h"
#include "xbase/xobject.h"
#include "xbase/xmem.h"
#include "xbase/xcontext.h"
#include "xbase/xvledger.h"

#include "xdata/xdatautil.h"
#include "xdata/xemptyblock.h"
#include "xdata/xblocktool.h"
#include "xdata/xlightunit.h"
#include "xdata/tests/test_blockutl.hpp"

#include "test_blockmock.hpp"
#include "xstore/xstore.h"
#include "xblockstore/xblockstore_face.h"

using namespace top;
using namespace top::base;
using namespace top::store;
using namespace top::data;

class test_xblockstore : public testing::Test {
protected:
    void SetUp() override {
    }

    void TearDown() override {
    }
};

xaccount_cmd_ptr_t make_account_cmd_map(xstore_face_t* store, const std::string & address, const std::string & property_name = {}) {
    xaccount_ptr_t account = make_object_ptr<xaccount_t>(address);
    xaccount_cmd_ptr_t accountcmd1 = std::make_shared<xaccount_cmd>(account.get(), store);
    std::string name;
    if (property_name.empty()) {
        name = "aaa";
    } else {
        name = property_name;
    }

    auto ret = accountcmd1->map_create(name, true);
    xassert(ret == 0);
    ret = accountcmd1->map_set(name, "111", "value111");
    xassert(ret == 0);
    ret = accountcmd1->map_set(name, "222", "value222");
    xassert(ret == 0);
    return accountcmd1;
}

xvblock_t* create_sample_block(xstore_face_t* store, const std::string& address) {
    assert(store != nullptr);
    auto account = store->clone_account(address);
    base::xvblock_t* genesis_block = nullptr;
    if (account == nullptr) {
        genesis_block = test_blocktuil::create_genesis_empty_unit(address);
        account = new xblockchain2_t(address);
    }
    uint64_t amount = 100;
    std::string to_account = "T-to-xxxxxxxxxxxxxxxxxxxxx";
    xtransaction_ptr_t tx = account->make_transfer_tx(to_account, -amount, 0, 0, 0);

    xlightunit_block_para_t para;
    para.set_one_input_tx(tx);
    para.set_balance_change(amount);

    base::xvblock_t* vblock = nullptr;
    if (genesis_block) {
        vblock = test_blocktuil::create_next_lightunit_with_consensus(para, genesis_block);
    } else {
        vblock = test_blocktuil::create_next_lightunit(para, account);
    }
    assert(vblock);
    auto lightunit = dynamic_cast<xblock_t*>(vblock);
    assert(lightunit);

    std::cout << "address: " << address << ", make block height: " << lightunit->get_height() << ", last block hash: " << data::to_hex_str(lightunit->get_last_block_hash()) << std::endl;

    return vblock;
}

TEST_F(test_xblockstore, store_load_block) {
    xobject_ptr_t<store::xstore_face_t> store_face = store::xstore_factory::create_store_with_memdb();
    auto store = store_face.get();

    std::string address = xblocktool_t::make_address_user_account("11111111111111111112");
    xvblockstore_t* blockstore = xblockstorehub_t::instance().create_block_store(*store, address);

    test_blockmock_t blockmock(store_face.get());

    // must greater than enum_max_cached_blocks(32)
    uint64_t count = 100;

    base::xvblock_t *prev_block = (blockstore->get_genesis_block(address).get());
    base::xvblock_t *curr_block = nullptr;
    for (uint64_t i = 1; i <= count; i++) {

        std::string value = std::to_string(i);
        xaccount_cmd_ptr_t cmd = nullptr;
        curr_block = blockmock.create_sample_block(prev_block, cmd.get(), address);

        ASSERT_TRUE(blockstore->store_block(curr_block));
        ASSERT_EQ(curr_block->get_height(), i);
        uint64_t chainheight = store->get_blockchain_height(address);
        ASSERT_EQ(chainheight, curr_block->get_height());
        prev_block = curr_block;
    }

    for (uint64_t i = 0; i <= count; i++) {
        auto unit = blockstore->load_block_object(address, i);
        if (unit == nullptr) {
            std::cout << "i = " << i << std::endl;
        }
        ASSERT_NE(unit, nullptr);
        ASSERT_EQ(unit->get_height(), i);
    }

    std::cout << "query count: " << count + 1 << std::endl;
    std::cout << "cache count: " << blockstore->get_cache_size(address) << std::endl;
    EXPECT_EQ(32, blockstore->get_cache_size(address));
}

TEST_F(test_xblockstore, create_units_and_tableblock) {
    xobject_ptr_t<store::xstore_face_t> store_face = store::xstore_factory::create_store_with_memdb();
    auto store = store_face.get();
    std::string taccount1 = data::xblocktool_t::make_address_table_account(base::enum_chain_zone_beacon_index, 0);

    xvblockstore_t* blockstore = xblockstorehub_t::instance().create_block_store(*store, taccount1);

    test_blockmock_t blockmock(store_face.get());
    uint64_t count = 10;

    std::map<std::string, std::string> prop_list;

    // create unit1
    std::string account1 = xblocktool_t::make_address_user_account("21111111111111111111");
    auto lightunit1 = blockmock.create_unit(account1, prop_list);

    // create unit2
    std::string account2 = xblocktool_t::make_address_user_account("21111111111111111112");
    auto lightunit2 = blockmock.create_unit(account1, prop_list);

    // create table-block
    xtable_block_para_t table_para;
    table_para.add_unit(lightunit1);
    table_para.add_unit(lightunit2);

    base::xvblock_t* taccount1_genesis_block = test_blocktuil::create_genesis_empty_table(taccount1);
    auto taccount1_proposal_block = test_blocktuil::create_next_tableblock(table_para, taccount1_genesis_block);

    // then make units from tableblock
    xtable_block_t* tableblock = dynamic_cast<xtable_block_t*>(taccount1_proposal_block);
    auto & cache_units = tableblock->get_tableblock_units(true);
    for (auto & cache_unit : cache_units) {

        base::xstream_t stream(base::xcontext_t::instance());
        auto ret = cache_unit->full_serialize_to(stream);
        ASSERT_TRUE(0 != ret);
        ASSERT_FALSE(cache_unit->get_block_hash().empty());
    }
    ASSERT_FALSE(taccount1_proposal_block->get_block_hash().empty());
    ASSERT_NE(taccount1_proposal_block, nullptr);
    ASSERT_EQ(tableblock->get_txs_count(), 2);

    ASSERT_TRUE(blockstore->store_block(taccount1_proposal_block));

    auto temp_block = store->get_block_by_height(taccount1, 1);
    ASSERT_NE(temp_block, nullptr);
}


TEST_F(test_xblockstore, disorder_store_unit_1) {
    xobject_ptr_t<store::xstore_face_t> store_face = store::xstore_factory::create_store_with_memdb();
    base::xauto_ptr<xvblockstore_t> blockstore(xblockstorehub_t::instance().create_block_store(*store_face.get(), {}));

    std::string account1 = xblocktool_t::make_address_user_account("21111111111111111111");
    base::xauto_ptr<base::xvblock_t> unit0(test_blocktuil::create_genesis_empty_unit(account1));
    xassert(true == blockstore->store_block(unit0.get()));
    xfullunit_block_para_t full_para;
    xlightunit_block_para_t light_para;
    xtransaction_ptr_t tx = make_object_ptr<xtransaction_t>();
    tx->set_digest();
    light_para.set_one_input_tx(tx);
    base::xauto_ptr<base::xvblock_t> unit1(test_blocktuil::create_next_lightunit_with_consensus(light_para, unit0.get()));
    base::xauto_ptr<base::xvblock_t> unit2(test_blocktuil::create_next_fullunit_with_consensus(full_para, unit1.get()));
    base::xauto_ptr<base::xvblock_t> unit3(test_blocktuil::create_next_lightunit_with_consensus(light_para, unit2.get()));
    base::xauto_ptr<base::xvblock_t> unit4(test_blocktuil::create_next_fullunit_with_consensus(full_para, unit3.get()));
    base::xauto_ptr<base::xvblock_t> unit5(test_blocktuil::create_next_lightunit_with_consensus(light_para, unit4.get()));
    xassert(true == blockstore->store_block(unit3.get()));
    xassert(true == blockstore->store_block(unit4.get()));
    xassert(true == blockstore->store_block(unit1.get()));
    xassert(true == blockstore->store_block(unit2.get()));
}

TEST_F(test_xblockstore, load_block_object_1) {
    xobject_ptr_t<store::xstore_face_t> store_face = store::xstore_factory::create_store_with_memdb();
    base::xauto_ptr<xvblockstore_t> blockstore(xblockstorehub_t::instance().create_block_store(*store_face.get(), {}));

    std::string account1 = xblocktool_t::make_address_user_account("21111111111111111111");
    base::xauto_ptr<base::xvblock_t> unit0(test_blocktuil::create_genesis_empty_unit(account1));
    xassert(true == blockstore->store_block(unit0.get()));
    xlightunit_block_para_t light_para;
    xtransaction_ptr_t tx = make_object_ptr<xtransaction_t>();
    tx->set_digest();
    light_para.set_one_input_tx(tx);

    base::xauto_ptr<base::xvblock_t> unit1(test_blocktuil::create_next_lightunit_with_consensus(light_para, unit0.get()));
    unit1->reset_block_flags();
    unit1->set_block_flag(base::enum_xvblock_flag_authenticated);

    xassert(true == blockstore->store_block(unit1.get()));
    auto read_block = blockstore->load_block_object(account1, 1);
    xassert(read_block == nullptr);
    auto read_block_2 = blockstore->get_latest_cert_block(account1);
    xassert(read_block_2 != nullptr);
    xassert(read_block_2->get_height() == 1);
}

// blockstore can't store non-empty block
TEST_F(test_xblockstore, DISABLED_store_genesis_unit) {
    std::string address = xblocktool_t::make_address_user_account("11111111111111111112");

    base::xauto_ptr<base::xvblock_t> unit0(test_blocktuil::create_genesis_empty_unit(address));

    uint64_t init_balance = 100000000000;
    base::xvblock_t* genesis_block = data::xblocktool_t::create_genesis_lightunit(address, init_balance);
    base::xauto_ptr<base::xvblock_t> auto_genesis_block(genesis_block);
    xassert(genesis_block);

    xobject_ptr_t<store::xstore_face_t> store_face = store::xstore_factory::create_store_with_memdb();
    auto store = store_face.get();
    xvblockstore_t* blockstore = xblockstorehub_t::instance().create_block_store(*store, address);

    auto ret = blockstore->store_block(genesis_block);
    // TODO(jimmy) ASSERT_TRUE(ret);
}

TEST_F(test_xblockstore, account_create_time_1) {
    std::string address = xblocktool_t::make_address_user_account("11111111111111111112");

    base::xauto_ptr<base::xvblock_t> unit0(test_blocktuil::create_genesis_empty_unit(address));

    uint64_t init_balance = 100000000000;
    base::xvblock_t* genesis_block = data::xblocktool_t::create_genesis_lightunit(address, init_balance);
    base::xauto_ptr<base::xvblock_t> auto_genesis_block(genesis_block);
    xassert(genesis_block);

    xobject_ptr_t<store::xstore_face_t> store_face = store::xstore_factory::create_store_with_memdb();
    auto ret = store_face->set_vblock(genesis_block);
    ASSERT_TRUE(ret);
    ret = store_face->execute_block(genesis_block);
    ASSERT_TRUE(ret);
    auto account = store_face->clone_account(address);
    ASSERT_EQ(account->get_account_create_time(), base::TOP_BEGIN_GMTIME);
}
TEST_F(test_xblockstore, account_create_time_2) {
    std::string address = xblocktool_t::make_address_user_account("11111111111111111112");

    base::xvblock_t* genesis_block = data::xblocktool_t::create_genesis_empty_unit(address);
    base::xauto_ptr<base::xvblock_t> auto_genesis_block(genesis_block);
    xassert(genesis_block);

    xobject_ptr_t<store::xstore_face_t> store_face = store::xstore_factory::create_store_with_memdb();
    auto store = store_face.get();
    xvblockstore_t* blockstore = xblockstorehub_t::instance().create_block_store(*store, address);

    xlightunit_block_para_t light_para;
    xtransaction_ptr_t tx = make_object_ptr<xtransaction_t>();
    tx->set_digest();
    light_para.set_one_input_tx(tx);

    base::xauto_ptr<base::xvblock_t> unit1(test_blocktuil::create_next_lightunit_with_consensus(light_para, genesis_block));
    // unit1->reset_block_flags();
    // unit1->set_block_flag(base::enum_xvblock_flag_authenticated);
    xassert(true == blockstore->store_block(unit1.get()));

    auto account = store_face->clone_account(address);
    ASSERT_EQ(account->get_account_create_time(), unit1->get_cert()->get_gmtime());
}
