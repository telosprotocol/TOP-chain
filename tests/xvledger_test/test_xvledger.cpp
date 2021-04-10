#include "gtest/gtest.h"
#include "xvledger/xvaccount.h"

using namespace top;
using namespace top::base;

class test_xvledger : public testing::Test {
protected:
    void SetUp() override {
    }

    void TearDown() override {
    }
};


TEST_F(test_xvledger, xvaccount_1) {
    std::string addr = "123456789";
    xvaccount_t vaddr(addr);
    std::cout << "vaddr=" << vaddr.get_xvid() << std::endl;
}


