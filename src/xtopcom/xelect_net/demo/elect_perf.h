#pragma once

#include "json/json.h"
#include "xkad/routing_table/callback_manager.h"
#include "xkad/routing_table/node_info.h"
#include "xkad/routing_table/routing_utils.h"

#include <stdint.h>

#include <map>
#include <memory>
#include <string>
#include <vector>
namespace top {

namespace kadmlia {
class RoutingTable;
typedef std::shared_ptr<RoutingTable> RoutingTablePtr;
};  // namespace kadmlia
namespace elect {

class ElectPerf {
public:
    ElectPerf();
    ~ElectPerf();
    void PrintRoutingTable(top::kadmlia::RoutingTablePtr & routing_table);

    // Entire network broadcast
    void TestChainTrade(uint32_t test_num,
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

    void TestChainTradeServiceNet(const std::string & src_node_id,
                                  const std::string & des_node_id,
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

    xJson::Value rpc_broadcast_all(uint32_t test_num,
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

    // for test rrs_gossip
    xJson::Value rpc_broadcast_all_new(uint32_t test_num,
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

    xJson::Value rpc_broadcast_to_cluster(const std::string & src_node_id,
                                          const std::string & des_node_id,
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

private:
    bool destroy_;

    DISALLOW_COPY_AND_ASSIGN(ElectPerf);
};

}  // namespace elect

}  // namespace top
