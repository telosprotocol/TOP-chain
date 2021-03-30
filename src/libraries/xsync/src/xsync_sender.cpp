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

using namespace syncbase;
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
        lists = m_role_xips_mgr->get_rand_archives(self_xip, max_peers);
    }

    if (lists.empty()) {
        xsync_warn("[xsync_sender_t] can't send gossip message due to get 0 peers for self xip : %s target_type:%d", self_xip.to_string().c_str(), target_type);
        return;
    }

    //xsync_dbg("[xsync_sender_t] get gossip peers %d", lists.size());

    for (auto& addr : lists) {
        xsync_dbg("[xsync_sender_t] send gossip %s -> %s", self_xip.to_string().c_str(), addr.to_string().c_str());
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

    XMETRICS_COUNTER_INCREMENT("sync_pkgs_gossip_send", 1);
    XMETRICS_COUNTER_INCREMENT("sync_bytes_gossip_send", msg.payload().size());
    XMETRICS_COUNTER_INCREMENT("sync_pkgs_out", 1);
    XMETRICS_COUNTER_INCREMENT("sync_bytes_out", msg.payload().size());

    xsync_dbg("[xsync_sender_t] gossip target %s -> %s", self_xip.to_string().c_str(), target.to_string().c_str());
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

    XMETRICS_COUNTER_INCREMENT("sync_pkgs_gossip_send", 1);
    XMETRICS_COUNTER_INCREMENT("sync_bytes_gossip_send", msg.payload().size());
    XMETRICS_COUNTER_INCREMENT("sync_pkgs_out", 1);
    XMETRICS_COUNTER_INCREMENT("sync_bytes_out", msg.payload().size());

    common::xnode_address_t addr(common::build_frozen_sharding_address(self_xip.network_id()));
    xsync_dbg("[xsync_sender_t] send frozen gossip %s -> %s", self_xip.to_string().c_str(), addr.to_string().c_str());
    //m_vhost->send(msg, self_xip, addr);
    std::error_code ec;
    m_vhost->broadcast(self_xip, addr.xip2(), msg, ec);
    xsync_dbg("[xsync_sender_t::gossip] frozen gossip ec=%d", ec);
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

    xsync_dbg("[xsync_sender_t] frozen gossip target %s -> %s", self_xip.to_string().c_str(), target.to_string().c_str());

    XMETRICS_COUNTER_INCREMENT("sync_pkgs_gossip_send", 1);
    XMETRICS_COUNTER_INCREMENT("sync_bytes_gossip_send", msg.payload().size());
    XMETRICS_COUNTER_INCREMENT("sync_pkgs_out", 1);
    XMETRICS_COUNTER_INCREMENT("sync_bytes_out", msg.payload().size());

    m_vhost->send(msg, self_xip, target);
}

void xsync_sender_t::send_get_blocks(const std::string &address,
            uint64_t start_height, uint32_t count,
            const vnetwork::xvnode_address_t &self_addr,
            const vnetwork::xvnode_address_t &target_addr) {

    //xsync_dbg("[xsync_sender_t] %s get blocks send to %s", address.c_str(), target_addr.to_string().c_str());

    base::xstream_t stream(base::xcontext_t::instance());
    auto header = make_object_ptr<xsync_message_header_t>(RandomUint64());
    header->serialize_to(stream);
    auto body = make_object_ptr<xsync_message_get_blocks_t>(address, start_height, count);
    body->serialize_to(stream);
    vnetwork::xmessage_t _msg = vnetwork::xmessage_t({stream.data(), stream.data() + stream.size()}, xmessage_id_sync_get_blocks);

    xmessage_t msg;
    xmessage_pack_t::pack_message(_msg, ((int) _msg.payload().size()) >= m_min_compress_threshold, msg);

    XMETRICS_COUNTER_INCREMENT("sync_pkgs_getblocks_send", 1);
    XMETRICS_COUNTER_INCREMENT("sync_pkgs_out", 1);
    XMETRICS_COUNTER_INCREMENT("sync_bytes_out", msg.payload().size());

    m_vhost->send(msg, self_addr, target_addr);
}

