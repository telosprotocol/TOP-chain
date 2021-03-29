//
//  top_commands.cc
//  user commands
//
//  Created by Charlie Xie on 01/15/2019.
//  Copyright (c) 2017-2019 Telos Foundation & contributors
//

#include "xelect_net/include/elect_command.h"

#include <string.h>
#ifdef _WIN32
#include <winsock2.h>
#include <time.h>
#else
#include <sys/time.h>
#endif

#include <algorithm>
#include <iostream>
#include <fstream>
#include <chrono>
#include <utility>
#include <list>
#include <string>

#include "xbase/xutl.h"

#include "xpbase/base/line_parser.h"
#include "xpbase/base/top_log.h"
#include "xpbase/base/top_utils.h"
#include "xpbase/base/check_cast.h"
#include "xpbase/base/xip_parser.h"
#include "xpbase/base/top_string_util.h"
#include "xpbase/base/sem.h"
#include "xpbase/base/kad_key/platform_kadmlia_key.h"
#include "xkad/routing_table/routing_table.h"
#include "xkad/routing_table/callback_manager.h"
#include "xkad/routing_table/local_node_info.h"
#include "xwrouter/register_routing_table.h"
#include "xwrouter/root/root_routing.h"
#include "xpbase/base/xid/xid_def.h"
#include "xpbase/base/xid/xid_parser.h"
#include "xpbase/base/xid/xid_db_session.h"
#include "xpbase/base/kad_key/kadmlia_key.h"
#include "xelect_net/include/elect_routing.h"
#include "xelect_net/include/elect_uitils.h"

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

