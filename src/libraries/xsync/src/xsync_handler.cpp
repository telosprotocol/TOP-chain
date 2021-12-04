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
// TODO(jimmy) #include "xbase/xvledger.h"
#include "xblockstore/xblockstore_face.h"
#include "xsync/xsync_util.h"
#include "xsync/xsync_sender.h"
#include "xmbus/xevent_role.h"
#include "xmbus/xevent_executor.h"
#include "xmbus/xevent_blockfetcher.h"
#include "xbasic/xutility.h"
#include "xsyncbase/xmessage_ids.h"
#include "xdata/xblock.h"
#include "xdata/xfull_tableblock.h"
#include "xdata/xtable_bstate.h"

NS_BEG2(top, sync)

using namespace base;
using namespace mbus;
using namespace vnetwork;
using namespace data;

xsync_handler_t::xsync_handler_t(std::string vnode_id,
    xsync_store_face_t *sync_store,
    const observer_ptr<base::xvcertauth_t> &certauth,
    xsession_manager_t *session_mgr, xdeceit_node_manager_t *blacklist,
    xrole_chains_mgr_t *role_chains_mgr, xrole_xips_manager_t *role_xips_mgr,
    xdownloader_t *downloader, xblock_fetcher_t *block_fetcher,
    xsync_gossip_t *sync_gossip,
    xsync_pusher_t *sync_pusher,
    xsync_broadcast_t *sync_broadcast,
    xsync_sender_t *sync_sender,
    xsync_on_demand_t *sync_on_demand,
    xsync_peerset_t *peerset, xsync_peer_keeper_t *peer_keeper, xsync_behind_checker_t *behind_checker,
    xsync_cross_cluster_chain_state_t *cross_cluster_chain_state):
m_vnode_id(vnode_id),
m_sync_store(sync_store),
m_certauth(certauth),
m_session_mgr(session_mgr),
m_blacklist(blacklist),
m_role_chains_mgr(role_chains_mgr),
m_role_xips_mgr(role_xips_mgr),
m_downloader(downloader),
m_block_fetcher(block_fetcher),
m_sync_gossip(sync_gossip),
m_sync_pusher(sync_pusher),
m_sync_broadcast(sync_broadcast),
m_sync_sender(sync_sender),
m_sync_on_demand(sync_on_demand),
m_peerset(peerset),
m_peer_keeper(peer_keeper),
m_behind_checker(behind_checker),
m_cross_cluster_chain_state(cross_cluster_chain_state) {

    register_handler(xmessage_id_sync_get_blocks, std::bind(&xsync_handler_t::get_blocks, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6, std::placeholders::_7));
    register_handler(xmessage_id_sync_blocks, std::bind(&xsync_handler_t::blocks, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6, std::placeholders::_7));
    register_handler(xmessage_id_sync_push_newblock, std::bind(&xsync_handler_t::push_newblock, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6, std::placeholders::_7));
    register_handler(xmessage_id_sync_push_newblockhash, std::bind(&xsync_handler_t::push_newblockhash, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6, std::placeholders::_7));
    register_handler(xmessage_id_sync_broadcast_newblockhash, std::bind(&xsync_handler_t::broadcast_newblockhash, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6, std::placeholders::_7));
    register_handler(xmessage_id_sync_gossip, std::bind(&xsync_handler_t::gossip, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6, std::placeholders::_7));
    register_handler(xmessage_id_sync_frozen_gossip, std::bind(&xsync_handler_t::gossip, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6, std::placeholders::_7));
    register_handler(xmessage_id_sync_get_on_demand_blocks, std::bind(&xsync_handler_t::get_on_demand_blocks, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6, std::placeholders::_7));
    register_handler(xmessage_id_sync_on_demand_blocks, std::bind(&xsync_handler_t::on_demand_blocks, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6, std::placeholders::_7));
    register_handler(xmessage_id_sync_broadcast_chain_state, std::bind(&xsync_handler_t::broadcast_chain_state, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6, std::placeholders::_7));
    register_handler(xmessage_id_sync_frozen_broadcast_chain_state, std::bind(&xsync_handler_t::broadcast_chain_state, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6, std::placeholders::_7));
    register_handler(xmessage_id_sync_response_chain_state, std::bind(&xsync_handler_t::response_chain_state, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6, std::placeholders::_7));
    register_handler(xmessage_id_sync_frozen_response_chain_state, std::bind(&xsync_handler_t::response_chain_state, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6, std::placeholders::_7));
    register_handler(xmessage_id_sync_get_blocks_by_hashes, std::bind(&xsync_handler_t::get_blocks_by_hashes, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6, std::placeholders::_7));
    register_handler(xmessage_id_sync_blocks_by_hashes, std::bind(&xsync_handler_t::blocks_by_hashes, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6, std::placeholders::_7));
    register_handler(xmessage_id_sync_cross_cluster_chain_state, std::bind(&xsync_handler_t::cross_cluster_chain_state, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6, std::placeholders::_7));
    register_handler(xmessage_id_sync_chain_snapshot_request, std::bind(&xsync_handler_t::handle_chain_snapshot_request, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6, std::placeholders::_7));
    register_handler(xmessage_id_sync_chain_snapshot_response, std::bind(&xsync_handler_t::handle_chain_snapshot_response, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6, std::placeholders::_7));
    register_handler(xmessage_id_sync_ondemand_chain_snapshot_request, std::bind(&xsync_handler_t::handle_ondemand_chain_snapshot_request, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6, std::placeholders::_7));
    register_handler(xmessage_id_sync_ondemand_chain_snapshot_response, std::bind(&xsync_handler_t::handle_ondemand_chain_snapshot_response, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6, std::placeholders::_7));
    register_handler(xmessage_id_sync_get_on_demand_by_hash_blocks, std::bind(&xsync_handler_t::get_on_demand_by_hash_blocks, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6, std::placeholders::_7));
    register_handler(xmessage_id_sync_on_demand_by_hash_blocks, std::bind(&xsync_handler_t::on_demand_by_hash_blocks, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6, std::placeholders::_7));
    register_handler(xmessage_id_sync_get_on_demand_blocks_with_proof, std::bind(&xsync_handler_t::get_on_demand_blocks_with_proof, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6, std::placeholders::_7));
    register_handler(xmessage_id_sync_on_demand_blocks_with_proof, std::bind(&xsync_handler_t::on_demand_blocks_with_proof, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6, std::placeholders::_7));
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
        xsync_warn("xsync_handler on_message decode failed %" PRIx64 " %s", msg_hash, from_address.to_string().c_str());
        return;
    }

    auto it = m_handlers.find(msg_type);
    if (it != m_handlers.end()) {
        xsync_handler_netmsg_callback cb = it->second;
        cb(msg_size, from_address, network_self, header, stream, msg_hash, recv_time);
    } else {
        xsync_warn("xsync_handler on_message(unrecognized) %" PRIx64 " %s", msg_hash, from_address.to_string().c_str());
    }
}

