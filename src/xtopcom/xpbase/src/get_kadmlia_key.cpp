#if 0
#include "xpbase/base/kad_key/get_kadmlia_key.h"

#include <assert.h>

#include "xpbase/base/kad_key/chain_kadmlia_key.h"
#include "xpbase/base/kad_key/platform_kadmlia_key.h"

namespace top {

namespace base {

base::KadmliaKeyPtr GetKadmliaKey(const base::XipParser& xip) {
    std::shared_ptr<base::KadmliaKey> kad_key_ptr;
    if (global_platform_type == kPlatform) {
        kad_key_ptr = std::make_shared<base::PlatformKadmliaKey>(xip);
    } else if (global_platform_type == kChain) {
        kad_key_ptr = std::make_shared<base::ChainKadmliaKey>(xip);
    } else {
        assert(false);
    }
    return kad_key_ptr;
}

base::KadmliaKeyPtr GetKadmliaKey(const base::XipParser& xip, const std::string& node_id) {
    std::shared_ptr<base::KadmliaKey> kad_key_ptr;
    if (global_platform_type == kPlatform) {
        kad_key_ptr = std::make_shared<base::PlatformKadmliaKey>(xip, node_id);
    } else if (global_platform_type == kChain) {
        kad_key_ptr = std::make_shared<base::ChainKadmliaKey>(xip, node_id);
    } else {
        assert(false);
    }
    return kad_key_ptr;
}

base::KadmliaKeyPtr GetKadmliaKey(const std::string& node_id, bool hash_tag) {
    std::shared_ptr<base::KadmliaKey> kad_key_ptr;
    if (global_platform_type == kPlatform) {
        kad_key_ptr = std::make_shared<base::PlatformKadmliaKey>(node_id, hash_tag);
    } else if (global_platform_type == kChain) {
        kad_key_ptr = std::make_shared<base::ChainKadmliaKey>(node_id, hash_tag);
    } else {
        assert(false);
    }
    return kad_key_ptr;
}

base::KadmliaKeyPtr GetKadmliaKey(uint64_t service_type) {
    std::shared_ptr<base::KadmliaKey> kad_key_ptr;
    if (global_platform_type == kPlatform) {
        kad_key_ptr = std::make_shared<base::PlatformKadmliaKey>(service_type);
    } else if (global_platform_type == kChain) {
        kad_key_ptr = std::make_shared<base::ChainKadmliaKey>(service_type);
    } else {
        assert(false);
    }
    return kad_key_ptr;
}

base::KadmliaKeyPtr GetKadmliaKey(const std::string& str_key) {
        std::shared_ptr<base::KadmliaKey> kad_key_ptr;
    if (global_platform_type == kPlatform) {
        kad_key_ptr = std::make_shared<base::PlatformKadmliaKey>(str_key);
    } else if (global_platform_type == kChain) {
        kad_key_ptr = std::make_shared<base::ChainKadmliaKey>(str_key);
    } else {
        assert(false);
    }
    return kad_key_ptr;
}

base::KadmliaKeyPtr GetKadmliaKey() {
    std::shared_ptr<base::KadmliaKey> kad_key_ptr;
    if (global_platform_type == kPlatform) {
        kad_key_ptr = std::make_shared<base::PlatformKadmliaKey>();
    } else if (global_platform_type == kChain) {
        kad_key_ptr = std::make_shared<base::ChainKadmliaKey>();
    } else {
        assert(false);
    }
    return kad_key_ptr;
}

} // namespace base

} // namespace top
#endif