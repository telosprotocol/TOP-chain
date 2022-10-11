// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xstate_sync/xstate_sync.h"

namespace top {
namespace state_sync {

class xtop_download_executer {
public:
    explicit xtop_download_executer(const observer_ptr<base::xiothread_t> & thread);
    ~xtop_download_executer() = default;

    void run_state_sync(std::shared_ptr<xstate_sync_face_t> syncer, std::function<void(sync_result)> callback);
    void loop(std::shared_ptr<xstate_sync_face_t> syncer, std::error_code & ec);
    void cancel();

    void push_track_req(const state_req & req);
    void push_state_pack(const state_res & res);
    void push_single_state(const single_state_detail & detail);

private:
    void pop_track_req();
    void pop_state_pack();
    void pop_single_state();

    observer_ptr<base::xiothread_t> m_thread{nullptr};
    bool m_cancel{false};
    bool m_notify{false};
    std::list<state_req> m_track_req;
    std::list<state_res> m_state_packs;
    std::list<single_state_detail> m_single_states;
    std::mutex m_track_mutex;
    std::mutex m_state_pack_mutex;
    std::mutex m_single_state_mutex;
};
using xdownload_executer_t = xtop_download_executer;

}  // namespace state_sync
}  // namespace top