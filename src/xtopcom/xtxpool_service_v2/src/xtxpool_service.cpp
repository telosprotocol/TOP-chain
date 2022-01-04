// Copyright (c) 2017-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xtxpool_service_v2/xtxpool_service.h"

#include "xchain_fork/xchain_upgrade_center.h"
#include "xcommon/xmessage_id.h"
#include "xdata/xblocktool.h"
#include "xdata/xtableblock.h"
#include "xmbus/xevent_behind.h"
#include "xmetrics/xmetrics.h"
#include "xtxpool_service_v2/xreceipt_strategy.h"
#include "xtxpool_v2/xtxpool.h"
#include "xtxpool_v2/xtxpool_error.h"
#include "xtxpool_v2/xtxpool_log.h"
#include "xutility/xhash.h"
#include "xverifier/xtx_verifier.h"
#include "xvledger/xvblock.h"
#include "xvnetwork/xvnetwork_error.h"
#include "xvnetwork/xvnetwork_error2.h"

#include <cinttypes>

NS_BEG2(top, xtxpool_service_v2)

using xtxpool_v2::xtxpool_t;

enum enum_txpool_service_msg_type  // under pdu  enum_xpdu_type_consensus
{
    enum_txpool_service_msg_type_void = 0,  // reserved for old version

    enum_txpool_service_msg_type_send_receipt = 1,
    enum_txpool_service_msg_type_recv_receipt = 2,
};

// pull 5 for table-table lacking receipts at most. if pull much, a node that newly selected into will pull a large number of receipts.
#define pull_lacking_receipt_num_max (5)
#define thresold_lacking_receipt_num_to_sync_by_neighbor (15)
#define neighbor_sync_receipt_num_max (10)
#define pull_lacking_permission_max_round_num_of_first_join_round (1)

class txpool_receipt_message_para_t : public top::base::xobject_t {
public:
    txpool_receipt_message_para_t(vnetwork::xvnode_address_t const & sender, vnetwork::xmessage_t const & message) : m_sender(sender), m_message(message) {
    }

private:
    ~txpool_receipt_message_para_t() override {
    }

public:
    vnetwork::xvnode_address_t m_sender;
    vnetwork::xmessage_t m_message;
};

xtxpool_service::xtxpool_service(const observer_ptr<router::xrouter_face_t> & router, const observer_ptr<xtxpool_svc_para_t> & para) : m_router(router), m_para(para) {
}

void xtxpool_service::set_params(const xvip2_t & xip, const std::shared_ptr<vnetwork::xvnetwork_driver_face_t> & vnet_driver) {
    if (xcons_utl::xip_equals(m_xip, xip)) {
        return;
    }
    m_xip = xip;
    m_vnet_driver = vnet_driver;
    m_vnetwork_str = vnet_driver->address().to_string();

    xinfo("xtxpool_service::set_params node:%s,joined election round:%llu,cur election round:%llu",
          m_vnetwork_str.c_str(),
          m_vnet_driver->joined_election_round().value(),
          m_vnet_driver->address().election_round().value());

    common::xnode_address_t node_addr = xcons_utl::to_address(m_xip, m_vnet_driver->address().election_round());

    m_node_id = static_cast<std::uint16_t>(get_node_id_from_xip2(m_xip));
    m_shard_size = static_cast<std::uint16_t>(get_group_nodes_count_from_xip2(m_xip));
    xassert(m_node_id < m_shard_size);

    auto type = node_addr.type();
    if (common::has<common::xnode_type_t::committee>(type)) {
        m_is_send_receipt_role = true;
        m_zone_index = base::enum_chain_zone_beacon_index;
        m_node_type = common::xnode_type_t::committee;
    } else if (common::has<common::xnode_type_t::zec>(type)) {
        m_is_send_receipt_role = true;
        m_zone_index = base::enum_chain_zone_zec_index;
        m_node_type = common::xnode_type_t::zec;
    } else if (common::has<common::xnode_type_t::auditor>(type)) {
        m_is_send_receipt_role = true;
        m_zone_index = base::enum_chain_zone_consensus_index;
        m_node_type = common::xnode_type_t::auditor;
    } else if (common::has<common::xnode_type_t::validator>(type)) {
        m_is_send_receipt_role = false;
        m_zone_index = base::enum_chain_zone_consensus_index;
        m_node_type = common::xnode_type_t::validator;
    } else {
        xassert(0);
    }

    std::vector<uint16_t> tables = m_vnet_driver->table_ids();
    if (tables.empty()) {
        xerror("xtxpool_service::set_params, load table failed.xip:{%" PRIu64 ", %" PRIu64 "} node:%s", xip.high_addr, xip.low_addr, m_vnetwork_str.c_str());
        return;
    }
    m_cover_front_table_id = tables.front();
    m_cover_back_table_id = tables.back();
}

bool xtxpool_service::start(const xvip2_t & xip) {
    m_vnet_driver->register_message_ready_notify(xmessage_category_txpool, std::bind(&xtxpool_service::on_message_receipt, this, std::placeholders::_1, std::placeholders::_2));

    xinfo("xtxpool_service::start node:%s,xip:{%" PRIu64 ", %" PRIu64 "} zone:%d table:%d %d,is_send_receipt_role:%d",
          m_vnetwork_str.c_str(),
          xip.high_addr,
          xip.low_addr,
          m_zone_index,
          m_cover_front_table_id,
          m_cover_back_table_id,
          m_is_send_receipt_role);
    m_status.store(enum_txpool_service_status_running, std::memory_order_release);
    return true;
}
bool xtxpool_service::unreg(const xvip2_t & xip) {
    xinfo("xtxpool_service::unreg node:%s,xip:{%" PRIu64 ", %" PRIu64 "} zone:%d table:%d %d,is_send_receipt_role:%d",
          m_vnetwork_str.c_str(),
          xip.high_addr,
          xip.low_addr,
          m_zone_index,
          m_cover_front_table_id,
          m_cover_back_table_id,
          m_is_send_receipt_role);
    xassert(status() != enum_txpool_service_status_not_run);
    m_vnet_driver->unregister_message_ready_notify(xmessage_category_txpool);
    m_status.store(enum_txpool_service_status_not_run, std::memory_order_release);
    return true;
}

