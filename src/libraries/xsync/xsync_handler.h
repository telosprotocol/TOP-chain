// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <time.h>
#include <functional>
#include "xbase/xmem.h"


#include "xmbus/xmessage_bus.h"
#include "xsync/xsync_message.h"
#include "xdata/xdatautil.h"
#include "xdata/xblock.h"
// TODO(jimmy) #include "xbase/xvledger.h"
//#include "xsync/xsession.h"
#include "xsync/xsync_session_manager.h"
#include "xsync/xrole_chains_mgr.h"
#include "xbase/xdata.h"
#include "xsync/xrole_xips_manager.h"
#include "xsync/xsync_sender.h"
#include "xsync/xdownloader.h"
#include "xsync/xblock_fetcher.h"
#include "xvnetwork/xvnetwork_message.h"
#include "xsync/xsync_gossip.h"
#include "xsync/xsync_on_demand.h"
#include "xsync/xsync_peerset.h"
#include "xsync/xsync_peer_keeper.h"
#include "xsync/xsync_behind_checker.h"
#include "xsync/xsync_cross_cluster_chain_state.h"
#include "xsync/xsync_pusher.h"
#include "xsync/xdeceit_node_manager.h"

NS_BEG2(top, sync)

using xsync_handler_netmsg_callback = std::function<void(uint32_t, const vnetwork::xvnode_address_t &, const vnetwork::xvnode_address_t &, const xsync_message_header_ptr_t &, base::xstream_t &, vnetwork::xvnetwork_message_t::hash_result_type, int64_t)>;

class xsync_handler_t {
public:
    xsync_handler_t(std::string vnode_id,
        xsync_store_face_t* sync_store,
        const observer_ptr<base::xvcertauth_t> &certauth,
        xsync_session_manager_t *session_mgr, xdeceit_node_manager_t *blacklist,
        xrole_chains_mgr_t *role_chains_mgr, xrole_xips_manager_t *role_xips_mgr,
        xdownloader_t *downloader, xblock_fetcher_t *block_fetcher, xsync_gossip_t *sync_gossip, xsync_pusher_t *sync_pusher,
        xsync_sender_t *sync_sender, xsync_on_demand_t *sync_on_demand,
        xsync_peerset_t *peerset, xsync_peer_keeper_t *peer_keeper, xsync_behind_checker_t *behind_checker,
        xsync_cross_cluster_chain_state_t *cross_cluster_chain_state);
    virtual ~xsync_handler_t();

public:
    void on_message(
        const vnetwork::xvnode_address_t &from_address,
        const vnetwork::xvnode_address_t &network_self,
        const xbyte_buffer_t &msg,
        vnetwork::xmessage_t::message_type msg_type,
        vnetwork::xvnetwork_message_t::hash_result_type msg_hash,
        int64_t recv_time);

    void on_event(const mbus::xevent_ptr_t& e);

private:

    void gossip(
        uint32_t msg_size,
        const vnetwork::xvnode_address_t &from_address,
        const vnetwork::xvnode_address_t &network_self,
        const xsync_message_header_ptr_t &header,
        base::xstream_t &stream,
        vnetwork::xvnetwork_message_t::hash_result_type msg_hash,
        int64_t recv_time);

    void broadcast_chain_state(
        uint32_t msg_size,
        const vnetwork::xvnode_address_t &from_address,
        const vnetwork::xvnode_address_t &network_self,
        const xsync_message_header_ptr_t &header,
        base::xstream_t &stream,
        vnetwork::xvnetwork_message_t::hash_result_type msg_hash,
        int64_t recv_time);

    void response_chain_state(
        uint32_t msg_size,
        const vnetwork::xvnode_address_t &from_address,
        const vnetwork::xvnode_address_t &network_self,
        const xsync_message_header_ptr_t &header,
        base::xstream_t &stream,
        vnetwork::xvnetwork_message_t::hash_result_type msg_hash,
        int64_t recv_time);

    void cross_cluster_chain_state(
        uint32_t msg_size,
        const vnetwork::xvnode_address_t &from_address,
        const vnetwork::xvnode_address_t &network_self,
        const xsync_message_header_ptr_t &header,
        base::xstream_t &stream,
        vnetwork::xvnetwork_message_t::hash_result_type msg_hash,
        int64_t recv_time);

    void recv_query_archive_height(
        uint32_t msg_size,
        const vnetwork::xvnode_address_t &from_address,
        const vnetwork::xvnode_address_t &network_self,
        const xsync_message_header_ptr_t &header,
        base::xstream_t &stream,
        vnetwork::xvnetwork_message_t::hash_result_type msg_hash,
        int64_t recv_time);


