#include "gtest/gtest.h"
#include "xvledger/xvaccount.h"
// #include "tests/mock/xvchain_creator.hpp"

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

TEST_F(test_xvledger, xvaccount_tableid) {
    {
        base::xvaccount_t _vaddr("T00000LfhWJA5JPcKPJovoBVtN4seYnnsVjx2VuB");
        auto tableid = _vaddr.get_short_table_id();
        std::cout << "tableid=" << tableid << std::endl;
    }
    {
        base::xvaccount_t _vaddr("T2000138Ao4jjYtrXoNwfzb6gdpD2XNBpqUv46p8B@0");
        auto tableid = _vaddr.get_short_table_id();
        std::cout << "tableid=" << tableid << std::endl;        
    }
}

