// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xgrpc_mgr/xgrpc_mgr.h"

#include "xgrpcservice/xgrpc_service.h"
#include "xmbus/xevent_store.h"
#include "xrpc/xrpc_query_manager.h"
#include "xdata/xblocktool.h"

NS_BEG2(top, grpcmgr)

using namespace top::data;
using namespace top::rpc;

xgrpc_event_monitor_t::xgrpc_event_monitor_t(observer_ptr<mbus::xmessage_bus_face_t> const & mbus, observer_ptr<base::xiothread_t> const & iothread, xgrpc_mgr_t * grpc_mgr)
  : xbase_sync_event_monitor_t(mbus, 10000, iothread), m_grpc_mgr(grpc_mgr) {
    mbus::xevent_queue_cb_t cb = std::bind(&xgrpc_event_monitor_t::push_event, this, std::placeholders::_1);
    m_reg_holder.add_listener((int)mbus::xevent_major_type_store, cb);
}

void xgrpc_event_monitor_t::before_event_pushed(const mbus::xevent_ptr_t &e, bool &discard) {
    XMETRICS_GAUGE(metrics::mailbox_grpc_total, discard ? 0 : 1);
}

bool xgrpc_event_monitor_t::filter_event(const mbus::xevent_ptr_t & e) {
#ifdef ENABLE_METRICS
    int64_t in, out;
    int32_t queue_size = m_observed_thread->count_calls(in, out);
    XMETRICS_GAUGE_SET_VALUE(metrics::mailbox_grpc_cur, queue_size);
#endif
    return true;
}

void xgrpc_event_monitor_t::process_event(const mbus::xevent_ptr_t & e) {
    m_grpc_mgr->process_event(e);
}

xgrpc_mgr_t::xgrpc_mgr_t(observer_ptr<mbus::xmessage_bus_face_t> const & mbus, observer_ptr<base::xiothread_t> const & iothread) : m_bus{mbus} {
    m_monitor = top::make_unique<xgrpc_event_monitor_t>(make_observer(m_bus.get()), iothread, this);
}

void xgrpc_mgr_t::try_add_listener(const bool is_arc) {
    if (is_arc) {
        m_arc_count++;
    }
    xdbg("grpc add listener arc cnt: %d, is_arc: %d", m_arc_count.load(), is_arc);
}

void xgrpc_mgr_t::try_remove_listener(const bool is_arc) {
    if (is_arc) {
        m_arc_count--;
    }
    xdbg("grpc remove listener arc cnt: %d, is_arc: %d", m_arc_count.load(), is_arc);
}

void xgrpc_mgr_t::process_event(const mbus::xevent_ptr_t & e) {
    if (e == nullptr || m_arc_count < 1) {
        xdbg("grpc stream xevent_ptr_t nullptr or not arc");
        return;
    }
    if (rpc_client_num <= 0) {
        xdbg("grpc no client connected");
        return;
    }
    if (e->minor_type != mbus::xevent_store_t::type_block_committed)
        return;

    mbus::xevent_store_block_committed_ptr_t block_event = dynamic_xobject_ptr_cast<mbus::xevent_store_block_committed_t>(e);
    // only process light-table
    if (block_event->blk_class != base::enum_xvblock_class_light
        || block_event->blk_level != base::enum_xvblock_level_table) {
        return;
    }

    auto block = mbus::extract_block_from(block_event, metrics::blockstore_access_from_mbus_grpc_process_event);
    xdbg("grpc stream get_block_type %d, minor_type %d \n", block->get_block_type(), e->minor_type);
    if (!block->is_tableblock()) {
        return;
    }

    top::xrpc::xrpc_query_manager bh(nullptr, nullptr, nullptr, nullptr);
    auto bp = block.get();
    Json::Value j;
    if (rpc::rpc_version == rpc::xrpc_version_2) {
        j["value"] = bh.get_block_json(bp, data::RPC_VERSION_V2);
    } else {
        j["value"] = bh.get_block_json(bp, data::RPC_VERSION_V1);
    }

    static std::atomic_int cnt{0};
    j["result"] = cnt++;

// #ifdef DEBUG
//     // adding push tx log info
//     auto txactions = data::xblockextract_t::unpack_txactions(bp);
//     for (auto & txaction : txactions) {
//         xdbg("grpc stream tx hash: tx_key=%s", txaction.get_tx_dump_key().c_str());
//     }
// #endif

    {
        std::unique_lock<std::mutex> lck(tableblock_mtx);
        xdbg("grpc stream tableblock_data before push , size %zu, cnt %d ", tableblock_data.size(), cnt.load());
        xdbg("grpc stream on_event_tableblock_update, address:%s height:%d", block->get_block_owner().c_str(), block->get_height());

        // only save 2048 table block at most in case grpc disconnected
        const uint16_t deque_size_max{2048};
        if (tableblock_data.size() >= deque_size_max) {
            tableblock_data.pop_front();
        }
        tableblock_data.push_back(j);

        xinfo("grpc stream xgetblock_handler tableblock_data after push: cache size %zu, cnt %d", tableblock_data.size(), cnt.load());
    }

    tableblock_cv.notify_one();
    xdbg("grpc stream xgetblock_handler tableblock_data after tableblock_cv.notify_one()");
}

handler_mgr::handler_mgr() {
}

void handler_mgr::add_handler(std::shared_ptr<xrpc_handle_face_t> handle) {
    m_handles.push_back(handle);
}

bool handler_mgr::handle(std::string & request, Json::Value& js_req, Json::Value& js_rsp, std::string & strResult, uint32_t & nErrorCode) {
    for (auto v : m_handles) {
        auto ret = v->handle(request, js_req, js_rsp, strResult, nErrorCode);
        js_rsp["result"] = strResult;
        std::string response = js_rsp.toStyledString();
#ifdef DEBUG
        std::cout << "======================== request " << request << std::endl;
        std::cout << "======================== response " << response << std::endl;
#endif
        if (ret) {
            xdbg("grpc request:%s success", request.c_str());
            return true;
        } else {
            xinfo("grpc request:%s error", request.c_str());
        }
    }
    return false;
}

int grpc_init(base::xvblockstore_t * block_store, sync::xsync_face_t * sync, uint16_t grpc_port) {
    // updating node infos periodically
    // elect::ElectMockReporterIntf::Instance()->RegCallback(on_report_node);

    static std::unique_ptr<xgrpc_service> grpc_srv = top::make_unique<xgrpc_service>("0.0.0.0", grpc_port);

    static std::shared_ptr<handler_mgr> handle_mgr = std::make_shared<handler_mgr>();
    static std::shared_ptr<xrpc_handle_face_t> handle_store =
        std::make_shared<top::xrpc::xrpc_query_manager>(observer_ptr<base::xvblockstore_t>(block_store), sync, nullptr);
    handle_mgr->add_handler(handle_store);

    grpc_srv->register_handle(handle_mgr);
    grpc_srv->start();

    return 0;
}

NS_END2
