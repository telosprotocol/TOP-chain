#include <map>
#include "gtest/gtest.h"
#include "xbase/xmem.h"
#include "xbase/xcontext.h"
// TODO(jimmy) #include "xbase/xvledger.h"
#include "xbase/xobject.h"
#include "xdata/xblock.h"
#include "xdata/xlightunit.h"
#include "xdata/xfullunit.h"
#include "xdata/xemptyblock.h"
#include "xdata/xblockchain.h"
#include "xdata/xtableblock.h"
#include "xdata/xblocktool.h"
#include "xbase/xhash.h"
#include "xtxpool_v2/xtxpool.h"
#include "xstore/xstore_face.h"
#include "xblockstore/test/test_blockstore_datamock.hpp"
#include "xdata/tests/test_blockutl.hpp"
#include "test_xtxexecutor_util.hpp"

using namespace top;
using namespace top::data;
using namespace top::xtxpool_v2;
using namespace top::store;

class test_basic_case : public testing::Test {
 protected:
    void SetUp() override {
    }

    void TearDown() override {
    }
};

xcons_transaction_ptr_t to_constx(const xtransaction_ptr_t & tx) {
    return make_object_ptr<xcons_transaction_t>(tx.get());;
}

void create_god_account(const xstore_face_ptr_t & store, const std::string & account) {
    auto genesis_block = xblocktool_t::create_genesis_lightunit(account, 1000000000);
    xassert(genesis_block != nullptr);
    xassert(true == store->set_vblock(genesis_block));
    xassert(true == store->execute_block(genesis_block));
}

TEST_F(test_basic_case, god_account_1) {
    xobject_ptr_t<xstore_face_t> store = xstore_factory::create_store_with_memdb();
    auto address = xblocktool_t::make_address_user_account("11111111111111111");
    auto genesis_block = xblocktool_t::create_genesis_lightunit(address, 1000000000);
    xassert(genesis_block != nullptr);
    std::cout << "genesis block " << data::to_hex_str(genesis_block->get_block_hash()) << std::endl;
    store->set_vblock(genesis_block);
    store->execute_block(genesis_block);
    auto account_ptr = store->clone_account(address);
    xassert(account_ptr != nullptr);
    xassert(account_ptr->balance() == 1000000000);
}

TEST_F(test_basic_case, god_account_2) {
    xobject_ptr_t<xstore_face_t> store = xstore_factory::create_store_with_memdb();
    auto address = xblocktool_t::make_address_user_account("11111111111111111");
    auto genesis_block = xblocktool_t::create_genesis_empty_unit(address);

    uint64_t amount = 100;
    xtransaction_ptr_t tx = make_object_ptr<xtransaction_t>();
    tx->set_digest();
    xlightunit_block_para_t para;
    para.set_one_input_tx(tx);
    para.set_balance_change(amount);

    base::xvblock_t* proposal_block = test_blocktuil::create_next_lightunit_with_consensus(para, genesis_block);
    // disorder set genesis block, only for empty genesis block
    store->set_vblock(genesis_block);
    store->execute_block(genesis_block);
    store->set_vblock(proposal_block);
    proposal_block->set_block_flag(base::enum_xvblock_flag_connected);
    xassert(true == store->execute_block(proposal_block));
    auto account_ptr = store->clone_account(address);
    xassert(account_ptr != nullptr);
    xassert(account_ptr->balance() == amount);
}

