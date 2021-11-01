// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xmbus/xevent_common.h"
#include "xmbus/xmessage_bus.h"
#include "xvnetwork/xvnetwork_driver_face.h"
#include "xmetrics/xmetrics.h"
#include "xsync/xmessage_pack.h"
#include "xvnetwork/xvhost_face.h"
#include "xsync/xrole_xips_manager.h"
#include "xsync/xsync_message.h"
#include "xsyncbase/xmessage_ids.h"

NS_BEG2(top, sync)

enum enum_gossip_target_type {
    enum_gossip_target_type_neighbor,
    enum_gossip_target_type_parent,
    enum_gossip_target_type_archive,
};

enum enum_latest_block_info_target_type {
    neighbor,
    archive,
};

class xsync_sender_t {
public:
    virtual ~xsync_sender_t() {}

    xsync_sender_t(std::string vnode_id, const observer_ptr<vnetwork::xvhost_face_t> &vhost, 
        xrole_xips_manager_t *role_xips_mgr, int min_compress_threshold = DEFAULT_MIN_COMPRESS_THRESHOLD);
    void send_gossip(const std::vector<xgossip_chain_info_ptr_t> &info_list, const xbyte_buffer_t &bloom_data, const vnetwork::xvnode_address_t& self_xip, uint32_t max_peers, enum_gossip_target_type target_type);
    void send_gossip_to_target(const std::vector<xgossip_chain_info_ptr_t> &info_list, const xbyte_buffer_t &bloom_data, const vnetwork::xvnode_address_t& self_xip, const vnetwork::xvnode_address_t& target);
    void send_frozen_gossip(const std::vector<xgossip_chain_info_ptr_t> &info_list, const xbyte_buffer_t &bloom_data, const vnetwork::xvnode_address_t& self_xip);
    void send_frozen_gossip_to_target(const std::vector<xgossip_chain_info_ptr_t> &info_list, const xbyte_buffer_t &bloom_data, const vnetwork::xvnode_address_t& self_xip, const vnetwork::xvnode_address_t& target);
    bool send_get_blocks(const std::string &address, uint64_t start_height, uint32_t count, const vnetwork::xvnode_address_t &self_addr, const vnetwork::xvnode_address_t &target_addr);
    void send_blocks(xsync_msg_err_code_t code, const std::string &address, const std::vector<data::xblock_ptr_t> &blocks, const vnetwork::xvnode_address_t& self_addr, const vnetwork::xvnode_address_t& target_addr);

    void send_get_on_demand_blocks(const std::string &address, uint64_t start_height, uint32_t count, bool is_consensus, const vnetwork::xvnode_address_t &self_addr, const vnetwork::xvnode_address_t &target_addr);
    void send_on_demand_blocks(const std::vector<data::xblock_ptr_t> &blocks, const common::xmessage_id_t msgid, const std::string metric_key, const vnetwork::xvnode_address_t &self_addr, const vnetwork::xvnode_address_t &target_addr);

    void send_get_on_demand_blocks_with_proof(const std::string &address, uint64_t start_height, uint32_t count, bool is_consensus, bool unit_proof, const vnetwork::xvnode_address_t &self_addr, const vnetwork::xvnode_address_t &target_addr);
    void send_on_demand_blocks_with_proof(const std::vector<data::xblock_ptr_t> &blocks, const common::xmessage_id_t msgid, const std::string metric_key, const vnetwork::xvnode_address_t &self_addr, const vnetwork::xvnode_address_t &target_addr, const std::string& unit_proof_str = "");

    void send_broadcast_chain_state(const std::vector<xchain_state_info_t> &info_list, const vnetwork::xvnode_address_t &self_addr, const vnetwork::xvnode_address_t &target_addr);
    void send_response_chain_state(const std::vector<xchain_state_info_t> &info_list, const vnetwork::xvnode_address_t &self_addr, const vnetwork::xvnode_address_t &target_addr);
    void send_frozen_broadcast_chain_state(const std::vector<xchain_state_info_t> &info_list, const vnetwork::xvnode_address_t &self_addr);
    void send_frozen_response_chain_state(const std::vector<xchain_state_info_t> &info_list, const vnetwork::xvnode_address_t &self_addr, const vnetwork::xvnode_address_t &target_addr);
    void send_cross_cluster_chain_state(const std::vector<xchain_state_info_t> &info_list, const vnetwork::xvnode_address_t &self_addr, const vnetwork::xvnode_address_t &target_addr);

    void push_newblock(const data::xblock_ptr_t &block, const vnetwork::xvnode_address_t& self_addr, const vnetwork::xvnode_address_t& target_addr);
    void push_newblockhash(const data::xblock_ptr_t &block, const vnetwork::xvnode_address_t& self_addr, const vnetwork::xvnode_address_t& target_addr);
    void broadcast_newblockhash(const data::xblock_ptr_t &block, const vnetwork::xvnode_address_t& self_addr, const vnetwork::xvnode_address_t& target_addr);

    void send_get_blocks_by_hashes(const std::vector<xblock_hash_t> &hashes, const vnetwork::xvnode_address_t &self_addr, const vnetwork::xvnode_address_t &target_addr);
    void send_blocks_by_hashes(const std::vector<data::xblock_ptr_t> &blocks, const vnetwork::xvnode_address_t &self_addr, const vnetwork::xvnode_address_t &target_addr);

    void send_chain_snapshot(const xsync_message_chain_snapshot_t &chain_snapshot, const common::xmessage_id_t msgid, const vnetwork::xvnode_address_t &self_addr, const vnetwork::xvnode_address_t &target_addr);
    bool send_chain_snapshot_meta(const xsync_message_chain_snapshot_meta_t &chain_snapshot_meta, const common::xmessage_id_t msgid, const vnetwork::xvnode_address_t &self_addr, const vnetwork::xvnode_address_t &target_addr);

    void send_get_on_demand_by_hash_blocks(const std::string &address, const std::string &hash, const vnetwork::xvnode_address_t &self_addr, const vnetwork::xvnode_address_t &target_addr);
    void send_on_demand_by_hash_blocks(const std::vector<data::xblock_ptr_t> &blocks, const vnetwork::xvnode_address_t &self_addr, const vnetwork::xvnode_address_t &target_addr);
    bool send_message(xobject_ptr_t<basic::xserialize_face_t> serializer, const common::xmessage_id_t msgid, const std::string metric_key, const vnetwork::xvnode_address_t &self_addr, const vnetwork::xvnode_address_t &target_addr);
protected:
    std::string m_vnode_id;
    observer_ptr<vnetwork::xvhost_face_t> m_vhost{};
    xrole_xips_manager_t *m_role_xips_mgr{};
    int m_min_compress_threshold{};
};

NS_END2
