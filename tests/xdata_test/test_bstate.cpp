#include "gtest/gtest.h"
#define protected public
#define private public
#include "xdata/xtransaction_v2.h"
#include "xdata/xtransaction_v1.h"
#include "xdata/xbstate_ctx.h"
#include "xdata/xblocktool.h"
#include "xdata/xunit_bstate.h"
#include "xconfig/xconfig_register.h"
#include "xbase/xmem.h"
#include "xbasic/xhex.h"
#include "xcrypto/xckey.h"

using namespace top;
using namespace top::base;
using namespace top::data;

class test_bstate : public testing::Test {
protected:
    void SetUp() override {
    }

    void TearDown() override {
    }
};

TEST_F(test_bstate, snapshot_rollback_1) {
    xobject_ptr_t<base::xvbstate_t> bstate = make_object_ptr<base::xvbstate_t>("T80000733b43e6a2542709dc918ef2209ae0fc6503c2f2", (uint64_t)1, (uint64_t)1, std::string(), std::string(), (uint64_t)0, (uint32_t)0, (uint16_t)0);
    
    std::string last_hash = "111";
    xobject_ptr_t<base::xvbstate_t> clone_bstate = make_object_ptr<base::xvbstate_t>(last_hash, *bstate.get());
    xbstate_ctx_t bstatectx(clone_bstate.get(), bstate.get());
    {
        ASSERT_EQ(0, bstatectx.string_create("@1"));
        ASSERT_EQ(0, bstatectx.string_set("@1", "v1"));
        ASSERT_EQ(0, bstatectx.string_create("@2"));
        ASSERT_EQ(0, bstatectx.string_set("@2", "v2"));
        ASSERT_EQ(4, bstatectx.get_canvas_records_size());
        ASSERT_EQ("v1", bstatectx.string_get("@1"));
        ASSERT_EQ("v2", bstatectx.string_get("@2"));
        ASSERT_EQ(false, bstate->find_property("@1"));
        ASSERT_EQ(false, bstate->find_property("@2"));

        ASSERT_EQ(true, bstatectx.do_rollback());
        ASSERT_EQ("", bstatectx.string_get("@1"));
        ASSERT_EQ(true, clone_bstate->find_property("@1"));  // TODO(jimmy) already created
        ASSERT_EQ(true, clone_bstate->find_property("@2"));
        ASSERT_EQ(false, bstate->find_property("@1"));
        ASSERT_EQ(false, bstate->find_property("@2")); 
        ASSERT_EQ(0, bstatectx.get_canvas_records_size());
    }

    {
        ASSERT_EQ(0, bstatectx.string_create("@1"));
        ASSERT_EQ(0, bstatectx.string_set("@1", "v1"));
        ASSERT_EQ(0, bstatectx.string_create("@2"));
        ASSERT_EQ(0, bstatectx.string_set("@2", "v2"));

        ASSERT_EQ(4, bstatectx.do_snapshot());
        ASSERT_EQ(false, bstate->find_property("@1"));
        ASSERT_EQ(false, bstate->find_property("@2")); 
        ASSERT_EQ("v1", bstatectx.string_get("@1"));
        ASSERT_EQ(true, bstatectx.do_rollback());
        ASSERT_EQ("v1", bstatectx.string_get("@1"));
        ASSERT_EQ(4, bstatectx.get_canvas_records_size());

        ASSERT_EQ(0, bstatectx.string_set("@1", "v1111"));
        ASSERT_EQ(0, bstatectx.string_set("@2", "v2222"));
        ASSERT_EQ(6, bstatectx.get_canvas_records_size());
        ASSERT_EQ("v1111", bstatectx.string_get("@1"));
        ASSERT_EQ("v2222", bstatectx.string_get("@2"));

        ASSERT_EQ(true, bstatectx.do_rollback());
        ASSERT_EQ("v1", bstatectx.string_get("@1"));
        auto string_var = clone_bstate->load_string_var("@1");
        ASSERT_EQ("v1", string_var->query());
        ASSERT_EQ(false, bstate->find_property("@1"));
        ASSERT_EQ(false, bstate->find_property("@2"));           
        ASSERT_EQ(4, bstatectx.get_canvas_records_size());
    }
    
}