TEST_F(test_basic_case, xvaccount_1) {
    {
        uint16_t ledger_id = base::xvaccount_t::make_ledger_id(base::enum_main_chain_id, base::enum_chain_zone_consensus_index);
        std::string addr = base::xvaccount_t::make_account_address(base::enum_vaccount_addr_type_secp256k1_user_account,
                                                                   ledger_id,
                                                                   "1234567890abcdef");
        std::cout << addr << std::endl;
        ASSERT_EQ(addr, "T000001234567890abcdef");
    }
    {
        uint16_t ledger_id = base::xvaccount_t::make_ledger_id(base::enum_main_chain_id, base::enum_chain_zone_consensus_index);
        std::string addr = base::xvaccount_t::make_account_address(base::enum_vaccount_addr_type_secp256k1_user_account,
                                                                    ledger_id,
                                                                    "1234567890abcdef",
                                                                    0);
        std::cout << addr << std::endl;
        ASSERT_EQ(addr, "T000001234567890abcdef@0");
    }
    {
        uint16_t ledger_id = base::xvaccount_t::make_ledger_id(base::enum_main_chain_id, base::enum_chain_zone_consensus_index);
        std::string addr = base::xvaccount_t::make_account_address(base::enum_vaccount_addr_type_secp256k1_user_account,
                                                                    ledger_id,
                                                                    "1234567890abcdef",
                                                                    100);
        std::cout << addr << std::endl;
        ASSERT_EQ(addr, "T000001234567890abcdef@100");
    }
    {
        uint16_t ledger_id = base::xvaccount_t::make_ledger_id(base::enum_main_chain_id, base::enum_chain_zone_consensus_index);
        std::string addr = base::xvaccount_t::make_account_address(base::enum_vaccount_addr_type_secp256k1_user_account, ledger_id, "1234567890abcdef");
        std::cout << addr << std::endl;
        ASSERT_EQ(addr, "T000001234567890abcdef");

        {
        ASSERT_EQ(base::xvaccount_t::get_addrtype_from_account(addr), base::enum_vaccount_addr_type_secp256k1_user_account);
        ASSERT_EQ(base::xvaccount_t::get_ledgerid_from_account(addr), ledger_id);
        ASSERT_EQ(base::xvaccount_t::get_chainid_from_ledgerid(ledger_id), base::enum_main_chain_id);
        ASSERT_EQ(base::xvaccount_t::get_zoneindex_from_ledgerid(ledger_id), base::enum_chain_zone_consensus_index);
        }

        {
        auto xid = base::xvaccount_t::get_xid_from_account(addr);
        ASSERT_EQ(get_vledger_chain_id(xid), base::enum_main_chain_id);
        ASSERT_EQ(get_vledger_zone_index(xid), base::enum_chain_zone_consensus_index);
        std::cout << "book index " << (uint32_t)get_vledger_book_index(xid) << std::endl;
        std::cout << "table index " << (uint32_t)get_vledger_table_index(xid) << std::endl;
        std::cout << "subaddr index " << (uint32_t)get_vledger_subaddr(xid) << std::endl;
        }
    }
}


TEST_F(test_basic_case, get_block_hash_2) {
    xobject_ptr_t<xstore_face_t> store_face = xstore_factory::create_store_with_memdb();
    std::string address = xblocktool_t::make_address_user_account("11111111111111111");
    std::string to_account = xblocktool_t::make_address_user_account("11111111111111112");
    {
        base::xvblock_t* genesis_block = data::xblocktool_t::create_genesis_empty_unit(address);
        ASSERT_TRUE(test_xtxexecutor_util_t::save_and_execute_block(store_face.get(), genesis_block));

        auto account = store_face->clone_account(address);
        xaccount_cmd accountcmd1(account, store_face.get());
        auto ret = accountcmd1.list_create("aaa", true);
        ASSERT_EQ(ret, 0);
        ret = accountcmd1.list_push_back("aaa", "111", true);
        ASSERT_EQ(ret, 0);
        ret = accountcmd1.list_push_back("aaa", "222", true);
        ASSERT_EQ(ret, 0);
        xproperty_log_ptr_t binlog = accountcmd1.get_property_log();
        ASSERT_NE(binlog, nullptr);
        accountcmd1.get_change_property();
        auto prop_hashs = accountcmd1.get_property_hash();
        ASSERT_EQ(prop_hashs.size(), 1);

        uint64_t amount = 100;
        xtransaction_ptr_t tx = make_object_ptr<xtransaction_t>();
        data::xproperty_asset asset(amount);
        tx->make_tx_transfer(asset);
        tx->set_different_source_target_address(address, to_account);
        tx->set_digest();

        xlightunit_block_para_t para;
        para.set_one_input_tx(tx);
        para.set_balance_change(-amount);
        para.set_property_log(accountcmd1.get_property_log());
        para.set_propertys_change(accountcmd1.get_property_hash());

        base::xvblock_t* proposal_block = test_blocktuil::create_next_lightunit_with_consensus(para, genesis_block);
        data::xlightunit_block_t* b = dynamic_cast<data::xlightunit_block_t*>(proposal_block);
        std::cout << "light unit property size: " << b->get_property_hash_map().size() << std::endl;

        ASSERT_TRUE(test_xtxexecutor_util_t::save_and_execute_block(store_face.get(), proposal_block));

        std::vector<std::string> values;
        ret = store_face->list_get_all(address, "aaa", values);
        ASSERT_EQ(ret, 0);
        ASSERT_EQ(values[0], "111");
        ASSERT_EQ(values[1], "222");
    }
}

