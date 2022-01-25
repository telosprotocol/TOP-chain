// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "tests/xvnode/xgroup_element_generator.h"
#include "xdata/xelection/xelection_info_bundle.h"
#include "xdata/xelection/xstandby_node_info.h"
#include "xelection/xcache/xcluster_element.h"
#include "xelection/xcache/xgroup_element.h"
#include "xelection/xcache/xnetwork_element.h"
#include "xelection/xcache/xzone_element.h"

#include <gtest/gtest.h>

#include <memory>

using top::election::cache::xcluster_element_t;
using top::election::cache::xgroup_element_t;
using top::election::cache::xnetwork_element_t;
using top::election::cache::xnode_element_t;
using top::election::cache::xzone_element_t;

using top::common::xcluster_id_t;
using top::common::xgroup_id_t;
using top::common::xnetwork_id_t;
using top::common::xelection_round_t;
using top::common::xzone_id_t;
using top::common::xnode_id_t;
using top::common::xslot_id_t;
using top::common::xminer_type_t;

using top::data::election::xelection_info_bundle_t;
using top::data::election::xelection_info_t;
using top::data::election::xelection_network_result_t;
using top::data::election::xelection_result_store_t;
using top::data::election::xelection_result_t;
using top::data::election::xstandby_node_info_t;

using top::common::xrotation_status_t;

NS_BEG3(top, tests, vnode)

std::uint16_t sharding_size{1};
std::uint64_t associated_election_blk_height{1};

std::shared_ptr<xgroup_element_t> group_element_generator::get_group_element(infos const & id, xnode_id_t node_id) {
    std::map<top::common::xslot_id_t, xelection_info_bundle_t> mp;
    mp.clear();

    std::shared_ptr<xgroup_element_t> group_element =
        std::make_shared<xgroup_element_t>(id.version, id.group_id, sharding_size, associated_election_blk_height, get_cluster_element(id));

    top::data::election::xelection_info_t new_election_info{};
    new_election_info.joined_version = id.version;
    new_election_info.stake = 0;
    new_election_info.comprehensive_stake = 0;
    new_election_info.consensus_public_key = top::xpublic_key_t{u8"test_public_key"};

    xelection_info_bundle_t election_info_bundle;
    election_info_bundle.node_id(node_id);
    election_info_bundle.election_info(std::move(new_election_info));

    mp[id.slot_id] = election_info_bundle;

    group_element->set_node_elements(mp);
    group_element->rotation_status(xrotation_status_t::started, 0);
    group_element->rotation_status(xrotation_status_t::faded, 0);
    group_element->rotation_status(xrotation_status_t::outdated, 0);
    return group_element;
}

std::shared_ptr<xcluster_element_t> group_element_generator::get_cluster_element(infos const & id) {
    return std::make_shared<xcluster_element_t>(xcluster_id_t{id.cluster_id}, get_zone_element(id));
}
std::shared_ptr<xzone_element_t> group_element_generator::get_zone_element(infos const & id) {
    return std::make_shared<xzone_element_t>(xzone_id_t{id.zone_id}, get_network_element(id));
}
std::shared_ptr<xnetwork_element_t> group_element_generator::get_network_element(infos const & id) {
    return std::make_shared<xnetwork_element_t>(xnetwork_id_t{id.network_id});
}

NS_END3
