// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <atomic>
#include "xdata/xchain_param.h"
#include "xcodec/xmsgpack_codec.hpp"
#include "xrpc/xerror/xrpc_error.h"
#include "xrpc/xrpc_define.h"
#include "xrpc/xrpc_msg_define.h"
#include "xedge_rpc_vhost.h"
#include "xedge_rpc_session.hpp"
#include "xvnetwork/xvnetwork_error.h"
#include "xconfig/xconfig_register.h"
#include "xconfig/xpredefined_configurations.h"
#include "xelection/xcache/xdata_accessor_face.h"

#include <cinttypes>

NS_BEG2(top, xrpc)
using std::atomic_ullong;
using vnetwork::xmessage_t;
using vnetwork::xvnode_address_t;

template<class T>
class xforward_session_base;

template<class T>
class xedge_handler_base {
public:
    typedef T conn_type;
    xedge_handler_base(shared_ptr<xrpc_edge_vhost> edge_vhost, std::shared_ptr<asio::io_service> ioc,
                       observer_ptr<election::cache::xdata_accessor_face_t> const & election_cache_data_accessor);
    virtual ~xedge_handler_base() {}
    void init();
    virtual void on_message(const xvnode_address_t&, const xrpc_msg_response_t& msg);
    void edge_send_msg(const std::vector<std::shared_ptr<xrpc_msg_request_t>>& edge_msg_list, const std::string &tx_hash, const std::string &account);
    virtual void insert_session(const std::vector<shared_ptr<xrpc_msg_request_t>>&, const unordered_set<xvnode_address_t>&, shared_ptr<T>&);
    virtual enum_xrpc_type type() = 0;
    uint64_t add_seq_id() { return ++m_msg_seq_id; }
    uint64_t get_seq_id() { return m_msg_seq_id; }
    shared_ptr<xrpc_edge_vhost> get_rpc_edge_vhost() { return m_edge_vhost_ptr; }
    void reset_edge_handler(shared_ptr<xrpc_edge_vhost> edge_vhost){
        m_edge_vhost_ptr->reset_edge_vhost(edge_vhost);
    }
protected:
    std::unordered_map<uint64_t, xforward_session_base<T>*> m_forward_session_map;
    std::mutex                                              m_mutex;
    shared_ptr<xrpc_edge_vhost>                             m_edge_vhost_ptr;
    atomic_ullong                                           m_msg_seq_id{ 0 };
    std::shared_ptr<asio::io_service>                       m_ioc;
    observer_ptr<election::cache::xdata_accessor_face_t>    m_election_cache_data_accessor;
};

template <class T>
xedge_handler_base<T>::xedge_handler_base(shared_ptr<xrpc_edge_vhost> edge_vhost, std::shared_ptr<asio::io_service> ioc,
                                          observer_ptr<election::cache::xdata_accessor_face_t> const & election_cache_data_accessor)
    :m_edge_vhost_ptr(edge_vhost), m_ioc(ioc), m_election_cache_data_accessor(election_cache_data_accessor)
{
    assert(m_edge_vhost_ptr);
}

template <class T>
void xedge_handler_base<T>::init()
{
    assert(type() != enum_xrpc_type::enum_xrpc_error_type);
    m_edge_vhost_ptr->register_message_handler(type(), std::bind(&xedge_handler_base<T>::on_message, this, _1, _2));
}