TEST_F(test_basic_case, create_cache_normal_case_2) {
    xobject_ptr_t<xstore_face_t> store = xstore_factory::create_store_with_memdb();
    xobject_ptr_t<base::xvblockstore_t> blockstore;
    blockstore.attach(xblockstorehub_t::instance().create_block_store(*store, ""));

    std::string sender = base::xvaccount_t::make_account_address(base::enum_vaccount_addr_type_secp256k1_user_account, 0, "1234567890abcde2");
    std::string receiver = base::xvaccount_t::make_account_address(base::enum_vaccount_addr_type_secp256k1_user_account, 0, "1234567890abcde3");

    base::xvblock_t* genesis_block = data::xblocktool_t::create_genesis_empty_unit(sender);
    //ASSERT_TRUE(blockstore->store_block(genesis_block));
    ASSERT_TRUE(store->set_vblock(genesis_block));
    ASSERT_TRUE(store->execute_block(genesis_block));

    auto account = store->clone_account(sender);
    xassert(account != nullptr);

    uint64_t amount = 100;
    xtransaction_ptr_t tx = account->make_transfer_tx(receiver, amount, 0, 0, 0);

    xlightunit_block_para_t para;
    para.set_one_input_tx(tx);
    para.set_balance_change(-amount);

    // create receipt by block directly
    base::xvblock_t* proposal_block = test_blocktuil::create_next_lightunit(para, account);
    auto block = dynamic_cast<data::xblock_t*>(proposal_block);
    data::xlightunit_block_t* lightunit = dynamic_cast<data::xlightunit_block_t*>(block);
    std::vector<xcons_transaction_ptr_t> sendtx_receipts;
    std::vector<xcons_transaction_ptr_t> recvtx_receipts;
    lightunit->create_txreceipts(sendtx_receipts, recvtx_receipts);
    ASSERT_EQ(false, sendtx_receipts.empty());
    std::vector<xcons_transaction_ptr_t> sendtx_receipts_1;
    lightunit->create_send_txreceipts(sendtx_receipts_1);
    ASSERT_EQ(false, sendtx_receipts_1.empty());
    ASSERT_EQ(sendtx_receipts.size(), sendtx_receipts_1.size());

    ASSERT_EQ(lightunit->get_height(), 1);
    ASSERT_TRUE(blockstore->store_block(proposal_block));

    //read from store and create receipt
    base::xauto_ptr<base::xvblock_t> unit = blockstore->load_block_object(sender, 1);
    ASSERT_EQ(unit->get_height(), 1);

    data::xlightunit_block_t* lightunit2 = dynamic_cast<data::xlightunit_block_t*>(unit.get());
    ASSERT_FALSE(lightunit2->get_txs().empty());
    std::vector<xcons_transaction_ptr_t> sendtx_receipts2;
    std::vector<xcons_transaction_ptr_t> recvtx_receipts2;
    lightunit2->create_txreceipts(sendtx_receipts2, recvtx_receipts2);
    ASSERT_EQ(1, sendtx_receipts2.size());
}

