// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

// TODO(jimmy) #include "xbase/xvledger.h"
#include "xbasic/xmemory.hpp"
#include "xgrpcservice/xgrpc_service.h"
#include "xmbus/xbase_sync_event_monitor.hpp"
#include "xmbus/xmessage_bus.h"
#include "xsyncbase/xsync_face.h"
#include "xvledger/xvblockstore.h"

#include <atomic>

NS_BEG2(top, grpcmgr)

int grpc_init(base::xvblockstore_t * block_store, sync::xsync_face_t * sync, uint16_t grpc_port);

class xgrpc_mgr_t;

class xgrpc_event_monitor_t : public mbus::xbase_sync_event_monitor_t {
public:
    xgrpc_event_monitor_t(observer_ptr<mbus::xmessage_bus_face_t> const & mbus, observer_ptr<base::xiothread_t> const & iothread, xgrpc_mgr_t * block_fetcher);
    void before_event_pushed(const mbus::xevent_ptr_t &e, bool &discard) override;
    bool filter_event(const mbus::xevent_ptr_t & e) override;
    void process_event(const mbus::xevent_ptr_t & e) override;

private:
    xgrpc_mgr_t * m_grpc_mgr{};
};

class xgrpc_mgr_t {
public:
    xgrpc_mgr_t(observer_ptr<mbus::xmessage_bus_face_t> const & mbus, observer_ptr<base::xiothread_t> const & iothread);

    void try_add_listener(const bool is_arc);
    void try_remove_listener(const bool is_arc);
    void process_event(const mbus::xevent_ptr_t & e);

private:
    observer_ptr<mbus::xmessage_bus_face_t> m_bus;
    std::unique_ptr<xgrpc_event_monitor_t> m_monitor{};
    std::atomic_uchar m_arc_count{0};
};

class handler_mgr : public top::rpc::xrpc_handle_face_t {
public:
    handler_mgr();

    void add_handler(std::shared_ptr<xrpc_handle_face_t> handle);
    bool handle(std::string& request, Json::Value& js_req, Json::Value& js_rsp, std::string & strResult, uint32_t& nErrorCode) override;

private:
    std::string m_response;
    std::vector<std::shared_ptr<xrpc_handle_face_t>> m_handles;
};

NS_END2
