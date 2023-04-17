// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include "xshard_rpc_handler.h"

#include "xbase/xcontext.h"
#include "xbase/xmem.h"
#include "xbasic/xscope_executer.h"
#include "xmetrics/xmetrics.h"
#include "xrpc/xerror/xrpc_error.h"
#include "xrpc/xerror/xrpc_error_json.h"
#include "xrpc/xrpc_init.h"
#include "xrpc/xrpc_method.h"
#include "xrpc/xuint_format.h"

#include <cinttypes>

NS_BEG2(top, xrpc)
using base::xcontext_t;
using base::xstream_t;
using common::xnode_type_t;

#define max_shard_rpc_mailbox_num (10000)

// xshard_rpc_handler::xshard_rpc_handler(xvhost_face_t* shard_host)
xshard_rpc_handler::xshard_rpc_handler(std::shared_ptr<xvnetwork_driver_face_t> shard_host,
                                       xtxpool_service_v2::xtxpool_proxy_face_ptr const & txpool_service,
                                       observer_ptr<top::base::xiothread_t> thread)
  : m_shard_vhost(shard_host), m_txpool_service(txpool_service), m_thread(thread) {
    assert(nullptr != txpool_service);
    m_rule_mgr_ptr = top::make_unique<xfilter_manager>();
}

void xshard_rpc_handler::on_message(const xvnode_address_t & edge_sender, xmessage_t const & message, std::uint64_t const timer_height) {
#if defined(DEBUG)
    auto msg_id = message.id();
    xdbg_rpc("xshard_rpc_handler on_message,id(%x,%s), timer_height %lld", msg_id, edge_sender.to_string().c_str(), timer_height);
#endif
    xkinfo("[global_trace][shard_rpc][recv advance msg]src %s,dst %s, timer_height %lld, %" PRIx64,
           edge_sender.to_string().c_str(),
           m_shard_vhost->address().to_string().c_str(),
           timer_height,
           message.hash());

    auto self = shared_from_this();
    auto process_request = [self](base::xcall_t & call, const int32_t cur_thread_id, const uint64_t timenow_ms) -> bool {
        rpc_message_para_t * para = dynamic_cast<rpc_message_para_t *>(call.get_param1().get_object());
        auto message = para->m_message;
        auto edge_sender = para->m_sender;
        auto timer_height = para->m_timer_height;

        auto msgid = para->m_message.id();
        if (msgid == rpc_msg_request || msgid == rpc_msg_eth_request) {
            xrpc_msg_request_t msg = codec::xmsgpack_codec_t<xrpc_msg_request_t>::decode(para->m_message.payload());
            msg.m_advance_address = edge_sender;
            msg.m_timer_height = timer_height;
            self->shard_process_request(msg, edge_sender, message.hash());
            XMETRICS_GAUGE(metrics::rpc_validator_tx_request, 1);
        }
        return true;
    };
    int64_t in, out;
    int32_t queue_size = m_thread->count_calls(in, out);
    if (queue_size >= max_shard_rpc_mailbox_num) {
        xkinfo_rpc("xshard_rpc_handler::on_message shard rpc mailbox is full:%d", queue_size);
        XMETRICS_GAUGE(metrics::mailbox_rpc_validator_total, 0);
        return;
    }
    XMETRICS_GAUGE(metrics::mailbox_rpc_validator_total, 1);
    XMETRICS_GAUGE_SET_VALUE(metrics::mailbox_rpc_validator_cur, queue_size);
    base::xauto_ptr<rpc_message_para_t> para = new rpc_message_para_t(edge_sender, message, timer_height);
    base::xcall_t asyn_call(process_request, para.get());
    m_thread->send_call(asyn_call);
}

