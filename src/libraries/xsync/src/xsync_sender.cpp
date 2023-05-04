// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xbase/xutl.h"
#include "xcommon/xnode_type.h"
#include "xsync/xsync_sender.h"
#include "xsync/xsync_log.h"
#include "xmetrics/xmetrics.h"
#include "xdata/xdatautil.h"
#include "xcommon/xmessage_id.h"
#include "xsync/xsync_util.h"
#include <inttypes.h> 

NS_BEG2(top, sync)

using namespace data;

xsync_sender_t::xsync_sender_t(std::string vnode_id, const observer_ptr<vnetwork::xvhost_face_t> &vhost,
            xrole_xips_manager_t *role_xips_mgr, xsync_store_face_t *sync_store,xsync_session_manager_t *session_mgr,  int min_compress_threshold):
m_vnode_id(vnode_id),
m_vhost(vhost),
m_role_xips_mgr(role_xips_mgr),
m_sync_store(sync_store),
m_session_mgr(session_mgr),
m_min_compress_threshold(min_compress_threshold) {
}

void xsync_sender_t::send_gossip(const std::vector<xgossip_chain_info_ptr_t> &info_list, const xbyte_buffer_t &bloom_data, const vnetwork::xvnode_address_t& self_xip, uint32_t max_peers, enum_gossip_target_type target_type) {

    base::xstream_t stream(base::xcontext_t::instance());
    auto header = make_object_ptr<xsync_message_header_t>(RandomUint64());
    header->serialize_to(stream);
    auto body = make_object_ptr<xsync_message_gossip_t>(info_list, bloom_data);
    body->serialize_to(stream);
    vnetwork::xmessage_t _msg = vnetwork::xmessage_t({stream.data(), stream.data() + stream.size()}, xmessage_id_sync_gossip);

    vnetwork::xmessage_t msg;
    xmessage_pack_t::pack_message(_msg, ((int) _msg.payload().size()) >= m_min_compress_threshold, msg, m_sync_store->is_support_big_pack_forked());

    std::vector<vnetwork::xvnode_address_t> lists;

    if (target_type == enum_gossip_target_type_neighbor) {
        lists = m_role_xips_mgr->get_rand_neighbors(self_xip, max_peers);
    } else if (target_type == enum_gossip_target_type_parent) {
        lists = m_role_xips_mgr->get_rand_parents(self_xip, max_peers);
    } else if (target_type == enum_gossip_target_type_archive) {
        lists = m_role_xips_mgr->get_rand_archives(max_peers);
    }

    for (auto& addr : lists) {
        xsync_dbg("xsync_sender_t send gossip %s -> %s", self_xip.to_string().c_str(), addr.to_string().c_str());
        std::error_code ec;
        m_vhost->send_to(self_xip, addr, msg, ec);
        if (ec) {
            XMETRICS_GAUGE(metrics::xsync_sender_net_succ, 0);
            xwarn("send_message error: gossip_send msg_hash: %" PRIx64" src:%s dst:%s  category %s; err msg %s",  msg.hash(), 
                 self_xip.to_string().c_str(), addr.to_string().c_str(), ec.category().name(), ec.message().c_str());
        }else{
            XMETRICS_GAUGE(metrics::xsync_sender_net_succ, 1);
        }
    }
}

void xsync_sender_t::send_gossip_to_target(const std::vector<xgossip_chain_info_ptr_t> &info_list, const xbyte_buffer_t &bloom_data, const vnetwork::xvnode_address_t& self_xip, const vnetwork::xvnode_address_t& target) {
    auto body = make_object_ptr<xsync_message_gossip_t>(info_list, bloom_data);
    send_message(body, xmessage_id_sync_gossip, "gossip_to_target", self_xip, target);
}

