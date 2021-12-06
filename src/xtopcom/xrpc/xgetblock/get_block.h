#pragma once

#include "json/json.h"
#include "xbase/xobject.h"
#include "xcodec/xmsgpack_codec.hpp"
#include "xdata/xcodec/xmsgpack/xelection_association_result_store_codec.hpp"
#include "xdata/xcodec/xmsgpack/xelection_result_store_codec.hpp"
#include "xdata/xcodec/xmsgpack/xstandby_result_store_codec.hpp"
#include "xdata/xelection/xelection_association_result_store.h"
#include "xdata/xelection/xelection_cluster_result.h"
#include "xdata/xelection/xelection_result_store.h"
#include "xdata/xelection/xstandby_result_store.h"
#include "xgrpcservice/xgrpc_service.h"
#include "xstore/xstore.h"
#include "xsyncbase/xsync_face.h"

#include <string>

namespace top {

namespace chain_info {

using namespace data::election;

const uint8_t ARG_TYPE_UINT64 = 1;
const uint8_t ARG_TYPE_STRING = 2;
const uint8_t ARG_TYPE_BOOL = 3;

using query_method_handler = std::function<void(void)>;

#define REGISTER_QUERY_METHOD(func_name)                                                                                                                                           \
    m_query_method_map.emplace(std::pair<std::string, query_method_handler>{std::string{#func_name}, std::bind(&get_block_handle::func_name, this)})

class xtx_exec_json_key {
public:
    xtx_exec_json_key(const std::string & rpc_version) {
        if (rpc_version == RPC_VERSION_V2) {
            m_send = "send_block_info";
            m_recv = "recv_block_info";
            m_confirm = "confirm_block_info";
        }
    }

    std::string m_send = "send_unit_info";
    std::string m_recv = "recv_unit_info";
    std::string m_confirm = "confirm_unit_info";
};
class get_block_handle : public rpc::xrpc_handle_face_t {
public:
    get_block_handle(store::xstore_face_t * store, base::xvblockstore_t * block_store, sync::xsync_face_t * sync)
      : m_store(store), m_block_store(block_store), m_sync(sync), m_edge_start_height(1), m_arc_start_height(1) {
        REGISTER_QUERY_METHOD(getBlock);
        REGISTER_QUERY_METHOD(getProperty);
        REGISTER_QUERY_METHOD(getAccount);
        REGISTER_QUERY_METHOD(getTransaction);
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
        REGISTER_QUERY_METHOD(getExchangeNodes);
        REGISTER_QUERY_METHOD(getConsensus);
        REGISTER_QUERY_METHOD(getStandbys);
        REGISTER_QUERY_METHOD(queryNodeInfo);
        REGISTER_QUERY_METHOD(queryNodeReward);

        REGISTER_QUERY_METHOD(getLatestBlock);
        REGISTER_QUERY_METHOD(getLatestFullBlock);
        REGISTER_QUERY_METHOD(getBlockByHeight);

        REGISTER_QUERY_METHOD(getSyncNeighbors);
        // REGISTER_QUERY_METHOD(get_sync_overview);
        // REGISTER_QUERY_METHOD(get_sync_detail_all_table);
        // REGISTER_QUERY_METHOD(get_sync_detail_processing_table);
        // REGISTER_QUERY_METHOD(get_all_sync_accounts);
        // REGISTER_QUERY_METHOD(get_syncing_accounts);
    }
    bool handle(std::string request) override;
    std::string get_response() override {
        m_js_rsp["result"] = m_result;
        std::string rsp;
        try {
            rsp = m_js_rsp.toStyledString();
        } catch (...) {
            xdbg("xJson toStyledString error");
        }
        return rsp;
    }
    xJson::Value get_block_json(data::xblock_t * bp, const std::string & rpc_version = RPC_VERSION_V2);
    void query_account_property_base(xJson::Value & jph, const std::string & owner, const std::string & prop_name, xaccount_ptr_t unitstate);
    void query_account_property(xJson::Value & jph, const std::string & owner, const std::string & prop_name);
    void query_account_property(xJson::Value & jph, const std::string & owner, const std::string & prop_name, const uint64_t height);
    void getLatestBlock();
    void getLatestFullBlock();
    void getBlockByHeight();
    void getAccount();
    uint64_t get_timer_height() const;
    void getTimerInfo();
    void getCGP();
    void getIssuanceDetail();
    void getWorkloadDetail();
    uint64_t get_timer_clock() const;
    xJson::Value parse_account(const std::string & account);
    void update_tx_state(xJson::Value & result, const xJson::Value & cons, const std::string & rpc_version);
    xJson::Value parse_tx(const uint256_t & tx_hash, xtransaction_t * cons_tx_ptr, const std::string & version);
    xJson::Value parse_tx(xtransaction_t * tx_ptr, const std::string & version);
    xJson::Value parse_action(const xaction_t & action);
    xJson::Value get_tx_exec_result(const std::string & account, uint64_t block_height, xtransaction_ptr_t tx_ptr, xlightunit_tx_info_ptr_t & recv_txinfo, const std::string & rpc_version);
    void getRecs();
    void getZecs();
    void getEdges();
    void getArcs();
    void getExchangeNodes();
    void getConsensus();
    void getStandbys();
    void queryNodeInfo();
    void queryNodeReward();
    xJson::Value parse_sharding_reward(const std::string & target, const std::string & prop_name);

private:
    void getBlock();
    void getProperty();
    void set_shared_info(xJson::Value & root, data::xblock_t * bp);
    void set_header_info(xJson::Value & header, data::xblock_t * bp);

    void set_property_info(xJson::Value & jph, const std::map<std::string, std::string> & ph);
    void set_addition_info(xJson::Value & body, data::xblock_t * bp);
    void set_fullunit_state(xJson::Value & body, data::xblock_t * bp);
    void set_body_info(xJson::Value & body, data::xblock_t * bp, const std::string & rpc_version);

    void getGeneralInfos();
    void getRootblockInfo();

    void getTransaction();

    void get_node_infos();

    void set_redeem_token_num(data::xaccount_ptr_t ac, xJson::Value & value);

    void set_proposal_map(xJson::Value & j, std::map<std::string, std::string> & ms);
    void set_proposal_map_v2(xJson::Value & j, std::map<std::string, std::string> & ms);

    void set_accumulated_issuance_yearly(xJson::Value & j, const std::string & value);

    // set json for slash
    void set_unqualified_node_map(xJson::Value & j, std::map<std::string, std::string> const & ms);
    void set_unqualified_slash_info_map(xJson::Value & j, std::map<std::string, std::string> const & ms);

    // set json for different tx types
    void parse_asset_out(xJson::Value & j, const data::xaction_t & action);
    void parse_create_contract_account(xJson::Value & j, const data::xaction_t & action);
    void parse_run_contract(xJson::Value & j, const data::xaction_t & action);
    void parse_asset_in(xJson::Value & j, const data::xaction_t & action);
    void parse_pledge_token(xJson::Value & j, const data::xaction_t & action);
    void parse_redeem_token(xJson::Value & j, const data::xaction_t & action);
    void parse_pledge_token_vote(xJson::Value & j, const data::xaction_t & action);
    void parse_redeem_token_vote(xJson::Value & j, const data::xaction_t & action);
    void set_account_keys_info(xJson::Value & j, const data::xaction_t & action);
    void set_lock_token_info(xJson::Value & j, const data::xaction_t & action);
    void set_unlock_token_info(xJson::Value & j, const data::xaction_t & action);
    void set_create_sub_account_info(xJson::Value & j, const data::xaction_t & action);
    void set_alias_name_info(xJson::Value & j, const data::xaction_t & action);

    void getSyncNeighbors();
    void get_sync_overview();
    void get_sync_detail_all_table();
    void get_sync_detail_processing_table();
    void get_all_sync_accounts();
    void get_syncing_accounts();

    void get_nodes(const std::string & sys_addr);

    std::string HexEncode(const std::string & str);
    void set_result(const std::string & result) {
        m_result = result;
    }
    xJson::Value m_js_req;
    xJson::Value m_js_rsp;
    store::xstore_face_t * m_store{nullptr};
    base::xvblockstore_t * m_block_store{nullptr};
    sync::xsync_face_t * m_sync{nullptr};
    std::string m_result;

    uint64_t m_edge_start_height;
    uint64_t m_arc_start_height;
    std::unordered_map<std::string, query_method_handler> m_query_method_map;
};

}  // namespace chain_info
}  // namespace top
