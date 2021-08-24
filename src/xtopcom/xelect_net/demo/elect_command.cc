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
#include "xwrouter/root/root_routing.h"
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
#if 0
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

void ElectCommands::get_rpc_instruction() {
    static std::unique_ptr<top::rpc::xgrpc_service> grpc_srv = top::make_unique<top::rpc::xgrpc_service>("0.0.0.0", (uint16_t)10125);
    static std::shared_ptr<top::rpc::xrpc_handle_face_t> handle = std::make_shared<net_cmd_handle>(this);

    grpc_srv->register_handle(handle);
    grpc_srv->start();
}
#endif
void ElectCommands::Run() {
    PrintUsage();

    // get_rpc_instruction();

    while (!destroy_) {
        if (!show_cmd_) {
            SleepUs(20000000);
            continue;
        }

        std::cout << std::endl << std::endl << "Enter command > ";
        std::string cmdline;
        std::getline(std::cin, cmdline);
        { ProcessCommand(cmdline); }
    }
    std::cout << "command exited." << std::endl;
}

void ElectCommands::PrintUsage() {
    std::cout << "\thelp Print options.\n";
    std::cout << "\tjoin Normal Join.\n";
    std::cout << "\tprt Print Local Routing Table.\n";
    std::cout << "\trrt <dest_index> Request Routing Table from peer node with the specified"
              << " identity-index.\n";
    std::cout << "\tsave save all nodes but nodes in routing table.\n";
    std::cout << "\trt relay test, this node to other hop count.\n";
    std::cout << "\tcsb super broadcast test, this node to other hop count.\n";
    std::cout << "\tart all nodes relay test, this node to other hop count.\n";
    std::cout << "\tgroup get groups by target id.\n";
    std::cout << "\tsync sync all nodes from bootstrap node.\n";
    std::cout << "\tgets get service nodes of service_type.\n";
    std::cout << "\tsets set service type for gets.\n";
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

void ElectCommands::ProcessCommand(const std::string & cmdline) {
    if (cmdline.empty()) {
        return;
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

    std::unique_lock<std::mutex> lock(map_commands_mutex_);
    auto it = map_commands_.find(cmd);
    if (it == map_commands_.end()) {
        std::cout << "Invalid command : " << cmd << std::endl;
        PrintUsage();
    } else {
        (it->second)(args);  // call command procedure
    }
}

// int ElectCommands::GetEcXip(const Arguments & args, base::XipParser & xip) try {
//     if (args.size() == 0) {
//         xip.set_xnetwork_id(kChainRecNet);
//         xip.set_xip_type(elect::kElectionCommittee);
//     } else if (args.size() == 1) {
//         xip.set_xnetwork_id(check_cast<uint32_t, const char *>(args[0].c_str()));
//         xip.set_xip_type(elect::kElectionCommittee);
//     } else if (args.size() == 2) {
//         xip.set_xnetwork_id(check_cast<uint32_t, const char *>(args[0].c_str()));
//         xip.set_xip_type(check_cast<uint8_t, const char *>(args[1].c_str()));
//     } else if (args.size() == 3) {
//         xip.set_xnetwork_id(check_cast<uint32_t, const char *>(args[0].c_str()));
//         xip.set_zone_id(check_cast<uint8_t, const char *>(args[1].c_str()));
//         xip.set_xip_type(check_cast<uint8_t, const char *>(args[2].c_str()));
//     } else if (args.size() == 4) {
//         xip.set_xnetwork_id(check_cast<uint32_t, const char *>(args[0].c_str()));
//         xip.set_zone_id(check_cast<uint8_t, const char *>(args[1].c_str()));
//         xip.set_cluster_id(check_cast<uint8_t, const char *>(args[2].c_str()));
//         xip.set_xip_type(check_cast<uint8_t, const char *>(args[3].c_str()));
//     } else if (args.size() == 5) {
//         xip.set_xnetwork_id(check_cast<uint32_t, const char *>(args[0].c_str()));
//         xip.set_zone_id(check_cast<uint8_t, const char *>(args[1].c_str()));
//         xip.set_cluster_id(check_cast<uint8_t, const char *>(args[2].c_str()));
//         xip.set_group_id(check_cast<uint8_t, const char *>(args[3].c_str()));
//         xip.set_xip_type(check_cast<uint8_t, const char *>(args[4].c_str()));
//     }
//     return 0;
// } catch (...) {
//     std::cout << "catched error" << std::endl;
//     return 1;
// }

void ElectCommands::AddBaseCommands() try {
    AddCommand("help", [this](const Arguments & args) { this->PrintUsage(); });
    AddCommand("gid", [this](const Arguments & args) {
        const auto gid = global_xid->Get();
        std::cout << "global_xid: " << HexEncode(gid) << std::endl;
    });
    // AddCommand("getnode", [this](const Arguments & args) {
    //     uint64_t service_type = 1;
    //     if (args.size() >= 1) {
    //         service_type = check_cast<uint64_t, const char *>(args[0].c_str());
    //     }
    //     std::vector<kadmlia::NodeInfoPtr> nodes;
    //     GetRootNodes(service_type, nodes);
    //     for (auto & n : nodes) {
    //         std::cout << "getnode:" << HexEncode(n->node_id) << " ip:" << n->public_ip << " port:" << n->public_port << std::endl;
    //     }
    // });

    // AddCommand("vsend", [this](const Arguments & args) {
    //     if (args.size() < 1) {
    //         std::cout << "param invaid, useage: vsend [hex_des_node_id] true false" << std::endl;
    //         return;
    //     }
    //     std::string des_node_id = args[0];
    //     bool broadcast = true;
    //     bool root = false;
    //     if (args.size() >= 2) {
    //         broadcast = check_cast<bool, const char *>(args[1].c_str());
    //     }
    //     if (args.size() >= 3) {
    //         root = check_cast<bool, const char *>(args[2].c_str());
    //     }
    //     vhost_send(des_node_id, broadcast, root);
    // });

    AddCommand("prt", [this](const Arguments & args) {
        std::vector<base::ServiceType> vec_type;
        wrouter::MultiRouting::Instance()->GetAllRegisterType(vec_type);
        std::cout << "GetAllRegisterType size:" << vec_type.size() << std::endl;
        // bool kroot_flag = false;
        // for (const auto & type : vec_type) {
        //     if (type == kRoot) {
        //         kroot_flag = true;
        //         break;
        //     }
        // }
        // if (!kroot_flag) {
        // vec_type.push_back(base::ServiceType{kRoot});
        // }

        for (const auto & type : vec_type) {
            auto routing_table = wrouter::MultiRouting::Instance()->GetElectRoutingTable(type);
            if (!routing_table) {
                std::cout << "warning: " << type.value() << " routing table invalid" << std::endl;
                continue;
            }
            LocalNodeInfoPtr local_node = routing_table->get_local_node_info();
            if (!local_node) {
                std::cout << "warning: " << type.value() << " routing table invalid" << std::endl;
                continue;
            }
            std::cout << "local_nodeid: " << (local_node->kad_key()) << std::endl
                      << "service_type: " << type.info() << std::endl
                    //   << "xnetwork_id: " << local_node->kadmlia_key()->xnetwork_id() << std::endl
                    //   << "zone_id: " << static_cast<uint32_t>(local_node->kadmlia_key()->zone_id()) << std::endl
                    //   << "cluster_id: " << static_cast<uint32_t>(local_node->kadmlia_key()->cluster_id()) << std::endl
                    //   << "group_id: " << static_cast<uint32_t>(local_node->kadmlia_key()->group_id()) << std::endl
                      << "neighbours: " << routing_table->nodes_size() << std::endl
                      << std::endl;
        }
    });
    // AddCommand("root", [this](const Arguments & args) {
    //     uint32_t xnetwork_id = kRoot;
    //     uint32_t xnetwork_type = 0;
    //     if (args.size() >= 1) {
    //         xnetwork_id = check_cast<uint32_t, const char *>(args[0].c_str());
    //     }
    //     if (xnetwork_id == kRoot) {
    //         auto routing_table = wrouter::GetRoutingTable(kRoot, true);
    //         if (!routing_table) {
    //             std::cout << "kRoot routing_table not registered" << std::endl;
    //             return;
    //         }
    //         elect_perf_.PrintRoutingTable(routing_table);
    //         return;
    //     }

    //     if (args.size() >= 2) {
    //         xnetwork_type = check_cast<uint8_t, const char *>(args[1].c_str());
    //     }
    //     std::vector<uint64_t> vec_type;
    //     wrouter::GetAllRegisterType(vec_type);
    //     std::cout << "found " << vec_type.size() << " routing table registered" << std::endl;
    //     for (auto & type : vec_type) {
    //         auto routing_table = wrouter::GetRoutingTable(type, true);
    //         if (!routing_table) {
    //             continue;
    //         }
    //         auto r_xnetwork_id = routing_table->get_local_node_info()->kadmlia_key()->xnetwork_id();
    //         auto r_xnetwork_type = routing_table->get_local_node_info()->kadmlia_key()->xip_type();
    //         std::cout << "r_xnetwork_id:" << r_xnetwork_id << " r_xnetwork_type:" << r_xnetwork_type << std::endl;
    //         if (r_xnetwork_id == xnetwork_id && r_xnetwork_type == xnetwork_type) {
    //             elect_perf_.PrintRoutingTable(routing_table);
    //             break;
    //         }
    //     }
    // });
    // AddCommand("xm", [this](const Arguments & args) {
    //     base::XipParser xip;
    //     GetEcXip(args, xip);
    //     uint64_t service_type = base::CreateServiceType(xip);
    //     auto routing_table = wrouter::GetRoutingTable(service_type);
    //     if (!routing_table) {
    //         return;
    //     }

    //     auto nodes = routing_table->nodes();
    //     auto local_node = routing_table->get_local_node_info();
    //     TOP_FATAL(
    //         "self: [%s - %s] [%s:%d]", HexSubstr(global_xid->Get()).c_str(), HexSubstr(local_node->id()).c_str(), local_node->public_ip().c_str(), (int)local_node->public_port());
    //     for (auto & node : nodes) {
    //         TOP_FATAL("node: [%s - %s] [%s:%d]", HexSubstr(node->xid).c_str(), HexSubstr(node->node_id).c_str(), node->public_ip.c_str(), (int)node->public_port);
    //     }
    // });

    AddCommand("crt", [this](const Arguments & args) {
        uint32_t test_num = 1;
        if (args.size() >= 1) {
            test_num = check_cast<uint32_t, const char *>(args[0].c_str());
        }
        uint32_t test_len = 100;
        if (args.size() >= 2) {
            test_len = check_cast<uint32_t, const char *>(args[1].c_str());
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

        elect_perf_.TestChainTrade(test_num, test_len, gossip_type, backup, neighbors_num, stop_times, max_hop_num, evil_rate, layer_switch_hop_num, left_overlap, right_overlap);
    });
    AddCommand("crtnode", [this](const Arguments & args) {
        if (args.size() < 2) {
            std::cout << "invalid params, usage: crtnode [src_node_id] [des_node_id]" << std::endl;
            return;
        }

        std::string src_node_id = args[0];
        std::string des_node_id = args[1];

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

        if (test_num > 1000) {
            std::size_t cnt = test_num / 100;
            for (std::size_t index = 0; index < cnt; ++index) {
                elect_perf_.TestChainTradeServiceNet(src_node_id,
                                                     des_node_id,
                                                     100,
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
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
            return;
        }
        elect_perf_.TestChainTradeServiceNet(src_node_id,
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
    });

    #if 0
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
        std::vector<uint64_t> vec_type;
        wrouter::GetAllRegisterType(vec_type);
        bool kroot_flag = false;
        for (const auto & type : vec_type) {
            if (type == kRoot) {
                kroot_flag = true;
                break;
            }
        }
        if (!kroot_flag) {
            vec_type.push_back(kRoot);
        }
        for (const auto & type : vec_type) {
            xJson::Value one_ret;

            auto routing_table = wrouter::GetRoutingTable(type, false);
            if (!routing_table) {
                std::cout << "warning: " << type << " routing table invalid" << std::endl;
                continue;
            }
            LocalNodeInfoPtr local_node = routing_table->get_local_node_info();
            if (!local_node) {
                std::cout << "warning: " << type << " routing table invalid" << std::endl;
                continue;
            }

            one_ret["local_nodeid"] = HexEncode(local_node->id());
            one_ret["service_type"] = (xJson::UInt64)local_node->kadmlia_key()->GetServiceType();
            one_ret["neighbours"] = routing_table->nodes_size();

            std::string net_type = get_net_type(local_node->kadmlia_key()->xnetwork_id(),
                                                static_cast<uint32_t>(local_node->kadmlia_key()->zone_id()),
                                                static_cast<uint32_t>(local_node->kadmlia_key()->cluster_id()),
                                                static_cast<uint32_t>(local_node->kadmlia_key()->group_id()));
            ret[net_type] = one_ret;

            std::cout << "local_nodeid: " << HexEncode(local_node->id()) << std::endl
                      << "service_type: " << local_node->kadmlia_key()->GetServiceType() << std::endl
                      << "xnetwork_id: " << local_node->kadmlia_key()->xnetwork_id() << std::endl
                      << "zone_id: " << static_cast<uint32_t>(local_node->kadmlia_key()->zone_id()) << std::endl
                      << "cluster_id: " << static_cast<uint32_t>(local_node->kadmlia_key()->cluster_id()) << std::endl
                      << "group_id: " << static_cast<uint32_t>(local_node->kadmlia_key()->group_id()) << std::endl
                      << "neighbours: " << routing_table->nodes_size() << std::endl
                      << std::endl;
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
    #endif

} catch (std::exception & e) {
    std::cout << "catch error: (" << e.what() << ") check_cast failed" << std::endl;
}

void ElectCommands::AddRpcCommand(const std::string & cmd_name, RpcCommandProc cmd_proc) {
    assert(cmd_proc);
    std::unique_lock<std::mutex> lock(rpc_cmd_map_mutex);

    auto it = rpc_cmd_map.find(cmd_name);
    if (it != rpc_cmd_map.end()) {
        return;
    }
    rpc_cmd_map[cmd_name] = cmd_proc;
}

void ElectCommands::AddCommand(const std::string & cmd_name, CommandProc cmd_proc) {
    assert(cmd_proc);
    std::unique_lock<std::mutex> lock(map_commands_mutex_);

    auto it = map_commands_.find(cmd_name);
    if (it != map_commands_.end()) {
        TOP_WARN("command(%s) exist and ignore new one", cmd_name.c_str());
        return;
    }

    map_commands_[cmd_name] = cmd_proc;
    TOP_INFO("add command(%s)", cmd_name.c_str());
}

// void ElectCommands::GetRootNodes(uint64_t service_type, std::vector<kadmlia::NodeInfoPtr> & nodes) {
//     auto routing = wrouter::GetRoutingTable(kRoot, true);
//     auto root = dynamic_cast<wrouter::RootRouting *>(routing.get());
//     if (!root) {
//         std::cout << "get kRoot failedl" << std::endl;
//         return;
//     }
//     root->GetRootNodes(service_type, nodes);
// }

// void ElectCommands::vhost_send(const std::string & des_node_id, bool broadcast, bool root) {
//     if (des_node_id.empty()) {
//         std::cout << "please input (Hex)des_node_id" << std::endl;
//         return;
//     }
//     // TODO(smaug) write some codes
//     return;
// }

}  //  namespace top