bool xtxpool_service::fade(const xvip2_t & xip) {
    xinfo("xtxpool_service::fade xip:{%" PRIu64 ", %" PRIu64 "} ", xip.high_addr, xip.low_addr);
    m_status.store(enum_txpool_service_status_faded, std::memory_order_release);
    return true;
}

enum_txpool_service_status xtxpool_service::status() const {
    return m_status.load(std::memory_order_acquire);
}

void xtxpool_service::get_service_table_boundary(base::enum_xchain_zone_index & zone_id,
                                                 uint32_t & fount_table_id,
                                                 uint32_t & back_table_id,
                                                 common::xnode_type_t & node_type) const {
    zone_id = m_zone_index;
    fount_table_id = m_cover_front_table_id;
    back_table_id = m_cover_back_table_id;
    node_type = m_node_type;
}

void xtxpool_service::pull_lacking_receipts(uint64_t now, xcovered_tables_t & covered_tables) {
    auto joined_round = m_vnet_driver->joined_election_round().value();
    auto cur_round = m_vnet_driver->address().election_round().value();
    if (joined_round == cur_round && joined_round > pull_lacking_permission_max_round_num_of_first_join_round) {
        xinfo("xtxpool_service::pull_lacking_receipts new service:%s not pull,joind round:%llu,cur round:%llu", m_vnetwork_str.c_str(), joined_round, cur_round);
        return;
    }

    for (uint32_t table_id = m_cover_front_table_id; table_id <= m_cover_back_table_id; table_id++) {
        if (!xreceipt_strategy_t::is_time_for_node_pull_lacking_receipts(now, table_id, m_node_id)) {
            continue;
        }

        if (covered_tables.is_covered(m_zone_index, table_id)) {
            continue;
        }
        covered_tables.add_covered_table(m_zone_index, table_id);

        if (!m_para->get_txpool()->need_sync_lacking_receipts(m_zone_index, table_id)) {
            continue;
        }

        std::string self_table_addr = data::xblocktool_t::make_address_table_account((base::enum_xchain_zone_index)m_zone_index, table_id);

        uint32_t total_lacking_confirm_tx_num = 0;
        uint32_t total_lacking_recv_tx_num = 0;
        auto lacking_confirm_tx_ids = m_para->get_txpool()->get_lacking_confirm_tx_ids(m_zone_index, table_id, total_lacking_confirm_tx_num);
        auto lacking_recv_tx_ids = m_para->get_txpool()->get_lacking_recv_tx_ids(m_zone_index, table_id, total_lacking_recv_tx_num);

        base::xtable_index_t table_idx(m_zone_index, table_id);

        // if (total_lacking_confirm_tx_num + total_lacking_recv_tx_num > thresold_lacking_receipt_num_to_sync_by_neighbor) {
        //     xwarn("xtxpool_service::pull_lacking_receipts too many lacking receipts:%u, not pull.", total_lacking_confirm_tx_num + total_lacking_recv_tx_num);
        //     send_neighbor_sync_req(table_idx.to_table_shortid());
        //     return;
        // }

        int32_t lacking_confirm_tx_num = 0;
        uint32_t left_num = pull_lacking_receipt_num_max;

        for (auto & table_lacking_ids : lacking_confirm_tx_ids) {
            xreceipt_pull_receipt_t pulled_confirm_receipt(m_vnet_driver->address(),
                                                           self_table_addr,
                                                           base::xvaccount_t::make_table_account_address(table_lacking_ids.get_peer_sid()),
                                                           table_lacking_ids.get_receipt_ids(),
                                                           left_num);
            for (auto & receiptid : pulled_confirm_receipt.m_receipt_ids) {
                xinfo("xtxpool_service::pull_lacking_receipts confirm tx reqnode:%s,table:%s:%s,receiptid:%llu,joind round:%llu,cur round:%llu",
                      m_vnetwork_str.c_str(),
                      pulled_confirm_receipt.m_tx_from_account.c_str(),
                      pulled_confirm_receipt.m_tx_to_account.c_str(),
                      receiptid,
                      joined_round,
                      cur_round);
            }
            send_pull_receipts_of_confirm(pulled_confirm_receipt);
            lacking_confirm_tx_num += pulled_confirm_receipt.m_receipt_ids.size();

            if (pulled_confirm_receipt.m_receipt_ids.size() >= left_num) {
                break;
            }
            left_num -= pulled_confirm_receipt.m_receipt_ids.size();
        }

        XMETRICS_GAUGE(metrics::txpool_pull_confirm_tx, lacking_confirm_tx_num);

        uint32_t lacking_recv_tx_num = 0;
        left_num = pull_lacking_receipt_num_max;
        for (auto & table_lacking_ids : lacking_recv_tx_ids) {
            xreceipt_pull_receipt_t pulled_recv_receipt(m_vnet_driver->address(),
                                                        base::xvaccount_t::make_table_account_address(table_lacking_ids.get_peer_sid()),
                                                        self_table_addr,
                                                        table_lacking_ids.get_receipt_ids(),
                                                        left_num);
            for (auto & receiptid : pulled_recv_receipt.m_receipt_ids) {
                xinfo("xtxpool_service::pull_lacking_receipts recv tx reqnode:%s,table:%s:%s,receiptid:%llu,joind round:%llu,cur round:%llu",
                      m_vnetwork_str.c_str(),
                      pulled_recv_receipt.m_tx_from_account.c_str(),
                      pulled_recv_receipt.m_tx_to_account.c_str(),
                      receiptid,
                      joined_round,
                      cur_round);
            }
            send_pull_receipts_of_recv(pulled_recv_receipt);
            lacking_recv_tx_num += pulled_recv_receipt.m_receipt_ids.size();
            if (pulled_recv_receipt.m_receipt_ids.size() >= left_num) {
                break;
            }
            left_num -= pulled_recv_receipt.m_receipt_ids.size();
        }
        XMETRICS_GAUGE(metrics::txpool_pull_recv_tx, lacking_recv_tx_num);
    }
}

