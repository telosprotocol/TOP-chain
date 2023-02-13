// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include "xbasic/xsimple_message.hpp"
#include "xcommon/xmessage_id.h"
#include "xvnetwork/xmessage.h"
#include "xvnetwork/xvhost.h"
#include "xvnetwork/xvnetwork_driver.h"

#include <gtest/gtest.h>

#include <memory>

NS_BEG3(top, tests, vnetwork)

class xtop_vnetwork_fixture : public testing::Test {
public:
    XDECLARE_DEFAULTED_DEFAULT_CONSTRUCTOR(xtop_vnetwork_fixture);
    XDECLARE_DELETED_COPY_DEFAULTED_MOVE_SEMANTICS(xtop_vnetwork_fixture);
    XDECLARE_DEFAULTED_OVERRIDE_DESTRUCTOR(xtop_vnetwork_fixture);

    common::xelection_round_t         test_version1{1}, test_version0{0};
    common::xnetwork_version_t test_network_version1{1};
    common::xnetwork_id_t      test_network_id{1}, test_network_id2{2};
    common::xzone_id_t         test_zone_id{1};
    common::xcluster_id_t      test_cluster_id{0};
    common::xgroup_id_t        test_group_id{65};

    top::vnetwork::xmessage_t test_msg{xbyte_buffer_t{}, xmessage_id_sync_blocks};

    top::vnetwork::xvnode_address_t get_address(common::xelection_round_t version, common::xnetwork_id_t network_id);

    top::common::xip2_t get_xip2_address(common::xnetwork_id_t network_id);

    top::common::xip2_t get_xip2_address(common::xnetwork_id_t      network_id,
                                         common::xzone_id_t         zone_id,
                                         common::xcluster_id_t      cluster_id,
                                         common::xgroup_id_t        group_id);

    top::vnetwork::xvnode_address_t get_address(common::xelection_round_t    version,
                                                common::xnetwork_id_t network_id,
                                                common::xzone_id_t    zone_id,
                                                common::xcluster_id_t cluster_id,
                                                common::xgroup_id_t   group_id);

    top::vnetwork::xvnode_address_t get_dst_group_address(common::xelection_round_t    version,
                                                          common::xnetwork_id_t network_id,
                                                          common::xzone_id_t    zone_id,
                                                          common::xcluster_id_t cluster_id,
                                                          common::xgroup_id_t   group_id);
};
using xvnetwork_fixture_t = xtop_vnetwork_fixture;

class xtop_vhost_fixture : public xvnetwork_fixture_t {
public:
    XDECLARE_DEFAULTED_DEFAULT_CONSTRUCTOR(xtop_vhost_fixture);
    XDECLARE_DELETED_COPY_DEFAULTED_MOVE_SEMANTICS(xtop_vhost_fixture);
    XDECLARE_DEFAULTED_OVERRIDE_DESTRUCTOR(xtop_vhost_fixture);

protected:
    void SetUp() override;

    void TearDown() override;

    std::shared_ptr<top::vnetwork::xvhost_t> vhost_test_ptr;
    common::xnode_address_t                  src_v1, src_v2, dst_v1, empty_dst, dst_group_address;
};
using xvhost_fixture_t = xtop_vhost_fixture;

class xtop_vnetwork_driver_fixture : public xvnetwork_fixture_t {
public:
    XDECLARE_DEFAULTED_DEFAULT_CONSTRUCTOR(xtop_vnetwork_driver_fixture);
    XDECLARE_DELETED_COPY_DEFAULTED_MOVE_SEMANTICS(xtop_vnetwork_driver_fixture);
    XDECLARE_DEFAULTED_OVERRIDE_DESTRUCTOR(xtop_vnetwork_driver_fixture);

protected:
    void SetUp() override;

    void TearDown() override;

    std::shared_ptr<top::vnetwork::xvhost_t>           vhost_test_ptr;
    std::shared_ptr<top::vnetwork::xvnetwork_driver_t> vnetwork_driver_test_ptr;
};
using xvnetwork_driver_fixture_t = xtop_vnetwork_driver_fixture;

NS_END3