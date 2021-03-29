// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xcommon/xnode_type.h"
#include "xsync/xsync_sender.h"
#include "xsync/xsync_log.h"
#include "xmetrics/xmetrics.h"
#include "xdata/xdatautil.h"
#include "xsyncbase/xmessage_ids.h"

NS_BEG2(top, sync)

using namespace data;

xsync_sender_t::xsync_sender_t(std::string vnode_id, const observer_ptr<vnetwork::xvhost_face_t> &vhost,
            xrole_xips_manager_t *role_xips_mgr, int min_compress_threshold):
m_vnode_id(vnode_id),
m_vhost(vhost),
m_role_xips_mgr(role_xips_mgr),
m_min_compress_threshold(min_compress_threshold) {
}

void xsync_sender_t::send_gossip(const std::vector<xgossip_chain_info_ptr_t> &info_list, const xbyte_buffer_t &bloom_data, const vnetwork::xvnode_address_t& self_xip, uint32_t max_peers, enum_gossip_target_type target_type) {

    base::xstream_t stream(base::xcontext_t::instance());
    auto header = make_object_ptr<xsync_message_header_t>(RandomUint64());
    header->serialize_to(stream);
    auto body = make_object_ptr<xsync_message_gossip_t>(info_list, bloom_data);
    body->serialize_to(stream);
    vnetwork::xmessage_t _msg = vnetwork::xmessage_t({stream.data(), stream.data() + stream.size()}, xmessage_id_sync_gossip);

    xmessage_t msg;
    xmessage_pack_t::pack_message(_msg, ((int) _msg.payload().size()) >= m_min_compress_threshold, msg);

    std::vector<vnetwork::xvnode_address_t> lists;

    if (target_type == enum_gossip_target_type_neighbor) {
        lists = m_role_xips_mgr->get_rand_neighbors(self_xip, max_peers);
    } else if (target_type == enum_gossip_target_type_parent) {
        lists = m_role_xips_mgr->get_rand_parents(self_xip, max_peers);
    } else if (target_type == enum_gossip_target_type_archive) {
        lists = m_role_xips_mgr->get_rand_archives(max_peers);
    }

    if (lists.empty()) {
        xsync_warn("xsync_sender_t can't send gossip message due to get 0 peers for self xip : %s target_type:%d", self_xip.to_string().c_str(), target_type);
        return;
    }

    //xsync_dbg("xsync_sender_t get gossip peers %d", lists.size());

    for (auto& addr : lists) {
        xsync_dbg("xsync_sender_t send gossip %s -> %s", self_xip.to_string().c_str(), addr.to_string().c_str());
        XMETRICS_COUNTER_INCREMENT("sync_pkgs_gossip_send", 1);
        XMETRICS_COUNTER_INCREMENT("sync_bytes_gossip_send", msg.payload().size());
        XMETRICS_COUNTER_INCREMENT("sync_pkgs_out", 1);
        XMETRICS_COUNTER_INCREMENT("sync_bytes_out", msg.payload().size());
        m_vhost->send(msg, self_xip, addr);
    }
}

void xsync_sender_t::send_gossip_to_target(const std::vector<xgossip_chain_info_ptr_t> &info_list, const xbyte_buffer_t &bloom_data, const vnetwork::xvnode_address_t& self_xip, const vnetwork::xvnode_address_t& target) {

    base::xstream_t stream(base::xcontext_t::instance());
    auto header = make_object_ptr<xsync_message_header_t>(RandomUint64());
    header->serialize_to(stream);
    auto body = make_object_ptr<xsync_message_gossip_t>(info_list, bloom_data);
    body->serialize_to(stream);
    vnetwork::xmessage_t _msg = vnetwork::xmessage_t({stream.data(), stream.data() + stream.size()}, xmessage_id_sync_gossip);

    xmessage_t msg;
    xmessage_pack_t::pack_message(_msg, ((int) _msg.payload().size()) >= m_min_compress_threshold, msg);

    XMETRICS_COUNTER_INCREMENT("sync_pkgs_gossip_to_target_send", 1);
    XMETRICS_COUNTER_INCREMENT("sync_bytes_gossip_to_target_send", msg.payload().size());
    XMETRICS_COUNTER_INCREMENT("sync_pkgs_out", 1);
    XMETRICS_COUNTER_INCREMENT("sync_bytes_out", msg.payload().size());

    xsync_dbg("xsync_sender_t gossip target src %s dst %s", self_xip.to_string().c_str(), target.to_string().c_str());
    m_vhost->send(msg, self_xip, target);
}