void xshard_rpc_handler::process_msg(const xrpc_msg_request_t & edge_msg) {
    // xinfo_rpc("process msg %d", edge_msg.m_tx_type);
    // xtop_scope_executer on_exit([&edge_msg, &json_proc] {
    //     json_proc.m_response_json[RPC_ERRNO] = RPC_OK_CODE;
    //     json_proc.m_response_json[RPC_ERRMSG] = RPC_OK_MSG;
    //     json_proc.m_response_json[RPC_SEQUENCE_ID] = edge_msg.m_client_id;
    // });
    switch (edge_msg.m_tx_type) {
    case enum_xrpc_tx_type::enum_xrpc_tx_type: {
        data::xtransaction_ptr_t tx_ptr = nullptr;
        if (false == data::xtransaction_t::set_tx_by_serialized_data(tx_ptr, edge_msg.m_message_body)) {
            xerror("xshard_rpc_handler::process_msg fail-parse tx from msg");
            return;
        }
        // json_proc.m_tx_ptr->m_from = edge_msg.m_advance_address;
        xdbg_rpc("deal tx hash: %s, version: %d, advance addr: %s",
                 data::uint_to_str(tx_ptr->digest().data(), tx_ptr->digest().size()).c_str(),
                 tx_ptr->get_tx_version(),
                 edge_msg.m_advance_address.to_string().c_str());

        /*consensus_service::xunit_service_face* unit_src =
            static_cast<consensus_service::xunit_service_face*>(xcontext_t::instance().get_global_object(enum_xtop_global_object_unit_service));
        unit_src->request_transaction_consensus(json_proc.m_tx_ptr);*/
        std::string tx_hash = data::uint_to_str(tx_ptr->digest().data(), tx_ptr->digest().size());
        // xkinfo("[global_trace][shard_rpc][push unit_service]%s,%s", tx_hash.c_str(), tx_ptr->get_source_addr().c_str());
        uint64_t now = (uint64_t)base::xtime_utl::gettimeofday();
        if (now < tx_ptr->get_fire_timestamp()) {
            XMETRICS_GAUGE(metrics::rpc_txdelay_client_timestamp_unmatch, 1);
        }

        if (xsuccess != m_txpool_service->request_transaction_consensus(tx_ptr, false)) {
            // throw xrpc_error{enum_xrpc_error_code::rpc_param_param_error, "tx hash or sign error"};
            xdbg("[global_trace][shard_rpc][push unit_service] fail %s,%s", tx_hash.c_str(), tx_ptr->source_address().to_string().c_str());
        } else {
            xdbg("[global_trace][shard_rpc][push unit_service] succ %s,%s", tx_hash.c_str(), tx_ptr->source_address().to_string().c_str());
        }
        break;
    }
    default: {
        xerror("process msg %d no match type", edge_msg.m_tx_type);
        // throw xrpc_error{enum_xrpc_error_code::rpc_shard_exec_error, "unknow msg type"};
    }
    }
}

void xshard_rpc_handler::shard_process_request(const xrpc_msg_request_t & edge_msg, const xvnode_address_t & edge_sender, const uint64_t msghash) {
    // TODO(jimmy) shard nodes no need send response msg
    // shared_ptr<xrpc_msg_response_t> response_msg_ptr = std::make_shared<xrpc_msg_response_t>(edge_msg);
    // bool send_to_edge = true;
    // xtop_scope_executer on_exit([response_msg_ptr, &msghash, &edge_sender, &send_to_edge, this] {
    //     if (send_to_edge) {
    //         response_msg_ptr->m_signature_address = vnetwork::address_cast<common::xnode_type_t::group>(this->m_shard_vhost->address());
    //         xmessage_t msg(codec::xmsgpack_codec_t<xrpc_msg_response_t>::encode(*response_msg_ptr), rpc_msg_response);
    //         xinfo_rpc("xshard_rpc_handler msg recv %" PRIx64 ", send %" PRIx64 ", %s", msghash, msg.hash(), response_msg_ptr->m_message_body.c_str());
    //         std::error_code ec;
    //         this->m_shard_vhost->send_to(edge_sender, msg, ec);
    //         if (ec) {
    //             // todo?
    //             // assert(false);
    //         }
    //     }
    // });
    // try {
    if (edge_msg.m_tx_type == enum_xrpc_tx_type::enum_xrpc_tx_type) {
        xdbg_rpc("xshard_rpc_handler msg recv tx %" PRIx64 ", send %s", msghash, edge_sender.to_string().c_str());
        // xjson_proc_t json_proc;
        process_msg(edge_msg);
        // response_msg_ptr->m_message_body = json_proc.get_response();
    }
    // } catch (const xrpc_error & e) {
    //     xwarn_rpc("error %s", e.what());
    //     xrpc_error_json error_json(e.code().value(), e.what(), edge_msg.m_client_id);
    //     // response_msg_ptr->m_message_body = error_json.write();
    // } catch (const std::exception & e) {
    //     xwarn_rpc("error %s", e.what());
    //     // xrpc_error_json error_json(RPC_EXCEPTION_CODE, e.what(), edge_msg.m_client_id);
    //     // response_msg_ptr->m_message_body = error_json.write();
    // } catch (...) {
    //     xwarn_rpc("error !!!!");
    //     // xrpc_error_json error_json(RPC_ERROR_CODE, RPC_ERROR_MSG, edge_msg.m_client_id);
    //     // response_msg_ptr->m_message_body = error_json.write();
    // }
}

void xshard_rpc_handler::start() {
    m_shard_vhost->register_message_ready_notify(xmessage_category_rpc, std::bind(&xshard_rpc_handler::on_message, shared_from_this(), _1, _2, _3));
    xinfo("shard register rpc");
}

void xshard_rpc_handler::stop() {
    m_shard_vhost->unregister_message_ready_notify(xmessage_category_rpc);
    xinfo("shard unregister rpc");
}

NS_END2
