#include "xbasic/xbyte_buffer.h"
#include "xbasic/xhex.h"
#include "xbasic/xstring.h"
#include "xwasmer_runner/wasmer_engine_interface.h"
#include "xwasmer_runner/wasmer_import_instance.h"
#include "xwasmer_runner/wasmer_logic_face.h"

#include <gtest/gtest.h>

NS_BEG3(top, wasm, tests)

class xtest_vm_logic_face_t : public wasm::xwasmer_logic_face_t {
    uint64_t block_index() override {
        return 666;
    }
};

TEST(wasmer, demo_1) {
    std::shared_ptr<wasm::xwasmer_logic_face_t> logic_ptr = std::make_shared<xtest_vm_logic_face_t>();
    wasmer_import_instance::instance()->add_wasmer_logic(logic_ptr);

    std::string hex_string_code = std::string{
        "0x0061736D01000000010F036000017E60027F7F017F6000017F02130103656E760B626C6F636B5F696E6465780000030403010002050301000007260403616464000109626C6B5F696E6465780002076E6F746869"
        "6E670003066D656D6F727902000A13030700200020016A0B040010000B040041030B002410736F757263654D617070696E6755524C122E2F72656C656173652E7761736D2E6D6170"};

    std::error_code ec;
    auto code = top::from_hex(hex_string_code, ec);
    ASSERT_TRUE(!ec);
    std::string code_str = top::to_string(code);

    auto contract = deploy_contract((unsigned char *)code_str.data(), code_str.size());

    set_gas_left(contract, 1000);

    call_contract_1(contract, 10, 20);
    call_contract_2(contract);
    call_contract_3(contract);

    wasmer_import_instance::instance()->remove_wasmer_logic();
}
NS_END3