TEST_F(test_basic_case, make_block_fullunit_1) {
    xobject_ptr_t<xstore_face_t> store = xstore_factory::create_store_with_memdb();
    std::string god_account = xblocktool_t::make_address_user_account("11111111111111111");
    create_god_account(store, god_account);
    base::xauto_ptr<base::xvblock_t> genesis_block(store->get_vblock(god_account, 0));

    base::xauto_ptr<xblockchain2_t> blockchain(store->clone_account(god_account));
    xassert(!blockchain->get_last_full_unit_hash().empty());
    auto fullunit = test_blocktuil::create_next_fullunit(blockchain.get());
    xassert(!fullunit->get_block_hash().empty());
    xassert(!fullunit->get_header()->get_last_full_block_hash().empty());
    xassert(fullunit->get_header()->get_last_full_block_hash() == genesis_block->get_block_hash());
    ASSERT_TRUE(fullunit->is_valid());
    ASSERT_TRUE(fullunit->is_valid(true));
    auto ret = store->set_vblock(fullunit);
    ASSERT_TRUE(ret);
}

TEST_F(test_basic_case, blockstore_store_block_1) {
    xobject_ptr_t<xstore_face_t> store = xstore_factory::create_store_with_memdb();
    xobject_ptr_t<base::xvblockstore_t> blockstore;
    blockstore.attach(xblockstorehub_t::instance().create_block_store(*store, ""));

    std::string god_account = xblocktool_t::make_address_user_account("11111111111111111");
    create_god_account(store, god_account);
    base::xauto_ptr<base::xvblock_t> genesis_block_1(store->get_vblock(god_account, 0));
    auto genesis_block_2 = blockstore->get_latest_committed_block(god_account);
    xassert(genesis_block_1->get_block_hash() == genesis_block_2->get_block_hash());

    base::xauto_ptr<xblockchain2_t> blockchain(store->clone_account(god_account));
    auto fullunit = test_blocktuil::create_next_fullunit(blockchain.get());
    auto ret = blockstore->store_block(fullunit);
    xassert(ret == true);
    base::xauto_ptr<xblockchain2_t> blockchain2(store->clone_account(god_account));
    xassert(blockchain2->get_chain_height() == 1);
}

TEST_F(test_basic_case, blockstore_store_block_2) {
    xobject_ptr_t<xstore_face_t> store = xstore_factory::create_store_with_memdb();
    xobject_ptr_t<base::xvblockstore_t> blockstore;
    blockstore.attach(xblockstorehub_t::instance().create_block_store(*store, ""));

    std::string god_account = xblocktool_t::make_address_user_account("11111111111111111");

    base::xvblock_t* genesis_block_1 = test_blocktuil::create_genesis_empty_unit(god_account);
    base::xauto_ptr<base::xvblock_t> genesis_block_ptr(genesis_block_1);
    xblockchain2_t* blockchain = new xblockchain2_t(god_account);
    auto ret = blockchain->update_state_by_genesis_block((xblock_t*)genesis_block_1);
    xassert(ret == true);
    auto fullunit = test_blocktuil::create_next_fullunit(blockchain);
    // should auto recreate genesis block when store a commited block by sync
    ret = blockstore->store_block(fullunit);
    xassert(ret == true);
    base::xauto_ptr<xblockchain2_t> blockchain2(store->clone_account(god_account));
    xassert(blockchain2->get_chain_height() == 1);
}

static xcons_transaction_ptr_t create_cons_transfer_tx(const std::string & from, const std::string & to, uint64_t amount = 100) {
    xtransaction_ptr_t tx = make_object_ptr<xtransaction_t>();
    data::xproperty_asset asset(amount);
    tx->make_tx_transfer(asset);
    tx->set_different_source_target_address(from, to);
    tx->set_digest();
    tx->set_tx_subtype(enum_transaction_subtype_send);

    xcons_transaction_ptr_t cons_tx = make_object_ptr<xcons_transaction_t>(tx.get());
    return cons_tx;
}

