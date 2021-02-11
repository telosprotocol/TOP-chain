// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xsync/xsync_handler.h"
#include <inttypes.h>
#include "xsync/xsync_log.h"
#include "xbase/xutl.h"
#include "xvnetwork/xaddress.h"
#include "xsync/xgossip_message.h"
#include "xmetrics/xmetrics.h"
#include "xconfig/xconfig_register.h"
#include "xsync/xdeceit_node_manager.h"
#include "xvnetwork/xmessage.h"
#include "xsync/xsync_message.h"
#include "xbase/xvledger.h"
#include "xblockstore/xblockstore_face.h"
#include "xsync/xsync_util.h"
#include "xsync/xsync_sender.h"
#include "xmbus/xevent_role.h"
#include "xmbus/xevent_downloader.h"
#include "xmbus/xevent_executor.h"
#include "xbasic/xutility.h"
#include "xsyncbase/xmessage_ids.h"

NS_BEG2(top, sync)

using namespace base;
using namespace mbus;
using namespace vnetwork;
using namespace syncbase;
using namespace data;

const uint32_t max_request_block_count = 100;

xsync_handler_t::xsync_handler_t(std::string vnode_id,
    xsync_store_face_t *sync_store,
    const observer_ptr<mbus::xmessage_bus_face_t> &mbus,
    xsession_manager_t *session_mgr, xdeceit_node_manager_t *blacklist,
    xrole_chains_mgr_t *role_chains_mgr, xrole_xips_manager_t *role_xips_mgr, 
    xdownloader_t *downloader, xblock_fetcher_t *block_fetcher,
    xsync_gossip_t *sync_gossip,
    xsync_sender_t *sync_sender,
    xsync_latest_t *sync_latest):
m_vnode_id(vnode_id),
m_sync_store(sync_store),
m_session_mgr(session_mgr),
m_blacklist(blacklist),
m_role_chains_mgr(role_chains_mgr),
m_role_xips_mgr(role_xips_mgr),
m_downloader(downloader),
m_block_fetcher(block_fetcher),
m_sync_gossip(sync_gossip),
m_block_processor(vnode_id, mbus),
m_sync_sender(sync_sender),
m_sync_latest(sync_latest) {
}

xsync_handler_t::~xsync_handler_t() {
}