bool xtxpool_service::is_belong_to_service(base::xtable_index_t tableid) const {
    if (tableid.get_zone_index() == m_zone_index && (tableid.get_subaddr() >= m_cover_front_table_id && tableid.get_subaddr() <= m_cover_back_table_id)) {
        return true;
    }
    return false;
}

bool xtxpool_service::table_boundary_equal_to(std::shared_ptr<xtxpool_service_face> & service) const {
    base::enum_xchain_zone_index zone_id;
    uint32_t fount_table_id;
    uint32_t back_table_id;
    common::xnode_type_t node_type;
    service->get_service_table_boundary(zone_id, fount_table_id, back_table_id, node_type);
    if (zone_id == this->m_zone_index && fount_table_id == m_cover_front_table_id && back_table_id == m_cover_back_table_id) {
        return true;
    }
    return false;
}

void xtxpool_service::drop_msg(vnetwork::xmessage_t const & message, std::string reason) {
    xtxpool_warn("xtxpool_service::drop_msg msg id:%x,hash:%x,resaon:%s", message.id(), message.hash(), reason.c_str());
    if (message.id() == xtxpool_v2::xtxpool_msg_send_receipt) {
        XMETRICS_GAUGE(metrics::txpool_drop_send_receipt_msg, 1);
        XMETRICS_GAUGE(metrics::txpool_receipt_tx, 0);
    } else if (message.id() == xtxpool_v2::xtxpool_msg_recv_receipt) {
        XMETRICS_GAUGE(metrics::txpool_drop_receive_receipt_msg, 1);
        XMETRICS_GAUGE(metrics::txpool_receipt_tx, 0);
    } else if (message.id() == xtxpool_v2::xtxpool_msg_push_receipt) {
        XMETRICS_GAUGE(metrics::txpool_drop_push_receipt_msg, 1);
        XMETRICS_GAUGE(metrics::txpool_receipt_tx, 0);
    } else if (message.id() == xtxpool_v2::xtxpool_msg_pull_recv_receipt) {
        XMETRICS_GAUGE(metrics::txpool_drop_pull_recv_receipt_msg, 1);
    } else if (message.id() == xtxpool_v2::xtxpool_msg_pull_confirm_receipt_v2) {
        XMETRICS_GAUGE(metrics::txpool_drop_pull_confirm_receipt_msg_v2, 1);
    } else if (message.id() == xtxpool_v2::xtxpool_msg_receipt_id_state) {
        XMETRICS_GAUGE(metrics::txpool_drop_receipt_id_state_msg, 1);
    }
}

void xtxpool_service::on_message_receipt(vnetwork::xvnode_address_t const & sender, vnetwork::xmessage_t const & message) {
    if (status() == enum_txpool_service_status_not_run) {
        return;
    }
    (void)sender;

    XMETRICS_TIME_RECORD("txpool_network_message_dispatch");

    xtxpool_info("xtxpool_service::on_message_receipt msg id:%x,hash:%x", message.id(), message.hash());

    if (message.id() == xtxpool_v2::xtxpool_msg_send_receipt || message.id() == xtxpool_v2::xtxpool_msg_recv_receipt || (message.id() == xtxpool_v2::xtxpool_msg_push_receipt)) {
        if (m_para->get_fast_dispatcher() == nullptr) {
            drop_msg(message, "fast_dispatcher_not_exist");
            return;
        }

        if (m_para->get_fast_dispatcher()->is_mailbox_over_limit()) {
            drop_msg(message, "fast_dispacher_mailbox_full");
            return;
        }

        base::xauto_ptr<txpool_receipt_message_para_t> para = new txpool_receipt_message_para_t(sender, message);
        if (message.id() == xtxpool_v2::xtxpool_msg_push_receipt) {
            auto handler = [this, self=shared_from_this()](base::xcall_t & call, const int32_t cur_thread_id, const uint64_t timenow_ms) -> bool {
                txpool_receipt_message_para_t * para = dynamic_cast<txpool_receipt_message_para_t *>(call.get_param1().get_object());
                this->on_message_push_receipt_received(para->m_sender, para->m_message);
                return true;
            };

            base::xcall_t asyn_call(handler, para.get());
            m_para->get_fast_dispatcher()->dispatch(asyn_call);
        } else {
            auto handler = [this, self=shared_from_this()](base::xcall_t & call, const int32_t cur_thread_id, const uint64_t timenow_ms) -> bool {
                txpool_receipt_message_para_t * para = dynamic_cast<txpool_receipt_message_para_t *>(call.get_param1().get_object());
                this->on_message_unit_receipt(para->m_sender, para->m_message);
                return true;
            };
            base::xcall_t asyn_call(handler, para.get());
            m_para->get_fast_dispatcher()->dispatch(asyn_call);
        }
    } else if ((message.id() == xtxpool_v2::xtxpool_msg_pull_recv_receipt) || (message.id() == xtxpool_v2::xtxpool_msg_pull_confirm_receipt_v2) ||
               (message.id() == xtxpool_v2::xtxpool_msg_receipt_id_state)) {
        // todo: not process xtxpool_msg_pull_confirm_receipt after a specified clock!!!

        if (m_para->get_slow_dispatcher() == nullptr) {
            drop_msg(message, "slow_dispatcher_not_exist");
            return;
        }

        if (m_para->get_slow_dispatcher()->is_mailbox_over_limit()) {
            drop_msg(message, "slow_dispacher_mailbox_full");
            return;
        }

        auto handler = [this, self=shared_from_this()](base::xcall_t & call, const int32_t cur_thread_id, const uint64_t timenow_ms) -> bool {
            txpool_receipt_message_para_t * para = dynamic_cast<txpool_receipt_message_para_t *>(call.get_param1().get_object());
            if (para->m_message.id() == xtxpool_v2::xtxpool_msg_pull_recv_receipt || para->m_message.id() == xtxpool_v2::xtxpool_msg_pull_confirm_receipt_v2) {
                this->on_message_pull_receipt_received(para->m_sender, para->m_message);
            } else {
                this->on_message_receipt_id_state_received(para->m_sender, para->m_message);
            }
            return true;
        };

        base::xauto_ptr<txpool_receipt_message_para_t> para = new txpool_receipt_message_para_t(sender, message);
        base::xcall_t asyn_call(handler, para.get());
        m_para->get_slow_dispatcher()->dispatch(asyn_call);
    } else {
        xassert(0);
    }
}