void xsync_handler_t::on_event(const mbus::xevent_ptr_t& e) {

    m_downloader->push_event(e);

    if (e->major_type == mbus::xevent_major_type_timer) {
        m_sync_gossip->on_timer();
        m_behind_checker->on_timer();
        m_peer_keeper->on_timer();
        m_cross_cluster_chain_state->on_timer();
    }

    if (e->major_type == mbus::xevent_major_type_chain_timer) {
        m_sync_gossip->on_chain_timer(e);
    }

    if (e->major_type==mbus::xevent_major_type_behind) {

        if (e->minor_type == mbus::xevent_behind_t::type_check)
            m_behind_checker->on_behind_check_event(e);
        else if (e->minor_type == mbus::xevent_behind_t::type_on_demand)
            m_sync_on_demand->on_behind_event(e);
        else if (e->minor_type == mbus::xevent_behind_t::type_on_demand_by_hash)
            m_sync_on_demand->on_behind_by_hash_event(e);
    }

    if (e->major_type == mbus::xevent_major_type_role) {
        handle_role_change(e);
    }

    if (e->major_type == mbus::xevent_major_type_consensus) {
        handle_consensus_result(e);
    }
}

void xsync_handler_t::get_blocks(uint32_t msg_size, const vnetwork::xvnode_address_t &from_address,
    const vnetwork::xvnode_address_t &network_self,
    const xsync_message_header_ptr_t &header,
    base::xstream_t &stream,
    xtop_vnetwork_message::hash_result_type msg_hash,
    int64_t recv_time) {
    
    XMETRICS_GAUGE(metrics::xsync_getblocks_recv_req, 1);
    auto ptr = make_object_ptr<xsync_message_get_blocks_t>();
    ptr->serialize_from(stream);

    const std::string &owner = ptr->owner;
    uint64_t start_height = ptr->start_height;
    uint32_t count = ptr->count;

    if (owner == "" || start_height == 0 || count == 0)
        return;

    xsync_info("xsync_handler receive getblocks %" PRIx64 " wait(%ldms) %s range[%lu,%lu] %s",
        msg_hash, get_time()-recv_time, owner.c_str(), start_height, start_height+count-1, from_address.to_string().c_str());

    std::vector<xblock_ptr_t> vector_blocks;
    for (uint32_t height = start_height, i = 0; height < start_height + count && i < max_request_block_count; height++) {
        auto blocks = m_sync_store->load_block_objects(owner, height);
        if (blocks.empty()) {
            break;
        }
        for (uint32_t j = 0; j < blocks.size(); j++,i++){
            vector_blocks.push_back(xblock_t::raw_vblock_to_object_ptr(blocks[j].get()));
        }
    }
    XMETRICS_GAUGE(metrics::xsync_getblocks_send_resp, vector_blocks.size());
    m_sync_sender->send_blocks(xsync_msg_err_code_t::succ, owner, vector_blocks, network_self, from_address);
}

void xsync_handler_t::push_newblock(uint32_t msg_size,
        const vnetwork::xvnode_address_t &from_address,
        const vnetwork::xvnode_address_t &network_self,
        const xsync_message_header_ptr_t &header,
        base::xstream_t &stream,
        xtop_vnetwork_message::hash_result_type msg_hash,
        int64_t recv_time) {

    XMETRICS_GAUGE(metrics::xsync_recv_new_block, 1);
    XMETRICS_GAUGE(metrics::xsync_recv_block_size, msg_size);

    auto ptr = make_object_ptr<xsync_message_push_newblock_t>();
    ptr->serialize_from(stream);

    if (ptr->block == nullptr)
        return;

    xblock_ptr_t &block = ptr->block;

    if (!common::has<common::xnode_type_t::storage>(network_self.type())) {
        xsync_warn("xsync_handler receive push_newblock(target must be archive) %" PRIx64 " %s %s %s",
            msg_hash, block->dump().c_str(), network_self.to_string().c_str(), from_address.to_string().c_str());
        XMETRICS_GAUGE(metrics::xsync_recv_invalid_block, 1);
        return;
    }

    if (!common::has<common::xnode_type_t::rec>(from_address.type()) && !common::has<common::xnode_type_t::zec>(from_address.type()) &&
        !common::has<common::xnode_type_t::consensus>(from_address.type())) {
        xsync_warn("xsync_handler receive push_newblock(source must be consensus) %" PRIx64 " %s %s %s",
            msg_hash, block->dump().c_str(), network_self.to_string().c_str(), from_address.to_string().c_str());
        XMETRICS_GAUGE(metrics::xsync_recv_invalid_block, 1);
        return;
    }

    const std::string &address = block->get_account();
    if (!m_role_chains_mgr->exists(address)) {
        xsync_warn("xsync_handler receive push_newblock(no role) %" PRIx64 " %s %s", msg_hash, block->dump().c_str(), from_address.to_string().c_str());
        XMETRICS_GAUGE(metrics::xsync_recv_invalid_block, 1);
        return;
    }

    // to be deleted
    // check block existed already
    auto exist_block = m_sync_store->existed(block->get_account(), block->get_height(), block->get_viewid());
    if (exist_block) {
        XMETRICS_GAUGE(metrics::xsync_recv_duplicate_block, 1);
        return;
    }

    if (!check_auth(m_certauth, block)) {
        xsync_warn("xsync_handler receive push_newblock(auth failed) %" PRIx64 " %s %s", msg_hash, block->dump().c_str(), from_address.to_string().c_str());
        XMETRICS_GAUGE(metrics::xsync_recv_invalid_block, 1);
        return;
    }

    xsync_info("xsync_handler receive push_newblock %" PRIx64 " %s %s", msg_hash, block->dump().c_str(), from_address.to_string().c_str());

    mbus::xevent_ptr_t ev = make_object_ptr<mbus::xevent_blockfetcher_block_t>(block, network_self, from_address);
    m_block_fetcher->push_event(ev);
}

