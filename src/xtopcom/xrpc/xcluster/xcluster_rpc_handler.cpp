// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include "xcluster_rpc_handler.h"

#include "xbase/xcontext.h"
#include "xcodec/xmsgpack_codec.hpp"
#include "xmetrics/xmetrics.h"
#include "xrpc/xerror/xrpc_error_json.h"
#include "xrpc/xrpc_init.h"
#include "xrpc/xrpc_method.h"
#include "xrpc/xuint_format.h"
#include "xvledger/xvaccount.h"
#include "xvnetwork/xvnetwork_error.h"
#include "xtxpool_v2/xtxpool_error.h"

#include <cinttypes>

NS_BEG2(top, xrpc)
using base::xcontext_t;
using base::xstream_t;
using data::xtransaction_ptr_t;
using data::xtransaction_t;

#define max_cluster_rpc_mailbox_num (10000)

xcluster_rpc_handler::xcluster_rpc_handler(std::shared_ptr<xvnetwork_driver_face_t> cluster_vhost,
                                           observer_ptr<xrouter_face_t> router_ptr,
                                           xtxpool_service_v2::xtxpool_proxy_face_ptr const & txpool_service,
                                           observer_ptr<base::xvblockstore_t> block_store,
                                           observer_ptr<base::xvtxstore_t> txstore,
                                           observer_ptr<top::base::xiothread_t> thread)
  : m_cluster_vhost(cluster_vhost)
  , m_router_ptr(router_ptr)
  , m_txpool_service(txpool_service)
  , m_rule_mgr_ptr(top::make_unique<xfilter_manager>())
  , m_thread(thread) {
}

void xcluster_rpc_handler::on_message(const xvnode_address_t & edge_sender, const xmessage_t & message) {
    XMETRICS_TIME_RECORD("rpc_net_iothread_dispatch_cluster_rpc_handler");
#if defined(DEBUG)
    auto msg_id = message.id();
    xdbg_rpc("xcluster_rpc_handler on_message,id(%x,%s)", msg_id, edge_sender.to_string().c_str());  // address to_string
#endif

    auto self = shared_from_this();
    auto process_request = [self](base::xcall_t & call, const int32_t cur_thread_id, const uint64_t timenow_ms) -> bool {
        rpc_message_para_t * para = dynamic_cast<rpc_message_para_t *>(call.get_param1().get_object());
        auto message = para->m_message;
        auto edge_sender = para->m_sender;
        auto msgid = message.id();
/*        if (msgid == rpc_msg_request || msgid == rpc_msg_query_request) {
            xrpc_msg_request_t msg = codec::xmsgpack_codec_t<xrpc_msg_request_t>::decode(message.payload());
            if (msgid == rpc_msg_request) {
                self->cluster_process_request(msg, edge_sender, message);
                XMETRICS_GAUGE(metrics::rpc_auditor_tx_request, 1);
            } else {
                xwarn("xcluster_rpc_handler::on_message msgid is rpc_msg_query_request");
            }*/
        if (msgid == rpc_msg_response || msgid == rpc_msg_eth_response) {
            self->cluster_process_response(message, edge_sender);
            return true;
        }
        xrpc_msg_request_t msg = codec::xmsgpack_codec_t<xrpc_msg_request_t>::decode(message.payload());
        self->cluster_process_request(msg, edge_sender, message);
        XMETRICS_GAUGE(metrics::rpc_auditor_tx_request, 1);
        return true;
    };
    int64_t in, out;
    int32_t queue_size = m_thread->count_calls(in, out);
    if (queue_size >= max_cluster_rpc_mailbox_num) {
        xkinfo_rpc("xcluster_rpc_handler::on_message cluster rpc mailbox is full:%d", queue_size);
        XMETRICS_GAUGE(metrics::mailbox_rpc_auditor_total, 0);
        return;
    }
    XMETRICS_GAUGE_SET_VALUE(metrics::mailbox_rpc_auditor_cur, queue_size);
    XMETRICS_GAUGE(metrics::mailbox_rpc_auditor_total, 1);

    base::xauto_ptr<rpc_message_para_t> para = new rpc_message_para_t(edge_sender, message);
    base::xcall_t asyn_call(process_request, para.get());
    m_thread->send_call(asyn_call);
}

