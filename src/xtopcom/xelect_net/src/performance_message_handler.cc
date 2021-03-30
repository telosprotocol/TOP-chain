#include "xelect_net/include/performance_message_handler.h"

#include "xpbase/base/top_log.h"
#include "xwrouter/register_message_handler.h"
#include "xwrouter/register_routing_table.h"
#include "xkad/routing_table/routing_table.h"
#include "xkad/routing_table/callback_manager.h"
#include "xtransport/transport.h"
#include "xkad/routing_table/local_node_info.h"
#include "xkad/routing_table/callback_manager.h"
#include "xelect_net/include/performance_utils.h"
#include "xpbase/base/redis_client.h"
#include "xwrouter/register_routing_table.h"
#include "xpbase/base/kad_key/platform_kadmlia_key.h"
#include "xelect_net/include/elect_perf.h"

using namespace top::kadmlia;

namespace top {
namespace elect {

PerformanceMessagehandler::PerformanceMessagehandler(
        top::elect::ElectPerf* routing_perf_ptr)
        : routing_perf_ptr_(routing_perf_ptr) {
    wrouter::WrouterRegisterMessageHandler(kRoundTripTimeRequest, [this](
            transport::protobuf::RoutingMessage& message,
            base::xpacket_t& packet) {
        HandleRoundTripTimeRequest(message, packet);
        TOP_DEBUG("HandleMessage kRoundTripTimeRequest");
    });

    wrouter::WrouterRegisterMessageHandler(kRoundTripTimeResponse, [this](
            transport::protobuf::RoutingMessage& message,
            base::xpacket_t& packet) {
        HandleRoundTripTimeResponse(message, packet);
        TOP_DEBUG("HandleMessage kRoundTripTimeResponse");
    });

    wrouter::WrouterRegisterMessageHandler(kRelayTestRequest, [this](
            transport::protobuf::RoutingMessage& message,
            base::xpacket_t& packet) {
        HandleRelayTestRequest(message, packet);
        TOP_DEBUG("HandleMessage kRelayTestRequest");
    });

    wrouter::WrouterRegisterMessageHandler(kRelayTestResponse, [this](
            transport::protobuf::RoutingMessage& message,
            base::xpacket_t& packet) {
        HandleRelayTestResponse(message, packet);
        TOP_DEBUG("HandleMessage kRelayTestResponse");
    });

    wrouter::WrouterRegisterMessageHandler(kTellBootstrapStopped, [this](
            transport::protobuf::RoutingMessage& message,
            base::xpacket_t& packet) {
        HandleTellBootstrapStopped(message, packet);
        TOP_DEBUG("HandleMessage kTellBootstrapStopped");
    });

    wrouter::WrouterRegisterMessageHandler(kGetGroupNodesRequest, [this](
            transport::protobuf::RoutingMessage& message,
            base::xpacket_t& packet) {
        HandleGetGroupNodesRequest(message, packet);
        TOP_DEBUG("HandleMessage kGetGroupNodesRequest");
    });

    wrouter::WrouterRegisterMessageHandler(kGetGroupNodesResponse, [this](
            transport::protobuf::RoutingMessage& message,
            base::xpacket_t& packet) {
        HandleGetGroupNodesResponse(message, packet);
        TOP_DEBUG("HandleMessage kGetGroupNodesResponse");
    });

    wrouter::WrouterRegisterMessageHandler(kBroadcastPerformaceTest, [this](
            transport::protobuf::RoutingMessage& message,
            base::xpacket_t& packet) {
        HandleBroadcastPerformaceTest(message, packet);
        TOP_DEBUG("HandleMessage kBroadcastPerformaceTest");
    });
        
    wrouter::WrouterRegisterMessageHandler(kBroadcastPerformaceTestReset, [this](
            transport::protobuf::RoutingMessage& message,
            base::xpacket_t& packet) {
        if (!message.has_gossip()) {
            return;
        }
        auto gossip = message.gossip();
        if (gossip.has_block()) {
            std::cout << "kBroadcastPerformaceTestReset with block coming." << std::endl;
        } else {
//             std::cout << "kBroadcastPerformaceTestReset header coming." << std::endl;
        }
    });

    wrouter::WrouterRegisterMessageHandler(kUdperfStart, [this](
            transport::protobuf::RoutingMessage& message,
            base::xpacket_t& packet) {
        this->udperf_count_ = 0;
        udperf_start_ = std::chrono::system_clock::now();
        TOP_FATAL("udperf start");
        udperf_timer_.Start(
            500 * 1000,
            500 * 1000,
            [this]{
                const int count = this->udperf_count_;
                TOP_FATAL("timer_count: %d", count);
            });
    });

    wrouter::WrouterRegisterMessageHandler(kUdperfFinish, [this](
            transport::protobuf::RoutingMessage& message,
            base::xpacket_t& packet) {
        auto end_time = std::chrono::system_clock::now();
        const int count = this->udperf_count_;
        auto cost_seconds = std::chrono::duration_cast<std::chrono::duration<double>>(end_time - udperf_start_).count();
        TOP_FATAL("send cost %.3lf, %.1lf packet/s", cost_seconds, count / cost_seconds);
        TOP_FATAL("udperf finish: %d", count);
        udperf_timer_.Join();
    });

    wrouter::WrouterRegisterMessageHandler(kUdperfTest, [this](
            transport::protobuf::RoutingMessage& message,
            base::xpacket_t& packet) {
        this->udperf_count_ += 1;
    });

    wrouter::WrouterRegisterMessageHandler(kTestChainTrade, [this](
            transport::protobuf::RoutingMessage& message,
            base::xpacket_t& packet) {
            TOP_WARN("1recv testchaintradehash:%u", message.gossip().msg_hash());
           return;
    });

    wrouter::WrouterRegisterMessageHandler(kTestSuperBroadcastRecv, [this](
            transport::protobuf::RoutingMessage& message,
            base::xpacket_t& packet) {
            TOP_WARN("kTestSuperBroadcastRecv msgid(%d) msg_hash(%u) src_node_id[%s] dst_node_id[%s]",
                            message.id(),message.gossip().msg_hash(),
                            HexEncode(message.src_node_id()).c_str(),
                            HexEncode(message.des_node_id()).c_str());
           return;
    });

}

PerformanceMessagehandler::~PerformanceMessagehandler() {
    wrouter::WrouterUnregisterMessageHandler(kRoundTripTimeRequest);
    wrouter::WrouterUnregisterMessageHandler(kRoundTripTimeResponse);
    wrouter::WrouterUnregisterMessageHandler(kRelayTestRequest);
    wrouter::WrouterUnregisterMessageHandler(kRelayTestResponse);
    wrouter::WrouterUnregisterMessageHandler(kTellBootstrapStopped);
    wrouter::WrouterUnregisterMessageHandler(kGetGroupNodesRequest);
    wrouter::WrouterUnregisterMessageHandler(kGetGroupNodesResponse);
    wrouter::WrouterUnregisterMessageHandler(kGetEcBackupFromBootRequest);
    wrouter::WrouterUnregisterMessageHandler(kGetEcBackupFromBootResponse);
    wrouter::WrouterUnregisterMessageHandler(kGetSubMemberFromBootRequest);
    wrouter::WrouterUnregisterMessageHandler(kGetSubMemberFromBootResponse);
}

void PerformanceMessagehandler::HandleTellBootstrapStopped(
        transport::protobuf::RoutingMessage& message,
        base::xpacket_t& packet) {
    RoutingTablePtr routing_table = wrouter::GetRoutingTable(message.des_service_type());
    if (!routing_table) {
        TOP_ERROR("service type[%llu] has not register routing table.", message.des_service_type());
        return;
    }

    LocalNodeInfoPtr local_node = routing_table->get_local_node_info();
    if (!local_node) {
        return;
    }

//    if (message.des_node_id() == local_node->id()) {
//        routing_perf_ptr_->NodeStopped(message.des_service_type(), message.src_node_id());
//    }
}

void PerformanceMessagehandler::HandleRelayTestRequest(
        transport::protobuf::RoutingMessage& message,
        base::xpacket_t& packet) {
    RoutingTablePtr routing_table = wrouter::GetRoutingTable(message.des_service_type());
    if (!routing_table) {
        TOP_ERROR("service type[%llu] has not register routing table.", message.des_service_type());
        return;
    }

    LocalNodeInfoPtr local_node = routing_table->get_local_node_info();
    if (!local_node) {
        return;
    }

    if (message.des_node_id() != local_node->id()) {
        routing_table->SendToClosestNode(message);
        return;
    }

    message.clear_hop_nodes();
    message.set_type(kRelayTestResponse);
    message.set_des_node_id(message.src_node_id());
    message.set_src_node_id(local_node->id());
    routing_table->SendToClosestNode(message);
}

void PerformanceMessagehandler::HandleRelayTestResponse(
        transport::protobuf::RoutingMessage& message,
        base::xpacket_t& packet) {
    RoutingTablePtr routing_table = wrouter::GetRoutingTable(message.des_service_type());
    if (!routing_table) {
        TOP_ERROR("service type[%llu] has not register routing table.", message.des_service_type());
        return;
    }

    LocalNodeInfoPtr local_node = routing_table->get_local_node_info();
    if (!local_node) {
        return;
    }

    if (message.des_node_id() != local_node->id()) {
        routing_table->SendToClosestNode(message);
        return;
    }

    kadmlia::CallbackManager::Instance()->Callback(message.id(), message, packet);
}

void PerformanceMessagehandler::HandleRoundTripTimeRequest(
        transport::protobuf::RoutingMessage& message,
        base::xpacket_t& packet) {
    message.clear_hop_nodes();
    RoutingTablePtr routing_table = wrouter::GetRoutingTable(message.des_service_type());
    if (!routing_table) {
        TOP_ERROR("service type[%llu] has not register routing table.", message.des_service_type());
        return;
    }

    LocalNodeInfoPtr local_node = routing_table->get_local_node_info();
    if (!local_node) {
        return;
    }

    message.set_type(kRoundTripTimeResponse);
    message.set_des_node_id(message.src_node_id());
    message.set_src_node_id(local_node->id());
    std::shared_ptr<transport::Transport> transport_ptr = routing_table->get_transport();
    if (!transport_ptr) {
        TOP_ERROR("service type[%llu] has not register udp transport.", message.des_service_type());
        return;
    }
    std::string msg;
    if (!message.SerializeToString(&msg)) {
        TOP_INFO("RoutingMessage SerializeToString failed!");
        return ;
    }
    xbyte_buffer_t xdata{msg.begin(), msg.end()};
    transport_ptr->SendData(
            xdata,
            packet.get_from_ip_addr(),
            packet.get_from_ip_port());
}

void PerformanceMessagehandler::HandleRoundTripTimeResponse(
        transport::protobuf::RoutingMessage& message,
        base::xpacket_t& packet) {
    kadmlia::CallbackManager::Instance()->Callback(message.id(), message, packet);
}

void PerformanceMessagehandler::HandleGetGroupNodesRequest(
        transport::protobuf::RoutingMessage& message,
        base::xpacket_t& packet) {
    RoutingTablePtr routing_table = wrouter::GetRoutingTable(message.des_service_type());
    if (!routing_table) {
        TOP_ERROR("service type[%llu] has not register routing table.", message.des_service_type());
        return;
    }

    LocalNodeInfoPtr local_node = routing_table->get_local_node_info();
    if (!local_node) {
        return;
    }

    if (message.des_node_id() != local_node->id()) {
        bool closest = false;
        if (routing_table->ClosestToTarget(
                    message.des_node_id(), closest) != kKadSuccess) {
            TOP_WARN("this message must drop! this node is not des "
                 "but nearest node is this node![%s] to [%s] [%d] [%d]",
                 HexSubstr(message.src_node_id()).c_str(),
                 HexSubstr(message.des_node_id()).c_str(),
                 message.type(), closest);
            return;
        }

        if (!closest) {
            return routing_table->SendToClosestNode(message);
        }
    }

    if (!message.has_data() || message.data().empty()) {
        TOP_INFO("HandleGetGroupNodesRequest has no data!");
        return;
    }

    protobuf::GetNearestNodesRequest get_nodes_req;
    if (!get_nodes_req.ParseFromString(message.data())) {
        TOP_INFO("GetNearestNodesRequest ParseFromString failed!");
        return;
    }

    auto nodes = routing_table->GetClosestNodes(
                     get_nodes_req.target_id(),
                     get_nodes_req.count() - 1);
    transport::protobuf::RoutingMessage res_message;
    res_message.set_src_service_type(message.des_service_type());
    res_message.set_des_service_type(message.src_service_type());
    res_message.set_hop_num(0);
    res_message.set_src_node_id(local_node->id());
    res_message.set_des_node_id(message.src_node_id());
    res_message.set_type(kGetGroupNodesResponse);
    res_message.set_id(message.id());
    protobuf::GetNearestNodesResponse get_nodes_res;
    protobuf::NodeInfo* node_info = get_nodes_res.add_nodes();
    node_info->set_id(local_node->id());
    for (uint32_t i = 0; i < nodes.size(); ++i) {
        if (static_cast<uint32_t>(get_nodes_res.nodes_size()) >= get_nodes_req.count()) {
            break;
        }

        protobuf::NodeInfo* node_info = get_nodes_res.add_nodes();
        node_info->set_id(nodes[i]->node_id);
    }
    std::string data;
    if (!get_nodes_res.SerializeToString(&data)) {
        TOP_INFO("GetNearestNodesResponse SerializeToString failed!");
        return;
    }

    res_message.set_data(data);
    routing_table->SendToClosestNode(res_message);
}

void PerformanceMessagehandler::HandleGetGroupNodesResponse(
        transport::protobuf::RoutingMessage& message,
        base::xpacket_t& packet) {
    RoutingTablePtr routing_table = wrouter::GetRoutingTable(message.des_service_type());
    if (!routing_table) {
        TOP_ERROR("service type[%llu] has not register routing table.", message.des_service_type());
        return;
    }

    LocalNodeInfoPtr local_node = routing_table->get_local_node_info();
    if (!local_node) {
        return;
    }

    if (message.des_node_id() != local_node->id()) {
        bool closest = false;
        if (routing_table->ClosestToTarget(
                    message.des_node_id(), closest) != kKadSuccess || closest) {
            TOP_WARN("this message must drop! this node is not des "
                 "but nearest node is this node![%s] to [%s] [%d] [%d]",
                 HexSubstr(message.src_node_id()).c_str(),
                 HexSubstr(message.des_node_id()).c_str(),
                 message.type(), closest);
            return;
        }

        routing_table->SendToClosestNode(message);
        return;
    }

    kadmlia::CallbackManager::Instance()->Callback(message.id(), message, packet);
}

void PerformanceMessagehandler::HandleGetAllNodesFromBootRequest(
        transport::protobuf::RoutingMessage& message,
        base::xpacket_t& packet) {
    RoutingTablePtr routing_table = wrouter::GetRoutingTable(message.des_service_type());
    if (!routing_table) {
        TOP_ERROR("service type[%llu] has not register routing table.", message.des_service_type());
        return;
    }

    LocalNodeInfoPtr local_node = routing_table->get_local_node_info();
    if (!local_node) {
        return;
    }

    if (message.des_node_id() != local_node->id()) {
        return;
    }

    if (!message.has_data() || message.data().empty()) {
        TOP_INFO("data is empty.");
        return;
    }

    protobuf::GetAllNodesFromBootRequest req;
    if (!req.ParseFromString(message.data())) {
        TOP_INFO("GetAllNodesFromBootRequest ParseFromString failed!");
        return;
    }
/*
    std::vector<NodeInfoPtr> nodes;
    routing_perf_ptr_->GetAllNodes(message.des_service_type(), req.start_pos(), req.len(), nodes);
    if (nodes.empty()) {
        return;
    }

    transport::protobuf::RoutingMessage res_message;
    res_message.set_src_service_type(message.des_service_type());
    res_message.set_des_service_type(message.src_service_type());
    res_message.set_src_node_id(message.des_node_id());
    res_message.set_des_node_id(message.src_node_id());
    res_message.set_type(kGetAllNodesFromBootResponse);
    res_message.set_id(message.id());
    res_message.set_hop_num(message.hop_num());

    protobuf::GetAllNodesFromBootResponse nodes_res;
    for (uint32_t i = 0; i < nodes.size(); ++i) {
        if (!nodes[i]) {
            continue;
        }
        auto ptr_node_info = nodes_res.add_nodes();
        ptr_node_info->set_id(
            nodes[i]->node_id);
        ptr_node_info->set_public_ip(
            nodes[i]->public_ip.c_str(),
            nodes[i]->public_ip.size());
        ptr_node_info->set_public_port(
            nodes[i]->public_port);
        ptr_node_info->set_local_ip(
            nodes[i]->local_ip.c_str(),
            nodes[i]->local_ip.size());
        ptr_node_info->set_local_port(nodes[i]->local_port);
    }
    std::string data;
    if (!nodes_res.SerializeToString(&data)) {
        TOP_INFO("UdpNatDetectResponse SerializeToString failed!");
        return;
    }

    res_message.set_data(data);
    std::shared_ptr<transport::Transport> transport_ptr = routing_table->get_transport();
    if (!transport_ptr) {
        TOP_ERROR("service type[%llu] has not register udp transport.", message.des_service_type());
        return;
    }


    std::string msg;
    if (!res_message.SerializeToString(&msg)) {
        TOP_INFO("RoutingMessage SerializeToString failed!");
        return ;
    }
    xbyte_buffer_t xdata{msg.begin(), msg.end()};
    transport_ptr->SendData(
            xdata,
            packet.get_from_ip_addr(),
            packet.get_from_ip_port());
*/
}

void PerformanceMessagehandler::HandleGetAllNodesFromBootResponse(
        transport::protobuf::RoutingMessage& message,
        base::xpacket_t& packet) {
    kadmlia::CallbackManager::Instance()->Callback(message.id(), message, packet);
}
void PerformanceMessagehandler::HandleBroadcastPerformaceTest(
        transport::protobuf::RoutingMessage& message,
        base::xpacket_t& packet) {
	if (!message.has_gossip()) {
		TOP_INFO("HandleBroadcastPerformaceTest, no gossip.");
		return;
	}
	auto gossip = message.gossip();
	if (gossip.has_block()) {
		TOP_INFO("kBroadcastPerformaceTest with block coming.");
	} else {
	   	TOP_INFO("kBroadcastPerformaceTest header coming.");
	}
}
}  // namespace kadmlia

}  // namespace top