void xtxpool_service::on_message_unit_receipt(vnetwork::xvnode_address_t const & sender, vnetwork::xmessage_t const & message) {
    (void)sender;
    XMETRICS_TIME_RECORD("txpool_message_unit_receipt");
    base::xstream_t stream(top::base::xcontext_t::instance(), (uint8_t *)message.payload().data(), (uint32_t)message.payload().size());
    data::xcons_transaction_ptr_t receipt = make_object_ptr<data::xcons_transaction_t>();
    int32_t ret = receipt->serialize_from(stream);
    if (ret <= 0) {
        xerror("xtxpool_service::on_message_unit_receipt receipt serialize_from fail ret:%d", ret);
        return;
    }

    xinfo("xtxpool_service::on_message_unit_receipt receipt=%s,at_node:%s,msg id:%x,hash:%x", receipt->dump().c_str(), m_vnetwork_str.c_str(), message.id(), message.hash());

    xtxpool_v2::xtx_para_t para;
    std::shared_ptr<xtxpool_v2::xtx_entry> tx_ent = std::make_shared<xtxpool_v2::xtx_entry>(receipt, para);
    XMETRICS_GAUGE(metrics::txpool_received_other_send_receipt_num, 1);
    ret = m_para->get_txpool()->push_receipt(tx_ent, false, false);
    XMETRICS_GAUGE(metrics::txpool_receipt_tx, (ret == xsuccess) ? 1 : 0);
}

// void xtxpool_service::on_message_neighbor_sync_req(vnetwork::xvnode_address_t const & sender, vnetwork::xmessage_t const & message) {
//     (void)sender;
//     XMETRICS_TIME_RECORD("txpool_message_neighbor_sync");
//     base::xstream_t stream(top::base::xcontext_t::instance(), (uint8_t *)message.payload().data(), (uint32_t)message.payload().size());
//     xneighbor_sync_req_t sync_req;
//     auto ret = sync_req.serialize_from(stream);

//     if (ret <= 0) {
//         xerror("xtxpool_service::on_message_neighbor_sync_req sync_req serialize_from fail ret:%d", ret);
//         return;
//     }

//     xinfo("xtxpool_service::on_message_neighbor_sync_req sync table:%d,at_node:%s,msg id:%x,hash:%x", sync_req.m_table_sid, m_vnetwork_str.c_str(), message.id(),
//     message.hash());

//     base::xtable_index_t table_idx(sync_req.m_table_sid);
//     auto receipts = m_para->get_txpool()->get_receipts(table_idx.get_zone_index(), table_idx.get_subaddr());

//     for (uint32_t i = 0; i < receipts.size();) {
//         xneighbor_sync_rsp_t neighbor_sync_rsp;
//         uint32_t num = (neighbor_sync_receipt_num_max < (receipts.size() - i)) ? neighbor_sync_receipt_num_max : (receipts.size() - i);
//         neighbor_sync_rsp.m_receipts.insert(neighbor_sync_rsp.m_receipts.end(), receipts.begin() + i, receipts.begin() + i + num);

//         base::xstream_t stream(base::xcontext_t::instance());
//         vnetwork::xmessage_t msg;
//         neighbor_sync_rsp.serialize_to(stream);
//         msg = vnetwork::xmessage_t({stream.data(), stream.data() + stream.size()}, xtxpool_v2::xtxpool_msg_neighbor_sync_rsp);
//         m_vnet_driver->send_to(sender, msg);
//         i += num;
//     }
// }

