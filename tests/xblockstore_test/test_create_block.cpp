#include "gtest/gtest.h"

#include "xbase/xcontext.h"
#include "xbase/xdata.h"
#include "xbase/xobject.h"
#include "xbase/xmem.h"
#include "xbase/xcontext.h"

#include "xdata/xdatautil.h"
#include "xdata/xemptyblock.h"
#include "xdata/xblocktool.h"
#include "xdata/xlightunit.h"
#include "xdata/xtransaction_maker.hpp"

#include "xblockstore/xblockstore_face.h"
#include "tests/mock/xcertauth_util.hpp"
#include "tests/mock/xvchain_creator.hpp"
#include "tests/mock/xdatamock_table.hpp"

using namespace top;
using namespace top::base;
using namespace top::data;
using namespace top::mock;
using namespace top::store;

class test_create_block : public testing::Test {
protected:
    void SetUp() override {
    }

    void TearDown() override {
    }
    static void TearDownTestCase() {
    }
    static void SetUpTestCase() {
    }

};

TEST_F(test_create_block, table_unit_hash_calc) {
    mock::xvchain_creator creator(true);

    uint64_t max_block_height = 1;
    std::cout << "1111111111111111111111111111111" << std::endl;
    mock::xdatamock_table mocktable(1, 2);

    std::cout << "22222222222222222222222222222222" << std::endl;
    mocktable.genrate_table_chain(max_block_height, nullptr);
}


TEST_F(test_create_block, create_time_clock) {
    mock::xvchain_creator creator;
    base::xvblockstore_t* blockstore = creator.get_blockstore();

    uint64_t max_block_height = 3;
    mock::xdatamock_table mocktable(1, 2);
    mocktable.genrate_table_chain(max_block_height, blockstore);
    const std::vector<xblock_ptr_t> & tableblocks = mocktable.get_history_tables();
    xassert(tableblocks.size() == max_block_height + 1);
    const std::vector<xdatamock_unit> & mockunits = mocktable.get_mock_units();

    auto _block = mockunits[0].get_history_units()[1];
    auto unitstate = statestore::xstatestore_hub_t::instance()->get_unit_state_by_unit_block(_block.get());

    if (false == unitstate->get_bstate()->find_property(XPROPERTY_ACCOUNT_CREATE_TIME)) {
        uint64_t clock_height = 100;
        auto propobj = unitstate->get_bstate()->new_uint64_var(XPROPERTY_ACCOUNT_CREATE_TIME, nullptr);
        propobj->set(clock_height, nullptr);

        data::xunitstate_ptr_t account = std::make_shared<xunit_bstate_t>(unitstate->get_bstate().get());
        auto create_time = account->get_account_create_time();
        EXPECT_EQ(create_time, clock_height * 10 + TOP_BEGIN_GMTIME);
    }
}

TEST_F(test_create_block, create_time_gmt) {
    mock::xvchain_creator creator;
    base::xvblockstore_t* blockstore = creator.get_blockstore();

    uint64_t max_block_height = 3;
    mock::xdatamock_table mocktable(1, 2);
    mocktable.genrate_table_chain(max_block_height, blockstore);
    const std::vector<xblock_ptr_t> & tableblocks = mocktable.get_history_tables();
    xassert(tableblocks.size() == max_block_height + 1);

    const std::vector<xdatamock_unit> & mockunits = mocktable.get_mock_units();

    auto _block = mockunits[0].get_history_units()[1];
    auto unitstate = statestore::xstatestore_hub_t::instance()->get_unit_state_by_unit_block(_block.get());

    if (false == unitstate->get_bstate()->find_property(XPROPERTY_ACCOUNT_CREATE_TIME)) {
        uint64_t gmt = TOP_BEGIN_GMTIME + 1000;
        auto propobj = unitstate->get_bstate()->new_uint64_var(XPROPERTY_ACCOUNT_CREATE_TIME, nullptr);
        propobj->set(gmt, nullptr);

        data::xunitstate_ptr_t account = std::make_shared<xunit_bstate_t>(unitstate->get_bstate().get());
        auto create_time = account->get_account_create_time();
        EXPECT_EQ(create_time, gmt);
    }
}