void xsync_sender_t::send_frozen_gossip(const std::vector<xgossip_chain_info_ptr_t> &info_list, const xbyte_buffer_t &bloom_data, const vnetwork::xvnode_address_t& self_xip) {

    base::xstream_t stream(base::xcontext_t::instance());
    auto header = make_object_ptr<xsync_message_header_t>(RandomUint64());
    header->serialize_to(stream);
    auto body = make_object_ptr<xsync_message_gossip_t>(info_list, bloom_data);
    body->serialize_to(stream);
    vnetwork::xmessage_t _msg = vnetwork::xmessage_t({stream.data(), stream.data() + stream.size()}, xmessage_id_sync_frozen_gossip);

    xmessage_t msg;
    xmessage_pack_t::pack_message(_msg, ((int) _msg.payload().size()) >= m_min_compress_threshold, msg);

    XMETRICS_COUNTER_INCREMENT("sync_pkgs_frozen_gossip_send", 1);
    XMETRICS_COUNTER_INCREMENT("sync_bytes_frozen_gossip_send", msg.payload().size());
    XMETRICS_COUNTER_INCREMENT("sync_pkgs_out", 1);
    XMETRICS_COUNTER_INCREMENT("sync_bytes_out", msg.payload().size());

    common::xnode_address_t addr(common::build_frozen_sharding_address(self_xip.network_id()));
    xsync_dbg("xsync_sender_t send frozen gossip src %s dst %s", self_xip.to_string().c_str(), addr.to_string().c_str());
    //m_vhost->send(msg, self_xip, addr);
    std::error_code ec;
    m_vhost->broadcast(self_xip, addr.xip2(), msg, ec);
    xsync_dbg("xsync_sender_t::gossip frozen gossip ec=%d", ec);
}

void xsync_sender_t::send_frozen_gossip_to_target(const std::vector<xgossip_chain_info_ptr_t> &info_list, const xbyte_buffer_t &bloom_data, const vnetwork::xvnode_address_t& self_xip, const vnetwork::xvnode_address_t& target) {

    base::xstream_t stream(base::xcontext_t::instance());
    auto header = make_object_ptr<xsync_message_header_t>(RandomUint64());
    header->serialize_to(stream);
    auto body = make_object_ptr<xsync_message_gossip_t>(info_list, bloom_data);
    body->serialize_to(stream);
    vnetwork::xmessage_t _msg = vnetwork::xmessage_t({stream.data(), stream.data() + stream.size()}, xmessage_id_sync_frozen_gossip);

    xmessage_t msg;
    xmessage_pack_t::pack_message(_msg, ((int) _msg.payload().size()) >= m_min_compress_threshold, msg);

    xsync_dbg("xsync_sender_t frozen gossip target src %s dst %s", self_xip.to_string().c_str(), target.to_string().c_str());

    XMETRICS_COUNTER_INCREMENT("sync_pkgs_frozen_gossip_to_target_send", 1);
    XMETRICS_COUNTER_INCREMENT("sync_bytes_frozen_gossip_to_target_send", msg.payload().size());
    XMETRICS_COUNTER_INCREMENT("sync_pkgs_out", 1);
    XMETRICS_COUNTER_INCREMENT("sync_bytes_out", msg.payload().size());

    m_vhost->send(msg, self_xip, target);
}

