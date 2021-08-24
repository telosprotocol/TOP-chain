#include "xelect_net/include/multilayer_network_chain_query.h"

#include "xgossip/include/gossip_utils.h"
#include "xwrouter/multi_routing/multi_routing.h"
#include "xwrouter/multi_routing/service_node_cache.h"
#include "xwrouter/multi_routing/small_net_cache.h"
#include "xwrouter/xwrouter.h"

#include <sys/utsname.h>

namespace top {

namespace elect {

bool MultilayerNetworkChainQuery::Joined() {
    auto root_ptr = wrouter::MultiRouting::Instance()->GetRootRoutingTable();
    if (!root_ptr) {
        return false;
    }
    return root_ptr->IsJoined();
}

std::string MultilayerNetworkChainQuery::Peers() {
    std::string result;

    auto rt = wrouter::MultiRouting::Instance()->GetRootRoutingTable();
    if (!rt) {
        result = "peers number: max 256, now 0";
        return result;
    }

    auto nodes = rt->nodes();
    if (nodes.empty()) {
        result = "peers number: max 256, now 0";
        return result;
    }

    result = "peers number: max 256, now:" + std::to_string(nodes.size()) + "\n";
    for (uint32_t i = 0; i < nodes.size(); ++i) {
        auto n = nodes[i];
        std::string ninfo = base::StringUtil::str_fmt("Peer#%3d: endpoints %s:%u; Connection quality good\n", i + 1, n->public_ip.c_str(), n->public_port);
        result += ninfo;
    }

    result += "\n";
    return result;
}

std::string MultilayerNetworkChainQuery::P2pAddr() {
    assert(global_xid);
    auto nodeid = global_xid->Get();

    auto root_ptr = wrouter::MultiRouting::Instance()->GetRootRoutingTable();
    if (!root_ptr) {
        return "";
    }

    auto ip = root_ptr->get_local_node_info()->public_ip();
    auto port = root_ptr->get_local_node_info()->public_port();
    if (port == 0) {
        port = root_ptr->get_local_node_info()->local_port();
        ip = root_ptr->get_local_node_info()->local_ip();
    }

    auto tnode = "tnode://" + nodeid + "@" + ip + ":" + std::to_string(port);
    return tnode;
}

#ifdef DEBUG
uint32_t MultilayerNetworkChainQuery::Broadcast(uint32_t msg_size, uint32_t count) {
    transport::protobuf::RoutingMessage message;
    message.set_broadcast(true);
    message.set_priority(enum_xpacket_priority_type_critical);
    message.set_is_root(true);

    auto root_ptr = wrouter::MultiRouting::Instance()->GetRootRoutingTable();
    if (!root_ptr) {
        return 0;
    }
    message.set_src_node_id(root_ptr->get_local_node_info()->kad_key());
    message.set_des_node_id(root_ptr->get_local_node_info()->kad_key());
    message.set_type(kElectVhostRumorGossipMessage);
    message.set_id(kadmlia::CallbackManager::MessageId());
    message.set_data(RandomString(msg_size));

    auto gossip_block = message.mutable_gossip();
    gossip_block->set_neighber_count(4);
    gossip_block->set_stop_times(gossip::kGossipBloomfilter);
    gossip_block->set_max_hop_num(20);

    gossip_block->set_block(message.data());
    uint32_t vhash = base::xhash32_t::digest(message.data());
    std::string header_hash = std::to_string(vhash);
    gossip_block->set_header_hash(header_hash);

    uint32_t sus = 0;
    for (uint32_t i = 0; i < count; ++i) {
        transport::protobuf::RoutingMessage tmp_message(message);
        tmp_message.set_data(RandomString(msg_size));
        auto gossip_block = tmp_message.mutable_gossip();
        uint32_t vhash = base::xhash32_t::digest(tmp_message.data());
        std::string header_hash = std::to_string(vhash);
        gossip_block->set_header_hash(header_hash);
        tmp_message.set_id(kadmlia::CallbackManager::MessageId());

        if (wrouter::Wrouter::Instance()->send(tmp_message) == 0) {
            ++sus;
        }
    }
    return sus;
}
#endif
}  // end namespace elect

}  // end namespace top