TEST_F(test_bstate, token_id_1) {
    std::string addr = "T80000733b43e6a2542709dc918ef2209ae0fc6503c2f2";
    xobject_ptr_t<base::xvbstate_t> bstate = make_object_ptr<base::xvbstate_t>(addr, (uint64_t)1, (uint64_t)1, std::string(), std::string(), (uint64_t)0, (uint32_t)0, (uint16_t)0);
    xbstate_ctx_t bstatectx(bstate.get(), bstate.get());

    evm_common::u256 add_token_256 = 10000000000000000000ULL;
    auto old_token_256 = bstatectx.tep_token_balance(common::xtoken_id_t::eth);
    ASSERT_EQ(old_token_256, 0);

    bstatectx.tep_token_deposit(common::xtoken_id_t::eth, add_token_256);
    auto new_token_256 = bstatectx.tep_token_balance(common::xtoken_id_t::eth);
    ASSERT_EQ(new_token_256, add_token_256);

    bstatectx.tep_token_deposit(common::xtoken_id_t::eth, add_token_256);
    new_token_256 = bstatectx.tep_token_balance(common::xtoken_id_t::eth);
    ASSERT_EQ(new_token_256, add_token_256*2);

    bstatectx.tep_token_withdraw(common::xtoken_id_t::eth, add_token_256);
    new_token_256 = bstatectx.tep_token_balance(common::xtoken_id_t::eth);
    ASSERT_EQ(new_token_256, add_token_256);

    bstatectx.tep_token_withdraw(common::xtoken_id_t::eth, add_token_256);
    new_token_256 = bstatectx.tep_token_balance(common::xtoken_id_t::eth);
    ASSERT_EQ(new_token_256, 0);    


    auto token_name = top::to_string(common::xtoken_id_t::eth);
    ASSERT_EQ(top::to_hex(token_name), "02");
}

TEST_F(test_bstate, empty_state) {
{
    std::string account = "T000000000000000000000000000000000000000";
    base::xauto_ptr<base::xvblock_t> unit = data::xblocktool_t::create_genesis_empty_unit(account);
    xobject_ptr_t<base::xvbstate_t> current_state = make_object_ptr<base::xvbstate_t>(*unit);
    data::xunitstate_ptr_t unitstate = std::make_shared<data::xunit_bstate_t>(current_state.get());
    ASSERT_TRUE(unitstate->is_empty_state());
}
{
    std::string account = "T000000000000000000000000000000000000000";
    base::xauto_ptr<base::xvblock_t> unit = data::xblocktool_t::create_genesis_lightunit(account, 10000);
    xobject_ptr_t<base::xvbstate_t> current_state = make_object_ptr<base::xvbstate_t>(*unit);
    std::string binlog = unit->get_binlog();
    ASSERT_TRUE(current_state->apply_changes_of_binlog(binlog));

    data::xunitstate_ptr_t unitstate = std::make_shared<data::xunit_bstate_t>(current_state.get());
    ASSERT_FALSE(unitstate->is_empty_state());
}
}