void xsync_handler_t::on_message(
    const vnetwork::xvnode_address_t &from_address,
    const vnetwork::xvnode_address_t &network_self,
    const xbyte_buffer_t &msg,
    xmessage_t::message_type msg_type,
    xtop_vnetwork_message::hash_result_type msg_hash,
    int64_t recv_time) {

    uint32_t msg_size = msg.size();

    base::xstream_t stream(base::xcontext_t::instance(), (uint8_t*)msg.data(), msg.size());

    xsync_message_header_ptr_t header = make_object_ptr<xsync_message_header_t>();
    header->serialize_from(stream);

    if (header->code == (uint8_t)xsync_msg_err_code_t::decode) {
        xsync_warn("[xsync_handler] on_message decode failed %" PRIx64 " %s", msg_hash, from_address.to_string().c_str());
        return;
    }

#if defined __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wswitch"
#elif defined __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wswitch"
#endif

    switch (msg_type) {
        case xmessage_id_sync_get_blocks:
        {
            XMETRICS_COUNTER_INCREMENT("sync_pkgs_getblocks_recv", 1);
            XMETRICS_COUNTER_INCREMENT("sync_bytes_getblocks_recv", msg_size);

            get_blocks(from_address, network_self, stream, msg_hash, recv_time);
            break;
        }
        case xmessage_id_sync_newblock:
        {
            XMETRICS_COUNTER_INCREMENT("sync_pkgs_newblock_recv", 1);
            XMETRICS_COUNTER_INCREMENT("sync_bytes_newblock_recv", msg_size);

            newblock(from_address, network_self, stream, msg_hash, recv_time);
            break;
        }
        case xmessage_id_sync_newblockhash:
        {
            XMETRICS_COUNTER_INCREMENT("sync_pkgs_newblockhash_recv", 1);
            XMETRICS_COUNTER_INCREMENT("sync_bytes_newblockhash_recv", msg_size);

            newblockhash(from_address, network_self, stream, msg_hash, recv_time);
            break;
        }
        case xmessage_id_sync_blocks:
        {
            XMETRICS_COUNTER_INCREMENT("sync_pkgs_blocks_recv", 1);
            XMETRICS_COUNTER_INCREMENT("sync_bytes_blocks_recv", msg_size);
            blocks(from_address, network_self, header, stream, msg_hash, recv_time);
            break;
        }
        case xmessage_id_sync_gossip:
        case xmessage_id_sync_frozen_gossip:
        {
            XMETRICS_COUNTER_INCREMENT("sync_pkgs_gossip_recv", 1);
            XMETRICS_COUNTER_INCREMENT("sync_bytes_gossip_recv", msg_size);
            gossip(from_address, network_self, stream, msg_hash, recv_time);
            break;
        }
        case xmessage_id_sync_latest_block_info:
        {
            XMETRICS_COUNTER_INCREMENT("sync_pkgs_latest_block_info_recv", 1);
            XMETRICS_COUNTER_INCREMENT("sync_bytes_latest_block_info_recv", msg_size);
            latest_block_info(from_address, network_self, stream, msg_hash, recv_time);
            break;
        }
        case xmessage_id_sync_get_latest_blocks:
        {
            XMETRICS_COUNTER_INCREMENT("sync_pkgs_get_latest_blocks_recv", 1);
            XMETRICS_COUNTER_INCREMENT("sync_bytes_get_latest_blocks_recv", msg_size);
            get_latest_blocks(from_address, network_self, stream, msg_hash, recv_time);
            break;
        }
        case xmessage_id_sync_latest_blocks:
        {
            XMETRICS_COUNTER_INCREMENT("sync_pkgs_latest_blocks_recv", 1);
            XMETRICS_COUNTER_INCREMENT("sync_bytes_latest_blocks_recv", msg_size);
            latest_blocks(from_address, network_self, stream, msg_hash, recv_time);
            break;
        }
    }
#if defined __clang__
#pragma clang diagnostic pop
#elif defined __GNUC__
#pragma GCC diagnostic pop
#endif
}

void xsync_handler_t::on_event(const mbus::xevent_ptr_t& e) {

    m_downloader->push_event(e);

    if (e->major_type == mbus::xevent_major_type_timer) {
        m_sync_gossip->on_timer();
        m_block_fetcher->on_timer_event(e);
        m_sync_latest->on_timer();
    }

    if (e->major_type == mbus::xevent_major_type_chain_timer) {
        m_sync_gossip->on_chain_timer(e);
    }

    if (e->major_type==mbus::xevent_major_type_behind && e->minor_type==mbus::xevent_behind_t::type_origin) {
        m_sync_gossip->on_behind_event(e);
    }

    if (e->major_type == mbus::xevent_major_type_role) {
        handle_role_change(e);
    }

    if (e->major_type == mbus::xevent_major_type_consensus) {
        handle_consensus_result(e);
    }

    if (e->major_type == mbus::xevent_major_type_store) {
        m_block_processor.handle_block(e);
    }

    if (e->major_type == mbus::xevent_major_type_downloader && e->minor_type==mbus::xevent_downloader_t::complete) {
        m_block_fetcher->on_downloader_event(e);
    }
}

