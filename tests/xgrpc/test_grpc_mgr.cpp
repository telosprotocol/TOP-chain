#include "gtest/gtest.h"

#include "xsyncbase/xsync_face.h"
#include "xgrpcservice/xgrpc_service.h"
#include "xrpc/xgetblock/get_block.h"
#include "xgrpc_mgr/xgrpc_mgr.h"
#include "xmbus/xmessage_bus.h"

using namespace top;
using namespace top::store;
using namespace top::rpc;
using namespace top::grpcmgr;

class test_grpc_mgr : public testing::Test {
 protected:
    void SetUp() override {
        m_store = xstore_factory::create_store_with_memdb();

        // auto sync_status = std::make_shared<syncbase::xsync_status_t>();
        // m_sync = std::make_shared<sync::xsync_t>(sync_status.get());
    }

    void TearDown() override {
    }
 public:
    xobject_ptr_t<xstore_face_t> m_store;
    base::xvblockstore_t* m_block_store{nullptr};
    std::shared_ptr<sync::xsync_face_t> m_sync{nullptr};
};

TEST_F(test_grpc_mgr, grpc_init) {
    uint16_t grpc_port = 19082;
    auto ret = grpc_init(m_store.get(), m_block_store, m_sync.get(), grpc_port);
    EXPECT_EQ(0, ret);
}

TEST_F(test_grpc_mgr, handler_mgr) {
    auto handle_mgr = std::make_shared<handler_mgr>();
    auto ret = handle_mgr->handle("");
    EXPECT_EQ(false, ret);
    auto res = handle_mgr->get_response();
    EXPECT_EQ(0, res.size());

    auto handle_store = std::make_shared<chain_info::get_block_handle>(m_store.get(), m_block_store, m_sync.get());
    handle_mgr->add_handler(handle_store);
    ret = handle_mgr->handle("haha");
    EXPECT_EQ(true, ret);
    res = handle_mgr->get_response();
    Json::Value jv;
    jv["result"] = "json parse error";
    EXPECT_EQ(jv.toStyledString(), res);
}

TEST_F(test_grpc_mgr, grpcmgr) {
    auto mbus = make_observer<mbus::xmessage_bus_t>(new mbus::xmessage_bus_t);
    xobject_ptr_t<base::xiothread_t> grpc_thread = make_object_ptr<base::xiothread_t>();
    auto grpc_mgr = std::make_shared<grpcmgr::xgrpc_mgr_t>(mbus, make_observer(grpc_thread));

    grpc_mgr->try_add_listener(true);
    grpc_mgr->try_remove_listener(true);
}
