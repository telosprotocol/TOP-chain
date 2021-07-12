// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xgrpcservice/xgrpc_service.h"

#include "xbase/xbase.h"
#include "xbasic/xutility.h"

#include <grpcpp/grpcpp.h>

#include <atomic>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

using namespace std;

using grpc::Server;
using grpc::ServerAsyncResponseWriter;
using grpc::ServerBuilder;
using grpc::ServerCompletionQueue;
using top::xrpc_reply;
using top::xrpc_request;
using top::xrpc_service;

namespace top {
namespace rpc {

std::atomic_int rpc_client_num{0};
std::deque<xJson::Value> tableblock_data;
std::mutex tableblock_mtx;
std::condition_variable tableblock_cv;

void xrpc_serviceimpl::register_handle(const std::shared_ptr<xrpc_handle_face_t> & handle) {
    m_handle = handle;
}

Status xrpc_serviceimpl::call(ServerContext * context, const xrpc_request * request, xrpc_reply * reply) {
    std::lock_guard<std::mutex> lock(m_call_mtx);
    string req = request->body();
    m_handle->handle(req);
    string rsp = m_handle->get_response();
    reply->set_body(rsp);
    return Status::OK;
}

Status xrpc_serviceimpl::table_stream(ServerContext * context, const xrpc_request * request, ServerWriter<xrpc_reply> * replys) {
    string req = request->body();
    xJson::Reader reader;
    xJson::Value js_req;
    if (!reader.parse(req, js_req)) {
        xdbg("grpc stream: %s json parse error", req.c_str());
        return Status(grpc::StatusCode::INVALID_ARGUMENT, "Invalid request argument: json parse error.");
    }
    rpc_client_num++;
    xdbg("grpc stream: consuming tableblock_data");
    while (true) {
        std::unique_lock<std::mutex> lck(tableblock_mtx);
        while (tableblock_data.empty()) {
            xdbg("grpc stream: empty tableblock_data , size %zu , waiting", tableblock_data.size());
            tableblock_cv.wait(lck);
        }

        if (context->IsCancelled()) {
            xinfo("grpc stream: cancelled");
            rpc_client_num--;
            return Status(grpc::StatusCode::CANCELLED, "Deadline exceeded or Client cancelled, abandoning.");
        }

        xdbg("grpc stream: tableblock_data before pop size %zu", tableblock_data.size());
        auto tmp_tb = tableblock_data.front();
        string rsp = tmp_tb.toStyledString();
        tableblock_data.pop_front();
        xdbg("grpc stream: tableblock_data after pop size %zu, json string size: %zu", tableblock_data.size(), rsp.size());
        lck.unlock();

        xrpc_reply reply;
        reply.set_body(rsp);
        if (!replys->Write(reply)) {
            xinfo("grpc stream: push err");
            std::lock_guard<std::mutex> tmp_lock(tableblock_mtx);
            tableblock_data.push_front(tmp_tb);
            rpc_client_num--;
            return Status(grpc::StatusCode::UNAVAILABLE, "Writing client failed.");
        }
    }
    rpc_client_num--;
    return Status(grpc::StatusCode::OK, "Stream finished successfully.");
}

// std::atomic<int> grpc_recv_num(0);
// uint64_t grpc_last_timestamp;

// class xrpc_async_serviceimpl final {
// public:
// xrpc_async_serviceimpl(const std::string & addr) :m_server_address(addr)
// {

// }
// ~xrpc_async_serviceimpl() {
//     server_->Shutdown();
//     // Always shutdown the completion queue after the server.
//     cq_->Shutdown();
// }

// There is no shutdown handling in this code.
// void Run() {
//     ServerBuilder builder;
//     // Listen on the given address without any authentication mechanism.
//     builder.AddListeningPort(m_server_address, grpc::InsecureServerCredentials());
//     // Register "service_" as the instance through which we'll communicate with
//     // clients. In this case it corresponds to an *asynchronous* service.
//     builder.RegisterService(&service_);
//     // Get hold of the completion queue used for the asynchronous communication
//     // with the gRPC runtime.
//     cq_ = builder.AddCompletionQueue();
//     // Finally assemble the server.
//     server_ = builder.BuildAndStart();
//     std::cout << "Server listening on " << m_server_address << std::endl;

//     // Proceed to the server's main loop.
//     // std::thread t1(&top::rpc::xrpc_async_serviceimpl::HandleRpcs, this);
//     // std::thread t2(&top::rpc::xrpc_async_serviceimpl::HandleRpcs, this);
//     // std::thread t3(&top::rpc::xrpc_async_serviceimpl::HandleRpcs, this);
//     // std::thread t4(&top::rpc::xrpc_async_serviceimpl::HandleRpcs, this);
//     // std::thread t5(&top::rpc::xrpc_async_serviceimpl::HandleRpcs, this);
//     // std::thread t6(&top::rpc::xrpc_async_serviceimpl::HandleRpcs, this);
//     // t1.join();
//     // t2.join();
//     // t3.join();
//     // t4.join();
//     // t5.join();
//     // t6.join();

// }

// private:
// // Class encompasing the state and logic needed to serve a request.
// class CallData {
// public:
//     // Take in the "service" instance (in this case representing an asynchronous
//     // server) and the completion queue "cq" used for asynchronous communication
//     // with the gRPC runtime.
//     CallData(top::xrpc_service::AsyncService* service, ServerCompletionQueue* cq)
//         : service_(service), cq_(cq), responder_(&ctx_), status_(CREATE) {
//     // Invoke the serving logic right away.
//     Proceed();
//     }

//     void Proceed() {
//     if (status_ == CREATE) {
//         // Make this instance progress to the PROCESS state.
//         status_ = PROCESS;

//         // As part of the initial CREATE state, we *request* that the system
//         // start processing SayHello requests. In this request, "this" acts are
//         // the tag uniquely identifying the request (so that different CallData
//         // instances can serve different requests concurrently), in this case
//         // the memory address of this CallData instance.
//         service_->Requestcall(&ctx_, &request_, &responder_, cq_, cq_,
//                                 this);
//     } else if (status_ == PROCESS) {
//         // Spawn a new CallData instance to serve new clients while we process
//         // the one for this CallData. The instance will deallocate itself as
//         // part of its FINISH state.
//         new CallData(service_, cq_);

//         // The actual processing.
// #if 0
//         // std::thread::id this_id = std::this_thread::get_id();
//         // std::cout << "thread " << this_id << endl;

//         string req = request_.body();
//         // cout << "request:" << endl;
//         // cout << req << endl;
//         auto handler (std::make_shared<xrpc_handler> (req));
//         handler->handle_request();
//         string rsp = handler->get_response();
//         reply_.set_body(rsp);

//         int num = grpc_recv_num++;
//         if(num % 1000 == 0)
//         {
//             uint64_t new_timestamp = xrpc_utility::get_timestamp_ms();
//             double rate = 1000000/(new_timestamp - grpc_last_timestamp);
//             grpc_last_timestamp = new_timestamp;
//             cout << "asyn server api rate: " << to_string(rate) <<  endl;
//         }
//         // cout << "response:" << endl;
//         // cout << rsp << endl  << endl;
// #endif
//         // And we are done! Let the gRPC runtime know we've finished, using the
//         // memory address of this instance as the uniquely identifying tag for
//         // the event.
//         status_ = FINISH;
//         responder_.Finish(reply_, Status::OK, this);
//     } else {
//         GPR_ASSERT(status_ == FINISH);
//         // Once in the FINISH state, deallocate ourselves (CallData).
//         delete this;
//     }
//     }

// private:
//     // The means of communication with the gRPC runtime for an asynchronous
//     // server.
//     top::xrpc_service::AsyncService* service_;
//     // The producer-consumer queue where for asynchronous server notifications.
//     ServerCompletionQueue* cq_;
//     // Context for the rpc, allowing to tweak aspects of it such as the use
//     // of compression, authentication, as well as to send metadata back to the
//     // client.
//     ServerContext ctx_;

//     // What we get from the client.
//     xrpc_request request_;
//     // What we send back to the client.
//     xrpc_reply reply_;

//     // The means to get back to the client.
//     ServerAsyncResponseWriter<xrpc_reply> responder_;

//     // Let's implement a tiny state machine with the following states.
//     enum CallStatus { CREATE, PROCESS, FINISH };
//     CallStatus status_;  // The current serving state.
// };

// // This can be run in multiple threads if needed.
// void HandleRpcs() {
//     // Spawn a new CallData instance to serve new clients.
//     new CallData(&service_, cq_.get());
//     void* tag;  // uniquely identifies a request.
//     bool ok;
//     while (true) {
//     // Block waiting to read the next event from the completion queue. The
//     // event is uniquely identified by its tag, which in this case is the
//     // memory address of a CallData instance.
//     // The return value of Next should always be checked. This return value
//     // tells us whether there is any kind of event or cq_ is shutting down.
//     GPR_ASSERT(cq_->Next(&tag, &ok));
//     GPR_ASSERT(ok);
//     static_cast<CallData*>(tag)->Proceed();

// #if 0
//     string *req_str = static_cast<string*>(tag);
//     cout << "req_str " << req_str << endl;

// #endif

//     }
// }

// std::unique_ptr<ServerCompletionQueue> cq_;
// top::xrpc_service::AsyncService service_;
// std::unique_ptr<Server> server_;
// std::string m_server_address;
// };

uint32_t xgrpc_service::sync_run(const std::shared_ptr<xrpc_handle_face_t> & handle) {
    xrpc_serviceimpl service;
    service.register_handle(handle);

    ServerBuilder builder;
    // reduce threads spawned by grpc
    builder.SetSyncServerOption(grpc::ServerBuilder::MAX_POLLERS, 1);
    // Listen on the given address without any authentication mechanism.
    builder.AddListeningPort(m_address, grpc::InsecureServerCredentials());
    // Register "service" as the instance through which we'll communicate with
    // clients. In this case it corresponds to an *synchronous* service.
    builder.RegisterService(&service);
    // Finally assemble the server.
    std::unique_ptr<Server> server(builder.BuildAndStart());
    if (server == nullptr) {
        xinfo("xrpc grpc server builder.BuildAndStart return nullptr");
        return -1;
    }
    std::cout << "xrpc grpc server listening on " << m_address << std::endl;
    xdbg("xrpc grpc server listening on %s", m_address.c_str());

    // Wait for the server to shutdown. Note that some other thread must be
    // responsible for shutting down the server for this call to ever return.
    server->Wait();
    return 0;
}

// uint32_t xgrpc_service::async_run()
// {
//     xrpc_async_serviceimpl server(m_address);
//     server.Run();
//     return 0;
// }

int32_t xgrpc_service::run() {
    xrpc_serviceimpl service;
    sync_run(m_handle);
    return 0;
}

int32_t xgrpc_service::start() {
    m_sync_thread = std::thread(&xgrpc_service::run, this);
    m_sync_thread.detach();
    return 0;
}
}  // namespace rpc
}  // namespace top
