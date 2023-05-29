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

xaccount_timer_t::xaccount_timer_t(std::string vnode_id, xsync_store_shadow_t* store_shadow, uint32_t thread_index, 
    base::xcontext_t &_context, int32_t timer_thread_id):
base::xxtimer_t(_context, timer_thread_id),
m_vnode_id(vnode_id),
m_store_shadow(store_shadow),
m_thread_index(thread_index) {
}

xaccount_timer_t::~xaccount_timer_t() {
}

void xaccount_timer_t::set_chain(xchain_downloader_face_ptr_t &chain_downloader) {
    for (auto it : m_chains) {
        if (it->get_address() == chain_downloader->get_address()) {
            break;
        }
    }

    m_chains.push_back(chain_downloader);
}

void xaccount_timer_t::del_chain(const std::string &address) {
    for (uint32_t i = 0; i < m_chains.size(); i++) {
        if (m_chains[i]->get_address() == address) {
            m_chains.erase(m_chains.begin() + i);
            break;
        }
    }
}

bool xaccount_timer_t::on_timer_fire(const int32_t thread_id,const int64_t timer_id,const int64_t current_time_ms,const int32_t start_timeout_ms,int32_t & in_out_cur_interval_ms) {
    int64_t now = base::xtime_utl::gmttime_ms();
    //xinfo("on_timer_fire,thread_id=%lld,timer_id=%lld,current_time_ms =%lld,start_timeout_ms=%d, in_out_cur_interval_ms=%d now=%d \n",thread_id,get_timer_id(), current_time_ms,start_timeout_ms,in_out_cur_interval_ms, now);

    if (m_chains.size() != 0) {
        uint32_t index = m_current_index_Of_chain % m_chains.size();
        for (uint32_t count = 0; count < m_chains.size(); index = (index + 1) % m_chains.size()) {
            count++;
            if (m_chains[index]->downloading(now)){
                continue;
            }
            
            if (!m_chains[index]->on_timer(now)) {
                break;
            }
        }

        m_current_index_Of_chain = index;
    }

    m_count++;

    if (m_count % m_shadow_time_out == 0){
        m_store_shadow->on_timer(m_thread_index);
    }
    
    return true;
}


xevent_monitor_t::xevent_monitor_t(uint32_t idx, observer_ptr<mbus::xmessage_bus_face_t> const &mb, observer_ptr<base::xiothread_t> const & iothread,
    xdownloader_t* downloader):
xbase_sync_event_monitor_t(mb, xsync_event_queue_size_max, iothread),
m_idx(idx),
m_downloader(downloader) {
    mbus::xevent_queue_cb_t cb = std::bind(&xevent_monitor_t::push_event, this, std::placeholders::_1);
    m_reg_holder.add_listener((int) mbus::xevent_major_type_sync_executor, cb);
    m_reg_holder.add_listener((int) mbus::xevent_major_type_behind, cb);
    m_reg_holder.add_listener((int) mbus::xevent_major_type_account, cb);
    m_reg_holder.add_listener((int) mbus::xevent_major_type_store, cb);
}

void xevent_monitor_t::before_event_pushed(const mbus::xevent_ptr_t &e, bool &discard) {
    if (e->minor_type == mbus::xevent_major_type_account) {
        discard = false;
    }
    XMETRICS_GAUGE(metrics::mailbox_downloader_total, discard ? 0 : 1);
}

bool xevent_monitor_t::filter_event(const mbus::xevent_ptr_t& e) {
    // TODO filter
#ifdef ENABLE_METRICS
    int64_t in, out;
    int32_t queue_size = m_observed_thread->count_calls(in, out);
    XMETRICS_GAUGE_SET_VALUE(metrics::mailbox_downloader_cur, queue_size);
#endif

    return true;
}

void xevent_monitor_t::process_event(const mbus::xevent_ptr_t& e) {
    m_downloader->process_event(m_idx, e);
}

////////

xdownloader_t::xdownloader_t(std::string vnode_id, xsync_store_face_t *sync_store,
            const observer_ptr<base::xvcertauth_t> &certauth,
            xrole_xips_manager_t *role_xips_mgr,
            xrole_chains_mgr_t *role_chains_mgr, xsync_sender_t *sync_sender,
            const std::vector<observer_ptr<base::xiothread_t>> &thread_pool, xsync_ratelimit_face_t *ratelimit, xsync_store_shadow_t * shadow):
