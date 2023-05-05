#pragma once
#include <string>
#include <json/json.h>
#include "xbase/xobject.h"
#include "xcodec/xmsgpack_codec.hpp"
#include "xdata/xcodec/xmsgpack/xelection_association_result_store_codec.hpp"
#include "xdata/xelection/xelection_association_result_store.h"
#include "xdata/xelection/xelection_cluster_result.h"
#include "xdata/xelection/xelection_result_store.h"
#include "xdata/xelection/xstandby_result_store.h"
#include "xgrpcservice/xgrpc_service.h"
#include "xsyncbase/xsync_face.h"
#include "xvledger/xvtxstore.h"
#include "xvledger/xvledger.h"
#include "xtxpool_service_v2/xtxpool_service_face.h"
#include "xrpc/xjson_proc.h"
#include "xrpc/xrpc_query_func.h"
#include "xrpc/xrpc_loader.h"
#include "xdata/xverifier/xverifier_utl.h"

namespace top {

namespace xrpc {

// using namespace data::election;
// using namespace top::data;
const uint8_t ARG_TYPE_UINT64 = 1;
const uint8_t ARG_TYPE_STRING = 2;
const uint8_t ARG_TYPE_BOOL = 3;

using query_method_handler = std::function<void(Json::Value &, Json::Value &, std::string &, uint32_t &)>;

#define ADDRESS_CHECK_VALID(x)                                                                                                                                                     \
    if (xverifier::xtx_utl::address_is_valid(x) != xverifier::xverifier_error::xverifier_success) {                                                                                \
        xwarn("xtx_verifier address_verify address invalid, account:%s", x.c_str());                                                                                               \
        strResult = "account address is invalid";                                                                                                                                  \
        nErrorCode = (uint32_t)enum_xrpc_error_code::rpc_param_param_error; \
        return; \
    }

#define REGISTER_QUERY_METHOD(func_name)                                                                                                                                           \
    m_query_method_map.emplace(std::pair<std::string, top::xrpc::query_method_handler>{std::string{#func_name},                                                                    \
                                                                                       std::bind(&xrpc_query_manager::func_name, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4)})

class xtx_exec_json_key {
public:
    xtx_exec_json_key(const std::string & rpc_version) {
        if (rpc_version == top::data::RPC_VERSION_V2) {
            m_send = "send_block_info";
            m_recv = "recv_block_info";
            m_confirm = "confirm_block_info";
        }
    }

    std::string m_send = "send_unit_info";
    std::string m_recv = "recv_unit_info";
    std::string m_confirm = "confirm_unit_info";
};

class xrpc_query_manager : public rpc::xrpc_handle_face_t {
public:
    xrpc_query_manager(observer_ptr<base::xvblockstore_t> block_store,
                       sync::xsync_face_t * sync,
                       xtxpool_service_v2::xtxpool_proxy_face_ptr const & txpool_service,
                       observer_ptr<base::xvtxstore_t> txstore = nullptr,
                       bool exchange_flag = false)
        : m_block_store(block_store)
        , m_sync(sync)
        , m_edge_start_height(1)
        , m_arc_start_height(1)
        , m_txpool_service(txpool_service)
        , m_txstore(txstore)
        , m_exchange_flag(exchange_flag) {
        REGISTER_QUERY_METHOD(getBlock);
        REGISTER_QUERY_METHOD(getProperty);
        REGISTER_QUERY_METHOD(getAccount);
        REGISTER_QUERY_METHOD(getTransaction);
        REGISTER_QUERY_METHOD(getTransactionV2);
        REGISTER_QUERY_METHOD(getGeneralInfos);
        REGISTER_QUERY_METHOD(getRootblockInfo);
        REGISTER_QUERY_METHOD(getTimerInfo);
        REGISTER_QUERY_METHOD(getCGP);
        REGISTER_QUERY_METHOD(getIssuanceDetail);
        REGISTER_QUERY_METHOD(getWorkloadDetail);

        REGISTER_QUERY_METHOD(getRecs);
        REGISTER_QUERY_METHOD(getZecs);
        REGISTER_QUERY_METHOD(getEdges);
        REGISTER_QUERY_METHOD(getArcs);
        REGISTER_QUERY_METHOD(getEVMs);
        REGISTER_QUERY_METHOD(getExchangeNodes);
        //REGISTER_QUERY_METHOD(getFullNodes);
        REGISTER_QUERY_METHOD(getFullNodes2);
        REGISTER_QUERY_METHOD(getConsensus);
        REGISTER_QUERY_METHOD(getStandbys);
        REGISTER_QUERY_METHOD(queryAllNodeInfo);
        REGISTER_QUERY_METHOD(queryNodeReward);

        REGISTER_QUERY_METHOD(getLatestBlock);
        REGISTER_QUERY_METHOD(getLatestFullBlock);
        REGISTER_QUERY_METHOD(getBlockByHeight);
        REGISTER_QUERY_METHOD(getBlocksByHeight);
        REGISTER_QUERY_METHOD(getSyncNeighbors);

        REGISTER_QUERY_METHOD(get_property);
        REGISTER_QUERY_METHOD(getChainInfo);
        REGISTER_QUERY_METHOD(queryNodeInfo);
        REGISTER_QUERY_METHOD(getElectInfo);
        REGISTER_QUERY_METHOD(listVoteUsed);
        REGISTER_QUERY_METHOD(queryVoterDividend);
        REGISTER_QUERY_METHOD(queryProposal);
        REGISTER_QUERY_METHOD(getLatestTables);
        REGISTER_QUERY_METHOD(getChainId);
        REGISTER_QUERY_METHOD(getCrossReceiptIds);
#if defined(XBUILD_CONSORTIUM)
        REGISTER_QUERY_METHOD(getConsortiumReward);
#endif 
    }
    void call_method(std::string strMethod, Json::Value & js_req, Json::Value & js_rsp, std::string & strResult, uint32_t & nErrorCode);
    bool handle(std::string & strReq, Json::Value & js_req, Json::Value & js_rsp, std::string & strResult, uint32_t & nErrorCode) override;
    Json::Value get_block_json(data::xblock_t * bp, const std::string & rpc_version = data::RPC_VERSION_V2);
    Json::Value get_blocks_json(data::xblock_t * bp, const std::string & rpc_version = data::RPC_VERSION_V2);
    //void query_account_property_base(Json::Value & jph, const std::string & owner, const std::string & prop_name, top::data::xunitstate_ptr_t unitstate, bool compatible_mode);
    //void query_account_property(Json::Value & jph, const std::string & owner, const std::string & prop_name, xfull_node_compatible_mode_t compatible_mode);
    //void query_account_property(Json::Value & jph, const std::string & owner, const std::string & prop_name, const uint64_t height, xfull_node_compatible_mode_t compatible_mode);
    void getLatestBlock(Json::Value & js_req, Json::Value & js_rsp, std::string & strResult, uint32_t & nErrorCode);
    void getLatestFullBlock(Json::Value & js_req, Json::Value & js_rsp, std::string & strResult, uint32_t & nErrorCode);
    void getBlockByHeight(Json::Value & js_req, Json::Value & js_rsp, std::string & strResult, uint32_t & nErrorCode);
    void getBlocksByHeight(Json::Value & js_req, Json::Value & js_rsp, std::string & strResult, uint32_t & nErrorCode);
    void getAccount(Json::Value & js_req, Json::Value & js_rsp, std::string & strResult, uint32_t & nErrorCode);
    uint64_t get_timer_height() const;
    void getTimerInfo(Json::Value & js_req, Json::Value & js_rsp, std::string & strResult, uint32_t & nErrorCode);
    void getCGP(Json::Value & js_req, Json::Value & js_rsp, std::string & strResult, uint32_t & nErrorCode);
    void getIssuanceDetail(Json::Value & js_req, Json::Value & js_rsp, std::string & strResult, uint32_t & nErrorCode);
    void getWorkloadDetail(Json::Value & js_req, Json::Value & js_rsp, std::string & strResult, uint32_t & nErrorCode);
    uint64_t get_timer_clock() const;
    Json::Value parse_account(const std::string & account, string & strResult, uint32_t & nErrorCode);
    void update_tx_state(Json::Value & result, const Json::Value & cons, const std::string & rpc_version);
    Json::Value parse_tx(top::data::xtransaction_t * tx_ptr, const std::string & version);
    int parse_tx(const std::string & tx_hash, data::xtransaction_t * txpool_tx, xtxindex_detail_ptr_t const& sendindex, const std::string & version, Json::Value & result_json, std::string & strResult, uint32_t & nErrorCode);
    Json::Value parse_action(const top::data::xaction_t & action, data::xtransaction_t const * tx);
    void getRecs(Json::Value & js_req, Json::Value & js_rsp, std::string & strResult, uint32_t & nErrorCode);
    void getZecs(Json::Value & js_req, Json::Value & js_rsp, std::string & strResult, uint32_t & nErrorCode);
    void getEdges(Json::Value & js_req, Json::Value & js_rsp, std::string & strResult, uint32_t & nErrorCode);
    void getArcs(Json::Value & js_req, Json::Value & js_rsp, std::string & strResult, uint32_t & nErrorCode);
    void getEVMs(Json::Value & js_req, Json::Value & js_rsp, std::string & strResult, uint32_t & nErrorCode);
    void getExchangeNodes(Json::Value & js_req, Json::Value & js_rsp, std::string & strResult, uint32_t & nErrorCode);
    //void getFullNodes(Json::Value & js_req, Json::Value & js_rsp, std::string & strResult, uint32_t & nErrorCode);
    void getFullNodes2(Json::Value & js_req, Json::Value & js_rsp, std::string & strResult, uint32_t & nErrorCode);
    void getConsensus(Json::Value & js_req, Json::Value & js_rsp, std::string & strResult, uint32_t & nErrorCode);
    void getStandbys(Json::Value & js_req, Json::Value & js_rsp, std::string & strResult, uint32_t & nErrorCode);
    void queryAllNodeInfo(Json::Value & js_req, Json::Value & js_rsp, std::string & strResult, uint32_t & nErrorCode);
    void queryNodeReward(Json::Value & js_req, Json::Value & js_rsp, std::string & strResult, uint32_t & nErrorCode);
    Json::Value parse_sharding_reward(const std::string & target, const std::string & prop_name, string & version);
    void getChainId(Json::Value & js_req, Json::Value & js_rsp, std::string & strResult, uint32_t & nErrorCode);
    void getCrossReceiptIds(Json::Value & js_req, Json::Value & js_rsp, std::string & strResult, uint32_t & nErrorCode);
#if defined(XBUILD_CONSORTIUM)
    void getConsortiumReward(Json::Value & js_req, Json::Value & js_rsp, string & strResult, uint32_t & nErrorCode);
#endif 

private:
    void getBlock(Json::Value & js_req, Json::Value & js_rsp, std::string & strResult, uint32_t & nErrorCode);
    void getProperty(Json::Value & js_req, Json::Value & js_rsp, std::string & strResult, uint32_t & nErrorCode);
    void set_shared_info(Json::Value & root, data::xblock_t * bp);
    void set_header_info(Json::Value & header, data::xblock_t * bp);

    void set_property_info(Json::Value & jph, const std::map<std::string, std::string> & ph);
    void set_addition_info(Json::Value & body, data::xblock_t * bp);
    void set_fullunit_state(Json::Value & body, data::xblock_t * bp);
    void set_body_info(Json::Value & body, data::xblock_t * bp, const std::string & rpc_version);

    void getGeneralInfos(Json::Value & js_req, Json::Value & js_rsp, std::string & strResult, uint32_t & nErrorCode);
    void getRootblockInfo(Json::Value & js_req, Json::Value & js_rsp, std::string & strResult, uint32_t & nErrorCode);

    void getTransaction(Json::Value & js_req, Json::Value & js_rsp, std::string & strResult, uint32_t & nErrorCode);
    void getTransactionV2(Json::Value & js_req, Json::Value & js_rsp, std::string & strResult, uint32_t & nErrorCode);

    void get_node_infos();

    void set_redeem_token_num(data::xunitstate_ptr_t ac, Json::Value & value);

    void set_proposal_map(Json::Value & j, std::map<std::string, std::string> & ms);
    void set_proposal_map_v2(Json::Value & j, std::map<std::string, std::string> & ms);

    void set_accumulated_issuance_yearly(Json::Value & j, const std::string & value);

    // set json for slash
    void set_unqualified_node_map(Json::Value & j, std::map<std::string, std::string> const & ms);
    void set_unqualified_slash_info_map(Json::Value & j, std::map<std::string, std::string> const & ms);

    // set json for different tx types
    void parse_asset_out(Json::Value & j, const data::xaction_t & action);
    void parse_create_contract_account(Json::Value & j, const data::xaction_t & action);
    void parse_run_contract(Json::Value & j, const data::xaction_t & action, data::xtransaction_t const * tx) const;
    void parse_asset_in(Json::Value & j, const data::xaction_t & action);
    void parse_pledge_token(Json::Value & j, const data::xaction_t & action);
    void parse_redeem_token(Json::Value & j, const data::xaction_t & action);
    void parse_pledge_token_vote(Json::Value & j, const data::xaction_t & action);
    void parse_redeem_token_vote(Json::Value & j, const data::xaction_t & action);
    void set_account_keys_info(Json::Value & j, const data::xaction_t & action);
    void set_lock_token_info(Json::Value & j, const data::xaction_t & action);
    void set_unlock_token_info(Json::Value & j, const data::xaction_t & action);
    void set_create_sub_account_info(Json::Value & j, const data::xaction_t & action);
    void set_alias_name_info(Json::Value & j, const data::xaction_t & action);

    void getSyncNeighbors(Json::Value & js_req, Json::Value & js_rsp, std::string & strResult, uint32_t & nErrorCode);

    void get_property(Json::Value & js_req, Json::Value & js_rsp, std::string & strResult, uint32_t & nErrorCode);
    void getChainInfo(Json::Value & js_req, Json::Value & js_rsp, std::string & strResult, uint32_t & nErrorCode);
    void queryNodeInfo(Json::Value & js_req, Json::Value & js_rsp, std::string & strResult, uint32_t & nErrorCode);
    void getElectInfo(Json::Value & js_req, Json::Value & js_rsp, std::string & strResult, uint32_t & nErrorCode);
    void listVoteUsed(Json::Value & js_req, Json::Value & js_rsp, std::string & strResult, uint32_t & nErrorCode);
    void queryVoterDividend(Json::Value & js_req, Json::Value & js_rsp, std::string & strResult, uint32_t & nErrorCode);
    void queryProposal(Json::Value & js_req, Json::Value & js_rsp, std::string & strResult, uint32_t & nErrorCode);
    void getLatestTables(Json::Value & js_req, Json::Value & js_rsp, std::string & strResult, uint32_t & nErrorCode);



    void get_sync_overview();
    void get_sync_detail_all_table();
    void get_sync_detail_processing_table();
    void get_all_sync_accounts();
    void get_syncing_accounts();

    void get_nodes(const std::string & sys_addr);

    std::string HexEncode(const std::string & str);

private:
    void set_sharding_vote_prop(Json::Value & js_req, Json::Value & js_rsp, std::string & prop_name, std::string & strResult, uint32_t & nErrorCode);
    void set_sharding_reward_claiming_prop(Json::Value & js_req, Json::Value & js_rsp, std::string & prop_name, std::string & strResult, uint32_t & nErrorCode);
    int get_transaction_on_demand(const std::string & account,
                                  const std::string & version,
                                  const uint256_t & tx_hash,
                                  Json::Value & result_json,
                                  std::string & strResult,
                                  uint32_t & nErrorCode);

private:
    observer_ptr<base::xvblockstore_t> m_block_store;
    sync::xsync_face_t * m_sync{nullptr};
    uint64_t m_edge_start_height;
    uint64_t m_arc_start_height;
    std::unordered_map<std::string, top::xrpc::query_method_handler> m_query_method_map;
    std::shared_ptr<top::vnetwork::xvnetwork_driver_face_t> m_shard_host;
    xtxpool_service_v2::xtxpool_proxy_face_ptr m_txpool_service;
    observer_ptr<base::xvtxstore_t> m_txstore;
    bool m_exchange_flag{false};
    xrpc_query_func m_xrpc_query_func;
};

}  // namespace xrpc
}  // namespace top
