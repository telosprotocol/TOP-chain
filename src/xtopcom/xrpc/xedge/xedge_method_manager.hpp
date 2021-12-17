// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include "xbase/xcontext.h"
#include "xbase/xutl.h"
#include "xchain_fork/xchain_upgrade_center.h"
#include "xdata/xcons_transaction.h"
#include "xdata/xnative_contract_address.h"
#include "xdata/xtransaction_cache.h"
#include "xdata/xtx_factory.h"
#include "xedge_local_method.hpp"
#include "xedge_rpc_handler.h"
#include "xmetrics/xmetrics.h"
#include "xrpc/xcluster/xcluster_query_manager.h"
#include "xrpc/xerror/xrpc_error.h"
#include "xrpc/xjson_proc.h"
#include "xrpc/xrpc_define.h"
#include "xrpc/xrpc_method.h"
#include "xrpc/xuint_format.h"
#include "xstore/xstore_face.h"
#include "xtxstore/xtransaction_prepare.h"
#include "xverifier/xwhitelist_verifier.h"
#include "xverifier/xblacklist_verifier.h"
#include "xvledger/xvblock.h"
#include "xvnetwork/xvhost_face.h"

NS_BEG2(top, xrpc)
using base::xcontext_t;

using tx_method_handler = std::function<void(xjson_proc_t & json_proc, const std::string & ip)>;