void xsync_sender_t::send_frozen_gossip(const std::vector<xgossip_chain_info_ptr_t> &info_list, const xbyte_buffer_t &bloom_data, const vnetwork::xvnode_address_t& self_xip) {

    base::xstream_t stream(base::xcontext_t::instance());
    auto header = make_object_ptr<xsync_message_header_t>(RandomUint64());
    header->serialize_to(stream);
    auto body = make_object_ptr<xsync_message_gossip_t>(info_list, bloom_data);
    body->serialize_to(stream);
    vnetwork::xmessage_t _msg = vnetwork::xmessage_t({stream.data(), stream.data() + stream.size()}, xmessage_id_sync_frozen_gossip);

    vnetwork::xmessage_t msg;
    xmessage_pack_t::pack_message(_msg, ((int) _msg.payload().size()) >= m_min_compress_threshold, msg, m_sync_store->is_support_big_pack_forked());

    common::xnode_address_t addr(common::build_frozen_sharding_address(self_xip.network_id()));
    xsync_dbg("xsync_sender_t send frozen gossip src %s dst %s", self_xip.to_string().c_str(), addr.to_string().c_str());
    //m_vhost->send(msg, self_xip, addr);
    std::error_code ec;
    m_vhost->broadcast(self_xip, addr, msg, ec);
    if (ec) {
        XMETRICS_GAUGE(metrics::xsync_sender_net_succ, 0);
        xwarn("send_message error: send_frozen_gossip msg_hash: %" PRIx64" src:%s dst:%s  category %s; err msg %s",  msg.hash(), 
                self_xip.to_string().c_str(), addr.to_string().c_str(), ec.category().name(), ec.message().c_str());
    }else{
        XMETRICS_GAUGE(metrics::xsync_sender_net_succ, 1);
    }
    xsync_dbg("xsync_sender_t::gossip frozen gossip ec=%d", ec);
}

void xsync_sender_t::send_frozen_gossip_to_target(const std::vector<xgossip_chain_info_ptr_t> &info_list, const xbyte_buffer_t &bloom_data, const vnetwork::xvnode_address_t& self_xip, const vnetwork::xvnode_address_t& target) {
    auto body = make_object_ptr<xsync_message_gossip_t>(info_list, bloom_data);
    send_message(body, xmessage_id_sync_frozen_gossip, "frozen_gossip_to_target", self_xip, target);
}

bool xsync_sender_t::send_get_blocks(const std::string &address,
            uint64_t start_height, uint32_t count,
            const vnetwork::xvnode_address_t &self_addr,
            const vnetwork::xvnode_address_t &target_addr) {
        uint32_t request_option = SYNC_MSG_OPTION_SET(enum_sync_block_request_ontime, enum_sync_block_by_height, enum_sync_data_all, enum_sync_block_object_xvblock, 0);
        auto body = make_object_ptr<xsync_msg_block_request_t>(address, request_option, start_height, count, "");
        if(m_session_mgr->sync_block_request_insert(body)) {
            xsync_dbg("send_get_blocks sessionid(%lx) %s send to %s", body->get_sessionID(), self_addr.to_string().c_str(), target_addr.to_string().c_str());
            return send_message(body, xmessage_id_sync_block_request, "send_get_blocks", self_addr, target_addr);
        }
        return false;
}

void xsync_sender_t::send_broadcast_chain_state(const std::vector<xchain_state_info_t> &info_list, const vnetwork::xvnode_address_t &self_addr, const vnetwork::xvnode_address_t &target_addr) {
    auto body = make_object_ptr<xsync_message_chain_state_info_t>(info_list);
    send_message(body, xmessage_id_sync_broadcast_chain_state, "broadcast_chain_state", self_addr, target_addr);
    xsync_dbg("xsync_sender_t send broadcast_chain_state count(%u) src %s dst %s", info_list.size(), self_addr.to_string().c_str(), target_addr.to_string().c_str());
}

