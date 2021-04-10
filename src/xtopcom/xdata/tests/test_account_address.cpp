#include <map>
#include "gtest/gtest.h"
#include "xbase/xmem.h"
#include "xbase/xcontext.h"
// TODO(jimmy) #include "xbase/xvledger.h"
#include "xdata/xdata_defines.h"
#include "xdata/xnative_contract_address.h"
#include "xbasic/xobject_ptr.h"
#include "xdata/xblock.h"
#include "xdata/xproperty.h"
#include "xdata/xdatautil.h"
#include "xdata/xblocktool.h"
#include "xdata/xgenesis_data.h"
#include "xutility/xhash.h"
#include "xcrypto/xcrypto_util.h"
#include "xcrypto/xckey.h"
// TODO(jimmy) #include "xbase/xvledger.h"

using namespace top;
using namespace top::data;

class test_account_address : public testing::Test {
 protected:
    void SetUp() override {
    }

    void TearDown() override {
    }
};

TEST_F(test_account_address, tableblock_addr) {
    uint16_t table_id = 230;
    std::string prefix = sys_contract_sharding_table_block_addr;
    std::string address = xdatautil::serialize_owner_str(prefix, table_id);
    ASSERT_EQ(address, prefix + "@230");

    uint32_t table_id_2;
    std::string prefix_2;
    bool ret = xdatautil::deserialize_owner_str(address, prefix_2, table_id_2);
    ASSERT_TRUE(ret);
    ASSERT_EQ(prefix, prefix_2);
    ASSERT_EQ(table_id, table_id_2);

    ret = xdatautil::extract_table_id_from_address(address, table_id_2);
    ASSERT_TRUE(ret);
    ASSERT_EQ(table_id, table_id_2);
}

TEST_F(test_account_address, tableblock_addr_1) {
    std::string tableblock_public_address = "gRD2qVpp2S7UpjAsznRiRhbE1qNnhMbEDp";
    base::enum_vaccount_addr_type addr_type = base::enum_vaccount_addr_type_block_contract;
    uint16_t main_beacon_ledgerid = base::xvaccount_t::make_ledger_id(base::enum_main_chain_id, base::enum_chain_zone_beacon_index);
    std::string beacon_tableblock_addr = base::xvaccount_t::make_account_address(addr_type, main_beacon_ledgerid, tableblock_public_address, 0);
    std::cout << "beacon_tableblock_addr = " << beacon_tableblock_addr << std::endl;

    uint16_t main_zec_ledgerid = base::xvaccount_t::make_ledger_id(base::enum_main_chain_id, base::enum_chain_zone_zec_index);
    std::string zec_tableblock_addr = base::xvaccount_t::make_account_address(addr_type, main_zec_ledgerid, tableblock_public_address, 0);
    std::cout << "zec_tableblock_addr = " << zec_tableblock_addr << std::endl;

    uint16_t main_consensus_ledgerid = base::xvaccount_t::make_ledger_id(base::enum_main_chain_id, base::enum_chain_zone_consensus_index);
    std::string consensus_tableblock_addr = base::xvaccount_t::make_account_address(addr_type, main_consensus_ledgerid, tableblock_public_address, 0);
    std::cout << "consensus_tableblock_addr = " << consensus_tableblock_addr << std::endl;

    std::cout << data::xblocktool_t::make_address_table_account(base::enum_chain_zone_beacon_index, 0) << std::endl;
    std::cout << data::xblocktool_t::make_address_table_account(base::enum_chain_zone_zec_index, 0) << std::endl;
    std::cout << data::xblocktool_t::make_address_table_account(base::enum_chain_zone_consensus_index, 0) << std::endl;
}

TEST_F(test_account_address, sys_contract_addr) {
    {
        auto              xid = base::xvaccount_t::get_xid_from_account(sys_contract_rec_elect_rec_addr);
        uint8_t           zone = get_vledger_zone_index(xid);
        uint16_t          subaddr = get_vledger_subaddr(xid);
        xassert(zone == base::enum_chain_zone_beacon_index);
        xassert(subaddr == 0);
        xassert(is_beacon_contract_address(common::xaccount_address_t{sys_contract_rec_elect_rec_addr}));
        xassert(is_sys_contract_address(common::xaccount_address_t{sys_contract_rec_elect_rec_addr}));
    }

    {
        auto address    = make_address_by_prefix_and_subaddr(sys_contract_sharding_slash_info_addr, 100);
        auto              xid = base::xvaccount_t::get_xid_from_account(address.value());
        uint8_t           zone = get_vledger_zone_index(xid);
        uint16_t          subaddr = get_vledger_subaddr(xid);
        xassert(zone == base::enum_chain_zone_consensus_index);
        xassert(subaddr == 100);
        xassert(!is_beacon_contract_address(address));
        xassert(is_sys_contract_address(address));
        xassert(is_sys_sharding_contract_address(address));
    }
}

