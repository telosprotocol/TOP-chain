// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xsync/xdownloader.h"
#include "xmbus/xevent_store.h"
#include "xmbus/xevent_lack.h"
#include "xmbus/xevent_executor.h"
#include "xsync/xsync_log.h"
#include "xmetrics/xmetrics.h"
#include "xdata/xgenesis_data.h"
#include "xsync/xsync_message.h"

NS_BEG2(top, sync)

using namespace mbus;

xaccount_timer_t::xaccount_timer_t(std::string vnode_id, base::xcontext_t &_context, int32_t timer_thread_id):
base::xxtimer_t(_context, timer_thread_id),
m_vnode_id(vnode_id) {
}

xaccount_timer_t::~xaccount_timer_t() {
}

void xaccount_timer_t::set_timeout_event(std::shared_ptr<xaccount_face_t> &account) {

    int64_t next_timeout = account->get_next_timeout();

    if (next_timeout == 0)
        return;

    std::string address = account->get_address();
    {
        auto it = m_accounts.find(address);
        if (it != m_accounts.end()) {

            if (it->second == next_timeout) {
                return;
            }

            auto it2 = m_timeout_events.find(it->second);
            assert(it2 != m_timeout_events.end());

            std::list<std::shared_ptr<xaccount_face_t>>::iterator it3 = it2->second.begin();
            for (;it3!=it2->second.end();) {
                std::string tmp = (*it3)->get_address();
                if (address != tmp) {
                    ++it3;
                } else {
                    it2->second.erase(it3++);
                    break;
                }
            }

            if (it2->second.empty())
                m_timeout_events.erase(it2);

            m_accounts.erase(address);
        }
    }

    auto it = m_timeout_events.find(next_timeout);
    if (it == m_timeout_events.end()) {
        std::list<std::shared_ptr<xaccount_face_t>> list_accounts;
        list_accounts.push_back(account);
        m_timeout_events[next_timeout] = list_accounts;
    } else {
        it->second.push_back(account);
    }

    xsync_dbg("[downloader] set_account_timer %s %ld", address.c_str(), next_timeout);

    m_accounts[address] = next_timeout;
}

bool xaccount_timer_t::on_timer_fire(const int32_t thread_id,const int64_t timer_id,const int64_t current_time_ms,const int32_t start_timeout_ms,int32_t & in_out_cur_interval_ms) {
    //printf("on_timer_fire,timer_id=%lld,current_time_ms =%lld,start_timeout_ms=%d, in_out_cur_interval_ms=%d \n",get_timer_id(), current_time_ms,start_timeout_ms,in_out_cur_interval_ms);
    int64_t now = base::xtime_utl::gmttime_ms();
    std::list<std::shared_ptr<xaccount_face_t>> list_timeout_events;

    std::map<int64_t,std::list<std::shared_ptr<xaccount_face_t>>>::iterator it = m_timeout_events.begin();
    for (;it!=m_timeout_events.end();) {

        if (now < it->first) {
            break;
        }

        while (!it->second.empty()) {
            std::shared_ptr<xaccount_face_t> &account = it->second.front();

            account->on_timer_event(now);
            list_timeout_events.push_back(account);

            std::string address = account->get_address();
            m_accounts.erase(address);

            it->second.pop_front();
        }

        m_timeout_events.erase(it++);
    }

    for (auto &it : list_timeout_events) {
        set_timeout_event(it);
    }

    return true;
}

////////

xevent_monitor_t::xevent_monitor_t(observer_ptr<mbus::xmessage_bus_face_t> const &mb, observer_ptr<base::xiothread_t> const & iothread,
    xaccount_timer_t *timer, xdownloader_t* downloader):
xbase_sync_event_monitor_t(mb, 10000, iothread),
m_timer(timer),
m_downloader(downloader) {
    mbus::xevent_queue_cb_t cb = std::bind(&xevent_monitor_t::push_event, this, std::placeholders::_1);
    m_reg_holder.add_listener((int) mbus::xevent_major_type_sync_executor, cb);
    m_reg_holder.add_listener((int) mbus::xevent_major_type_behind, cb);
    m_reg_holder.add_listener((int) mbus::xevent_major_type_account, cb);
    m_reg_holder.add_listener((int) mbus::xevent_major_type_lack, cb);
}

