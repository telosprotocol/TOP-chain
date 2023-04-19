#include "gtest/gtest.h"
#include "xrpc/xrpc_init.h"
#include "tests/xvnetwork/xdummy_vnetwork_driver.h"
#include "simplewebserver/server_http.hpp"
#include "simplewebserver/status_code.hpp"
#include "simplewebserver/client_https.hpp"
#include "xdata/xtransaction_v2.h"
#include "xdata/xaction_parse.h"
#include "xelect/client/xelect_client.h"
#include "xrpc/prerequest/xpre_request_handler_mgr.h"
#include "xrpc/xhttp/xhttp_server.h"
#include "xrpc/xerror/xrpc_error.h"

using namespace top;
using namespace top::xrpc;
using namespace std;

using HttpClient = SimpleWeb::Client<SimpleWeb::HTTP>;

class test_edge : public testing::Test {
 protected:
    void SetUp() override {
        m_vhost = std::make_shared<tests::vnetwork::xdummy_vnetwork_driver_t>();
        m_router_ptr = top::make_observer<router::xrouter_t>(new router::xrouter_t);
        auto const & config_register = top::config::xconfig_register_t::get_instance();
        m_http_port = XGET_CONFIG(http_port);
        m_ws_port = XGET_CONFIG(ws_port);
        m_thread = top::make_observer(base::xiothread_t::create_thread(base::xcontext_t::instance(), 0, -1));

        m_edge_handler = std::make_shared<xrpc_edge_vhost>(m_vhost, m_router_ptr, m_thread);
        m_rpc_service = top::make_unique<xrpc::xrpc_service<xedge_http_method>>(m_edge_handler, m_vhost->address().xip2(), false);
        m_rpc_ws_service = top::make_unique<xrpc::xrpc_service<xedge_ws_method>>(m_edge_handler, m_vhost->address().xip2(), false);
        m_pre_request_handler_mgr_ptr = top::make_unique<xpre_request_handler_mgr>();
        m_rule_mgr_ptr = top::make_unique<xfilter_manager>();
    }

    void TearDown() override {
    }

 public:
    std::string m_request_data{"target_account_addr=aaa&version=1.0&method=requestToken&sequence_id=1"};
    std::shared_ptr<top::tests::vnetwork::xdummy_vnetwork_driver_t> m_vhost;
    observer_ptr<xrouter_face_t> m_router_ptr{nullptr};
    std::shared_ptr<xrpc_edge_vhost> m_edge_handler{ nullptr };
    unique_ptr<xpre_request_handler_mgr> m_pre_request_handler_mgr_ptr;
    unique_ptr<xfilter_manager> m_rule_mgr_ptr;

    config::xtop_http_port_configuration::type m_http_port;
    config::xtop_ws_port_configuration::type m_ws_port;
    unique_ptr<xrpc::xrpc_service<xedge_http_method>> m_rpc_service;
    unique_ptr<xrpc::xrpc_service<xedge_ws_method>> m_rpc_ws_service;

    observer_ptr<top::base::xiothread_t> m_thread;
};

// TEST_F(test_edge, http) {
//     shared_ptr<xhttp_server> http_server_ptr = std::make_shared<xhttp_server>(m_edge_handler, m_vhost->address().xip2());
//     http_server_ptr->start(m_http_port, std::thread::hardware_concurrency());
// }

//TEST_F(test_edge, illegal_request) {
//    int32_t cnt{0};
//    auto content = "version=1.0&target_account_addr=T-123456789012345678901234567890123&identity_token=1111111&method=getAccount&sequence_id=2&body={\"params\":{\"account\":\"\"}}";
//    xpre_request_data_t pre_request_data;
//    m_pre_request_handler_mgr_ptr->execute(pre_request_data, content);
//    EXPECT_EQ(false, pre_request_data.m_finish);
//    xjson_proc_t json_proc;
//    json_proc.parse_json(pre_request_data);
//    try{
//        m_rule_mgr_ptr->filter(json_proc);
//    } catch(...){
//        cnt++;
//    }
//
//    content = "version=1.0&target_account_addr=T-123456789012345678901234567890123&identity_token=111&method=get_property&sequence_id=4&body={\"params\":{\"account\":\"T-123456789012345678901234567890124\",\"type\":\"string\",\"data\":55}}";
//    m_pre_request_handler_mgr_ptr->execute(pre_request_data, content);
//    json_proc.parse_json(pre_request_data);
//    try{
//        m_rule_mgr_ptr->filter(json_proc);
//    } catch(...){
//        cnt++;
//    }
//
//    auto tx = make_object_ptr<data::xtransaction_v2_t>();
//    tx->source_address("m_source_account");
//    tx->target_address("m_target_account");
//    Json::Value tx_json;
//    tx->parse_to_json(tx_json);
//    json_proc.m_request_json = tx_json;
//    try{
//        m_rule_mgr_ptr->filter(json_proc);
//    } catch(...){
//        cnt++;
//    }
//    EXPECT_EQ(3, cnt);
//
//    tx->target_address("");
//    tx->parse_to_json(tx_json);
//    json_proc.m_request_json = tx_json;
//    try{
//        m_rule_mgr_ptr->filter(json_proc);
//    } catch(...){
//        cnt++;
//    }
//    EXPECT_EQ(4, cnt);
//}