void xsync_handler_t::push_newblockhash(uint32_t msg_size,
        const vnetwork::xvnode_address_t &from_address,
        const vnetwork::xvnode_address_t &network_self,
        const xsync_message_header_ptr_t &header,
        base::xstream_t &stream,
        xtop_vnetwork_message::hash_result_type msg_hash,
        int64_t recv_time) {

    XMETRICS_GAUGE(metrics::xsync_recv_new_hash, 1);
    XMETRICS_GAUGE(metrics::xsync_recv_block_size, msg_size);

    auto ptr = make_object_ptr<xsync_message_general_newblockhash_t>();
    ptr->serialize_from(stream);

    const std::string &address = ptr->address;
    uint64_t height = ptr->height;
    const std::string &hash = ptr->hash;

    if (address == "" || height == 0 || hash == "")
        return;

    if (!common::has<common::xnode_type_t::storage>(network_self.type())) {
        xsync_warn("xsync_handler receive push_newblockhash(target must be archive) %" PRIx64 " %s,height=%lu, %s %s",
            msg_hash, address.c_str(), height, network_self.to_string().c_str(), from_address.to_string().c_str());
        return;
    }

    if (!common::has<common::xnode_type_t::rec>(from_address.type()) && !common::has<common::xnode_type_t::zec>(from_address.type()) &&
        !common::has<common::xnode_type_t::consensus>(from_address.type())) {
        xsync_warn("xsync_handler receive push_newblockhash(target must be consensus) %" PRIx64 " %s,height=%lu, %s %s",
            msg_hash, address.c_str(), height, network_self.to_string().c_str(), from_address.to_string().c_str());
        return;
    }

    xsync_dbg("xsync_handler receive push_newblockhash %" PRIx64 " wait(%ldms) %s,height=%lu %s,",
        msg_hash, get_time()-recv_time, address.c_str(), height, from_address.to_string().c_str());

    mbus::xevent_ptr_t ev = make_object_ptr<mbus::xevent_blockfetcher_blockhash_t>(address, height, hash, network_self, from_address);
    m_block_fetcher->push_event(ev);
}

void xsync_handler_t::broadcast_newblockhash(uint32_t msg_size,
        const vnetwork::xvnode_address_t &from_address,
        const vnetwork::xvnode_address_t &network_self,
        const xsync_message_header_ptr_t &header,
        base::xstream_t &stream,
        xtop_vnetwork_message::hash_result_type msg_hash,
        int64_t recv_time) {

    XMETRICS_GAUGE(metrics::xsync_recv_broadcast_newblockhash, 1);
    XMETRICS_GAUGE(metrics::xsync_recv_block_size, msg_size);

    auto ptr = make_object_ptr<xsync_message_general_newblockhash_t>();
    ptr->serialize_from(stream);

    const std::string &address = ptr->address;
    uint64_t height = ptr->height;
    const std::string &hash = ptr->hash;

    if (address == "" || height == 0 || hash == "")
        return;

    if (!common::has<common::xnode_type_t::storage_archive>(network_self.type())) {
        xsync_warn("xsync_handler receive broadcast_newblockhash(target must be archive) %" PRIx64 " %s,height=%lu, %s %s",
            msg_hash, address.c_str(), height, network_self.to_string().c_str(), from_address.to_string().c_str());
        return;
    }

    if (!common::has<common::xnode_type_t::storage_archive>(from_address.type())) {
        xsync_warn("xsync_handler receive broadcast_newblockhash(source must be archive) %" PRIx64 " %s,height=%lu, %s %s",
            msg_hash, address.c_str(), height, network_self.to_string().c_str(), from_address.to_string().c_str());
        return;
    }

    xsync_info("xsync_handler receive broadcast_newblockhash %" PRIx64 " wait(%ldms) %s,height=%lu %s,",
        msg_hash, get_time()-recv_time, address.c_str(), height, from_address.to_string().c_str());

    mbus::xevent_ptr_t ev = make_object_ptr<mbus::xevent_blockfetcher_blockhash_t>(address, height, hash, network_self, from_address);
    m_block_fetcher->push_event(ev);
}

void xsync_handler_t::blocks(uint32_t msg_size, const vnetwork::xvnode_address_t &from_address,
    const vnetwork::xvnode_address_t &network_self,
    const xsync_message_header_ptr_t &header,
    base::xstream_t &stream,
    xtop_vnetwork_message::hash_result_type msg_hash,
    int64_t recv_time) {

    XMETRICS_GAUGE(metrics::xsync_recv_blocks, 1);
    XMETRICS_GAUGE(metrics::xsync_recv_blocks_size, msg_size);

    auto ptr = make_object_ptr<xsync_message_blocks_t>();
    ptr->serialize_from(stream);

    std::vector<data::xblock_ptr_t> &blocks = ptr->blocks;

    uint32_t count = blocks.size();

    if (count == 0) {
        xsync_info("xsync_handler receive blocks %" PRIx64 " wait(%ldms) count(%u) code(%u) %s",
            msg_hash, get_time()-recv_time, count, header->code, from_address.to_string().c_str());

        return;
    }

    XMETRICS_GAUGE(metrics::xsync_handler_blocks, count);

    xsync_info("xsync_handler receive blocks %" PRIx64 " wait(%ldms) %s count(%u) code(%u) %s",
        msg_hash, get_time()-recv_time, blocks[0]->get_account().c_str(), count, header->code, from_address.to_string().c_str());

    // check continuous
    xvblock_t *successor = nullptr;
    std::vector<data::xblock_ptr_t>::reverse_iterator rit = blocks.rbegin();
    for (;rit!=blocks.rend(); rit++) {
        xblock_ptr_t &block = *rit;

        if (successor != nullptr) {

            if (block->get_account() != successor->get_account()) {
                xsync_warn("xsync_handler receive blocks(address error) (%s, %s)",
                    block->get_account().c_str(), successor->get_account().c_str());
                return;
            }
        }

        successor = block.get();
    }

    if (!data::is_table_address(common::xaccount_address_t{successor->get_account()}) && !data::is_drand_address(common::xaccount_address_t{successor->get_account()})) {
        xsync_dbg("xsync_handler_t::blocks, address type error: %s", successor->get_account().c_str());
        return;
    }

    mbus::xevent_ptr_t e = make_object_ptr<mbus::xevent_sync_response_blocks_t>(blocks, network_self, from_address);
    m_downloader->push_event(e);
}

