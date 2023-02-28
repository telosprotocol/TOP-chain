// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <mutex>
#include <unordered_map>
#include <memory>
#include "xbasic/xmemory.hpp"
#include "xsync/xsync_store.h"
#include "xsync/xsync_sender.h"
#include "xsync/xsync_message.h"
#include "xmbus/xbase_sync_event_monitor.hpp"
// TODO(jimmy) #include "xbase/xvledger.h"
#include "xsync/xchain_block_fetcher.h"
#include "xsync/xrole_chains_mgr.h"

NS_BEG2(top, sync)

class xblock_fetcher_t;

class xblock_fetcher_event_monitor_t : public mbus::xbase_sync_event_monitor_t {
public:
    xblock_fetcher_event_monitor_t(observer_ptr<mbus::xmessage_bus_face_t> const &mbus,
        observer_ptr<base::xiothread_t> const & iothread,
        xblock_fetcher_t* block_fetcher);
    bool filter_event(const mbus::xevent_ptr_t& e) override;
    void process_event(const mbus::xevent_ptr_t& e) override;
    void before_event_pushed(const mbus::xevent_ptr_t &e, bool &discard) override;
private:
    xblock_fetcher_t* m_block_fetcher{};
};

class xblock_fetcher_timer_t : public top::base::xxtimer_t {
public:
    xblock_fetcher_timer_t(observer_ptr<mbus::xmessage_bus_face_t> const &mbus, base::xcontext_t &_context, int32_t timer_thread_id);

protected:
    ~xblock_fetcher_timer_t() override;

protected:
    bool on_timer_fire(const int32_t thread_id,const int64_t timer_id,const int64_t current_time_ms,const int32_t start_timeout_ms,int32_t & in_out_cur_interval_ms) override;

private:
    std::string m_vnode_id;
    observer_ptr<mbus::xmessage_bus_face_t> m_mbus;
};

class xblock_fetcher_t {
public:
    friend class xblock_fetcher_event_monitor_t;
    xblock_fetcher_t(std::string vnode_id, observer_ptr<base::xiothread_t> const & iothread,
        const observer_ptr<base::xvcertauth_t> &certauth,
        xrole_chains_mgr_t *role_chains_mgr,
        xsync_store_face_t *sync_store,
        xsync_sender_t *sync_sender);

public:
    void push_event(const mbus::xevent_ptr_t &e);

private:
    void process_event(const mbus::xevent_ptr_t &e);
    std::string get_address_by_event(const mbus::xevent_ptr_t &e);

    xchain_block_fetcher_ptr_t on_add_role(const std::string &address, const mbus::xevent_ptr_t& e);
    xchain_block_fetcher_ptr_t on_remove_role(const std::string &address, const mbus::xevent_ptr_t& e);
    xchain_block_fetcher_ptr_t on_timer_event(const mbus::xevent_ptr_t& e);
    xchain_block_fetcher_ptr_t on_newblock_event(const std::string &address, const mbus::xevent_ptr_t& e);
    xchain_block_fetcher_ptr_t on_newblockhash_event(const std::string &address, const mbus::xevent_ptr_t& e);
    xchain_block_fetcher_ptr_t on_response_block_event(const std::string &address, const mbus::xevent_ptr_t& e);

    xchain_block_fetcher_ptr_t find_chain(const std::string &address);
    xchain_block_fetcher_ptr_t create_chain(const std::string &address);
    void remove_chain(const std::string &address);

private:
    std::string m_vnode_id;
    observer_ptr<base::xvcertauth_t> m_certauth;
    xrole_chains_mgr_t *m_role_chains_mgr;
    xsync_store_face_t *m_sync_store;
    xsync_sender_t *m_sync_sender;

    std::unique_ptr<mbus::xmessage_bus_face_t> m_self_mbus{};
    std::unique_ptr<xblock_fetcher_event_monitor_t> m_monitor{};
    xblock_fetcher_timer_t *m_timer{};

    std::unordered_map<std::string, xchain_block_fetcher_ptr_t> m_chains;
};

NS_END2