void xsync_sender_t::send_response_chain_state(const std::vector<xchain_state_info_t> &info_list, const vnetwork::xvnode_address_t &self_addr, const vnetwork::xvnode_address_t &target_addr) {
    auto body = make_object_ptr<xsync_message_chain_state_info_t>(info_list);
    send_message(body, xmessage_id_sync_response_chain_state, "response_chain_state", self_addr, target_addr);
    xsync_dbg("xsync_sender_t send response_chain_state count(%u) src %s dst %s", info_list.size(), self_addr.to_string().c_str(), target_addr.to_string().c_str());
}

void xsync_sender_t::send_frozen_response_chain_state(const std::vector<xchain_state_info_t> &info_list, const vnetwork::xvnode_address_t &self_addr, const vnetwork::xvnode_address_t &target_addr) {
    auto body = make_object_ptr<xsync_message_chain_state_info_t>(info_list);
    send_message(body, xmessage_id_sync_frozen_response_chain_state, "frozen_response_chain_state", self_addr, target_addr);
    xsync_dbg("xsync_sender_t send frozen_response_chain_state count(%u) src %s dst %s", info_list.size(), self_addr.to_string().c_str(), target_addr.to_string().c_str());
}

void xsync_sender_t::send_cross_cluster_chain_state(const std::vector<xchain_state_info_t> &info_list, const vnetwork::xvnode_address_t &self_addr, const vnetwork::xvnode_address_t &target_addr) {
    auto body = make_object_ptr<xsync_message_chain_state_info_t>(info_list);
    send_message(body, xmessage_id_sync_cross_cluster_chain_state, "cross_cluster_chain_state", self_addr, target_addr);
    xsync_dbg("xsync_sender_t send cross_cluster_chain_state count(%u) src %s dst %s", info_list.size(), self_addr.to_string().c_str(), target_addr.to_string().c_str());
}

void xsync_sender_t::push_newblock(const data::xblock_ptr_t &block, const vnetwork::xvnode_address_t& self_addr, const vnetwork::xvnode_address_t& target_addr) {
    std::vector<data::xblock_ptr_t> block_vec{block};
    uint32_t request_option = SYNC_MSG_OPTION_SET(enum_sync_block_request_push, 0, enum_sync_data_all, enum_sync_block_object_xvblock, 0);
    auto block_str_vec = convert_blocks_to_stream(enum_sync_data_all, block_vec);
    auto body = make_object_ptr<xsync_msg_block_push_t>(block->get_account(), request_option, block_str_vec);
    xsync_dbg("xsync_sender_t push_newblock sessionid(%lx) accout %s height=%lu,viewid=%lu, send to %s", body->get_sessionID(), block->get_account().c_str(),
              block->get_height(), block->get_viewid(), self_addr.to_string().c_str(), target_addr.to_string().c_str());
    send_message(body, xmessage_id_sync_newblock_push, "newblock_push", self_addr, target_addr);
}

void xsync_sender_t::send_get_blocks_by_hashes(std::vector<xblock_hash_t>& hashes, const vnetwork::xvnode_address_t& self_addr, const vnetwork::xvnode_address_t& target_addr)
{

    base::xstream_t stream(base::xcontext_t::instance());
    SERIALIZE_CONTAINER(hashes)
    {
            item.serialize_to(stream);
    }
    std::string hashs_param((char*)stream.data(), stream.size());
    uint32_t request_option = SYNC_MSG_OPTION_SET(enum_sync_block_request_ontime, enum_sync_block_by_hash, enum_sync_data_all, enum_sync_block_object_xvblock, 0);
    // address, 1 ,1 is not used
    auto body = make_object_ptr<xsync_msg_block_request_t>(hashes[0].address, request_option, (uint64_t)1, (uint32_t)1, hashs_param);
    if (m_session_mgr->sync_block_request_insert(body)) {
        xsync_dbg("send_get_blocks_by_hashes sessionid(%lx) %s send to %s", body->get_sessionID(), self_addr.to_string().c_str(), target_addr.to_string().c_str());
        send_message(body, xmessage_id_sync_block_request, "send_get_blocks_by_hashes", self_addr, target_addr);
    }
}

