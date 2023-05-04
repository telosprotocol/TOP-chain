// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xsync/xblock_fetcher.h"

#include "xdata/xnative_contract_address.h"
#include "xmbus/xevent_account.h"
#include "xmbus/xevent_blockfetcher.h"
#include "xmbus/xevent_executor.h"
#include "xsync/xsync_log.h"
#include "xsync/xrole_chains_mgr.h"
#include "xsync/xsync_util.h"

NS_BEG2(top, sync)

using namespace mbus;


xblock_fetcher_event_monitor_t::xblock_fetcher_event_monitor_t(observer_ptr<mbus::xmessage_bus_face_t> const &mbus, 
        observer_ptr<base::xiothread_t> const & iothread,
        xblock_fetcher_t* block_fetcher):
xbase_sync_event_monitor_t(mbus, xsync_event_queue_size_max, iothread),
m_block_fetcher(block_fetcher) {

    mbus::xevent_queue_cb_t cb = std::bind(&xblock_fetcher_event_monitor_t::push_event, this, std::placeholders::_1);
    m_reg_holder.add_listener((int) mbus::xevent_major_type_account, cb);
    //m_reg_holder.add_listener((int) mbus::xevent_major_type_timer, cb);
    m_reg_holder.add_listener((int) mbus::xevent_major_type_sync_executor, cb);
    m_reg_holder.add_listener((int) mbus::xevent_major_type_blockfetcher, cb);
}

bool xblock_fetcher_event_monitor_t::filter_event(const mbus::xevent_ptr_t& e) {
#ifdef ENABLE_METRICS
    int64_t in, out;
    int32_t queue_size = m_observed_thread->count_calls(in, out);
    XMETRICS_GAUGE_SET_VALUE(metrics::mailbox_block_fetcher_cur, queue_size);
#endif
    return true;
}

void xblock_fetcher_event_monitor_t::process_event(const mbus::xevent_ptr_t& e) {
    m_block_fetcher->process_event(e);
}

void xblock_fetcher_event_monitor_t::before_event_pushed(const mbus::xevent_ptr_t& e, bool &discard) {
    switch(e->major_type) {
        case mbus::xevent_major_type_account:
            discard = false;
            break;
        case mbus::xevent_major_type_blockfetcher:
            if (e->minor_type == mbus::xevent_blockfetcher_t::newblock) {
                auto bme = dynamic_xobject_ptr_cast<mbus::xevent_blockfetcher_block_t>(e);
                std::string address_prefix;
                uint32_t table_id = 0;

                if (!data::xdatautil::extract_parts(bme->block->get_account(), address_prefix, table_id))
                    return;
                if (address_prefix == common::rec_table_base_address.to_string()) {
                    discard = false;
                } else if (address_prefix == common::zec_table_base_address.to_string()) {
                    discard = false;
                }
            }
            break;
    }
    XMETRICS_GAUGE(metrics::mailbox_block_fetcher_total, discard ? 0 : 1);
}

/////////
xblock_fetcher_timer_t::xblock_fetcher_timer_t(observer_ptr<mbus::xmessage_bus_face_t> const &mbus, base::xcontext_t &_context, int32_t timer_thread_id):
base::xxtimer_t(_context, timer_thread_id),
m_mbus(mbus) {

}

xblock_fetcher_timer_t::~xblock_fetcher_timer_t() {

}

bool xblock_fetcher_timer_t::on_timer_fire(const int32_t thread_id,const int64_t timer_id,const int64_t current_time_ms,const int32_t start_timeout_ms,int32_t & in_out_cur_interval_ms) {
    xevent_ptr_t ev = make_object_ptr<xevent_timer_t>();
    m_mbus->push_event(ev);
    return true;
}

///////////
xblock_fetcher_t::xblock_fetcher_t(std::string vnode_id, observer_ptr<base::xiothread_t> const &iothread, 
        const observer_ptr<base::xvcertauth_t> &certauth,
        xrole_chains_mgr_t *role_chains_mgr,
        xsync_store_face_t *sync_store,
        xsync_sender_t *sync_sender):
m_vnode_id(vnode_id),
m_certauth(certauth),
m_role_chains_mgr(role_chains_mgr),
m_sync_store(sync_store),
m_sync_sender(sync_sender) {

    m_self_mbus = top::make_unique<mbus::xmessage_bus_t>();
    m_monitor = top::make_unique<xblock_fetcher_event_monitor_t>(make_observer(m_self_mbus.get()), iothread, this);

    //chain_block_fetch do nothing now, so close it.
    //m_timer = new xblock_fetcher_timer_t(make_observer(m_self_mbus.get()), top::base::xcontext_t::instance(), iothread->get_thread_id());
    //m_timer->start(0, 50);
}

void xblock_fetcher_t::push_event(const mbus::xevent_ptr_t &e) {
    m_monitor->push_event(e);
}

std::string xblock_fetcher_t::get_address_by_event(const mbus::xevent_ptr_t &e) {

    switch(e->major_type) {
    case mbus::xevent_major_type_account:
        if (e->minor_type == mbus::xevent_account_t::add_role) {
            auto bme = dynamic_xobject_ptr_cast<mbus::xevent_account_add_role_t>(e);
            return bme->address;
        } else if (e->minor_type == mbus::xevent_account_t::remove_role) {
            auto bme = dynamic_xobject_ptr_cast<mbus::xevent_account_remove_role_t>(e);
            return bme->address;
        }
        break;
    case mbus::xevent_major_type_timer:
        break;
    case mbus::xevent_major_type_blockfetcher:
        if (e->minor_type == mbus::xevent_blockfetcher_t::newblock) {
            auto bme = dynamic_xobject_ptr_cast<mbus::xevent_blockfetcher_block_t>(e);
            return bme->block->get_account();
        } else if (e->minor_type == mbus::xevent_blockfetcher_t::newblockhash) {
            auto bme = dynamic_xobject_ptr_cast<mbus::xevent_blockfetcher_blockhash_t>(e);
            return bme->address;
        }
        break;
    case mbus::xevent_major_type_sync_executor:
        {
            auto bme = dynamic_xobject_ptr_cast<mbus::xevent_sync_response_blocks_t>(e);
            return bme->blocks[0]->get_account();
        }
        break;
    default:
        break;
    }

    return "";
}

