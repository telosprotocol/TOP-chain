#include "client_mgr.h"
#include "xrpc.grpc.pb.h"
#include <ctime>
#include <fstream>
#include <grpcpp/grpcpp.h>
#include <iomanip>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

#include "json/json.h"

#include "xrpc_client.hpp"

using grpc::Channel;
using grpc::ClientContext;
using grpc::ClientReader;
using grpc::Status;
using top::xrpc_reply;
using top::xrpc_request;
using top::xrpc_service;

using namespace std;

namespace top {

bool xrpc_client::do_call(const string &req, string &rsp) {
  xrpc_request request;
  request.set_body(req);

  xrpc_reply reply;
  ClientContext context;

  Status status = m_stub->call(&context, request, &reply);

  if (status.ok()) {
    rsp = reply.body();
    return true;
  } else {
    std::cout << "RPC failed " << status.error_code() << ": "
              << status.error_message() << std::endl;
    return false;
  }
}

bool xrpc_client::do_stream_call(const string &req, string &rsp) {
  xrpc_request request;
  request.set_body(req);

  ClientContext context;
  auto deadline = std::chrono::system_clock::now() + std::chrono::seconds(600);
  context.set_deadline(deadline);
  auto ct = get_cur_time();

  std::ofstream log_file("/home/test/grpc_cli1.log",
                         std::ios::out | std::ios::app);
  auto reader = m_stub->table_stream(&context, request);
  log_file << "cli start time: " << ct << std::endl;

  static int i = 0;
  xrpc_reply reply;
  while (reader->Read(&reply)) {
    std::cout << i++ << ": xrpc_client.cpp wen " << reply.body() << std::endl;
    xJson::Value rsp_json;
    xJson::Reader reader;

    log_file << "cli receive time: " << get_cur_time() << std::endl;
    if (!reader.parse(reply.body(), rsp_json)) {
      log_file << "table stream parse error!" << std::endl;
      return false;
    }
    log_file << "table stream client receive: " << i
             << ", server send: " << rsp_json["result"].asUInt64() << std::endl;
    log_file << "owner: " << rsp_json["value"]["owner"].asString() << std::endl;
    log_file << "height: " << rsp_json["value"]["height"].asString()
             << std::endl;
  }

  Status status = reader->Finish();
  if (status.ok()) {
    log_file << "Server grpc streaming Ok: " << status.error_message()
             << std::endl;
    return true;
  } else {
    log_file << "Server grpc streaming Fail: " << status.error_message()
             << std::endl;
    return false;
  }

  // if (status.ok())
  // {
  //     rsp = reply.body();
  //     return true;
  // }
  // else
  // {
  //     std::cout << "RPC failed "<< status.error_code() << ": " <<
  //     status.error_message()
  //                 << std::endl;
  //     return false;
  // }
}

} // namespace top