void xsync_sender_t::send_get_on_demand_by_hash_blocks(const std::string& address, const std::string& hash, const vnetwork::xvnode_address_t& self_addr, const vnetwork::xvnode_address_t& target_addr)
{
    uint32_t request_option = SYNC_MSG_OPTION_SET(enum_sync_block_request_demand, enum_sync_block_by_txhash, enum_sync_data_all, enum_sync_block_object_xvblock, 0);
    auto body = make_object_ptr<xsync_msg_block_request_t>(address, request_option, (uint64_t)1, (uint32_t)1, hash);
    if (m_session_mgr->sync_block_request_insert(body)) {
        xsync_dbg("send_get_on_demand_by_hash_blocks sessionid(%lx) %s send to %s", body->get_sessionID(), self_addr.to_string().c_str(), target_addr.to_string().c_str());
        send_message(body, xmessage_id_sync_block_request, "send_get_on_demand_by_hash_blocks", self_addr, target_addr);
    }
}

void xsync_sender_t::send_query_archive_height(const std::vector<xchain_state_info_t>& info_list,
    const vnetwork::xvnode_address_t &self_addr, const vnetwork::xvnode_address_t &target_addr) {
    auto body = make_object_ptr<xsync_message_chain_state_info_t>(info_list);
    send_message(body, xmessage_id_sync_query_archive_height, "query_archive_height", self_addr, target_addr);
    xsync_dbg("xsync_sender_t send_query_archive_height: %d, src %s dst %s", info_list.size(), self_addr.to_string().c_str(), target_addr.to_string().c_str());
}

void xsync_sender_t::send_archive_height_list(std::vector<xchain_state_info_t>& info_list,
    const vnetwork::xvnode_address_t& self_addr, const vnetwork::xvnode_address_t& target_addr)
{
    base::xstream_t stream(base::xcontext_t::instance());
    SERIALIZE_CONTAINER(info_list)
    {
        item.serialize_to(stream);
    }
    std::string hashs_param((char*)stream.data(), stream.size());

    uint32_t request_option = SYNC_MSG_OPTION_SET(enum_sync_block_request_ontime, enum_sync_block_by_height_lists, enum_sync_data_all, enum_sync_block_object_xvblock, 0);
    // address, 1 ,1 is not used
    auto body = make_object_ptr<xsync_msg_block_request_t>(info_list[0].address, request_option, (uint64_t)1, (uint32_t)1, hashs_param);
    send_message(body, xmessage_id_sync_block_request, "send_archive_height_list", self_addr, target_addr);
    xsync_dbg("send_archive_height_list sessionid(%lx) list: %d ,%s send to %s", body->get_sessionID(), info_list.size(), self_addr.to_string().c_str(), target_addr.to_string().c_str());
}

void xsync_sender_t::send_get_on_demand_blocks_with_params(const std::string &address,
            uint64_t start_height, uint32_t count, bool is_consensus, const std::string & last_unit_hash,
            const vnetwork::xvnode_address_t &self_addr,
            const vnetwork::xvnode_address_t &target_addr) {

    uint32_t request_param = enum_sync_block_by_height; 
    if (!last_unit_hash.empty()) {
        request_param = enum_sync_block_by_hash; 
    }

    if (request_param == enum_sync_block_by_height && data::is_unit_address(common::xaccount_address_t { address })) {
        xwarn("send_get_on_demand_blocks_with_params  sync unit by height.accout:(%s) height(%ld) count(%d) last_unit_hash(%s)",
            address.c_str(), start_height, count, base::xstring_utl::to_hex(last_unit_hash).c_str());
    }

    uint32_t request_option = SYNC_MSG_OPTION_SET(enum_sync_block_request_demand, request_param, enum_sync_data_all, enum_sync_block_object_xvblock, (is_consensus == true ? 1: 0));
    auto body = make_object_ptr<xsync_msg_block_request_t>(address, request_option, start_height, count, last_unit_hash);
    if (m_session_mgr->sync_block_request_insert(body)) {
        send_message(body, xmessage_id_sync_block_request, "send_get_on_demand_blocks_with_params", self_addr, target_addr);
        xsync_dbg("send_get_on_demand_blocks_with_params sessionid(%lx) %s send to %s", body->get_sessionID(), self_addr.to_string().c_str(), target_addr.to_string().c_str());
    }
}