void xsync_handler_t::get_blocks(const vnetwork::xvnode_address_t &from_address,
    const vnetwork::xvnode_address_t &network_self,
    base::xstream_t &stream,
    xtop_vnetwork_message::hash_result_type msg_hash,
    int64_t recv_time) {

    xsync_message_get_blocks_ptr_t ptr = make_object_ptr<xsync_message_get_blocks_t>();
    ptr->serialize_from(stream);

    const std::string &owner = ptr->owner;
    uint64_t start_height = ptr->start_height;
    uint32_t count = ptr->count;

    if (owner == "" || start_height == 0 || count == 0)
        return;

    xsync_info("[xsync_handler] receive getblocks %" PRIx64 " wait(%ldms) %s range[%lu,%lu] %s",
        msg_hash, get_time()-recv_time, owner.c_str(), start_height, start_height+count-1, from_address.to_string().c_str());

    std::vector<xblock_ptr_t> vector_blocks;

    // check service counts
    auto const max_total_counts = XGET_CONFIG(executor_max_total_sessions_service_counts);
    auto const total_counts = XGET_CONFIG(executor_max_session_service_counts);
    auto const time_interval = XGET_CONFIG(executor_session_time_interval);

    if (!m_session_mgr->plus(from_address, max_total_counts, total_counts, time_interval)) {
        xsync_warn("[xsync_handler] getblocks out of quota %s %s", owner.c_str(), from_address.to_string().c_str());
        m_sync_sender->send_blocks(xsync_msg_err_code_t::limit, owner, vector_blocks, from_address, network_self);
        return;
    }

    for (uint32_t i=0; i<count && i<max_request_block_count; i++) {
        uint64_t height = start_height + (uint64_t)i;

        xblock_ptr_t block = nullptr;
        xauto_ptr<xvblock_t> blk_ptr = m_sync_store->load_block_object(owner, height);
        if (blk_ptr != nullptr) {
            block = autoptr_to_blockptr(blk_ptr);
        } else {
            base::xauto_ptr<base::xvblock_t> blk_ptr = m_sync_store->get_current_block(owner);
            if (blk_ptr!=nullptr && blk_ptr->get_height()==height) {

                assert(blk_ptr->is_input_ready(true));
                assert(blk_ptr->is_output_ready(true));

                block = autoptr_to_blockptr(blk_ptr);
            }
        }

        if (block != nullptr) {
#ifdef DEBUG
            xsync_dbg("[xsync_handler] get_blocks %s %lu %s",
                ptr->owner.c_str(), block->get_height(), block->dump().c_str());
#endif

            vector_blocks.push_back(block);
        }
    }

    m_sync_sender->send_blocks(xsync_msg_err_code_t::succ, owner, vector_blocks, from_address, network_self);
}

void xsync_handler_t::newblock(
        const vnetwork::xvnode_address_t &from_address,
        const vnetwork::xvnode_address_t &network_self,
        base::xstream_t &stream,
        xtop_vnetwork_message::hash_result_type msg_hash,
        int64_t recv_time) {

    xsync_message_newblock_ptr_t ptr = make_object_ptr<xsync_message_newblock_t>();
    ptr->serialize_from(stream);

    if (ptr->block == nullptr)
        return;

    xblock_ptr_t &block = ptr->block;

    xsync_info("[xsync_handler] receive newblock %" PRIx64 " %s %s", msg_hash, block->dump().c_str(), from_address.to_string().c_str());

    const std::string &address = block->get_account();
    if (!m_role_chains_mgr->exists(address))
        return;

    m_block_fetcher->handle_newblock(block, from_address, network_self);
}

void xsync_handler_t::newblockhash(
        const vnetwork::xvnode_address_t &from_address,
        const vnetwork::xvnode_address_t &network_self,
        base::xstream_t &stream,
        xtop_vnetwork_message::hash_result_type msg_hash,
        int64_t recv_time) {

    xsync_message_newblockhash_ptr_t ptr = make_object_ptr<xsync_message_newblockhash_t>();
    ptr->serialize_from(stream);

    const std::string &address = ptr->address;
    uint64_t height = ptr->height;
    uint64_t view_id = ptr->view_id;

    if (address == "" || height == 0 || view_id == 0)
        return;

    xsync_info("[xsync_handler] receive newblockhash %" PRIx64 " wait(%ldms) %s,height=%lu,view_id:%lu %s", 
        msg_hash, get_time()-recv_time, address.c_str(), height, view_id, from_address.to_string().c_str());

    m_block_fetcher->handle_newblockhash(address, height, view_id, from_address, network_self);
}

