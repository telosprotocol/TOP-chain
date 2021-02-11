#include <vector>
#include <iostream>

#include "gtest/gtest.h"
#include "xbase/xint.h"
#include "xbase/xmem.h"
#include "xbase/xcontext.h"
#include "xbase/xutl.h"
#include "xutility/xcallback_mgr.hpp"

using namespace top;
using namespace top::utl;

class test_cbmgr : public testing::Test {
protected:
    void SetUp() override {
    }

    void TearDown() override {
    }
};


using test_cbmgr_cb_t = std::function<void()>;

class test_cbmgr_t {
 public:
    void test_fun1() {
        std::cout << "test_fun1" << std::endl;
    }
};

TEST_F(test_cbmgr, cbmgr_register) {
    xcallback_mgr<std::string, test_cbmgr_cb_t> mgr;
    test_cbmgr_t test_cb;
    mgr.register_cb("aaa", std::bind(&test_cbmgr_t::test_fun1, &test_cb));
    mgr.register_cb("bbb", std::bind(&test_cbmgr_t::test_fun1, &test_cb));

    test_cbmgr_cb_t cb1 = mgr.get_cb("aaa");
    cb1();
    test_cbmgr_cb_t cb2 = mgr.get_cb("bbb");
    cb2();
}