void xsync_sender_t::send_block_response(const xsync_msg_block_request_ptr_t& request_ptr, const std::vector<xblock_ptr_t> &vector_blocks, uint32_t response_extend_option, 
                            std::string extend_data, const vnetwork::xvnode_address_t& self_addr, const vnetwork::xvnode_address_t& target_addr) 
{
    auto block_str_vec = convert_blocks_to_stream(request_ptr->get_data_type(), vector_blocks); 
    auto body = make_object_ptr<xsync_msg_block_response_t>(request_ptr->get_sessionID(), request_ptr->get_address(), request_ptr->get_option(),
                 block_str_vec, response_extend_option, extend_data);

    if (!m_sync_store->is_support_big_pack_forked()) {
        send_message(body, xmessage_id_sync_block_response, "xmessage_id_sync_block_response", self_addr, target_addr);
    } else {
        send_message(body, xmessage_id_sync_block_response_bigpack, "xmessage_id_sync_block_response", self_addr, target_addr, true);
    }
    xsync_dbg("send_block_response sessionid(%lx) %s send to %s. block size(%ld), response_option(%ld), extend_data(%s). ", 
            request_ptr->get_sessionID(), self_addr.to_string().c_str(), target_addr.to_string().c_str(), vector_blocks.size(), response_extend_option, extend_data.c_str());
}

bool xsync_sender_t::send_message(
            const xobject_ptr_t<basic::xserialize_face_t> serializer,
            const common::xmessage_id_t msgid, 
            const std::string metric_key,
            const vnetwork::xvnode_address_t &self_addr,
            const vnetwork::xvnode_address_t &target_addr,
            bool  without_dataunit_serialize) {

    base::xstream_t stream(base::xcontext_t::instance());
    auto header = make_object_ptr<xsync_message_header_t>(RandomUint64());
    header->serialize_to(stream);
    if(!without_dataunit_serialize) {
        serializer->serialize_to(stream);
    } else {
        //no serialize dataunit to stream after forked.only sync_block_response use it now.
        serializer->do_write(stream);
    }
    
    vnetwork::xmessage_t _msg = vnetwork::xmessage_t({stream.data(), stream.data() + stream.size()}, msgid);

    vnetwork::xmessage_t msg;
    xmessage_pack_t::pack_message(_msg, ((int) _msg.payload().size()) >= m_min_compress_threshold, msg, m_sync_store->is_support_big_pack_forked());

    std::string bytes_metric_name = "sync_pkgs_" + metric_key + "_send";
    
    xsync_info("xsync_sender_t %s %s msg_id: %x msg_hash: %" PRIx64" send to %s", self_addr.to_string().c_str(), metric_key.c_str(), msg.id(),  msg.hash(),target_addr.to_string().c_str());
    std::error_code ec;
    if (self_addr.zone_id() == common::xfrozen_zone_id)
        m_vhost->send_to_through_frozen(self_addr, target_addr, msg, ec);
    else
        m_vhost->send_to(self_addr, target_addr, msg, ec);
    if (ec) {
        XMETRICS_GAUGE(metrics::xsync_sender_net_succ, 0);
        xwarn("send_message error: message:%s msg_hash: %" PRIx64" src:%s dst:%s  category %s; err msg %s", metric_key.c_str(), msg.hash(), 
            self_addr.to_string().c_str(), target_addr.to_string().c_str(), ec.category().name(), ec.message().c_str());
        return false;
    }
     XMETRICS_GAUGE(metrics::xsync_sender_net_succ, 1);
    return true;
}

NS_END2
