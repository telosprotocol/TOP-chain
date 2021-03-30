
#include "xpbase/base/xid/xid_def.h"

namespace top {
namespace base {

const std::string LOCAL_XID_KEY = "local_xid";

XID::XID() : xnetwork_id_(-1), zone_id_(-1), public_key_(), private_key_() {}

XID::XID(const uint32_t xnetwork_id, const uint8_t zone_id,
         const std::string public_key, const std::string &private_key)
    : xnetwork_id_(xnetwork_id), zone_id_(zone_id), public_key_(public_key),
      private_key_(private_key) {}

XID::~XID() {}

bool XID::operator==(const XID &other) const {
    return xnetwork_id_ == other.xnetwork_id_ && zone_id_ == other.zone_id_ &&
           public_key_ == other.public_key_ &&
           private_key_ == other.private_key_;
}

std::string XID::ToString() const {
    uint32_t high = 0;
    high = set_zone_id_to_xid(high, zone_id_);
    high = set_network_id_to_xid(high, xnetwork_id_);
    std::string str_high;
    str_high.resize(sizeof(high));
    memcpy((char *)str_high.c_str(), (char *)(&high), sizeof(high));
    return str_high + public_key_;
}

void XID::Reset() {
    xnetwork_id_ = -1;
    zone_id_ = -1;
    public_key_ = "";
    private_key_ = "";
}

bool XID::IsInvalid() const {
    return xnetwork_id_ == static_cast<uint32_t>(-1) &&
           zone_id_ == static_cast<uint8_t>(-1) && public_key_.empty() &&
           private_key_.empty();
}

uint32_t XID::GetXNetworkID() const { return xnetwork_id_; }

uint8_t XID::GetZoneID() const { return zone_id_; }

std::string XID::GetPublicKey() const { return public_key_; }

std::string XID::GetPrivateKey() const { return private_key_; }

XIDType::XIDType() : xnetwork_id_(-1), zone_id_(-1) {}

XIDType::XIDType(const uint32_t xnetwork_id, const uint8_t zone_id)
    : xnetwork_id_(xnetwork_id), zone_id_(zone_id) {}

XIDType::~XIDType() {}

bool XIDType::operator<(const XIDType &other) const {
    if (xnetwork_id_ < other.xnetwork_id_) {
        return true;
    } else if (xnetwork_id_ == other.xnetwork_id_) {
        if (zone_id_ < other.zone_id_) {
            return true;
        }
    }
    return false;
}

std::string XIDType::ToString() const {
    return StringUtil::str_fmt("%s-%d-%d", LOCAL_XID_KEY.c_str(), xnetwork_id_,
                               zone_id_);
}

void XIDType::Reset() {
    xnetwork_id_ = -1;
    zone_id_ = -1;
}

} // namespace base
} // namespace top