void xsync_sender_t::send_get_blocks(const std::string &address,
            uint64_t start_height, uint32_t count,
            const vnetwork::xvnode_address_t &self_addr,
            const vnetwork::xvnode_address_t &target_addr) {

    //xsync_dbg("xsync_sender_t %s get blocks send to %s", address.c_str(), target_addr.to_string().c_str());

    base::xstream_t stream(base::xcontext_t::instance());
    auto header = make_object_ptr<xsync_message_header_t>(RandomUint64());
    header->serialize_to(stream);
    auto body = make_object_ptr<xsync_message_get_blocks_t>(address, start_height, count);
    body->serialize_to(stream);
    vnetwork::xmessage_t _msg = vnetwork::xmessage_t({stream.data(), stream.data() + stream.size()}, xmessage_id_sync_get_blocks);

    xmessage_t msg;
    xmessage_pack_t::pack_message(_msg, ((int) _msg.payload().size()) >= m_min_compress_threshold, msg);

    XMETRICS_COUNTER_INCREMENT("sync_pkgs_getblocks_send", 1);
    XMETRICS_COUNTER_INCREMENT("sync_bytes_getblocks_send", msg.payload().size());
    XMETRICS_COUNTER_INCREMENT("sync_pkgs_out", 1);
    XMETRICS_COUNTER_INCREMENT("sync_bytes_out", msg.payload().size());

    m_vhost->send(msg, self_addr, target_addr);
}

void xsync_sender_t::send_blocks(xsync_msg_err_code_t code, const std::string &address, const std::vector<data::xblock_ptr_t> &blocks, const vnetwork::xvnode_address_t& self_addr, const vnetwork::xvnode_address_t& target_addr) {

    base::xstream_t stream(base::xcontext_t::instance());
    auto header = make_object_ptr<xsync_message_header_t>(RandomUint64(), (uint8_t)code);
    header->serialize_to(stream);
    auto body = make_object_ptr<xsync_message_blocks_t>(address, blocks);
    body->serialize_to(stream);
    vnetwork::xmessage_t _msg = vnetwork::xmessage_t({stream.data(), stream.data() + stream.size()}, xmessage_id_sync_blocks);

    xsync_dbg("xsync_sender_t blocks send to target %s", target_addr.to_string().c_str());
    xmessage_t msg;
    xmessage_pack_t::pack_message(_msg,  ((int) _msg.payload().size()) >= m_min_compress_threshold, msg);

    XMETRICS_COUNTER_INCREMENT("sync_pkgs_blocks_send", 1);
    XMETRICS_COUNTER_INCREMENT("sync_bytes_blocks_send", msg.payload().size());
    XMETRICS_COUNTER_INCREMENT("sync_pkgs_out", 1);
    XMETRICS_COUNTER_INCREMENT("sync_bytes_out", msg.payload().size());

    m_vhost->send(msg, self_addr, target_addr);
}

void xsync_sender_t::send_get_on_demand_blocks(const std::string &address,
            uint64_t start_height, uint32_t count, bool is_consensus,
            const vnetwork::xvnode_address_t &self_addr,
            const vnetwork::xvnode_address_t &target_addr) {

    xsync_dbg("xsync_sender_t %s get_on_demand_blocks send to %s", address.c_str(), target_addr.to_string().c_str());

    base::xstream_t stream(base::xcontext_t::instance());
    auto header = make_object_ptr<xsync_message_header_t>(RandomUint64());
    header->serialize_to(stream);
    auto body = make_object_ptr<xsync_message_get_on_demand_blocks_t>(address, start_height, count, is_consensus);
    body->serialize_to(stream);
    vnetwork::xmessage_t _msg = vnetwork::xmessage_t({stream.data(), stream.data() + stream.size()}, xmessage_id_sync_get_on_demand_blocks);

    xmessage_t msg;
    xmessage_pack_t::pack_message(_msg, ((int) _msg.payload().size()) >= m_min_compress_threshold, msg);

    XMETRICS_COUNTER_INCREMENT("sync_pkgs_get_on_demand_blocks_send", 1);
    XMETRICS_COUNTER_INCREMENT("sync_bytes_get_on_demand_blocks_send", msg.payload().size());
    XMETRICS_COUNTER_INCREMENT("sync_pkgs_out", 1);
    XMETRICS_COUNTER_INCREMENT("sync_bytes_out", msg.payload().size());

    m_vhost->send(msg, self_addr, target_addr);
}

