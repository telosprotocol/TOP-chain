#include "gtest/gtest.h"
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
        xobject_ptr_t<base::xvbstate_t> bstate = make_object_ptr<base::xvbstate_t>("aaa", (uint64_t)0, (uint64_t)0, std::string(), std::string(), (uint64_t)0, (uint32_t)0, (uint16_t)0);
        xauto_ptr<xtokenvar_t> token = bstate->new_token_var("@1", canvas.get());
        vtoken_t add_token = (vtoken_t)100;
        vtoken_t value = token->deposit(add_token, canvas.get());
        xassert(value == add_token);

        xassert(canvas->get_refcount() == 1);
        xassert(bstate->get_refcount() == 1);
        xassert(token->get_refcount() == 2);
    }
}
