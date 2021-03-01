#include <gtest/gtest.h>
#include "xvm/xcontract/xcontract_base.h"
#include "xbase/xmem.h"
#include "xbase/xcontext.h"
#include "xstore/xaccount_context.h"
#include "xstore/test/test_datamock.hpp"
#include "xbasic/xserializable_based_on.h"

using namespace top::store;
using namespace top::data;
using namespace top::xvm::xcontract;
using namespace top::base;
using namespace top::xvm;

class test_xcontract_base_sub : public xcontract_base, public testing::Test {
public:
    using xbase_t = xcontract_base;
    XDECLARE_DELETED_COPY_DEFAULTED_MOVE_SEMANTICS(test_xcontract_base_sub);
    XDECLARE_DEFAULTED_OVERRIDE_DESTRUCTOR(test_xcontract_base_sub);

    test_xcontract_base_sub() : xbase_t{ top::common::xtopchain_network_id } {
    }

    xcontract_base* clone() override {return nullptr;}
    void exec(xvm_context* vm_ctx) {}
};

struct test_xcontract_base_entity final : public xserializable_based_on<void> {
    test_xcontract_base_entity(uint32_t v) :
    value(v) {}

private:
    int32_t do_read(xstream_t& stream) override {
        uint32_t size = stream.size();
        stream >> value;
        return size - stream.size();
    }

    int32_t do_write(xstream_t& stream) const override {
        uint32_t size = stream.size();
        stream << value;
        return stream.size() - size;
    }

public:
    int32_t value{};
};

// xblock_ptr_t test_xcontract_base_create_block(test_datamock_t& dm, const std::string& address, const test_xcontract_base_entity& entity) {
//     xstream_t stream(xcontext_t::instance());
//     entity.serialize_to(stream);
//     std::string value((char*) stream.data(), stream.size());
//     auto tx = make_object_ptr<xlightunit_state_t>();
//     tx->m_native_property.native_map_set(data::XPORPERTY_CONTRACT_BLOCK_CONTENT_KEY, "_default", value);
//     auto block = dm.create_lightunit(address, make_object_ptr<xlightunit_input_t>(), tx);
//     dm.m_store->set_block(block);
//     return block;
// }

TEST_F(test_xcontract_base_sub, block_get_and_set) {
    // auto store_ptr = store::xstore_factory::create_store_with_memdb(nullptr);
    // test_datamock_t dm(store_ptr.get());
    // std::string address = "test.contract";

    // xaccount_context_t ac(address, store_ptr.get());
    // set_contract_helper(make_shared<xcontract_helper>(&ac, top::common::xaccount_address_t{address}, address));

    // auto block = test_xcontract_base_create_block(dm, address, 111);

    // std::string value = READ_FROM_BLOCK(block, "_default");
    // ASSERT_TRUE(!value.empty());

    // xstream_t stream(xcontext_t::instance(), (uint8_t*) value.data(), value.size());
    // test_xcontract_base_entity ent(0);
    // ent.serialize_from(stream);
    // ASSERT_TRUE(ent.value == 111);
}