void xsync_sender_t::send_on_demand_blocks(const std::vector<data::xblock_ptr_t> &blocks, const vnetwork::xvnode_address_t& self_addr, const vnetwork::xvnode_address_t& target_addr) {

    base::xstream_t stream(base::xcontext_t::instance());
    auto header = make_object_ptr<xsync_message_header_t>(RandomUint64());
    header->serialize_to(stream);
    auto body = make_object_ptr<xsync_message_general_blocks_t>(blocks);
    body->serialize_to(stream);
    vnetwork::xmessage_t _msg = vnetwork::xmessage_t({stream.data(), stream.data() + stream.size()}, xmessage_id_sync_on_demand_blocks);

    xsync_dbg("xsync_sender_t on_demand_blocks send to target %s", target_addr.to_string().c_str());
    xmessage_t msg;
    xmessage_pack_t::pack_message(_msg,  ((int) _msg.payload().size()) >= m_min_compress_threshold, msg);

    XMETRICS_COUNTER_INCREMENT("sync_pkgs_on_demand_blocks_send", 1);
    XMETRICS_COUNTER_INCREMENT("sync_bytes_on_demand_blocks_send", msg.payload().size());
    XMETRICS_COUNTER_INCREMENT("sync_pkgs_out", 1);
    XMETRICS_COUNTER_INCREMENT("sync_bytes_out", msg.payload().size());

    m_vhost->send(msg, self_addr, target_addr);
}

void xsync_sender_t::send_broadcast_chain_state(const std::vector<xchain_state_info_t> &info_list, const vnetwork::xvnode_address_t &self_addr, const vnetwork::xvnode_address_t &target_addr) {

    base::xstream_t stream(base::xcontext_t::instance());
    auto header = make_object_ptr<xsync_message_header_t>(RandomUint64());
    header->serialize_to(stream);
    auto body = make_object_ptr<xsync_message_chain_state_info_t>(info_list);
    body->serialize_to(stream);
    vnetwork::xmessage_t _msg = vnetwork::xmessage_t({stream.data(), stream.data() + stream.size()}, xmessage_id_sync_broadcast_chain_state);

    xmessage_t msg;
    xmessage_pack_t::pack_message(_msg, ((int) _msg.payload().size()) >= m_min_compress_threshold, msg);

    XMETRICS_COUNTER_INCREMENT("sync_pkgs_broadcast_chain_state_send", 1);
    XMETRICS_COUNTER_INCREMENT("sync_bytes_broadcast_chain_state_send", msg.payload().size());
    XMETRICS_COUNTER_INCREMENT("sync_pkgs_out", 1);
    XMETRICS_COUNTER_INCREMENT("sync_bytes_out", msg.payload().size());

    xsync_dbg("xsync_sender_t send broadcast_chain_state count(%u) src %s dst %s", info_list.size(), self_addr.to_string().c_str(), target_addr.to_string().c_str());

    m_vhost->send(msg, self_addr, target_addr);
}

void xsync_sender_t::send_response_chain_state(const std::vector<xchain_state_info_t> &info_list, const vnetwork::xvnode_address_t &self_addr, const vnetwork::xvnode_address_t &target_addr) {
    base::xstream_t stream(base::xcontext_t::instance());
    auto header = make_object_ptr<xsync_message_header_t>(RandomUint64());
    header->serialize_to(stream);
    auto body = make_object_ptr<xsync_message_chain_state_info_t>(info_list);
    body->serialize_to(stream);
    vnetwork::xmessage_t _msg = vnetwork::xmessage_t({stream.data(), stream.data() + stream.size()}, xmessage_id_sync_response_chain_state);

    xmessage_t msg;
    xmessage_pack_t::pack_message(_msg, ((int) _msg.payload().size()) >= m_min_compress_threshold, msg);

    XMETRICS_COUNTER_INCREMENT("sync_pkgs_response_chain_state_send", 1);
    XMETRICS_COUNTER_INCREMENT("sync_bytes_response_chain_state_send", msg.payload().size());
    XMETRICS_COUNTER_INCREMENT("sync_pkgs_out", 1);
    XMETRICS_COUNTER_INCREMENT("sync_bytes_out", msg.payload().size());

    xsync_dbg("xsync_sender_t send response_chain_state count(%u) src %s dst %s", info_list.size(), self_addr.to_string().c_str(), target_addr.to_string().c_str());

    m_vhost->send(msg, self_addr, target_addr);
}

