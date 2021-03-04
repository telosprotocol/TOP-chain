// Copyright (c) 2017-2019 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <atomic>
#include <condition_variable>
#include <memory>
#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <functional>

#include "xkad/routing_table/node_info.h"
#include "xkad/routing_table/routing_utils.h"
#include "xpbase/base/xip_parser.h"
#include "xpbase/base/top_timer.h"

namespace top {

using Arguments = std::vector<std::string>;
using CommandProc = std::function<void (const Arguments&)>;
using MapCommands = std::map<std::string, CommandProc>;

namespace elect {
class EcVHost;
}

namespace kadmlia {
class RoutingTable;
}

namespace base {
class xpacket_t;
};

namespace transport {
namespace protobuf {
class RoutingMessage;
};
};

namespace wrouter {
class Wrouter;
};

struct PacketInfo {
    uint32_t des_network_id;
    uint32_t msg_hash;
    uint32_t hop_num;
    uint64_t spread_time;
    uint64_t spread_time_sync;   // sync message
    std::string msg_src_id;
};


class TopCommands {
public:
    static TopCommands* Instance();
    bool Init(bool first_node, bool show_cmd);
    void Run();
    void Destroy() { destroy_ = true; }


    void ProcessCommand(const std::string& cmdline);
protected:
    TopCommands();
    ~TopCommands();
    virtual bool InitExtra() { return true; }
    void AddCommand(const std::string& cmd_name, CommandProc cmd_proc);
    void AddBaseCommands();
    virtual void AddExtraCommands() {}
    void PrintUsage();

    // 
    int GetRootNodes(uint64_t service_type);
    int GetRootNodes(const std::string& hex_target_id);
    void PrintStat();

    void TestChainTrade(
            transport::protobuf::RoutingMessage& message,
            uint32_t des_network_id,
            uint32_t test_num,
            uint32_t backup,
            uint32_t neighbors_num,
            uint32_t stop_times,
            uint32_t max_hop_num,
            uint32_t evil_rate,
            uint32_t gossip_type,
            uint32_t layer_switch_hop_num,
            uint32_t left_overlap,
            uint32_t right_overlap,
            bool block_header = false);

    void HandleTestChainTrade(
            transport::protobuf::RoutingMessage& message,
            base::xpacket_t& packet);
    void PrintRoutingTable(uint64_t service_type, bool root = false);
    void GetRootNodes(uint32_t xnetwork_id, std::vector<kadmlia::NodeInfoPtr>& nodes);

    void test_send(const std::string& des_node_id, bool broadcast = true, bool root = false);

    void GossipWithHeaderBlock(
            transport::protobuf::RoutingMessage& pbft_message,
            uint32_t block_gossip_type,
            uint32_t chain_data_hash,
            uint32_t chain_msgid);
 

    std::mutex wait_mutex_;
    std::condition_variable wait_cond_var_;
    std::mutex init_mutex_;
    bool first_node_;
    bool show_cmd_;
    bool inited_;
    bool destroy_;

    std::mutex map_commands_mutex_;
    MapCommands map_commands_;
    std::shared_ptr<base::TimerRepeated> test_timer_ {nullptr};

    std::mutex stat_map_mutex_;
    std::map<uint32_t,std::shared_ptr<std::vector<PacketInfo>>> stat_map_;
    std::shared_ptr<base::TimerRepeated> stat_map_timer_ {nullptr};
    std::shared_ptr<elect::EcVHost> ec_vhost_{nullptr};
};

}  //  namespace top
