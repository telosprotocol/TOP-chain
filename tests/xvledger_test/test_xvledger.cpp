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

TEST_F(test_xvledger, table_address_check_3) {
    std::string unit_address = "T00000LPwjnLXJW9Nb6cXFV5BtoWc8TSbM3vrrRo";
    xvaccount_t _unit_vaddr(unit_address);

    uint32_t low_hash = (uint32_t)xhash64_t::digest(unit_address);
    uint32_t ledger_subaddr = (low_hash & enum_vbucket_has_tables_count_mask);
    std::cout << "T00000LPwjnLXJW9Nb6cXFV5BtoWc8TSbM3vrrRo tableid=" << _unit_vaddr.get_ledger_subaddr() << " subaddr=" << ledger_subaddr << std::endl;    
    ASSERT_EQ(_unit_vaddr.get_ledger_subaddr(), ledger_subaddr);
}

TEST_F(test_xvledger, calc_address_tableid) {
    std::string unit_address = "T00000LN1VBhmjztTBgLPuBCW7iEo9cw58LE1nZF";
    xvaccount_t _unit_vaddr(unit_address);
    std::cout << "T00000LN1VBhmjztTBgLPuBCW7iEo9cw58LE1nZF tableid=" << _unit_vaddr.get_ledger_subaddr() << std::endl;    
    ASSERT_EQ(_unit_vaddr.get_ledger_subaddr(), xvaccount_t::get_ledgersubaddr_from_account(unit_address));
}

TEST_F(test_xvledger, calc_address_tableid_2) {
    std::string unit_address = "T00000LN1VBhmjztTBgLPuBCW7iEo9cw58LE1nZF";
    xvaccount_t _unit_vaddr(unit_address);
    ASSERT_EQ(_unit_vaddr.get_ledger_subaddr(), xvaccount_t::get_ledgersubaddr_from_account(unit_address));
    ASSERT_EQ(_unit_vaddr.get_zone_index(), enum_chain_zone_consensus_index);
    ASSERT_EQ(_unit_vaddr.get_short_table_id(),  (uint16_t)((enum_chain_zone_consensus_index << 10) | _unit_vaddr.get_ledger_subaddr()));

    ASSERT_EQ(_unit_vaddr.get_zone_index(), _unit_vaddr.get_tableid().get_zone_index());
    ASSERT_EQ(_unit_vaddr.get_ledger_subaddr(), _unit_vaddr.get_tableid().get_subaddr());
    ASSERT_EQ(_unit_vaddr.get_short_table_id(), _unit_vaddr.get_tableid().to_table_shortid());

    auto xid = base::xvaccount_t::get_xid_from_account(unit_address);
    ASSERT_EQ((base::enum_xchain_zone_index)get_vledger_zone_index(xid), _unit_vaddr.get_tableid().get_zone_index());
    ASSERT_EQ(get_vledger_subaddr(xid), _unit_vaddr.get_tableid().get_subaddr());
}

TEST_F(test_xvledger, calc_address_tableid_3) {
    {
        std::string unit_address = "Ta0001@2";
        xvaccount_t _unit_vaddr(unit_address);
        ASSERT_EQ(_unit_vaddr.get_ledger_subaddr(), 2);
        ASSERT_EQ(_unit_vaddr.get_zone_index(), enum_chain_zone_beacon_index);
        ASSERT_EQ(_unit_vaddr.get_ledger_subaddr(), _unit_vaddr.get_tableid().get_subaddr());
        ASSERT_EQ(_unit_vaddr.get_short_table_id(),  (uint16_t)((enum_chain_zone_beacon_index << 10) | 2));
        ASSERT_EQ(_unit_vaddr.get_zone_index(), _unit_vaddr.get_tableid().get_zone_index());
        ASSERT_EQ(_unit_vaddr.get_ledger_subaddr(), _unit_vaddr.get_tableid().get_subaddr());
        ASSERT_EQ(_unit_vaddr.get_short_table_id(), _unit_vaddr.get_tableid().to_table_shortid());    
    }
    {
        std::string unit_address = "Ta0002@34";
        xvaccount_t _unit_vaddr(unit_address);
        ASSERT_EQ(_unit_vaddr.get_ledger_subaddr(), 34);
        ASSERT_EQ(_unit_vaddr.get_zone_index(), enum_chain_zone_zec_index);
        ASSERT_EQ(_unit_vaddr.get_ledger_subaddr(), _unit_vaddr.get_tableid().get_subaddr());
        ASSERT_EQ(_unit_vaddr.get_short_table_id(),  (uint16_t)((enum_chain_zone_zec_index << 10) | 34));
        ASSERT_EQ(_unit_vaddr.get_zone_index(), _unit_vaddr.get_tableid().get_zone_index());
        ASSERT_EQ(_unit_vaddr.get_ledger_subaddr(), _unit_vaddr.get_tableid().get_subaddr());
        ASSERT_EQ(_unit_vaddr.get_short_table_id(), _unit_vaddr.get_tableid().to_table_shortid());    
    }
}