void xsync_sender_t::send_blocks(xsync_msg_err_code_t code, const std::string &address, const std::vector<data::xblock_ptr_t> &blocks, const vnetwork::xvnode_address_t& target, const vnetwork::xvnode_address_t& through_address) {

    base::xstream_t stream(base::xcontext_t::instance());
    auto header = make_object_ptr<xsync_message_header_t>(RandomUint64(), (uint8_t)code);
    header->serialize_to(stream);
    xsync_message_blocks_ptr_t body = make_object_ptr<xsync_message_blocks_t>(address, blocks);
    body->serialize_to(stream);
    vnetwork::xmessage_t _msg = vnetwork::xmessage_t({stream.data(), stream.data() + stream.size()}, xmessage_id_sync_blocks);

    xsync_dbg("[xsync_sender_t] blocks send to target %s", target.to_string().c_str());
    xmessage_t msg;
    xmessage_pack_t::pack_message(_msg,  ((int) _msg.payload().size()) >= m_min_compress_threshold, msg);

    XMETRICS_COUNTER_INCREMENT("sync_pkgs_blocks_send", 1);
    XMETRICS_COUNTER_INCREMENT("sync_bytes_blocks_send", msg.payload().size());
    XMETRICS_COUNTER_INCREMENT("sync_pkgs_out", 1);
    XMETRICS_COUNTER_INCREMENT("sync_bytes_out", msg.payload().size());

    m_vhost->send(msg, through_address, target);
}

void xsync_sender_t::send_latest_block_info(const std::vector<xlatest_block_info_t> &info_list, const vnetwork::xvnode_address_t& self_addr, uint32_t max_peers, enum_latest_block_info_target_type target_type) {
    base::xstream_t stream(base::xcontext_t::instance());
    auto header = make_object_ptr<xsync_message_header_t>(RandomUint64());
    header->serialize_to(stream);
    auto body = make_object_ptr<xsync_message_latest_block_info_t>(info_list);
    body->serialize_to(stream);
    vnetwork::xmessage_t _msg = vnetwork::xmessage_t({stream.data(), stream.data() + stream.size()}, xmessage_id_sync_latest_block_info);

    xmessage_t msg;
    xmessage_pack_t::pack_message(_msg, ((int) _msg.payload().size()) >= m_min_compress_threshold, msg);

    std::vector<vnetwork::xvnode_address_t> lists;

    if (target_type == enum_latest_block_info_target_type::neighbor) {
        lists = m_role_xips_mgr->get_rand_neighbors(self_addr, max_peers);
    } else if (target_type == enum_latest_block_info_target_type::archive) {
        lists = m_role_xips_mgr->get_rand_archives(self_addr, max_peers);
    }

    if (lists.empty()) {
        xsync_warn("[xsync_sender_t] can't send latest block info due to get 0 peers self xip : %s target_type:%d", self_addr.to_string().c_str(), target_type);
        return;
    }

    for (auto& target_addr : lists) {
        xsync_dbg("[xsync_sender_t] send latest block info %s -> %s", self_addr.to_string().c_str(), target_addr.to_string().c_str());
        XMETRICS_COUNTER_INCREMENT("sync_pkgs_latest_block_info_send", 1);
        XMETRICS_COUNTER_INCREMENT("sync_bytes_latest_block_info_send", msg.payload().size());
        XMETRICS_COUNTER_INCREMENT("sync_pkgs_out", 1);
        XMETRICS_COUNTER_INCREMENT("sync_bytes_out", msg.payload().size());
        m_vhost->send(msg, self_addr, target_addr);
    }
}

void xsync_sender_t::send_latest_block_info_to_target(const std::vector<xlatest_block_info_t> &info_list, const vnetwork::xvnode_address_t &self_addr, const vnetwork::xvnode_address_t &target_addr) {

    base::xstream_t stream(base::xcontext_t::instance());
    auto header = make_object_ptr<xsync_message_header_t>(RandomUint64());
    header->serialize_to(stream);
    auto body = make_object_ptr<xsync_message_latest_block_info_t>(info_list);
    body->serialize_to(stream);
    vnetwork::xmessage_t _msg = vnetwork::xmessage_t({stream.data(), stream.data() + stream.size()}, xmessage_id_sync_latest_block_info);

    xmessage_t msg;
    xmessage_pack_t::pack_message(_msg, ((int) _msg.payload().size()) >= m_min_compress_threshold, msg);

    XMETRICS_COUNTER_INCREMENT("sync_pkgs_latest_block_info_send", 1);
    XMETRICS_COUNTER_INCREMENT("sync_bytes_latest_block_info_send", msg.payload().size());
    XMETRICS_COUNTER_INCREMENT("sync_pkgs_out", 1);
    XMETRICS_COUNTER_INCREMENT("sync_bytes_out", msg.payload().size());

    xsync_dbg("[xsync_sender_t] send latest block info target %s -> %s", self_addr.to_string().c_str(), target_addr.to_string().c_str());
    m_vhost->send(msg, self_addr, target_addr);
}

