// Copyright (c) 2017-2019 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xpbase/base/top_timer.h"

#include <memory>
#include <mutex>
#include <unordered_map>

namespace top {
namespace wrouter {

class RRSParamsMgr final : public std::enable_shared_from_this<RRSParamsMgr> {
public:
    RRSParamsMgr() = default;
    RRSParamsMgr(RRSParamsMgr const &) = delete;
    RRSParamsMgr(RRSParamsMgr &&) = delete;
    RRSParamsMgr & operator=(RRSParamsMgr const &) = delete;
    RRSParamsMgr & operator=(RRSParamsMgr &&) = delete;
    ~RRSParamsMgr() = default;

public:
    bool set_callback(std::function<void(uint64_t & node_size, std::error_code & ec)> cb);

private:
    void update_rrs_params_with_node_size();

private:
    std::function<void(uint64_t & node_size, std::error_code & ec)> m_callback;
    base::TimerManager * timer_manager_{base::TimerManager::Instance()};
    std::shared_ptr<base::TimerRepeated> update_rrs_params_timer;
};

}  // namespace wrouter
}  // namespace top
