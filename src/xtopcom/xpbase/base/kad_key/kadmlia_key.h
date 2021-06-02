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
    uint64_t GetServiceType();

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

uint64_t CreateServiceType(const common::xip2_t &xip);

base::KadmliaKeyPtr GetRootKadmliaKey(std::string const &node_id);
base::KadmliaKeyPtr GetKadmliaKey(common::xip2_t const &xip);
base::KadmliaKeyPtr GetKadmliaKey(std::string const &node_id);

} // namespace base

} // namespace top
