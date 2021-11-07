// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include "xrpc/xgetblock/get_block.h"
#include "xrpc/xjson_proc.h"
#include "xrpc/xrpc_define.h"
#include "xstore/xstore_face.h"
#include "xtxpool_service_v2/xtxpool_service_face.h"
#include "xvnetwork/xvhost_face.h"
#include "xtxstore/xtxstore_face.h"

#include <string>
#include <utility>

NS_BEG2(top, xrpc)

using query_method_handler = std::function<void(xjson_proc_t &)>;

class xcluster_query_manager {
public:
    explicit xcluster_query_manager(observer_ptr<store::xstore_face_t> store,
                                    observer_ptr<base::xvblockstore_t> block_store,
                                    observer_ptr<base::xvtxstore_t> txstore,
                                    xtxpool_service_v2::xtxpool_proxy_face_ptr const & txpool_service,
                                    bool archive_flag = false);
    void call_method(xjson_proc_t & json_proc);
    void getAccount(xjson_proc_t & json_proc);
    void getTransaction(xjson_proc_t & json_proc);
    void get_transactionlist(xjson_proc_t & json_proc);
    void get_property(xjson_proc_t & json_proc);
    void getBlock(xjson_proc_t & json_proc);
    void getChainInfo(xjson_proc_t & json_proc);
    void getIssuanceDetail(xjson_proc_t & json_proc);
    void getTimerInfo(xjson_proc_t & json_proc);
    void queryNodeInfo(xjson_proc_t & json_proc);
    void getElectInfo(xjson_proc_t & json_proc);
    void queryNodeReward(xjson_proc_t & json_proc);
    void listVoteUsed(xjson_proc_t & json_proc);
    void queryVoterDividend(xjson_proc_t & json_proc);
    void queryProposal(xjson_proc_t & json_proc);
    void getStandbys(xjson_proc_t & json_proc);
    void getCGP(xjson_proc_t & json_proc);
    void getLatestTables(xjson_proc_t & json_proc);

private:
    void set_sharding_vote_prop(xjson_proc_t & json_proc, std::string & prop_name);
    void set_sharding_reward_claiming_prop(xjson_proc_t & json_proc, std::string & prop_name);
    int  get_transaction_on_demand(const std::string& account, xtransaction_t * tx_ptr, const string & version, const uint256_t& tx_hash, xJson::Value& result_json);
private:
    std::shared_ptr<top::vnetwork::xvnetwork_driver_face_t> m_shard_host;
    observer_ptr<store::xstore_face_t> m_store;
    observer_ptr<base::xvblockstore_t> m_block_store;
    observer_ptr<base::xvtxstore_t> m_txstore;
    xtxpool_service_v2::xtxpool_proxy_face_ptr m_txpool_service;
    std::unordered_map<std::string, query_method_handler> m_query_method_map;
    chain_info::get_block_handle m_bh;
    bool m_archive_flag{false};
};
NS_END2
