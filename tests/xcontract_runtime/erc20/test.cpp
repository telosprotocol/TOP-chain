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
        top::xobject_ptr_t<xvbstate_t> vbstate;
        // vbstate.attach(new xvbstate_t{address, 1, std::vector<top::base::xvproperty_t *>()});
        xproperty_access_control_data_t ac_data;
        std::shared_ptr<xproperty_access_control_t> api_ = std::make_shared<xproperty_access_control_t>(top::make_observer(vbstate.get()), ac_data);

        // top::contract_common::xcontract_metadata_t meta;
        top::contract_common::xcontract_state_t * context_ = new top::contract_common::xcontract_state_t{top::common::xaccount_address_t{address}, top::make_observer(api_.get())};
        exe_ctx = new top::contract_common::xcontract_execution_context_t{tx, top::make_observer(context_)};
    }
};

TEST_F(test_erc20, _2) {
    // wasm bytes
    uint8_t * bytes;
    uint32_t bytes_size;

    char const * file_path = "/home/charles/Eluvk-project/TOP-chain/test_erc20.wasm";

    std::ifstream file_size(file_path, std::ifstream::ate | std::ifstream::binary);
    bytes_size = file_size.tellg();

    file_size.close();

    std::ifstream in(file_path, std::ifstream::binary);
    bytes = (uint8_t *)malloc(bytes_size);
    in.read(reinterpret_cast<char *>(bytes), bytes_size);
    in.close();


    std::vector<top::xbyte_buffer_t> input;
    input.push_back(top::xbyte_buffer_t{bytes, bytes + bytes_size});

    top::contract_runtime::user::xwasm_engine_t wasm_engine;
    wasm_engine.deploy_contract_erc20(input, top::make_observer(exe_ctx));
}