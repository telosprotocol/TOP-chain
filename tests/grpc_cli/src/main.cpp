#include "client_mgr.h"
#include "xrpc_client.hpp"
#include "json/json.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

using namespace std;
using namespace top;

std::string owner = "";
std::string type = "";
uint64_t height = 0;
std::string g_hash;
std::string prop = "";
std::string action = "getBlock";
std::string addr = "";

void getBlock(client_mgr_t &g_client) {
  xJson::Value req_json;
  xJson::Value rsp_json;
  req_json["action"] = action;
  req_json["type"] = type;
  // req_json["owner"] = "T-0-1AgrXKKpbrr6FCaYWtKpbA567ZHHizMRRy";
  // req_json["owner"] = "T-0-137XhQJzUuptVbnT6vzyxiuraZcsTTkEBA";
  // req_json["block_type"] = 2;
  req_json["account_addr"] = owner;
  req_json["hash"] = g_hash;
  req_json["height"] = static_cast<unsigned long long>(height);
  req_json["prop"] = prop;
  req_json["version"] = "2.0";
  auto ret = g_client.do_rpc_call(req_json, rsp_json);
  cout << "getBlock ret " << ret << endl;
  cout << "req_json" << req_json.toStyledString() << endl;
  cout << "rsp_json" << rsp_json.toStyledString() << endl;
}

void getProperty(client_mgr_t &g_client) {
  xJson::Value req_json;
  xJson::Value rsp_json;
  req_json["action"] = action;
  req_json["type"] = type;
  req_json["account_addr"] = owner;
  req_json["height"] = static_cast<unsigned long long>(height);
  req_json["prop"] = prop;
  auto ret = g_client.do_rpc_call(req_json, rsp_json);
  cout << "getProperty ret " << ret << endl;
  cout << "req_json" << req_json.toStyledString() << endl;
  cout << "rsp_json" << rsp_json.toStyledString() << endl;
}

void getLatestFullBlock(client_mgr_t &g_client) {
  xJson::Value req_json;
  xJson::Value rsp_json;
  req_json["action"] = action;
  req_json["account_addr"] = owner;
  auto ret = g_client.do_rpc_call(req_json, rsp_json);
  cout << "getLatestFullBlock ret " << ret << endl;
  cout << "req_json" << req_json.toStyledString() << endl;
  cout << "rsp_json" << rsp_json.toStyledString() << endl;
}

void get_browser(client_mgr_t &g_client) {
  xJson::Value req_json;
  req_json["version"] = "2.0";
  xJson::Value rsp_json;
  bool ret;
  if (action == "table_stream") {
    ret = g_client.do_rpc_stream_call(req_json, rsp_json);
    return;
  } else {
    ret = g_client.do_rpc_call(req_json, rsp_json);
  }
  cout << "get_browser ret " << ret << endl;
  cout << "req_json" << req_json.toStyledString() << endl;
  cout << "rsp_json" << rsp_json.toStyledString() << endl;
}

void getTransaction(client_mgr_t &g_client) {
  xJson::Value req_json;
  xJson::Value rsp_json;
  req_json["action"] = action;
  req_json["addr"] = addr;
  req_json["tx_hash"] = g_hash;
  req_json["version"] = "2.0";
  auto ret = g_client.do_rpc_call(req_json, rsp_json);
  cout << "get_transaction ret " << ret << endl;
  cout << "req_json" << req_json.toStyledString() << endl;
  cout << "rsp_json" << rsp_json.toStyledString() << endl;
}

void getAccount(client_mgr_t &g_client) {
  xJson::Value req_json;
  xJson::Value rsp_json;
  req_json["action"] = action;
  req_json["account_addr"] = addr;
  auto ret = g_client.do_rpc_call(req_json, rsp_json);
  cout << "getAccount ret " << ret << endl;
  cout << "req_json" << req_json.toStyledString() << endl;
  cout << "rsp_json" << rsp_json.toStyledString() << endl;
}

void getGeneralInfos(client_mgr_t &g_client) {
  xJson::Value req_json;
  xJson::Value rsp_json;
  req_json["action"] = action;
  auto ret = g_client.do_rpc_call(req_json, rsp_json);
  cout << "getGeneralInfos ret " << ret << endl;
  cout << "req_json" << req_json.toStyledString() << endl;
  cout << "rsp_json" << rsp_json.toStyledString() << endl;
}

void getTimerInfo(client_mgr_t &g_client) {
  xJson::Value req_json;
  xJson::Value rsp_json;
  req_json["action"] = action;
  auto ret = g_client.do_rpc_call(req_json, rsp_json);
  cout << "getTimerInfo ret " << ret << endl;
  cout << "req_json" << req_json.toStyledString() << endl;
  cout << "rsp_json" << rsp_json.toStyledString() << endl;
}