void xcluster_rpc_handler::cluster_process_request(const xrpc_msg_request_t & edge_msg, const xvnode_address_t & edge_sender, const xmessage_t & message) {
    std::string tx_hash;
    std::string account;
    bool is_evm_tx = false;
    if (edge_msg.m_tx_type == enum_xrpc_tx_type::enum_xrpc_tx_type) {
        xtransaction_ptr_t tx_ptr;
        auto ret = xtransaction_t::set_tx_by_serialized_data(tx_ptr, edge_msg.m_message_body);
        if (ret == false) {
            xwarn("xcluster_rpc_handler::cluster_process_request: fail to extract transaction object from rpc message");
            return;
        }

        tx_hash = tx_ptr->get_digest_hex_str();
        account = tx_ptr->get_source_addr();
        xdbg("[global_trace][advance_rpc][recv edge msg][push unit_service] deal tx hash: %s, version: %d, %s,src %s,dst %s,%" PRIx64,
               tx_hash.c_str(),
               tx_ptr->get_tx_version(),
               account.c_str(),
               edge_sender.to_string().c_str(),
               m_cluster_vhost->address().to_string().c_str(),
               message.hash());
        is_evm_tx = base::xvaccount_t::get_addrtype_from_account(account) == base::enum_vaccount_addr_type_secp256k1_evm_user_account;

        uint64_t now = (uint64_t)base::xtime_utl::gettimeofday();
        // uint64_t delay_time_s = tx_ptr->get_delay_from_fire_timestamp(now);
        if (now < tx_ptr->get_fire_timestamp()) {
            XMETRICS_GAUGE(metrics::txdelay_client_timestamp_unmatch, 1);
        }
        XMETRICS_GAUGE(metrics::txdelay_from_client_to_auditor, tx_ptr->get_delay_from_fire_timestamp(now));

        // todo: check return value. if signature is invalid, not forward to validators.
        m_txpool_service->request_transaction_consensus(tx_ptr, false);
        // if (xsuccess != m_txpool_service->request_transaction_consensus(tx_ptr, false)) {
        //     // xwarn("[global_trace][advance_rpc][recv edge msg][push unit_service] tx hash: %s,%s,src %s,dst %s,%" PRIx64 " ignored",
        //     //       tx_hash.c_str(),
        //     //       account.c_str(),
        //     //       edge_sender.to_string().c_str(),
        //     //       m_cluster_vhost->address().to_string().c_str(),
        //     //       message.hash());
        //     return;
        // }
    } else {
        xerror("cluster error tx_type %d", edge_msg.m_tx_type);
        return;
    }

    auto neighbors = m_cluster_vhost->neighbors_info2();
    auto me_addr = m_cluster_vhost->address();
    size_t count = 0;
    for (auto & item : neighbors) {
        if (me_addr == item.second.address && (message.hash() % neighbors.size() == count || (message.hash() + 1) % neighbors.size() == count)) {
            // xkinfo("m_cluster_vhost send:%s,%" PRIu64, item.second.address.to_string().c_str(), message.hash());

            common::xnode_type_t type = is_evm_tx ? common::xnode_type_t::evm_validator : common::xnode_type_t::consensus_validator;
            xmessage_t msg(codec::xmsgpack_codec_t<xrpc_msg_request_t>::encode(edge_msg), rpc_msg_request);
            auto cluster_addr =
                m_router_ptr->sharding_address_from_account(common::xaccount_address_t{edge_msg.m_account}, edge_sender.network_id(), type);
            assert(is_evm_tx ? common::has<common::xnode_type_t::evm_validator>(cluster_addr.type()) : common::has<common::xnode_type_t::consensus_validator>(cluster_addr.type()));
            vnetwork::xvnode_address_t vaddr{std::move(cluster_addr)};
            xkinfo("[global_trace][advance_rpc][forward shard]%s,%s,src %s,dst %s,old_msg=%" PRIx64",new_msg=%" PRIx64,
                   tx_hash.c_str(),
                   account.c_str(),
                   m_cluster_vhost->address().to_string().c_str(),
                   vaddr.to_string().c_str(),
                   message.hash(),
                   msg.hash());
            try {
                //m_cluster_vhost->forward_broadcast_message(msg, vaddr);
                std::error_code ec;
                m_cluster_vhost->broadcast(vaddr.xip2(), msg, ec);
                XMETRICS_GAUGE(metrics::rpc_auditor_forward_request, 1);
                if (ec) {
                    xwarn("[global_trace][advance_rpc][forward shard] %s src %s dst %s msg hash %" PRIx64 " msg id %" PRIx32,
                          tx_hash.c_str(),
                          m_cluster_vhost->address().to_string().c_str(),
                          vaddr.to_string().c_str(),
                          msg.hash(),
                          static_cast<std::uint32_t>(msg.id()));
                    // todo ?
                    // assert(false);
                }
            } catch (top::error::xtop_error_t const & eh) {
                xwarn("[global_trace][advance_rpc][forward shard] %s src %s dst %s msg hash %" PRIx64 " msg id %" PRIx32,
                      tx_hash.c_str(),
                      m_cluster_vhost->address().to_string().c_str(),
                      vaddr.to_string().c_str(),
                      msg.hash(),
                      static_cast<std::uint32_t>(msg.id()));
            }
        }
        ++count;
    }
}

void xcluster_rpc_handler::cluster_process_response(const xmessage_t & msg, const xvnode_address_t & edge_sender) {
    xrpc_msg_response_t shard_msg = codec::xmsgpack_codec_t<xrpc_msg_response_t>::decode(msg.payload());
    try {
        xkinfo("m_cluster_vhost response:%" PRIx64, msg.hash());
        std::error_code ec;
        m_cluster_vhost->send_to(shard_msg.m_source_address, msg, ec);
        if (ec) {
            // todo ?
            // assert(false);
        }
    } catch (top::error::xtop_error_t const & eh) {
        xwarn("[global_trace][advance_rpc][send] src %s send msg %" PRIx64 " to dst %s",
              m_cluster_vhost->address().to_string().c_str(),
              msg.hash(),
              shard_msg.m_source_address.to_string().c_str());
    }
}

void xcluster_rpc_handler::start() {
    m_cluster_vhost->register_message_ready_notify(xmessage_category_rpc, std::bind(&xcluster_rpc_handler::on_message, shared_from_this(), _1, _2));
    xinfo("cluster register rpc");
}

void xcluster_rpc_handler::stop() {
    m_cluster_vhost->unregister_message_ready_notify(xmessage_category_rpc);
    xinfo("cluster unregister rpc");
}
NS_END2
