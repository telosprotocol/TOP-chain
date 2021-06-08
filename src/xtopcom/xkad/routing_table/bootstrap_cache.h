#if 0
// Copyright (c) 2017-2019 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <stdint.h>
#include <vector>
#include <string>
#include <functional>
#include <map>
#include <mutex>

#include "xpbase/base/top_utils.h"
#include "xpbase/base/top_config.h"


using BootstrapEndpoint = std::pair<std::string, uint16_t>;
using VecBootstrapEndpoint = std::vector<BootstrapEndpoint>;
using on_bootstrap_cache_get_callback_t = std::function<bool(
        const uint64_t& service_type,
        VecBootstrapEndpoint& vec_bootstrap_endpoint)>;
using on_bootstrap_cache_set_callback_t = std::function<bool(
        const uint64_t& service_type,
        const VecBootstrapEndpoint& vec_bootstrap_endpoint)>;

namespace top {
namespace kadmlia {

class BootstrapCache {
    using Lock = std::unique_lock<std::mutex>;
    const uint32_t INVALID_SERVICE_TYPE = (uint32_t)-1;

public:
    BootstrapCache(const uint64_t& service_type);
    ~BootstrapCache();
    bool GetCache(VecBootstrapEndpoint& vec_bootstrap_endpoint);
    bool SetCache(const VecBootstrapEndpoint& vec_bootstrap_endpoint);
    void RegisterBootstrapCacheCallback(
        on_bootstrap_cache_get_callback_t get_cache_callback,
        on_bootstrap_cache_set_callback_t set_cache_callback);
    void UnRegisterBootstrapCacheCallback();


private:
    uint64_t service_type_{INVALID_SERVICE_TYPE};
    std::mutex bootstrap_cache_mutex_;
    on_bootstrap_cache_get_callback_t get_cache_callback_;
    on_bootstrap_cache_set_callback_t set_cache_callback_;

private:
    DISALLOW_COPY_AND_ASSIGN(BootstrapCache);
};

using BootstrapCachePtr = std::shared_ptr<BootstrapCache>;

}  // namespace kadmlia
}  // namespace top
#endif