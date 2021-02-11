
#include <string.h>

#include <string>
#include <iostream>
#include <memory>

#include <gtest/gtest.h>
#include "xpbase/base/top_utils.h"
#define private public
#define protected public
#include "xpbase/base/kad_key/kadmlia_key.h"

namespace top {
namespace base {
namespace test {

class TestKadmliaKey : public testing::Test {
public:
    static void SetUpTestCase() {
    }

    static void TearDownTestCase() {
    }

    virtual void SetUp() {

    }

    virtual void TearDown() {
    }
};

TEST_F(TestKadmliaKey, CreateServiceType) {
    uint32_t network_id = 1;
    CreateServiceType(network_id);
}

TEST_F(TestKadmliaKey, CreateServiceType_2) {
    uint32_t network_id = 1;
    uint8_t zone_id = 2;
    CreateServiceType(network_id, zone_id);
}

TEST_F(TestKadmliaKey, CreateServiceType_3) {
    uint32_t network_id = 1;
    uint8_t zone_id = 2;
    uint8_t xip_type = 3;
    CreateServiceType(network_id, zone_id, xip_type);
}

TEST_F(TestKadmliaKey, CreateServiceType_4) {
    uint32_t network_id = 1;
    uint8_t zone_id = 2;
    uint8_t xip_type = 3;
    uint8_t reserve = 4;
    CreateServiceType(network_id, zone_id, xip_type, reserve);
}

TEST_F(TestKadmliaKey, CreateServiceType_5) {
    uint32_t network_id = 1;
    uint8_t zone_id = 2;
    uint8_t cluster_id = 4;
    uint8_t group_id = 5;
    uint8_t node_id = 6;
    uint8_t xip_type = 7;
    uint8_t reserve = 8;
    CreateServiceType(network_id, zone_id,
        cluster_id, group_id, node_id, xip_type,
        reserve);
}

TEST_F(TestKadmliaKey, CreateServiceType_xip) {
    base::XipParser xip;
    xip.set_xnetwork_id(1);
    CreateServiceType(xip);
}

TEST_F(TestKadmliaKey, CreateServiceType_xip_2) {
    base::XipParser xip;
    xip.set_xnetwork_id(1);
    uint8_t reserve = 2;
    CreateServiceType(xip, reserve);
}

TEST_F(TestKadmliaKey, GetServiceStruct) {
    uint64_t service_type = 1;
    GetServiceStruct(service_type);
}

TEST_F(TestKadmliaKey, GetXipFromServiceType) {
    uint64_t service_type = 1;
    GetXipFromServiceType(service_type);
}

TEST_F(TestKadmliaKey, GetXipFromServiceType_2) {
    UnionServiceTypeXipType service_type = {0,};
    GetXipFromServiceType(service_type);
}

TEST_F(TestKadmliaKey, PrintXip) {
    uint64_t service_type = 1;
    PrintXip(service_type);
}

TEST_F(TestKadmliaKey, PrintXip_2) {
    const base::XipParser xip;
    PrintXip(xip);
}

TEST_F(TestKadmliaKey, PrintXip_3) {
    const std::string xip(16u, '\0');
    PrintXip(xip);
}

TEST_F(TestKadmliaKey, GetNetworkType) {
    uint32_t network_id = kChainRecNet;
    GetNetworkType(network_id);
}

}  // namespace test
}  // namespace kadmlia
}  // namespace top