void xsync_handler_t::gossip(uint32_t msg_size, const vnetwork::xvnode_address_t &from_address,
    const vnetwork::xvnode_address_t &network_self,
    const xsync_message_header_ptr_t &header,
    base::xstream_t &stream,
    xtop_vnetwork_message::hash_result_type msg_hash,
    int64_t recv_time) {

    XMETRICS_GAUGE(metrics::xsync_recv_gossip_recv, 1);
    XMETRICS_GAUGE(metrics::xsync_bytes_gossip_recv, msg_size);

    auto ptr = make_object_ptr<xsync_message_gossip_t>();
    ptr->serialize_from(stream);

    std::vector<xgossip_chain_info_ptr_t> &info_list = ptr->info_list;
    std::map<std::string, xgossip_behind_info_t> behind_chain_set;

    m_sync_gossip->handle_message(info_list, from_address, network_self, behind_chain_set);

    std::string reason = "gossip";

    for (auto &it: behind_chain_set) {
        const std::string &address = it.first;
        const xgossip_behind_info_t &info = it.second;
        if (info.local_height == info.peer_height)
            continue;

        mbus::xevent_ptr_t ev = make_object_ptr<mbus::xevent_behind_download_t>(address, 0u, info.peer_height, enum_chain_sync_policy_full, network_self, from_address, reason);
        m_downloader->push_event(ev);
    }
}

void xsync_handler_t::get_on_demand_blocks(uint32_t msg_size, const vnetwork::xvnode_address_t &from_address,
    const vnetwork::xvnode_address_t &network_self,
    const xsync_message_header_ptr_t &header,
    base::xstream_t &stream,
    xtop_vnetwork_message::hash_result_type msg_hash,
    int64_t recv_time) {

    if (m_sync_store->remove_empty_unit_forked()) {
        xwarn("xsync_handler_t::get_on_demand_blocks forked but recv old on demand request, drop it!");
        return;
    }
    XMETRICS_GAUGE(metrics::xsync_recv_get_on_demand_blocks, 1);
    XMETRICS_GAUGE(metrics::xsync_recv_get_on_demand_blocks_bytes, msg_size);

    auto ptr = make_object_ptr<xsync_message_get_on_demand_blocks_t>();
    auto len = ptr->serialize_from(stream);
    if (len <= 0) {
        xerror("xsync_handler_t::get_on_demand_blocks deserialize fail");
        return;
    }

   m_sync_on_demand->handle_blocks_request(*(ptr.get()), from_address, network_self);
}

void xsync_handler_t::get_on_demand_blocks_with_proof(uint32_t msg_size, const vnetwork::xvnode_address_t &from_address,
    const vnetwork::xvnode_address_t &network_self,
    const xsync_message_header_ptr_t &header,
    base::xstream_t &stream,
    xtop_vnetwork_message::hash_result_type msg_hash,
    int64_t recv_time) {

    if (!m_sync_store->remove_empty_unit_forked()) {
        xwarn("xsync_handler_t::get_on_demand_blocks_with_proof not forked but recv new on demand request, drop it!");
        return;
    }

    XMETRICS_GAUGE(metrics::xsync_recv_get_on_demand_blocks, 1);
    XMETRICS_GAUGE(metrics::xsync_recv_get_on_demand_blocks_bytes, msg_size);

    auto ptr = make_object_ptr<xsync_message_get_on_demand_blocks_with_proof_t>();
    auto len = ptr->serialize_from(stream);
    if (len <= 0) {
        xerror("xsync_handler_t::get_on_demand_blocks_with_proof deserialize fail");
        return;
    }

   m_sync_on_demand->handle_blocks_request_with_proof(*(ptr.get()), from_address, network_self);
}

void xsync_handler_t::on_demand_blocks(uint32_t msg_size, const vnetwork::xvnode_address_t &from_address,
    const vnetwork::xvnode_address_t &network_self,
    const xsync_message_header_ptr_t &header,
    base::xstream_t &stream,
    xtop_vnetwork_message::hash_result_type msg_hash,
    int64_t recv_time) {

    if (m_sync_store->remove_empty_unit_forked()) {
        xwarn("xsync_handler_t::on_demand_blocks forked but recv old on demand response, drop it!");
        return;
    }

    XMETRICS_GAUGE(metrics::xsync_recv_on_demand_blocks, 1);
    XMETRICS_GAUGE(metrics::xsync_recv_on_demand_blocks_bytes, msg_size);

    auto ptr = make_object_ptr<xsync_message_general_blocks_t>();
    auto len = ptr->serialize_from(stream);
    if (len <= 0) {
        xerror("xsync_handler_t::on_demand_blocks deserialize fail");
        return;
    }

    std::vector<xblock_ptr_t> &blocks = ptr->blocks;
    if (blocks.size() == 0)
        return;

    m_sync_on_demand->handle_blocks_response(blocks, from_address, network_self);
}