TEST_F(test_bstate, state_reset_1) {
    std::string account = "T8000037d4fbc08bf4513a68a287ed218b0adbd497ef30";    
    base::xauto_ptr<base::xvblock_t> unit1 = data::xblocktool_t::create_genesis_lightunit(account, 10000);

    xobject_ptr_t<base::xvbstate_t> bstate1 = make_object_ptr<base::xvbstate_t>(*unit1);
    data::xunitstate_ptr_t unitstate1 = std::make_shared<data::xunit_bstate_t>(bstate1.get(), bstate1.get());
    unitstate1->get_bstate()->apply_changes_of_binlog(unit1->get_binlog());
    ASSERT_EQ(unitstate1->balance(), 10000);
    size_t propertys_size = unitstate1->get_bstate()->get_all_property_names().size();
    std::string state_bin = unitstate1->get_bstate()->export_state();

    xobject_ptr_t<base::xvbstate_t> bstate1_clone = make_object_ptr<base::xvbstate_t>(*unit1, *bstate1.get());
    data::xunitstate_ptr_t unitstate1_clone = std::make_shared<data::xunit_bstate_t>(bstate1_clone.get(), bstate1_clone.get());
    ASSERT_EQ(unitstate1_clone->balance(), 10000);

    // make new version state
    ASSERT_EQ(0, unitstate1->token_deposit(XPROPERTY_BALANCE_AVAILABLE, base::vtoken_t(20000)));
    ASSERT_EQ(unitstate1->balance(), 10000 + 20000);
    ASSERT_EQ(0, unitstate1->string_create("@1"));
    ASSERT_EQ(0, unitstate1->string_set("@1", "aaa"));
    ASSERT_EQ(unitstate1->get_bstate()->get_all_property_names().size(), propertys_size+1);
    // new version state reset to old snapshot
    // ASSERT_EQ(true, unitstate1->reset_state(state_bin));
    std::string binlog = unitstate1->take_binlog();
    ASSERT_TRUE(binlog.size() > 0);

    unitstate1_clone->get_bstate()->apply_changes_of_binlog(binlog);
    xobject_ptr_t<base::xvbstate_t> bstate1_clone2 = make_object_ptr<base::xvbstate_t>(*unit1, *unitstate1_clone->get_bstate());
    data::xunitstate_ptr_t unitstate1_clone2 = std::make_shared<data::xunit_bstate_t>(bstate1_clone2.get(), bstate1_clone2.get());
    ASSERT_EQ(unitstate1_clone2->balance(), 10000 + 20000);

    ASSERT_TRUE(unitstate1_clone->reset_state(state_bin));
    ASSERT_EQ(unitstate1_clone->get_bstate()->get_all_property_names().size(), propertys_size);
    ASSERT_EQ(unitstate1_clone->balance(), 10000);
    ASSERT_EQ("", unitstate1_clone->string_get("@1"));

    std::string binlog2 = unitstate1_clone->take_binlog();
    ASSERT_TRUE(unitstate1_clone2->get_bstate()->apply_changes_of_binlog(binlog2));
    ASSERT_EQ(unitstate1_clone2->get_bstate()->get_all_property_names().size(), propertys_size);
    ASSERT_EQ(unitstate1_clone2->balance(), 10000);
    ASSERT_EQ("", unitstate1_clone2->string_get("@1"));    
}

TEST_F(test_bstate, state_reset_2) {
    std::string account = "T8000037d4fbc08bf4513a68a287ed218b0adbd497ef30";    
    base::xauto_ptr<base::xvblock_t> unit1 = data::xblocktool_t::create_genesis_lightunit(account, 10000);

    xobject_ptr_t<base::xvbstate_t> bstate1 = make_object_ptr<base::xvbstate_t>(*unit1);
    data::xunitstate_ptr_t unitstate1 = std::make_shared<data::xunit_bstate_t>(bstate1.get(), bstate1.get());
    unitstate1->get_bstate()->apply_changes_of_binlog(unit1->get_binlog());
    ASSERT_EQ(unitstate1->balance(), 10000);
    size_t propertys_size = unitstate1->get_bstate()->get_all_property_names().size();
    // make new version state
    ASSERT_EQ(0, unitstate1->token_deposit(XPROPERTY_BALANCE_AVAILABLE, base::vtoken_t(20000)));
    ASSERT_EQ(0, unitstate1->string_create("@1"));
    ASSERT_EQ(0, unitstate1->string_set("@1", "aaa"));
    ASSERT_EQ(unitstate1->get_bstate()->get_all_property_names().size(), propertys_size+1);

    std::string binlog = unitstate1->take_binlog();
    ASSERT_TRUE(binlog.size() > 0);
    std::string state_bin = unitstate1->get_bstate()->export_state();
    std::string snapshot = unitstate1->take_snapshot();
    std::cout << "propertys size " << unitstate1->get_bstate()->get_all_property_names().size() << std::endl;
    std::cout << "state_bin size " << state_bin.size() << std::endl;
    std::cout << "snapshot size " << snapshot.size() << std::endl;
}