bool xevent_monitor_t::filter_event(const mbus::xevent_ptr_t& e) {
    // TODO filter
    XMETRICS_COUNTER_INCREMENT("sync_downloader_event_count", 1);
    return true;
}

void xevent_monitor_t::process_event(const mbus::xevent_ptr_t& e) {
    XMETRICS_COUNTER_INCREMENT("sync_downloader_event_count", -1);
    m_downloader->process_event(e, m_timer);
}

////////

xdownloader_t::xdownloader_t(std::string vnode_id, xsync_store_face_t *sync_store,
            const observer_ptr<mbus::xmessage_bus_face_t> &mbus,
            const observer_ptr<base::xvcertauth_t> &certauth,
            xrole_chains_mgr_t *role_chains_mgr, xsync_status_t *sync_status, xsync_sender_t *sync_sender,
            const std::vector<observer_ptr<base::xiothread_t>> &thread_pool, xsync_ratelimit_face_t *ratelimit):
m_vnode_id(vnode_id),
m_sync_store(sync_store),
m_mbus(mbus),
m_certauth(certauth),
m_role_chains_mgr(role_chains_mgr),
m_sync_status(sync_status),
m_sync_sender(sync_sender),
m_ratelimit(ratelimit),
m_account_queue(100000) {

    m_thread_count = thread_pool.size();
    for (uint32_t i=0; i<m_thread_count; i++) {
        std::shared_ptr<xmessage_bus_t> bus = std::make_shared<xmessage_bus_t>();
        m_mbus_list.push_back(bus);

        xaccount_timer_t *timer = new xaccount_timer_t(vnode_id, top::base::xcontext_t::instance(), thread_pool[i]->get_thread_id());
        timer->start(0, 10);
        m_timer_list.push_back(timer);

        std::shared_ptr<xevent_monitor_t> monitor = std::make_shared<xevent_monitor_t>(make_observer(bus.get()), thread_pool[i], timer, this);
        m_monitor_list.push_back(monitor);
    }

    xsync_kinfo("[downloader] create %u", m_thread_count);
}

xdownloader_t::~xdownloader_t() {}

void xdownloader_t::push_event(const mbus::xevent_ptr_t &e) {

    std::string address = get_address_by_event(e);
    if (address.empty())
        return;

    uint32_t idx = get_idx_by_address(address);

    m_mbus_list[idx]->push_event(e);
}

std::string xdownloader_t::get_address_by_event(const mbus::xevent_ptr_t &e) {
    switch(e->major_type) {
    case mbus::xevent_major_type_sync_executor:
        if (e->minor_type == mbus::xevent_sync_executor_t::blocks) {
            auto bme = std::static_pointer_cast<mbus::xevent_sync_response_blocks_t>(e);
            return bme->blocks[0]->get_account();
        }
        break;
    case mbus::xevent_major_type_behind:
        if (e->minor_type == mbus::xevent_behind_t::type_known) {
            auto bme = std::static_pointer_cast<mbus::xevent_behind_block_t>(e);
            return bme->successor_block->get_account();
        }
        break;
    case mbus::xevent_major_type_account:
        if (e->minor_type == mbus::xevent_account_t::find_block) {
            return "";
        } else if (e->minor_type == mbus::xevent_account_t::add_role) {
            auto bme = std::static_pointer_cast<mbus::xevent_account_add_role_t>(e);
            return bme->address;
        } else if (e->minor_type == mbus::xevent_account_t::remove_role) {
            auto bme = std::static_pointer_cast<mbus::xevent_account_remove_role_t>(e);
            return bme->address;
        }
        break;
    case mbus::xevent_major_type_lack:
        if (e->minor_type == mbus::xevent_lack_t::type_block) {
            return "";
        }
        break;
    default:
        break;
    }

    return "";
}

uint32_t xdownloader_t::get_idx_by_address(const std::string &address) {
    uint8_t x = 0;
    for (auto &it: address) {
        x += (uint8_t)it;
    }
    x = x%m_thread_count;

    return (uint32_t)x;
}