void xsync_handler_t::blocks(const vnetwork::xvnode_address_t &from_address,
    const vnetwork::xvnode_address_t &network_self,
    const xsync_message_header_ptr_t &header,
    base::xstream_t &stream,
    xtop_vnetwork_message::hash_result_type msg_hash,
    int64_t recv_time) {

    xsync_message_blocks_ptr_t ptr = make_object_ptr<xsync_message_blocks_t>();
    ptr->serialize_from(stream);

    std::vector<data::xblock_ptr_t> &blocks = ptr->blocks;

    uint32_t count = blocks.size();

    if (count == 0) {
        xsync_info("[xsync_handler] receive blocks %" PRIx64 " wait(%ldms) count(%u) code(%u) %s", 
            msg_hash, get_time()-recv_time, count, header->code, from_address.to_string().c_str());

        return;
    }

    XMETRICS_COUNTER_INCREMENT("sync_handler_blocks", count);

    xsync_info("[xsync_handler] receive blocks %" PRIx64 " wait(%ldms) %s count(%u) code(%u) %s", 
        msg_hash, get_time()-recv_time, blocks[0]->get_account().c_str(), count, header->code, from_address.to_string().c_str());

    // check continuous
    xvblock_t *successor = nullptr;
    std::vector<data::xblock_ptr_t>::reverse_iterator rit = blocks.rbegin();
    for (;rit!=blocks.rend(); rit++) {
        xblock_ptr_t &block = *rit;

        if (successor != nullptr) {

            if (block->get_account() != successor->get_account()) {
                xsync_warn("[xsync_handler] receive blocks(address error) (%s, %s)",
                    block->get_account().c_str(), successor->get_account().c_str());
                return;
            }

            if (block->get_block_hash() != successor->get_last_block_hash()) {
                xsync_warn("[xsync_handler] receive blocks(sequence error) (%s, %s)",
                    block->dump().c_str(), successor->dump().c_str());
                return;
            }
        }

        successor = block.get();
    }

    if (m_block_fetcher->filter_block(blocks, from_address, network_self))
        return;

    if (count > 0) {
        mbus::xevent_ptr_t e = std::make_shared<mbus::xevent_sync_response_blocks_t>(blocks, network_self, from_address);
        m_downloader->push_event(e);
    }
}

void xsync_handler_t::gossip(const vnetwork::xvnode_address_t &from_address,
    const vnetwork::xvnode_address_t &network_self,
    base::xstream_t &stream,
    xtop_vnetwork_message::hash_result_type msg_hash,
    int64_t recv_time) {

    xsync_message_gossip_ptr_t ptr = make_object_ptr<xsync_message_gossip_t>();
    ptr->serialize_from(stream);

    std::vector<xgossip_chain_info_ptr_t> &info_list = ptr->info_list;
    std::map<std::string, xgossip_behind_info_t> behind_chain_set;

    m_sync_gossip->handle_message(info_list, from_address, network_self, behind_chain_set);

    for (auto &it: behind_chain_set) {
        const std::string &address = it.first;
        uint64_t height = it.second.peer_height;
        uint64_t view_id = it.second.peer_view_id;

        //xsync_info("[xsync_handler] on_gossip %s,height=%lu,viewid=%lu, %s", address.c_str(), height, view_id, from_address.to_string().c_str());
        m_block_fetcher->handle_gossip_behind(address, height, view_id, from_address, network_self);
    }
}

void xsync_handler_t::latest_block_info(const vnetwork::xvnode_address_t &from_address,
    const vnetwork::xvnode_address_t &network_self,
    base::xstream_t &stream,
    xtop_vnetwork_message::hash_result_type msg_hash,
    int64_t recv_time) {

    xsync_message_latest_block_info_ptr_t ptr = make_object_ptr<xsync_message_latest_block_info_t>();
    ptr->serialize_from(stream);

    std::vector<xlatest_block_info_t> &info_list = ptr->info_list;

    m_sync_latest->handle_latest_block_info(info_list, from_address, network_self);
}

void xsync_handler_t::get_latest_blocks(const vnetwork::xvnode_address_t &from_address,
    const vnetwork::xvnode_address_t &network_self,
    base::xstream_t &stream,
    xtop_vnetwork_message::hash_result_type msg_hash,
    int64_t recv_time) {

    xsync_message_get_latest_blocks_ptr_t ptr = make_object_ptr<xsync_message_get_latest_blocks_t>();
    ptr->serialize_from(stream);

    const std::string &address = ptr->address;
    const std::vector<xlatest_block_item_t> &range = ptr->list;
    base::xvaccount_t _vaccount(address);

    std::vector<xblock_ptr_t> vector_blocks;

    for (const auto &it: range) {

        const xlatest_block_item_t &item = it;

        base::xauto_ptr<base::xvblock_t> vblock = m_sync_store->query_block(_vaccount, item.height, item.hash);
 
        if (vblock != nullptr) {
            xblock_ptr_t block = autoptr_to_blockptr(vblock);
            vector_blocks.push_back(block);
        }
    }

    m_sync_sender->send_latest_blocks(vector_blocks, network_self, from_address);
}