    void on_block_request_process(uint32_t msg_size,
        const vnetwork::xvnode_address_t &from_address,
        const vnetwork::xvnode_address_t &network_self,
        const xsync_message_header_ptr_t &header,
        base::xstream_t &stream,
        vnetwork::xvnetwork_message_t::hash_result_type msg_hash,
        int64_t recv_time);

    void on_block_response_process(uint32_t msg_size,
        const vnetwork::xvnode_address_t &from_address,
        const vnetwork::xvnode_address_t &network_self,
        const xsync_message_header_ptr_t &header,
        base::xstream_t &stream,
        vnetwork::xvnetwork_message_t::hash_result_type msg_hash,
        int64_t recv_time);
        
    void on_block_push_newblock(uint32_t msg_size,
        const vnetwork::xvnode_address_t &from_address,
        const vnetwork::xvnode_address_t &network_self,
        const xsync_message_header_ptr_t &header,
        base::xstream_t &stream,
        vnetwork::xvnetwork_message_t::hash_result_type msg_hash,
        int64_t recv_time);

    void handle_blocks_request_with_height(const xsync_msg_block_request_ptr_t& request,
        const vnetwork::xvnode_address_t& to_address,
        const vnetwork::xvnode_address_t& network_self);

    void handle_blocks_demand_request_with_hash(const xsync_msg_block_request_ptr_t& request,
        const vnetwork::xvnode_address_t& to_address,
        const vnetwork::xvnode_address_t& network_self);

    void handle_blocks_request_with_txhash(const xsync_msg_block_request_ptr_t& request,
        const vnetwork::xvnode_address_t& to_address,
        const vnetwork::xvnode_address_t& network_self);

    void handle_blocks_ontime_request_with_hash(const xsync_msg_block_request_ptr_t& request,
        const vnetwork::xvnode_address_t& to_address,
        const vnetwork::xvnode_address_t& network_self);

    void handle_blocks_ontime_request_with_height_lists(const xsync_msg_block_request_ptr_t& request,
        const vnetwork::xvnode_address_t& to_address,
        const vnetwork::xvnode_address_t& network_self);

    void on_block_response_process_bigpack(uint32_t msg_size,
        const vnetwork::xvnode_address_t& from_address,
        const vnetwork::xvnode_address_t& network_self,
        const xsync_message_header_ptr_t& header,
        base::xstream_t& stream,
        vnetwork::xvnetwork_message_t::hash_result_type msg_hash,
        int64_t recv_time);

    void on_block_response_process_handler(uint32_t msg_size,
        const vnetwork::xvnode_address_t& from_address,
        const vnetwork::xvnode_address_t& network_self,
        const xsync_message_header_ptr_t& header,
        const xsync_msg_block_response_ptr_t& response_ptr,
        vnetwork::xvnetwork_message_t::hash_result_type msg_hash,
        int64_t recv_time);

private:
    void handle_role_change(const mbus::xevent_ptr_t& e);
    void handle_consensus_result(const mbus::xevent_ptr_t& e);

private:
    int64_t get_time();
    void notify_deceit_node(const vnetwork::xvnode_address_t& address);
    void register_handler(vnetwork::xmessage_t::message_type msgid, xsync_handler_netmsg_callback cb);
    int init_prune(const map_chain_info_t &chains, const mbus::xevent_ptr_t& e);
private:
    std::string m_vnode_id;
    xsync_store_face_t *m_sync_store{};
    observer_ptr<base::xvcertauth_t> m_certauth;
    xsync_session_manager_t *m_session_mgr;
    xdeceit_node_manager_t *m_blacklist;
    xrole_chains_mgr_t *m_role_chains_mgr;
    xrole_xips_manager_t *m_role_xips_mgr;
    xdownloader_t *m_downloader;
    xblock_fetcher_t *m_block_fetcher;
    xsync_gossip_t *m_sync_gossip;
    xsync_pusher_t *m_sync_pusher;
    xsync_sender_t *m_sync_sender;
    xsync_on_demand_t *m_sync_on_demand;
    xsync_peerset_t *m_peerset;
    xsync_peer_keeper_t *m_peer_keeper;
    xsync_behind_checker_t *m_behind_checker;
    xsync_cross_cluster_chain_state_t *m_cross_cluster_chain_state;
    std::unordered_map<vnetwork::xmessage_t::message_type, xsync_handler_netmsg_callback> m_handlers;
};

using xsync_handler_ptr_t = std::shared_ptr<xsync_handler_t>;

NS_END2
