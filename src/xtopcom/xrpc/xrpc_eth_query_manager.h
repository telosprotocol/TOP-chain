#pragma once
#include <string>
#include "json/json.h"
#include "xbase/xobject.h"
#include "xcodec/xmsgpack_codec.hpp"
#include "xdata/xcodec/xmsgpack/xelection_association_result_store_codec.hpp"
#include "xdata/xelection/xelection_association_result_store.h"
#include "xdata/xelection/xelection_cluster_result.h"
#include "xdata/xelection/xelection_result_store.h"
#include "xdata/xelection/xstandby_result_store.h"
#include "xgrpcservice/xgrpc_service.h"
#include "xstore/xstore.h"
#include "xsyncbase/xsync_face.h"
#include "xvledger/xvtxstore.h"
#include "xvledger/xvledger.h"
#include "xtxpool_service_v2/xtxpool_service_face.h"
#include "xrpc/xjson_proc.h"
#include "xevm_common/fixed_hash.h"

namespace top {
namespace xrpc {

using namespace data::election;
using namespace top::data;

using query_method_handler = std::function<void(xJson::Value &, xJson::Value &, std::string &, uint32_t &)>;

#define ADDRESS_CHECK_VALID(x)                                                                                                                                                     \
    if (xverifier::xtx_utl::address_is_valid(x) != xverifier::xverifier_error::xverifier_success) {                                                                                \
        xwarn("xtx_verifier address_verify address invalid, account:%s", x.c_str());                                                                                               \
        strResult = "account address is invalid";                                                                                                                                  \
        nErrorCode = (uint32_t)enum_xrpc_error_code::rpc_param_param_error; \
        return; \
    }

#define REGISTER_ETH_QUERY_METHOD(func_name)                                                                                                                                           \
    m_query_method_map.emplace(std::pair<std::string, top::xrpc::query_method_handler>{std::string{#func_name},                                                                    \
                                                                                       std::bind(&xrpc_eth_query_manager::func_name, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4)})

class xrpc_eth_query_manager : public rpc::xrpc_handle_face_t {
public:
    xrpc_eth_query_manager(observer_ptr<store::xstore_face_t> store,
                       observer_ptr<base::xvblockstore_t> block_store,
                       sync::xsync_face_t * sync,
                       xtxpool_service_v2::xtxpool_proxy_face_ptr const & txpool_service,
                       observer_ptr<base::xvtxstore_t> txstore = nullptr,
                       bool exchange_flag = false)
        : m_store(store)
        , m_block_store(block_store)
        , m_sync(sync)
        , m_edge_start_height(1)
        , m_arc_start_height(1)
        , m_txpool_service(txpool_service)
        , m_txstore(txstore)
      , m_exchange_flag(exchange_flag) {

        REGISTER_ETH_QUERY_METHOD(eth_getBalance);
        REGISTER_ETH_QUERY_METHOD(eth_getTransactionByHash);
        REGISTER_ETH_QUERY_METHOD(eth_getTransactionReceipt);
        REGISTER_ETH_QUERY_METHOD(eth_getTransactionCount);
        REGISTER_ETH_QUERY_METHOD(eth_blockNumber);
        REGISTER_ETH_QUERY_METHOD(eth_getBlockByHash);
        REGISTER_ETH_QUERY_METHOD(eth_getBlockByNumber);
        REGISTER_ETH_QUERY_METHOD(eth_getCode);
        REGISTER_ETH_QUERY_METHOD(eth_call);
    }
    void call_method(std::string strMethod, xJson::Value & js_req, xJson::Value & js_rsp, std::string & strResult, uint32_t & nErrorCode);
    bool handle(std::string & strReq, xJson::Value & js_req, xJson::Value & js_rsp, std::string & strResult, uint32_t & nErrorCode) override;

    void eth_getBalance(xJson::Value & js_req, xJson::Value & js_rsp, string & strResult, uint32_t & nErrorCode);
    void eth_getTransactionByHash(xJson::Value & js_req, xJson::Value & js_rsp, std::string & strResult, uint32_t & nErrorCode);
    void eth_getTransactionReceipt(xJson::Value & js_req, xJson::Value & js_rsp, std::string & strResult, uint32_t & nErrorCode);
    void eth_getTransactionCount(xJson::Value & js_req, xJson::Value & js_rsp, std::string & strResult, uint32_t & nErrorCode);
    void eth_blockNumber(xJson::Value & js_req, xJson::Value & js_rsp, string & strResult, uint32_t & nErrorCode);
    void eth_getBlockByHash(xJson::Value & js_req, xJson::Value & js_rsp, string & strResult, uint32_t & nErrorCode);
    void eth_getBlockByNumber(xJson::Value & js_req, xJson::Value & js_rsp, string & strResult, uint32_t & nErrorCode);
    void eth_getCode(xJson::Value & js_req, xJson::Value & js_rsp, string & strResult, uint32_t & nErrorCode);
    void eth_call(xJson::Value & js_req, xJson::Value & js_rsp, string & strResult, uint32_t & nErrorCode);
    top::evm_common::h2048 calculate_bloom(const std::string & hexstr);
private:
    std::string safe_get_json_value(xJson::Value & json_value, const std::string& key);
    void set_block_result(const base::xauto_ptr<base::xvblock_t>&  block, xJson::Value& js_result);
private:
    observer_ptr<store::xstore_face_t> m_store;
    observer_ptr<base::xvblockstore_t> m_block_store;
    sync::xsync_face_t * m_sync{nullptr};
    uint64_t m_edge_start_height;
    uint64_t m_arc_start_height;
    std::unordered_map<std::string, top::xrpc::query_method_handler> m_query_method_map;
    std::shared_ptr<top::vnetwork::xvnetwork_driver_face_t> m_shard_host;
    xtxpool_service_v2::xtxpool_proxy_face_ptr m_txpool_service;
    observer_ptr<base::xvtxstore_t> m_txstore;
    bool m_exchange_flag{false};
};

}  // namespace xrpc
}  // namespace top