void xsync_handler_t::on_demand_blocks_with_proof(uint32_t msg_size, const vnetwork::xvnode_address_t &from_address,
    const vnetwork::xvnode_address_t &network_self,
    const xsync_message_header_ptr_t &header,
    base::xstream_t &stream,
    xtop_vnetwork_message::hash_result_type msg_hash,
    int64_t recv_time) {

    if (!m_sync_store->remove_empty_unit_forked()) {
        xwarn("xsync_handler_t::on_demand_blocks_with_proof not forked but recv new on demand response, drop it!");
        return;
    }

    XMETRICS_GAUGE(metrics::xsync_recv_on_demand_blocks, 1);
    XMETRICS_GAUGE(metrics::xsync_recv_on_demand_blocks_bytes, msg_size);

    auto ptr = make_object_ptr<xsync_message_general_blocks_with_proof_t>();
    auto len = ptr->serialize_from(stream);
    if (len <= 0) {
        xerror("xsync_handler_t::on_demand_blocks_with_proof deserialize fail");
        return;
    }

    std::vector<xblock_ptr_t> &blocks = ptr->blocks;
    if (blocks.size() == 0)
        return;
    std::string unit_proof_str = ptr->unit_proof_str;

    m_sync_on_demand->handle_blocks_response_with_proof(blocks, unit_proof_str, from_address, network_self);
}

void xsync_handler_t::broadcast_chain_state(uint32_t msg_size, const vnetwork::xvnode_address_t &from_address,
    const vnetwork::xvnode_address_t &network_self,
    const xsync_message_header_ptr_t &header,
    base::xstream_t &stream,
    xtop_vnetwork_message::hash_result_type msg_hash,
    int64_t recv_time) {

    XMETRICS_GAUGE(metrics::xsync_recv_broadcast_chain_state, 1);
    XMETRICS_GAUGE(metrics::xsync_recv_broadcast_chain_state_bytes, msg_size);

    auto ptr = make_object_ptr<xsync_message_chain_state_info_t>();
    ptr->serialize_from(stream);

    std::vector<xchain_state_info_t> &info_list = ptr->info_list;

#if 0
    // TODO if consensus node build peers with archives?
    if (network_self.type() != from_address.type())
        return;
#endif
    xsync_info("xsync_handler receive broadcast_chain_state %" PRIx64 " wait(%ldms) count:%u %s",
        msg_hash, get_time()-recv_time, (uint32_t)info_list.size(), from_address.to_string().c_str());

    if (info_list.size() > 500) {
        return;
    }

    // 1. update local peers
    m_peer_keeper->handle_message(network_self, from_address, info_list);

    std::shared_ptr<xrole_chains_t> role_chains = m_role_chains_mgr->get_role(network_self);
    if (role_chains == nullptr) {
        xsync_dbg("xsync_handler broadcast_chain_state network address %s is not exist", network_self.to_string().c_str());
        return;
    }

    const map_chain_info_t &chains = role_chains->get_chains_wrapper().get_chains();

    std::vector<xchain_state_info_t> rsp_info_list;
    // 2. response to peer
    for (auto &it: info_list) {

        const std::string &address = it.address;

        auto it2 = chains.find(address);
        if (it2 == chains.end()) {
            xsync_dbg("xsync_handler broadcast_chain_state chain address %s is not exist", address.c_str());
            continue;
        }

        const xchain_info_t &chain_info = it2->second;

        xchain_state_info_t info;
        info.address = address;
        if (chain_info.sync_policy == enum_chain_sync_policy_fast) {
            base::xauto_ptr<base::xvblock_t> latest_start_block = m_sync_store->get_latest_start_block(address, chain_info.sync_policy);
            xblock_ptr_t block = autoptr_to_blockptr(latest_start_block);
            if (!block->is_full_state_block()) {
                info.start_height = 0;
                info.end_height = 0;
            } else {
                info.start_height = latest_start_block->get_height();
                info.end_height = m_sync_store->get_latest_end_block_height(address, chain_info.sync_policy);
            }
        } else {
            info.start_height = m_sync_store->get_latest_start_block_height(address, chain_info.sync_policy);
            info.end_height = m_sync_store->get_latest_end_block_height(address, chain_info.sync_policy);
        }
        rsp_info_list.push_back(info);
    }

    if (rsp_info_list.size() != 0) {
        if (common::has<common::xnode_type_t::frozen>(network_self.type())) {
            m_sync_sender->send_frozen_response_chain_state(rsp_info_list, network_self, from_address);
        } else {
            m_sync_sender->send_response_chain_state(rsp_info_list, network_self, from_address);
        }
    }
}

void xsync_handler_t::response_chain_state(uint32_t msg_size, const vnetwork::xvnode_address_t &from_address,
    const vnetwork::xvnode_address_t &network_self,
    const xsync_message_header_ptr_t &header,
    base::xstream_t &stream,
    xtop_vnetwork_message::hash_result_type msg_hash,
    int64_t recv_time) {

    XMETRICS_GAUGE(metrics::xsync_recv_response_chain_state, 1);
    XMETRICS_GAUGE(metrics::xsync_recv_response_chain_state_bytes, msg_size);

    auto ptr = make_object_ptr<xsync_message_chain_state_info_t>();
    ptr->serialize_from(stream);

    std::vector<xchain_state_info_t> &info_list = ptr->info_list;

    xsync_info("xsync_handler receive response_chain_state %" PRIx64 " wait(%ldms) count:%u %s",
        msg_hash, get_time()-recv_time, (uint32_t)info_list.size(), from_address.to_string().c_str());

    m_peer_keeper->handle_message(network_self, from_address, info_list);
}

