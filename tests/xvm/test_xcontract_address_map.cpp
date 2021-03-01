#include <gtest/gtest.h>
#include "xvm/manager/xcontract_address_map.h"
#include "xvnetwork/xaddress.h"

using namespace top::common;
using namespace top::contract;

xnode_address_t
make_address(int zone_id, int cluster_id, int group_id) {
    xcluster_address_t cluster_address {
        xnetwork_id_t{0},
        xzone_id_t{static_cast<xzone_id_t::value_type>(zone_id)}, // to be bec network
        xcluster_id_t{static_cast<xcluster_id_t::value_type>(cluster_id)}, // {vnetwork::consensus_cluster_id_base}
        xgroup_id_t{static_cast<xgroup_id_t::value_type>(group_id)}
    };
    if (cluster_address.empty()) {
        return xnode_address_t{};
    }

    return xnode_address_t{cluster_address, xaccount_election_address_t{ xnode_id_t{"account_addr"}, xslot_id_t{} }};
}

TEST(xcontract_address_map_t, calc_cluster_address) {
    // rec contract
    std::string address = xcontract_address_map_t::calc_cluster_address(xaccount_address_t{ "T-x-beacon.contract" }, 0x12).value();
    ASSERT_TRUE(address == "T-x-beacon.contract");

    // zec contract
    address = xcontract_address_map_t::calc_cluster_address(xaccount_address_t{ "T-z-zec.contract" }, 0x23).value();
    ASSERT_TRUE(address == "T-z-zec.contract");

    // sys sharing contract
    address = xcontract_address_map_t::calc_cluster_address(xaccount_address_t{ "T-s-validator.contract" }, 0x23).value();
    ASSERT_TRUE(address == "T-s-validator.contract-0023");
}

TEST(xcontract_address_map_t, match) {
    ASSERT_TRUE(xcontract_address_map_t::match(xaccount_address_t{"T-123456"}, xaccount_address_t{"T-123456"}));
    ASSERT_FALSE(xcontract_address_map_t::match(xaccount_address_t{"T-123456789"}, xaccount_address_t{"89"}));
    ASSERT_FALSE(xcontract_address_map_t::match(xaccount_address_t{"T-1234567"}, xaccount_address_t{"T-123456"}));
    ASSERT_FALSE(xcontract_address_map_t::match(xaccount_address_t{"01234567890123456789012345678901234567"}, xaccount_address_t{"01234567890123456789012345678901234567-0123"}));
}