void xsync_sender_t::send_frozen_broadcast_chain_state(const std::vector<xchain_state_info_t> &info_list, const vnetwork::xvnode_address_t &self_addr) {

    base::xstream_t stream(base::xcontext_t::instance());
    auto header = make_object_ptr<xsync_message_header_t>(RandomUint64());
    header->serialize_to(stream);
    auto body = make_object_ptr<xsync_message_chain_state_info_t>(info_list);
    body->serialize_to(stream);
    vnetwork::xmessage_t _msg = vnetwork::xmessage_t({stream.data(), stream.data() + stream.size()}, xmessage_id_sync_frozen_broadcast_chain_state);

    xmessage_t msg;
    xmessage_pack_t::pack_message(_msg, ((int) _msg.payload().size()) >= m_min_compress_threshold, msg);

    XMETRICS_COUNTER_INCREMENT("sync_pkgs_frozen_broadcast_chain_state_send", 1);
    XMETRICS_COUNTER_INCREMENT("sync_bytes_frozen_broadcast_chain_state_send", msg.payload().size());
    XMETRICS_COUNTER_INCREMENT("sync_pkgs_out", 1);
    XMETRICS_COUNTER_INCREMENT("sync_bytes_out", msg.payload().size());

    common::xnode_address_t target_addr(common::build_frozen_sharding_address(self_addr.network_id()));

    xsync_dbg("xsync_sender_t send frozen_broadcast_chain_state count(%u) src %s dst %s", info_list.size(), self_addr.to_string().c_str(), target_addr.to_string().c_str());
    std::error_code ec;
    m_vhost->broadcast(self_addr, target_addr.xip2(), msg, ec);
    xsync_dbg("xsync_sender_t frozen frozen_broadcast_chain_state ec=%d", ec);
}

void xsync_sender_t::send_frozen_response_chain_state(const std::vector<xchain_state_info_t> &info_list, const vnetwork::xvnode_address_t &self_addr, const vnetwork::xvnode_address_t &target_addr) {
    base::xstream_t stream(base::xcontext_t::instance());
    auto header = make_object_ptr<xsync_message_header_t>(RandomUint64());
    header->serialize_to(stream);
    auto body = make_object_ptr<xsync_message_chain_state_info_t>(info_list);
    body->serialize_to(stream);
    vnetwork::xmessage_t _msg = vnetwork::xmessage_t({stream.data(), stream.data() + stream.size()}, xmessage_id_sync_frozen_response_chain_state);

    xmessage_t msg;
    xmessage_pack_t::pack_message(_msg, ((int) _msg.payload().size()) >= m_min_compress_threshold, msg);

    XMETRICS_COUNTER_INCREMENT("sync_pkgs_frozen_response_chain_state_send", 1);
    XMETRICS_COUNTER_INCREMENT("sync_bytes_frozen_response_chain_state_send", msg.payload().size());
    XMETRICS_COUNTER_INCREMENT("sync_pkgs_out", 1);
    XMETRICS_COUNTER_INCREMENT("sync_bytes_out", msg.payload().size());

    xsync_dbg("xsync_sender_t send frozen_response_chain_state count(%u) src %s dst %s", info_list.size(), self_addr.to_string().c_str(), target_addr.to_string().c_str());

    m_vhost->send(msg, self_addr, target_addr);
}

