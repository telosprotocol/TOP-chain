#pragma once

#include <atomic>
#include <mutex>
#include <unordered_map>
#include "xdata/xdata_common.h"
#include "xbasic/xmemory.hpp"
#include "xbase/xutl.h"

#include "xmbus/xmessage_bus.h"
#include "xvledger/xvblock.h"
#include "xsync/xsync_store.h"
#include "xsync/xrole_chains_mgr.h"
#include "xsync/xrole_xips_manager.h"
#include "xsync/xsync_sender.h"
#include "xsync/xgossip_message.h"

#include "xsync/xsync_peerset.h"
#include "xsync/xdownloader.h"
#include "xsync/xsync_time_rejecter.h"

NS_BEG2(top, sync)

class xsync_behind_checker_t {
public:
    xsync_behind_checker_t(std::string vnode_id, xsync_store_face_t *sync_store, xrole_chains_mgr_t *role_chains_mgr, xsync_peerset_t *peerset, xdownloader_face_t *downloader);
    void on_timer();
    void on_behind_check_event(const mbus::xevent_ptr_t &e);

private:
    void check_one(const std::string &address, enum_chain_sync_policy sync_policy, const vnetwork::xvnode_address_t &self_addr, const std::string &reason);

private:
    std::string m_vnode_id;
    xsync_store_face_t *m_sync_store;
    xrole_chains_mgr_t *m_role_chains_mgr;
    xsync_peerset_t *m_peerset;
    xdownloader_face_t *m_downloader;
    xsync_time_rejecter_t m_time_rejecter{900};

    int m_counter{0};
};

NS_END2
