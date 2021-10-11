// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include "xarc_rpc_handler.h"

#include "xbase/xcontext.h"
#include "xcodec/xmsgpack_codec.hpp"
#include "xmetrics/xmetrics.h"
#include "xrpc/xerror/xrpc_error_json.h"
#include "xrpc/xrpc_init.h"
#include "xrpc/xrpc_method.h"
#include "xrpc/xuint_format.h"
#include "xvnetwork/xvnetwork_error.h"

#include <cinttypes>

NS_BEG2(top, xrpc)
using base::xcontext_t;
using base::xstream_t;
using data::xtransaction_ptr_t;
using data::xtransaction_t;

#define max_cluster_rpc_mailbox_num (10000)

xarc_rpc_handler::xarc_rpc_handler(std::shared_ptr<xvnetwork_driver_face_t> arc_vhost,
                                           observer_ptr<xrouter_face_t> router_ptr,
                                           xtxpool_service_v2::xtxpool_proxy_face_ptr const & txpool_service,
                                           observer_ptr<store::xstore_face_t> store,
                                           observer_ptr<base::xvblockstore_t> block_store,
                                           observer_ptr<top::base::xiothread_t> thread)
  : m_arc_vhost(arc_vhost)
  , m_router_ptr(router_ptr)
  , m_txpool_service(txpool_service)
  , m_rule_mgr_ptr(top::make_unique<xfilter_manager>())
  , m_arc_query_mgr(std::make_shared<xarc_query_manager>(store, block_store, txpool_service))
  , m_thread(thread) {
}

void xarc_rpc_handler::on_message(const xvnode_address_t & edge_sender, const xmessage_t & message) {
    XMETRICS_TIME_RECORD("rpc_net_iothread_dispatch_cluster_rpc_handler");
    auto msgid = message.id();

    xdbg_rpc("xarc_rpc_handler on_message,id(%x,%s)", msgid, edge_sender.to_string().c_str());  // address to_string

    auto process_request = [self = shared_from_this()](base::xcall_t & call, const int32_t cur_thread_id, const uint64_t timenow_ms) -> bool {
        rpc_message_para_t * para = dynamic_cast<rpc_message_para_t *>(call.get_param1().get_object());
        auto message = para->m_message;
        auto edge_sender = para->m_sender;
        auto msgid = message.id();
        if (msgid == rpc_msg_request || msgid == rpc_msg_query_request) {
            xrpc_msg_request_t msg = codec::xmsgpack_codec_t<xrpc_msg_request_t>::decode(message.payload());
            if (msgid == rpc_msg_request) {
                xdbg_rpc("wish arc tx");
                return true;
            } else {
                self->cluster_process_query_request(msg, edge_sender, message);
                XMETRICS_GAUGE(metrics::rpc_auditor_query_request, 1);
            }
        } else if (msgid == rpc_msg_response) {
            // xrpc_msg_response_t msg = codec::xmsgpack_codec_t<xrpc_msg_response_t>::decode(message.payload());
            self->cluster_process_response(message, edge_sender);
        }
        return true;
    };
    int64_t in, out;
    int32_t queue_size = m_thread->count_calls(in, out);
    if (queue_size >= max_cluster_rpc_mailbox_num) {
        xkinfo_rpc("xarc_rpc_handler::on_message cluster rpc mailbox is full:%d", queue_size);
        XMETRICS_GAUGE(metrics::mailbox_rpc_auditor_total, 0);
        return;
    }
    XMETRICS_GAUGE_SET_VALUE(metrics::mailbox_rpc_auditor_cur, queue_size);
    XMETRICS_GAUGE(metrics::mailbox_rpc_auditor_total, 1);

    base::xauto_ptr<rpc_message_para_t> para = new rpc_message_para_t(edge_sender, message);
    base::xcall_t asyn_call(process_request, para.get());
    m_thread->send_call(asyn_call);
}

