// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xsync/xdownloader.h"
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

void xaccount_timer_t::set_timeout_event(xchain_downloader_face_ptr_t &chain_downloader) {

    int64_t next_timeout = chain_downloader->get_next_timeout();

    if (next_timeout == 0)
        return;

    std::string address = chain_downloader->get_address();
    {
        auto it = m_accounts.find(address);
        if (it != m_accounts.end()) {

            if (it->second == next_timeout) {
                return;
            }

            auto it2 = m_timeout_events.find(it->second);
            assert(it2 != m_timeout_events.end());

            std::list<xchain_downloader_face_ptr_t>::iterator it3 = it2->second.begin();
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
        std::list<xchain_downloader_face_ptr_t> list_accounts;
        list_accounts.push_back(chain_downloader);
        m_timeout_events[next_timeout] = list_accounts;
    } else {
        it->second.push_back(chain_downloader);
    }

    xsync_dbg("downloader set_account_timer %s %ld", address.c_str(), next_timeout);

    m_accounts[address] = next_timeout;
}

bool xaccount_timer_t::on_timer_fire(const int32_t thread_id,const int64_t timer_id,const int64_t current_time_ms,const int32_t start_timeout_ms,int32_t & in_out_cur_interval_ms) {
    //printf("on_timer_fire,timer_id=%lld,current_time_ms =%lld,start_timeout_ms=%d, in_out_cur_interval_ms=%d \n",get_timer_id(), current_time_ms,start_timeout_ms,in_out_cur_interval_ms);
    int64_t now = base::xtime_utl::gmttime_ms();
    std::list<xchain_downloader_face_ptr_t> list_timeout_events;

    std::map<int64_t,std::list<xchain_downloader_face_ptr_t>>::iterator it = m_timeout_events.begin();
    for (;it!=m_timeout_events.end();) {

        if (now < it->first) {
            break;
        }

        while (!it->second.empty()) {
            xchain_downloader_face_ptr_t &chain_downloader = it->second.front();

            chain_downloader->on_timer(now);
            list_timeout_events.push_back(chain_downloader);

            std::string address = chain_downloader->get_address();
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

xevent_monitor_t::xevent_monitor_t(uint32_t idx, observer_ptr<mbus::xmessage_bus_face_t> const &mb, observer_ptr<base::xiothread_t> const & iothread,
    xaccount_timer_t *timer, xdownloader_t* downloader):
xbase_sync_event_monitor_t(mb, 10000, iothread),
m_idx(idx),
m_timer(timer),
m_downloader(downloader) {
    mbus::xevent_queue_cb_t cb = std::bind(&xevent_monitor_t::push_event, this, std::placeholders::_1);
    m_reg_holder.add_listener((int) mbus::xevent_major_type_sync_executor, cb);
    m_reg_holder.add_listener((int) mbus::xevent_major_type_behind, cb);
    m_reg_holder.add_listener((int) mbus::xevent_major_type_account, cb);
}

bool xevent_monitor_t::filter_event(const mbus::xevent_ptr_t& e) {
    // TODO filter
    XMETRICS_COUNTER_INCREMENT("sync_downloader_event_count", 1);
    return true;
}

void xevent_monitor_t::process_event(const mbus::xevent_ptr_t& e) {
    XMETRICS_COUNTER_INCREMENT("sync_downloader_event_count", -1);
    m_downloader->process_event(m_idx, e, m_timer);
}

////////

xdownloader_t::xdownloader_t(std::string vnode_id, xsync_store_face_t *sync_store,
            const observer_ptr<mbus::xmessage_bus_face_t> &mbus,
            const observer_ptr<base::xvcertauth_t> &certauth,
            xrole_chains_mgr_t *role_chains_mgr, xsync_sender_t *sync_sender,
            const std::vector<observer_ptr<base::xiothread_t>> &thread_pool, xsync_ratelimit_face_t *ratelimit):
m_vnode_id(vnode_id),
m_sync_store(sync_store),
m_mbus(mbus),
m_certauth(certauth),
m_role_chains_mgr(role_chains_mgr),
m_sync_sender(sync_sender),
m_ratelimit(ratelimit) {

    m_thread_count = thread_pool.size();
    for (uint32_t i=0; i<m_thread_count; i++) {
        std::shared_ptr<xmessage_bus_t> bus = std::make_shared<xmessage_bus_t>();
        m_mbus_list.push_back(bus);

        xaccount_timer_t *timer = new xaccount_timer_t(vnode_id, top::base::xcontext_t::instance(), thread_pool[i]->get_thread_id());
        timer->start(0, GET_TOKEN_RETRY_INTERVAL / 10);
        m_timer_list.push_back(timer);

        std::shared_ptr<xevent_monitor_t> monitor = std::make_shared<xevent_monitor_t>(i, make_observer(bus.get()), thread_pool[i], timer, this);
        m_monitor_list.push_back(monitor);

        std::unordered_map<std::string, xchain_downloader_face_ptr_t> empty_chains;
        m_vector_chains.push_back(empty_chains);
    }

    xsync_kinfo("downloader create %u", m_thread_count);
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
            auto bme = dynamic_xobject_ptr_cast<mbus::xevent_sync_response_blocks_t>(e);
            return bme->blocks[0]->get_account();
        } else if (e->minor_type == mbus::xevent_sync_executor_t::chain_snapshot) {
            auto bme = dynamic_xobject_ptr_cast<mbus::xevent_chain_snaphsot_t>(e);
            return bme->m_tbl_account_addr;
        }
        break;
    case mbus::xevent_major_type_behind:
        if (e->minor_type == mbus::xevent_behind_t::type_download) {
            auto bme = dynamic_xobject_ptr_cast<mbus::xevent_behind_download_t>(e);
            return bme->address;
        }
        break;
    case mbus::xevent_major_type_account:
        if (e->minor_type == mbus::xevent_account_t::add_role) {
            auto bme = dynamic_xobject_ptr_cast<mbus::xevent_account_add_role_t>(e);
            return bme->address;
        } else if (e->minor_type == mbus::xevent_account_t::remove_role) {
            auto bme = dynamic_xobject_ptr_cast<mbus::xevent_account_remove_role_t>(e);
            return bme->address;
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

void xdownloader_t::process_event(uint32_t idx, const mbus::xevent_ptr_t &e, xaccount_timer_t *timer) {

    xchain_downloader_face_ptr_t chain_downloader = nullptr;

    switch(e->major_type) {
    case mbus::xevent_major_type_sync_executor:
        if (e->minor_type == mbus::xevent_sync_executor_t::blocks) {
            XMETRICS_TIME_RECORD("sync_cost_response_blocks_event");
            chain_downloader = on_response_event(idx, e);
        } else if (e->minor_type == mbus::xevent_sync_executor_t::chain_snapshot) {
            chain_downloader = on_chain_snapshot_response_event(idx, e);
        }
        break;
    case mbus::xevent_major_type_behind:
        if (e->minor_type == mbus::xevent_behind_t::type_download) {
            XMETRICS_TIME_RECORD("sync_cost_behind_event");
            chain_downloader = on_behind_event(idx, e);
        }
        break;
    case mbus::xevent_major_type_account:
        if (e->minor_type == mbus::xevent_account_t::add_role) {
            XMETRICS_TIME_RECORD("sync_cost_chain_add_role_event");
            chain_downloader = on_add_role(idx, e);
        } else if (e->minor_type == mbus::xevent_account_t::remove_role) {
            XMETRICS_TIME_RECORD("sync_cost_chain_remove_role_event");
            chain_downloader = on_remove_role(idx, e);
        }
        break;
    default:
        break;
    }

    if (chain_downloader != nullptr) {
        timer->set_timeout_event(chain_downloader);
    }
}

xchain_downloader_face_ptr_t xdownloader_t::on_add_role(uint32_t idx, const mbus::xevent_ptr_t &e) {
    auto bme = dynamic_xobject_ptr_cast<mbus::xevent_account_add_role_t>(e);
    const std::string &address = bme->address;

    if (!m_role_chains_mgr->exists(address))
        return nullptr;

    xchain_downloader_face_ptr_t chain_downloader = find_chain_downloader(idx, address);
    if (chain_downloader == nullptr) {
        chain_downloader = create_chain_downloader(idx, address);
    } else {
        // TODO
        //chain_downloader->on_role_changed(info);
    }
    return chain_downloader;
}

xchain_downloader_face_ptr_t xdownloader_t::on_remove_role(uint32_t idx, const mbus::xevent_ptr_t &e) {
    auto bme = dynamic_xobject_ptr_cast<mbus::xevent_account_remove_role_t>(e);
    const std::string &address = bme->address;

    if (!m_role_chains_mgr->exists(address)) {
        remove_chain_downloader(idx, address);
        return nullptr;
    }

    xchain_downloader_face_ptr_t chain_downloader = find_chain_downloader(idx, address);
    if (chain_downloader != nullptr) {
        // TODO
        //chain_downloader->on_role_changed(info);
    }

    return chain_downloader;
}

// any sync request has event source, only need find, not create
xchain_downloader_face_ptr_t xdownloader_t::on_response_event(uint32_t idx, const mbus::xevent_ptr_t &e) {
    //xsync_dbg("downloader on_response_event");

    auto bme = dynamic_xobject_ptr_cast<mbus::xevent_sync_response_blocks_t>(e);

    const std::string &address = bme->blocks[0]->get_account();

    xchain_downloader_face_ptr_t chain_downloader = find_chain_downloader(idx, address);
    if (chain_downloader != nullptr) {

        std::vector<data::xblock_ptr_t> &blocks = bme->blocks;
        vnetwork::xvnode_address_t &self_addr = bme->self_address;
        vnetwork::xvnode_address_t &from_addr = bme->from_address;

        chain_downloader->on_response(blocks, self_addr, from_addr);
    }

    return chain_downloader;
}

// any sync request has event source, only need find, not create
xchain_downloader_face_ptr_t xdownloader_t::on_chain_snapshot_response_event(uint32_t idx, const mbus::xevent_ptr_t &e) {
    //xsync_dbg("downloader on_response_event");
    auto bme = dynamic_xobject_ptr_cast<mbus::xevent_chain_snaphsot_t>(e);
    const std::string &address = bme->m_tbl_account_addr;

    xchain_downloader_face_ptr_t chain_downloader = find_chain_downloader(idx, address);
    if (chain_downloader != nullptr) {
        vnetwork::xvnode_address_t &self_addr = bme->self_address;
        vnetwork::xvnode_address_t &from_addr = bme->from_address;
        chain_downloader->on_chain_snapshot_response(bme->m_tbl_account_addr,
                bme->m_chain_snapshot, bme->m_height, self_addr, from_addr);
    }

    return chain_downloader;
}

xchain_downloader_face_ptr_t xdownloader_t::on_behind_event(uint32_t idx, const mbus::xevent_ptr_t &e) {

    auto bme = dynamic_xobject_ptr_cast<mbus::xevent_behind_download_t>(e);

    //xsync_dbg("downloader on_behind_event %s", bme->address.c_str());

    xchain_downloader_face_ptr_t chain_downloader = find_chain_downloader(idx, bme->address);
    if (chain_downloader != nullptr) {

        uint64_t start_height = bme->start_height;
        uint64_t end_height = bme->end_height;
        enum_chain_sync_policy sync_policy = bme->sync_policy;
        vnetwork::xvnode_address_t &self_addr = bme->self_addr;
        vnetwork::xvnode_address_t &from_addr = bme->from_addr;
        const std::string &reason = bme->reason;

        chain_downloader->on_behind(start_height, end_height, sync_policy, self_addr, from_addr, reason);
    }

    return chain_downloader;
}

xchain_downloader_face_ptr_t xdownloader_t::find_chain_downloader(uint32_t idx, const std::string &address) {

    auto it = m_vector_chains[idx].find(address);
    if (it != m_vector_chains[idx].end()) {
        return it->second;
    }

    return nullptr;
}

xchain_downloader_face_ptr_t xdownloader_t::create_chain_downloader(uint32_t idx, const std::string &address) {

    xchain_downloader_face_ptr_t account = std::make_shared<xchain_downloader_t>(m_vnode_id, m_sync_store, m_mbus, m_certauth, m_sync_sender, m_ratelimit, address);
    m_vector_chains[idx][address] = account;
    return account;
}

void xdownloader_t::remove_chain_downloader(uint32_t idx, const std::string &address) {
    xchain_downloader_face_ptr_t account = find_chain_downloader(idx, address);
    if (account != nullptr) {
        // clear timer
        account->destroy();
    }
    m_vector_chains[idx].erase(address);
}

NS_END2