TEST_F(test_bstate, state_reset_3) {
    std::string account = "T8000037d4fbc08bf4513a68a287ed218b0adbd497ef30";    
    base::xauto_ptr<base::xvblock_t> unit1 = data::xblocktool_t::create_genesis_lightunit(account, 10000);

    xobject_ptr_t<base::xvbstate_t> bstate1 = make_object_ptr<base::xvbstate_t>(*unit1);
    data::xunitstate_ptr_t unitstate1 = std::make_shared<data::xunit_bstate_t>(bstate1.get(), bstate1.get());
    unitstate1->get_bstate()->apply_changes_of_binlog(unit1->get_binlog());
    ASSERT_EQ(unitstate1->balance(), 10000);
    std::string state_bin = unitstate1->get_bstate()->export_state();

    base::xvheader_t _header;
    _header.set_height(1);
    _header.set_account(account);
    xobject_ptr_t<base::xvbstate_t> bstate1_clone = make_object_ptr<base::xvbstate_t>(_header, *bstate1.get());
    data::xunitstate_ptr_t unitstate1_clone = std::make_shared<data::xunit_bstate_t>(bstate1_clone.get(), bstate1_clone.get());
    ASSERT_EQ(unitstate1_clone->balance(), 10000);
    ASSERT_EQ(unitstate1_clone->height(), 1);
    ASSERT_EQ(true, unitstate1_clone->reset_state(state_bin));
    ASSERT_EQ(unitstate1_clone->height(), 1);
}


TEST_F(test_bstate, state_reset_4) {
    std::string account = "T8000037d4fbc08bf4513a68a287ed218b0adbd497ef30";    
    base::xauto_ptr<base::xvblock_t> unit1 = data::xblocktool_t::create_genesis_lightunit(account, 10000);

    xobject_ptr_t<base::xvbstate_t> bstate1 = make_object_ptr<base::xvbstate_t>(*unit1);
    data::xunitstate_ptr_t unitstate1 = std::make_shared<data::xunit_bstate_t>(bstate1.get(), bstate1.get());
    unitstate1->get_bstate()->apply_changes_of_binlog(unit1->get_binlog());
    ASSERT_EQ(unitstate1->balance(), 10000);
    size_t propertys_size = unitstate1->get_bstate()->get_all_property_names().size();
    std::string state_bin = unitstate1->get_bstate()->export_state();

    xobject_ptr_t<base::xvbstate_t> bstate1_clone = make_object_ptr<base::xvbstate_t>(*unit1, *bstate1.get());
    data::xunitstate_ptr_t unitstate1_clone = std::make_shared<data::xunit_bstate_t>(bstate1_clone.get(), bstate1_clone.get());
    ASSERT_EQ(unitstate1_clone->balance(), 10000);

    // make new version state
    ASSERT_EQ(0, unitstate1->token_deposit(XPROPERTY_BALANCE_AVAILABLE, base::vtoken_t(20000)));
    ASSERT_EQ(unitstate1->balance(), 10000 + 20000);
    ASSERT_EQ(0, unitstate1->string_create("@1"));
    ASSERT_EQ(0, unitstate1->string_set("@1", "aaa"));
    ASSERT_EQ(unitstate1->get_bstate()->get_all_property_names().size(), propertys_size+1);
    // new version state reset to old snapshot
    // ASSERT_EQ(true, unitstate1->reset_state(state_bin));
    std::string binlog = unitstate1->take_binlog();
    ASSERT_TRUE(binlog.size() > 0);

    unitstate1_clone->get_bstate()->apply_changes_of_binlog(binlog);
    xobject_ptr_t<base::xvbstate_t> bstate1_clone2 = make_object_ptr<base::xvbstate_t>(*unit1, *unitstate1_clone->get_bstate());
    data::xunitstate_ptr_t unitstate1_clone2 = std::make_shared<data::xunit_bstate_t>(bstate1_clone2.get(), bstate1_clone2.get());
    ASSERT_EQ(unitstate1_clone2->balance(), 10000 + 20000);

    ASSERT_TRUE(unitstate1_clone2->reset_state(state_bin));
    ASSERT_EQ(unitstate1_clone2->get_bstate()->get_all_property_names().size(), propertys_size);
    ASSERT_EQ(unitstate1_clone2->balance(), 10000);
    ASSERT_EQ("", unitstate1_clone2->string_get("@1"));
    ASSERT_EQ("aaa", unitstate1_clone->string_get("@1"));
}

