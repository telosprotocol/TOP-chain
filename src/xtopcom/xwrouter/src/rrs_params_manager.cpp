// Copyright (c) 2017-2019 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xwrouter/multi_routing/rrs_params_manager.h"

#include "assert.h"
#include "xwrouter/xwrouter.h"

namespace top {
namespace wrouter {

static const int32_t kUpdateRegisterNodeSizePeriod = 5 * 60 * 1000 * 1000; // 5min

bool RRSParamsMgr::set_callback(std::function<void(uint64_t & node_size, std::error_code & ec)> cb) {
    if (m_callback) {
        assert(false); // only allow to set once.
        return false;
    }
    m_callback = cb;
    update_rrs_params_timer = std::make_shared<base::TimerRepeated>(timer_manager_, "RRSParamsMgr::UpdateRegisterNodeSize");
    update_rrs_params_timer->Start(kUpdateRegisterNodeSizePeriod, kUpdateRegisterNodeSizePeriod, std::bind(&RRSParamsMgr::update_rrs_params_with_node_size, shared_from_this()));
    return true;
}

void RRSParamsMgr::update_rrs_params_with_node_size() {
    uint64_t node_size = 0;
    std::error_code ec;
    if (!m_callback) {
        return;
    }
    m_callback(node_size, ec);
    if (ec) {
        xinfo("update_rrs_params_with_node_size, get_node_size from rec_standby failed %s %s", ec.category().name(), ec.message().c_str());
        return;
    }

#if defined(XBUILD_CI)
    uint32_t default_params_t = 4u;
    uint32_t default_params_k = 3u;
#elif defined(XBUILD_DEV)
    uint32_t default_params_t = 3u;
    uint32_t default_params_k = 3u;
#elif defined(XBUILD_GALILEO)
    uint32_t default_params_t = 5u;
    uint32_t default_params_k = 3u;
#else  // mainnet
    uint32_t default_params_t = 6u;
    uint32_t default_params_k = 4u;
#endif

#ifndef __INCLUDE_RRS_BEST_PARAMS__
#    define __INCLUDE_RRS_BEST_PARAMS__
#    define RRS(min, max, t, k)                                                                                                                                                    \
        if (node_size >= min && node_size < max + 10) {                                                                                                                            \
            default_params_t = t;                                                                                                                                                  \
            default_params_k = k;                                                                                                                                                  \
            break;                                                                                                                                                                 \
        }

    do {
#    include "xwrouter/multi_routing/rrs_params_data.inc"
    } while (0);

#    undef RRS
#endif
    xinfo("update_rrs_params_with_node_size, node_size: %llu, t: %u, k: %u", node_size, default_params_t, default_params_k);
    wrouter::Wrouter::Instance()->update_rrs_params(default_params_t, default_params_k);
}

}  // namespace wrouter
}  // namespace top