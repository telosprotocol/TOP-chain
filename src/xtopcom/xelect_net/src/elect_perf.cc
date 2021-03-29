#include "xelect_net/include/elect_perf.h"

#include "xpbase/base/top_log.h"
#include "xkad/routing_table/routing_table.h"
#include "xwrouter/xwrouter.h"
#include "xwrouter/register_routing_table.h"
#include "xelect_net/include/performance_utils.h"
#include "xkad/routing_table/local_node_info.h"
#include "xbase/xutl.h"
#include "xpbase/base/kad_key/chain_kadmlia_key.h"


#include "xutility/xhash.h"

using namespace top::kadmlia;
namespace top {
namespace elect {

ElectPerf::ElectPerf() {
	performance_message_handler_ = new PerformanceMessagehandler(this);
}

ElectPerf::~ElectPerf() {
	delete performance_message_handler_;
}

void ElectPerf::PrintRoutingTable(top::kadmlia::RoutingTablePtr& routing_table) {
    if (!routing_table) {
        TOP_ERROR("routing table empty");
        return;
    }

    top::kadmlia::LocalNodeInfoPtr local_node = routing_table->get_local_node_info();
    if (!local_node) {
        return;
    }

    auto udp_transport = routing_table->get_transport();
    if (!udp_transport) {
        return;
    }

    std::cout << "self: " << HexSubstr(local_node->id()) << ", " <<
            local_node->local_ip() << ":" << local_node->local_port() << ", " <<
            local_node->public_ip() << ":" << local_node->public_port() <<  ", " <<
            "[" << local_node->nat_type() << "]" <<
            ", " << HexEncode(local_node->xip()) << ", xid:" << HexSubstr(local_node->xid()) << std::endl;
    std::vector<top::kadmlia::NodeInfoPtr> nodes = routing_table->GetClosestNodes(
            local_node->id(),
            kRoutingMaxNodesSize);
    if (nodes.empty()) {
        return;
    }
    top::kadmlia::NodeInfoPtr node = nodes[0];
    for (uint32_t i = 0; i < nodes.size(); ++i) {
        std::cout << HexSubstr(nodes[i]->node_id) << ", " <<
            nodes[i]->local_ip << ":" << nodes[i]->local_port << ", " <<
            nodes[i]->public_ip << ":" << nodes[i]->public_port <<  ", " <<
            "[" << nodes[i]->nat_type << "]" << ", " << HexEncode(nodes[i]->xip)
            << ", xid:" << HexSubstr(nodes[i]->xid)
            << ", bucket_index:" << nodes[i]->bucket_index <<  std::endl;
    }
    std::cout << "all node size(include self) " << nodes.size() + 1  << std::endl;
}

void ElectPerf::PrintRoutingTableAll(uint64_t service_type) {
    RoutingTablePtr routing_table = wrouter::GetRoutingTable(service_type);
    if (!routing_table) {
        TOP_ERROR("routing table not registered[%llu]", service_type);
        return;
    }

    LocalNodeInfoPtr local_node = routing_table->get_local_node_info();
    if (!local_node) {
        return;
    }

    std::cout << HexEncode(local_node->id()) << std::endl;
    PrintRoutingTable(routing_table);
}

void ElectPerf::TestLayerdBroadcast2(
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
        uint32_t b_right_overlap) {
    transport::protobuf::RoutingMessage pbft_message;
    auto routing_table = wrouter::GetRoutingTable(kRoot, true);
    if (!routing_table) {
        TOP_WARN("routing table not exists.");
        return;
    }

    auto node_ptr = routing_table->GetRandomNode();
    if (!node_ptr) {
        TOP_WARN("no node here.");
        return;
    }
    routing_table->SetFreqMessage(pbft_message);
	pbft_message.set_is_root(true);
    pbft_message.set_des_node_id(node_ptr->node_id);
    pbft_message.set_type(kBroadcastPerformaceTest);
    pbft_message.set_id(CallbackManager::MessageId());
    pbft_message.set_broadcast(true);
    pbft_message.set_xid(global_xid->Get());
    auto gossip = pbft_message.mutable_gossip();

    gossip->set_neighber_count(h_neighbors_num);
    gossip->set_stop_times(h_stop_times);
    gossip->set_gossip_type(h_gossip_type);
    gossip->set_max_hop_num(h_max_hop_num);
    gossip->set_evil_rate(h_evil_rate);
    gossip->set_switch_layer_hop_num(h_layer_switch_hop_num);
    gossip->set_left_overlap(h_left_overlap);
    gossip->set_right_overlap(h_right_overlap);


    std::string block;  // = std::string(h_packet_len, 'a');  // RandomString(h_packet_len);
	const uint32_t random_len1 = h_packet_len; // top::base::xtime_utl::get_fast_randomu() % 4192;
	for (uint32_t j = 0; j < random_len1; ++j) //avg 2048 bytes per packet
	{
		int8_t random_seed1 = (int8_t)(top::base::xtime_utl::get_fast_randomu());
		if(random_seed1 < 0)
			random_seed1 = -random_seed1;
		if(random_seed1 <= 33)
			random_seed1 += 33;
		block.push_back(random_seed1);
	}	
    
    uint256_t hash = utl::xsha3_256_t::digest(block.c_str(), block.size());
    gossip->set_header_hash(std::string((char*)hash.data(), hash.size()));  // NOLINT

    uint64_t tt = GetCurrentTimeMicSec();
    std::cout << "pbft_message ready:" << tt << ",msg_type:" << kBroadcastPerformaceTest<< std::endl;
	TOP_INFO("TestLayerdBroadcast2, send header:%d,%d", h_backup, h_neighbors_num);
    // send header first
    for (uint32_t i = 0; i < h_backup; ++i) {
        auto node_ptr = routing_table->GetRandomNode();
        if (!node_ptr) {
            continue;
        }
        pbft_message.set_des_node_id(node_ptr->node_id);
        wrouter::Wrouter::Instance()->send(pbft_message);
    }
    tt = GetCurrentTimeMicSec();
	TOP_INFO("TestLayerdBroadcast2, send header ok");
    std::cout << "header finished:" << tt << std::endl;
    //std::cout << "header broadcast finished" << std::endl;

    transport::protobuf::RoutingMessage pbft_message_block(pbft_message);
    top::transport::protobuf::GossipParams* gossip_block = pbft_message_block.mutable_gossip();
    gossip_block->set_block(block);

    gossip_block->set_neighber_count(b_neighbors_num);
    gossip_block->set_stop_times(b_stop_times);
    gossip_block->set_gossip_type(b_gossip_type);
    gossip_block->set_max_hop_num(b_max_hop_num);
    gossip_block->set_evil_rate(b_evil_rate);
    gossip_block->set_switch_layer_hop_num(b_layer_switch_hop_num);
    gossip_block->set_left_overlap(b_left_overlap);
    gossip_block->set_right_overlap(b_right_overlap);
	gossip_block->clear_msg_hash();

    // different from header id
    pbft_message_block.set_id(CallbackManager::MessageId());
	TOP_INFO("TestLayerdBroadcast2, send block");

    for (uint32_t i = 0; i < b_backup; ++i) {
        auto node_ptr = routing_table->GetRandomNode();
        if (!node_ptr) {
            continue;
        }
        pbft_message_block.set_des_node_id(node_ptr->node_id);
        wrouter::Wrouter::Instance()->send(pbft_message_block);
    }
    //std::cout << "block broadcast finished" << std::endl;
    tt = GetCurrentTimeMicSec();
	TOP_INFO("TestLayerdBroadcast2, send block ok");
    std::cout << "block finished:" << tt << std::endl;
}

// Entire network broadcast
void ElectPerf::TestChainTrade(
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
    transport::protobuf::RoutingMessage message;
    auto routing_table = wrouter::GetRoutingTable(kRoot,true);
    if (!routing_table) {
        TOP_WARN("kRoot routing table not exists.");
        return;
    }
	std::cout<<"num:"<<test_num<<",len:"<<test_len<<",type:"<<gossip_type<<std::endl;

	std::string test_raw_data;
	for(uint32_t j = 0; j < test_len; ++j) //avg 2048 bytes per packet
	{
		int8_t random_seed1 = (int8_t)(top::base::xtime_utl::get_fast_randomu());
		if(random_seed1 < 0)
			random_seed1 = -random_seed1;
		if(random_seed1 <= 33)
			random_seed1 += 33;
		test_raw_data.push_back(random_seed1);
	}	

	
    message.set_broadcast(true);
    routing_table->SetFreqMessage(message);
    auto des_kad_key = std::make_shared<base::ChainKadmliaKey>(); //anything
    message.set_des_node_id(des_kad_key->Get());
    message.set_type(kTestChainTrade);
    message.set_id(CallbackManager::MessageId());
    message.set_data(test_raw_data);
    TOP_DEBUG("test chain trade msg(id:%d)", message.id());
    message.set_xid(global_xid->Get());
    message.set_is_root(true); // Entire network broadcast

    auto gossip = message.mutable_gossip();

    gossip->set_neighber_count(neighbors_num);
    gossip->set_stop_times(stop_times);
    gossip->set_gossip_type(gossip_type);
    gossip->set_max_hop_num(max_hop_num);
    gossip->set_evil_rate(evil_rate);
    gossip->set_switch_layer_hop_num(layer_switch_hop_num);
    gossip->set_left_overlap(left_overlap);
    gossip->set_right_overlap(right_overlap);
    gossip->set_ign_bloomfilter_level(0);


    std::string data;
    if (!message.SerializeToString(&data)) {
        TOP_WARN("wrouter message SerializeToString failed");
        return;
    }

//    uint8_t local_buf[kUdpPacketBufferSize];
    static std::atomic<uint32_t> total_send_count(0);
    uint32_t send_count = 0;

//    _xip2_header header;
//    memset(&header, 0, sizeof(header));
//    std::string xheader((const char*)&header, enum_xip2_header_len);
//    std::string xdata = xheader + data;
//    std::cout << "msg.type:" << message.type() << " packet size:" << xdata.size() << std::endl;
    uint32_t looop = test_num * backup;

    uint64_t start2 = GetCurrentTimeMsec();
//    base::xpacket_t packet(base::xcontext_t::instance(), local_buf, sizeof(local_buf), 0, false);
    for (uint32_t n = 0; n < looop ; ++n) {
        message.set_id(CallbackManager::MessageId());
        message.clear_bloomfilter();
        uint32_t msg_hash =
            base::xhash32_t::digest(message.xid() + std::to_string(message.id()) + message.data());
        message.mutable_gossip()->set_msg_hash(msg_hash);
        TOP_WARN("send testchaintradehash:%u", msg_hash);
//        printf("send testchaintradehash:%u", msg_hash);
        //uint64_t t = GetCurrentTimeMsec();
        //message.set_src_service_type(t);
//        message.SerializeToString(&data);
//        xdata = xheader + data;

//        packet.reset();
//        packet.get_body().push_back((uint8_t*)xdata.data(), xdata.size());
        wrouter::Wrouter::Instance()->send(message);
        ++send_count;
    }
    total_send_count += send_count;
    auto use_time_ms = double(GetCurrentTimeMsec() - start2) / 1000.0;
    std::cout << "send " << send_count << " use time: " << use_time_ms
        << " sec. QPS: " << (uint32_t)((double)send_count / use_time_ms) * neighbors_num
        << " total_send:" << total_send_count << std::endl;
}

// Super node broadcast
void ElectPerf::TestSuperBroadcast(
        uint32_t test_num,
        uint32_t test_len,
        uint32_t backup,
        uint32_t neighbors_num,
        uint32_t stop_times,
        uint32_t max_hop_num,
        uint32_t evil_rate,
        std::string & src_node_id) {
    transport::protobuf::RoutingMessage message;
    auto routing_table = wrouter::GetRoutingTable(kRoot,true);
    if (!routing_table) {
        TOP_WARN("kRoot routing table not exists.");
        return;
    }

	std::string test_raw_data;
	for(uint32_t j = 0; j < test_len; ++j) //avg 2048 bytes per packet
	{
		int8_t random_seed1 = (int8_t)(top::base::xtime_utl::get_fast_randomu());
		if(random_seed1 < 0)
			random_seed1 = -random_seed1;
		if(random_seed1 <= 33)
			random_seed1 += 33;
		test_raw_data.push_back(random_seed1);
	}	

	message.set_broadcast(true);
    routing_table->SetFreqMessage(message);
    auto des_kad_key = std::make_shared<base::ChainKadmliaKey>(); //anything

    message.set_src_node_id(HexDecode(src_node_id));

    message.set_des_node_id(des_kad_key->Get());
    message.set_type(kTestChainTrade);
    message.set_id(CallbackManager::MessageId());
    message.set_data(test_raw_data);
    TOP_DEBUG("test super broadcast msg(id:%d)", message.id());
    message.set_xid(global_xid->Get());
    message.set_is_root(true); // Entire network broadcast

    auto gossip = message.mutable_gossip();

    gossip->set_neighber_count(neighbors_num);
    gossip->set_stop_times(stop_times);
    gossip->set_gossip_type(7); 
    gossip->set_max_hop_num(max_hop_num);
    gossip->set_evil_rate(evil_rate);
    gossip->set_ign_bloomfilter_level(0);
    gossip->set_switch_layer_hop_num(3);
    gossip->set_left_overlap(0);
    gossip->set_right_overlap(10);

    std::string data;
    if (!message.SerializeToString(&data)) {
        TOP_WARN("wrouter message SerializeToString failed");
        return;
    }

    uint32_t looop = test_num * backup;

    uint64_t start2 = GetCurrentTimeMsec();
    
    for (uint32_t n = 0; n < looop ; ++n) {
        message.set_id(CallbackManager::MessageId());
        message.clear_bloomfilter();
        uint32_t msg_hash =
            base::xhash32_t::digest(message.xid() + std::to_string(message.id()) + message.data());
        message.mutable_gossip()->set_msg_hash(msg_hash);
        TOP_WARN("send testchaintradehash:%u", msg_hash);

        wrouter::Wrouter::Instance()->send(message);

        auto use_time_ms = double(GetCurrentTimeMsec() - start2) / 1000.0;
    std::cout 
    << "send" 
    << " time:"<< use_time_ms 
    << " msgid:" << message.id() 
    << " msg_hash:" << msg_hash 
    << " msg_len:" << test_len
    << std::endl;
    }

}

}  // namespace elect

}  // namespace top
