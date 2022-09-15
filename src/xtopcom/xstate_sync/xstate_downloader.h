// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xhash.hpp"
#include "xstate_sync/xstate_sync.h"
#include "xvledger/xvdbstore.h"
#include "xvnetwork/xvnetwork_driver_face.h"

namespace top {
namespace state_sync {

class xtop_state_downloader {
public:
    explicit xtop_state_downloader(base::xvdbstore_t * db);
    ~xtop_state_downloader() = default;

    void sync_state(const std::string & table, const xhash256_t & root, std::shared_ptr<vnetwork::xvnetwork_driver_face_t> network, std::error_code & ec);
    void run_state_sync(std::shared_ptr<xstate_sync_t> syncer, std::error_code & ec);

    void clear();

private:
    base::xvdbstore_t * m_db{nullptr};
    bool m_runnning{false};

    uint32_t m_req_id{0};
    std::list<state_req> m_track_req;
    std::list<state_res> m_state_packs;
};
using xstate_downloader_t = xtop_state_downloader;

}  // namespace state_sync
}  // namespace top