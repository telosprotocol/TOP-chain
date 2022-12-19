#include "gtest/gtest.h"
#include "xgrpcservice/src/xrpc.grpc.pb.h"

#include "xsyncbase/xsync_face.h"
#include "xrpc/xgetblock/get_block.h"

using namespace top;
using namespace top::store;
using grpc::Status;


class test_grpc_service : public testing::Test {
 protected:
    void SetUp() override {
        m_store = xstore_factory::create_store_with_memdb();
        // auto sync_status = std::make_shared<syncbase::xsync_status_t>();
        // m_sync = std::make_shared<sync::xsync_t>(sync_status.get());
        auto handle = std::make_shared<chain_info::get_block_handle>(m_store.get(), m_block_store, m_sync.get());
        service.register_handle(handle);
        p_service = &service;
    }

    void TearDown() override {
    }
 public:
    top::xrpc_service m_grpc_service;
    xobject_ptr_t<xstore_face_t> m_store;
    base::xvblockstore_t* m_block_store{nullptr};
    std::shared_ptr<sync::xsync_face_t> m_sync{nullptr};
    top::xrpc_service::Service* p_service;
    top::rpc::xrpc_serviceimpl service;
    std::shared_ptr<chain_info::get_block_handle> handle;
};

TEST_F(test_grpc_service, call) {
   xrpc_request *request = new xrpc_request;
   Json::Value jv;
   jv["action"] = "getGeneralInfos";
   std::string req = jv.toStyledString();
   request->set_body(req);

   xrpc_reply *reply = new xrpc_reply;
   auto ret = p_service->call(nullptr, request, reply);
   EXPECT_EQ(Status::OK.ok(), ret.ok());
   delete request;
   delete reply;
}

TEST_F(test_grpc_service, table_stream_err) {
//    Json::Value jv;
//    jv["action"] = "getGeneralInfos";
//    std::string req = jv.toStyledString();
   xrpc_request *request = new xrpc_request;
//    request->set_body(req);

//    auto ret = p_service->table_stream(nullptr, request, nullptr);
//    EXPECT_EQ(Status::CANCELLED.ok(), ret.ok());

   request->set_body("aaa");
   auto ret = p_service->table_stream(nullptr, request, nullptr);
   EXPECT_EQ(Status::CANCELLED.ok(), ret.ok());

   delete request;
}

// TEST_F(test_grpc_service, table_stream) {
//    Json::Value jv;
//    jv["action"] = "stream_table";
//    std::string req = jv.toStyledString();
//    xrpc_request *request = new xrpc_request;
//    request->set_body(req);

//    // xrpc_reply *reply = new ServerWriter<xrpc_reply>();
//    auto ret = p_service->table_stream(nullptr, request, nullptr);
//    EXPECT_EQ(Status::CANCELLED.ok(), ret.ok());
// }
