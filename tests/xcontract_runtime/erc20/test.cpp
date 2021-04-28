#define protected public
#include "xbasic/xbyte_buffer.h"
#include "xbasic/xmemory.hpp"
#include "xcontract_common/xcontract_api_params.h"
#include "xcontract_runtime/xuser/xwasm/xwasm_engine.h"
#include "xdata/xblocktool.h"

#include <assert.h>

#include <gtest/gtest.h>

#include <cinttypes>
#include <fstream>
#include <string>
#include <vector>
using namespace top::base;
using namespace top::data;
using namespace top::contract_common::properties;
class test_erc20 : public testing::Test {
public:
    top::contract_common::xcontract_execution_context_t * exe_ctx;

protected:
    void SetUp() override {
        auto tx = top::make_object_ptr<top::data::xtransaction_t>();

        std::string address = xblocktool_t::make_address_user_account("T00000LPggAizKRsMxzS3xwKBRk3Q8qu5xGbz2Q3");
        // xvbstate_t(const std::string & account,const uint64_t block_height,const uint64_t block_viewid,const std::string & last_block_hash,const std::string
        // &last_full_block_hash,const uint64_t last_full_block_height, const uint32_t raw_block_versions,const uint16_t raw_block_types, xvexeunit_t * parent_unit = NULL);
        xvbstate_t vbstate{address, 1, 1, "last_block_hash", "last_full_block_hash", 0, 0, 1}; // ? how to construct a vbstate
        xproperty_access_control_data_t ac_data;
        std::shared_ptr<xproperty_access_control_t> api_ = std::make_shared<xproperty_access_control_t>(top::make_observer(&vbstate), ac_data);

        // top::contract_common::xcontract_metadata_t meta;
        top::contract_common::xcontract_state_t * context_ = new top::contract_common::xcontract_state_t{top::common::xaccount_address_t{address}, top::make_observer(api_.get())};
        exe_ctx = new top::contract_common::xcontract_execution_context_t{tx, top::make_observer(context_)};
    }
};

TEST_F(test_erc20, _2) {
    std::vector<top::xbyte_buffer_t> input;

    top::contract_runtime::user::xwasm_engine_t wasm_engine;
    wasm_engine.deploy_contract_erc20(input, top::make_observer(exe_ctx));
}