void xsync_handler_t::latest_blocks(const vnetwork::xvnode_address_t &from_address,
    const vnetwork::xvnode_address_t &network_self,
    base::xstream_t &stream,
    xtop_vnetwork_message::hash_result_type msg_hash,
    int64_t recv_time) {

    xsync_message_latest_blocks_ptr_t ptr = make_object_ptr<xsync_message_latest_blocks_t>();
    ptr->serialize_from(stream);

    std::vector<xblock_ptr_t> &blocks = ptr->blocks;
    if (blocks.size() == 0)
        return;

    m_sync_latest->handle_latest_blocks(blocks, from_address);
}

void xsync_handler_t::handle_role_change(const mbus::xevent_ptr_t& e) {
    if (e->minor_type == xevent_role_t::add_role) {

        auto bme = std::static_pointer_cast<mbus::xevent_role_add_t>(e);
        std::shared_ptr<vnetwork::xvnetwork_driver_face_t> &vnetwork_driver = bme->m_vnetwork_driver;

        vnetwork::xvnode_address_t addr = vnetwork_driver->address();
        std::vector<uint16_t> table_ids = vnetwork_driver->table_ids();


        std::vector<common::xnode_address_t> parent_addresses;
        std::vector<common::xnode_address_t> neighbor_addresses;
        std::vector<common::xnode_address_t> archive_addresses;

        if (common::has<common::xnode_type_t::consensus_validator>(vnetwork_driver->type())) {
            auto const & parents_info = vnetwork_driver->parents_info2();
            for (auto const & info : parents_info) {
                parent_addresses.push_back(top::get<data::xnode_info_t>(info).address);
            }
        }

        // except edge
        if ((vnetwork_driver->type() & common::xnode_type_t::edge) == common::xnode_type_t::invalid) {
            auto const & neighbors_info = vnetwork_driver->neighbors_info2();
            for (auto const & info : neighbors_info) {
                neighbor_addresses.push_back(top::get<data::xnode_info_t>(info).address);
            }
        }

        m_role_xips_mgr->add_role(addr, neighbor_addresses, parent_addresses, vnetwork_driver->archive_addresses());


        XMETRICS_COUNTER_INCREMENT("sync_cost_role_add_event", 1);

        int64_t tm1 = base::xtime_utl::gmttime_ms();

        map_chain_info_t old_chains = m_role_chains_mgr->get_all_chains();

        std::string old_role_string = m_role_chains_mgr->get_roles_string();
        std::shared_ptr<xrole_chains_t> role_chains = std::make_shared<xrole_chains_t>(addr, table_ids);
        m_role_chains_mgr->add_role(role_chains);
        std::string new_role_string = m_role_chains_mgr->get_roles_string();

        int64_t tm2 = base::xtime_utl::gmttime_ms();

        xsync_kinfo("[xsync_handler] add_role_phase1 %s cost:%dms", addr.to_string().c_str(), tm2-tm1);

        xchains_wrapper_t& chains_wrapper = role_chains->get_chains_wrapper();
        const map_chain_info_t &chains = chains_wrapper.get_chains();
        for (const auto &it: chains) {
            xevent_ptr_t ev = std::make_shared<mbus::xevent_account_add_role_t>(it.second.address);
            m_downloader->push_event(ev);
        }

        m_sync_gossip->add_role(addr);
        m_sync_latest->add_role(addr);

        int64_t tm3 = base::xtime_utl::gmttime_ms();
        xsync_kinfo("[xsync_handler] add_role_phase2 %s cost:%dms", addr.to_string().c_str(), tm3-tm2);
        xsync_kinfo("[xsync_handler] add_role_result(before) %s %s", addr.to_string().c_str(), old_role_string.c_str());
        xsync_kinfo("[xsync_handler] add_role_result(after) %s %s", addr.to_string().c_str(), new_role_string.c_str());

    } else if (e->minor_type == xevent_role_t::remove_role) {
        auto bme = std::static_pointer_cast<mbus::xevent_role_remove_t>(e);
        std::shared_ptr<vnetwork::xvnetwork_driver_face_t> &vnetwork_driver = bme->m_vnetwork_driver;

        vnetwork::xvnode_address_t addr = vnetwork_driver->address();
        std::vector<uint16_t> table_ids = vnetwork_driver->table_ids();

        m_role_xips_mgr->remove_role(addr);

        map_chain_info_t old_chains = m_role_chains_mgr->get_all_chains();

        XMETRICS_COUNTER_INCREMENT("sync_cost_role_remove_event", 1);

        int64_t tm1 = base::xtime_utl::gmttime_ms();

        std::string old_role_string = m_role_chains_mgr->get_roles_string();
        std::shared_ptr<xrole_chains_t> role_chains = std::make_shared<xrole_chains_t>(addr, table_ids);
        m_role_chains_mgr->remove_role(role_chains);
        std::string new_role_string = m_role_chains_mgr->get_roles_string();

        int64_t tm2 = base::xtime_utl::gmttime_ms();

        xsync_kinfo("[xsync_handler] remove_role_phase1 %s cost:%dms", addr.to_string().c_str(), tm2-tm1);

        xchains_wrapper_t& chains_wrapper = role_chains->get_chains_wrapper();
        const map_chain_info_t &chains = chains_wrapper.get_chains();
        for (const auto &it: chains) {
            xevent_ptr_t ev = std::make_shared<mbus::xevent_account_remove_role_t>(it.second.address);
            m_downloader->push_event(ev);
        }

        m_sync_gossip->remove_role(addr);

        int64_t tm3 = base::xtime_utl::gmttime_ms();
        xsync_kinfo("[xsync_handler] remove_role_phase2 %s cost:%dms", addr.to_string().c_str(), tm3-tm2);
        xsync_kinfo("[xsync_handler] remove_role_result(before) %s %s", addr.to_string().c_str(), old_role_string.c_str());
        xsync_kinfo("[xsync_handler] remove_role_result(after) %s %s", addr.to_string().c_str(), new_role_string.c_str());
    }
}