// TEST_F(test_xvledger, tx_auto_set_shard_contract_addr) {

//     std::string src_addr = "T00000LPwjnLXJW9Nb6cXFV5BtoWc8TSbM3vrrRo";
//     std::string dst_addr = "T20000MTotTKfAJRxrfvEwEJvtgCqzH9GkpMmAUg";
//     xvaccount_t _src_vaddr(src_addr);

//     using namespace top::data;

//     xtransaction_ptr_t tx = make_object_ptr<xtransaction_t>();
//     tx->set_different_source_target_address(src_addr, dst_addr);
//     auto tableid = data::account_map_to_table_id(common::xaccount_address_t{tx->get_source_addr()});

//     if (is_sys_sharding_contract_address(common::xaccount_address_t{tx->get_target_addr()})) {
//         tx->adjust_target_address(tableid.get_subaddr());
//     }
//     std::cout << "target_addr=" << tx->get_target_addr() << std::endl;    

//     xvaccount_t _target_vaddr(tx->get_target_addr());
//     ASSERT_EQ(_src_vaddr.get_ledger_subaddr(), _target_vaddr.get_ledger_subaddr());
// }


TEST_F(test_xvledger, address_compact_1) {
    {
        std::string address = "T80000077ae60e9d17e4f59fd614a09eae3d1312b2041a";
        std::string compact_addr = base::xvaccount_t::compact_address_to(address);
        std::cout << "address = " << address << " origin size = " << address.size() << " --> compact size = " << compact_addr.size() << std::endl;
        std::string address2 = base::xvaccount_t::compact_address_from(compact_addr);
        EXPECT_EQ(address,address2);
        EXPECT_EQ(46,address.size());
        EXPECT_EQ(21,compact_addr.size());
    }
    {
        std::string address = "T00000LN1VBhmjztTBgLPuBCW7iEo9cw58LE1nZF";
        std::string compact_addr = base::xvaccount_t::compact_address_to(address);
        std::cout << "address = " << address << " origin size = " << address.size() << " --> compact size = " << compact_addr.size() << std::endl;
        std::string address2 = base::xvaccount_t::compact_address_from(compact_addr);
        EXPECT_EQ(address,compact_addr);
        EXPECT_EQ(address,address2);
    }
    {
        std::string address = "T20000MTotTKfAJRxrfvEwEJvtgCqzH9GkpMmAUg@21";
        std::string compact_addr = base::xvaccount_t::compact_address_to(address);
        std::cout << "address = " << address << " origin size = " << address.size() << " --> compact size = " << compact_addr.size() << std::endl;
        std::string address2 = base::xvaccount_t::compact_address_from(compact_addr);
        EXPECT_EQ(address,compact_addr);
        EXPECT_EQ(address,address2);
    }
    {
        std::string address = "Ta0001@2";
        std::string compact_addr = base::xvaccount_t::compact_address_to(address);
        std::cout << "address = " << address << " origin size = " << address.size() << " --> compact size = " << compact_addr.size() << std::endl;
        std::string address2 = base::xvaccount_t::compact_address_from(compact_addr);
        EXPECT_EQ(address,compact_addr);
        EXPECT_EQ(address,address2);
    }
}
