#pragma once

#include "xbase/xobject.h"
#include "xcodec/xmsgpack_codec.hpp"
#include "xdata/xcodec/xmsgpack/xelection_association_result_store_codec.hpp"
#include "xdata/xelection/xelection_association_result_store.h"
#include "xdata/xelection/xelection_cluster_result.h"
#include "xdata/xelection/xelection_result_store.h"
#include "xdata/xelection/xstandby_result_store.h"
#include "xdata/xunit_bstate.h"
#include "xgrpcservice/xgrpc_service.h"
#include "xrpc/xjson_proc.h"
#include "xsyncbase/xsync_face.h"
#include "xtxpool_service_v2/xtxpool_service_face.h"
#include "xvledger/xvledger.h"

#include <string>
#include <json/json.h>

namespace top {
namespace xrpc {

using query_method_handler = std::function<void(Json::Value &, Json::Value &, std::string &, uint32_t &)>;

enum enum_query_result {
    enum_success,
    enum_block_not_found,
    enum_unit_not_found,
};

class xrpc_eth_query_manager final : public rpc::xrpc_handle_face_t {
public:
    xrpc_eth_query_manager(xrpc_eth_query_manager const &) = delete;
    xrpc_eth_query_manager & operator=(xrpc_eth_query_manager const &) = delete;
    xrpc_eth_query_manager(xrpc_eth_query_manager &&) = default;
    xrpc_eth_query_manager & operator=(xrpc_eth_query_manager &&) = delete;
    ~xrpc_eth_query_manager() override = default;

    explicit xrpc_eth_query_manager(observer_ptr<base::xvblockstore_t> block_store);

    void call_method(std::string strMethod, Json::Value & js_req, Json::Value & js_rsp, std::string & strResult, uint32_t & nErrorCode);
    bool handle(std::string & strReq, Json::Value & js_req, Json::Value & js_rsp, std::string & strResult, uint32_t & nErrorCode) override;

    void eth_getBalance(Json::Value & js_req, Json::Value & js_rsp, string & strResult, uint32_t & nErrorCode);
    void eth_getTransactionByHash(Json::Value & js_req, Json::Value & js_rsp, std::string & strResult, uint32_t & nErrorCode);
    void eth_getTransactionReceipt(Json::Value & js_req, Json::Value & js_rsp, std::string & strResult, uint32_t & nErrorCode);
    void eth_getTransactionCount(Json::Value & js_req, Json::Value & js_rsp, std::string & strResult, uint32_t & nErrorCode);
    void eth_blockNumber(Json::Value & js_req, Json::Value & js_rsp, string & strResult, uint32_t & nErrorCode);
    void eth_getBlockByHash(Json::Value & js_req, Json::Value & js_rsp, string & strResult, uint32_t & nErrorCode);
    void eth_getBlockByNumber(Json::Value & js_req, Json::Value & js_rsp, string & strResult, uint32_t & nErrorCode);
    void eth_getCode(Json::Value & js_req, Json::Value & js_rsp, string & strResult, uint32_t & nErrorCode);
    void eth_call(Json::Value & js_req, Json::Value & js_rsp, string & strResult, uint32_t & nErrorCode);
    void eth_estimateGas(Json::Value & js_req, Json::Value & js_rsp, string & strResult, uint32_t & nErrorCode);
    void eth_getStorageAt(Json::Value & js_req, Json::Value & js_rsp, string & strResult, uint32_t & nErrorCode);
    void eth_getLogs(Json::Value & js_req, Json::Value & js_rsp, string & strResult, uint32_t & nErrorCode);
    void eth_feeHistory(Json::Value & js_req, Json::Value & js_rsp, string & strResult, uint32_t & nErrorCode);

    void topRelay_getBlockByNumber(Json::Value & js_req, Json::Value & js_rsp, string & strResult, uint32_t & nErrorCode);
    void topRelay_getBlockByHash(Json::Value & js_req, Json::Value & js_rsp, string & strResult, uint32_t & nErrorCode);
    void topRelay_blockNumber(Json::Value & js_req, Json::Value & js_rsp, string & strResult, uint32_t & nErrorCode);
    void topRelay_getTransactionByHash(Json::Value & js_req, Json::Value & js_rsp, string & strResult, uint32_t & nErrorCode);
    void topRelay_getTransactionReceipt(Json::Value & js_req, Json::Value & js_rsp, string & strResult, uint32_t & nErrorCode);

    void top_getBalance(Json::Value & js_req, Json::Value & js_rsp, string & strResult, uint32_t & nErrorCode);
private:
    std::string safe_get_json_value(Json::Value & json_value, const std::string& key);
    void set_block_result(const xobject_ptr_t<base::xvblock_t>&  block, Json::Value& js_result, bool fullTx, std::error_code & ec);
    enum_query_result query_account_by_number(const std::string &unit_address, const std::string& table_height, data::xunitstate_ptr_t& ptr);
    xobject_ptr_t<base::xvblock_t> query_block_by_height(const std::string& height_str);
    xobject_ptr_t<base::xvblock_t> query_relay_block_by_height(const std::string& height_str);
    uint64_t get_block_height(const std::string& table_height);
    int get_log(Json::Value & js_rsp, const uint64_t begin, const uint64_t end, const std::vector<std::set<std::string>>& vTopics, const std::set<std::string>& sAddress);
    bool check_log_is_match(evm_common::xevm_log_t const& log, const std::vector<std::set<std::string>>& vTopics, const std::set<std::string>& sAddress) const;
    bool check_block_log_bloom(xobject_ptr_t<base::xvblock_t>& block, const std::vector<std::set<std::string>>& vTopics, const std::set<std::string>& sAddress) const;
    int parse_topics(const Json::Value& t, std::vector<std::set<std::string>>& vTopics, Json::Value & js_rsp);
    int set_relay_block_result(const xobject_ptr_t<base::xvblock_t>& block, Json::Value & js_rsp, int have_txs, std::string blocklist_type);
private:
    observer_ptr<base::xvblockstore_t> m_block_store;
    std::unordered_map<std::string, query_method_handler> m_query_method_map;
};

}  // namespace xrpc
}  // namespace top