TEST_F(test_account_address, user_addr) {
    {
        // user account address
        std::string public_address = "LejZL6rp7nyH7vtr8Qdj8XoECssSpCW6DY";
        base::enum_vaccount_addr_type addr_type = base::enum_vaccount_addr_type_secp256k1_user_account;
        uint16_t ledgerid = base::xvaccount_t::make_ledger_id(base::enum_main_chain_id, base::enum_chain_zone_consensus_index);
        std::string addr = base::xvaccount_t::make_account_address(addr_type, ledgerid, public_address);
        std::cout << "user addr = " << addr << std::endl;
        ASSERT_EQ(addr, "T00000LejZL6rp7nyH7vtr8Qdj8XoECssSpCW6DY");
    }
    {
        // user account address
        std::string public_address = "LejZL6rp7nyH7vtr8Qdj8XoECssSpCW6DY";
        base::enum_vaccount_addr_type addr_type = base::enum_vaccount_addr_type_secp256k1_user_account;
        uint16_t ledgerid = base::xvaccount_t::make_ledger_id(base::enum_test_chain_id, base::enum_chain_zone_consensus_index);
        std::string addr = base::xvaccount_t::make_account_address(addr_type, ledgerid, public_address);
        std::cout << "user addr = " << addr << std::endl;
        ASSERT_EQ(addr, "T00ff0LejZL6rp7nyH7vtr8Qdj8XoECssSpCW6DY");
    }
    {
        std::string address = "T00000LejZL6rp7nyH7vtr8Qdj8XoECssSpCW6DY";
        auto              xid = base::xvaccount_t::get_xid_from_account(address);
        uint8_t           zone = get_vledger_zone_index(xid);
        uint16_t          subaddr = get_vledger_subaddr(xid);
        std::cout << "T00000LejZL6rp7nyH7vtr8Qdj8XoECssSpCW6DY" << " zone:" << (uint32_t)zone << " subaddr:" << subaddr << std::endl;
        ASSERT_EQ(zone,  base::enum_chain_zone_consensus_index);
        auto tableid = data::account_map_to_table_id(common::xaccount_address_t {address});
        std::cout << "T00000LejZL6rp7nyH7vtr8Qdj8XoECssSpCW6DY" << " zone:" << (uint32_t)tableid.get_zone_index() << " subaddr:" << tableid.get_subaddr() << std::endl;
    }
}

TEST_F(test_account_address, subaddr_calc_1) {
    uint8_t addr_type = base::enum_vaccount_addr_type_secp256k1_user_account;
    uint16_t ledger_id = base::xvaccount_t::make_ledger_id(base::enum_main_chain_id, base::enum_chain_zone_consensus_index);

    std::string addr;
    for (uint32_t i = 0; i < 20; i++) {
        addr = utl::xcrypto_util::make_address_by_random_key(addr_type, ledger_id);

        uint16_t subaddr1 = base::xvaccount_t::get_ledgersubaddr_from_account(addr);
        uint16_t ledger_id = base::xvaccount_t::get_ledgerid_from_account(addr);
        uint8_t zoneid1 = base::xvaccount_t::get_zoneindex_from_ledgerid(ledger_id);

        std::cout << "addr=" << addr << " _subaddr=" << subaddr1 << std::endl;

        auto xid = base::xvaccount_t::get_xid_from_account(addr);
        uint8_t zoneid2 = get_vledger_zone_index(xid);
        uint16_t subaddr2 = get_vledger_subaddr(xid);
        ASSERT_EQ(subaddr1, subaddr2);
        ASSERT_EQ(zoneid1, zoneid2);

        base::xvaccount_t vaccount{addr};
        ASSERT_EQ(subaddr1, vaccount.get_ledger_subaddr());
        ASSERT_EQ(zoneid1, vaccount.get_zone_index());
    }
}

