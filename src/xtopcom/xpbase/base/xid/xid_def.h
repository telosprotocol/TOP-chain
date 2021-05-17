#pragma once

#include <stdint.h>
#include <string.h>
#include <algorithm>
#include <memory>
#include <string>

#include "xpbase/base/top_log.h"
#include "xpbase/base/top_string_util.h"
#include "xpbase/base/top_utils.h"

namespace top {

namespace base {

#define reset_zone_id_to_xid(xid_high_32bit)                                   \
    ((uint32_t)(xid_high_32bit) & (((uint32_t)0xFFFFFF00)))
#define set_zone_id_to_xid(xid_high_32bit, id)                                 \
    (reset_zone_id_to_xid((uint32_t)(xid_high_32bit)) | (((uint8_t)(id)&0xFF)))
#define reset_network_id_to_xid(xid_high_32bit)                                \
    ((uint32_t)(xid_high_32bit) & (((uint32_t)(0x000000FF))))
#define set_network_id_to_xid(xid_high_32bit, id)                              \
    (reset_network_id_to_xid((uint32_t)(xid_high_32bit)) |                     \
     (((uint32_t)((id)&0xFFFFFF)) << 8))

extern const std::string LOCAL_XID_KEY;

struct XID {
    XID();
    XID(const uint32_t xnetwork_id, const uint8_t zone_id,
        const std::string public_key, const std::string &private_key);
    ~XID();
    bool operator==(const XID &other) const;
    std::string ToString() const;
    void Reset();
    bool IsInvalid() const;
    uint32_t GetXNetworkID() const;
    uint8_t GetZoneID() const;
    std::string GetPublicKey() const;
    std::string GetPrivateKey() const;
    uint32_t xnetwork_id_;
    uint8_t zone_id_;
    std::string public_key_;
    std::string private_key_;
};

typedef std::shared_ptr<XID> XIDSptr;

struct XIDType {
    XIDType();
    XIDType(const uint32_t xnetwork_id, const uint8_t zone_id);
    ~XIDType();
    bool operator<(const XIDType &other) const;
    std::string ToString() const;
    void Reset();
    uint32_t xnetwork_id_;
    uint8_t zone_id_;
};

} // namespace base

} // namespace top