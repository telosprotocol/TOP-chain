#pragma once

#include "xbase/xpacket.h"
#include "xkad/routing_table/routing_utils.h"
#include "xkad/proto/kadmlia.pb.h"
#include "xkad/routing_table/callback_manager.h"
#include "xelect_net/include/elect_perf.h"

//using namespace top::elect;
namespace top {

namespace elect {
class ElectPerf;

class PerformanceMessagehandler {
public:
    PerformanceMessagehandler(ElectPerf*);
    ~PerformanceMessagehandler();

private:
    void HandleRoundTripTimeRequest(transport::protobuf::RoutingMessage& message, base::xpacket_t& packet);
    void HandleRoundTripTimeResponse(transport::protobuf::RoutingMessage& message, base::xpacket_t& packet);
    void HandleRelayTestRequest(transport::protobuf::RoutingMessage& message, base::xpacket_t& packet);
    void HandleRelayTestResponse(transport::protobuf::RoutingMessage& message, base::xpacket_t& packet);
    void HandleTellBootstrapStopped(transport::protobuf::RoutingMessage& message, base::xpacket_t& packet);
    void HandleGetGroupNodesRequest(transport::protobuf::RoutingMessage& message, base::xpacket_t& packet);
    void HandleGetGroupNodesResponse(transport::protobuf::RoutingMessage& message, base::xpacket_t& packet);
    void HandleGetAllNodesFromBootRequest(
            transport::protobuf::RoutingMessage& message,
            base::xpacket_t& packet);
    void HandleGetAllNodesFromBootResponse(
            transport::protobuf::RoutingMessage& message,
            base::xpacket_t& packet);
    void HandleGetEcBackupFromBootRequest(
            transport::protobuf::RoutingMessage& message,
            base::xpacket_t& packet);
    void HandleGetEcBackupFromBootResponse(
            transport::protobuf::RoutingMessage& message,
            base::xpacket_t& packet);
    void HandleGetSubMemberFromBootRequest(
            transport::protobuf::RoutingMessage& message,
            base::xpacket_t& packet);
    void HandleGetSubMemberFromBootResponse(
            transport::protobuf::RoutingMessage& message,
            base::xpacket_t& packet);
    void HandleBroadcastPerformaceTest(
            transport::protobuf::RoutingMessage& message,
            base::xpacket_t& packet);

    top::elect::ElectPerf* routing_perf_ptr_;
    std::atomic<int> udperf_count_{0};
    std::chrono::system_clock::time_point udperf_start_;
    base::TimerRepeated udperf_timer_{base::TimerManager::Instance(), "PerformanceMessagehandler"};

    DISALLOW_COPY_AND_ASSIGN(PerformanceMessagehandler);
};

}  // namespace kadmlia

}  // namespace top
