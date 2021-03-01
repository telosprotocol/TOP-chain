#include <gtest/gtest.h>
#include "xvm/xsystem_contracts/deploy/xcontract_deploy.h"
#include "xcommon/xnode_type.h"

using namespace top::contract;
using namespace top::common;

TEST(xcontract_deploy_t, deploy) {
    xcontract_deploy_t::instance().deploy(
        xaccount_address_t{"T-x-25bvBwvHrh7CxEqigmD7vJVji8XgxLAf4b"},
        top::common::xnode_type_t::all,
        "",
        enum_broadcast_policy_t::normal,
        "beacon,on_timer,C,10;source.contract1,on_block1,NC");

    xcontract_deploy_t::instance().deploy(
        xaccount_address_t{"T-x-thUWvZvuSTc8jWsPWT15wN1wXyf865ECt"},
        top::common::xnode_type_t::consensus_auditor,
        "",
        enum_broadcast_policy_t::normal,
        "source.contract1,on_block1,C;source.contract2,on_block2,NC");

    xcontract_deploy_t::instance().deploy(
        xaccount_address_t{"T-1-anycontractaddress"},
        top::common::xnode_type_t::consensus_auditor,
        "",
        enum_broadcast_policy_t::normal,
        "source.contract1,on_block1,C;source.contract2,on_block2,NC");

    ASSERT_TRUE(xcontract_deploy_t::instance().find(xaccount_address_t{"any.contract"}) == nullptr);
    ASSERT_TRUE(xcontract_deploy_t::instance().find(xaccount_address_t{"T-x-25bvBwvHrh7CxEqigmD7vJVji8XgxLAf4b"}) != nullptr);
    ASSERT_TRUE(xcontract_deploy_t::instance().find(xaccount_address_t{"T-x-thUWvZvuSTc8jWsPWT15wN1wXyf865ECt"}) != nullptr);
    ASSERT_TRUE(xcontract_deploy_t::instance().find(xaccount_address_t{"T-1-anycontractaddress"}) != nullptr);

    auto const & map = xcontract_deploy_t::instance().get_map();
    xcontract_info_t* info = map.at(xaccount_address_t{"T-x-25bvBwvHrh7CxEqigmD7vJVji8XgxLAf4b"});
    ASSERT_TRUE(info->address == xaccount_address_t{"T-x-25bvBwvHrh7CxEqigmD7vJVji8XgxLAf4b"});
    ASSERT_TRUE(info->roles == top::common::xnode_type_t::all);
    ASSERT_TRUE(info->broadcast_types == top::common::xnode_type_t::invalid);
    ASSERT_TRUE(info->monitor_map.size() == 2);
    ASSERT_TRUE(info->monitor_map[xaccount_address_t{"beacon"}]->action == "on_timer");
    ASSERT_TRUE(info->monitor_map[xaccount_address_t{"beacon"}]->call_way == enum_call_action_way_t::consensus);
    ASSERT_TRUE(info->monitor_map[xaccount_address_t{"beacon"}]->type == enum_monitor_type_t::timer);
    ASSERT_TRUE(info->monitor_map[xaccount_address_t{"beacon"}]->monitor_address == xaccount_address_t{"beacon"});
    //ASSERT_TRUE(((xtimer_block_monitor_info_t*)info->monitor_map["beacon"])->timer_interval == 10);
    ASSERT_TRUE(info->monitor_map[xaccount_address_t{"source.contract1"}]->action == "on_block1");
    ASSERT_TRUE(info->monitor_map[xaccount_address_t{"source.contract1"}]->call_way == enum_call_action_way_t::direct);
    ASSERT_TRUE(info->monitor_map[xaccount_address_t{"source.contract1"}]->type == enum_monitor_type_t::normal);
    ASSERT_TRUE(info->monitor_map[xaccount_address_t{"source.contract1"}]->monitor_address == xaccount_address_t{"source.contract1"});

    info = map.at(xaccount_address_t{"T-x-thUWvZvuSTc8jWsPWT15wN1wXyf865ECt"});
    ASSERT_TRUE(info->address == xaccount_address_t{"T-x-thUWvZvuSTc8jWsPWT15wN1wXyf865ECt"});
    ASSERT_TRUE(info->roles == top::common::xnode_type_t::consensus_auditor);
    ASSERT_TRUE(info->broadcast_types == top::common::xnode_type_t::invalid);
    ASSERT_TRUE(info->monitor_map[xaccount_address_t{"source.contract1"}]->action == "on_block1");
    ASSERT_TRUE(info->monitor_map[xaccount_address_t{"source.contract1"}]->call_way == enum_call_action_way_t::consensus);
    ASSERT_TRUE(info->monitor_map[xaccount_address_t{"source.contract1"}]->type == enum_monitor_type_t::normal);
    ASSERT_TRUE(info->monitor_map[xaccount_address_t{"source.contract1"}]->monitor_address == xaccount_address_t{"source.contract1"});
    ASSERT_TRUE(info->monitor_map[xaccount_address_t{"source.contract2"}]->action == "on_block2");
    ASSERT_TRUE(info->monitor_map[xaccount_address_t{"source.contract2"}]->call_way == enum_call_action_way_t::direct);
    ASSERT_TRUE(info->monitor_map[xaccount_address_t{"source.contract2"}]->type == enum_monitor_type_t::normal);
    ASSERT_TRUE(info->monitor_map[xaccount_address_t{"source.contract2"}]->monitor_address == xaccount_address_t{"source.contract2"});

    info = map.at(xaccount_address_t{"T-1-anycontractaddress"});
    ASSERT_TRUE(info->address == xaccount_address_t{"T-1-anycontractaddress"});
    ASSERT_TRUE(info->roles == top::common::xnode_type_t::consensus_auditor);
    ASSERT_TRUE(info->broadcast_types == top::common::xnode_type_t::invalid);
    ASSERT_TRUE(info->monitor_map.find(xaccount_address_t{"source.contract1"}) == info->monitor_map.end());
    ASSERT_TRUE(info->monitor_map.find(xaccount_address_t{"source.contract2"}) == info->monitor_map.end());
}