#if 0
TEST_F(test_create_block, unit_random_single_sign) {
    mock::xvchain_creator creator;
    base::xvblockstore_t* blockstore = creator.get_blockstore();
    std::string address = xblocktool_t::make_address_user_account("11111111111111111112");
    base::xvaccount_t _vaddress(address);

    base::xauto_ptr<base::xvblock_t> genesis_block = blockstore->get_genesis_block(_vaddress);
    base::xauto_ptr<base::xvblock_t> block1 = xblocktool_t::create_next_emptyblock(genesis_block.get());

    if (block1->get_cert()->get_consensus_flags() & base::enum_xconsensus_flag_extend_cert) {
        std::cout << "unit block using extend cert" << std::endl;
    }
    xcertauth_util::instance().do_sign(block1.get());

    ASSERT_TRUE(blockstore->store_block(_vaddress, block1.get()));

    base::xauto_ptr<base::xvblock_t> block2 = xblocktool_t::create_next_emptyblock(block1.get());
    xcertauth_util::instance().do_sign(block2.get());
    ASSERT_TRUE(blockstore->store_block(_vaddress, block2.get()));
}

TEST_F(test_create_block, unit_multi_sign) {
    xobject_ptr_t<store::xstore_face_t> store_face = store::xstore_factory::create_store_with_memdb();
    auto store = store_face.get();

    std::string address = xblocktool_t::make_address_user_account("11111111111111111111");
    xvblockstore_t* blockstore = xblockstorehub_t::instance().create_block_store(*store, address);

    base::xauto_ptr<base::xvblock_t> genesis_block = blockstore->get_genesis_block(address);
    base::xauto_ptr<base::xvblock_t> block1 = xblocktool_t::create_next_emptyblock(genesis_block.get());

    xcertauth_util::instance().do_multi_sign(block1.get());

    ASSERT_TRUE(blockstore->store_block(block1.get()));

    base::xauto_ptr<base::xvblock_t> block2 = xblocktool_t::create_next_emptyblock(block1.get());
    xcertauth_util::instance().do_multi_sign(block2.get());
    ASSERT_TRUE(blockstore->store_block(block2.get()));
}

TEST_F(test_create_block, tableblock_with_unit_multi_sign) {
    xobject_ptr_t<store::xstore_face_t> store_face = store::xstore_factory::create_store_with_memdb();
    auto store = store_face.get();

    std::string taccount1 = xblocktool_t::make_address_shard_table_account(1);
    xvblockstore_t* blockstore = xblockstorehub_t::instance().create_block_store(*store, taccount1);

    // create unit1
    std::string addr1 = xblocktool_t::make_address_user_account("11111111111111111111");
    std::string to_addr = xblocktool_t::make_address_user_account("111111111111111111113");
    base::xauto_ptr<base::xvblock_t> account1_genesis_block = xblocktool_t::create_genesis_empty_unit(addr1);
    data::xunitstate_ptr_t account_1 = make_object_ptr<xblockchain2_t>(addr1);
    xtransaction_ptr_t tx1 = xtransaction_maker::make_transfer_tx(account_1, to_addr, 1, 10000, 100, 1000);

    // create unit2
    std::string addr2 = xblocktool_t::make_address_user_account("11111111111111111112");
    base::xauto_ptr<base::xvblock_t> account2_genesis_block = xblocktool_t::create_genesis_empty_unit(addr2);
    data::xunitstate_ptr_t account_2 = make_object_ptr<xblockchain2_t>(addr2);
    xtransaction_ptr_t tx2 = xtransaction_maker::make_transfer_tx(account_2, to_addr, 1, 10000, 100, 1000);

    {
        xlightunit_block_para_t para1;
        para1.set_one_input_tx(tx1);
        base::xauto_ptr<base::xvblock_t> lightunit1 = xblocktool_t::create_next_lightunit(para1, account1_genesis_block.get());
        xlightunit_block_para_t para2;
        para2.set_one_input_tx(tx2);
        base::xauto_ptr<base::xvblock_t> lightunit2 = xblocktool_t::create_next_lightunit(para2, account2_genesis_block.get());
        std::vector<base::xvblock_t*> units;
        units.push_back(lightunit1.get());
        units.push_back(lightunit2.get());

        {
            base::xauto_ptr<base::xvblock_t> taccount1_genesis_block = xblocktool_t::create_genesis_empty_table(taccount1);

            test_blockmock_t mock(store);
            base::xauto_ptr<base::xvblock_t> taccount1_proposal_block = xtableblock_util::create_tableblock(units, taccount1_genesis_block.get());
            ASSERT_EQ(base::enum_vcert_auth_result::enum_successful, xcertauth_util::instance().get_certauth().verify_muti_sign(taccount1_proposal_block.get()));

            ASSERT_TRUE(blockstore->store_block(taccount1_proposal_block.get()));
            ASSERT_NE(taccount1_proposal_block, nullptr);
            ASSERT_EQ(dynamic_cast<xblock_t*>(taccount1_proposal_block.get())->get_txs_count(), 2);
            ASSERT_FALSE(taccount1_proposal_block->get_block_hash().empty());

            xtable_block_t* tableblock = dynamic_cast<xtable_block_t*>(taccount1_proposal_block.get());
            auto & cache_units = tableblock->get_tableblock_units(false);
            for (auto & cache_unit : cache_units) {
                ASSERT_EQ(cache_unit->get_refcount(), 1);  // no unit cache
            }
        }
        ASSERT_EQ(lightunit1->get_refcount(), 1);
        ASSERT_EQ(lightunit2->get_refcount(), 1);
    }
}