//TEST_F(test_edge, send_transaction) {
//    auto tx = make_object_ptr<data::xtransaction_v2_t>();
//    tx->source_address("m_source_account");
//    tx->target_address("m_target_account");
//    Json::Value tx_json;
//    tx->parse_to_json(tx_json);
//
//    xjson_proc_t json_proc;
//    json_proc.m_request_json = tx_json;
//
//    auto edge_http_method_ptr = m_rpc_service->m_edge_method_mgr_ptr.get();
//    try{
//        std::string ip = "127.0.0.1";
//        edge_http_method_ptr->sendTransaction_method(json_proc, ip);
//    }catch(xrpc_error& e){
//        EXPECT_EQ(string("transaction hash error"), string(e.what()));
//    }
//}

//TEST_F(test_edge, forward_method) {
//    auto tx = make_object_ptr<data::xtransaction_v2_t>();
//    tx->source_address("m_source_account");
//    tx->target_address("m_target_account");
//    Json::Value tx_json;
//    tx->parse_to_json(tx_json);
//
//    xjson_proc_t json_proc;
//    json_proc.m_request_json = tx_json;
//    shared_ptr<HttpServer::Response> response;
//
//    auto edge_http_method_ptr = m_rpc_service->m_edge_method_mgr_ptr.get();
//    try{
//        edge_http_method_ptr->forward_method(response, json_proc);
//    }catch(xrpc_error& e){
//        EXPECT_EQ(string("msg list is empty"), string(e.what()));
//    }
//
//    // json_proc.m_account_set.insert("T-a");
//    // edge_http_method_ptr = m_rpc_service->m_edge_method_mgr_ptr.get();
//    // try{
//    //     edge_http_method_ptr->forward_method(response, json_proc);
//    // }catch(xrpc_error& e){
//    //     EXPECT_EQ(string("msg list is empty"), string(e.what()));
//    // }
//}

TEST_F(test_edge, local_method){
    int32_t cnt{0};
    auto content = "version=1.0&target_account_addr=T-123456789012345678901234567890123&identity_token=1111111&method=account&sequence_id=2&body={\"params\":{\"account\":\"T-aaa\"}}";
    xpre_request_data_t pre_request_data;
    m_pre_request_handler_mgr_ptr->execute(pre_request_data, content);
    EXPECT_EQ(false, pre_request_data.m_finish);
    xjson_proc_t json_proc;
    json_proc.parse_json(pre_request_data);

    auto edge_local_method_ptr = top::make_unique<xedge_local_method<xedge_http_method>>(nullptr, m_vhost->address().xip2());

    // const string version = "1.0";
    // string method = "account";
    // auto version_method = pair<string, string>(version, method);
    // auto ret = edge_local_method_ptr->do_local_method(version_method, json_proc);
    // EXPECT_EQ(true, ret);

    // method = "sign_transaction";
    // version_method = pair<string, string>(version, method);
    // try{
    //     edge_local_method_ptr->do_local_method(version_method, json_proc);
    // } catch(...){
    //     cnt++;
    // }
    // EXPECT_EQ(1, cnt);

    // method = "import_private_key";
    // json_proc.m_request_json["params"]["private_key"] = "0x7ae7bbe2f230f0f116512d0092464e023600da585552280e04640dc8d55cf98a";
    // version_method = pair<string, string>(version, method);
    // ret = edge_local_method_ptr->do_local_method(version_method, json_proc);
    // EXPECT_EQ(true, ret);

    // method = "get_private_keys";
    // version_method = pair<string, string>(version, method);
    // ret = edge_local_method_ptr->do_local_method(version_method, json_proc);
    // EXPECT_EQ(true, ret);
    // json_proc.get_response();

    // method = "get_private_keys_wrong";
    // version_method = pair<string, string>(version, method);
    // ret = edge_local_method_ptr->do_local_method(version_method, json_proc);
    // EXPECT_EQ(false, ret);
}