void xsync_sender_t::send_get_latest_blocks(const std::string &address, const std::vector<xlatest_block_item_t> &list, const vnetwork::xvnode_address_t &self_addr, const vnetwork::xvnode_address_t &target_addr) {
    base::xstream_t stream(base::xcontext_t::instance());
    auto header = make_object_ptr<xsync_message_header_t>(RandomUint64());
    header->serialize_to(stream);
    auto body = make_object_ptr<xsync_message_get_latest_blocks_t>(address, list);
    body->serialize_to(stream);
    vnetwork::xmessage_t _msg = vnetwork::xmessage_t({stream.data(), stream.data() + stream.size()}, xmessage_id_sync_get_latest_blocks);

    xmessage_t msg;
    xmessage_pack_t::pack_message(_msg, ((int) _msg.payload().size()) >= m_min_compress_threshold, msg);

    XMETRICS_COUNTER_INCREMENT("sync_pkgs_get_latest_block_send", 1);
    XMETRICS_COUNTER_INCREMENT("sync_pkgs_out", 1);
    XMETRICS_COUNTER_INCREMENT("sync_bytes_out", msg.payload().size());

    xsync_dbg("[xsync_sender_t] send get latest block %s -> %s", self_addr.to_string().c_str(), target_addr.to_string().c_str());

    m_vhost->send(msg, self_addr, target_addr);
}

void xsync_sender_t::send_latest_blocks(const std::vector<data::xblock_ptr_t> &blocks, const vnetwork::xvnode_address_t &self_addr, const vnetwork::xvnode_address_t &target_addr) {
    base::xstream_t stream(base::xcontext_t::instance());
    auto header = make_object_ptr<xsync_message_header_t>(RandomUint64());
    header->serialize_to(stream);
    auto body = make_object_ptr<xsync_message_latest_blocks_t>(blocks);
    body->serialize_to(stream);
    vnetwork::xmessage_t _msg = vnetwork::xmessage_t({stream.data(), stream.data() + stream.size()}, xmessage_id_sync_latest_blocks);

    xmessage_t msg;
    xmessage_pack_t::pack_message(_msg, ((int) _msg.payload().size()) >= m_min_compress_threshold, msg);

    XMETRICS_COUNTER_INCREMENT("sync_pkgs_latest_block_send", 1);
    XMETRICS_COUNTER_INCREMENT("sync_pkgs_out", 1);
    XMETRICS_COUNTER_INCREMENT("sync_bytes_out", msg.payload().size());

    xsync_dbg("[xsync_sender_t] send latest block %s -> %s", self_addr.to_string().c_str(), target_addr.to_string().c_str());

    m_vhost->send(msg, self_addr, target_addr);
}

static std::vector<vnetwork::xvnode_address_t> calc_target_node_list(uint32_t count, uint32_t positon, const std::vector<vnetwork::xvnode_address_t> &nodes) {

    assert(positon < count);

    uint32_t base_val = 0;
    std::vector<vnetwork::xvnode_address_t> pieces;

    uint32_t size = nodes.size();
    for (uint32_t i=0; i<size; i++) {
        if (i%count == positon) {
            pieces.push_back(nodes[i]);
        }
    }

    return pieces;
}

void xsync_sender_t::broadcast_newblock(const xblock_ptr_t &block, const vnetwork::xvnode_address_t& self_xip, uint32_t self_position, uint32_t deliver_node_count){

    base::xstream_t stream(base::xcontext_t::instance());
    auto header = make_object_ptr<xsync_message_header_t>(RandomUint64());
    header->serialize_to(stream);
    auto body = make_object_ptr<xsync_message_newblock_t>(block);
    body->serialize_to(stream);
    vnetwork::xmessage_t _msg = vnetwork::xmessage_t({stream.data(), stream.data() + stream.size()}, xmessage_id_sync_newblock);

    base::xvblock_t* vblock = block.get();

    std::vector<vnetwork::xvnode_address_t> vrf_archive_list = m_role_xips_mgr->get_vrf_sqrt_archive(vblock->get_block_hash());
    std::vector<vnetwork::xvnode_address_t> target_nodes = calc_target_node_list(deliver_node_count, self_position, vrf_archive_list);

    if (target_nodes.empty()) {
        xsync_warn("[xsync_sender_t] send newblock failed. vrf_archive_size:%d self_xip:%s %s", vrf_archive_list.size(), self_xip.to_string().c_str(), vblock->dump().c_str());
        return;
    }

    xmessage_t msg;
    xmessage_pack_t::pack_message(_msg, ((int) _msg.payload().size()) >= m_min_compress_threshold, msg);

#if 0
    for (auto &it: vrf_archive_list) {
        xsync_warn("[xsync_sender_t] newblock position vrf %s %s", it.to_string().c_str(), vblock->dump().c_str());
    }

    std::vector<vnetwork::xvnode_address_t> all_archive_list = m_role_xips_mgr->get_archive_list();
    for (auto &it: all_archive_list) {
        xsync_warn("[xsync_sender_t] newblock position all %s %s", it.to_string().c_str(), vblock->dump().c_str());
    }
#endif

    for (auto& addr : target_nodes) {
        xsync_dbg("[xsync_sender_t] send newblock %s -> %s : %s", self_xip.to_string().c_str(), addr.to_string().c_str(), vblock->dump().c_str());
        XMETRICS_COUNTER_INCREMENT("sync_pkgs_newblock_send", 1);
        XMETRICS_COUNTER_INCREMENT("sync_bytes_newblock_send", msg.payload().size());
        XMETRICS_COUNTER_INCREMENT("sync_pkgs_out", 1);
        XMETRICS_COUNTER_INCREMENT("sync_bytes_out", msg.payload().size());
        m_vhost->send(msg, self_xip, addr);
    }
}