#define EDGE_REGISTER_V1_ACTION(T, func_name)                                                                                                                                      \
    m_edge_tx_method_map.emplace(                                                                                                                                                  \
        pair<pair<string, string>, tx_method_handler>{pair<string, string>{"1.0", #func_name}, std::bind(&xedge_method_base<T>::func_name##_method, this, _1, _2)})

template <class T>
class xedge_method_base {
public:
    typedef typename T::conn_type conn_type;
    xedge_method_base(shared_ptr<xrpc_edge_vhost> edge_vhost,
                      common::xip2_t xip2,
                      shared_ptr<asio::io_service> & ioc,
                      bool archive_flag = false,
                      observer_ptr<store::xstore_face_t> store = nullptr,
                      observer_ptr<base::xvblockstore_t> block_store = nullptr,
                      observer_ptr<base::xvtxstore_t> txstore = nullptr,
                      observer_ptr<elect::ElectMain> elect_main = nullptr,
                      observer_ptr<election::cache::xdata_accessor_face_t> const & election_cache_data_accessor = nullptr);
    virtual ~xedge_method_base() {
    }
    void do_method(shared_ptr<conn_type> & response, xjson_proc_t & json_proc, const std::string & ip);
    void sendTransaction_method(xjson_proc_t & json_proc, const std::string & ip);
    void forward_method(shared_ptr<conn_type> & response, xjson_proc_t & json_proc);
    shared_ptr<xrpc_msg_request_t> generate_request(const xvnode_address_t & source_address, const uint64_t uuid, const string account, xjson_proc_t & json_proc);
    virtual void write_response(shared_ptr<conn_type> & response, const string & content) = 0;
    T * get_edge_handler() {
        return m_edge_handler_ptr.get();
    }
    virtual enum_xrpc_type type() = 0;
    void reset_edge_method(shared_ptr<xrpc_edge_vhost> edge_vhost, common::xip2_t xip2) {
        m_edge_handler_ptr->reset_edge_handler(edge_vhost);
        m_edge_local_method_ptr->reset_edge_local_method(xip2);
    }

    void set_tx_by_version(xtransaction_ptr_t & tx_ptr, uint32_t version);

protected:
    unique_ptr<T> m_edge_handler_ptr;
    unordered_map<pair<string, string>, tx_method_handler> m_edge_tx_method_map;
    unique_ptr<xedge_local_method<T>> m_edge_local_method_ptr;
    std::shared_ptr<xcluster_query_manager> m_cluster_query_mgr;
    observer_ptr<store::xstore_face_t> m_store;
    top::observer_ptr<base::xvtxstore_t> m_txstore;
    bool m_archive_flag{false};  // for local query
    bool m_enable_sign{true};
};

class xedge_http_method : public xedge_method_base<xedge_http_handler> {
public:
    xedge_http_method(shared_ptr<xrpc_edge_vhost> edge_vhost,
                      common::xip2_t xip2,
                      shared_ptr<asio::io_service> & ioc,
                      bool archive_flag,
                      observer_ptr<store::xstore_face_t> store,
                      observer_ptr<base::xvblockstore_t> block_store,
                      observer_ptr<base::xvtxstore_t> txstore,
                      observer_ptr<elect::ElectMain> elect_main,
                      observer_ptr<election::cache::xdata_accessor_face_t> const & election_cache_data_accessor)
      : xedge_method_base<xedge_http_handler>(edge_vhost, xip2, ioc, archive_flag, store, block_store, txstore, elect_main, election_cache_data_accessor) {
    }
    void write_response(shared_ptr<conn_type> & response, const string & content) override {
        response->write(content);
    }
    enum_xrpc_type type() override {
        return enum_xrpc_type::enum_xrpc_http_type;
    }
};

class xedge_ws_method : public xedge_method_base<xedge_ws_handler> {
public:
    xedge_ws_method(shared_ptr<xrpc_edge_vhost> edge_vhost,
                    common::xip2_t xip2,
                    shared_ptr<asio::io_service> & ioc,
                    bool archive_flag,
                    observer_ptr<store::xstore_face_t> store,
                    observer_ptr<base::xvblockstore_t> block_store,
                    observer_ptr<base::xvtxstore_t> txstore,
                    observer_ptr<elect::ElectMain> elect_main,
                    observer_ptr<election::cache::xdata_accessor_face_t> const & election_cache_data_accessor)
      : xedge_method_base<xedge_ws_handler>(edge_vhost, xip2, ioc, archive_flag, store, block_store, txstore, elect_main, election_cache_data_accessor) {
    }
    void write_response(shared_ptr<conn_type> & response, const string & content) override {
        response->send(content, [](const error_code & ec) {
            if (ec) {
                xkinfo_rpc("Server: Error sending message. Error: %d, error message:%s", ec.value(), ec.message().c_str());
            }
        });
    }
    enum_xrpc_type type() override {
        return enum_xrpc_type::enum_xrpc_ws_type;
    }
};

template <class T>
xedge_method_base<T>::xedge_method_base(shared_ptr<xrpc_edge_vhost> edge_vhost,
                                        common::xip2_t xip2,
                                        shared_ptr<asio::io_service> & ioc,
                                        bool archive_flag,
                                        observer_ptr<store::xstore_face_t> store,
                                        observer_ptr<base::xvblockstore_t> block_store,
                                        observer_ptr<base::xvtxstore_t> txstore,
                                        observer_ptr<elect::ElectMain> elect_main,
                                        observer_ptr<election::cache::xdata_accessor_face_t> const & election_cache_data_accessor)
  : m_edge_local_method_ptr(top::make_unique<xedge_local_method<T>>(elect_main, xip2))
  , m_cluster_query_mgr(std::make_shared<xcluster_query_manager>(store, block_store, txstore, nullptr))
  , m_store(store)
  , m_txstore{txstore}
  , m_archive_flag(archive_flag)
{
    m_edge_handler_ptr = top::make_unique<T>(edge_vhost, ioc, election_cache_data_accessor);
    m_edge_handler_ptr->init();
    EDGE_REGISTER_V1_ACTION(T, sendTransaction);
}

template <class T>
void xedge_method_base<T>::do_method(shared_ptr<conn_type> & response, xjson_proc_t & json_proc, const std::string & ip) {
    // set account
    const string & version = json_proc.m_request_json["version"].asString();
    const string & method = json_proc.m_request_json["method"].asString();
    auto version_method = pair<string, string>(version, method);
    if (m_edge_local_method_ptr->do_local_method(version_method, json_proc)) {
        write_response(response, json_proc.get_response());
        return;
    }
    auto iter = m_edge_tx_method_map.find(version_method);
    if (iter != m_edge_tx_method_map.end()) {
        json_proc.m_tx_type = enum_xrpc_tx_type::enum_xrpc_tx_type;
        iter->second(json_proc, ip);
    } else {
        if (m_archive_flag) {
            xdbg("local arc query method: %s", method.c_str());
            m_cluster_query_mgr->call_method(json_proc);
            json_proc.m_response_json[RPC_ERRNO] = RPC_OK_CODE;
            json_proc.m_response_json[RPC_ERRMSG] = RPC_OK_MSG;
            write_response(response, json_proc.get_response());
            return;
        } else {
            json_proc.m_tx_type = enum_xrpc_tx_type::enum_xrpc_query_type;
            json_proc.m_account_set.emplace(json_proc.m_request_json["params"]["account_addr"].asString());
        }
    }
    assert(json_proc.m_account_set.size());
    forward_method(response, json_proc);
}

template <class T>
void xedge_method_base<T>::sendTransaction_method(xjson_proc_t & json_proc, const std::string & ip) {
    auto & request = json_proc.m_request_json["params"];
    json_proc.m_tx_ptr = xtx_factory::create_tx(static_cast<data::enum_xtransaction_version>(request["tx_structure_version"].asUInt()));
    auto & tx = json_proc.m_tx_ptr;
    tx->construct_from_json(request);

    if (!tx->digest_check()) {
        XMETRICS_COUNTER_INCREMENT("xtransaction_cache_fail_digest", 1);
        throw xrpc_error{enum_xrpc_error_code::rpc_param_param_error, "transaction hash error"};
    }
    if (!(tx->get_origin_target_addr() == sys_contract_rec_standby_pool_addr && tx->get_target_action_name() == "nodeJoinNetwork2")) {
        if (!tx->sign_check()) {
            XMETRICS_COUNTER_INCREMENT("xtransaction_cache_fail_sign", 1);
            throw xrpc_error{enum_xrpc_error_code::rpc_param_param_error, "transaction sign error"};
        }
    }
    tx->set_len();

    auto const& fork_config = top::chain_fork::xtop_chain_fork_config_center::chain_fork_config();
    auto logic_clock = (top::base::xtime_utl::gmttime() - top::base::TOP_BEGIN_GMTIME) / 10;
    if (chain_fork::xtop_chain_fork_config_center::is_forked(fork_config.blacklist_function_fork_point, logic_clock)) {
        xdbg_rpc("[sendTransaction_method] in blacklist fork point time, logic clock height: %" PRIu64, logic_clock);

        // filter out black list transaction
        if (xverifier::xblacklist_utl_t::is_black_address(tx->get_source_addr())) {
            xdbg_rpc("[sendTransaction_method] in black address rpc:%s, %s, %s", tx->get_digest_hex_str().c_str(), tx->get_target_addr().c_str(), tx->get_source_addr().c_str());
            XMETRICS_COUNTER_INCREMENT("xtransaction_cache_fail_blacklist", 1);
            throw xrpc_error{enum_xrpc_error_code::rpc_param_param_error, "blacklist check failed"};
        }
    } else {
        xdbg_rpc("[sendTransaction_method] not up to blacklist fork point time, logic clock height: %" PRIu64, logic_clock);
    }

    if (m_archive_flag) {
        xcons_transaction_ptr_t cons_tx = make_object_ptr<xcons_transaction_t>(tx.get());
        txexecutor::xtransaction_prepare_t tx_prepare(nullptr, cons_tx);
        int32_t ret = tx_prepare.check();
        if (ret != xsuccess) {
            XMETRICS_COUNTER_INCREMENT("xtransaction_cache_fail_prepare", 1);
            //std::string err = std::string("transaction txpool check error (") + std::to_string(ret) + ")";
            throw xrpc_error{enum_xrpc_error_code::rpc_param_param_error, tx_prepare.get_err_msg(ret)};
        }
        std::string old_target_addr = tx->get_origin_target_addr();
        if (is_sys_sharding_contract_address(common::xaccount_address_t{tx->get_origin_target_addr()})) {
            auto tableid = data::account_map_to_table_id(common::xaccount_address_t{tx->get_source_addr()});
            tx->adjust_target_address(tableid.get_subaddr());
        }
        // 1. validation check
        ret = xverifier::xtx_verifier::verify_send_tx_validation(tx.get());
        if (ret) {
            XMETRICS_COUNTER_INCREMENT("xtransaction_cache_fail_validation", 1);
            std::string err = std::string("transaction validation check failed (") + std::to_string(ret) + ")";
            throw xrpc_error{enum_xrpc_error_code::rpc_param_param_error, err};
        }
        // 2. legal check, include hash/signature check and white/black check
        if (xverifier::xwhitelist_utl::check_whitelist_limit_tx(tx.get())) {
            XMETRICS_COUNTER_INCREMENT("xtransaction_cache_fail_whitelist", 1);
            throw xrpc_error{enum_xrpc_error_code::rpc_param_param_error, "whitelist check failed"};
        }
        // 3. tx duration expire check
        uint64_t now = xverifier::xtx_utl::get_gmttime_s();
        ret = xverifier::xtx_verifier::verify_tx_duration_expiration(tx.get(), now);
        if (ret) {
            XMETRICS_COUNTER_INCREMENT("xtransaction_cache_fail_expiration", 1);
            throw xrpc_error{enum_xrpc_error_code::rpc_param_param_error, "duration expiration check failed"};
        }
        tx->set_target_addr(old_target_addr);

        std::string hash((char *)json_proc.m_tx_ptr->digest().data(), json_proc.m_tx_ptr->digest().size());
        if (m_txstore != nullptr) {
            m_txstore->tx_cache_add(hash, tx);
        }
    }

    std::string tx_hash = uint_to_str(json_proc.m_tx_ptr->digest().data(), json_proc.m_tx_ptr->digest().size());
    xinfo_rpc("send tx hash:%s", tx_hash.c_str());
    XMETRICS_PACKET_INFO("rpc_tx_ip",
                         "tx_hash", tx_hash,
                         "client_ip", ip);
    const auto & from = tx->get_source_addr();
    json_proc.m_account_set.emplace(from);
    json_proc.m_response_json["tx_hash"] = tx_hash;
    json_proc.m_response_json["tx_size"] = tx->get_tx_len();
}

template <class T>
shared_ptr<xrpc_msg_request_t> xedge_method_base<T>::generate_request(const xvnode_address_t & source_address,
                                                                      const uint64_t uuid,
                                                                      const string account,
                                                                      xjson_proc_t & json_proc) {
    shared_ptr<xrpc_msg_request_t> edge_msg_ptr;
    const string client_id = json_proc.m_request_json[RPC_SEQUENCE_ID].asString();
    if (json_proc.m_tx_type == enum_xrpc_tx_type::enum_xrpc_query_type) {
        edge_msg_ptr = std::make_shared<xrpc_msg_request_t>(type(), json_proc.m_tx_type, source_address, uuid, client_id, account, json_proc.get_request());
    } else if (json_proc.m_tx_type == enum_xrpc_tx_type::enum_xrpc_tx_type) {
        base::xstream_t stream(xcontext_t::instance());
        // json_proc.m_tx_ptr->add_modified_count();
        json_proc.m_tx_ptr->serialize_to(stream);
        std::string metric_name = "edge_txout_v" + std::to_string(json_proc.m_tx_ptr->get_tx_version()) + "_type" + std::to_string(json_proc.m_tx_ptr->get_tx_type());
        XMETRICS_COUNTER_INCREMENT(metric_name, stream.size());
        edge_msg_ptr =
            std::make_shared<xrpc_msg_request_t>(type(), json_proc.m_tx_type, source_address, uuid, client_id, account, string((const char *)stream.data(), stream.size()));
    } else {
        assert(false);
    }
    return edge_msg_ptr;
}

template <class T>
void xedge_method_base<T>::forward_method(shared_ptr<conn_type> & response, xjson_proc_t & json_proc) {
    do {
        // get shard address
        unordered_set<xvnode_address_t> shard_addr_set;
        std::vector<shared_ptr<xrpc_msg_request_t>> edge_msg_list;
        uint64_t uuid = m_edge_handler_ptr->add_seq_id();
        auto vd = m_edge_handler_ptr->get_rpc_edge_vhost()->get_vnetwork_driver();
        const xvnode_address_t & source_address = vd->address();

        for (const auto & account : json_proc.m_account_set) {
            auto cluster_addr = m_edge_handler_ptr->get_rpc_edge_vhost()->get_router()->sharding_address_from_account(
                common::xaccount_address_t{account}, vd->network_id(), common::xnode_type_t::consensus_validator);
            assert(common::has<common::xnode_type_t::consensus_validator>(cluster_addr.type()) || common::has<common::xnode_type_t::committee>(cluster_addr.type()) ||
                   common::has<common::xnode_type_t::zec>(cluster_addr.type()));
            vnetwork::xvnode_address_t shard_addr{std::move(cluster_addr)};

            if (shard_addr_set.find(shard_addr) == shard_addr_set.end()) {
                shared_ptr<xrpc_msg_request_t> edge_msg_ptr = generate_request(source_address, uuid, account, json_proc);
                edge_msg_list.emplace_back(edge_msg_ptr);
                shard_addr_set.emplace(shard_addr);
            }
        }
        if (edge_msg_list.empty()) {
            throw xrpc_error{enum_xrpc_error_code::rpc_param_param_lack, "msg list is empty"};
        }
        if (nullptr != json_proc.m_tx_ptr) {
            uint64_t now = (uint64_t)base::xtime_utl::gettimeofday();
            if (now < json_proc.m_tx_ptr->get_fire_timestamp()) {
                XMETRICS_GAUGE(metrics::txdelay_client_timestamp_unmatch, 1);
            }
            uint64_t delay_time_s = json_proc.m_tx_ptr->get_delay_from_fire_timestamp(now);
            XMETRICS_GAUGE(metrics::txdelay_from_client_to_edge, delay_time_s);
            m_edge_handler_ptr->edge_send_msg(edge_msg_list, json_proc.m_tx_ptr->get_digest_hex_str(), json_proc.m_tx_ptr->get_source_addr());
        } else {
            m_edge_handler_ptr->edge_send_msg(edge_msg_list, "", "");
        }
#if (defined ENABLE_RPC_SESSION) && (ENABLE_RPC_SESSION != 0)
        m_edge_handler_ptr->insert_session(edge_msg_list, shard_addr_set, response);
#else
        if (json_proc.m_tx_type == enum_xrpc_tx_type::enum_xrpc_tx_type) {
            json_proc.m_response_json[RPC_ERRNO] = RPC_OK_CODE;
            json_proc.m_response_json[RPC_ERRMSG] = RPC_OK_MSG;
            XMETRICS_COUNTER_INCREMENT("rpc_edge_tx_response", 1);
            write_response(response, json_proc.get_response());
        } else {
            XMETRICS_COUNTER_INCREMENT("rpc_edge_query_response", 1);
            // auditor return query result directly, so clear shard addr set
            shard_addr_set.clear();
            m_edge_handler_ptr->insert_session(edge_msg_list, shard_addr_set, response);
        }
#endif
    } while (0);
}

NS_END2
