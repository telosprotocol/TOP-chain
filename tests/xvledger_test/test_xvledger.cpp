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


TEST_F(test_xvledger, table_address_check_1) {
    {
        std::string prefix = "Ta0001";
        std::string address = base::xvaccount_t::make_account_address(prefix, 1);
        base::xvaccount_t _vaddr(address);
        ASSERT_EQ(_vaddr.get_zone_index(), enum_chain_zone_beacon_index);
        ASSERT_EQ(_vaddr.get_ledger_subaddr(), 1);
        ASSERT_EQ(_vaddr.get_addr_type(), enum_vaccount_addr_type_block_contract);

        std::string public_address;
        xvaccount_t::get_public_address_from_account(address, public_address);
        ASSERT_TRUE(public_address.empty());

        // ASSERT_EQ(_vaddr.get_xvid());
        std::cout << "xvid=" << _vaddr.get_xvid() << std::endl;
        std::cout << "xvid_str=" << _vaddr.get_xvid_str() << std::endl;
        std::cout << "tableid=" << _vaddr.get_short_table_id() << std::endl;
    }
    {
        std::string prefix = "Ta0002";
        std::string address = base::xvaccount_t::make_account_address(prefix, 25);
        base::xvaccount_t _vaddr(address);
        ASSERT_EQ(_vaddr.get_zone_index(), enum_chain_zone_zec_index);
        ASSERT_EQ(_vaddr.get_ledger_subaddr(), 25);
        ASSERT_EQ(_vaddr.get_addr_type(), enum_vaccount_addr_type_block_contract);

        std::string public_address;
        xvaccount_t::get_public_address_from_account(address, public_address);
        ASSERT_TRUE(public_address.empty());

        // ASSERT_EQ(_vaddr.get_xvid());
        std::cout << "xvid=" << _vaddr.get_xvid() << std::endl;
        std::cout << "xvid_str=" << _vaddr.get_xvid_str() << std::endl;
        std::cout << "tableid=" << _vaddr.get_short_table_id() << std::endl;
    }
    {
        std::string prefix = "Ta0000";
        std::string address = base::xvaccount_t::make_account_address(prefix, 25);
        base::xvaccount_t _vaddr(address);
        ASSERT_EQ(_vaddr.get_zone_index(), enum_chain_zone_consensus_index);
        ASSERT_EQ(_vaddr.get_ledger_subaddr(), 25);
        ASSERT_EQ(_vaddr.get_addr_type(), enum_vaccount_addr_type_block_contract);

        std::string public_address;
        xvaccount_t::get_public_address_from_account(address, public_address);
        ASSERT_TRUE(public_address.empty());

        // ASSERT_EQ(_vaddr.get_xvid());
        std::cout << "xvid=" << _vaddr.get_xvid() << std::endl;
        std::cout << "xvid_str=" << _vaddr.get_xvid_str() << std::endl;
        std::cout << "tableid=" << _vaddr.get_short_table_id() << std::endl;
    }    
}

TEST_F(test_xvledger, table_address_check_2) {
    {
        std::string unit_address = "T80000077ae60e9d17e4f59fd614a09eae3d1312b2041a";
        xvaccount_t _unit_vaddr(unit_address);

        std::string table_address = xvaccount_t::make_table_account_address(_unit_vaddr);
        std::cout << "table_address=" << table_address << std::endl;
    }
    {
        std::string unit_address = "T200024uN3e6AujFyvDXY4h5t6or3DgKpu5rTKELD@2";
        xvaccount_t _unit_vaddr(unit_address);

        std::string table_address = xvaccount_t::make_table_account_address(_unit_vaddr);
        std::cout << "table_address=" << table_address << std::endl;
    }    
    {
        std::string unit_address = "T20000MTotTKfAJRxrfvEwEJvtgCqzH9GkpMmAUg@21";
        xvaccount_t _unit_vaddr(unit_address);

        std::string table_address = xvaccount_t::make_table_account_address(_unit_vaddr);
        std::cout << "table_address=" << table_address << std::endl;
    }        
}
