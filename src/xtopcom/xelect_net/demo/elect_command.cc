//
//  top_commands.cc
//  user commands
//
//  Created by Charlie Xie on 01/15/2019.
//  Copyright (c) 2017-2019 Telos Foundation & contributors
//

#include "xelect_net/demo/elect_command.h"

#include "xbase/xutl.h"
#include "xbasic/xmemory.hpp"
#include "xelect_net/include/elect_uitils.h"
#include "xgrpcservice/xgrpc_service.h"
#include "xkad/routing_table/callback_manager.h"
#include "xkad/routing_table/local_node_info.h"
#include "xpbase/base/check_cast.h"
#include "xpbase/base/kad_key/kadmlia_key.h"
#include "xpbase/base/line_parser.h"
#include "xpbase/base/top_log.h"
#include "xpbase/base/top_string_util.h"
#include "xpbase/base/top_utils.h"
#include "xwrouter/multi_routing/multi_routing.h"
#include "xwrouter/root/root_routing.h"

#include <string.h>

#include <algorithm>
#include <chrono>
#include <fstream>
#include <iostream>
#include <list>
#include <string>
#include <utility>

using namespace top::kadmlia;  // NOLINT

namespace top {

ElectCommands::ElectCommands() {
}

ElectCommands::~ElectCommands() {
    destroy_ = true;
}

bool ElectCommands::Init(bool first_node, bool show_cmd) {
    first_node_ = first_node;
    show_cmd_ = show_cmd;
    AddBaseCommands();
    AddExtraCommands();
    return true;
}

void ElectCommands::set_netcard(elect::EcNetcardPtr ec_netcard) {
    ec_netcard_ = ec_netcard;
}
// #if 0
using query_method_handler = std::function<void(void)>;

#define REGISTER_NET_CMD_METHOD(func_name)                                                                                                                                         \
    m_query_method_map.emplace(std::pair<std::string, query_method_handler>{std::string{#func_name}, std::bind(&net_cmd_handle::func_name, this)})

class net_cmd_handle : public top::rpc::xrpc_handle_face_t {
public:
    net_cmd_handle(top::ElectCommands * elect_cmd) : m_elect_cmd{elect_cmd} {
        REGISTER_NET_CMD_METHOD(p2ptest);
    }
    void p2ptest() {
        std::cout << "p2p test" << std::endl;
        std::string test_cmd = m_js_req["test_cmd"].asString();
        uint64_t index = m_js_req["index"].asUInt64();
        std::cout << "p2p test cmd:" << test_cmd << " index: " << index << std::endl;

        m_js_rsp["value"] = m_elect_cmd->ProcessRpcCommand(test_cmd, index);
    }
    bool handle(std::string request) override {
        std::cout << "handler request" << request << std::endl;
        m_js_req.clear();
        m_js_rsp.clear();

        xJson::Reader reader;
        m_result = "ok";

        if (!reader.parse(request, m_js_req)) {
            m_result = "json parse error";
            return true;
        }
        std::string action = m_js_req["action"].asString();
        auto iter = m_query_method_map.find(action);
        if (iter != m_query_method_map.end()) {
            iter->second();
        } else {
            m_result = "do not have this action";
            return false;
        }

        return true;
    }
    std::string get_response() override {
        m_js_rsp["result"] = m_result;
        std::string rsp;
        try {
            rsp = m_js_rsp.toStyledString();
        } catch (...) {
            xdbg("xJson toStyledString error");
        }
        return rsp;
    }

    top::ElectCommands * m_elect_cmd;

    xJson::Value m_js_req;
    xJson::Value m_js_rsp;
    std::string m_result;

    std::unordered_map<std::string, query_method_handler> m_query_method_map;
};

void ElectCommands::get_rpc_instruction(uint16_t delta) {
    static std::unique_ptr<top::rpc::xgrpc_service> grpc_srv = top::make_unique<top::rpc::xgrpc_service>("0.0.0.0", (uint16_t)(10125 + delta));
    static std::shared_ptr<top::rpc::xrpc_handle_face_t> handle = std::make_shared<net_cmd_handle>(this);

    grpc_srv->register_handle(handle);
    grpc_srv->start();
}
// #endif
void ElectCommands::Run(uint16_t delta) {
    PrintUsage();

    get_rpc_instruction(delta);

    while (!destroy_) {
        SleepUs(20000000);
        continue;
    }
    std::cout << "command exited." << std::endl;
}

void ElectCommands::PrintUsage() {
    // std::cout << "\thelp Print options.\n";
    // std::cout << "\tjoin Normal Join.\n";
    // std::cout << "\tprt Print Local Routing Table.\n";
    // std::cout << "\trrt <dest_index> Request Routing Table from peer node with the specified"
    //           << " identity-index.\n";
    // std::cout << "\tsave save all nodes but nodes in routing table.\n";
    // std::cout << "\trt relay test, this node to other hop count.\n";
    // std::cout << "\tcsb super broadcast test, this node to other hop count.\n";
    // std::cout << "\tart all nodes relay test, this node to other hop count.\n";
    // std::cout << "\tgroup get groups by target id.\n";
    // std::cout << "\tsync sync all nodes from bootstrap node.\n";
    // std::cout << "\tgets get service nodes of service_type.\n";
    // std::cout << "\tsets set service type for gets.\n";
}

xJson::Value ElectCommands::ProcessRpcCommand(const std::string & cmdline, uint64_t cmd_index) {
    if (cmdline.empty()) {
        return {};
    }
    std::string cmd;
    Arguments args;
    try {
        top::base::LineParser line_split(cmdline.c_str(), ' ', cmdline.size());
        cmd = "";
        for (uint32_t i = 0; i < line_split.Count(); ++i) {
            if (strlen(line_split[i]) == 0) {
                continue;
            }

            if (cmd == "")
                cmd = line_split[i];
            else
                args.push_back(line_split[i]);
        }
    } catch (const std::exception & e) {
        TOP_WARN("Error processing command: %s", e.what());
    }

    std::unique_lock<std::mutex> lock(rpc_cmd_map_mutex);
    auto it = rpc_cmd_map.find(cmd);
    std::cout << cmdline << "   :  " << cmd << std::endl;
    if (it != rpc_cmd_map.end()) {
        return (it->second)(args);  // call command procedure
    }
    return {};
}

void ElectCommands::AddBaseCommands() {
    auto get_net_type = [this](uint32_t nid, uint32_t zid, uint32_t cid, uint32_t gid) -> std::string {
        if (nid == 16777215)
            return "root";
        if (zid == 1 && cid == 0 && gid == 0)
            return "rec";
        if (zid == 2 && cid == 0 && gid == 0)
            return "zec";
        if (zid == 15 && cid == 1 && gid == 1)
            return "edge";
        if (zid == 14 && cid == 1 && gid == 1)
            return "arc";
        if (zid == 0 && cid == 1 && gid < 64)
            return "adv";
        if (zid == 0 && cid == 1 && gid >= 64)
            return "val";
        return "false net type";
    };
    AddRpcCommand("prt", [this, &get_net_type](Arguments const & args) {
        xJson::Value ret;

        auto root_routing_table = wrouter::MultiRouting::Instance()->GetRootRoutingTable();
        if (!root_routing_table) {
            ret["root_rt"] = "null";
        } else {
            xJson::Value root_rt;
            root_rt["local_nodeid"] = root_routing_table->get_local_node_info()->kad_key();
            root_rt["service_type"] = root_routing_table->get_local_node_info()->service_type().info();
            root_rt["neighbours"] = static_cast<xJson::UInt64>(root_routing_table->nodes_size());
            ret["root_rt"] = root_rt;
        }
        std::vector<base::ServiceType> vec_type;
        wrouter::MultiRouting::Instance()->GetAllRegisterType(vec_type);
        for (const auto & type : vec_type) {
            xJson::Value one_ret;

            auto routing_table = wrouter::MultiRouting::Instance()->GetElectRoutingTable(type);
            if (!routing_table) {
                std::cout << "warning: " << type.info() << " routing table invalid" << std::endl;
                continue;
            }
            one_ret["local_nodeid"] = routing_table->get_local_node_info()->kad_key();
            one_ret["service_type"] = routing_table->get_local_node_info()->service_type().info();
            one_ret["neighbours"] = static_cast<xJson::UInt64>(routing_table->nodes_size());

            std::string net_type = get_net_type(routing_table->get_local_node_info()->kadmlia_key()->xnetwork_id(),
                                                static_cast<uint32_t>(routing_table->get_local_node_info()->kadmlia_key()->zone_id()),
                                                static_cast<uint32_t>(routing_table->get_local_node_info()->kadmlia_key()->cluster_id()),
                                                static_cast<uint32_t>(routing_table->get_local_node_info()->kadmlia_key()->group_id()));
            ret[net_type] = one_ret;
        }

        return ret;
    });

    AddRpcCommand("broadcast_all", [this](Arguments const & args) {
        // broadcast_all msg_cnt msg_size
        uint32_t msg_cnt = 1;
        if (args.size() >= 1) {
            msg_cnt = check_cast<uint32_t, const char *>(args[0].c_str());
        }
        uint32_t msg_size = 100;
        if (args.size() >= 2) {
            msg_size = check_cast<uint32_t, const char *>(args[1].c_str());
        }

        uint32_t gossip_type = 1;
        if (args.size() >= 3) {
            gossip_type = check_cast<uint32_t, const char *>(args[2].c_str());
        }
        uint32_t backup = 1;
        if (args.size() >= 4) {
            backup = check_cast<uint32_t, const char *>(args[3].c_str());
        }
        uint32_t neighbors_num = 3;
        if (args.size() >= 5) {
            neighbors_num = check_cast<uint32_t, const char *>(args[4].c_str());
        }
        uint32_t stop_times = 3;
        if (args.size() >= 6) {
            stop_times = check_cast<uint32_t, const char *>(args[5].c_str());
        }
        uint32_t max_hop_num = 10;
        if (args.size() >= 7) {
            max_hop_num = check_cast<uint32_t, const char *>(args[6].c_str());
        }
        uint32_t evil_rate = 0;
        if (args.size() >= 8) {
            evil_rate = check_cast<uint32_t, const char *>(args[7].c_str());
        }
        uint32_t layer_switch_hop_num = 2;
        if (args.size() >= 9) {
            layer_switch_hop_num = check_cast<uint32_t, const char *>(args[8].c_str());
        }
        uint32_t left_overlap = 0;
        if (args.size() >= 10) {
            left_overlap = check_cast<uint32_t, const char *>(args[9].c_str());
        }
        uint32_t right_overlap = 0;
        if (args.size() >= 11) {
            right_overlap = check_cast<uint32_t, const char *>(args[10].c_str());
        }
        auto ret = elect_perf_.rpc_broadcast_all(
            msg_cnt, msg_size, gossip_type, backup, neighbors_num, stop_times, max_hop_num, evil_rate, layer_switch_hop_num, left_overlap, right_overlap);
        return ret;
    });

    AddRpcCommand("broadcast_all_new", [this](Arguments const & args) {
        // broadcast_all msg_cnt msg_size
        uint32_t msg_cnt = 1;
        if (args.size() >= 1) {
            msg_cnt = check_cast<uint32_t, const char *>(args[0].c_str());
        }
        uint32_t msg_size = 100;
        if (args.size() >= 2) {
            msg_size = check_cast<uint32_t, const char *>(args[1].c_str());
        }

        uint32_t gossip_type = 8;
        if (args.size() >= 3) {
            gossip_type = check_cast<uint32_t, const char *>(args[2].c_str());
        }
        uint32_t backup = 1;
        if (args.size() >= 4) {
            backup = check_cast<uint32_t, const char *>(args[3].c_str());
        }
        uint32_t neighbors_num = 3;
        if (args.size() >= 5) {
            neighbors_num = check_cast<uint32_t, const char *>(args[4].c_str());
        }
        uint32_t stop_times = 3;
        if (args.size() >= 6) {
            stop_times = check_cast<uint32_t, const char *>(args[5].c_str());
        }
        uint32_t max_hop_num = 10;
        if (args.size() >= 7) {
            max_hop_num = check_cast<uint32_t, const char *>(args[6].c_str());
        }
        uint32_t evil_rate = 0;
        if (args.size() >= 8) {
            evil_rate = check_cast<uint32_t, const char *>(args[7].c_str());
        }
        uint32_t layer_switch_hop_num = 3;
        if (args.size() >= 9) {
            layer_switch_hop_num = check_cast<uint32_t, const char *>(args[8].c_str());
        }
        uint32_t left_overlap = 0;
        if (args.size() >= 10) {
            left_overlap = check_cast<uint32_t, const char *>(args[9].c_str());
        }
        uint32_t right_overlap = 0;
        if (args.size() >= 11) {
            right_overlap = check_cast<uint32_t, const char *>(args[10].c_str());
        }
        auto ret = elect_perf_.rpc_broadcast_all_new(
            msg_cnt, msg_size, gossip_type, backup, neighbors_num, stop_times, max_hop_num, evil_rate, layer_switch_hop_num, left_overlap, right_overlap);
        return ret;
    });

    AddRpcCommand("broadcast_to_cluster", [this](Arguments const & args) {
        if (args.size() < 2) {
            std::cout << "invalid params, usage: crtnode [src_node_id] [des_node_id]" << std::endl;
            return xJson::Value{};
        }

        // std::string src_node_id = HexDecode(nodeid)

        std::string src_node_id = HexDecode(args[0]);
        std::string des_node_id = HexDecode(args[1]);

        uint32_t test_num = 1;
        if (args.size() >= 3) {
            test_num = check_cast<uint32_t, const char *>(args[2].c_str());
        }
        uint32_t test_len = 100;
        if (args.size() >= 4) {
            test_len = check_cast<uint32_t, const char *>(args[3].c_str());
        }
        uint32_t gossip_type = 3;
        if (args.size() >= 5) {
            gossip_type = check_cast<uint32_t, const char *>(args[4].c_str());
        }
        uint32_t backup = 1;
        if (args.size() >= 6) {
            backup = check_cast<uint32_t, const char *>(args[5].c_str());
        }
        uint32_t neighbors_num = 3;
        if (args.size() >= 7) {
            neighbors_num = check_cast<uint32_t, const char *>(args[6].c_str());
        }
        uint32_t stop_times = 99;
        if (args.size() >= 8) {
            stop_times = check_cast<uint32_t, const char *>(args[7].c_str());
        }
        uint32_t max_hop_num = 15;
        if (args.size() >= 9) {
            max_hop_num = check_cast<uint32_t, const char *>(args[8].c_str());
        }
        uint32_t evil_rate = 0;
        if (args.size() >= 10) {
            evil_rate = check_cast<uint32_t, const char *>(args[9].c_str());
        }
        uint32_t layer_switch_hop_num = 0;
        if (args.size() >= 11) {
            layer_switch_hop_num = check_cast<uint32_t, const char *>(args[10].c_str());
        }
        uint32_t left_overlap = 0;
        if (args.size() >= 12) {
            left_overlap = check_cast<uint32_t, const char *>(args[11].c_str());
        }
        uint32_t right_overlap = 0;
        if (args.size() >= 13) {
            right_overlap = check_cast<uint32_t, const char *>(args[12].c_str());
        }

        auto ret = elect_perf_.rpc_broadcast_to_cluster(src_node_id,
                                                        des_node_id,
                                                        test_num,
                                                        test_len,
                                                        gossip_type,
                                                        backup,
                                                        neighbors_num,
                                                        stop_times,
                                                        max_hop_num,
                                                        evil_rate,
                                                        layer_switch_hop_num,
                                                        left_overlap,
                                                        right_overlap);
        return ret;
    });

}  //  namespace top

void ElectCommands::AddRpcCommand(const std::string & cmd_name, RpcCommandProc cmd_proc) {
    assert(cmd_proc);
    std::unique_lock<std::mutex> lock(rpc_cmd_map_mutex);

    auto it = rpc_cmd_map.find(cmd_name);
    if (it != rpc_cmd_map.end()) {
        return;
    }
    rpc_cmd_map[cmd_name] = cmd_proc;
}
}