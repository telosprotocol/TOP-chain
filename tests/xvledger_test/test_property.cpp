#include "gtest/gtest.h"
#include "xbase/xmem.h"
#include "xvledger/xvaccount.h"
#include "xvledger/xvproperty.h"
#include "xvledger/xvstate.h"

using namespace top;
using namespace top::base;

class test_property : public testing::Test {
protected:
    void SetUp() override {
    }

    void TearDown() override {
    }
};


TEST_F(test_property, object_memory_leak_1)
{
    {
        xauto_ptr<xvcanvas_t> canvas = new xvcanvas_t();
        xobject_ptr_t<base::xvbstate_t> bstate = make_object_ptr<base::xvbstate_t>("T80000733b43e6a2542709dc918ef2209ae0fc6503c2f2", (uint64_t)0, (uint64_t)0, std::string(), std::string(), (uint64_t)0, (uint32_t)0, (uint16_t)0);
        xauto_ptr<xtokenvar_t> token = bstate->new_token_var("@1", canvas.get());
        vtoken_t add_token = (vtoken_t)100;
        vtoken_t value = token->deposit(add_token, canvas.get());
        xassert(value == add_token);

        xassert(canvas->get_refcount() == 1);
        xassert(bstate->get_refcount() == 1);
        xassert(token->get_refcount() == 2);
    }
}


TEST_F(test_property, serialize_compare_1)
{
    {
        uint64_t value = 1;
        std::string vstr = std::to_string(value);
        std::cout << "value = 1 std::to_string size=" << vstr.size() << std::endl;
    }
    {
        uint64_t value = 1;
        base::xstream_t stream(base::xcontext_t::instance());
        stream.write_compact_var(value);
        std::string vstr = std::string((char*)stream.data(), stream.size());
        std::cout << "value = 1 write_compact_var size=" << vstr.size() << std::endl;
    }

    {
        uint64_t value = 123;
        std::string vstr = std::to_string(value);
        std::cout << "value = 123 std::to_string size=" << vstr.size() << std::endl;
    }
    {
        uint64_t value = 123;
        base::xstream_t stream(base::xcontext_t::instance());
        stream.write_compact_var(value);
        std::string vstr = std::string((char*)stream.data(), stream.size());
        std::cout << "value = 123 write_compact_var size=" << vstr.size() << std::endl;
    }

    {
        uint64_t value = 123456789;
        std::string vstr = std::to_string(value);
        xassert(vstr == "123456789");
        xassert(vstr.size() == 9);
        std::cout << "value = 123456789 std::to_string size=" << vstr.size() << std::endl;
    }
    {
        uint64_t value = 123456789;
        base::xstream_t stream(base::xcontext_t::instance());
        stream.write_compact_var(value);
        std::string vstr = std::string((char*)stream.data(), stream.size());
        std::cout << "value = 123456789 write_compact_var size=" << vstr.size() << std::endl;
    }
    {
        uint64_t value = 123456789;
        std::string vstr = base::xstring_utl::uint642hex(value);
        std::cout << "value = 123456789 uint642hex size=" << vstr.size() << std::endl;
    }

    {
        uint64_t value = UINT64_MAX;
        std::string vstr = std::to_string(value);
        std::cout << "value = UINT64_MAX std::to_string size=" << vstr.size() << std::endl;
    }  
    {
        uint64_t value = UINT64_MAX;
        base::xstream_t stream(base::xcontext_t::instance());
        stream.write_compact_var(value);
        std::string vstr = std::string((char*)stream.data(), stream.size());
        std::cout << "value = UINT64_MAX write_compact_var size=" << vstr.size() << std::endl;
    }    
    
    {
        std::string vstr = "123456789abcd";
        base::xstream_t stream(base::xcontext_t::instance());
        stream << vstr;
        std::cout << "vstr = 123456789abcd  << stream size=" << stream.size() << std::endl;
    }    
    {
        std::string vstr = "123456789abcd";
        base::xstream_t stream(base::xcontext_t::instance());
        stream.write_compact_var(vstr);
        std::cout << "vstr = 123456789abcd  write_compact_var stream size=" << stream.size() << std::endl;
    }
    {
        std::string vstr = "123456789abcd";
        base::xstream_t stream(base::xcontext_t::instance());
        stream.write_tiny_string(vstr);
        std::cout << "vstr = 123456789abcd  write_tiny_string stream size=" << stream.size() << std::endl;
    }          
}

TEST_F(test_property, serialize_compare_2)
{
    uint64_t count = 1000000;
    uint64_t i = 0;
    for (uint64_t value = UINT64_MAX; value > 0; value--) {
        if (i++ > count) {
            break;
        }

        base::xstream_t stream(base::xcontext_t::instance());
        stream.write_compact_var(value);
        std::string vstr = std::string((char*)stream.data(), stream.size());
        
        base::xstream_t stream2(base::xcontext_t::instance(), (uint8_t*)vstr.data(), vstr.size());
        uint64_t value2;
        stream.read_compact_var(value2);
        xassert(value2 == value);
    }
}
