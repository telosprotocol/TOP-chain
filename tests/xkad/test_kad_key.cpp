#include "xbase/xbase.h"
#include "xcommon/xip.h"
#include "xpbase/base/kad_key/kadmlia_key.h"
#include "xpbase/base/top_utils.h"

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

    // EXPECT_TRUE(s2.IsNewer(s0, 2));
    // EXPECT_TRUE(s2.IsNewer(s1, 1));
    // EXPECT_FALSE(s2.IsNewer(s0, 3));
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

    // EXPECT_FALSE(s_o_1.IsNewer(s0, 2));
    // EXPECT_FALSE(s_o_2.IsNewer(s0, 2));
    // EXPECT_FALSE(s_o_3.IsNewer(s0, 2));
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
    common::xip2_t xip2_1{network_id, zid, cluster_id, group_id, slot_id, size, common::xelection_round_t::max().value()};

    auto s0 = base::GetKadmliaKey(xip2_0)->GetServiceType();
    auto s1 = base::GetKadmliaKey(xip2_1)->GetServiceType();

    EXPECT_FALSE(s0.IsBroadcastService());
    EXPECT_TRUE(s1.IsBroadcastService());
    // EXPECT_FALSE(s1.IsNewer(s0));
}

TEST(xkad_test, test_service_type_group_xip) {
    for (std::size_t _ = 0; _ < 10000; _++) {
        common::xnetwork_id_t network_id{RandomUint8() % common::xmax_network_id_value};
        common::xzone_id_t zid{RandomUint8() % common::xmax_zone_id_value};
        common::xcluster_id_t cluster_id{RandomUint8() % common::xmax_cluster_id_value};
        common::xgroup_id_t group_id{RandomUint8() % common::xmax_group_id_value};
        common::xslot_id_t slot_id{RandomUint16() % common::xmax_slot_id_value};
        uint16_t size = RandomUint16();
        uint64_t height = RandomUint64();

        common::xip2_t xip2_0{network_id, zid, cluster_id, group_id, slot_id, size, height};
        // std::cout << "xip2_0.to_string() : " << xip2_0.to_string() << std::endl;
        // std::cout << "network_id.to_string() : " << network_id.to_string() << std::endl;
        // std::cout << "zid.to_string() : " << zid.to_string() << std::endl;
        // std::cout << "cluster_id.to_string() : " << cluster_id.to_string() << std::endl;
        // std::cout << "group_id.to_string() : " << group_id.to_string() << std::endl;
        // std::cout << "height.to_string() : " << height << std::endl;

        auto s0 = base::GetKadmliaKey(xip2_0)->GetServiceType();

        EXPECT_TRUE(xip2_0.group_xip2() == s0.group_xip2());

        // std::cout << s0.info() << std::endl;
        EXPECT_FALSE(s0.is_root_service());
    }
}

// TEST(xkad_test, test_service_type_ver_changed) {
    // common::xnetwork_id_t network_id{RandomUint8() % common::xmax_network_id_value};
    // common::xzone_id_t zid{RandomUint8() % common::xmax_zone_id_value};
    // common::xcluster_id_t cluster_id{RandomUint8() % common::xmax_cluster_id_value};
    // common::xgroup_id_t group_id{RandomUint8() % common::xmax_group_id_value};
    // common::xslot_id_t slot_id{RandomUint16() % common::xmax_slot_id_value};
    // uint16_t size = RandomUint16();
    // uint64_t height = RandomUint64();

    // common::xip2_t xip2_0{network_id, zid, cluster_id, group_id, slot_id, size, height};
    // std::cout << "xip2_0.to_string() : " << xip2_0.to_string() << std::endl;
    // std::cout << "network_id.to_string() : " << network_id.to_string() << std::endl;
    // std::cout << "zid.to_string() : " << zid.to_string() << std::endl;
    // std::cout << "cluster_id.to_string() : " << cluster_id.to_string() << std::endl;
    // std::cout << "group_id.to_string() : " << group_id.to_string() << std::endl;
    // std::cout << "height.to_string() : " << height << std::endl;


    // auto s0 = base::GetKadmliaKey(xip2_0)->GetServiceType();
    // std::cout << s0.info() << std::endl;

    // EXPECT_EQ(s0.ver(), base::now_service_type_ver);

    // s0.set_ver(base::service_type_height_use_version);
    // EXPECT_EQ(s0.ver(), base::service_type_ver::service_type_height_use_version);
    // std::cout << s0.info() << std::endl;
    // s0.set_ver(base::service_type_height_use_version);
    // EXPECT_EQ(s0.ver(), base::service_type_ver::service_type_height_use_version);
    // std::cout << s0.info() << std::endl;

    // s0.set_ver(base::service_type_height_use_blk_height);
    // EXPECT_EQ(s0.ver(), base::service_type_ver::service_type_height_use_blk_height);
    // std::cout << s0.info() << std::endl;
    // s0.set_ver(base::service_type_height_use_blk_height);
    // EXPECT_EQ(s0.ver(), base::service_type_ver::service_type_height_use_blk_height);
    // std::cout << s0.info() << std::endl;

    
    // s0.set_height(10);
    // EXPECT_EQ(s0.ver(), base::service_type_ver::service_type_height_use_blk_height);
    // std::cout << s0.info() << std::endl;

// }

}  // namespace test
}  // namespace top
