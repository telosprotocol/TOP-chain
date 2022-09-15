// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xstate_sync/xstate_downloader.h"

#include "xstate_sync/xerror.h"
#include "xstate_sync/xstate_sync.h"

namespace top {
namespace state_sync {

#define TIMEOUT_MSEC 1000U

xtop_state_downloader::xtop_state_downloader(base::xvdbstore_t * db) : m_db(db) {
}

void xtop_state_downloader::sync_state(const std::string & table, const xhash256_t & root, std::shared_ptr<vnetwork::xvnetwork_driver_face_t> network, std::error_code & ec) {
    if (m_runnning) {
        ec = error::xerrc_t::downloader_is_running;
        return;
    }
    m_runnning = true;
    auto syncer = xstate_sync_t::new_state_sync(table, root, network, std::make_shared<std::list<state_req>>(m_track_req), m_db);
    run_state_sync(syncer, ec);
    clear();
}

void xtop_state_downloader::clear() {
    m_runnning = false;
    m_req_id = 0;
    m_track_req.clear();
}

void xtop_state_downloader::run_state_sync(std::shared_ptr<xstate_sync_t> s, std::error_code & ec) {
    std::map<uint32_t, state_req, std::less<uint32_t>> active;
    std::thread run_th(&xtop_state_sync::run2, s);

    while (!s->is_done()) {
        if (!m_track_req.empty()) {
            active[m_req_id++] = m_track_req.front();
            m_track_req.pop_front();
        } else if (!m_state_packs.empty()) {
            // recv packs
            auto res = m_state_packs.front();
            xdbg("xtop_state_downloader::run_state_sync get state pack, id: %u", res.id);
            if (!active.count(res.id)) {
                xwarn("xtop_state_downloader::run_state_sync unrequested id: %u", res.id);
                continue;
            }
            auto req = active[res.id];
            req.response = res.states;
            req.delivered = base::xtime_utl::gettimeofday();
            s->push_deliver_req(req);
            active.erase(res.id);
            m_state_packs.pop_front();
        } else if (!active.empty()) {
            // timeout check
            uint64_t time = base::xtime_utl::time_now_ms();
            for (auto it = active.begin(); it != active.end();) {
                if (it->second.start + TIMEOUT_MSEC > time) {
                    break;
                }
                xwarn("xtop_state_downloader::run_state_sync req: %u timeout %lu, %lu", it->first, it->second.start, time);
                it->second.delivered = base::xtime_utl::gettimeofday();
                s->push_deliver_req(it->second);
                active.erase(it++);
            }
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }
    }

    s->cancel();
    run_th.join();
    xinfo("xtop_state_downloader::run_state_sync sync thread finish: %s", s->root().as_hex_str().c_str());
    return;
}

}  // namespace state_sync
}  // namespace top