template <class T>
void xedge_handler_base<T>::edge_send_msg(const std::vector<std::shared_ptr<xrpc_msg_request_t>>& edge_msg_list, const std::string &tx_hash, const std::string &account)
{
    for (auto msg_ptr : edge_msg_list) {
        try {
            xmessage_t msg;
            if (msg_ptr->m_tx_type == enum_xrpc_tx_type::enum_xrpc_tx_type) {
                xmessage_t tx_msg(codec::xmsgpack_codec_t<xrpc_msg_request_t>::encode(*msg_ptr), rpc_msg_request);
                msg = std::move(tx_msg);
            } else if (msg_ptr->m_tx_type == enum_xrpc_tx_type::enum_xrpc_query_type) {
                xmessage_t query_msg(codec::xmsgpack_codec_t<xrpc_msg_request_t>::encode(*msg_ptr), rpc_msg_query_request);
                msg = std::move(query_msg);
            } else {
                xerror("error tx_type %d", msg_ptr->m_tx_type);
                return;
            }

            uint32_t edge_max_msg_packet_size = XGET_CONFIG(edge_max_msg_packet_size);
            if (msg.payload().size() > edge_max_msg_packet_size) {
                throw xrpc_error{ enum_xrpc_error_code::rpc_param_param_error, "msg packet size " + std::to_string(msg.payload().size()) + " bigger than " + std::to_string(edge_max_msg_packet_size) };
            }

            auto vd = m_edge_vhost_ptr->get_vnetwork_driver();
            auto group_addr =
                m_edge_vhost_ptr->get_router()->sharding_address_from_account(common::xaccount_address_t{msg_ptr->m_account}, vd->network_id(), xnode_type_t::consensus_auditor);
            vnetwork::xvnode_address_t dst{ group_addr };
            if (msg_ptr->m_tx_type == enum_xrpc_tx_type::enum_xrpc_tx_type) {
                xdbg("[global_trace][edge][forward advance]%s,%s,src %s, dst %s,%" PRIx64,
                   tx_hash.c_str(),
                   msg_ptr->m_account.c_str(),
                   vd->address().to_string().c_str(),
                   dst.to_string().c_str(),
                   msg.hash());
                // vd->forward_broadcast_message(msg, dst);
                std::error_code ec;
                vd->broadcast(dst.xip2(), msg, ec);
                XMETRICS_GAUGE(metrics::rpc_edge_tx_request, 1);
                if (ec) {
                    xwarn("[global_trace][edge][forward advance] failed. %s,%s,src %s, dst %s,%" PRIx64,
                          tx_hash.c_str(),
                          msg_ptr->m_account.c_str(),
                          vd->address().to_string().c_str(),
                          dst.to_string().c_str(),
                          msg.hash());
                    assert(false);
                }
            } else {
                auto count = 0;
                auto msghash = msg.hash();
                auto cluster_addresses = vd->archive_addresses(common::xnode_type_t::storage_archive);

                for (auto & cluster : cluster_addresses) {
                    if ((msghash % cluster_addresses.size() == count || (msghash + 1) % cluster_addresses.size() == count)) {
                        xdbg("[global_trace][edge][forward advance]%s,src %s, dst %s, cluster size %zu, %" PRIx64,
                            msg_ptr->m_account.c_str(),
                            vd->address().to_string().c_str(),
                            cluster.to_string().c_str(),
                            cluster_addresses.size(),
                            msg.hash());
                        std::error_code ec;
                        vd->send_to(cluster.xip2(), msg, ec);
                        if (ec) {
                            xdbg("send_to arc fail: %s %s", ec.category().name(), ec.message().c_str());
                        }
                    }
                    ++count;
                }
                XMETRICS_GAUGE(metrics::rpc_edge_query_request, 1);
            }
        } catch (const xrpc_error &e) {
            throw e;
        } catch (top::error::xtop_error_t const & eh) {
            xwarn("[xrpc_edge_vhost] xtop_error_t exception caught: cateogry:%s; msg:%s; error code:%d; error msg:%s", eh.code().category().name(), eh.what(), eh.code().value(), eh.code().message().c_str());
            throw xrpc_error{ enum_xrpc_error_code::rpc_param_unkown_error, eh.what() };
        } catch (std::exception const & eh) {
            xwarn("[xrpc_edge_vhost] std::exception caught: %s", eh.what());
            throw xrpc_error{ enum_xrpc_error_code::rpc_param_unkown_error, eh.what() };
        } catch (...) {
            xwarn("[xrpc_edge_vhost] unknown exception caught");
            throw xrpc_error{ enum_xrpc_error_code::rpc_param_unkown_error, "unknown exception" };
        }
    }
}

template <class T>
void xedge_handler_base<T>::on_message(const xvnode_address_t&, const xrpc_msg_response_t& msg)
{
    xdbg_rpc("xrpc_edge_vhost edge_process_response:%s" , msg.to_string().c_str());
    std::lock_guard<std::mutex> lock(m_mutex);
    auto iter = m_forward_session_map.find(msg.m_uuid);
    if (iter != m_forward_session_map.end()) {
        iter->second->m_destination_address.erase(vnetwork::xvnode_address_t{
                    msg.m_signature_address.cluster_address()
        });
        if (iter->second->m_destination_address.empty()) {
            iter->second->write_response(msg.m_message_body);
            iter->second->cancel_timeout();
        }
    }
}
template <class T>
void xedge_handler_base<T>::insert_session(const std::vector<shared_ptr<xrpc_msg_request_t>>& edge_msg_ptr_list, const unordered_set<xvnode_address_t>& addr_set, shared_ptr<T>& response)
{
    using session_type = forward_session<T>;
    auto forward_session = shared_ptr<session_type>(new session_type(edge_msg_ptr_list, this, response, m_ioc, addr_set),
        [&m_forward_session_map = m_forward_session_map, &m_mutex = m_mutex](session_type* session) {
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                auto iter = m_forward_session_map.find(session->m_edge_msg_ptr_list.front()->m_uuid);
                if (iter != m_forward_session_map.end())
                    m_forward_session_map.erase(iter);
            }
            DELETE(session);
    });

    std::lock_guard<std::mutex> lock(m_mutex);
    m_forward_session_map.emplace(edge_msg_ptr_list.front()->m_uuid, forward_session.get());
    forward_session->set_timeout(TIME_OUT);
}
NS_END2
