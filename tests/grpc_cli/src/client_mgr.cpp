#include "client_mgr.h"
#include <chrono>
#include <fstream>
#include <thread>

using namespace std;
using namespace top;

std::string get_cur_time() {
  auto tt = chrono::system_clock::to_time_t(std::chrono::system_clock::now());
  auto time_tm = localtime(&tt);
  char strTime[25] = {0};
  sprintf(strTime, "%d-%02d-%02d %02d:%02d:%02d", time_tm->tm_year + 1900,
          time_tm->tm_mon + 1, time_tm->tm_mday, time_tm->tm_hour,
          time_tm->tm_min, time_tm->tm_sec);
  std::string ft(strTime);
  return ft;
}

bool client_mgr_t::do_rpc_call(const xJson::Value &req_json,
                               xJson::Value &rsp_json) {
  string response;
  bool ret = m_client.do_call(req_json.toStyledString(), response);
  if (!ret) {
    std::cout << "grpc call fail ret: " << ret << std::endl;
    return ret;
  }
  xJson::Reader reader;
  if (!reader.parse(response, rsp_json)) {
    std::cout << "grpc res $" << response << "$ parse fail!" << std::endl;
    return false;
  }
  return true;
}

bool client_mgr_t::do_rpc_stream_call(const xJson::Value &req_json,
                                      xJson::Value &rsp_json) {
  string response;
  std::ofstream log_file("/home/test/grpc_cli1.log",
                         std::ios::out | std::ios::app);
  while (true) {
    bool ret = m_client.do_stream_call(req_json.toStyledString(), response);
    if (ret) {
      log_file << "Server table stream closed successfully!" << std::endl;
      return ret;
    }
    log_file << "cli timeout time: " << get_cur_time() << std::endl;
    std::this_thread::sleep_for(chrono::seconds(1));
    log_file << "Retrying table stream!" << std::endl;
  }

  return true;
}
