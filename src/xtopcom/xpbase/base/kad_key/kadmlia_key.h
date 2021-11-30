//
//  kadmlia_key.h
//
//  Created by Charlie Xie on 04/01/2019.
//  Copyright (c) 2017-2019 Telos Foundation & contributors
//

#pragma once

#include "xbase/xbase.h"
#include "xcommon/xip.h"
#include "xcommon/xnode_type.h"

#include <string>
namespace top {

namespace base {

enum service_type_ver {
    service_type_height_use_version = 0,
    service_type_height_use_blk_height = 1,
};

static const service_type_ver now_service_type_ver = service_type_ver::service_type_height_use_blk_height;

class ServiceType {
public:
    ServiceType() {}
    explicit ServiceType(uint64_t type);

    bool operator==(ServiceType const &other) const;
    bool operator!=(ServiceType const &other) const;
    bool operator<(ServiceType const &other) const;

    bool IsNewer(ServiceType const &other) const;

    // for edge && archive node routing table. The height is always the same. 
    // The P2P net should have only one round of working node.
    // Thus have no choose but trust the upper layer updating the routing table with election result.
    bool IsBroadcastService() const;

    uint64_t value() const;
    std::string info() const;

    bool is_root_service() const;
    common::xip2_t group_xip2() const;

    service_type_ver ver() const;
    
    common::xnetwork_id_t network_id() const;
    common::xzone_id_t zone_id() const;
    common::xcluster_id_t cluster_id() const;
    common::xgroup_id_t group_id() const;
    uint64_t height() const;

    void set_ver(uint64_t new_ver); 
    void set_height(uint64_t new_height); 
private:
    void update_info();

private:
    uint64_t m_type{0};
    std::string m_info{"uninitialized"};
};

class KadmliaKey {
public:
    explicit KadmliaKey(common::xip2_t const &xip);
    explicit KadmliaKey(std::string const &from_str); // Get()-> from_str
    KadmliaKey() = delete;

    KadmliaKey(KadmliaKey const &) = delete;
    KadmliaKey &operator=(KadmliaKey const &) = delete;
    KadmliaKey(KadmliaKey &&) = delete;
    KadmliaKey &operator=(KadmliaKey &&) = delete;
    virtual ~KadmliaKey() = default;

    std::string Get();
    ServiceType GetServiceType();

public:
    inline void SetXip(const common::xip2_t &xip) { xip_ = xip; }

    inline common::xip2_t Xip() const { return xip_; }

    inline uint32_t xnetwork_id() const { return xip_.network_id().value(); }

    inline uint8_t zone_id() const { return xip_.zone_id().value(); }

    inline uint8_t cluster_id() const { return xip_.cluster_id().value(); }

    inline uint8_t group_id() const { return xip_.group_id().value(); }

    inline uint8_t slot_id() const { return xip_.slot_id().value(); }

    // use xip2 height as version
    // inline uint64_t version() const { return xip_.height(); }

private:
    common::xip2_t xip_;
};

typedef std::shared_ptr<KadmliaKey> KadmliaKeyPtr;

ServiceType CreateServiceType(const common::xip2_t &xip);

base::KadmliaKeyPtr GetRootKadmliaKey(std::string const &node_id);
base::KadmliaKeyPtr GetKadmliaKey(common::xip2_t const &xip);
base::KadmliaKeyPtr GetKadmliaKey(std::string const &node_id);

} // namespace base

} // namespace top

namespace std {
template <> struct hash<top::base::ServiceType> {
    size_t operator()(const top::base::ServiceType &k) const {
        return static_cast<size_t>(k.value());
    }
};
} // namespace std