void xsync_sender_t::send_cross_cluster_chain_state(const std::vector<xchain_state_info_t> &info_list, const vnetwork::xvnode_address_t &self_addr, const vnetwork::xvnode_address_t &target_addr) {
    base::xstream_t stream(base::xcontext_t::instance());
    auto header = make_object_ptr<xsync_message_header_t>(RandomUint64());
    header->serialize_to(stream);
    auto body = make_object_ptr<xsync_message_chain_state_info_t>(info_list);
    body->serialize_to(stream);
    vnetwork::xmessage_t _msg = vnetwork::xmessage_t({stream.data(), stream.data() + stream.size()}, xmessage_id_sync_cross_cluster_chain_state);

    xmessage_t msg;
    xmessage_pack_t::pack_message(_msg, ((int) _msg.payload().size()) >= m_min_compress_threshold, msg);

    XMETRICS_COUNTER_INCREMENT("sync_pkgs_cross_cluster_chain_state_send", 1);
    XMETRICS_COUNTER_INCREMENT("sync_bytes_cross_cluster_chain_state_send", msg.payload().size());
    XMETRICS_COUNTER_INCREMENT("sync_pkgs_out", 1);
    XMETRICS_COUNTER_INCREMENT("sync_bytes_out", msg.payload().size());

    xsync_dbg("xsync_sender_t send cross_cluster_chain_state count(%u) src %s dst %s", info_list.size(), self_addr.to_string().c_str(), target_addr.to_string().c_str());

    m_vhost->send(msg, self_addr, target_addr);
}

void xsync_sender_t::push_newblock(const data::xblock_ptr_t &block, const vnetwork::xvnode_address_t& self_addr, const vnetwork::xvnode_address_t& target_addr) {
    base::xstream_t stream(base::xcontext_t::instance());
    auto header = make_object_ptr<xsync_message_header_t>(RandomUint64());
    header->serialize_to(stream);
    auto body = make_object_ptr<xsync_message_push_newblock_t>(block);
    body->serialize_to(stream);
    vnetwork::xmessage_t _msg = vnetwork::xmessage_t({stream.data(), stream.data() + stream.size()}, xmessage_id_sync_push_newblock);

    xmessage_t msg;
    xmessage_pack_t::pack_message(_msg, ((int) _msg.payload().size()) >= m_min_compress_threshold, msg);

    xsync_dbg("xsync_sender_t send push_newblock %s,height=%lu,viewid=%lu,hash=%s, src %s dst %s", 
        block->get_account().c_str(), block->get_height(), block->get_viewid(), to_hex_str(block->get_block_hash()).c_str(), self_addr.to_string().c_str(), target_addr.to_string().c_str());
    XMETRICS_COUNTER_INCREMENT("sync_pkgs_push_newblock_send", 1);
    XMETRICS_COUNTER_INCREMENT("sync_bytes_push_newblock_send", msg.payload().size());
    XMETRICS_COUNTER_INCREMENT("sync_pkgs_out", 1);
    XMETRICS_COUNTER_INCREMENT("sync_bytes_out", msg.payload().size());
    m_vhost->send(msg, self_addr, target_addr);
}

void xsync_sender_t::push_newblockhash(const data::xblock_ptr_t &block, const vnetwork::xvnode_address_t& self_addr, const vnetwork::xvnode_address_t& target_addr) {
    base::xstream_t stream(base::xcontext_t::instance());
    auto header = make_object_ptr<xsync_message_header_t>(RandomUint64());
    header->serialize_to(stream);
    auto body = make_object_ptr<xsync_message_general_newblockhash_t>(block->get_account(), block->get_height(), block->get_viewid(), block->get_block_hash());
    body->serialize_to(stream);
    vnetwork::xmessage_t _msg = vnetwork::xmessage_t({stream.data(), stream.data() + stream.size()}, xmessage_id_sync_push_newblockhash);

    xmessage_t msg;
    xmessage_pack_t::pack_message(_msg, ((int) _msg.payload().size()) >= m_min_compress_threshold, msg);

    XMETRICS_COUNTER_INCREMENT("sync_pkgs_push_newblockhash_send", 1);
    XMETRICS_COUNTER_INCREMENT("sync_bytes_push_newblockhash_send", msg.payload().size());
    XMETRICS_COUNTER_INCREMENT("sync_pkgs_out", 1);
    XMETRICS_COUNTER_INCREMENT("sync_bytes_out", msg.payload().size());

    xsync_dbg("xsync_sender_t send push_newblockhash %s,height=%lu,viewid=%lu,hash=%s, src %s dst %s", 
        block->get_account().c_str(), block->get_height(), block->get_viewid(), to_hex_str(block->get_block_hash()).c_str(), self_addr.to_string().c_str(), target_addr.to_string().c_str());
    m_vhost->send(msg, self_addr, target_addr);
}