TEST_F(test_basic_case, create_units_and_tableblock) {
    xobject_ptr_t<store::xstore_face_t> store = store::xstore_factory::create_store_with_memdb();

    // create unit1
    std::string account1 = xblocktool_t::make_address_user_account("11111111111111111111");
    base::xvblock_t* account1_genesis_block = test_blocktuil::create_genesis_empty_unit(account1);
    xcons_transaction_ptr_t account1_tx1 = create_cons_transfer_tx(account1, "T000001111111111111111");
    xcons_transaction_ptr_t account1_tx2 = create_cons_transfer_tx(account1, "T000001111111111111111");
    xlightunit_block_para_t para1;
    para1.set_one_input_tx(account1_tx1);
    para1.set_one_input_tx(account1_tx2);
    xblock_t* lightunit1 = (xblock_t*)test_blocktuil::create_next_lightunit(para1, account1_genesis_block);

    // create unit2
    std::string account2 = xblocktool_t::make_address_user_account("11111111111111111112");
    base::xvblock_t* account2_genesis_block = test_blocktuil::create_genesis_empty_unit(account2);
    xcons_transaction_ptr_t account2_tx1 = create_cons_transfer_tx(account2, "T000001111111111111111");
    xcons_transaction_ptr_t account2_tx2 = create_cons_transfer_tx(account2, "T000001111111111111111");
    xlightunit_block_para_t para2;
    para2.set_one_input_tx(account2_tx1);
    para2.set_one_input_tx(account2_tx2);
    xblock_t* lightunit2 = (xblock_t*)test_blocktuil::create_next_lightunit(para2, account2_genesis_block);

    // create table-block
    xtable_block_para_t table_para;
    table_para.add_unit(lightunit1);
    table_para.add_unit(lightunit2);

    std::string taccount1 = data::xblocktool_t::make_address_table_account(base::enum_chain_zone_beacon_index, 0);
    base::xvblock_t* taccount1_genesis_block = test_blocktuil::create_genesis_empty_table(taccount1);
    auto taccount1_proposal_block = test_blocktuil::create_next_tableblock(table_para, taccount1_genesis_block);
    auto next_block = test_blocktuil::create_next_emptyblock(taccount1_proposal_block);
    auto next_next_block = test_blocktuil::create_next_emptyblock_with_justify(next_block, taccount1_proposal_block);

    // then make units from tableblock
    xtable_block_t* tableblock = dynamic_cast<xtable_block_t*>(taccount1_proposal_block);
    // auto & cache_units = tableblock->get_consensused_units();
    // for (auto & cache_unit : cache_units) {
    //     ASSERT_EQ(cache_unit->get_cert()->get_parent_block_height(), tableblock->get_height());
    //     base::xstream_t stream(base::xcontext_t::instance());
    //     auto ret = cache_unit->full_serialize_to(stream);
    //     ASSERT_TRUE(0 != ret);
    //     ASSERT_FALSE(cache_unit->get_block_hash().empty());
    // }
    ASSERT_FALSE(taccount1_proposal_block->get_block_hash().empty());

    ASSERT_TRUE(store->set_vblock(taccount1_proposal_block));
    ASSERT_NE(taccount1_proposal_block, nullptr);
    ASSERT_EQ(tableblock->get_txs_count(), 4);

    auto temp_block = store->get_block_by_height(taccount1, 1);
    ASSERT_NE(temp_block, nullptr);
}

