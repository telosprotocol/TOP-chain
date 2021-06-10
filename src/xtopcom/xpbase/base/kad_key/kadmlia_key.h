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

class ServiceType {
public:
    ServiceType() {}
    explicit ServiceType(uint64_t type);

    bool operator==(ServiceType const &other) const;
    bool operator!=(ServiceType const &other) const;
    bool operator<(ServiceType const &other) const;

    bool IsNewer(ServiceType const &other, int _value = 1) const;

    // for edge && archive node routing table. The height is always the same. 
    // The P2P net should have only one round of working node.
    // Thus have no choose but trust the upper layer updating the routing table with election result.
    bool IsBroadcastService() const;

    uint64_t value() const;
    std::string info() const;

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