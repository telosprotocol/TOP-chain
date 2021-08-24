#include "xelect_net/demo/elect_perf.h"

#include "xbase/xutl.h"
#include "xwrouter/wrouter_utils/wrouter_utils.h"
#include "xkad/routing_table/local_node_info.h"
#include "xmetrics/xmetrics.h"
#include "xpbase/base/top_log.h"
#include "xutility/xhash.h"
#include "xwrouter/register_message_handler.h"
#include "xwrouter/multi_routing/multi_routing.h"
#include "xwrouter/xwrouter.h"

using namespace top::kadmlia;
namespace top {
namespace elect {

ElectPerf::ElectPerf() {
#define IS_BROADCAST(message) (message.broadcast())
#define IS_RRS_GOSSIP_MESSAGE(message) message.is_root() && message.broadcast() && message.gossip().gossip_type() == 8
#define IS_RRS_PULLED_MESSAGE(message) message.ack_id() == 181819
#define MESSAGE_BASIC_INFO(message) "src_node_id", (message.src_node_id()), "dst_node_id", (message.des_node_id()), "hop_num", message.hop_num()
#define MESSAGE_RRS_FEATURE(message) "gossip_header_hash", std::stol(message.gossip().header_hash()), "gossip_block_size", message.gossip().block().size()
#define MESSAGE_FEATURE(message) "msg_hash", message.msg_hash(), "msg_size", message.gossip().block().size()
#define IS_ROOT_BROADCAST(message) "is_root", message.is_root(), "is_broadcast", message.broadcast()
#define PACKET_SIZE(packet) "packet_size", packet.get_size()
#define NOW_TIME "timestamp", GetCurrentTimeMsec()

    wrouter::WrouterRegisterMessageHandler(kTestMessageType, [this](transport::protobuf::RoutingMessage & message, base::xpacket_t & packet) {
        if (IS_RRS_GOSSIP_MESSAGE(message)) {
            XMETRICS_PACKET_INFO("p2pperf_vhostrecv_info",
                                 MESSAGE_BASIC_INFO(message),
                                 MESSAGE_RRS_FEATURE(message),
                                 IS_ROOT_BROADCAST(message),
                                 "is_pulled",
                                 IS_RRS_PULLED_MESSAGE(message),
                                 PACKET_SIZE(packet),
                                 NOW_TIME);
        } else {
            if (IS_BROADCAST(message)) {
                XMETRICS_PACKET_INFO(
                    "p2pnormal_vhostrecv_info", MESSAGE_BASIC_INFO(message), MESSAGE_FEATURE(message), IS_ROOT_BROADCAST(message), "is_pulled", 0, PACKET_SIZE(packet), NOW_TIME);
            }
        }
        return;
    });
#undef IS_BROADCAST
#undef IS_RRS_GOSSIP_MESSAGE
#undef IS_RRS_PULLED_MESSAGE
#undef MESSAGE_BASIC_INFO
#undef MESSAGE_RRS_FEATURE
#undef MESSAGE_FEATURE
#undef IS_ROOT_BROADCAST
#undef PACKET_SIZE
#undef NOW_TIME
}

ElectPerf::~ElectPerf() {
}

// void ElectPerf::PrintRoutingTable(top::kadmlia::RoutingTablePtr & routing_table) {
//     if (!routing_table) {
//         TOP_ERROR("routing table empty");
//         return;
//     }

//     top::kadmlia::LocalNodeInfoPtr local_node = routing_table->get_local_node_info();
//     if (!local_node) {
//         return;
//     }

//     auto udp_transport = routing_table->get_transport();
//     if (!udp_transport) {
//         return;
//     }

//     std::cout << "self: " << HexSubstr(local_node->id()) << ", " << local_node->local_ip() << ":" << local_node->local_port() << ", " << local_node->public_ip() << ":"
//               << local_node->public_port() << ", "
//               << "[" << local_node->nat_type() << "]"
//               << ", xid:" << HexSubstr(local_node->xid()) << std::endl;
//     std::vector<top::kadmlia::NodeInfoPtr> nodes = routing_table->GetClosestNodes(local_node->id(), kRoutingMaxNodesSize);
//     if (nodes.empty()) {
//         return;
//     }
//     top::kadmlia::NodeInfoPtr node = nodes[0];
//     for (uint32_t i = 0; i < nodes.size(); ++i) {
//         std::cout << HexSubstr(nodes[i]->node_id) << ", " << nodes[i]->local_ip << ":" << nodes[i]->local_port << ", " << nodes[i]->public_ip << ":" << nodes[i]->public_port
//                   << ", "
//                   << "[" << nodes[i]->nat_type << "]"
//                   << ", " << HexEncode(nodes[i]->xip) << ", xid:" << HexSubstr(nodes[i]->xid) << ", bucket_index:" << nodes[i]->bucket_index << std::endl;
//     }
//     std::cout << "all node size(include self) " << nodes.size() + 1 << std::endl;
// }

void ElectPerf::TestChainTrade(uint32_t test_num,
                               uint32_t test_len,
                               uint32_t gossip_type,
                               uint32_t backup,
                               uint32_t neighbors_num,
                               uint32_t stop_times,
                               uint32_t max_hop_num,
                               uint32_t evil_rate,
                               uint32_t layer_switch_hop_num,
                               uint32_t left_overlap,
                               uint32_t right_overlap) {
    // auto routing_table = wrouter::GetRoutingTable(kRoot, true);
    // auto routing_table = wrouter::MultiRouting::Instance()->GetRootRoutingTable();
    // if (!routing_table) {
    //     TOP_WARN("kRoot routing table not exists.");
    //     return;
    // }
    std::cout << std::dec << "num:" << test_num << ",len:" << test_len << ",type:" << gossip_type << std::endl;

    static std::atomic<uint32_t> total_send_count(0);
    uint32_t send_count = 0;

    uint32_t looop = test_num * backup;

    uint64_t start2 = GetCurrentTimeMsec();
    for (uint32_t n = 0; n < looop; ++n) {
        std::string test_raw_data = RandomString(test_len);
        transport::protobuf::RoutingMessage message;
        message.set_broadcast(true);
        // routing_table->SetFreqMessage(message);
        message.set_is_root(true);  // Entire network broadcast
        message.set_src_node_id(global_xid->Get());
        auto des_kad_key = std::make_shared<base::KadmliaKey>("anything");  // anything
        message.set_des_node_id(des_kad_key->Get());
        message.set_type(kTestMessageType);
        message.set_id(CallbackManager::MessageId());
        // message.set_data(test_raw_data);
        // message.clear_data();
        uint32_t vhash = base::xhash32_t::digest(test_raw_data);
        std::string header_hash = std::to_string(vhash);

        auto gossip = message.mutable_gossip();

        gossip->set_neighber_count(neighbors_num);
        gossip->set_stop_times(stop_times);
        gossip->set_gossip_type(gossip_type);
        gossip->set_max_hop_num(max_hop_num);
        gossip->set_block(test_raw_data);
        gossip->set_header_hash(header_hash);

        std::string data;
        if (!message.SerializeToString(&data)) {
            TOP_WARN("wrouter message SerializeToString failed");
            return;
        }

        auto new_msg = message;
        message.set_id(CallbackManager::MessageId());
        message.clear_bloomfilter();
        uint32_t msg_hash = base::xhash32_t::digest(std::to_string(message.id()) + test_raw_data);
        message.set_msg_hash(msg_hash);
        wrouter::Wrouter::Instance()->send(message);
        ++send_count;
    }

    total_send_count += send_count;
    auto use_time_ms = double(GetCurrentTimeMsec() - start2) / 1000.0;
    std::cout << "send " << send_count << " use time: " << use_time_ms << " sec. QPS: " << (uint32_t)((double)send_count / use_time_ms) * neighbors_num
              << " total_send:" << total_send_count << std::endl;
}

void ElectPerf::TestChainTradeServiceNet(const std::string & src_node_id,
                                         const std::string & des_node_id,
                                         uint32_t test_num,
                                         uint32_t test_len,
                                         uint32_t gossip_type,
                                         uint32_t overlap_rate,
                                         uint32_t neighbors_num,
                                         uint32_t stop_times,
                                         uint32_t max_hop_num,
                                         uint32_t evil_rate,
                                         uint32_t layer_switch_hop_num,
                                         uint32_t left_overlap,
                                         uint32_t right_overlap) {
    std::cout << std::dec << "src_node_id" << src_node_id << "des_node_id" << des_node_id << std::endl << "num:" << test_num << ",len:" << test_len << ",type:" << gossip_type << std::endl;
    uint32_t looop = test_num;

    uint64_t start2 = GetCurrentTimeMsec();
    static std::atomic<uint32_t> total_send_count(0);
    uint32_t send_count = 0;
    for (uint32_t n = 0; n < looop; ++n) {
        transport::protobuf::RoutingMessage message;
        std::string test_raw_data = RandomString(test_len);

        message.set_broadcast(true);
        message.set_is_root(false);  // for only one service network
        message.set_src_node_id(src_node_id);
        message.set_des_node_id(des_node_id);
        message.set_type(kTestMessageType);
        message.set_id(CallbackManager::MessageId());
        message.set_data(test_raw_data);

        auto gossip = message.mutable_gossip();

        gossip->set_neighber_count(neighbors_num);
        gossip->set_stop_times(stop_times);
        gossip->set_gossip_type(gossip_type);
        gossip->set_max_hop_num(max_hop_num);
        gossip->set_overlap_rate(overlap_rate);

        std::string data;
        if (!message.SerializeToString(&data)) {
            TOP_WARN("wrouter message SerializeToString failed");
            return;
        }

        message.set_id(CallbackManager::MessageId());
        message.clear_bloomfilter();
        uint32_t msg_hash = base::xhash32_t::digest(std::to_string(message.id()) + message.data());
        message.set_msg_hash(msg_hash);

        wrouter::Wrouter::Instance()->send(message);
        ++send_count;
    }
    total_send_count += send_count;
    auto use_time_ms = double(GetCurrentTimeMsec() - start2) / 1000.0;
    std::cout << "send " << send_count << " use time: " << use_time_ms << " sec. QPS: " << (uint32_t)((double)send_count / use_time_ms) * neighbors_num
              << " total_send:" << total_send_count << std::endl;
}

xJson::Value ElectPerf::rpc_broadcast_all(uint32_t test_num,
                                          uint32_t test_len,
                                          uint32_t gossip_type,
                                          uint32_t backup,
                                          uint32_t neighbors_num,
                                          uint32_t stop_times,
                                          uint32_t max_hop_num,
                                          uint32_t evil_rate,
                                          uint32_t layer_switch_hop_num,
                                          uint32_t left_overlap,
                                          uint32_t right_overlap) {
    xJson::Value ret;
    ret["msghash"] = {};

    // auto routing_table = wrouter::GetRoutingTable(kRoot, true);
    // auto routing_table = wrouter::MultiRouting::Instance()->GetRootRoutingTable();
    // if (!routing_table) {
    //     TOP_WARN("kRoot routing table not exists.");
    //     return ret;
    // }
    static std::atomic<uint32_t> total_send_count(0);
    uint32_t send_count = 0;

    uint32_t looop = test_num * backup;

    uint64_t start2 = GetCurrentTimeMsec();
    for (uint32_t n = 0; n < looop; ++n) {
        std::string test_raw_data = RandomString(test_len);
        transport::protobuf::RoutingMessage message;
        message.set_broadcast(true);

        message.set_is_root(true);  // Entire network broadcast
        message.set_src_node_id(global_xid->Get());
        auto des_kad_key = std::make_shared<base::KadmliaKey>("anything");  // anything
        message.set_des_node_id(des_kad_key->Get());
        message.set_type(kTestMessageType);
        message.set_id(CallbackManager::MessageId());
        message.set_data(test_raw_data);
        uint32_t vhash = base::xhash32_t::digest(test_raw_data);
        std::string header_hash = std::to_string(vhash);

        auto gossip = message.mutable_gossip();

        gossip->set_neighber_count(neighbors_num);
        gossip->set_stop_times(stop_times);
        gossip->set_gossip_type(gossip_type);
        gossip->set_max_hop_num(max_hop_num);

        gossip->set_header_hash(header_hash);

        std::string data;
        if (!message.SerializeToString(&data)) {
            TOP_WARN("wrouter message SerializeToString failed");
            return ret;
        }

        auto new_msg = message;
        message.set_id(CallbackManager::MessageId());
        message.clear_bloomfilter();
        uint32_t msg_hash = base::xhash32_t::digest(std::to_string(message.id()) + message.data());
        message.set_msg_hash(msg_hash);

        XMETRICS_PACKET_INFO("p2pdemo_broadcast_sendmsg_hash",
                             "msg_hash",
                             msg_hash,
                             "msg_size",
                             message.data().size(),
                             "timestamp",
                             GetCurrentTimeMicSec(),
                             "is_broadcast",
                             static_cast<uint64_t>(message.broadcast()));
        ret["msghash"].append(msg_hash);

        wrouter::Wrouter::Instance()->send(message);
        ++send_count;
    }

    total_send_count += send_count;

    XMETRICS_PACKET_INFO("p2pdemo_broadcast_sendmsg_actually_tps", "send_num", send_count, "use_time", (GetCurrentTimeMsec() - start2));
    return ret;
}

xJson::Value ElectPerf::rpc_broadcast_all_new(uint32_t test_num,
                                              uint32_t test_len,
                                              uint32_t gossip_type,
                                              uint32_t backup,
                                              uint32_t neighbors_num,
                                              uint32_t stop_times,
                                              uint32_t max_hop_num,
                                              uint32_t evil_rate,
                                              uint32_t layer_switch_hop_num,
                                              uint32_t left_overlap,
                                              uint32_t right_overlap) {
    xJson::Value ret;
    ret["msghash"] = {};

    // auto routing_table = wrouter::GetRoutingTable(kRoot, true);
    // auto routing_table = wrouter::MultiRouting::Instance()->GetRootRoutingTable();
    // if (!routing_table) {
    //     TOP_WARN("kRoot routing table not exists.");
    //     return ret;
    // }
    static std::atomic<uint32_t> total_send_count(0);
    uint32_t send_count = 0;

    uint32_t looop = test_num * backup;

    uint64_t start2 = GetCurrentTimeMsec();
    for (uint32_t n = 0; n < looop; ++n) {
        std::string test_raw_data = RandomString(test_len);
        transport::protobuf::RoutingMessage message;
        message.set_broadcast(true);

        message.set_is_root(true);  // Entire network broadcast
        message.set_src_node_id(global_xid->Get());
        auto des_kad_key = std::make_shared<base::KadmliaKey>("anything");  // anything
        message.set_des_node_id(des_kad_key->Get());
        message.set_type(kTestMessageType);
        message.set_id(CallbackManager::MessageId());
        // message.set_data(test_raw_data);
        // message.clear_data();
        uint32_t vhash = base::xhash32_t::digest(test_raw_data);
        std::string header_hash = std::to_string(vhash);

        auto gossip = message.mutable_gossip();

        gossip->set_neighber_count(neighbors_num);
        gossip->set_stop_times(stop_times);
        gossip->set_gossip_type(gossip_type);
        gossip->set_max_hop_num(max_hop_num);

        gossip->set_block(test_raw_data);
        gossip->set_header_hash(header_hash);

        std::string data;
        if (!message.SerializeToString(&data)) {
            TOP_WARN("wrouter message SerializeToString failed");
            return ret;
        }

        auto new_msg = message;
        message.set_id(CallbackManager::MessageId());
        message.clear_bloomfilter();
        uint32_t msg_hash = base::xhash32_t::digest(std::to_string(message.id()) + test_raw_data);
        message.set_msg_hash(msg_hash);

        XMETRICS_PACKET_INFO("p2pdemo_broadcast_sendmsg_hash",
                             "msg_hash",
                             msg_hash,
                             "msg_size",
                             message.gossip().block().size(),
                             "timestamp",
                             GetCurrentTimeMicSec(),
                             "is_broadcast",
                             static_cast<uint64_t>(message.broadcast()));
        ret["msghash"].append(msg_hash);

        wrouter::Wrouter::Instance()->send(message);
        ++send_count;
    }

    total_send_count += send_count;

    XMETRICS_PACKET_INFO("p2pdemo_broadcast_sendmsg_actually_tps", "send_num", send_count, "use_time", (GetCurrentTimeMsec() - start2));
    return ret;
}

xJson::Value ElectPerf::rpc_broadcast_to_cluster(const std::string & src_node_id,
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
                                                 uint32_t right_overlap) {
    xJson::Value ret;
    ret["msghash"] = {};

    uint32_t looop = test_num * backup;
    uint64_t start2 = GetCurrentTimeMsec();
    static std::atomic<uint32_t> total_send_count(0);
    uint32_t send_count = 0;

    for (uint32_t n = 0; n < looop; ++n) {
        transport::protobuf::RoutingMessage message;
        std::string test_raw_data = RandomString(test_len);

        message.set_broadcast(true);
        message.set_is_root(false);  // for only one service network
        message.set_src_node_id(src_node_id);
        message.set_des_node_id(des_node_id);
        message.set_type(kTestMessageType);
        message.set_id(CallbackManager::MessageId());
        message.set_data(test_raw_data);
        
        uint32_t vhash = base::xhash32_t::digest(test_raw_data);
        std::string header_hash = std::to_string(vhash);

        auto gossip = message.mutable_gossip();

        gossip->set_neighber_count(neighbors_num);
        gossip->set_stop_times(stop_times);
        gossip->set_gossip_type(gossip_type);
        gossip->set_max_hop_num(max_hop_num);

        gossip->set_block(test_raw_data);
        gossip->set_header_hash(header_hash);

        std::string data;
        if (!message.SerializeToString(&data)) {
            TOP_WARN("wrouter message SerializeToString failed");
            return ret;
        }

        message.set_id(CallbackManager::MessageId());
        message.clear_bloomfilter();
        uint32_t msg_hash = base::xhash32_t::digest(std::to_string(message.id()) + message.data());
        message.set_msg_hash(msg_hash);

        XMETRICS_PACKET_INFO("p2pdemo_send_sendmsg_hash",
                             "msg_hash",
                             msg_hash,
                             "msg_size",
                             message.data().size(),
                             "timestamp",
                             GetCurrentTimeMicSec(),
                             "is_broadcast",
                             static_cast<uint64_t>(message.broadcast()));
        ret["msghash"].append(msg_hash);

        wrouter::Wrouter::Instance()->send(message);
        ++send_count;
    }
    total_send_count += send_count;

    XMETRICS_PACKET_INFO("p2pdemo_send_sendmsg_actually_tps", "send_num", send_count, "use_time", (GetCurrentTimeMsec() - start2));
    return ret;
}

}  // namespace elect

}  // namespace top
