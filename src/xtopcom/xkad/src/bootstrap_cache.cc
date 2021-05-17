// Copyright (c) 2017-2019 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xkad/routing_table/bootstrap_cache.h"

#include <assert.h>
#include <stdint.h>
#include <mutex>

namespace top {
namespace kadmlia {

BootstrapCache::BootstrapCache(const uint64_t& service_type) 
        : service_type_(service_type) {}

BootstrapCache::~BootstrapCache() {}

bool BootstrapCache::GetCache(VecBootstrapEndpoint& vec_bootstrap_endpoint) {
    if (!get_cache_callback_) {
        return false;
    }
    return get_cache_callback_(service_type_, vec_bootstrap_endpoint);
}

bool BootstrapCache::SetCache(const VecBootstrapEndpoint& vec_bootstrap_endpoint) {
    if (!set_cache_callback_) {
        return false;
    }
    return set_cache_callback_(service_type_, vec_bootstrap_endpoint);
}

void BootstrapCache::RegisterBootstrapCacheCallback(
        on_bootstrap_cache_get_callback_t get_cache_callback,
        on_bootstrap_cache_set_callback_t set_cache_callback) {
    assert(!get_cache_callback_);
    assert(!set_cache_callback_);
    std::unique_lock<std::mutex> lock(bootstrap_cache_mutex_);
    get_cache_callback_ = get_cache_callback;
    set_cache_callback_ = set_cache_callback;
}

void BootstrapCache::UnRegisterBootstrapCacheCallback() {
    std::unique_lock<std::mutex> lock(bootstrap_cache_mutex_);
    get_cache_callback_ = nullptr;
    set_cache_callback_ = nullptr;
}

}  // namespace kadmlia
}  // namespace top
