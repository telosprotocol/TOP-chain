#pragma once

#include <cstdlib>
#include <grpcpp/grpcpp.h>
#include <iostream>
#include <memory>
#include <string>

#include "xrpc.grpc.pb.h"

using grpc::Channel;
using top::xrpc_service;

using namespace std;

namespace top {

class xrpc_client {
public:
  xrpc_client(std::shared_ptr<Channel> channel)
      : m_stub(xrpc_service::NewStub(channel)) {}
  bool do_call(const string &req, string &rsp);
  bool do_stream_call(const string &req, string &rsp);

private:
  std::unique_ptr<xrpc_service::Stub> m_stub;
};

} // namespace top