void getRecs(client_mgr_t &g_client) {
  xJson::Value req_json;
  xJson::Value rsp_json;
  req_json["action"] = action;
  auto ret = g_client.do_rpc_call(req_json, rsp_json);
  cout << "getRecs ret " << ret << endl;
  cout << "req_json" << req_json.toStyledString() << endl;
  cout << "rsp_json" << rsp_json.toStyledString() << endl;
}

void getZecs(client_mgr_t &g_client) {
  xJson::Value req_json;
  xJson::Value rsp_json;
  req_json["action"] = action;
  auto ret = g_client.do_rpc_call(req_json, rsp_json);
  cout << "getZecs ret " << ret << endl;
  cout << "req_json" << req_json.toStyledString() << endl;
  cout << "rsp_json" << rsp_json.toStyledString() << endl;
}

void getEdges(client_mgr_t &g_client) {
  xJson::Value req_json;
  xJson::Value rsp_json;
  req_json["action"] = action;
  auto ret = g_client.do_rpc_call(req_json, rsp_json);
  cout << "getEdges ret " << ret << endl;
  cout << "req_json" << req_json.toStyledString() << endl;
  cout << "rsp_json" << rsp_json.toStyledString() << endl;
}

void getArcs(client_mgr_t &g_client) {
  xJson::Value req_json;
  xJson::Value rsp_json;
  req_json["action"] = action;
  auto ret = g_client.do_rpc_call(req_json, rsp_json);
  cout << "getArcs ret " << ret << endl;
  cout << "req_json" << req_json.toStyledString() << endl;
  cout << "rsp_json" << rsp_json.toStyledString() << endl;
}

void getConsensus(client_mgr_t &g_client) {
  xJson::Value req_json;
  xJson::Value rsp_json;
  req_json["action"] = action;
  auto ret = g_client.do_rpc_call(req_json, rsp_json);
  cout << "getEdges ret " << ret << endl;
  cout << "req_json" << req_json.toStyledString() << endl;
  cout << "rsp_json" << rsp_json.toStyledString() << endl;
}

void getStandbys(client_mgr_t &g_client) {
  xJson::Value req_json;
  xJson::Value rsp_json;
  req_json["action"] = action;
  auto ret = g_client.do_rpc_call(req_json, rsp_json);
  cout << "getStandbys ret " << ret << endl;
  cout << "req_json" << req_json.toStyledString() << endl;
  cout << "rsp_json" << rsp_json.toStyledString() << endl;
}

void getExchangeNodes(client_mgr_t &g_client) {
  xJson::Value req_json;
  xJson::Value rsp_json;
  req_json["action"] = action;
  auto ret = g_client.do_rpc_call(req_json, rsp_json);
  cout << "getExchangeNodes ret " << ret << endl;
  cout << "req_json" << req_json.toStyledString() << endl;
  cout << "rsp_json" << rsp_json.toStyledString() << endl;
}

void getFullNodes(client_mgr_t &g_client) {
  xJson::Value req_json;
  xJson::Value rsp_json;
  req_json["action"] = action;
  auto ret = g_client.do_rpc_call(req_json, rsp_json);
  cout << "getFullNodes ret " << ret << endl;
  cout << "req_json" << req_json.toStyledString() << endl;
  cout << "rsp_json" << rsp_json.toStyledString() << endl;
}

void getFullStateNodes(client_mgr_t &g_client) {
  xJson::Value req_json;
  xJson::Value rsp_json;
  req_json["action"] = action;
  auto ret = g_client.do_rpc_call(req_json, rsp_json);
  cout << "getFullStateNodes ret " << ret << endl;
  cout << "req_json" << req_json.toStyledString() << endl;
  cout << "rsp_json" << rsp_json.toStyledString() << endl;
}

void getOthers(client_mgr_t &g_client) {
  xJson::Value req_json;
  xJson::Value rsp_json;
  req_json["action"] = action;
  auto ret = g_client.do_rpc_call(req_json, rsp_json);
  cout << "getOthers" << " " << action << " ret " << ret << endl;
  cout << "req_json " << req_json.toStyledString() << endl;
  cout << "rsp_json " << rsp_json.toStyledString() << endl;
}

void queryNodeInfo(client_mgr_t &g_client) {
  xJson::Value req_json;
  xJson::Value rsp_json;
  req_json["action"] = action;
  auto ret = g_client.do_rpc_call(req_json, rsp_json);
  cout << "queryNodeInfo ret " << ret << endl;
  cout << "req_json" << req_json.toStyledString() << endl;
  cout << "rsp_json" << rsp_json.toStyledString() << endl;
}