void xsync_handler_t::cross_cluster_chain_state(uint32_t msg_size, const vnetwork::xvnode_address_t &from_address,
    const vnetwork::xvnode_address_t &network_self,
    const xsync_message_header_ptr_t &header,
    base::xstream_t &stream,
    xtop_vnetwork_message::hash_result_type msg_hash,
    int64_t recv_time) {

    XMETRICS_GAUGE(metrics::xsync_recv_cross_cluster_chain_state, 1);
    XMETRICS_GAUGE(metrics::xsync_recv_cross_cluster_chain_state_bytes, msg_size);

    auto ptr = make_object_ptr<xsync_message_chain_state_info_t>();
    ptr->serialize_from(stream);
    std::vector<xchain_state_info_t> &info_list = ptr->info_list;

    if (!common::has<common::xnode_type_t::storage>(network_self.type())) {
        xsync_warn("xsync_handler receive cross_cluster_chain_state(target must be archive or full node) %" PRIx64 " count(%u), %s %s",
            msg_hash, info_list.size(), network_self.to_string().c_str(), from_address.to_string().c_str());
        return;
    }

    if (!common::has<common::xnode_type_t::rec>(from_address.type()) && !common::has<common::xnode_type_t::zec>(from_address.type()) &&
        !common::has<common::xnode_type_t::consensus>(from_address.type()) && !common::has<common::xnode_type_t::storage_archive>(from_address.type())) {
        xsync_warn("xsync_handler receive cross_cluster_chain_state(source must be consensus) %" PRIx64 " count(%u) %s %s",
            msg_hash, info_list.size(), network_self.to_string().c_str(), from_address.to_string().c_str());
        return;
    }

    xsync_info("xsync_handler receive cross_cluster_chain_state %" PRIx64 " wait(%ldms) count:%u %s",
        msg_hash, get_time()-recv_time, (uint32_t)info_list.size(), from_address.to_string().c_str());

    if (info_list.size() > 500) {
        return;
    }

    m_cross_cluster_chain_state->handle_message(network_self, from_address, info_list);
}

void xsync_handler_t::get_blocks_by_hashes(uint32_t msg_size,
        const vnetwork::xvnode_address_t &from_address,
        const vnetwork::xvnode_address_t &network_self,
        const xsync_message_header_ptr_t &header,
        base::xstream_t &stream,
        xtop_vnetwork_message::hash_result_type msg_hash,
        int64_t recv_time) {

    XMETRICS_GAUGE(metrics::xsync_recv_get_blocks_by_hashes, 1);
    XMETRICS_GAUGE(metrics::xsync_recv_get_blocks_by_hashes_bytes, msg_size);

    auto ptr = make_object_ptr<xsync_message_get_blocks_by_hashes_t>();
    ptr->serialize_from(stream);

    std::vector<xblock_hash_t> &info_list = ptr->info_list;

    if (info_list.size() == 0)
        return;

    xsync_info("xsync_handler receive get_blocks_by_hashes %" PRIx64 " wait(%ldms) count:%u %s",
        msg_hash, get_time()-recv_time, info_list.size(), from_address.to_string().c_str());

    std::vector<xblock_ptr_t> vector_blocks;

    for (auto &it: info_list) {
        xblock_hash_t &info = it;
        base::xauto_ptr<base::xvblock_t> vblock = m_sync_store->query_block(info.address, info.height, info.hash);
        if (vblock != nullptr) {
            xblock_ptr_t block_ptr = autoptr_to_blockptr(vblock);
            vector_blocks.push_back(block_ptr);
            xsync_info("xsync_handler receive get_blocks_by_hashes and success to query store, block hash is %s wait(%ldms), from address:%s",
                data::to_hex_str(info.hash).c_str(), get_time()-recv_time, from_address.to_string().c_str());
        } else {
            xsync_info("xsync_handler receive get_blocks_by_hashes and fail to query store, block hash is %s wait(%ldms), from address:%s",
                data::to_hex_str(info.hash).c_str(), get_time()-recv_time, from_address.to_string().c_str());
        }
    }

    if (!vector_blocks.empty()) {
        m_sync_sender->send_blocks_by_hashes(vector_blocks, network_self, from_address);
    }
}

void xsync_handler_t::blocks_by_hashes(uint32_t msg_size,
    const vnetwork::xvnode_address_t &from_address,
    const vnetwork::xvnode_address_t &network_self,
    const xsync_message_header_ptr_t &header,
    base::xstream_t &stream,
    xtop_vnetwork_message::hash_result_type msg_hash,
    int64_t recv_time) {

    XMETRICS_GAUGE(metrics::xsync_recv_blocks_by_hashes, 1);
    XMETRICS_GAUGE(metrics::xsync_recv_blocks_by_hashes_bytes, msg_size);

    auto ptr = make_object_ptr<xsync_message_general_blocks_t>();
    ptr->serialize_from(stream);

    std::vector<data::xblock_ptr_t> &blocks = ptr->blocks;

    uint32_t count = blocks.size();

    if (count == 0) {
        xsync_info("xsync_handler receive blocks_by_hashes %" PRIx64 " wait(%ldms) count(%u) code(%u) %s",
            msg_hash, get_time()-recv_time, count, header->code, from_address.to_string().c_str());

        return;
    }

    XMETRICS_GAUGE(metrics::xsync_handler_blocks_by_hashes, count);

    xsync_info("xsync_handler receive blocks_by_hashes %" PRIx64 " wait(%ldms) count(%u) code(%u) %s",
        msg_hash, get_time()-recv_time, count, header->code, from_address.to_string().c_str());

    mbus::xevent_ptr_t e = make_object_ptr<mbus::xevent_sync_response_blocks_t>(blocks, network_self, from_address);
    m_block_fetcher->push_event(e);
}