void xblock_fetcher_t::process_event(const mbus::xevent_ptr_t& e) {

    std::string address = get_address_by_event(e);

    switch(e->major_type) {
    case mbus::xevent_major_type_account:
        if (e->minor_type == mbus::xevent_account_t::add_role) {
            on_add_role(address, e);
        } else if (e->minor_type == mbus::xevent_account_t::remove_role) {
            on_remove_role(address, e);
        }
        break;
    case mbus::xevent_major_type_timer:
        {
            on_timer_event(e);
        }
        break;
    case mbus::xevent_major_type_blockfetcher:
        if (e->minor_type == mbus::xevent_blockfetcher_t::newblock) {
            on_newblock_event(address, e);
        } else if (e->minor_type == mbus::xevent_blockfetcher_t::newblockhash) {
            on_newblockhash_event(address, e);
        }
        break;
    case mbus::xevent_major_type_sync_executor:
        if (e->minor_type == mbus::xevent_sync_executor_t::blocks) {
            on_response_block_event(address, e);
        }
        break;
    default:
        break;
    }
}

xchain_block_fetcher_ptr_t xblock_fetcher_t::on_add_role(const std::string &address, const mbus::xevent_ptr_t& e) {
    auto bme = dynamic_xobject_ptr_cast<mbus::xevent_account_add_role_t>(e);

    if (!m_role_chains_mgr->exists(address))
        return nullptr;

    xchain_block_fetcher_ptr_t chain = find_chain(address);
    if (chain == nullptr) {
        chain = create_chain(address);
    }
    return chain;
}

xchain_block_fetcher_ptr_t xblock_fetcher_t::on_remove_role(const std::string &address, const mbus::xevent_ptr_t& e) {
    auto bme = dynamic_xobject_ptr_cast<mbus::xevent_account_remove_role_t>(e);

    if (!m_role_chains_mgr->exists(address)) {
        remove_chain(address);
        return nullptr;
    }

    xchain_block_fetcher_ptr_t chain = find_chain(address);

    return chain;
}

xchain_block_fetcher_ptr_t xblock_fetcher_t::on_timer_event(const mbus::xevent_ptr_t& e) {

    // get all chains
    for (auto &it: m_chains) {
        it.second->on_timer();
    }

    return nullptr;
}

xchain_block_fetcher_ptr_t xblock_fetcher_t::on_newblock_event(const std::string &address, const mbus::xevent_ptr_t& e) {
    xchain_block_fetcher_ptr_t chain = find_chain(address);
    if (chain != nullptr) {

        auto bme = dynamic_xobject_ptr_cast<mbus::xevent_blockfetcher_block_t>(e);
        data::xblock_ptr_t & block = bme->block;
        const vnetwork::xvnode_address_t &network_self = bme->network_self;
        const vnetwork::xvnode_address_t &from_address = bme->from_address;

        chain->on_newblock(block, network_self, from_address);
    }

    return chain;
}

xchain_block_fetcher_ptr_t xblock_fetcher_t::on_newblockhash_event(const std::string &address, const mbus::xevent_ptr_t& e) {
    xchain_block_fetcher_ptr_t chain = find_chain(address);
    if (chain != nullptr) {
        auto bme = dynamic_xobject_ptr_cast<mbus::xevent_blockfetcher_blockhash_t>(e);
        uint64_t height = bme->height;
        std::string hash = bme->hash;
        const vnetwork::xvnode_address_t &network_self = bme->network_self;
        const vnetwork::xvnode_address_t &from_address = bme->from_address;
        chain->on_newblockhash(height, hash, network_self, from_address);
    }
    return chain;
}

xchain_block_fetcher_ptr_t xblock_fetcher_t::on_response_block_event(const std::string &address, const mbus::xevent_ptr_t& e) {
    xchain_block_fetcher_ptr_t chain = find_chain(address);
    if (chain != nullptr) {

        auto bme = dynamic_xobject_ptr_cast<mbus::xevent_sync_response_blocks_t>(e);
        data::xblock_ptr_t & block = bme->blocks[0];
        const vnetwork::xvnode_address_t &network_self = bme->self_address;
        const vnetwork::xvnode_address_t &from_address = bme->from_address;

        chain->on_response_blocks(block, network_self, from_address);
    }

    return chain;
}

xchain_block_fetcher_ptr_t xblock_fetcher_t::find_chain(const std::string &address) {

    auto it = m_chains.find(address);
    if (it != m_chains.end()) {
        return it->second;
    }

    return nullptr;
}

xchain_block_fetcher_ptr_t xblock_fetcher_t::create_chain(const std::string &address) {

    xchain_block_fetcher_ptr_t chain = std::make_shared<xchain_block_fetcher_t>(m_vnode_id, address, m_certauth, m_sync_store,  m_sync_sender);
    m_chains[address] = chain;
    return chain;
}

void xblock_fetcher_t::remove_chain(const std::string &address) {
    m_chains.erase(address);
}

NS_END2