void queryNodeReward(client_mgr_t &g_client) {
  xJson::Value req_json;
  xJson::Value rsp_json;
  req_json["action"] = action;
  req_json["node_account_addr"] = addr;
  auto ret = g_client.do_rpc_call(req_json, rsp_json);
  cout << "queryNodeReward ret " << ret << endl;
  cout << "req_json" << req_json.toStyledString() << endl;
  cout << "rsp_json" << rsp_json.toStyledString() << endl;
}

void getPropertyByHeight(client_mgr_t &g_client) {
  xJson::Value req_json;
  xJson::Value rsp_json;
  req_json["action"] = action;
  req_json["account_addr"] = owner;
  req_json["height"] = static_cast<unsigned long long>(height);;
  req_json["prop"] = prop;
  auto ret = g_client.do_rpc_call(req_json, rsp_json);
  cout << "getPropertyByHeight ret " << ret << endl;
  cout << "req_json" << req_json.toStyledString() << endl;
  cout << "rsp_json" << rsp_json.toStyledString() << endl;
}

void getSyncNeighbors(client_mgr_t &g_client) {
  xJson::Value req_json;
  xJson::Value rsp_json;
  req_json["action"] = action;
  auto ret = g_client.do_rpc_call(req_json, rsp_json);
  cout << "getSyncNeighbors ret " << ret << endl;
  cout << "req_json" << req_json.toStyledString() << endl;
  cout << "rsp_json" << rsp_json.toStyledString() << endl;
}

void getCGP(client_mgr_t &g_client) {
  xJson::Value req_json;
  xJson::Value rsp_json;
  req_json["action"] = action;
  auto ret = g_client.do_rpc_call(req_json, rsp_json);
  cout << "getCGP ret " << ret << endl;
  cout << "req_json" << req_json.toStyledString() << endl;
  cout << "rsp_json" << rsp_json.toStyledString() << endl;
}

int main(int argc, char **argv) {
  assert(argc >= 3);
  std::string server_addr = std::string(argv[1]);
  client_mgr_t grpc_client = client_mgr_t(server_addr);

  action = argv[2];

  if ("getBlock" == action || "getblock" == action) {
    owner = argv[3];
    type = argv[4];
    if (type == "height") {
      height = std::stoul(argv[5]);
    } else if (type == "hash") {
      g_hash = argv[5];
    } else if (type == "prop") {
      prop = argv[5];
      height = std::stoul(argv[6]);
    }
    getBlock(grpc_client);
  } else if ("getProperty" == action) {
    owner = argv[3];
    type = argv[4];
    if (type == "height") {
      height = std::stoul(argv[5]);
      prop = argv[6];
    } else if (type == "last") {
      prop = argv[5];
    }
    getProperty(grpc_client);
  } else if ("table_stream" == action) {
    get_browser(grpc_client);
  } else if ("getTransaction" == action || "gettx" == action) {
    g_hash = argv[3];
    getTransaction(grpc_client);
  } else if ("getAccount" == action) {
    addr = argv[3];
    getAccount(grpc_client);
  } else if ("getGeneralInfos" == action) {
    getGeneralInfos(grpc_client);
  } else if ("getTimerInfo" == action) {
    getTimerInfo(grpc_client);
  } else if ("getRecs" == action) {
    getRecs(grpc_client);
  } else if ("getZecs" == action) {
    getZecs(grpc_client);
  } else if ("getEdges" == action) {
    getEdges(grpc_client);
  } else if ("getArcs" == action) {
    getArcs(grpc_client);
  } else if ("getConsensus" == action) {
    getConsensus(grpc_client);
  } else if ("getStandbys" == action) {
    getStandbys(grpc_client);
  } else if ("getExchangeNodes" == action) {
    getExchangeNodes(grpc_client);
  } else if ("getFullNodes" == action) {
    getFullNodes(grpc_client);
  } else if ("getFullStateNodes" == action) {
    getFullStateNodes(grpc_client);
  } else if ("getCGP" == action) {
    getCGP(grpc_client);
  } else if ("queryNodeInfo" == action) {
    queryNodeInfo(grpc_client);
  } else if ("queryNodeReward" == action) {
    if (argc == 4) {
      addr = argv[3];
    }
    queryNodeReward(grpc_client);
  } else if ("getLatestBlock" == action) {
    owner = argv[3];
    type = "last";
    getBlock(grpc_client);
  } else if ("getLatestFullBlock" == action) {
    owner = argv[3];
    getLatestFullBlock(grpc_client);
  } else if ("getBlockByHeight" == action) {
    owner = argv[3];
    height = std::stoul(argv[4]);
    getBlock(grpc_client);
  } else if ("getPropertyByHeight" == action) {
    owner = argv[3];
    height = std::stoul(argv[4]);
    prop = argv[5];
    getPropertyByHeight(grpc_client);
  } else if ("getSyncNs" == action) {
    action = "getSyncNeighbors";
    getSyncNeighbors(grpc_client);
  } else {
    getOthers(grpc_client);
  }

  return 0;
}