void xarc_rpc_handler::cluster_process_request(const xrpc_msg_request_t & edge_msg, const xvnode_address_t & edge_sender, const xmessage_t & message) {
    std::string tx_hash;
    std::string account;
    if (edge_msg.m_tx_type == enum_xrpc_tx_type::enum_xrpc_tx_type) {
        xtransaction_ptr_t tx_ptr;
        auto ret = xtransaction_t::set_tx_by_serialized_data(tx_ptr, edge_msg.m_message_body);
        assert(ret == true);

        tx_hash = tx_ptr->get_digest_hex_str();
        account = tx_ptr->get_source_addr();
        xkinfo("[global_trace][advance_rpc][recv edge msg][push unit_service] deal tx hash: %s, version: %d, %s,src %s,dst %s,%" PRIx64,
               tx_hash.c_str(),
               tx_ptr->get_tx_version(),
               account.c_str(),
               edge_sender.to_string().c_str(),
               m_arc_vhost->address().to_string().c_str(),
               message.hash());

        if (xsuccess != m_txpool_service->request_transaction_consensus(tx_ptr, false)) {
            xkinfo("[global_trace][advance_rpc][recv edge msg][push unit_service] tx hash: %s,%s,src %s,dst %s,%" PRIx64 " ignored",
                  tx_hash.c_str(),
                  account.c_str(),
                  edge_sender.to_string().c_str(),
                  m_arc_vhost->address().to_string().c_str(),
                  message.hash());
            return;
        }
    } else {
        xerror("cluster error tx_type %d", edge_msg.m_tx_type);
        return;
    }

    auto neighbors = m_arc_vhost->neighbors_info2();
    auto me_addr = m_arc_vhost->address();
    size_t count = 0;
    for (auto & item : neighbors) {
        if (me_addr == item.second.address && (message.hash() % neighbors.size() == count || (message.hash() + 1) % neighbors.size() == count)) {
            xkinfo("m_arc_vhost send:%s,%" PRIu64, item.second.address.to_string().c_str(), message.hash());

            xmessage_t msg(codec::xmsgpack_codec_t<xrpc_msg_request_t>::encode(edge_msg), rpc_msg_request);
            auto cluster_addr =
                m_router_ptr->sharding_address_from_account(common::xaccount_address_t{edge_msg.m_account}, edge_sender.network_id(), common::xnode_type_t::consensus_validator);
            assert(common::has<common::xnode_type_t::consensus_validator>(cluster_addr.type()));
            vnetwork::xvnode_address_t vaddr{std::move(cluster_addr)};
            xkinfo("[global_trace][advance_rpc][forward shard]%s,%s,src %s,dst %s,%" PRIx64,
                   tx_hash.c_str(),
                   account.c_str(),
                   m_arc_vhost->address().to_string().c_str(),
                   vaddr.to_string().c_str(),
                   msg.hash());
            try {
                m_arc_vhost->forward_broadcast_message(msg, vaddr);
                XMETRICS_GAUGE(metrics::rpc_auditor_forward_request, 1);
            } catch (top::error::xtop_error_t const & eh) {
                xwarn("[global_trace][advance_rpc][forward shard] %s src %s dst %s msg hash %" PRIx64 " msg id %" PRIx32,
                      tx_hash.c_str(),
                      m_arc_vhost->address().to_string().c_str(),
                      vaddr.to_string().c_str(),
                      msg.hash(),
                      static_cast<std::uint32_t>(msg.id()));
            }
        }
        ++count;
    }
}

void xarc_rpc_handler::cluster_process_query_request(const xrpc_msg_request_t & edge_msg, const xvnode_address_t & edge_sender, const xmessage_t & message) {
    if (edge_msg.m_tx_type != enum_xrpc_tx_type::enum_xrpc_query_type) {
        xerror("cluster error tx_type %d", edge_msg.m_tx_type);
        return;
    }
    xinfo_rpc("process query msg %d, %s", edge_msg.m_tx_type, edge_msg.m_source_address.to_string().c_str());
    shared_ptr<xrpc_msg_response_t> response_msg_ptr = std::make_shared<xrpc_msg_response_t>(edge_msg);
    try {
        xjson_proc_t json_proc;
        json_proc.parse_json(edge_msg.m_message_body);
        m_rule_mgr_ptr->filter(json_proc);

        m_arc_query_mgr->call_method(json_proc);
        json_proc.m_response_json[RPC_ERRNO] = RPC_OK_CODE;
        json_proc.m_response_json[RPC_ERRMSG] = RPC_OK_MSG;
        json_proc.m_response_json[RPC_SEQUENCE_ID] = edge_msg.m_client_id;
        response_msg_ptr->m_message_body = json_proc.get_response();
    } catch (const xrpc_error & e) {
        xinfo_rpc("error %s", e.what());
        xrpc_error_json error_json(e.code().value(), e.what(), edge_msg.m_client_id);
        response_msg_ptr->m_message_body = error_json.write();
    } catch (const std::exception & e) {
        xinfo_rpc("error %s", e.what());
        xrpc_error_json error_json(RPC_EXCEPTION_CODE, e.what(), edge_msg.m_client_id);
        response_msg_ptr->m_message_body = error_json.write();
    } catch (...) {
        xinfo_rpc("error !!!!");
        xrpc_error_json error_json(RPC_ERROR_CODE, RPC_ERROR_MSG, edge_msg.m_client_id);
        response_msg_ptr->m_message_body = error_json.write();
    }

    response_msg_ptr->m_signature_address = m_arc_vhost->address();
    xmessage_t msg(codec::xmsgpack_codec_t<xrpc_msg_response_t>::encode(*response_msg_ptr), rpc_msg_response);
    xdbg_rpc("xarc_rpc_handler response recv %" PRIx64 ", send %" PRIx64 ", %s", message.hash(), msg.hash(), response_msg_ptr->m_message_body.c_str());
    m_arc_vhost->send_to(edge_sender, msg);
}

void xarc_rpc_handler::cluster_process_response(const xmessage_t & msg, const xvnode_address_t & edge_sender) {
    xrpc_msg_response_t shard_msg = codec::xmsgpack_codec_t<xrpc_msg_response_t>::decode(msg.payload());
    try {
        xkinfo("m_arc_vhost response:%" PRIx64, msg.hash());
        m_arc_vhost->send_to(shard_msg.m_source_address, msg);
    } catch (top::error::xtop_error_t const & eh) {
        xwarn("[global_trace][advance_rpc][send] src %s send msg %" PRIx64 " to dst %s",
              m_arc_vhost->address().to_string().c_str(),
              msg.hash(),
              shard_msg.m_source_address.to_string().c_str());
    }
}

void xarc_rpc_handler::start() {
    m_arc_vhost->register_message_ready_notify(xmessage_category_rpc, std::bind(&xarc_rpc_handler::on_message, shared_from_this(), _1, _2));
}

void xarc_rpc_handler::stop() {
    m_arc_vhost->unregister_message_ready_notify(xmessage_category_rpc);
}
NS_END2