void xdownloader_t::process_event(const mbus::xevent_ptr_t &e, xaccount_timer_t *timer) {

    xaccount_face_ptr_t account = nullptr;

    switch(e->major_type) {
    case mbus::xevent_major_type_sync_executor:
        if (e->minor_type == mbus::xevent_sync_executor_t::blocks) {
            XMETRICS_TIME_RECORD("sync_cost_response_blocks_event");
            account = on_response_event(e);
        }
        break;
    case mbus::xevent_major_type_behind:
        if (e->minor_type == mbus::xevent_behind_t::type_known) {
            XMETRICS_TIME_RECORD("sync_cost_behind_event");
            account = on_behind_event(e);
        }
        break;
    case mbus::xevent_major_type_account:
        if (e->minor_type == mbus::xevent_account_t::find_block) {
            XMETRICS_TIME_RECORD("sync_cost_find_block_event");
            account = on_find_block(e);
        } else if (e->minor_type == mbus::xevent_account_t::add_role) {
            XMETRICS_TIME_RECORD("sync_cost_chain_add_role_event");
            account = on_add_role(e);
        } else if (e->minor_type == mbus::xevent_account_t::remove_role) {
            XMETRICS_TIME_RECORD("sync_cost_chain_remove_role_event");
            account = on_remove_role(e);
        }
        break;
    case mbus::xevent_major_type_lack:
        if (e->minor_type == mbus::xevent_lack_t::type_block) {
            XMETRICS_TIME_RECORD("sync_cost_lack_event");
            account = on_lack_event(e);
        }
        break;
    default:
        break;
    }

    if (account != nullptr) {
        timer->set_timeout_event(account);
    }
}

xaccount_face_ptr_t xdownloader_t::on_add_role(const mbus::xevent_ptr_t &e) {
    auto bme = std::static_pointer_cast<mbus::xevent_account_add_role_t>(e);
    const std::string &address = bme->address;

    xchain_info_t info;
    if (!m_role_chains_mgr->get_chain(address, info))
        return nullptr;

    xaccount_face_ptr_t account = find_account(info.address, info);
    if (account == nullptr) {
        account = create_account(info.address, info);
    } else {
        account->on_role_changed(info);
    }
    return account;
}

xaccount_face_ptr_t xdownloader_t::on_remove_role(const mbus::xevent_ptr_t &e) {
    auto bme = std::static_pointer_cast<mbus::xevent_account_remove_role_t>(e);
    const std::string &address = bme->address;

    xchain_info_t info;
    if (!m_role_chains_mgr->get_chain(address, info)) {
        uint32_t which = WALK_MAP_ALWAYS_STAY|WALK_MAP_ACTIVE;
        m_account_queue.remove(address, which);
        return nullptr;
    }

    xaccount_face_ptr_t account = find_account(info.address, info);
    if (account != nullptr) {
        account->on_role_changed(info);
    }

    return account;
}

// any sync request has event source, only need find, not create
xaccount_face_ptr_t xdownloader_t::on_response_event(const mbus::xevent_ptr_t &e) {
    //xsync_dbg("[downloader] on_response_event");

    auto bme = std::static_pointer_cast<mbus::xevent_sync_response_blocks_t>(e);

    const std::string &owner = bme->blocks[0]->get_account();

    xchain_info_t info;
    if (!m_role_chains_mgr->get_chain(owner, info))
        return nullptr;

    xaccount_face_ptr_t account = find_account_except_done(info.address, info);
    if (account != nullptr) {
        account->on_response_event(e);
        if (account->is_idle()) {
            m_account_queue.done(info.address);
        }
    }

    return account;
}

xaccount_face_ptr_t xdownloader_t::on_behind_event(const mbus::xevent_ptr_t &e) {

    auto bme = std::static_pointer_cast<mbus::xevent_behind_block_t>(e);

    //xsync_dbg("[downloader] on_behind_event %s", bme->owner.c_str());

    xchain_info_t info;
    if (!m_role_chains_mgr->get_chain(bme->successor_block->get_account(), info)) {
        return nullptr;
    }

    xaccount_face_ptr_t account = find_account(info.address, info);
    if (account == nullptr)
        account = create_account(info.address, info);

    if (account != nullptr) {
        account->on_behind_event(e);
        if (account->is_idle()) {
            m_account_queue.done(info.address);
        }
    }

    return account;
}

