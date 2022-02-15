#pragma once

#include "xrpc_client.hpp"
#include "json/json.h"
#include <string>

std::string get_cur_time();

class client_mgr_t {
public:
  client_mgr_t(std::string &server_addr)
      : m_client(grpc::CreateChannel(server_addr,
                                     grpc::InsecureChannelCredentials())) {}
  bool do_rpc_call(const xJson::Value &req_json, xJson::Value &rsp_json);
  bool do_rpc_stream_call(const xJson::Value &req_json, xJson::Value &rsp_json);

private:
  top::xrpc_client m_client;
};