void xtxpool_service::push_send_fail_record(int32_t err_type) {
#ifdef ENABLE_METRICS
    if (err_type == xsuccess) {
        XMETRICS_GAUGE(metrics::txpool_request_origin_tx, 1);
        return;
    }
    XMETRICS_GAUGE(metrics::txpool_request_origin_tx, 0);

    switch (err_type) {
    case xtxpool_v2::xtxpool_error_table_reached_upper_limit:
        XMETRICS_GAUGE(metrics::txpool_push_send_fail_table_limit, 1);
        break;
    case xtxpool_v2::xtxpool_error_role_reached_upper_limit:
        XMETRICS_GAUGE(metrics::txpool_push_send_fail_role_limit, 1);
        break;
    case xtxpool_v2::xtxpool_error_request_tx_repeat:
        XMETRICS_GAUGE(metrics::txpool_push_send_fail_repeat, 1);
        break;
    case xtxpool_v2::xtxpool_error_account_unconfirm_txs_reached_upper_limit:
        XMETRICS_GAUGE(metrics::txpool_push_send_fail_unconfirm_limit, 1);
        break;
    case xtxpool_v2::xtxpool_error_tx_nonce_out_of_scope:
        XMETRICS_GAUGE(metrics::txpool_push_send_fail_nonce_limit, 1);
        break;
    case xtxpool_v2::xtxpool_error_account_state_fall_behind:
        XMETRICS_GAUGE(metrics::txpool_push_send_fail_account_fall_behind, 1);
        break;
    case xtxpool_v2::xtxpool_error_account_not_in_charge:
        XMETRICS_GAUGE(metrics::txpool_push_send_fail_account_not_in_charge, 1);
        break;
    case xtxpool_v2::xtxpool_error_tx_nonce_expired:
        XMETRICS_GAUGE(metrics::txpool_push_send_fail_nonce_expired, 1);
        break;
    case xtxpool_v2::xtxpool_error_tx_nonce_duplicate:
        XMETRICS_GAUGE(metrics::txpool_push_send_fail_nonce_duplicate, 1);
        break;
    default:
        XMETRICS_GAUGE(metrics::txpool_push_send_fail_other, 1);
        break;
    }
#endif
}

int32_t xtxpool_service::request_transaction_consensus(const data::xtransaction_ptr_t & tx, bool local) {
    xdbg("xtxpool_service::request_transaction_consensus in, tx:source:%s target:%s hash:%s",
         tx->get_source_addr().c_str(),
         tx->get_target_addr().c_str(),
         tx->get_digest_hex_str().c_str());
    if (status() == enum_txpool_service_status_not_run) {
        xwarn(
            "[xtxpool_service]not running, tx dropped:source:%s target:%s hash:%s", tx->get_source_addr().c_str(), tx->get_target_addr().c_str(), tx->get_digest_hex_str().c_str());
        push_send_fail_record(xtxpool_v2::xtxpool_error_service_not_running);
        return xtxpool_v2::xtxpool_error_service_not_running;
    }

    int32_t ret = xverifier::xtx_verifier::verify_send_tx_source(tx.get(), local);
    if (ret) {
        xwarn("[global_trace][xtxpool_service]tx=%s,account=%s verify send tx source fail", tx->get_digest_hex_str().c_str(), tx->get_source_addr().c_str());
        push_send_fail_record(ret);
        return ret;
    }

    auto tableid = data::account_map_to_table_id(common::xaccount_address_t{tx->get_source_addr()});
    if (!is_belong_to_service(tableid)) {
        xerror("[global_trace][xtxpool_service]%s %s zone%d table%d not match this network driver",
               tx->get_digest_hex_str().c_str(),
               tx->get_source_addr().c_str(),
               tableid.get_zone_index(),
               tableid.get_subaddr());
        push_send_fail_record(xtxpool_v2::xtxpool_error_transaction_not_belong_to_this_service);
        return xtxpool_v2::xtxpool_error_transaction_not_belong_to_this_service;
    }

    if (is_sys_sharding_contract_address(common::xaccount_address_t{tx->get_target_addr()})) {
        tx->adjust_target_address(tableid.get_subaddr());
    }

    xcons_transaction_ptr_t cons_tx = make_object_ptr<xcons_transaction_t>(tx.get());
    xtxpool_v2::xtx_para_t para;
    std::shared_ptr<xtxpool_v2::xtx_entry> tx_ent = std::make_shared<xtxpool_v2::xtx_entry>(cons_tx, para);
    ret = m_para->get_txpool()->push_send_tx(tx_ent);
    push_send_fail_record(ret);
    return ret;
}

void xtxpool_service::send_pull_receipts_of_recv(xreceipt_pull_receipt_t & pulled_receipt) {
    base::xstream_t stream(base::xcontext_t::instance());
    vnetwork::xmessage_t msg;
    pulled_receipt.serialize_to(stream);
    msg = vnetwork::xmessage_t({stream.data(), stream.data() + stream.size()}, xtxpool_v2::xtxpool_msg_pull_recv_receipt);
    send_receipt_sync_msg(msg, pulled_receipt.m_tx_from_account);
}

void xtxpool_service::send_pull_receipts_of_confirm(xreceipt_pull_receipt_t & pulled_receipt) {
    base::xstream_t stream(base::xcontext_t::instance());
    vnetwork::xmessage_t msg;
    pulled_receipt.serialize_to(stream);
    msg = vnetwork::xmessage_t({stream.data(), stream.data() + stream.size()}, xtxpool_v2::xtxpool_msg_pull_confirm_receipt_v2);
    send_receipt_sync_msg(msg, pulled_receipt.m_tx_to_account);
}

void xtxpool_service::send_push_receipts(xreceipt_push_t & pushed_receipt) {
    base::xstream_t stream(base::xcontext_t::instance());
    vnetwork::xmessage_t msg;
    pushed_receipt.serialize_to(stream);
    msg = vnetwork::xmessage_t({stream.data(), stream.data() + stream.size()}, xtxpool_v2::xtxpool_msg_push_receipt);

    std::error_code ec;
    m_vnet_driver->broadcast(pushed_receipt.m_req_node.xip2().group_xip2(), msg, ec);
    if (ec) {
        assert(false);
    }
}

