#pragma once

#include <string>
#include <thread>
#include <deque>
#include <condition_variable>
#include "json/json.h"
#include "src/xrpc.grpc.pb.h"
#include <atomic>

using grpc::Status;
using grpc::ServerContext;
using grpc::ServerWriter;

namespace top { namespace rpc {

extern std::atomic_int rpc_client_num;
extern std::deque<xJson::Value> tableblock_data;
extern std::mutex tableblock_mtx;
extern std::condition_variable tableblock_cv;


class xrpc_handle_face_t
{
public:
    virtual bool handle(std::string request) = 0;
    virtual std::string get_response() = 0;
};

class xgrpc_service
{
public:
    xgrpc_service(const std::string & host, const uint16_t port):m_address(host+":"+std::to_string(port))
    {
    }
    virtual ~xgrpc_service()
    {
    }
    void register_handle(const std::shared_ptr<xrpc_handle_face_t>& handle){
        m_handle = handle;
    }
    int32_t start();
private:
    int32_t run();
    uint32_t sync_run(const std::shared_ptr<xrpc_handle_face_t>& handle);
    uint32_t async_run();

    std::thread m_sync_thread;
    std::string m_address;
    std::shared_ptr<xrpc_handle_face_t> m_handle;
};

class xrpc_serviceimpl final : public top::xrpc_service::Service {

public:
    void register_handle(const std::shared_ptr<xrpc_handle_face_t>& handle);

private:
    Status call(ServerContext *context, const xrpc_request *request,
                xrpc_reply *reply) override;

    Status table_stream(ServerContext *context, const xrpc_request *request,
                ServerWriter<xrpc_reply> *replys) override;

    std::shared_ptr<xrpc_handle_face_t> m_handle;
    mutable std::mutex m_call_mtx;
};

}}


