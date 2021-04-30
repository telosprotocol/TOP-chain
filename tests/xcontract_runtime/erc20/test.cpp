#define protected public
#include "xbasic/xbyte_buffer.h"
#include "xbasic/xmemory.hpp"
#include "xcontract_common/xcontract_api_params.h"
#include "xcontract_common/xcontract_execution_context.h"
#include "xcontract_runtime/xuser/xwasm/xwasm_engine.h"
#include "xcontract_runtime/xuser/xwasm/xwasm_rustvm_extern_api.h"
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
    void SetUp() override {}
};

TEST_F(test_erc20, erc20) {
    // wasm bytes
    // uint8_t * bytes;
    // uint32_t bytes_size;

    // char const * file_path = "/home/wens/top_projects/github_xchain/src/xtopcom/xwasm_vm/test_rust_in_c/test_erc20.wasm";

    // std::ifstream file_size(file_path, std::ifstream::ate | std::ifstream::binary);
    // bytes_size = file_size.tellg();

    // file_size.close();

    // std::ifstream in(file_path, std::ifstream::binary);
    // bytes = (uint8_t *)malloc(bytes_size);
    // in.read(reinterpret_cast<char *>(bytes), bytes_size);
    // in.close();


    std::vector<top::xbyte_buffer_t> input;
    input.push_back(erc20_code);
    // symbol
    std::string symbol = "erc20_symbol";
    input.push_back(top::xbyte_buffer_t{symbol.data(), symbol.data() + symbol.size()});
    // supply
    std::string supply = "20000000000";
    input.push_back(top::xbyte_buffer_t{supply.data(), supply.data() + supply.size()});

    // setup contract execution context
    auto tx = top::make_object_ptr<top::data::xtransaction_t>();
    std::string address = xblocktool_t::make_address_user_account("T00000LPggAizKRsMxzS3xwKBRk3Q8qu5xGbz2Q3");
    top::xobject_ptr_t<xvbstate_t> vbstate;
    top::base::xvblock_t::register_object(top::base::xcontext_t::instance());
    vbstate.attach(new xvbstate_t{address, (uint64_t)1, (uint64_t)1, std::string(), std::string(), (uint64_t)0, (uint32_t)0, (uint16_t)0});
    // vbstate.attach(new xvbstate_t{address, 1, std::vector<top::base::xvproperty_t *>()});
    xproperty_access_control_data_t ac_data;
    std::shared_ptr<xproperty_access_control_t> api_ = std::make_shared<xproperty_access_control_t>(top::make_observer(vbstate.get()), ac_data);
    // top::contract_common::xcontract_metadata_t meta;
    top::contract_common::xcontract_state_t * context_ = new top::contract_common::xcontract_state_t{top::common::xaccount_address_t{address}, top::make_observer(api_.get())};
    exe_ctx = new top::contract_common::xcontract_execution_context_t{tx, top::make_observer(context_)};


    top::contract_runtime::user::xwasm_engine_t wasm_engine;
    wasm_engine.deploy_contract_erc20(input, top::make_observer(exe_ctx));

    auto res_symbol_state = exe_ctx->contract_state()->access_control()->STR_PROP_QUERY("symbol");
    EXPECT_EQ(res_symbol_state, "erc20_symbol");

    // call function
    input.clear();
    std::string function_name = "balanceof";
    input.push_back(top::xbyte_buffer_t{function_name.data(), function_name.data() + function_name.size()});
    // just test param
    std::string test_param = "justtestparam";
    input.push_back(top::xbyte_buffer_t{test_param.data(), test_param.data() + test_param.size()});


    wasm_engine.call_contract_erc20(input, top::make_observer(exe_ctx));



}