void xtxpool_service::send_receipt_sync_msg(const vnetwork::xmessage_t & msg, const std::string & target_table_addr) {
    auto cluster_addr =
        m_router->sharding_address_from_account(common::xaccount_address_t{target_table_addr}, m_vnet_driver->network_id(), common::xnode_type_t::consensus_auditor);
    // if (cluster_addr == m_vnet_driver->address().cluster_address()) {
    //    m_vnet_driver->broadcast(msg);
    //} else {
    //    m_vnet_driver->forward_broadcast_message(msg, common::xnode_address_t{std::move(cluster_addr)});
    //}
    std::error_code ec;
    m_vnet_driver->broadcast(common::xnode_address_t{cluster_addr}.xip2(), msg, ec);
    if (ec) {
        xwarn("xtxpool_service::send_receipt_sync_msg failed. sender %s, dst %s", m_vnet_driver->address().to_string().c_str(), cluster_addr.to_string().c_str());
        assert(false);
    }
}

void xtxpool_service::on_message_push_receipt_received(vnetwork::xvnode_address_t const & sender, vnetwork::xmessage_t const & message) {
    xinfo("xtxpool_service::on_message_push_receipt_received at_node:%s,msg id:%x,hash:%x", m_vnetwork_str.c_str(), message.id(), message.hash());
    uint64_t now = xverifier::xtx_utl::get_gmttime_s();

    xreceipt_push_t pushed_receipt;
    base::xstream_t stream(top::base::xcontext_t::instance(), (uint8_t *)message.payload().data(), (uint32_t)message.payload().size());
    pushed_receipt.serialize_from(stream);

    // if (pushed_receipt.m_req_node != m_vnet_driver->address()) {
    //     xdbg("xtxpool_service::on_message_receipts_received xtxpool_msg_push_receipt this node(%s) is not req node(%s).table:%s:%s",
    //          m_vnetwork_str.c_str(),
    //          pushed_receipt.m_req_node.to_string().c_str(),
    //          pushed_receipt.m_tx_from_account.c_str(),
    //          pushed_receipt.m_tx_to_account.c_str());
    //     auto type = pushed_receipt.m_req_node.type();

    //     // base::xvaccount_t vaccount(pushed_receipt.m_tx_from_account);
    //     // if (common::has<common::xnode_type_t::auditor>(type) &&
    //     //     xreceipt_strategy_t::is_selected_receipt_pull_msg_processor(now, vaccount.get_table_index(), m_shard_size, m_node_id)) {
    //     if (common::has<common::xnode_type_t::auditor>(type) &&
    //         xreceipt_strategy_t::is_selected_receipt_pull_msg_sender(pushed_receipt.m_tx_from_account, now, m_node_id, m_shard_size)) {
    //         xdbg("xtxpool_service::on_message_receipts_received xtxpool_msg_push_receipt transmit to reqnode:%s.table:%s:%s",
    //              pushed_receipt.m_req_node.to_string().c_str(),
    //              pushed_receipt.m_tx_from_account.c_str(),
    //              pushed_receipt.m_tx_to_account.c_str());
    //         m_vnet_driver->send_to(pushed_receipt.m_req_node, message);
    //     }
    //     return;
    // }

    xinfo("xtxpool_service::on_message_receipts_received push receipts.table:%s:%s,receipts num=%u",
          pushed_receipt.m_tx_from_account.c_str(),
          pushed_receipt.m_tx_to_account.c_str(),
          pushed_receipt.m_receipts.size());
    for (auto & tx : pushed_receipt.m_receipts) {
        xdbg("xtxpool_service::on_message_receipts_received xtxpool_msg_push_receipt at node:%s,table:%s:%s receipt:%s",
             m_vnetwork_str.c_str(),
             pushed_receipt.m_tx_from_account.c_str(),
             pushed_receipt.m_tx_to_account.c_str(),
             tx->dump().c_str());
        xtxpool_v2::xtx_para_t para;
        std::shared_ptr<xtxpool_v2::xtx_entry> tx_ent = std::make_shared<xtxpool_v2::xtx_entry>(tx, para);
        m_para->get_txpool()->push_receipt(tx_ent, false, true);
    }
}

