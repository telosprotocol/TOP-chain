// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <time.h>
#include "xbasic/xns_macro.h"
#include "xbase/xmem.h"


#include "xstore/xstore_face.h"
#include "xmbus/xmessage_bus.h"
#include "xsync/xsync_message.h"
#include "xdata/xdatautil.h"
#include "xdata/xblock.h"
#include "xsync/xsession.h"
#include "xsync/xrole_chains_mgr.h"
#include "xsync/xsync_status.h"
#include "xbase/xdata.h"
#include "xsync/xrole_xips_manager.h"
#include "xsync/xsync_sender.h"
#include "xsync/xdownloader.h"
#include "xsync/xblock_fetcher.h"
#include "xvnetwork/xvnetwork_message.h"
#include "xsync/xsync_gossip.h"
#include "xsync/xblock_processor.h"
#include "xsync/xsync_latest.h"

NS_BEG2(top, sync)

class xsync_handler_t {
public:
    xsync_handler_t(std::string vnode_id,
        xsync_store_face_t* sync_store,
        const observer_ptr<mbus::xmessage_bus_face_t> &mbus,
        xsession_manager_t *session_mgr, xdeceit_node_manager_t *blacklist,
        xrole_chains_mgr_t *role_chains_mgr, xrole_xips_manager_t *role_xips_mgr,
        xdownloader_t *downloader, xblock_fetcher_t *block_fetcher, xsync_gossip_t *sync_gossip,
        xsync_sender_t *sync_sender, xsync_latest_t *sync_latest);
    virtual ~xsync_handler_t();

public:
    void on_message(
        const vnetwork::xvnode_address_t &from_address,
        const vnetwork::xvnode_address_t &network_self,
        const xbyte_buffer_t &msg,
        vnetwork::xmessage_t::message_type msg_type,
        vnetwork::xtop_vnetwork_message::hash_result_type msg_hash,
        int64_t recv_time);

    void on_event(const mbus::xevent_ptr_t& e);

private:

    void get_blocks(
        const vnetwork::xvnode_address_t &from_address,
        const vnetwork::xvnode_address_t &network_self,
        base::xstream_t &stream,
        xtop_vnetwork_message::hash_result_type msg_hash,
        int64_t recv_time);

    void newblock(
        const vnetwork::xvnode_address_t &from_address,
        const vnetwork::xvnode_address_t &network_self,
        base::xstream_t &stream,
        xtop_vnetwork_message::hash_result_type msg_hash,
        int64_t recv_time);

    void newblockhash(
        const vnetwork::xvnode_address_t &from_address,
        const vnetwork::xvnode_address_t &network_self,
        base::xstream_t &stream,
        xtop_vnetwork_message::hash_result_type msg_hash,
        int64_t recv_time);

    void blocks(
        const vnetwork::xvnode_address_t &from_address,
        const vnetwork::xvnode_address_t &network_self,
        const xsync_message_header_ptr_t &header,
        base::xstream_t &stream,
        xtop_vnetwork_message::hash_result_type msg_hash,
        int64_t recv_time);

    void gossip(
        const vnetwork::xvnode_address_t &from_address,
        const vnetwork::xvnode_address_t &network_self,
        base::xstream_t &stream,
        xtop_vnetwork_message::hash_result_type msg_hash,
        int64_t recv_time);

    void latest_block_info(
        const vnetwork::xvnode_address_t &from_address,
        const vnetwork::xvnode_address_t &network_self,
        base::xstream_t &stream,
        xtop_vnetwork_message::hash_result_type msg_hash,
        int64_t recv_time);

    void get_latest_blocks(
        const vnetwork::xvnode_address_t &from_address,
        const vnetwork::xvnode_address_t &network_self,
        base::xstream_t &stream,
        xtop_vnetwork_message::hash_result_type msg_hash,
        int64_t recv_time);

    void latest_blocks(
        const vnetwork::xvnode_address_t &from_address,
        const vnetwork::xvnode_address_t &network_self,
        base::xstream_t &stream,
        xtop_vnetwork_message::hash_result_type msg_hash,
        int64_t recv_time);

private:
    void handle_role_change(const mbus::xevent_ptr_t& e);
    void handle_consensus_result(const mbus::xevent_ptr_t& e);

private:
    int64_t get_time();
    void notify_deceit_node(const vnetwork::xvnode_address_t& address);

private:
    std::string m_vnode_id;
    xsync_store_face_t *m_sync_store{};
    xsession_manager_t *m_session_mgr;
    xdeceit_node_manager_t *m_blacklist;
    xrole_chains_mgr_t *m_role_chains_mgr;
    xrole_xips_manager_t *m_role_xips_mgr;
    xdownloader_t *m_downloader;
    xblock_fetcher_t *m_block_fetcher;
    xsync_gossip_t *m_sync_gossip;
    xblock_processor_t m_block_processor;
    xsync_sender_t *m_sync_sender;
    xsync_latest_t *m_sync_latest;
};

using xsync_handler_ptr_t = std::shared_ptr<xsync_handler_t>;

NS_END2