TEST_F(test_create_block, single_unit_in_table_store) {
    xobject_ptr_t<xstore_face_t> store_face = xstore_factory::create_store_with_memdb();

    uint64_t count = 100;
    auto store = store_face.get();

    std::string address = xblocktool_t::make_address_user_account("11111111111111111112");
    std::string db_path = "block_connect_in_table";
    xvblockstore_t* blockstore = xblockstorehub_t::instance().create_block_store(*store, db_path);

    test_blockmock_t blockmock(store_face.get());

    std::string property("election_list");
    base::xvblock_t *prev_block = blockmock.create_property_block(nullptr, address, property);;
    base::xvblock_t *curr_block = nullptr;

    std::string table_account = xblocktool_t::make_address_shard_table_account(9);
    base::xvblock_t* prev_tableblock = xblocktool_t::create_genesis_empty_table(table_account);

    // set property for the account in the height divided by interval
    size_t interval = 5;
    for (uint64_t i = 1; i <= count; i++) {
        if (i % interval == 0) {
            std::string value(std::to_string(i));
            curr_block = blockmock.create_property_block(prev_block, address, property, value);
        } else {
            curr_block = blockmock.create_property_block(prev_block, address, property);
        }
        std::vector<base::xvblock_t*> units;
        units.push_back(curr_block);

        base::xvblock_t* proposal_tableblock = xtableblock_util::create_tableblock(units, prev_tableblock);
        ASSERT_EQ(base::enum_vcert_auth_result::enum_successful, xcertauth_util::instance().get_certauth().verify_muti_sign(proposal_tableblock));

        ASSERT_TRUE(blockstore->store_block(proposal_tableblock));
        for (auto& unit : units) {
            unit->release_ref();
        }

        base::xauto_ptr<base::xvblock_t> lock_tableblock = blockmock.create_next_empty_tableblock(proposal_tableblock);
        ASSERT_EQ(base::enum_vcert_auth_result::enum_successful, xcertauth_util::instance().get_certauth().verify_muti_sign(lock_tableblock.get()));
        ASSERT_TRUE(blockstore->store_block(lock_tableblock.get()));

        base::xauto_ptr<base::xvblock_t> cert_tableblock = blockmock.create_next_empty_tableblock(lock_tableblock.get());
        ASSERT_EQ(base::enum_vcert_auth_result::enum_successful, xcertauth_util::instance().get_certauth().verify_muti_sign(cert_tableblock.get()));
        ASSERT_TRUE(blockstore->store_block(cert_tableblock.get()));

        base::xauto_ptr<base::xvblock_t> commit_unitblock = blockstore->get_latest_committed_block(address);
        prev_block->release_ref();
        prev_block = commit_unitblock.get();
        prev_block->add_ref();

        prev_tableblock->release_ref();
        prev_tableblock = cert_tableblock.get();
        prev_tableblock->add_ref();
    }
    prev_block->release_ref();
    prev_tableblock->release_ref();

    base::xauto_ptr<xvblock_t> connected_block = blockstore->get_latest_connected_block(address);
    ASSERT_TRUE(connected_block != nullptr);
    ASSERT_EQ(connected_block->get_height(), count);

    base::xauto_ptr<xvblock_t> executed_block = blockstore->get_latest_executed_block(address);
    ASSERT_TRUE(executed_block != nullptr);
    ASSERT_EQ(executed_block->get_height(), count);

    std::vector<std::string> values;

    // get property value at random height
    uint64_t heights[] = {8, 30, 80, 67};
    std::map<uint64_t, std::vector<std::string> > results;

    for (auto height : heights) {
        std::vector<std::string> expected_values;
        for (size_t i = interval; i <= height; i += interval) {
            expected_values.push_back(std::to_string(i));
        }
        results.insert(std::make_pair(height, expected_values));
    }

    for (uint64_t height : heights) {
        int32_t ret = store->get_list_property(address, height, property, values);
        ASSERT_EQ(ret, xsuccess);

        ASSERT_EQ(results[height], values);
    }
}

