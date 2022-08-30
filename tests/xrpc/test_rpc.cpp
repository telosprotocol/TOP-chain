#include "test_rpc.h"

#include "gtest/gtest.h"
#include "tests/xvnetwork/xdummy_vnetwork_driver.h"
#include "xelection/xcache/xdata_accessor_face.h"
#include "xrpc/xrpc_init.h"

using namespace top;
using namespace top::xrpc;

class test_rpc : public testing::Test {
protected:
    void SetUp() override {
        m_http_port = XGET_CONFIG(http_port);
        m_ws_port = XGET_CONFIG(ws_port);

        m_vhost = std::make_shared<tests::vnetwork::xdummy_vnetwork_driver_t>();
        m_router_ptr = top::make_observer<router::xrouter_t>(new router::xrouter_t);
        m_unit_service = std::make_shared<xtop_dummy_txpool_proxy_face>();
    }

    void TearDown() override {}

public:
    std::shared_ptr<tests::vnetwork::xdummy_vnetwork_driver_t> m_vhost;
    observer_ptr<xrouter_face_t> m_router_ptr{nullptr};
    xtxpool_service_v2::xtxpool_proxy_face_ptr m_unit_service;
    observer_ptr<base::xvblockstore_t> m_block_store;
    observer_ptr<base::xvtxstore_t> m_txstore;
    observer_ptr<elect::ElectMain> m_elect_main;
    observer_ptr<top::election::cache::xdata_accessor_face_t> m_election_cache_data_accessor;
    config::xtop_http_port_configuration::type m_http_port;
    config::xtop_ws_port_configuration::type m_ws_port;
};

// TEST_F(test_rpc, edge) {
//         auto m_rpc_services = std::make_shared<xrpc::xrpc_init>(m_vhost,
//                                                                 common::xnode_type_t::edge,
//                                                                 m_router_ptr,
//                                                                 m_http_port,
//                                                                 m_ws_port,
//                                                                 m_unit_service,
//                                                                 m_store,
//                                                                 m_block_store,
//                                                                 m_elect_main,
//                                                                 m_election_cache_data_accessor);
// }

TEST_F(test_rpc, auditor) {
    auto m_rpc_services = std::make_shared<xrpc::xrpc_init>(m_vhost,
                                                            common::xnode_type_t::consensus_auditor,
                                                            m_router_ptr,
                                                            m_http_port,
                                                            m_ws_port,
                                                            m_unit_service,
                                                            m_block_store,
                                                            m_txstore,
                                                            m_elect_main,
                                                            m_election_cache_data_accessor);
    m_rpc_services->stop();
}

TEST_F(test_rpc, validator) {
    auto m_rpc_services = std::make_shared<xrpc::xrpc_init>(m_vhost,
                                                            common::xnode_type_t::consensus_validator,
                                                            m_router_ptr,
                                                            m_http_port,
                                                            m_ws_port,
                                                            m_unit_service,
                                                            m_block_store,
                                                            m_txstore,
                                                            m_elect_main,
                                                            m_election_cache_data_accessor);
    m_rpc_services->stop();
}