void xsync_handler_t::handle_consensus_result(const mbus::xevent_ptr_t& e) {

    auto ptr = std::static_pointer_cast<mbus::xevent_consensus_data_t>(e);
    xblock_ptr_t block = autoptr_to_blockptr(ptr->vblock_ptr);

    xsync_info("[xsync_handler] on_consensus_block %s", block->dump().c_str());

    vnetwork::xvnode_address_t from_address;
    vnetwork::xvnode_address_t network_self;

    const std::string &address = block->get_account();
    if (!m_role_chains_mgr->exists(address)) {
        xsync_warn("[xsync_handler] on_consensus_block not exist %s", block->get_account().c_str());
        return;
    }

    bool ret = m_role_xips_mgr->is_consensus_role_exist(block.get());
    if (!ret) {
        xsync_dbg("[xsync_handler] on_consensus_block consensus role not exist %s", block->get_account().c_str());
        return;
    }

    ret = m_role_xips_mgr->get_rand_neighbor_by_block(block.get(), network_self, from_address);
    if (!ret) {
        xsync_warn("[xsync_handler] on_consensus_block get rand neighbor failed %s", block->get_account().c_str());
        return;
    }

    m_block_fetcher->handle_consensus_block(block, from_address, network_self);
}

int64_t xsync_handler_t::get_time() {
    return base::xtime_utl::gmttime_ms();
}

void xsync_handler_t::notify_deceit_node(const vnetwork::xvnode_address_t& address) {
    xsync_warn("[xsync_handler] deceit_node %s", address.to_string().c_str());
    m_blacklist->add_deceit_node(address);
    m_role_xips_mgr->remove_xips_by_id(address.node_id());
}

NS_END2
