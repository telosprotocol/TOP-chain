#include <iostream>
#include <fstream>
#include "gtest/gtest.h"
#include "xcontract_runtime/xuser/xwasm/xwasm_engine.h"

#if defined(__clang__)
#    pragma clang diagnostic push
#    pragma clang diagnostic ignored "-Wpedantic"
#elif defined(__GNUC__)
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wpedantic"
#elif defined(_MSC_VER)
#    pragma warning(push, 0)
#endif

#include "xcontract_common/xcontract_state.h"
#include "xcontract_common/xbasic_contract.h"
#include "xbase/xmem.h"
#include "xdata/xblocktool.h"
#include "xstore/xstore.h"


#if defined(__clang__)
#    pragma clang diagnostic pop
#elif defined(__GNUC__)
#    pragma GCC diagnostic pop
#elif defined(_MSC_VER)
#    pragma warning(pop)
#endif



NS_BEG3(top, tests, contract_runtime)

using namespace top::base;
using namespace top::data;
using namespace top::contract_common::properties;

extern "C" bool validate_wasm_with_content(uint8_t *s, uint32_t size);

class test_wasm_contract : public testing::Test {
public:
    top::contract_common::xcontract_execution_context_t* exe_ctx;

protected:
    void SetUp() override {
        using namespace base;
        auto tx = top::make_object_ptr<top::data::xtransaction_t>();

        std::string address = xblocktool_t::make_address_user_account("T00000LPggAizKRsMxzS3xwKBRk3Q8qu5xGbz2Q3");
        top::xobject_ptr_t<xvbstate_t> vbstate;
        vbstate.attach(new xvbstate_t{address, 1,std::vector<top::base::xvproperty_t*>()});
        xproperty_access_control_data_t ac_data;
        std::shared_ptr<xproperty_access_control_t> api_ = std::make_shared<xproperty_access_control_t>(top::make_observer(vbstate.get()), ac_data);

        top::contract_common::xcontract_metadata_t meta;
        top::contract_common::xcontract_state_t* context_ =  new top::contract_common::xcontract_state_t{top::common::xaccount_address_t{address}, top::make_observer(api_.get())};
        exe_ctx = new top::contract_common::xcontract_execution_context_t{tx, top::make_observer(context_)};

    }
};

TEST_F(test_wasm_contract, wasm_contract_test_deploy) {
    uint8_t *bytes;
    uint32_t bytes_size;

    char const* file_path = "/home/wens/top_projects/github_xchain/src/xtopcom/xcontract_runtime/xuser/test/example/add.wasm";

    std::ifstream file_size(file_path, std::ifstream::ate | std::ifstream::binary);
    bytes_size = file_size.tellg();
    std::cout << bytes_size << "\n";

    file_size.close();

    std::ifstream in(file_path, std::ifstream::binary);
    bytes = (uint8_t *)malloc(bytes_size);
    in.read(reinterpret_cast<char *>(bytes), bytes_size);
    in.close();

    std::cout << bytes_size << "\n";
    EXPECT_TRUE(validate_wasm_with_content(bytes, bytes_size));

    top::contract_runtime::user::xwasm_engine_t wasm_engine;
    xbyte_buffer_t buffer{bytes, bytes + bytes_size};
    std::cout << buffer.size() << "\n";

    // wasm_engine.deploy_contract(buffer, top::make_observer(exe_ctx));

}



NS_END3