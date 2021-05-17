#include "xpbase/base/xid/xid_generator.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <limits>

#include "xpbase/base/xid/xid_db_session.h"
#include "xpbase/base/top_log.h"
#include "xpbase/base/rand_util.h"
#include "common/xdfcurve.h"
#include "common/sha2.h"
#include "xpbase/base/xid/xid_def.h"

namespace top {

namespace base {

bool XIDGenerator::CreateXID(
        const XIDType& xid_type) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::string private_key;
    std::string public_key;
    bool ret = CreatePrivateAndPublicKey(private_key, public_key);
    if (!ret) {
        return false;
    }
    xid_.xnetwork_id_ = xid_type.xnetwork_id_;
    xid_.zone_id_ = xid_type.zone_id_;
    xid_.public_key_ = public_key;
    xid_.private_key_ = private_key;
    type_.xnetwork_id_ = xid_type.xnetwork_id_;
    type_.zone_id_ = xid_type.zone_id_;
    return true;
}

bool XIDGenerator::GetXID(
        XID& xid) const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (xid_.IsInvalid()) {
        return false;
    }
    xid = xid_;
    return true;
}

bool XIDGenerator::GetXIDFromDB(
        const XIDType& xid_type, 
        XID& xid) const {
    std::string xid_key = xid_type.ToString();
    XIDSptr xid_ptr;
    if (!XIdDBSession::Select(xid_key, xid_ptr)) {
        return false;
    }
    if (!xid_ptr) {
        return false;
    }
    xid = *xid_ptr;
    return true;
}

void XIDGenerator::Reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    type_.Reset();
    xid_.Reset();
}

bool XIDGenerator::DeleteXID(
        const XIDType& xid_type) {
    std::string xid_key = xid_type.ToString();
    if (!XIdDBSession::Delete(xid_key)) {
        return false;
    }
    return true;
}
bool XIDGenerator::SaveXID() {
    std::lock_guard<std::mutex> lock(mutex_);
    std::string xid_key = type_.ToString();
    if (!XIdDBSession::Insert(xid_key, xid_)) {
        return false;
    }
    return true;
}

bool XIDGenerator::CreatePrivateAndPublicKey(
        std::string& private_key,
        std::string& public_key) {
    // sha256 create private key
    SHA256_CTX* sha256_context = static_cast<SHA256_CTX*>(malloc(sizeof(SHA256_CTX)));
    if (sha256_context == NULL) {
        TOP_ERROR("create sha256 context failed!");
        return false;
    }

    key25519 prikey = { 0 };
    sha256_Init(sha256_context);
    int64_t time_seed = base::GetRandomInt64() % std::numeric_limits<int64_t>::max();
    sha256_Update(sha256_context, (const uint8_t*)&time_seed, sizeof(time_seed));
    sha256_Update(sha256_context, (const uint8_t*)prikey, sizeof(prikey));
    std::string rand_str = RandomString(kNodeIdSize);
    sha256_Update(sha256_context, (const uint8_t*)rand_str.c_str(), rand_str.size());
    uint8_t raw_uint8[32];
    sha256_Final(sha256_context, (uint8_t*)raw_uint8);  // NOLINT
    memcpy(prikey, raw_uint8, sizeof(raw_uint8));
    private_key = std::string((char*)prikey, sizeof(prikey)); // NOLINT

    // create public key with private key
    key25519 pubkey = { 0 };
    keygen25519(prikey, pubkey);
    public_key = std::string((const char*)pubkey, sizeof(key25519));
    free(sha256_context);
    return true;
}

XIDGenerator::XIDGenerator()
    :type_(),
    xid_(),
    mutex_() {
}

XIDGenerator::~XIDGenerator() {}

}  // namespace base

}  // namespace top