TEST_F(test_bstate, bstate_size_BENCH) {
#if 0// TODO(jimmy) too large bstate create will crash
    std::string account = "T8000037d4fbc08bf4513a68a287ed218b0adbd497ef30";    
    base::xauto_ptr<base::xvblock_t> unit1 = data::xblocktool_t::create_genesis_lightunit(account, 10000);

    xobject_ptr_t<base::xvbstate_t> bstate1 = make_object_ptr<base::xvbstate_t>(*unit1);
    data::xunitstate_ptr_t unitstate1 = std::make_shared<data::xunit_bstate_t>(bstate1.get(), bstate1.get());

    std::string prop_name = "@11";
    unitstate1->map_create(prop_name);
    uint32_t count = 10000000; // TODO(jimmy) 10000000
    for (uint32_t i = 0; i < count; i++) {
        utl::xecprikey_t prikey;
        utl::xecpubkey_t pubkey = prikey.get_public_key();
        std::string addr = base::xstring_utl::to_hex(pubkey.to_raw_eth_address());
        std::string value = addr;
        unitstate1->map_set(prop_name, addr, addr);
        // std::cout << "map_set:" << addr << std::endl;

        if (i % 100000 == 0) {
            std::string state_bin;
            unitstate1->get_bstate()->serialize_to_string(state_bin);
            base::xauto_ptr<base::xvbstate_t> state_ptr = base::xvblock_t::create_state_object(state_bin);
            if (nullptr == state_ptr) {
                std::cout << "fail test count:" << count << ",state_bin:" << state_bin.size() << std::endl;
                xassert(false);
                break;
            }
            
            std::string state_bin2;
            state_ptr->serialize_to_string(state_bin2);
            if (state_bin != state_bin2) {
                std::cout << "fail test count:" << count << ",state_bin:" << state_bin.size() << ",state_bin2:" << state_bin2.size() << std::endl;
                xassert(false);
                break;
            }
            
            std::cout << "succ test count:" << count << ",state_bin:" << state_bin.size() << std::endl;
        }
    }
#endif
}

TEST_F(test_bstate, bstate_create_BENCH) {
#if 0// TODO(jimmy) read 16M binary data from file, it will crash
    std::string file_content;
    base::xfile_utl::read_file("/home/xu/test/dump", file_content);

    std::cout << "file_content " << file_content.size() << std::endl;
    base::xauto_ptr<base::xvbstate_t> state_ptr = base::xvblock_t::create_state_object(file_content);
    if (nullptr == state_ptr) {
        std::cout << "fail" << std::endl;
        xassert(false);
    } else {
        std::cout << "succ" << std::endl;
    }
#endif
}