void xtxpool_service::on_message_pull_receipt_received(vnetwork::xvnode_address_t const & sender, vnetwork::xmessage_t const & message) {
    xinfo("xtxpool_service::on_message_pull_receipt_received at_node:%s,msg id:%x,hash:%" PRIx64 "", m_vnetwork_str.c_str(), message.id(), message.hash());
    uint64_t now = xverifier::xtx_utl::get_gmttime_s();

    if (!m_is_send_receipt_role) {
        xinfo("xtxpool_service::on_message_pull_receipt_received xtxpool_msg_pull_recv_receipt droped at_node:%s,msg id:%x", m_vnetwork_str.c_str(), message.id());
        return;
    }
    bool is_pull_recv = (message.id() == xtxpool_v2::xtxpool_msg_pull_recv_receipt);
    xreceipt_pull_receipt_t pulled_receipt;
    base::xstream_t stream(top::base::xcontext_t::instance(), (uint8_t *)message.payload().data(), (uint32_t)message.payload().size());
    pulled_receipt.serialize_from(stream);

    // base::xvaccount_t vaccount(pulled_receipt.m_tx_to_account);
    // if (!xreceipt_strategy_t::is_selected_receipt_pull_msg_processor(now, vaccount.get_table_index(), m_shard_size, m_node_id)) {
    if (!xreceipt_strategy_t::is_selected_receipt_pull_msg_sender(pulled_receipt.m_tx_to_account, now, m_node_id, m_shard_size)) {
        return;
    }

    if (pulled_receipt.m_receipt_ids.size() > pull_lacking_receipt_num_max) {
        xerror("xtxpool_service::on_message_pull_receipt_received receipts in one request");
        return;
    }
    XMETRICS_TIME_RECORD("txpool_on_message_pull_receipt_received");
    auto & table_addr = is_pull_recv ? pulled_receipt.m_tx_from_account : pulled_receipt.m_tx_to_account;
    auto cluster_addr =
        m_router->sharding_address_from_account(common::xaccount_address_t{table_addr}, m_vnet_driver->network_id(), common::xnode_type_t::consensus_auditor);
    if (cluster_addr != m_vnet_driver->address().cluster_address()) {
        xdbg("xtxpool_service::on_message_pull_receipt_received forward broadcast message, cluster address is %s", cluster_addr.to_string().c_str());
        std::error_code ec;
        m_vnet_driver->broadcast(common::xnode_address_t{std::move(cluster_addr)}.xip2(), message, ec);
        if (ec) {
            xwarn("xtxpool_service::on_message_pull_receipt_received forward broadcast message, cluster address is %s", cluster_addr.to_string().c_str());
            assert(false);
        }
    } else {
        xinfo("xtxpool_service::on_message_pull_receipt_received pull %s txs.table:%s:%s,receiptid num=%u",
              (is_pull_recv ? "recv" : "confirm"),
              pulled_receipt.m_tx_from_account.c_str(),
              pulled_receipt.m_tx_to_account.c_str(),
              pulled_receipt.m_receipt_ids.size());
        xreceipt_push_t pushed_receipt;

        base::xtable_shortid_t from_table_sid = base::xvaccount_t(pulled_receipt.m_tx_from_account).get_short_table_id();
        base::xtable_shortid_t to_table_sid = base::xvaccount_t(pulled_receipt.m_tx_to_account).get_short_table_id();
        if (is_pull_recv) {
            m_para->get_txpool()->build_recv_tx(from_table_sid, to_table_sid, pulled_receipt.m_receipt_ids, pushed_receipt.m_receipts);
        } else {
            m_para->get_txpool()->build_confirm_tx(from_table_sid, to_table_sid, pulled_receipt.m_receipt_ids, pushed_receipt.m_receipts);
        }

        if (!pushed_receipt.m_receipts.empty()) {
            pushed_receipt.m_receipt_type = is_pull_recv ? enum_transaction_subtype_recv : enum_transaction_subtype_confirm;
            pushed_receipt.m_tx_from_account = pulled_receipt.m_tx_from_account;
            pushed_receipt.m_tx_to_account = pulled_receipt.m_tx_to_account;
            pushed_receipt.m_req_node = pulled_receipt.m_req_node;
            send_push_receipts(pushed_receipt);
        }
    }
}

void xtxpool_service::on_message_receipt_id_state_received(vnetwork::xvnode_address_t const & sender, vnetwork::xmessage_t const & message) {
    xinfo("xtxpool_service::on_message_receipt_id_state_received at_node:%s,msg id:%x,hash:%" PRIx64 "", m_vnetwork_str.c_str(), message.id(), message.hash());

    XMETRICS_TIME_RECORD("txpool_on_message_receipt_id_state_received");
    base::xstream_t stream(top::base::xcontext_t::instance(), (uint8_t *)message.payload().data(), (uint32_t)message.payload().size());
    base::xdataunit_t * _dataunit = base::xdataunit_t::read_from(stream);
    xassert(_dataunit != nullptr);
    base::xvproperty_prove_t * _property_prove_raw = dynamic_cast<base::xvproperty_prove_t *>(_dataunit);
    xassert(_property_prove_raw != nullptr);
    base::xvproperty_prove_ptr_t property_prove_ptr;
    property_prove_ptr.attach(_property_prove_raw);
    xassert(property_prove_ptr->is_valid());

    auto receiptid_state = xblocktool_t::get_receiptid_from_property_prove(property_prove_ptr);
    m_para->get_txpool()->update_peer_receipt_id_state(receiptid_state);
}

