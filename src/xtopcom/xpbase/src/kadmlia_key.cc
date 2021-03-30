//
//  consensus_base.cc
//
//  Created by Charlie Xie on 04/01/2019.
//  Copyright (c) 2017-2019 Telos Foundation & contributors
//

#include "xpbase/base/kad_key/kadmlia_key.h"

#include <assert.h>
#include <iostream>

#include "xpbase/base/check_cast.h"
#include "xpbase/base/top_string_util.h"

namespace top {

namespace base {

uint64_t CreateServiceType(uint32_t network_id) {
    if (network_id == kRoot) {
        return static_cast<uint64_t>(kRoot);
    }
    base::UnionServiceTypeXipType tmp_type{
        {network_id, 0xFF, 0xFF, 0xFF, 0xFF, 0x03, 0x1F } };
    return tmp_type.service_type;
}

uint64_t CreateServiceType(uint32_t network_id, uint8_t zone_id) {
    if (network_id == kRoot) {
        return static_cast<uint64_t>(kRoot);
    }
    base::UnionServiceTypeXipType tmp_type{
        { network_id, zone_id, 0xFF, 0xFF, 0xFF, 0x03, 0x1F } };
    return tmp_type.service_type;
}

uint64_t CreateServiceType(uint32_t network_id, uint8_t zone_id, uint8_t xip_type) {
    if (network_id == kRoot) {
        return static_cast<uint64_t>(kRoot);
    }
    base::UnionServiceTypeXipType tmp_type{
        { network_id, zone_id,
        0xFF, 0xFF, 0xFF, xip_type, 0x1F } };
    return tmp_type.service_type;
}

uint64_t CreateServiceType(
        uint32_t network_id,
        uint8_t zone_id,
        uint8_t xip_type,
        uint8_t reserve) {
    if (network_id == kRoot) {
        return static_cast<uint64_t>(kRoot);
    }
    base::UnionServiceTypeXipType tmp_type{
        { network_id, zone_id,
        0xFF, 0xFF, 0xFF, xip_type, reserve } };
    return tmp_type.service_type;
}

uint64_t CreateServiceType(
        uint32_t network_id,
        uint8_t zone_id,
        uint8_t cluster_id,
        uint8_t group_id,
        uint8_t node_id,
        uint8_t xip_type,
        uint8_t reserve) {
    if (network_id == kRoot) {
        return static_cast<uint64_t>(kRoot);
    }
    base::UnionServiceTypeXipType tmp_type{
        { network_id, zone_id, cluster_id,
        group_id, node_id, xip_type, reserve
    } };
    return tmp_type.service_type;
}

uint64_t CreateServiceType(const base::XipParser& xip) {
    return CreateServiceType(
            xip.xnetwork_id(),
            xip.zone_id(),
            xip.cluster_id(),
            xip.group_id(),
            xip.node_id(),
            xip.xip_type(),
            0x1F);
}

uint64_t CreateServiceType(const base::XipParser& xip, uint8_t reserve) {
    return CreateServiceType(
            xip.xnetwork_id(),
            xip.zone_id(),
            xip.cluster_id(),
            xip.group_id(),
            xip.node_id(),
            xip.xip_type(),
            reserve);
}

UnionServiceTypeXipType GetServiceStruct(uint64_t service_type) {
    base::UnionServiceTypeXipType tmp_type;
    tmp_type.service_type = service_type;
    return tmp_type;
}

base::XipParser GetXipFromServiceType(uint64_t service_type) {
    base::UnionServiceTypeXipType tmp_type;
    tmp_type.service_type = service_type;
    return base::GetXipFromServiceType(tmp_type);
}

base::XipParser GetXipFromServiceType(UnionServiceTypeXipType service_type) {
    base::XipParser xip;
    xip.set_xnetwork_id(service_type.type_detail.network_id);
    xip.set_zone_id(service_type.type_detail.zone_id);
    xip.set_cluster_id(service_type.type_detail.cluster_id);
    xip.set_group_id(service_type.type_detail.group_id);
    xip.set_node_id(service_type.type_detail.node_id);
    xip.set_xip_type(service_type.type_detail.xip_type);
    return xip;
}

void PrintXip(uint64_t service_type) {
    base::XipParser xip = base::GetXipFromServiceType(service_type);
    base::PrintXip(xip);
}

void PrintXip(const base::XipParser& xip) {
    std::cout << base::StringUtil::str_fmt(
        "[network_id: %d][zone_id: %d]"
        "[cluster_id: %d][group_id: %d][node_id: %d][xip_type: %d]",
        (uint32_t)xip.xnetwork_id(),
        (uint32_t)xip.zone_id(),
        (uint32_t)xip.cluster_id(),
        (uint32_t)xip.group_id(),
        (uint32_t)xip.node_id(),
        (uint32_t)xip.xip_type()) << std::endl;
}

void PrintXip(const std::string& str_xip) {
    base::XipParser xip(str_xip);
    base::PrintXip(xip);
}

common::xnode_type_t GetNetworkType(uint32_t network_id) {
    switch (network_id) {
    case kChainRecNet:
        return common::xnode_type_t::consensus_auditor;
    case kChainZecNet:
        return common::xnode_type_t::consensus_validator;
    case kChainEdgeNet:
        return common::xnode_type_t::edge;
    default:
        assert(false);
    }
    return common::xnode_type_t::consensus_validator;
}

}  // namespase base

}  // namespace top
