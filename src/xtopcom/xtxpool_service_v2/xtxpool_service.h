// Copyright (c) 2017-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xobject.h"
#include "xbasic/xmemory.hpp"
#include "xbasic/xserialize_face.h"
#include "xchain_timer/xchain_timer_face.h"
#include "xdata/xblock.h"
#include "xdata/xtransaction.h"
#include "xtxpool_service_v2/xcons_utl.h"
#include "xtxpool_service_v2/xtxpool_service_face.h"
#include "xtxpool_service_v2/xtxpool_svc_para.h"
#include "xtxpool_v2/xtxpool_face.h"
#include "xtxpool_service_v2/xreceipt_sync.h"

#include <set>
#include <string>
#include <vector>
#include <map>

NS_BEG2(top, xtxpool_service_v2)
using xtxpool_service_v2::xcons_utl;

class xtxpool_service final
  : public xtxpool_service_face
  , public std::enable_shared_from_this<xtxpool_service> {
public:
    explicit xtxpool_service(const observer_ptr<router::xrouter_face_t> & router, const observer_ptr<xtxpool_svc_para_t> & para);

public:
    bool start(const xvip2_t & xip) override;
    bool fade(const xvip2_t & xip) override;
    void set_params(const xvip2_t & xip, const std::shared_ptr<vnetwork::xvnetwork_driver_face_t> & vnet_driver) override;
    bool is_running() const override;
    bool is_receipt_sender(const xtable_id_t & tableid) const override;
    bool is_send_receipt_role() const override {return m_is_send_receipt_role;}
    bool table_boundary_equal_to(std::shared_ptr<xtxpool_service_face> & service) const override;
    void get_service_table_boundary(base::enum_xchain_zone_index & zone_id, uint32_t & fount_table_id, uint32_t & back_table_id) const override;
    void resend_receipts(uint64_t now) override;
    int32_t request_transaction_consensus(const data::xtransaction_ptr_t & tx, bool local) override;
    xcons_transaction_ptr_t query_tx(const std::string & account, const uint256_t & hash) const override {
        return nullptr;
    };
    void pull_lacking_receipts(uint64_t now, xcovered_tables_t & covered_tables) override;
    xvip2_t get_cluster_xip() const override;

private:
    bool is_belong_to_service(xtable_id_t tableid) const;
    void on_message_receipt(vnetwork::xvnode_address_t const & sender, vnetwork::xmessage_t const & message);
    void on_message_unit_receipt(vnetwork::xvnode_address_t const & sender, vnetwork::xmessage_t const & message);
    void on_message_push_receipt_received(vnetwork::xvnode_address_t const & sender, vnetwork::xmessage_t const & message);
    void on_message_pull_recv_receipt_received(vnetwork::xvnode_address_t const & sender, vnetwork::xmessage_t const & message);
    void on_message_pull_confirm_receipt_received(vnetwork::xvnode_address_t const & sender, vnetwork::xmessage_t const & message);
    void send_receipt_real(const data::xcons_transaction_ptr_t & cons_tx);
    bool has_receipt_right(const xcons_transaction_ptr_t & cons_tx, uint32_t resend_time) const;
    void forward_broadcast_message(const vnetwork::xvnode_address_t & addr, const vnetwork::xmessage_t & message);
    void send_receipt_retry(xcons_transaction_ptr_t & cons_tx);
    xcons_transaction_ptr_t create_confirm_tx_by_hash(const uint256_t & hash);
    xcons_transaction_ptr_t get_confirmed_tx(const uint256_t & hash);
    void send_pull_receipts_of_confirm(xreceipt_pull_confirm_receipt_t & pulled_receipt);
    void send_pull_receipts_of_recv(xreceipt_pull_recv_receipt_t & pulled_receipt);
    void send_push_receipts(xreceipt_push_t &pushed_receipt, vnetwork::xvnode_address_t const & target);
    void send_receipt_sync_msg(const vnetwork::xmessage_t & msg, const std::string & target_table_addr);

private:
    xvip2_t m_xip;
    std::shared_ptr<vnetwork::xvnetwork_driver_face_t> m_vnet_driver;
    observer_ptr<router::xrouter_face_t> m_router;
    observer_ptr<xtxpool_svc_para_t> m_para;
    bool m_is_send_receipt_role{false};
    uint16_t m_cover_front_table_id;  // [m_front_table_id,m_back_table_id) is the scope for this service
    uint16_t m_cover_back_table_id;   // present empty if m_front_table_id == m_back_table_id
    base::enum_xchain_zone_index m_zone_index;
    uint16_t m_node_id;
    uint16_t m_shard_size;
    volatile bool m_running{false};
    std::string m_vnetwork_str;
    std::unordered_map<uint16_t, uint64_t> m_table_db_event_height_map;
};

NS_END2