void xsync_handler_t::handle_role_change(const mbus::xevent_ptr_t& e) {
    if (e->minor_type == xevent_role_t::add_role) {

        auto bme = dynamic_xobject_ptr_cast<mbus::xevent_role_add_t>(e);
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

        std::set<uint16_t> set_table_ids;
        for (auto &id: table_ids)
            set_table_ids.insert(id);

        m_role_xips_mgr->add_role(addr, neighbor_addresses, parent_addresses,
            vnetwork_driver->archive_addresses(common::xnode_type_t::storage_archive),
            vnetwork_driver->archive_addresses(common::xnode_type_t::storage_full_node), set_table_ids);

        XMETRICS_GAUGE(metrics::xsync_cost_role_add_event, 1);

        int64_t tm1 = base::xtime_utl::gmttime_ms();

        std::string old_role_string = m_role_chains_mgr->get_roles_string();
        std::shared_ptr<xrole_chains_t> role_chains = std::make_shared<xrole_chains_t>(addr, set_table_ids);
        m_role_chains_mgr->add_role(role_chains);
        std::string new_role_string = m_role_chains_mgr->get_roles_string();

        int64_t tm2 = base::xtime_utl::gmttime_ms();

        xsync_kinfo("xsync_handler add_role_phase1 %s cost:%dms", addr.to_string().c_str(), tm2-tm1);

        xchains_wrapper_t& chains_wrapper = role_chains->get_chains_wrapper();
        const map_chain_info_t &chains = chains_wrapper.get_chains();
        for (const auto &it: chains) {
            xevent_ptr_t ev = make_object_ptr<mbus::xevent_account_add_role_t>(it.second.address);
            m_downloader->push_event(ev);
            m_block_fetcher->push_event(ev);
        }

        m_sync_gossip->add_role(addr);
        m_peer_keeper->add_role(addr);

        int64_t tm3 = base::xtime_utl::gmttime_ms();
        xsync_kinfo("xsync_handler add_role_phase2 %s cost:%dms", addr.to_string().c_str(), tm3-tm2);
        xsync_kinfo("xsync_handler add_role_result(before) %s %s", addr.to_string().c_str(), old_role_string.c_str());
        xsync_kinfo("xsync_handler add_role_result(after) %s %s", addr.to_string().c_str(), new_role_string.c_str());

    } else if (e->minor_type == xevent_role_t::remove_role) {
        auto bme = dynamic_xobject_ptr_cast<mbus::xevent_role_remove_t>(e);
        std::shared_ptr<vnetwork::xvnetwork_driver_face_t> &vnetwork_driver = bme->m_vnetwork_driver;

        vnetwork::xvnode_address_t addr = vnetwork_driver->address();
        std::vector<uint16_t> table_ids = vnetwork_driver->table_ids();
        std::set<uint16_t> set_table_ids;
        for (auto &id: table_ids)
            set_table_ids.insert(id);

        m_role_xips_mgr->remove_role(addr);

        XMETRICS_GAUGE(metrics::xsync_cost_role_remove_event, 1);

        int64_t tm1 = base::xtime_utl::gmttime_ms();

        std::string old_role_string = m_role_chains_mgr->get_roles_string();
        std::shared_ptr<xrole_chains_t> role_chains = std::make_shared<xrole_chains_t>(addr, set_table_ids);
        m_role_chains_mgr->remove_role(role_chains);
        std::string new_role_string = m_role_chains_mgr->get_roles_string();

        int64_t tm2 = base::xtime_utl::gmttime_ms();

        xsync_kinfo("xsync_handler remove_role_phase1 %s cost:%dms", addr.to_string().c_str(), tm2-tm1);

        xchains_wrapper_t& chains_wrapper = role_chains->get_chains_wrapper();
        const map_chain_info_t &chains = chains_wrapper.get_chains();
        for (const auto &it: chains) {
            xevent_ptr_t ev = make_object_ptr<mbus::xevent_account_remove_role_t>(it.second.address);
            m_downloader->push_event(ev);
            m_block_fetcher->push_event(ev);
        }

        m_sync_gossip->remove_role(addr);
        m_peer_keeper->remove_role(addr);

        int64_t tm3 = base::xtime_utl::gmttime_ms();
        xsync_kinfo("xsync_handler remove_role_phase2 %s cost:%dms", addr.to_string().c_str(), tm3-tm2);
        xsync_kinfo("xsync_handler remove_role_result(before) %s %s", addr.to_string().c_str(), old_role_string.c_str());
        xsync_kinfo("xsync_handler remove_role_result(after) %s %s", addr.to_string().c_str(), new_role_string.c_str());
    }
}

void xsync_handler_t::handle_consensus_result(const mbus::xevent_ptr_t& e) {

    auto ptr = dynamic_xobject_ptr_cast<mbus::xevent_consensus_data_t>(e);
    xblock_ptr_t block = autoptr_to_blockptr(ptr->vblock_ptr);

    const std::string &address = block->get_account();
    if (!m_role_chains_mgr->exists(address)) {
        xsync_warn("xsync_handler on_consensus_block role not exist %s", block->get_account().c_str());
        return;
    }

    xsync_info("xsync_handler on_consensus_block %s", block->dump().c_str());

    m_sync_pusher->push_newblock_to_archive(block);
}

void xsync_handler_t::handle_chain_snapshot_request(
    uint32_t msg_size, const vnetwork::xvnode_address_t &from_address,
    const vnetwork::xvnode_address_t &network_self,
    const xsync_message_header_ptr_t &header,
    base::xstream_t &stream,
    xtop_vnetwork_message::hash_result_type msg_hash,
    int64_t recv_time) {

    auto ptr = make_object_ptr<xsync_message_chain_snapshot_meta_t>();
    ptr->serialize_from(stream);

    XMETRICS_GAUGE(metrics::xsync_handle_chain_snapshot_request, 1);
    xsync_info("xsync_handler receive chain_snapshot_request %" PRIx64 " wait(%ldms) %s, account %s, height %llu",
        msg_hash, get_time()-recv_time, from_address.to_string().c_str(), ptr->m_account_addr.c_str(), ptr->m_height_of_fullblock);
    base::xauto_ptr<base::xvblock_t> blk = m_sync_store->load_block_object(ptr->m_account_addr, ptr->m_height_of_fullblock, false);
    if (blk != nullptr) {
        if (blk->get_block_level() == base::enum_xvblock_level_table && blk->get_block_class() == base::enum_xvblock_class_full) {
            // it must be full-table block now
            if (base::xvchain_t::instance().get_xstatestore()->get_blkstate_store()->get_full_block_offsnapshot(blk.get(), metrics::statestore_access_from_sync_chain_snapshot)) {
                std::string property_snapshot = blk->get_full_state();
                xsync_message_chain_snapshot_t chain_snapshot(ptr->m_account_addr,
                    property_snapshot, ptr->m_height_of_fullblock);
                m_sync_sender->send_chain_snapshot(chain_snapshot, xmessage_id_sync_chain_snapshot_response, network_self, from_address);
            } else {
                xsync_warn("xsync_handler receive chain_snapshot_request, and the full block state is not exist,account:%s, height:%llu, block_type:%d",
                    ptr->m_account_addr.c_str(), ptr->m_height_of_fullblock, blk->get_block_class());
            }
        } else {
            xsync_error("xsync_handler receive chain_snapshot_request, and it is not full table,account:%s, height:%llu",
                    ptr->m_account_addr.c_str(), ptr->m_height_of_fullblock);
        }
    } else {
        xsync_info("xsync_handler receive chain_snapshot_request, and the full block is not exist,account:%s, height:%llu",
                ptr->m_account_addr.c_str(), ptr->m_height_of_fullblock);
    }
}