void xsync_sender_t::broadcast_newblockhash(const data::xblock_ptr_t &block, const vnetwork::xvnode_address_t& self_addr, const vnetwork::xvnode_address_t& target_addr) {
    base::xstream_t stream(base::xcontext_t::instance());
    auto header = make_object_ptr<xsync_message_header_t>(RandomUint64());
    header->serialize_to(stream);
    auto body = make_object_ptr<xsync_message_general_newblockhash_t>(block->get_account(), block->get_height(), block->get_viewid(), block->get_block_hash());
    body->serialize_to(stream);
    vnetwork::xmessage_t _msg = vnetwork::xmessage_t({stream.data(), stream.data() + stream.size()}, xmessage_id_sync_broadcast_newblockhash);

    xmessage_t msg;
    xmessage_pack_t::pack_message(_msg, ((int) _msg.payload().size()) >= m_min_compress_threshold, msg);

    XMETRICS_COUNTER_INCREMENT("sync_pkgs_broadcast_newblockhash_send", 1);
    XMETRICS_COUNTER_INCREMENT("sync_bytes_broadcast_newblockhash_send", msg.payload().size());
    XMETRICS_COUNTER_INCREMENT("sync_pkgs_out", 1);
    XMETRICS_COUNTER_INCREMENT("sync_bytes_out", msg.payload().size());

    xsync_dbg("xsync_sender_t send broadcast_newblockhash %s,height=%lu,viewid=%lu,hash=%s, src %s dst %s", 
        block->get_account().c_str(), block->get_height(), block->get_viewid(), to_hex_str(block->get_block_hash()).c_str(), self_addr.to_string().c_str(), target_addr.to_string().c_str());
    m_vhost->send(msg, self_addr, target_addr);
}

void xsync_sender_t::send_get_blocks_by_hashes(const std::vector<xblock_hash_t> &hashes, const vnetwork::xvnode_address_t &self_addr, const vnetwork::xvnode_address_t &target_addr) {

    //xsync_dbg("xsync_sender_t %s get blocks by hashes send to %s", address.c_str(), target_addr.to_string().c_str());

    base::xstream_t stream(base::xcontext_t::instance());
    auto header = make_object_ptr<xsync_message_header_t>(RandomUint64());
    header->serialize_to(stream);
    auto body = make_object_ptr<xsync_message_get_blocks_by_hashes_t>(hashes);
    body->serialize_to(stream);
    vnetwork::xmessage_t _msg = vnetwork::xmessage_t({stream.data(), stream.data() + stream.size()}, xmessage_id_sync_get_blocks_by_hashes);

    xmessage_t msg;
    xmessage_pack_t::pack_message(_msg, ((int) _msg.payload().size()) >= m_min_compress_threshold, msg);

    XMETRICS_COUNTER_INCREMENT("sync_pkgs_getblocks_by_hashes_send", 1);
    XMETRICS_COUNTER_INCREMENT("sync_bytes_getblocks_by_hashes_send", msg.payload().size());
    XMETRICS_COUNTER_INCREMENT("sync_pkgs_out", 1);
    XMETRICS_COUNTER_INCREMENT("sync_bytes_out", msg.payload().size());

    m_vhost->send(msg, self_addr, target_addr);
}

