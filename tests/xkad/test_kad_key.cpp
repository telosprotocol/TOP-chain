#include "xbase/xbase.h"
#include "xcommon/xip.h"
#include "xpbase/base/kad_key/kadmlia_key.h"

#include <gtest/gtest.h>

namespace top {
namespace test {

TEST(xkad_test, _kad_key_1) {
    common::xnetwork_id_t network_id{255};
    common::xzone_id_t zid = common::xdefault_zone_id;
    common::xcluster_id_t cluster_id = common::xdefault_cluster_id;
    common::xgroup_id_t group_id = common::xdefault_group_id;
    common::xslot_id_t slot_id{0};

    uint16_t size = 10;
    uint64_t height = 0;

    common::xip2_t xip2_0{network_id, zid, cluster_id, group_id, slot_id, size, (uint64_t)0};
    common::xip2_t xip2_1{network_id, zid, cluster_id, group_id, slot_id, size, (uint64_t)1};
    common::xip2_t xip2_2{network_id, zid, cluster_id, group_id, slot_id, size, (uint64_t)2};

    auto s0 = base::GetKadmliaKey(xip2_0)->GetServiceType();
    auto s1 = base::GetKadmliaKey(xip2_1)->GetServiceType();
    auto s2 = base::GetKadmliaKey(xip2_2)->GetServiceType();

    EXPECT_TRUE(s2.IsNewer(s0, 2));
    EXPECT_TRUE(s2.IsNewer(s1, 1));
    EXPECT_FALSE(s2.IsNewer(s0, 3));
}

TEST(xkad_test, _kad_key_2) {
    common::xnetwork_id_t network_id{255};
    common::xzone_id_t zid = common::xdefault_zone_id;
    common::xzone_id_t zid2 = common::xcommittee_zone_id;
    common::xcluster_id_t cluster_id = common::xdefault_cluster_id;
    common::xcluster_id_t cluster_id2 = common::xcommittee_cluster_id;
    common::xgroup_id_t group_id = common::xdefault_group_id;
    common::xgroup_id_t group_id2 = common::xcommittee_group_id;
    common::xslot_id_t slot_id{0};

    uint16_t size = 10;
    uint64_t height = 0;

    common::xip2_t xip2_0{network_id, zid, cluster_id, group_id, slot_id, size, height};
    common::xip2_t xip2_o_1{network_id, zid2, cluster_id, group_id, slot_id, size, (uint64_t)2};
    common::xip2_t xip2_o_2{network_id, zid, cluster_id2, group_id, slot_id, size, (uint64_t)2};
    common::xip2_t xip2_o_3{network_id, zid, cluster_id, group_id2, slot_id, size, (uint64_t)2};

    auto s0 = base::GetKadmliaKey(xip2_0)->GetServiceType();
    auto s_o_1 = base::GetKadmliaKey(xip2_o_1)->GetServiceType();
    auto s_o_2 = base::GetKadmliaKey(xip2_o_2)->GetServiceType();
    auto s_o_3 = base::GetKadmliaKey(xip2_o_3)->GetServiceType();

    EXPECT_FALSE(s_o_1.IsNewer(s0, 2));
    EXPECT_FALSE(s_o_2.IsNewer(s0, 2));
    EXPECT_FALSE(s_o_3.IsNewer(s0, 2));
}

TEST(xkad_test, _kad_key_3) {
    common::xnetwork_id_t network_id{255};
    common::xzone_id_t zid = common::xdefault_zone_id;
    common::xcluster_id_t cluster_id = common::xdefault_cluster_id;
    common::xgroup_id_t group_id = common::xdefault_group_id;
    common::xslot_id_t slot_id{0};

    uint16_t size = 10;
    uint64_t height = 0;

    common::xip2_t xip2_0{network_id, zid, cluster_id, group_id, slot_id, size, height};
    common::xip2_t xip2_1{network_id, zid, cluster_id, group_id, slot_id, size, common::xversion_t::max().value()};

    auto s0 = base::GetKadmliaKey(xip2_0)->GetServiceType();
    auto s1 = base::GetKadmliaKey(xip2_1)->GetServiceType();

    EXPECT_FALSE(s0.IsBroadcastService());
    EXPECT_TRUE(s1.IsBroadcastService());
    EXPECT_FALSE(s1.IsNewer(s0));
}

}  // namespace test
}  // namespace top