using table_pending_set_t = std::map<std::string, std::set<std::string> >;


bool unit_table_matched(const std::string& unit_addr, const std::string& table_addr) {
    uint16_t ledger_subaddr = base::xvaccount_t::get_ledgersubaddr_from_account(table_addr);
    uint16_t subaddr = get_vledger_subaddr(base::xvaccount_t::get_xid_from_account(unit_addr));
    if (subaddr == ledger_subaddr) {
        return true;
    }
    return false;
}

bool unit_in_pending_table(const std::string& unit_addr, const std::string& table_addr, const table_pending_set_t& pending_table_units) {
    auto it = pending_table_units.find(table_addr);
    // check the units in the pending set
    if (it != pending_table_units.end()) {
        const auto& units = it->second;
        if (units.find(unit_addr) != units.end()) {
            return true;
        } else {
            return false;
        }
    }
    return false;
}

TEST_F(test_create_block, multi_address_in_table_store_BENCH) {
    xobject_ptr_t<xstore_face_t> store_face = xstore_factory::create_store_with_memdb();

    auto store = store_face.get();
    std::string db_path = "multi_address_in_table_store";
    xvblockstore_t* blockstore = xblockstorehub_t::instance().create_block_store(*store, db_path);

    test_blockmock_t blockmock(store_face.get());

    size_t units_per_table = 100;
    size_t unit_address_count = 10000;
    size_t table_count = unit_address_count / units_per_table;

    std::vector<std::string> unit_addresses;
    std::vector<std::string> table_addresses;

    uint8_t addr_type = base::enum_vaccount_addr_type_secp256k1_user_account;
    uint16_t ledger_id = base::xvaccount_t::make_ledger_id(base::enum_main_chain_id, base::enum_chain_zone_consensus_index);

    for (size_t i = 0; i < unit_address_count; ++i) {
        unit_addresses.push_back(utl::xcrypto_util::make_address_by_random_key(addr_type, ledger_id));
    }

    for (size_t i = 0; i < table_count; ++i) {
        table_addresses.push_back(xblocktool_t::make_address_shard_table_account(i));
    }

    // record unit's previous block for building the next block
    std::map<std::string, xvblock_t*> prev_blocks;
    // unit property modify interval
    std::map<std::string, size_t> property_intervals;

    // record table's previous block for building the next block
    std::map<std::string, xvblock_t*> prev_tableblocks;

    // record table packed units in previous two height, since these unit addresses can't be packed in this height
    std::map<std::string, std::set<std::string> > pending_table_units;

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<size_t> interval_dist(4, 10);
    std::uniform_int_distribution<size_t> unit_dist(0, unit_address_count - 1);
    std::uniform_int_distribution<size_t> table_dist(0, table_count - 1);
    std::uniform_int_distribution<size_t> packed_tx_dist(0, 64);

    std::string property("election_list");

    for (size_t i = 0; i < unit_address_count; ++i) {
        auto prev_block = blockmock.create_property_block(nullptr, unit_addresses[i], property);
        assert(prev_block != nullptr);

        prev_blocks.insert(std::make_pair(unit_addresses[i], prev_block));
        property_intervals.insert(std::make_pair(unit_addresses[i], interval_dist(gen)));
    }

    for (size_t i = 0; i < table_count; ++i) {
        auto prev_block = xblocktool_t::create_genesis_empty_table(table_addresses[i]);
        assert(prev_block != nullptr);

        prev_tableblocks.insert(std::make_pair(table_addresses[i], prev_block));
    }

    base::xvblock_t *curr_block = nullptr;

    size_t iterations = 30;
    for (size_t index = 0; index < iterations; ++index) {
        for (size_t i = 0; i < table_count; ++i) {
            std::string table_addr = table_addresses[i];
            std::set<std::string> packed_unit_addresses;

            // distribute unit into table randomly
            size_t packed_tx_count = packed_tx_dist(gen);
            size_t matched = 0;
            for (size_t j = 0; j < unit_address_count; ++j) {
                std::string unit_address = unit_addresses[unit_dist(gen)];
                if (unit_table_matched(unit_address, table_addr)) {
                    // check the units in the pending set
                    if (unit_in_pending_table(unit_address, table_addr, pending_table_units)) {
                        continue;
                    } else {
                        packed_unit_addresses.insert(unit_address);
                        if (++matched >= packed_tx_count) {
                            break;
                        }
                    }
                }
            }

            // build the necessary units for the table address
            std::vector<base::xvblock_t*> packed_units;
            for (const auto& unit_account : packed_unit_addresses) {
                base::xvblock_t* prev_block = prev_blocks[unit_account];
                // set property for the account in the height divided by interval
                if (prev_block->get_height() > 0 && prev_block->get_height() % property_intervals[unit_account] == 0) {
                    std::string value(std::to_string(prev_block->get_height()));
                    curr_block = blockmock.create_property_block(prev_block, unit_account, property, value);
                } else {
                    curr_block = blockmock.create_property_block(prev_block, unit_account, property);
                }
                packed_units.push_back(curr_block);
            }
#if 0
            std::cout << "table block: " << table_addr << " begins height: " << prev_tableblocks[table_addr]->get_height() + 1 << std::endl;
            for (const auto& unit : packed_unit_addresses) {
                std::cout << "unit: " << unit << std::endl;
            }
            std::cout << "table block: " << table_addr << " ends height: " << prev_tableblocks[table_addr]->get_height() + 1 << std::endl;
#endif
            // create commit/lock/cert table to commit the unit
            base::xvblock_t* proposal_tableblock = xtableblock_util::create_tableblock(packed_units, prev_tableblocks[table_addr]);
            ASSERT_EQ(base::enum_vcert_auth_result::enum_successful, xcertauth_util::instance().get_certauth().verify_muti_sign(proposal_tableblock));

            ASSERT_TRUE(blockstore->store_block(proposal_tableblock));
            for (auto& unit : packed_units) {
                unit->release_ref();
            }

            // a better way to decide if unit can be packeed
            // add these unit to the pending list and remove committed unit in (height - 2) tableblock
            if (proposal_tableblock->get_height() > 2) {
                // remove the accounts in (height - 2) tableblock
                base::xauto_ptr<base::xvblock_t> commit_block = blockstore->get_latest_committed_block(table_addr);
                assert(commit_block != nullptr);
                assert(commit_block->get_height() == (proposal_tableblock->get_height() - 2));
                if (commit_block->get_block_class() == base::enum_xvblock_class_light) {
                    data::xtable_block_t* commit_tableblock = dynamic_cast<data::xtable_block_t*>(commit_block.get());
                    assert(commit_tableblock != nullptr);
                    // get unit addresses from commit tableblock
                    auto & cache_units = commit_tableblock->get_tableblock_units(true);
                    auto it = pending_table_units.find(table_addr);
                    if (it != pending_table_units.end()) {
                        for (auto & cache_unit : cache_units) {
                            // check the units in the pending set
                            const std::string& addr = cache_unit->get_account();
                            it->second.erase(addr);

                            // update the previous block (commited unit)
                            prev_blocks[addr]->release_ref();
                            prev_blocks[addr] = cache_unit.get();
                            prev_blocks[addr]->add_ref();
                        }
                    }
                }
            }

            // add unit addresses to the pending table's unit list
            auto it = pending_table_units.find(table_addr);
            if (it != pending_table_units.end()) {
                it->second.insert(packed_unit_addresses.begin(), packed_unit_addresses.end());
            } else {
                pending_table_units.insert(std::make_pair(table_addr, packed_unit_addresses));
            }

            prev_tableblocks[table_addr]->release_ref();
            prev_tableblocks[table_addr] = proposal_tableblock;
        }
    }

    for (size_t i = 0; i < unit_address_count; ++i) {
        base::xauto_ptr<xvblock_t> connected_block = blockstore->get_latest_connected_block(unit_addresses[i]);
        ASSERT_TRUE(connected_block != nullptr);
        std::cout << unit_addresses[i] << " connected height: " << connected_block->get_height() << std::endl;
    }

    for (size_t i = 0; i < table_count; ++i) {
        base::xauto_ptr<xvblock_t> executed_block = blockstore->get_latest_executed_block(table_addresses[i]);
        ASSERT_TRUE(executed_block != nullptr);
        std::cout << table_addresses[i] << " execute height : " << executed_block->get_height() << std::endl;
    }

    for (size_t i = 0; i < unit_address_count; ++i) {
        prev_blocks[unit_addresses[i]]->release_ref();
    }

    for (size_t i = 0; i < table_count; ++i) {
        prev_tableblocks[table_addresses[i]]->release_ref();
    }
}
#endif