TEST_F(test_account_address, subaddr_calc_BENCH) {
    uint8_t addr_type = base::enum_vaccount_addr_type_secp256k1_user_account;
    uint16_t ledger_id = base::xvaccount_t::make_ledger_id(base::enum_main_chain_id, base::enum_chain_zone_consensus_index);

    uint32_t tableid_count[enum_vledger_const::enum_vbucket_has_tables_count];
    for (size_t i = 0; i < enum_vbucket_has_tables_count; i++) {
        tableid_count[i] = 0;
    }

    uint32_t total_count = 100000;
    std::string addr;
    std::string last_addr;
    for (uint32_t i = 0; i < total_count; i++) {
        do {
            addr = utl::xcrypto_util::make_address_by_random_key(addr_type, ledger_id);
            if (addr != last_addr) {
                break;
            }
        } while (1);
        last_addr = addr;
        uint16_t subaddr = base::xvaccount_t::get_ledgersubaddr_from_account(addr);
        tableid_count[subaddr]++;
    }
    uint32_t min = 0;
    uint32_t min_count = 0;
    while (min < total_count) {
        for (size_t i = 0; i < enum_vbucket_has_tables_count; i++) {
            if (tableid_count[i] == min) {
                if (min_count++ > 10) {
                    break;
                }
                std::cout << "subaddr=" << i << " count=" <<  tableid_count[i] << std::endl;
            }
        }
        min++;
    }
    uint32_t max = total_count;
    uint32_t max_count = 0;
    while (max > 0) {
        for (size_t i = 0; i < enum_vbucket_has_tables_count; i++) {
            if (tableid_count[i] == max) {
                if (max_count++ > 10) {
                    break;
                }
                std::cout << "subaddr=" << i << " count=" <<  tableid_count[i] << std::endl;
            }
        }
        max--;
    }
}

TEST_F(test_account_address, subaddr_calc_3) {
    uint8_t addr_type = base::enum_vaccount_addr_type_secp256k1_user_account;
    uint16_t ledger_id = base::xvaccount_t::make_ledger_id(base::enum_main_chain_id, base::enum_chain_zone_consensus_index);

    std::string addr;
    for (uint32_t i = 0; i < 1000; i++) {
        addr = utl::xcrypto_util::make_address_by_random_key(addr_type, ledger_id);

        uint16_t subaddr1 = base::xvaccount_t::get_ledgersubaddr_from_account(addr);
        xassert(subaddr1 < enum_vbucket_has_tables_count);
        uint16_t ledger_id = base::xvaccount_t::get_ledgerid_from_account(addr);
        uint8_t zoneid1 = base::xvaccount_t::get_zoneindex_from_ledgerid(ledger_id);
        xassert(base::xvaccount_t::get_book_index_from_subaddr(subaddr1) < enum_vbucket_has_books_count);
        xassert(base::xvaccount_t::get_table_index_from_subaddr(subaddr1) < enum_vbook_has_tables_count);

        auto xid = base::xvaccount_t::get_xid_from_account(addr);
        uint8_t zoneid2 = get_vledger_zone_index(xid);
        uint16_t subaddr2 = get_vledger_subaddr(xid);
        ASSERT_EQ(subaddr1, subaddr2);
        ASSERT_EQ(zoneid1, zoneid2);
    }

    uint16_t subaddr = base::xvaccount_t::make_subaddr_of_ledger(0xFF, 0xFF);
    xassert(subaddr == enum_vbucket_has_tables_count - 1);

    std::string prefix;
    bool ret = base::xvaccount_t::get_prefix_subaddr_from_account("T-2-123444444444444444444@2000", prefix, subaddr);
    xassert(ret == true);
    xassert(prefix == "T-2-123444444444444444444");
    xassert(subaddr == (2000 & enum_vbucket_has_tables_count_mask));
}

TEST_F(test_account_address, subaddr_calc_5) {
    uint8_t addr_type = base::enum_vaccount_addr_type_secp256k1_user_account;
    uint16_t ledger_id = base::xvaccount_t::make_ledger_id(base::enum_main_chain_id, base::enum_chain_zone_consensus_index);

    std::string addr;
    for (uint32_t i = 0; i < 20; i++) {
        addr = xblocktool_t::make_address_shard_table_account(i);

        uint16_t subaddr1 = base::xvaccount_t::get_ledgersubaddr_from_account(addr);
        uint16_t ledger_id = base::xvaccount_t::get_ledgerid_from_account(addr);
        uint8_t zoneid1 = base::xvaccount_t::get_zoneindex_from_ledgerid(ledger_id);

        std::cout << "addr=" << addr << " _subaddr=" << subaddr1 << std::endl;

        auto xid = base::xvaccount_t::get_xid_from_account(addr);
        uint8_t zoneid2 = get_vledger_zone_index(xid);
        uint16_t subaddr2 = get_vledger_subaddr(xid);
        ASSERT_EQ(subaddr1, subaddr2);
        ASSERT_EQ(zoneid1, zoneid2);

        base::xvaccount_t vaccount{addr};
        ASSERT_EQ(subaddr1, vaccount.get_ledger_subaddr());
        ASSERT_EQ(zoneid1, vaccount.get_zone_index());
    }
}