xaccount_face_ptr_t xdownloader_t::on_lack_event(const mbus::xevent_ptr_t &e) {

    //xsync_dbg("[downloader] on_lack_elect_event");

    auto bme = std::static_pointer_cast<mbus::xevent_lack_block_t>(e);

    xchain_info_t info;
    if (!m_role_chains_mgr->get_chain(bme->address, info)) {
        return nullptr;
    }

    xaccount_face_ptr_t account = find_account(info.address, info);
    if (account == nullptr)
        account = create_account(info.address, info);

    if (account != nullptr) {
        account->on_lack_event(bme->set_heights);
        if (account->is_idle()) {
            m_account_queue.done(info.address);
        }
    }

    return account;
}

xaccount_face_ptr_t xdownloader_t::on_find_block(const mbus::xevent_ptr_t &e) {

    auto bme = std::static_pointer_cast<mbus::xevent_account_find_block_t>(e);

    const std::string &address = bme->address;
    uint64_t height = bme->height;
    uint64_t block_time = bme->block_time;

    xchain_info_t info;
    if (!m_role_chains_mgr->get_chain(address, info))
        return nullptr;

    xaccount_face_ptr_t account = find_account(info.address, info);
    if (account == nullptr)
        account = create_account(info.address, info);

    if (account != nullptr) {
        //xsync_dbg("[downloader] on_find_block %d %s %lu", base::enum_xvblock_level_unit, address.c_str(), height);
        account->on_find_block(height, block_time);
    }
    return account;
}

xaccount_face_ptr_t xdownloader_t::find_account(const std::string &key, const xchain_info_t &info) {

    uint32_t which = 0;
    if (info.is_sys_account)
        which = WALK_MAP_ALWAYS_STAY;
    else
        which = WALK_MAP_ACTIVE|WALK_MAP_DONE;

    return m_account_queue.get(key, which);
}

xaccount_face_ptr_t xdownloader_t::find_sys_account(const std::string &key) {
    return m_account_queue.get(key, WALK_MAP_ALWAYS_STAY);
}

xaccount_face_ptr_t xdownloader_t::find_account_except_done(const std::string &key, const xchain_info_t &info) {

    uint32_t which = 0;
    if (info.is_sys_account)
        which = WALK_MAP_ALWAYS_STAY;
    else
        which = WALK_MAP_ACTIVE;

    return m_account_queue.get(key, which);
}

xaccount_face_ptr_t xdownloader_t::create_account(const std::string &key, const xchain_info_t &info) {

    xaccount_face_ptr_t account = nullptr;

    // unused now, it was planned to prepare for beacon-chain only
    //account = std::make_shared<xaccount_sequence_t>(m_vnode_id, m_sync_store, m_mbus.get(), m_sync_sender, m_ratelimit, info);

    account = std::make_shared<xaccount_general_t>(m_vnode_id, m_sync_store, m_mbus, m_certauth, m_sync_sender, m_ratelimit, info);

    m_account_queue.add(key, account, info.is_sys_account);

    return account;
}

void xdownloader_t::clear_old_account() {
    // unit first, ensure unit's window is same as table's
    m_account_queue.walk_remove([&](const std::string &id, const xaccount_face_ptr_t &account){

        std::string address = account->get_address();

        bool is_unit_address = data::is_unit_address(common::xaccount_address_t{address});
        if (is_unit_address) {
            xchain_info_t info;
            if (!m_role_chains_mgr->get_chain(address, info))
                return true;

            account->on_role_changed(info);
        }

        return false;

    }, WALK_MAP_ALWAYS_STAY|WALK_MAP_ACTIVE|WALK_MAP_DONE);

    m_account_queue.walk_remove([&](const std::string &id, const xaccount_face_ptr_t &account){

        std::string address = account->get_address();

        bool is_table_address = data::is_table_address(common::xaccount_address_t{address});
        if (is_table_address) {
            xchain_info_t info;
            if (!m_role_chains_mgr->get_chain(address, info))
                return true;

            account->on_role_changed(info);
        }

        return false;

    }, WALK_MAP_ALWAYS_STAY);
}

NS_END2