void xsync_sender_t::broadcast_newblockhash(const xblock_ptr_t &block, const vnetwork::xvnode_address_t& self_xip) {

    base::xstream_t stream(base::xcontext_t::instance());
    auto header = make_object_ptr<xsync_message_header_t>(RandomUint64());
    header->serialize_to(stream);
    auto body = make_object_ptr<xsync_message_newblockhash_t>(block->get_account(), block->get_height(), block->get_viewid());
    body->serialize_to(stream);
    vnetwork::xmessage_t _msg = vnetwork::xmessage_t({stream.data(), stream.data() + stream.size()}, xmessage_id_sync_newblockhash);

    base::xvblock_t* vblock = block.get();

    std::vector<vnetwork::xvnode_address_t> vrf_archive_list = m_role_xips_mgr->get_vrf_sqrt_archive(vblock->get_block_hash());

    uint32_t positon = UINT32_MAX;
    for (uint32_t i=0; i<vrf_archive_list.size(); i++) {
        if ((vrf_archive_list[i] == self_xip)
        || vrf_archive_list[i].account_address()==self_xip.account_address()) { // avoid elect round changed
            positon = i;
            break;
        }
    }

    if (positon == UINT32_MAX) {
        xsync_warn("[xsync_sender_t] newblockhash position error self_xip: %s %s", self_xip.to_string().c_str(), vblock->dump().c_str());
#if 0
        for (auto &it: vrf_archive_list) {
            xsync_warn("[xsync_sender_t] newblockhash position error vrf %s %s", it.to_string().c_str(), vblock->dump().c_str());
        }
        std::vector<vnetwork::xvnode_address_t> all_archive_list = m_role_xips_mgr->get_archive_list();
        for (auto &it: all_archive_list) {
            xsync_warn("[xsync_sender_t] newblockhash position error all %s %s", it.to_string().c_str(), vblock->dump().c_str());
        }
#endif
        return;
    }

    std::vector<vnetwork::xvnode_address_t> all_archive_list = m_role_xips_mgr->get_archive_list();
    std::vector<vnetwork::xvnode_address_t> target_nodes = calc_target_node_list(vrf_archive_list.size(), positon, all_archive_list);

    if (target_nodes.empty()) {
        xsync_warn("[xsync_sender_t] send newblockhash failed. self_xip:%s %s", self_xip.to_string().c_str(), vblock->dump().c_str());
        return;
    }

    std::set<vnetwork::xvnode_address_t> newblock_node_set;
    for (auto &it: vrf_archive_list)
        newblock_node_set.insert(it);

    xmessage_t msg;
    xmessage_pack_t::pack_message(_msg, ((int) _msg.payload().size()) >= m_min_compress_threshold, msg);

    for (auto& addr : target_nodes) {

        // should not send to newblock recv node
        auto it = newblock_node_set.find(addr);
        if (it != newblock_node_set.end()) {
            xsync_dbg("[xsync_sender_t] send newblockhash(ignore) %s -> %s : %s", self_xip.to_string().c_str(), addr.to_string().c_str(), vblock->dump().c_str());
            continue;
        }

        XMETRICS_COUNTER_INCREMENT("sync_pkgs_newblockhash_send", 1);
        XMETRICS_COUNTER_INCREMENT("sync_bytes_newblockhash_send", msg.payload().size());
        XMETRICS_COUNTER_INCREMENT("sync_pkgs_out", 1);
        XMETRICS_COUNTER_INCREMENT("sync_bytes_out", msg.payload().size());

        xsync_dbg("[xsync_sender_t] send newblockhash %s -> %s : %s", self_xip.to_string().c_str(), addr.to_string().c_str(), vblock->dump().c_str());
        m_vhost->send(msg, self_xip, addr);
    }
}

NS_END2
