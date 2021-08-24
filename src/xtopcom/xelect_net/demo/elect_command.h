//
//  top_commands.h
//  user commands
//
//  Created by Charlie Xie on 01/15/2019.
//  Copyright (c) 2017-2019 Telos Foundation & contributors
//

#pragma once

#include "json/json.h"
#include "xelect_net/demo/elect_perf.h"
#include "xelect_net/include/elect_netcard.h"
#include "xkad/routing_table/node_info.h"
#include "xkad/routing_table/routing_utils.h"

#include <atomic>
#include <condition_variable>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace top {

using Arguments = std::vector<std::string>;
using CommandProc = std::function<void(const Arguments &)>;
using RpcCommandProc = std::function<xJson::Value(const Arguments &)>;
using MapCommands = std::map<std::string, CommandProc>;

class ServiceDemo;

namespace kadmlia {
class RoutingTable;
struct NodeInfo;
typedef std::shared_ptr<NodeInfo> NodeInfoPtr;
}  // namespace kadmlia

namespace wrouter {
class Wrouter;
};

class ElectCommands {
public:
    ElectCommands();
    ~ElectCommands();

    /**
     * @brief init ElectCommands, add basic commands
     *
     * @param first_node is this node the first node or not
     * @param show_cmd start shell then waiting for input-commands or not
     * @return true
     * @return false
     */
    bool Init(bool first_node, bool show_cmd);
    /**
     * @brief start loop and waiting for commands
     *
     */
    void Run();
    /**
     * @brief stop loop and exit ElectCommands
     *
     */
    void Destroy() {
        destroy_ = true;
    }
    /**
     * @brief Set the netcard object
     *
     * @param ec_netcard
     */
    void set_netcard(elect::EcNetcardPtr ec_netcard);
    /**
     * @brief handle commands
     *
     * @param cmdline command that to be executed
     */
    void ProcessCommand(const std::string & cmdline);

    xJson::Value ProcessRpcCommand(const std::string & cmdline, uint64_t cmd_index);

    void get_udp_instruction();
    void get_rpc_instruction();

private:
    void AddCommand(const std::string & cmd_name, CommandProc cmd_proc);
    void AddBaseCommands();
    virtual void AddExtraCommands() {
    }
    void PrintUsage();
    // int GetEcXip(const Arguments & args, base::XipParser & xip);
    void GetRootNodes(uint64_t service_type, std::vector<kadmlia::NodeInfoPtr> & nodes);
    void vhost_send(const std::string & des_node_id, bool broadcast = true, bool root = false);

    MapCommands map_commands_;
    std::mutex map_commands_mutex_;

    void AddRpcCommand(const std::string & cmd_name, RpcCommandProc cmd_proc);
    std::map<std::string, RpcCommandProc> rpc_cmd_map;
    std::mutex rpc_cmd_map_mutex;

    elect::ElectPerf elect_perf_;
    bool destroy_{false};
    bool show_cmd_{false};
    bool first_node_{false};
    elect::EcNetcardPtr ec_netcard_;
};

}  //  namespace top
