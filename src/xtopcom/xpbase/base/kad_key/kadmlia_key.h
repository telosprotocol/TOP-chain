//
//  kadmlia_key.h
//
//  Created by Charlie Xie on 04/01/2019.
//  Copyright (c) 2017-2019 Telos Foundation & contributors
//

#pragma once

// #include "xpbase/base/top_utils.h"
// #include "xpbase/base/xip_generator.h"
// #include "xpbase/base/xip_parser.h"
#include "xbase/xbase.h"
#include "xcommon/xip.h"
#include "xcommon/xnode_type.h"

#include <string>
namespace top {

namespace base {

class ServiceType {
public:
    ServiceType() {}
    explicit ServiceType(uint64_t type) : m_type{type} {}

#define IS_BROADCAST_HEIGHT(service_type_value)                                \
    ((service_type_value & 0x1FFFFF) == 0x1FFFFF)
#define BROADCAST_HEIGHT(service_type_value) ((service_type_value | 0x1FFFFF))
    bool operator==(ServiceType const &other) const {
        if (IS_BROADCAST_HEIGHT(other.value()) || IS_BROADCAST_HEIGHT(m_type)) {
            return BROADCAST_HEIGHT(other.value()) == BROADCAST_HEIGHT(m_type);
        } else {
            return other.value() == m_type;
        }
    }
    bool operator!=(ServiceType const &other) const {
        return !(*this == other);
    }
    bool operator<(ServiceType const &other) const {
        return m_type < other.value();
    }

    uint64_t value() const { return m_type; }

private:
    uint64_t m_type{0};
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
    size_t operator()(const top::base::ServiceType &k) const { return static_cast<size_t>(k.value()); }
};
} // namespace std