TEST_F(test_basic_case, blockstore_store_block_3) {
    xobject_ptr_t<xstore_face_t> store = xstore_factory::create_store_with_memdb();
    xobject_ptr_t<base::xvblockstore_t> blockstore;
    blockstore.attach(xblockstorehub_t::instance().create_block_store(*store, ""));

    // create unit1
    std::string account1 = xblocktool_t::make_address_user_account("11111111111111111111");
    base::xvblock_t* account1_genesis_block = test_blocktuil::create_genesis_empty_unit(account1);
    xcons_transaction_ptr_t account1_tx1 = create_cons_transfer_tx(account1, "T000001111111111111111");
    xcons_transaction_ptr_t account1_tx2 = create_cons_transfer_tx(account1, "T000001111111111111111");
    xlightunit_block_para_t para1;
    para1.set_one_input_tx(account1_tx1);
    para1.set_one_input_tx(account1_tx2);
    base::xvblock_t* lightunit1 = test_blocktuil::create_next_lightunit(para1, account1_genesis_block);

    // create unit2
    std::string account2 = xblocktool_t::make_address_user_account("11111111111111111112");
    base::xvblock_t* account2_genesis_block = test_blocktuil::create_genesis_empty_unit(account2);
    xcons_transaction_ptr_t account2_tx1 = create_cons_transfer_tx(account2, "T000001111111111111111");
    xcons_transaction_ptr_t account2_tx2 = create_cons_transfer_tx(account2, "T000001111111111111111");
    xlightunit_block_para_t para2;
    para2.set_one_input_tx(account2_tx1);
    para2.set_one_input_tx(account2_tx2);
    base::xvblock_t* lightunit2 = test_blocktuil::create_next_lightunit(para2, account2_genesis_block);

    // create table-block
    xtable_block_para_t table_para;
    table_para.add_unit(lightunit1);
    table_para.add_unit(lightunit2);

    std::string taccount1 = data::xblocktool_t::make_address_table_account(base::enum_chain_zone_beacon_index, 0);
    base::xvblock_t* taccount1_genesis_block = test_blocktuil::create_genesis_empty_table(taccount1);
    auto taccount1_proposal_block = test_blocktuil::create_next_tableblock(table_para, taccount1_genesis_block);
    auto next_block = test_blocktuil::create_next_emptyblock(taccount1_proposal_block);
    taccount1_proposal_block->reset_next_block(next_block);
    auto next_next_block = test_blocktuil::create_next_emptyblock_with_justify(next_block, taccount1_proposal_block);
    next_block->reset_next_block(next_next_block);

    // // block send to backup
    // base::xautostream_t<4096> stream(base::xcontext_t::instance());
    // taccount1_proposal_block->serialize_to(stream);
    // stream << taccount1_proposal_block->get_input();
    // stream << taccount1_proposal_block->get_output();
    // std::string message = std::string((const char*)stream.data(), stream.size());

    // // backup verify table-block
    // base::xstream_t stream2(base::xcontext_t::instance(), (uint8_t*)message.data(), message.size());
    // xblock_t* backup_block = dynamic_cast<xblock_t*>(base::xdataobj_t::read_from(stream2));
    // ASSERT_NE(backup_block, nullptr);
    // std::string input_str;
    // stream2 >> input_str;
    // backup_block->set_input(input_str);
    // std::string output_str;
    // stream2 >> output_str;
    // backup_block->set_output(output_str);

    // should auto recreate genesis block when store a commited block by sync
    auto ret = blockstore->store_block(taccount1_proposal_block);
    xassert(ret == true);
    base::xauto_ptr<xblockchain2_t> blockchain2(store->clone_account(taccount1_proposal_block->get_account()));
    xassert(blockchain2->get_chain_height() == 1);

    ret = store->set_vblock(taccount1_proposal_block);
    xassert(ret == true);  // repeat should also return true
}

