#pragma once

#include <stdint.h>
#include <string>
#include <vector>
#include <map>
#include <memory>

#include "xkad/routing_table/node_info.h"
#include "xkad/routing_table/routing_utils.h"
#include "xkad/routing_table/callback_manager.h"
#include "xelect_net/include/performance_message_handler.h"

namespace top {

namespace kadmlia {
class RoutingTable;
typedef std::shared_ptr<RoutingTable> RoutingTablePtr;
};
namespace elect {
class PerformanceMessagehandler;

class ElectPerf {
public:
    ElectPerf();
    ~ElectPerf();
    void PrintRoutingTable(top::kadmlia::RoutingTablePtr& routing_table);
    void PrintRoutingTableAll(uint64_t service_type);
    void TestLayerdBroadcast2(
            uint32_t h_backup,
            uint32_t h_neighbors_num,
	        uint32_t h_packet_len,
            uint32_t h_stop_times,
            uint32_t h_max_hop_num,
            uint32_t h_evil_rate,
            uint32_t h_gossip_type,
            uint32_t h_layer_switch_hop_num,
            uint32_t h_left_overlap,
            uint32_t h_right_overlap,
            uint32_t b_backup,
            uint32_t b_neighbors_num,
            uint32_t b_stop_times,
            uint32_t b_max_hop_num,
            uint32_t b_evil_rate,
            uint32_t b_gossip_type,
            uint32_t b_layer_switch_hop_num,
            uint32_t b_left_overlap,
            uint32_t b_right_overlap);

    // Entire network broadcast
    void TestChainTrade(
            uint32_t test_num,
            uint32_t test_len,
            uint32_t gossip_type,
            uint32_t backup,
            uint32_t neighbors_num,
            uint32_t stop_times,
            uint32_t max_hop_num,
            uint32_t evil_rate,
            uint32_t layer_switch_hop_num,
            uint32_t left_overlap,
            uint32_t right_overlap);
            
    // Super node broadcast
    void TestSuperBroadcast(
        uint32_t test_num,
        uint32_t test_len,
        uint32_t backup,
        uint32_t neighbors_num,
        uint32_t stop_times,
        uint32_t max_hop_num,
        uint32_t evil_rate,
        std::string & src_node_id);
 

private:
    bool destroy_;
    PerformanceMessagehandler* performance_message_handler_;

    DISALLOW_COPY_AND_ASSIGN(ElectPerf);
};

}  // namespace elect

}  // namespace top
