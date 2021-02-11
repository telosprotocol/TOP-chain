#pragma once

#include <unordered_map>
#include "xsync/xaccount.h"
#include "xsync/xrole_chains_mgr.h"
#include "xsync/xsync_status.h"
#include "xsync/xaccount_cache.h"
#include "xsync/xsync_store.h"
#include "xsync/xsync_sender.h"
#include "xmbus/xbase_sync_event_monitor.hpp"
#include "xsync/xsync_ratelimit.h"

NS_BEG2(top, sync)

class xdownloader_t;

class xaccount_timer_t : public top::base::xxtimer_t {
public:
    xaccount_timer_t(std::string vnode_id, base::xcontext_t &_context, int32_t timer_thread_id);
    void set_timeout_event(std::shared_ptr<xaccount_face_t> &account);

protected:
    ~xaccount_timer_t() override;

protected:
    bool on_timer_fire(const int32_t thread_id,const int64_t timer_id,const int64_t current_time_ms,const int32_t start_timeout_ms,int32_t & in_out_cur_interval_ms) override;

private:
    std::string m_vnode_id;
    std::map<int64_t,std::list<std::shared_ptr<xaccount_face_t>>> m_timeout_events;
    std::unordered_map<std::string,int64_t> m_accounts;
};

class xevent_monitor_t : public mbus::xbase_sync_event_monitor_t {
public:
    xevent_monitor_t(observer_ptr<mbus::xmessage_bus_face_t> const &mb, 
        observer_ptr<base::xiothread_t> const & iothread,
        xaccount_timer_t *timer,
        xdownloader_t* downloader);
    bool filter_event(const mbus::xevent_ptr_t& e) override;
    void process_event(const mbus::xevent_ptr_t& e) override;

public:
    xaccount_timer_t *m_timer;
    xdownloader_t* m_downloader;
};

class xdownloader_t {
public:
    friend class xevent_monitor_t;

    xdownloader_t(std::string vnode_id, xsync_store_face_t *sync_store,
                const observer_ptr<mbus::xmessage_bus_face_t> &mbus,
                const observer_ptr<base::xvcertauth_t> &certauth,
                xrole_chains_mgr_t *role_chains_mgr, xsync_status_t *sync_status, xsync_sender_t *sync_sender,
                const std::vector<observer_ptr<base::xiothread_t>> &thread_pool, xsync_ratelimit_face_t *ratelimit);

    virtual ~xdownloader_t();

    void push_event(const mbus::xevent_ptr_t &e);

private:
    std::string get_address_by_event(const mbus::xevent_ptr_t &e);
    uint32_t get_idx_by_address(const std::string &address);

    void process_event(const mbus::xevent_ptr_t &e, xaccount_timer_t *timer);

    xaccount_face_ptr_t on_add_role(const mbus::xevent_ptr_t &e);
    xaccount_face_ptr_t on_remove_role(const mbus::xevent_ptr_t &e);

    // any sync request has event source, only need find, not create
    xaccount_face_ptr_t on_response_event(const mbus::xevent_ptr_t &e);

    xaccount_face_ptr_t on_behind_event(const mbus::xevent_ptr_t &e);

    xaccount_face_ptr_t on_lack_event(const mbus::xevent_ptr_t &e);

    xaccount_face_ptr_t on_finish_lack_event(const mbus::xevent_ptr_t &e);

    xaccount_face_ptr_t on_find_block(const mbus::xevent_ptr_t &e);

private:

    xaccount_face_ptr_t find_account(const std::string &key, const xchain_info_t &info);

    xaccount_face_ptr_t find_sys_account(const std::string &key);

    xaccount_face_ptr_t find_account_except_done(const std::string &key, const xchain_info_t &info);

    xaccount_face_ptr_t create_account(const std::string &key, const xchain_info_t &info);

    void clear_old_account();

protected:
    std::string m_vnode_id;
    xsync_store_face_t *m_sync_store{};
    observer_ptr<mbus::xmessage_bus_face_t> m_mbus;
    observer_ptr<base::xvcertauth_t> m_certauth;
    xrole_chains_mgr_t *m_role_chains_mgr;
    xsync_status_t *m_sync_status;
    xsync_sender_t *m_sync_sender{};
    xsync_ratelimit_face_t *m_ratelimit;
    xaccount_cache_t m_account_queue;

    std::vector<xaccount_timer_t*> m_timer_list;
    std::vector<std::shared_ptr<mbus::xmessage_bus_t>> m_mbus_list;
    std::vector<std::shared_ptr<xevent_monitor_t>> m_monitor_list;
    uint32_t m_thread_count{0};
};

NS_END2