void xsync_handler_t::handle_chain_snapshot_response(uint32_t msg_size, const vnetwork::xvnode_address_t &from_address,
    const vnetwork::xvnode_address_t &network_self,
    const xsync_message_header_ptr_t &header,
    base::xstream_t &stream,
    xtop_vnetwork_message::hash_result_type msg_hash,
    int64_t recv_time) {

    auto ptr = make_object_ptr<xsync_message_chain_snapshot_t>();
    ptr->serialize_from(stream);

    xsync_info("xsync_handler chain snapshot reponse %" PRIx64 " wait(%ldms) %s, account %s, height %llu",
        msg_hash, get_time()-recv_time, from_address.to_string().c_str(), ptr->m_tbl_account_addr.c_str(), ptr->m_height_of_fullblock);

    XMETRICS_GAUGE(metrics::xsync_handler_chain_snapshot_reponse, 1);

    mbus::xevent_ptr_t e = make_object_ptr<mbus::xevent_chain_snaphsot_t>(ptr->m_tbl_account_addr, ptr->m_chain_snapshot, ptr->m_height_of_fullblock, network_self, from_address);
    m_downloader->push_event(e);
}

void xsync_handler_t::handle_ondemand_chain_snapshot_request(
    uint32_t msg_size, const vnetwork::xvnode_address_t &from_address,
    const vnetwork::xvnode_address_t &network_self,
    const xsync_message_header_ptr_t &header,
    base::xstream_t &stream,
    xtop_vnetwork_message::hash_result_type msg_hash,
    int64_t recv_time) {

    auto ptr = make_object_ptr<xsync_message_chain_snapshot_meta_t>();
    ptr->serialize_from(stream);

    XMETRICS_GAUGE(metrics::xsync_handle_ondemand_chain_snapshot_request, 1);
    xsync_info("xsync_handler receive ondemand_chain_snapshot_request %" PRIx64 " wait(%ldms) %s, account %s, height %llu",
        msg_hash, get_time()-recv_time, from_address.to_string().c_str(), ptr->m_account_addr.c_str(), ptr->m_height_of_fullblock);
    m_sync_on_demand->handle_chain_snapshot_meta(*(ptr.get()), network_self, from_address);
}

void xsync_handler_t::handle_ondemand_chain_snapshot_response(uint32_t msg_size, const vnetwork::xvnode_address_t &from_address,
    const vnetwork::xvnode_address_t &network_self,
    const xsync_message_header_ptr_t &header,
    base::xstream_t &stream,
    xtop_vnetwork_message::hash_result_type msg_hash,
    int64_t recv_time) {

    auto ptr = make_object_ptr<xsync_message_chain_snapshot_t>();
    ptr->serialize_from(stream);

    xsync_info("xsync_handler ondemand chain snapshot reponse %" PRIx64 " wait(%ldms) %s, account %s, height %llu",
        msg_hash, get_time()-recv_time, from_address.to_string().c_str(), ptr->m_tbl_account_addr.c_str(), ptr->m_height_of_fullblock);

    XMETRICS_GAUGE(metrics::xsync_handle_ondemand_chain_snapshot_reponse, 1);

    m_sync_on_demand->handle_chain_snapshot(*(ptr.get()), from_address, network_self);
}

void xsync_handler_t::get_on_demand_by_hash_blocks(uint32_t msg_size, const vnetwork::xvnode_address_t &from_address,
    const vnetwork::xvnode_address_t &network_self,
    const xsync_message_header_ptr_t &header,
    base::xstream_t &stream,
    xtop_vnetwork_message::hash_result_type msg_hash,
    int64_t recv_time) {

    XMETRICS_GAUGE(metrics::xsync_recv_on_demand_by_hash_blocks_req, 1);
    XMETRICS_GAUGE(metrics::xsync_recv_on_demand_by_hash_blocks_req_bytes, msg_size);

    auto ptr = make_object_ptr<xsync_message_get_on_demand_by_hash_blocks_t>();
    ptr->serialize_from(stream);

   m_sync_on_demand->handle_blocks_by_hash_request(*(ptr.get()), from_address, network_self);
}

void xsync_handler_t::on_demand_by_hash_blocks(uint32_t msg_size, const vnetwork::xvnode_address_t &from_address,
    const vnetwork::xvnode_address_t &network_self,
    const xsync_message_header_ptr_t &header,
    base::xstream_t &stream,
    xtop_vnetwork_message::hash_result_type msg_hash,
    int64_t recv_time) {

    XMETRICS_GAUGE(metrics::xsync_recv_on_demand_by_hash_blocks_resp, 1);
    XMETRICS_GAUGE(metrics::xsync_recv_on_demand_by_hash_blocks_resp_bytes, msg_size);

    auto ptr = make_object_ptr<xsync_message_general_blocks_t>();
    ptr->serialize_from(stream);

    std::vector<xblock_ptr_t> &blocks = ptr->blocks;
    if (blocks.size() == 0)
        return;

    m_sync_on_demand->handle_blocks_by_hash_response(blocks, from_address, network_self);
}

int64_t xsync_handler_t::get_time() {
    return base::xtime_utl::gmttime_ms();
}

void xsync_handler_t::notify_deceit_node(const vnetwork::xvnode_address_t& address) {
    xsync_warn("xsync_handler deceit_node %s", address.to_string().c_str());
    m_blacklist->add_deceit_node(address);
    m_role_xips_mgr->remove_xips_by_id(address.node_id());
}

void xsync_handler_t::register_handler(xmessage_t::message_type msgid, xsync_handler_netmsg_callback cb) {
    m_handlers[msgid] = cb;
}

NS_END2