void xsync_sender_t::send_blocks_by_hashes(const std::vector<data::xblock_ptr_t> &blocks, const vnetwork::xvnode_address_t &self_addr, const vnetwork::xvnode_address_t &target_addr) {

    base::xstream_t stream(base::xcontext_t::instance());
    auto header = make_object_ptr<xsync_message_header_t>(RandomUint64());
    header->serialize_to(stream);
    auto body = make_object_ptr<xsync_message_general_blocks_t>(blocks);
    body->serialize_to(stream);
    vnetwork::xmessage_t _msg = vnetwork::xmessage_t({stream.data(), stream.data() + stream.size()}, xmessage_id_sync_blocks_by_hashes);

    xsync_dbg("xsync_sender_t blocks by hashes send to target %s", target_addr.to_string().c_str());
    xmessage_t msg;
    xmessage_pack_t::pack_message(_msg,  ((int) _msg.payload().size()) >= m_min_compress_threshold, msg);

    XMETRICS_COUNTER_INCREMENT("sync_pkgs_blocks_hashes_send", 1);
    XMETRICS_COUNTER_INCREMENT("sync_bytes_blocks_hashes_send", msg.payload().size());
    XMETRICS_COUNTER_INCREMENT("sync_pkgs_out", 1);
    XMETRICS_COUNTER_INCREMENT("sync_bytes_out", msg.payload().size());

    m_vhost->send(msg, self_addr, target_addr);
}

void xsync_sender_t::send_chain_snapshot_meta(const xsync_message_chain_snapshot_meta_t &chain_snapshot_meta, const vnetwork::xvnode_address_t &self_addr, const vnetwork::xvnode_address_t &target_addr) {
    base::xstream_t stream(base::xcontext_t::instance());
    auto header = make_object_ptr<xsync_message_header_t>(RandomUint64());
    header->serialize_to(stream);
    auto body = make_object_ptr<xsync_message_chain_snapshot_meta_t>(chain_snapshot_meta.m_account_addr,chain_snapshot_meta.m_height_of_fullblock);
    body->serialize_to(stream);
    vnetwork::xmessage_t _msg = vnetwork::xmessage_t({stream.data(), stream.data() + stream.size()}, xmessage_id_sync_chain_snapshot_request);
    xmessage_t msg;
    xmessage_pack_t::pack_message(_msg, ((int) _msg.payload().size()) >= m_min_compress_threshold, msg);
    XMETRICS_COUNTER_INCREMENT("sync_pkgs_chain_snapshot_meta_send", 1);
    XMETRICS_COUNTER_INCREMENT("sync_bytes_chain_snapshot_meta_send", msg.payload().size());
    XMETRICS_COUNTER_INCREMENT("sync_pkgs_out", 1);
    XMETRICS_COUNTER_INCREMENT("sync_bytes_out", msg.payload().size());
    xsync_dbg("xsync_sender_t send chain_snapshot_meta src %s dst %s", self_addr.to_string().c_str(), target_addr.to_string().c_str());
    m_vhost->send(msg, self_addr, target_addr);
}
void xsync_sender_t::send_chain_snapshot(const xsync_message_chain_snapshot_t &chain_snapshot, const vnetwork::xvnode_address_t &self_addr, const vnetwork::xvnode_address_t &target_addr) {
    base::xstream_t stream(base::xcontext_t::instance());
    auto header = make_object_ptr<xsync_message_header_t>(RandomUint64());
    header->serialize_to(stream);
    auto body = make_object_ptr<xsync_message_chain_snapshot_t>(chain_snapshot.m_tbl_account_addr, chain_snapshot.m_chain_snapshot, chain_snapshot.m_height_of_fullblock);
    body->serialize_to(stream);
    vnetwork::xmessage_t _msg = vnetwork::xmessage_t({stream.data(), stream.data() + stream.size()}, xmessage_id_sync_chain_snapshot_response);
    xmessage_t msg;
    xmessage_pack_t::pack_message(_msg, ((int) _msg.payload().size()) >= m_min_compress_threshold, msg);
    XMETRICS_COUNTER_INCREMENT("sync_pkgs_chain_snapshot_detail_send", 1);
    XMETRICS_COUNTER_INCREMENT("sync_bytes_chain_snapshot_detail_send", msg.payload().size());
    XMETRICS_COUNTER_INCREMENT("sync_pkgs_out", 1);
    XMETRICS_COUNTER_INCREMENT("sync_bytes_out", msg.payload().size());
    xsync_dbg("xsync_sender_t send chain_snapshot_detail count(%u) src %s dst %s", 1, self_addr.to_string().c_str(), target_addr.to_string().c_str());
    m_vhost->send(msg, self_addr, target_addr);
}
NS_END2
