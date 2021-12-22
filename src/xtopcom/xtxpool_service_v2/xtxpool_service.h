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
#include "xtxpool_service_v2/xreceipt_sync.h"
#include "xtxpool_service_v2/xtxpool_service_face.h"
#include "xtxpool_service_v2/xtxpool_svc_para.h"
#include "xtxpool_v2/xtxpool_face.h"
#include "xvledger/xvpropertyprove.h"

#include <map>
#include <set>
#include <string>
#include <vector>

NS_BEG2(top, xtxpool_service_v2)
using xtxpool_service_v2::xcons_utl;

struct table_info {
    table_info(uint64_t last_property_height, const base::xvproperty_prove_ptr_t & property_prove)
      : m_last_property_height(last_property_height), m_property_prove(property_prove) {
    }
    uint64_t m_last_property_height{0};
    base::xvproperty_prove_ptr_t m_property_prove{nullptr};
};

enum enum_txpool_service_status
{
    enum_txpool_service_status_running = 0,
    enum_txpool_service_status_faded = 1,
    enum_txpool_service_status_not_run = 2,
};

class xtxpool_service final
  : public xtxpool_service_face
  , public std::enable_shared_from_this<xtxpool_service> {
public:
    explicit xtxpool_service(const observer_ptr<router::xrouter_face_t> & router, const observer_ptr<xtxpool_svc_para_t> & para);

public:
    bool start(const xvip2_t & xip) override;
    bool unreg(const xvip2_t & xip) override;
    bool fade(const xvip2_t & xip) override;
    void set_params(const xvip2_t & xip, const std::shared_ptr<vnetwork::xvnetwork_driver_face_t> & vnet_driver) override;
    // bool is_receipt_sender(const base::xtable_index_t & tableid) const override;
    bool is_send_receipt_role() const override {
        return m_is_send_receipt_role;
    }
    bool table_boundary_equal_to(std::shared_ptr<xtxpool_service_face> & service) const override;
    void get_service_table_boundary(base::enum_xchain_zone_index & zone_id, uint32_t & fount_table_id, uint32_t & back_table_id, common::xnode_type_t & node_type) const override;
    int32_t request_transaction_consensus(const data::xtransaction_ptr_t & tx, bool local) override;
    xcons_transaction_ptr_t query_tx(const std::string & account, const uint256_t & hash) const override {
        return nullptr;
    };
    void pull_lacking_receipts(uint64_t now, xcovered_tables_t & covered_tables) override;
    void send_receipt_id_state(uint64_t now) override;
    bool is_running() const override;

private:
    bool is_belong_to_service(base::xtable_index_t tableid) const;
    void on_message_receipt(vnetwork::xvnode_address_t const & sender, vnetwork::xmessage_t const & message);
    void on_message_unit_receipt(vnetwork::xvnode_address_t const & sender, vnetwork::xmessage_t const & message);
    void on_message_push_receipt_received(vnetwork::xvnode_address_t const & sender, vnetwork::xmessage_t const & message);
    void on_message_pull_receipt_received(vnetwork::xvnode_address_t const & sender, vnetwork::xmessage_t const & message);
    void on_message_receipt_id_state_received(vnetwork::xvnode_address_t const & sender, vnetwork::xmessage_t const & message);
    // void on_message_neighbor_sync_req(vnetwork::xvnode_address_t const & sender, vnetwork::xmessage_t const & message);
    void send_pull_receipts_of_confirm(xreceipt_pull_receipt_t & pulled_receipt);
    void send_pull_receipts_of_recv(xreceipt_pull_receipt_t & pulled_receipt);
    void send_push_receipts(xreceipt_push_t & pushed_receipt);
    void send_receipt_sync_msg(const vnetwork::xmessage_t & msg, const std::string & target_table_addr);
    void send_table_receipt_id_state(uint16_t table_id);
    void drop_msg(vnetwork::xmessage_t const & message, std::string reason);
    void push_send_fail_record(int32_t err_type);
    // void send_neighbor_sync_req(base::xtable_shortid_t table_sid);
    enum_txpool_service_status status() const;

private:
    xvip2_t m_xip;
    std::shared_ptr<vnetwork::xvnetwork_driver_face_t> m_vnet_driver;
    observer_ptr<router::xrouter_face_t> m_router;
    observer_ptr<xtxpool_svc_para_t> m_para;
    bool m_is_send_receipt_role{false};
    uint16_t m_cover_front_table_id;  // [m_cover_front_table_id,m_cover_back_table_id) is the scope for this service
    uint16_t m_cover_back_table_id;
    base::enum_xchain_zone_index m_zone_index;
    common::xnode_type_t m_node_type;
    uint16_t m_node_id;
    uint16_t m_shard_size;
    std::atomic<enum_txpool_service_status> m_status{enum_txpool_service_status_not_run};
    std::string m_vnetwork_str;
    std::unordered_map<uint16_t, table_info> m_table_info_cache;
};

NS_END2