TEST_F(test_basic_case, blockstore_store_block_4) {
    xobject_ptr_t<xstore_face_t> store = xstore_factory::create_store_with_memdb();
    xobject_ptr_t<base::xvblockstore_t> blockstore;
    blockstore.attach(xblockstorehub_t::instance().create_block_store(*store, ""));
    xobject_ptr_t<base::xvblockstore_t> blockstore2;
    blockstore2.attach(xblockstorehub_t::instance().create_block_store(*store, ""));
    // create unit1
    std::string account1 = xblocktool_t::make_address_user_account("11111111111111111111");
    base::xvblock_t* account1_genesis_block = test_blocktuil::create_genesis_empty_unit(account1);
    xcons_transaction_ptr_t account1_tx1 = create_cons_transfer_tx(account1, "T000001111111111111111");
    xcons_transaction_ptr_t account1_tx2 = create_cons_transfer_tx(account1, "T000001111111111111111");
    xlightunit_block_para_t para1;
    para1.set_one_input_tx(account1_tx1);
    para1.set_one_input_tx(account1_tx2);
    base::xvblock_t* lightunit1 = test_blocktuil::create_next_lightunit(para1, account1_genesis_block);

    // create table-block
    xtable_block_para_t table_para;
    table_para.add_unit(lightunit1);

    std::string taccount1 = data::xblocktool_t::make_address_table_account(base::enum_chain_zone_beacon_index, 0);
    base::xvblock_t* taccount1_genesis_block = test_blocktuil::create_genesis_empty_table(taccount1);
    data::xtable_block_t* tableblock1 = (data::xtable_block_t*)test_blocktuil::create_next_tableblock(table_para, taccount1_genesis_block);
    {
        std::vector<xobject_ptr_t<base::xvblock_t>> units;
        tableblock1->extract_sub_blocks(units);
        xassert(units.size() != 0);
        for (auto & cache_unit : units) {
            ASSERT_FALSE(cache_unit->get_block_hash().empty());
        }
    }
}

TEST_F(test_basic_case, blockstore_store_block_5) {
    xobject_ptr_t<xstore_face_t> store = xstore_factory::create_store_with_memdb();
    xobject_ptr_t<base::xvblockstore_t> blockstore;
    blockstore.attach(xblockstorehub_t::instance().create_block_store(*store, ""));

    // create full unit
    std::string god_account = xblocktool_t::make_address_user_account("11111111111111111");
    create_god_account(store, god_account);
    base::xauto_ptr<base::xvblock_t> genesis_block(store->get_vblock(god_account, 0));
    base::xauto_ptr<xblockchain2_t> blockchain(store->clone_account(god_account));

    xfullunit_block_para_t fullblock_para;
    fullblock_para.m_account_state = blockchain->get_account_mstate();
    fullblock_para.m_account_propertys["111"] = "111111";
    fullblock_para.m_account_propertys["222"] = "222222";

    xblock_t* fullunit = (xblock_t*)xblocktool_t::create_next_fullunit(fullblock_para, genesis_block.get());
    auto propertys_ptr = fullunit->get_fullunit_propertys();
    xassert(propertys_ptr != nullptr);
    std::map<std::string, std::string> propertys_map = *propertys_ptr;
    xassert(propertys_map["111"] == "111111");
    xassert(propertys_map["222"] == "222222");

    // create tableblock
    xtable_block_para_t table_para;
    table_para.add_unit(fullunit);

    std::string taccount1 = data::xblocktool_t::make_address_table_account(base::enum_chain_zone_beacon_index, 0);
    base::xvblock_t* taccount1_genesis_block = test_blocktuil::create_genesis_empty_table(taccount1);
    auto taccount1_proposal_block = test_blocktuil::create_next_tableblock(table_para, taccount1_genesis_block);
    auto next_block = test_blocktuil::create_next_emptyblock(taccount1_proposal_block);
    taccount1_proposal_block->reset_next_block(next_block);
    auto next_next_block = test_blocktuil::create_next_emptyblock_with_justify(next_block, taccount1_proposal_block);
    next_block->reset_next_block(next_next_block);

    // should auto recreate genesis block when store a commited block by sync
    auto ret = blockstore->store_block(taccount1_proposal_block);
    xassert(ret == true);
}

