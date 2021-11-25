// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xdata/xelection/xelection_info_bundle.h"
#include "xelection/xcache/xcluster_element.h"
#include "xelection/xcache/xgroup_element.h"
#include "xelection/xcache/xnetwork_element.h"
#include "xelection/xcache/xnode_element.h"
#include "xelection/xcache/xzone_element.h"
#include "xvnode/xvnode.h"
#include "xvnode/xvnode_manager.h"

#include <gtest/gtest.h>

#include <list>
#include <memory>

using top::election::cache::xcluster_element_t;
using top::election::cache::xgroup_element_t;
using top::election::cache::xnetwork_element_t;
using top::election::cache::xnode_element_t;
using top::election::cache::xzone_element_t;

using top::data::election::xelection_info_bundle_t;

using top::common::xcluster_id_t;
using top::common::xgroup_id_t;
using top::common::xnetwork_id_t;
using top::common::xzone_id_t;

using top::common::xnode_id_t;
using top::common::xslot_id_t;

using top::common::xlogic_time_t;
using top::common::xminer_type_t;
using top::common::xelection_round_t;

NS_BEG3(top, tests, vnode)

struct infos {
    xnetwork_id_t network_id;
    xzone_id_t zone_id;
    xcluster_id_t cluster_id;
    xgroup_id_t group_id;
    xslot_id_t slot_id;
    xminer_type_t role;
    common::xelection_round_t version;

    infos(xnetwork_id_t n, xzone_id_t z, xcluster_id_t c, xgroup_id_t g, xslot_id_t s, xminer_type_t r, common::xelection_round_t v) {
        network_id = n;
        zone_id = z;
        cluster_id = c;
        group_id = g;
        slot_id = s;
        role = r;
        version = v;
    }
};

class group_element_generator {
public:
    XDECLARE_DEFAULTED_DEFAULT_CONSTRUCTOR(group_element_generator);
    XDECLARE_DEFAULTED_COPY_AND_MOVE_SEMANTICS(group_element_generator);
    XDECLARE_DEFAULTED_DESTRUCTOR(group_element_generator);

    std::shared_ptr<xgroup_element_t> get_group_element(infos const & id, xnode_id_t node_id = xnode_id_t{std::string("test_node_id")});

private:
    std::shared_ptr<xcluster_element_t> get_cluster_element(infos const & id);
    std::shared_ptr<xzone_element_t> get_zone_element(infos const & id);
    std::shared_ptr<xnetwork_element_t> get_network_element(infos const & id);
};

NS_END3
