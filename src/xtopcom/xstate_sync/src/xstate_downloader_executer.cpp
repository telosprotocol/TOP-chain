// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xstate_sync/xstate_downloader_executer.h"

#include "xmbus/xevent_state_sync.h"
#include "xstate_sync/xerror.h"
#include "xstate_sync/xstate_sync.h"
#include "xstate_sync/xunit_state_sync.h"

namespace top {
namespace state_sync {

#define TIMEOUT_MSEC 5000U

xtop_download_executer::xtop_download_executer(observer_ptr<base::xiothread_t> thread) : m_syncer_thread{thread} {
}

void xtop_download_executer::run_state_sync(std::shared_ptr<xstate_sync_face_t> syncer, std::function<void(sync_result)> callback) {
#if !defined(NDEBUG)
    if (executor_thread_id_ == std::thread::id{}) {
        executor_thread_id_ = std::this_thread::get_id();
    }
    assert(executor_thread_id_ == std::this_thread::get_id());
#endif

    xinfo("xtop_download_executer::run_state_sync sync thread start, %s", syncer->symbol().c_str());

    auto f = [syncer](base::xcall_t &, const int32_t, const uint64_t) -> bool {
        syncer->run();
        return true;
    };
    base::xcall_t call(f);
    m_syncer_thread->send_call(call);

    std::error_code loop_ec;
    loop(syncer, loop_ec);

    auto done = syncer->is_done();
    auto res = syncer->result();
    syncer->cancel();
    if (done) {
        if (res.ec) {
            xwarn("xtop_download_executer::run_state_sync sync thread finish but error, %s, error: %s, %s",
                  syncer->symbol().c_str(),
                  res.ec.category().name(),
                  res.ec.message().c_str());
        } else {
            xinfo("xtop_download_executer::run_state_sync sync thread finish, %s", syncer->symbol().c_str());
        }
    } else {
        if (!res.ec && !loop_ec) {
            xwarn("xtop_download_executer::run_state_sync not finish but no error code");
            xassert(false);
        }
        if (loop_ec) {
            xwarn("xtop_download_executer::run_state_sync origin error: %s %s, replace by loop error: %s %s",
                  res.ec.category().name(),
                  res.ec.message().c_str(),
                  loop_ec.category().name(),
                  loop_ec.message().c_str());
            res.ec = loop_ec;
        }
        xwarn("xtop_download_executer::run_state_sync thread overtime, maybe error, account: %s, height: %lu, root: %s, error: %s %s",
              res.account.c_str(),
              res.height,
              res.root_hash.as_hex_str().c_str(),
              res.ec.category().name(),
              res.ec.message().c_str());
    }
    if (callback) {
        callback(res);
    }
    return;
}

void xtop_download_executer::loop(std::shared_ptr<xstate_sync_face_t> syncer, std::error_code & ec) {
#if !defined(NDEBUG)
    assert(executor_thread_id_ == std::this_thread::get_id());
#endif

    std::map<uint32_t, state_req> active;
    int cnt{0};

    while (!syncer->is_done()) {
        if (!m_single_states.empty()) {
            auto detail = m_single_states.front();
            xdbg("xtop_download_executer::loop push single state, account: %s, height: %lu, hash: %s, value: %s",
                 detail.address.c_str(),
                 detail.height,
                 to_hex(detail.hash).c_str(),
                 to_hex(detail.value).c_str());
            syncer->push_deliver_state(detail);
            pop_single_state();
        } else if (!m_track_req.empty()) {
            auto req = m_track_req.front();
            active[req.id] = req;
            xdbg("xtop_download_executer::loop add active, id: %u", req.id);
            pop_track_req();
        } else if (!m_state_packs.empty()) {
            // recv packs
            auto res = m_state_packs.front();
            xdbg("xtop_download_executer::loop get state pack, id: %u", res.id);
            if (!active.count(res.id)) {
                xwarn("xtop_download_executer::loop unrequested id: %u", res.id);
                pop_state_pack();
                continue;
            }
            auto req = active[res.id];
            req.nodes_response = res.nodes;
            req.units_response = res.units;
            req.delivered = base::xtime_utl::gettimeofday();
            syncer->push_deliver_req(req);
            active.erase(res.id);
            pop_state_pack();
        } else if (!active.empty()) {
            // timeout check
            uint64_t time = base::xtime_utl::time_now_ms();
            for (auto it = active.begin(); it != active.end();) {
                if (it->second.start + TIMEOUT_MSEC > time) {
                    break;
                }
                xwarn("xtop_download_executer::loop req: %u timeout %lu, %lu", it->first, it->second.start, time);
                it->second.delivered = base::xtime_utl::gettimeofday();
                syncer->push_deliver_req(it->second);
                active.erase(it++);
            }
        } else if (m_cancel) {
            syncer->cancel();
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            cnt++;
            if (cnt > 10 * 60 * 10) {
                xwarn("xtop_download_executer::loop overtime: %s", syncer->symbol().c_str());
                ec = error::xerrc_t::state_sync_overtime;
                break;
            }
            continue;
        }
    }
}

void xtop_download_executer::cancel() {
    m_cancel = true;
}

void xtop_download_executer::push_track_req(const state_req & req) {
    std::lock_guard<std::mutex> lock(m_track_mutex);
    m_track_req.emplace_back(req);
}

void xtop_download_executer::pop_track_req() {
    std::lock_guard<std::mutex> lock(m_track_mutex);
    m_track_req.pop_front();
}

void xtop_download_executer::push_state_pack(const state_res & res) {
    std::lock_guard<std::mutex> lock(m_state_pack_mutex);
    m_state_packs.emplace_back(res);
}

void xtop_download_executer::pop_state_pack() {
    std::lock_guard<std::mutex> lock(m_state_pack_mutex);
    m_state_packs.pop_front();
}

void xtop_download_executer::push_single_state(const single_state_detail & detail) {
    std::lock_guard<std::mutex> lock(m_single_state_mutex);
    m_single_states.emplace_back(detail);
}

void xtop_download_executer::pop_single_state() {
    std::lock_guard<std::mutex> lock(m_single_state_mutex);
    m_single_states.pop_front();
}

}  // namespace state_sync
}  // namespace top
