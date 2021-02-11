//
//  kadmlia_key.h
//
//  Created by Charlie Xie on 04/01/2019.
//  Copyright (c) 2017-2019 Telos Foundation & contributors
//

#pragma once

#include "xpbase/base/top_utils.h"
#include "xpbase/base/xip_generator.h"
#include "xpbase/base/xip_parser.h"
#include "xcommon/xnode_type.h"

namespace top {

namespace base {

union UnionServiceTypeXipType {
    struct {
        uint32_t network_id : 24;
        uint8_t zone_id : 8;
        uint8_t cluster_id : 8;
        uint8_t group_id : 8;
        uint8_t node_id : 8;
        uint8_t xip_type : 2;
        uint8_t reserve : 6;
    } type_detail;
    uint64_t service_type;
};

enum XipProcessType {
    kXipProcessNodeId = 1,
};

uint64_t CreateServiceType(uint32_t network_id);
uint64_t CreateServiceType(uint32_t network_id, uint8_t zone_id);
uint64_t CreateServiceType(uint32_t network_id, uint8_t zone_id, uint8_t xip_type);
uint64_t CreateServiceType(
        uint32_t network_id,
        uint8_t zone_id,
        uint8_t xip_type,
        uint8_t reserve);
uint64_t CreateServiceType(
        uint32_t network_id,
        uint8_t zone_id,
        uint8_t cluster_id,
        uint8_t group_id,
        uint8_t node_id,
        uint8_t xip_type,
        uint8_t reserve);
uint64_t CreateServiceType(const base::XipParser& xip);
uint64_t CreateServiceType(const base::XipParser& xip, uint8_t reserve);
UnionServiceTypeXipType GetServiceStruct(uint64_t service_type);
base::XipParser GetXipFromServiceType(uint64_t service_type);
base::XipParser GetXipFromServiceType(UnionServiceTypeXipType service_type);
void PrintXip(uint64_t service_type);
void PrintXip(const base::XipParser& xip);
void PrintXip(const std::string& xip);
common::xnode_type_t GetNetworkType(uint32_t network_id);

class KadmliaKey {
public:
    virtual const std::string& Get() = 0;
    virtual std::string GetPrivateKey() = 0;
    virtual std::string GetPublicKey() = 0;
    virtual uint64_t GetServiceType() = 0;
    virtual uint64_t GetServiceType(uint8_t network_type) = 0;
    virtual const std::string& GetNodeIdHash() = 0;

    void RandomResetXip() {
        xip_ = XipParser(RandomUint64(), RandomUint64());
    }

    inline void SetXip(const base::XipParser& xip) {
        xip_ = xip;
    }

    inline base::XipParser Xip() const {
        return xip_;
    }

    inline uint32_t xnetwork_id() const {
        return xip_.xnetwork_id();
    }

    inline uint8_t xnetwork_version() const {
        return xip_.xnetwork_version();
    }

    inline uint32_t xinterface_id() const {
        return xip_.xinterface_id();
    }

    inline uint8_t zone_id() const {
        return xip_.zone_id();
    }

    inline uint8_t network_type() const {
        return xip_.network_type();
    }

    inline uint8_t xaddress_domain_xip() const {
        return xip_.xaddress_domain_xip();
    }

    inline uint8_t xip_type() const {
        return xip_.xip_type();
    }

    inline uint8_t cluster_id() const {
        return xip_.cluster_id();
    }

    inline uint8_t group_id() const {
        return xip_.group_id();
    }

    inline uint8_t node_id() const {
        return xip_.node_id();
    }

    inline uint8_t process_id() const {
        return xip_.process_id();
    }

    inline uint8_t router_id() const {
        return xip_.router_id();
    }

    inline uint8_t switch_id() const {
        return xip_.switch_id();
    }

    inline uint8_t local_id() const {
        return xip_.local_id();
    }

    inline uint32_t server_id() const {
        return xip_.server_id();
    }

    inline void set_xnetwork_id(uint32_t xnetwork_id) {
        xip_.set_xnetwork_id(xnetwork_id);
    }

    inline void set_xnetwork_version(uint8_t xnetwork_version) {
        xip_.set_xnetwork_version(xnetwork_version);
    }

    inline void set_xinterface_id(uint32_t xinterface_id) {
        xip_.set_xinterface_id(xinterface_id);
    }

    inline void set_zone_id(uint8_t zone_id) {
        xip_.set_zone_id(zone_id);
    }

    inline void set_network_type(uint8_t xnetwork_type) {
        xip_.set_network_type(xnetwork_type);
    }

    inline void set_xaddress_domain_xip(char enum_xaddress_domain_xip) {
        xip_.set_xaddress_domain_xip(enum_xaddress_domain_xip);
    }

    inline void set_xip_type(uint8_t enum_xip_type) {
        xip_.set_xip_type(enum_xip_type);
    }

    inline void set_cluster_id(uint8_t cluster_id) {
        xip_.set_cluster_id(cluster_id);
    }

    inline void set_group_id(uint8_t group_id) {
        xip_.set_group_id(group_id);
    }

    inline void set_node_id(uint8_t node_id) {
        xip_.set_node_id(node_id);
    }

    inline void set_process_id(uint8_t process_id) {
        xip_.set_process_id(process_id);
    }

    inline void set_router_id(uint8_t router_id) {
        xip_.set_router_id(router_id);
    }

    inline void set_switch_id(uint8_t switch_id) {
        xip_.set_switch_id(switch_id);
    }

    inline void set_local_id(uint8_t local_id) {
        xip_.set_local_id(local_id);
    }

    inline void set_server_id(uint32_t server_id) {
        xip_.set_server_id(server_id);
    }

protected:
    KadmliaKey() {}

    explicit KadmliaKey(const base::XipParser& xip) : xip_(xip) {}

    virtual ~KadmliaKey() {}

    base::XipParser xip_;

    DISALLOW_COPY_AND_ASSIGN(KadmliaKey);
};

typedef std::shared_ptr<KadmliaKey> KadmliaKeyPtr;

}  // namespase base

}  // namespace top