void xtxpool_service::send_table_receipt_id_state(uint16_t table_id) {
    XMETRICS_TIME_RECORD("txpool_send_table_receipt_id_state");
    std::string table_addr = data::xblocktool_t::make_address_table_account((base::enum_xchain_zone_index)m_zone_index, table_id);
    base::xvaccount_t vaccount(table_addr);
    base::xvproperty_prove_ptr_t property_prove = nullptr;

    auto iter = m_table_info_cache.find(table_id);
    if (iter != m_table_info_cache.end()) {
        auto & table_info = iter->second;
        uint64_t cur_height = m_para->get_vblockstore()->get_latest_committed_block_height(vaccount, metrics::blockstore_access_from_txpool_id_state);
        if (table_info.m_last_property_height >= cur_height) {
            xinfo("xtxpool_service::send_table_receipt_id_state table:%s height(%llu:%llu) not change,reuse property_prove",
                  table_addr.c_str(),
                  table_info.m_last_property_height,
                  cur_height);
            property_prove = table_info.m_property_prove;
        }
    }
    if (property_prove == nullptr) {
        auto latest_commit_block = m_para->get_vblockstore()->get_latest_committed_block(vaccount, metrics::blockstore_access_from_txpool_id_state);
        base::enum_xvblock_class block_class = latest_commit_block->get_block_class();
        uint32_t nil_block_num = 0;
        xvblock_ptr_t non_nil_commit_block = nullptr;
        uint64_t height = latest_commit_block->get_height();
        if (block_class != base::enum_xvblock_class_nil) {
            xvblock_t * _block = latest_commit_block.get();
            _block->add_ref();
            non_nil_commit_block.attach(_block);
        } else {
            while (block_class == base::enum_xvblock_class_nil && height > 0) {
                nil_block_num++;
                if (nil_block_num > 2) {
                    xerror("xtxpool_service::send_table_receipt_id_state, continuous nil table block number is more than 2,table:%s,height:%llu", table_addr.c_str(), height);
                }

                auto commit_block = m_para->get_vblockstore()->load_block_object(vaccount, height - 1, base::enum_xvblock_flag_committed, false, metrics::blockstore_access_from_txpool_id_state);
                if (commit_block == nullptr) {
                    xwarn("xtxpool_service::send_table_receipt_id_state load block fail,table:%s,height:%llu", table_addr.c_str(), height - 1);
                    return;
                }
                height = commit_block->get_height();

                if (commit_block->get_block_class() != base::enum_xvblock_class_nil) {
                    xvblock_t * _block = commit_block.get();
                    _block->add_ref();
                    non_nil_commit_block.attach(_block);
                    break;
                }
            }
        }

        if (height == 0 || block_class == base::enum_xvblock_class_nil) {
            xinfo("xtxpool_service::send_receipt_id_state latest commit height is 0, no need send receipt id state.table:%s", table_addr.c_str());
            return;
        }

        auto bstate = base::xvchain_t::instance().get_xstatestore()->get_blkstate_store()->get_block_state(non_nil_commit_block.get());
        if (bstate == nullptr) {
            xwarn("xtxpool_service::send_receipt_id_state table:%s,height:%llu,get bstate fail", table_addr.c_str(), non_nil_commit_block->get_height());
            return;
        }

        auto cert_block = m_para->get_vblockstore()->load_block_object(vaccount, non_nil_commit_block->get_height() + 2, 0, false, metrics::blockstore_access_from_txpool_id_state);
        if (cert_block == nullptr) {
            xinfo("xtxpool_service::send_receipt_id_state cert block load fail.table:%s, cert height:%llu", table_addr.c_str(), non_nil_commit_block->get_height() + 2);
            return;
        }

        xtablestate_ptr_t tablestate = std::make_shared<xtable_bstate_t>(bstate.get());
        if (tablestate->get_receiptid_state()->get_all_receiptid_pairs()->get_all_pairs().empty()) {
            xinfo("xtxpool_service::send_receipt_id_state table have no receipt id pairs.table:%s, commit height:%llu", table_addr.c_str(), non_nil_commit_block->get_height());
            return;
        }
        property_prove = base::xpropertyprove_build_t::create_property_prove(non_nil_commit_block.get(), cert_block.get(), bstate.get(), XPROPERTY_TABLE_RECEIPTID);
        if (property_prove == nullptr) {
            xwarn("xtxpool_service::send_table_receipt_id_state create receipt state fail.table:%s, commit height:%llu", table_addr.c_str(), non_nil_commit_block->get_height());
            return;
        }
        xassert(property_prove->is_valid());

        m_para->get_txpool()->update_peer_receipt_id_state(tablestate->get_receiptid_state());
        if (iter == m_table_info_cache.end()) {
            m_table_info_cache.insert(std::make_pair(table_id, table_info(non_nil_commit_block->get_height(), property_prove)));
        } else {
            iter->second.m_last_property_height = non_nil_commit_block->get_height();
            iter->second.m_property_prove = property_prove;
        }
    }

    base::xstream_t stream(base::xcontext_t::instance());
    vnetwork::xmessage_t msg;
    property_prove->serialize_to(stream);
    msg = vnetwork::xmessage_t({stream.data(), stream.data() + stream.size()}, xtxpool_v2::xtxpool_msg_receipt_id_state);

    std::error_code ec = vnetwork::xvnetwork_errc2_t::success;
    xvip2_t to_addr{(uint64_t)-1, (uint64_t)-1};  // broadcast to all
    common::xip2_t dst{to_addr.low_addr, to_addr.high_addr};
    m_vnet_driver->broadcast(dst, msg, ec);

    XMETRICS_GAUGE(metrics::txpool_receipt_id_state_msg_send_num, 1);
}

void xtxpool_service::send_receipt_id_state(uint64_t now) {
    if (m_is_send_receipt_role) {
        for (uint32_t table_id = m_cover_front_table_id; table_id <= m_cover_back_table_id; table_id++) {
            if (!xreceipt_strategy_t::is_receiptid_state_sender_for_talbe(now, table_id, m_shard_size, m_node_id)) {
                continue;
            }
            send_table_receipt_id_state(table_id);
        }
    }
}

bool xtxpool_service::is_running() const {
    return (status() == enum_txpool_service_status_running);
}

// void xtxpool_service::send_neighbor_sync_req(base::xtable_shortid_t table_sid) {
//     xvip2_t neighbor_xip = m_xip;
//     uint16_t rand_node_id = xverifier::xtx_utl::get_gmttime_s() % m_shard_size;
//     if (rand_node_id != m_node_id) {
//         set_node_id_to_xip2(neighbor_xip, rand_node_id);
//     } else if (m_node_id + 1 >= m_shard_size) {
//         set_node_id_to_xip2(neighbor_xip, 0);
//     } else {
//         set_node_id_to_xip2(neighbor_xip, m_node_id + 1);
//     }
//     xdbg("xtxpool_service::send_receipt_sync_req m_xip:{%" PRIu64 ", %" PRIu64 "} ,neighbor_xip:{%" PRIu64 ", %" PRIu64 "}",
//          m_xip.high_addr,
//          m_xip.low_addr,
//          neighbor_xip.high_addr,
//          neighbor_xip.low_addr);

//     common::xnode_address_t neighbor_node_addr = xcons_utl::to_address(neighbor_xip, m_vnet_driver->address().election_round());

//     xneighbor_sync_req_t neighbor_sync_req;
//     neighbor_sync_req.m_table_sid = table_sid;
//     base::xstream_t stream(base::xcontext_t::instance());
//     vnetwork::xmessage_t msg;
//     neighbor_sync_req.serialize_to(stream);
//     msg = vnetwork::xmessage_t({stream.data(), stream.data() + stream.size()}, xtxpool_v2::xtxpool_msg_neighbor_sync_req);
//     m_vnet_driver->send_to(neighbor_node_addr, msg);
// }

NS_END2
