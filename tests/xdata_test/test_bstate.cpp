#include "gtest/gtest.h"
#include "xdata/xtransaction_v2.h"
#include "xdata/xtransaction_v1.h"
#include "xdata/xbstate_ctx.h"
#include "xconfig/xconfig_register.h"
#include "xbase/xmem.h"
#include "xbasic/xhex.h"

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
    xbstate_ctx_t bstatectx(bstate.get(), false);
    {
        ASSERT_EQ(0, bstatectx.string_create("@1"));
        ASSERT_EQ(0, bstatectx.string_set("@1", "v1"));
        ASSERT_EQ(0, bstatectx.string_create("@2"));
        ASSERT_EQ(0, bstatectx.string_set("@2", "v2"));
        ASSERT_EQ(4, bstatectx.get_canvas_records_size());
        ASSERT_EQ("v1", bstatectx.string_get("@1"));
        ASSERT_EQ("v2", bstatectx.string_get("@2"));

        ASSERT_EQ(true, bstatectx.do_rollback());
        ASSERT_EQ("", bstatectx.string_get("@1"));
        ASSERT_EQ(0, bstatectx.get_canvas_records_size());
    }

    {
        ASSERT_EQ(0, bstatectx.string_create("@1"));
        ASSERT_EQ(0, bstatectx.string_set("@1", "v1"));
        ASSERT_EQ(0, bstatectx.string_create("@2"));
        ASSERT_EQ(0, bstatectx.string_set("@2", "v2"));

        ASSERT_EQ(4, bstatectx.do_snapshot());
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
        ASSERT_EQ(4, bstatectx.get_canvas_records_size());
    }
    
}

TEST_F(test_bstate, token_id_1) {
    std::string addr = "T80000733b43e6a2542709dc918ef2209ae0fc6503c2f2";
    xobject_ptr_t<base::xvbstate_t> bstate = make_object_ptr<base::xvbstate_t>(addr, (uint64_t)1, (uint64_t)1, std::string(), std::string(), (uint64_t)0, (uint32_t)0, (uint16_t)0);
    xbstate_ctx_t bstatectx(bstate.get(), false);

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