m_vnode_id(vnode_id),
m_sync_store(sync_store),
m_certauth(certauth),
m_role_xips_mgr(role_xips_mgr),
m_role_chains_mgr(role_chains_mgr),
m_sync_sender(sync_sender),
m_ratelimit(ratelimit),
m_store_shadow(shadow){
    m_store_shadow->set_downloader(this);
    m_thread_count = thread_pool.size();
    for (uint32_t i=0; i<m_thread_count; i++) {
        std::shared_ptr<xmessage_bus_t> bus = std::make_shared<xmessage_bus_t>();
        m_mbus_list.push_back(bus);

        xaccount_timer_t *timer = new xaccount_timer_t(vnode_id, m_store_shadow, i, top::base::xcontext_t::instance(), thread_pool[i]->get_thread_id());
        timer->start(0, GET_TOKEN_RETRY_INTERVAL / 10);
        m_timer_list.push_back(timer);

        std::shared_ptr<xevent_monitor_t> monitor = std::make_shared<xevent_monitor_t>(i, make_observer(bus.get()), thread_pool[i],  this);
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

std::string xdownloader_t::get_address_by_event(const mbus::xevent_ptr_t &e) const {
    switch(e->major_type) {
    case mbus::xevent_major_type_sync_executor:
        if (e->minor_type == mbus::xevent_sync_executor_t::blocks) {
            auto const bme = dynamic_xobject_ptr_cast<mbus::xevent_sync_response_blocks_t>(e);
            return bme->blocks[0]->get_account();
        } else if (e->minor_type == mbus::xevent_sync_executor_t::archive_blocks) {
            auto const bme = dynamic_xobject_ptr_cast<mbus::xevent_sync_archive_blocks_t>(e);
            return bme->blocks[0]->get_account();
        } 
        break;
    case mbus::xevent_major_type_behind:
        if (e->minor_type == mbus::xevent_behind_t::type_download) {
            auto const bme = dynamic_xobject_ptr_cast<mbus::xevent_behind_download_t>(e);
            return bme->address;
        }
        break;
    case mbus::xevent_major_type_account:
        if (e->minor_type == mbus::xevent_account_t::add_role) {
            auto const bme = dynamic_xobject_ptr_cast<mbus::xevent_account_add_role_t>(e);
            return bme->address;
        } else if (e->minor_type == mbus::xevent_account_t::remove_role) {
            auto const bme = dynamic_xobject_ptr_cast<mbus::xevent_account_remove_role_t>(e);
            return bme->address;
        }
        break;
    case mbus::xevent_major_type_store:
        if (e->minor_type == mbus::xevent_store_t::type_block_committed) {
            auto const bme = dynamic_xobject_ptr_cast<mbus::xevent_store_block_committed_t>(e);
            return bme->owner;
        }
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

void xdownloader_t::process_event(uint32_t idx, const mbus::xevent_ptr_t &e) {

    xchain_downloader_face_ptr_t chain_downloader = nullptr;

    switch(e->major_type) {
    case mbus::xevent_major_type_sync_executor:
        if (e->minor_type == mbus::xevent_sync_executor_t::blocks) {
            chain_downloader = on_response_event(idx, e);
        } else if (e->minor_type == mbus::xevent_sync_executor_t::archive_blocks) {
            chain_downloader = on_archive_blocks(idx, e);
        } 
        break;
    case mbus::xevent_major_type_behind:
        if (e->minor_type == mbus::xevent_behind_t::type_download) {
            chain_downloader = on_behind_event(idx, e);
        }
        break;
    case mbus::xevent_major_type_account:
        if (e->minor_type == mbus::xevent_account_t::add_role) {
            chain_downloader = on_add_role(idx, e);
        } else if (e->minor_type == mbus::xevent_account_t::remove_role) {
            chain_downloader = on_remove_role(idx, e);
        }
        break;
    case mbus::xevent_major_type_store:
        if (e->minor_type == mbus::xevent_store_t::type_block_committed) {
            chain_downloader = on_block_committed_event(idx,e);
        }    
    default:
        break;
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
        if (chain_downloader != nullptr){
            m_timer_list[idx]->set_chain(chain_downloader);
        }    
    }
    return chain_downloader;
}

xchain_downloader_face_ptr_t xdownloader_t::on_remove_role(uint32_t idx, const mbus::xevent_ptr_t &e) {
    auto bme = dynamic_xobject_ptr_cast<mbus::xevent_account_remove_role_t>(e);
    const std::string &address = bme->address;

    if (!m_role_chains_mgr->exists(address)) {
        m_timer_list[idx]->del_chain(address);
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
xchain_downloader_face_ptr_t xdownloader_t::on_archive_blocks(uint32_t idx, const mbus::xevent_ptr_t &e) {
    //xsync_dbg("downloader on_response_event");

    auto bme = dynamic_xobject_ptr_cast<mbus::xevent_sync_archive_blocks_t>(e);

    const std::string &address = bme->blocks[0]->get_account();

    xchain_downloader_face_ptr_t chain_downloader = find_chain_downloader(idx, address);
    if (chain_downloader != nullptr) {

        std::vector<data::xblock_ptr_t> &blocks = bme->blocks;
        vnetwork::xvnode_address_t &self_addr = bme->self_address;
        vnetwork::xvnode_address_t &from_addr = bme->from_address;

        chain_downloader->on_archive_blocks(blocks, self_addr, from_addr);
    }

    return chain_downloader;
}

xchain_downloader_face_ptr_t xdownloader_t::on_behind_event(uint32_t idx, const mbus::xevent_ptr_t &e) {

    auto bme = dynamic_xobject_ptr_cast<mbus::xevent_behind_download_t>(e);

    //xsync_dbg("downloader on_behind_event %s", bme->address.c_str());

    xchain_downloader_face_ptr_t chain_downloader = find_chain_downloader(idx, bme->address);
    if (chain_downloader != nullptr) {
        const std::string &reason = bme->reason;
        chain_downloader->on_behind(bme->sync_policy, bme->chain_behind_address_map, reason);
    }

    return chain_downloader;
}

xchain_downloader_face_ptr_t xdownloader_t::on_block_committed_event(uint32_t idx, const mbus::xevent_ptr_t &e) {
    auto bme = dynamic_xobject_ptr_cast<mbus::xevent_store_block_committed_t>(e);
    m_store_shadow->on_chain_event(bme->owner, bme->blk_height);
    xchain_downloader_face_ptr_t chain_downloader = find_chain_downloader(idx, bme->owner);
    if (chain_downloader != nullptr) {
        chain_downloader->on_block_committed_event(bme->blk_height);
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

    xchain_downloader_face_ptr_t account = std::make_shared<xchain_downloader_t>(m_vnode_id, m_sync_store, m_role_xips_mgr, 
        m_role_chains_mgr, m_certauth, m_sync_sender, m_ratelimit, address);
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
