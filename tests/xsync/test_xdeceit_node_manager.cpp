#include <gtest/gtest.h>
#include <vector>
#include "xsync/xdeceit_node_manager.h"
#include "common.h"

using namespace top::sync;
using namespace top::vnetwork;

extern
xvnode_address_t
get_address(int id, int version);

TEST(xdeceit_node_manager_t, tests) {

    xdeceit_node_manager_t blacklist;

    auto list = get_validator_addresses(0, 0, 0, 5);
    for(auto& addr : list) {
        blacklist.add_deceit_node(addr);
    }

    blacklist.filter_deceit_nodes([&](const std::set<xaccount_address_t>& _set) {
        for(auto& account_addr : _set) {
            bool find = false;
            for(auto& addr : list) {
                if(addr.account_address() == account_addr) {
                    find = true;
                    break;
                }
            }
            ASSERT_TRUE(find);
        }
    });
}