void ElectCommands::Run() {
    PrintUsage();

    while (!destroy_) {
        if (!show_cmd_) {
            SleepUs(200000);
            continue;
        }

        std::cout << std::endl << std::endl << "Enter command > ";
        std::string cmdline;
        std::getline(std::cin, cmdline);
        {
            ProcessCommand(cmdline);
        }
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

void ElectCommands::ProcessCommand(const std::string& cmdline) {
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
    } catch (const std::exception& e) {
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

int ElectCommands::GetEcXip(const Arguments& args, base::XipParser& xip) try {
    if (args.size() == 0) {
        xip.set_xnetwork_id(kChainRecNet);
        xip.set_xip_type(elect::kElectionCommittee);
    } else if (args.size() == 1) {
        xip.set_xnetwork_id(check_cast<uint32_t, const char*>(args[0].c_str()));
        xip.set_xip_type(elect::kElectionCommittee);
    } else if (args.size() == 2) {
        xip.set_xnetwork_id(check_cast<uint32_t, const char*>(args[0].c_str()));
        xip.set_xip_type(check_cast<uint8_t, const char*>(args[1].c_str()));
    } else if (args.size() == 3) {
        xip.set_xnetwork_id(check_cast<uint32_t, const char*>(args[0].c_str()));
        xip.set_zone_id(check_cast<uint8_t, const char*>(args[1].c_str()));
        xip.set_xip_type(check_cast<uint8_t, const char*>(args[2].c_str()));
    } else if (args.size() == 4) {
        xip.set_xnetwork_id(check_cast<uint32_t, const char*>(args[0].c_str()));
        xip.set_zone_id(check_cast<uint8_t, const char*>(args[1].c_str()));
        xip.set_cluster_id(check_cast<uint8_t, const char*>(args[2].c_str()));
        xip.set_xip_type(check_cast<uint8_t, const char*>(args[3].c_str()));
    } else if (args.size() == 5) {
        xip.set_xnetwork_id(check_cast<uint32_t, const char*>(args[0].c_str()));
        xip.set_zone_id(check_cast<uint8_t, const char*>(args[1].c_str()));
        xip.set_cluster_id(check_cast<uint8_t, const char*>(args[2].c_str()));
        xip.set_group_id(check_cast<uint8_t, const char*>(args[3].c_str()));
        xip.set_xip_type(check_cast<uint8_t, const char*>(args[4].c_str()));
    }
    return 0;
} catch (...) {
    std::cout << "catched error" << std::endl;
    return 1;
}

void ElectCommands::AddBaseCommands() try {
    AddCommand("help", [this](const Arguments& args){
        this->PrintUsage();
    });
    AddCommand("gid", [this](const Arguments& args) {
        const auto gid = global_xid->Get();
        std::cout << "global_xid: " << HexEncode(gid) << std::endl;
    });
    AddCommand("getnode", [this](const Arguments& args) {
        uint64_t service_type = 1;
        if (args.size() >= 1) {
            service_type = check_cast<uint64_t, const char*>(args[0].c_str());
        }
        std::vector<kadmlia::NodeInfoPtr> nodes;
        GetRootNodes(service_type, nodes);
        for (auto& n : nodes) {
            std::cout << "getnode:" << HexEncode(n->node_id) << " ip:" << n->public_ip << " port:" << n->public_port << std::endl;
        }
    });

    AddCommand("vsend", [this](const Arguments& args) {
        if (args.size() < 1) {
            std::cout << "param invaid, useage: vsend [hex_des_node_id] true false" << std::endl;
            return;
        }
        std::string des_node_id =  args[0];
        bool broadcast = true;
        bool root = false;
        if (args.size() >= 2) {
            broadcast = check_cast<bool, const char*>(args[1].c_str());
        }
        if (args.size() >= 3) {
            root = check_cast<bool, const char*>(args[2].c_str());
        }
        vhost_send(des_node_id, broadcast, root);
    });

    AddCommand("prt", [this](const Arguments& args){
        uint32_t xnetwork_id = kChainRecNet;
        uint32_t xnetwork_type = elect::kElectionCommittee;
        if (args.size() >= 1) {
            xnetwork_id = check_cast<uint32_t, const char*>(args[0].c_str());
        }
        if (args.size() >= 2) {
            xnetwork_type = check_cast<uint8_t, const char*>(args[1].c_str());
        }
        std::vector<uint64_t> vec_type;
        wrouter::GetAllRegisterType(vec_type);
        std::cout << "found " << vec_type.size() << " routing table registered" << std::endl;
        for (auto& type : vec_type) {
            auto routing_table = wrouter::GetRoutingTable(type, false);
            if (!routing_table) {
                continue;
            }
            auto r_xnetwork_id = routing_table->get_local_node_info()->kadmlia_key()->xnetwork_id();
            auto r_xnetwork_type = routing_table->get_local_node_info()->kadmlia_key()->xip_type();
            std::cout << "r_xnetwork_id:" << r_xnetwork_id << " r_xnetwork_type:" << r_xnetwork_type << std::endl;
            if (r_xnetwork_id == xnetwork_id && r_xnetwork_type == xnetwork_type) {
                elect_perf_.PrintRoutingTable(routing_table);
                break;
            }
        }
    });
    AddCommand("root", [this](const Arguments& args) {
        uint32_t xnetwork_id = kRoot;
        uint32_t xnetwork_type = 0;
        if (args.size() >= 1) {
            xnetwork_id = check_cast<uint32_t, const char*>(args[0].c_str());
        }
        if (xnetwork_id == kRoot) {
            auto routing_table = wrouter::GetRoutingTable(kRoot, true);
            if (!routing_table) {
                std::cout << "kRoot routing_table not registered" << std::endl;
                return;
            }
            elect_perf_.PrintRoutingTable(routing_table);
            return;
        }

        if (args.size() >= 2) {
            xnetwork_type = check_cast<uint8_t, const char*>(args[1].c_str());
        }
        std::vector<uint64_t> vec_type;
        wrouter::GetAllRegisterType(vec_type);
        std::cout << "found " << vec_type.size() << " routing table registered" << std::endl;
        for (auto& type : vec_type) {
            auto routing_table = wrouter::GetRoutingTable(type, true);
            if (!routing_table) {
                continue;
            }
            auto r_xnetwork_id = routing_table->get_local_node_info()->kadmlia_key()->xnetwork_id();
            auto r_xnetwork_type = routing_table->get_local_node_info()->kadmlia_key()->xip_type();
            std::cout << "r_xnetwork_id:" << r_xnetwork_id << " r_xnetwork_type:" << r_xnetwork_type << std::endl;
            if (r_xnetwork_id == xnetwork_id && r_xnetwork_type == xnetwork_type) {
                elect_perf_.PrintRoutingTable(routing_table);
                break;
            }
        }
    });
    AddCommand("prt_all", [this](const Arguments& args){
        elect_perf_.PrintRoutingTableAll(kEdgeXVPN);
    });

    AddCommand("xm", [this](const Arguments& args) {
        base::XipParser xip;
        GetEcXip(args, xip);
        uint64_t service_type = base::CreateServiceType(xip);
        auto routing_table = wrouter::GetRoutingTable(service_type);
        if (!routing_table) {
            return;
        }

        auto nodes = routing_table->nodes();
        auto local_node = routing_table->get_local_node_info();
        TOP_FATAL("self: [%s - %s] [%s:%d]", HexSubstr(global_xid->Get()).c_str(), HexSubstr(local_node->id()).c_str(),
            local_node->public_ip().c_str(), (int)local_node->public_port());
        for (auto& node : nodes) {
            TOP_FATAL("node: [%s - %s] [%s:%d]", HexSubstr(node->xid).c_str(), HexSubstr(node->node_id).c_str(),
                node->public_ip.c_str(), (int)node->public_port);
        }
    });


    AddCommand("brthash", [this](const Arguments& args) {
        uint32_t num = 1;
        if (args.size() >= 1) {
            num = check_cast<uint32_t, const char*>(args[0].c_str());
        }

        uint32_t h_backup = 1;
        if (args.size() >= 2) {
            h_backup = check_cast<uint32_t, const char*>(args[1].c_str());
        }

        uint32_t h_neighbors_num = 3;
        if (args.size() >= 3) {
            h_neighbors_num = check_cast<uint32_t, const char*>(args[2].c_str());
        }

        uint32_t h_packet_len = 256;

        if (args.size() >= 4) {
            h_packet_len = check_cast<uint32_t, const char*>(args[3].c_str());
        }

        uint32_t h_stop_times = 2;

        uint32_t h_max_hop_num = 10;
/*        if (args.size() >= 5) {
            h_max_hop_num = check_cast<uint32_t, const char*>(args[4].c_str());
        }*/

        uint32_t h_evil_rate = 0;
/*        if (args.size() >= 6) {
            h_evil_rate = check_cast<uint32_t, const char*>(args[5].c_str());
       }*/

        uint32_t h_gossip_type = 1;
/*        if (args.size() >= 7) {
            h_gossip_type = check_cast<uint32_t, const char*>(args[6].c_str());
        }*/

        uint32_t h_layer_switch_hop_num = 3;
/*        if (args.size() >= 8) {
            h_layer_switch_hop_num = check_cast<uint32_t, const char*>(args[7].c_str());
        }*/
        uint32_t h_left_overlap = 0;
        uint32_t h_right_overlap = 10;

        // for block
        uint32_t b_evil_rate = h_evil_rate;
/*        if (args.size() >= 9) {
            h_evil_rate = check_cast<uint32_t, const char*>(args[8].c_str());
        }*/
        uint32_t b_backup = 1;
        uint32_t b_neighbors_num = h_neighbors_num;
        uint32_t b_stop_times = h_stop_times;
        uint32_t b_max_hop_num = h_max_hop_num;
        uint32_t b_gossip_type = 3;
        uint32_t b_layer_switch_hop_num = h_layer_switch_hop_num;
        uint32_t b_left_overlap = 0;
        uint32_t b_right_overlap = 10;


        for (uint32_t i = 0; i < num; ++i) {
            elect_perf_.TestLayerdBroadcast2(
                    h_backup,
                    h_neighbors_num,
                    h_packet_len,
                    h_stop_times,
                    h_max_hop_num,
                    h_evil_rate,
                    h_gossip_type,
                    h_layer_switch_hop_num,
                    h_left_overlap,
                    h_right_overlap,
                    b_backup,
                    b_neighbors_num,
                    b_stop_times,
                    b_max_hop_num,
                    b_evil_rate,
                    b_gossip_type,
                    b_layer_switch_hop_num,
                    b_left_overlap,
                    b_right_overlap);
        }
    });
    AddCommand("crt", [this](const Arguments& args) {
        uint32_t test_num = 1;
        if (args.size() >= 1) {
            test_num = check_cast<uint32_t, const char*>(args[0].c_str());
        }
        uint32_t test_len = 100;
        if (args.size() >= 2) {
            test_len = check_cast<uint32_t, const char*>(args[1].c_str());
        }
        uint32_t gossip_type = 1;
        if (args.size() >= 3) {
            gossip_type = check_cast<uint32_t, const char*>(args[2].c_str());
        }
        uint32_t backup = 1;
        if (args.size() >= 4) {
            backup = check_cast<uint32_t, const char*>(args[3].c_str());
        }
        uint32_t neighbors_num = 3;
        if (args.size() >= 5) {
            neighbors_num = check_cast<uint32_t, const char*>(args[4].c_str());
        }
        uint32_t stop_times = 3;
        if (args.size() >= 6) {
            stop_times = check_cast<uint32_t, const char*>(args[5].c_str());
        }
        uint32_t max_hop_num = 10;
        if (args.size() >= 7) {
            max_hop_num = check_cast<uint32_t, const char*>(args[6].c_str());
        }
        uint32_t evil_rate = 0;
        if (args.size() >= 8) {
            evil_rate = check_cast<uint32_t, const char*>(args[7].c_str());
        }
        uint32_t layer_switch_hop_num = 2;
        if (args.size() >= 9) {
            layer_switch_hop_num = check_cast<uint32_t, const char*>(args[8].c_str());
        }
        uint32_t left_overlap = 0;
        if (args.size() >= 10) {
            left_overlap = check_cast<uint32_t, const char*>(args[9].c_str());
        }
        uint32_t right_overlap = 0;
        if (args.size() >= 11) {
            right_overlap = check_cast<uint32_t, const char*>(args[10].c_str());
        }

        elect_perf_.TestChainTrade(
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

    AddCommand("csb", [this](const Arguments& args) {
        uint32_t test_num = 1;
        if (args.size() >= 1) {
            test_num = check_cast<uint32_t, const char*>(args[0].c_str());
        }
        uint32_t test_len = 100;
        if (args.size() >= 2) {
            test_len = check_cast<uint32_t, const char*>(args[1].c_str());
        }
        uint32_t backup = 1;
        if (args.size() >= 3) {
            backup = check_cast<uint32_t, const char*>(args[2].c_str());
        }
        uint32_t neighbors_num = 3;
        if (args.size() >= 4) {
            neighbors_num = check_cast<uint32_t, const char*>(args[3].c_str());
        }
        uint32_t stop_times = 3;
        if (args.size() >= 5) {
            stop_times = check_cast<uint32_t, const char*>(args[4].c_str());
        }
        uint32_t max_hop_num = 10;
        if (args.size() >= 6) {
            max_hop_num = check_cast<uint32_t, const char*>(args[5].c_str());
        }
        uint32_t evil_rate = 0;
        if (args.size() >= 7) {
            evil_rate = check_cast<uint32_t, const char*>(args[6].c_str());
        }

        std::string str_node_id("010000010143ffffffffffffffffffff000000009e8ae44a2f14cf4e64487d9e777a2606");
        if (args.size() >= 8) {
            str_node_id =args[7];
        }
        elect_perf_.TestSuperBroadcast(
                test_num,
                test_len,
                backup,
                neighbors_num,
                stop_times,
                max_hop_num,
                evil_rate,
                str_node_id);
    });

 } catch (std::exception& e) {
     std::cout << "catch error: (" << e.what() << ") check_cast failed" << std::endl;
 }

void ElectCommands::AddCommand(const std::string& cmd_name, CommandProc cmd_proc) {
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


void ElectCommands::GetRootNodes(uint64_t service_type, std::vector<kadmlia::NodeInfoPtr>& nodes) {
    auto routing = wrouter::GetRoutingTable(kRoot, true);
    auto root = dynamic_cast<wrouter::RootRouting*>(routing.get());
    if (!root)  {
        std::cout << "get kRoot failedl" << std::endl;
        return;
    }
    root->GetRootNodes(service_type, nodes);
}

void ElectCommands::vhost_send(const std::string& des_node_id, bool broadcast, bool root) {
    if (des_node_id.empty()) {
        std::cout << "please input (Hex)des_node_id" << std::endl;
        return;
    }
    // TODO(smaug) write some codes
    return;
}

}  //  namespace top
