#include "xelect_net/include/multilayer_network_chain_query.h"

#include <sys/utsname.h>

#include "xwrouter/register_routing_table.h"
#include "xwrouter/xwrouter.h"
#include "xgossip/include/gossip_utils.h"
#include "xwrouter/multi_routing/small_net_cache.h"
#include "xwrouter/multi_routing/service_node_cache.h"

namespace top {

namespace elect {

bool MultilayerNetworkChainQuery::Joined() {
    auto root_ptr = wrouter::GetRoutingTable(kRoot, true);
    if (!root_ptr) {
        return false;
    }
    return root_ptr->IsJoined();
}

uint32_t MultilayerNetworkChainQuery::PeerCount() {
    auto root_ptr = wrouter::GetRoutingTable(kRoot, true);
    if (!root_ptr) {
        return 0;
    }
    return root_ptr->nodes_size();

}

uint32_t MultilayerNetworkChainQuery::ChainId() {
    std::vector<kadmlia::RoutingTablePtr> vec_rt;
    wrouter::GetAllRegisterRoutingTable(vec_rt);

    for (uint32_t i = 0; i < vec_rt.size(); ++i) {
        auto rt = vec_rt[i];
        auto kad_key = rt->get_local_node_info()->kadmlia_key();
        return kad_key->xnetwork_id();
    }
    return -1;
}

uint32_t MultilayerNetworkChainQuery::MaxPeers() {
    // TODO(smaug) just return 256, in the fact, i don't know how many peers
    return 256;
}

std::string MultilayerNetworkChainQuery::Peers() {
    std::string result;


    auto rt = wrouter::GetRoutingTable(kRoot, true);
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
        std::string ninfo = base::StringUtil::str_fmt("Peer#%3d: endpoints %s:%u; Connection quality good\n",
                i + 1,
                n->public_ip.c_str(),
                n->public_port);
        result += ninfo;
    }

    result += "\n";
    return result;
}


std::string MultilayerNetworkChainQuery::AllPeers() {
    std::string result;
    std::vector<kadmlia::RoutingTablePtr> vec_rt;
    wrouter::GetAllRegisterRoutingTable(vec_rt);

    std::set<std::string> node_set;
    for (uint32_t i = 0; i < vec_rt.size(); ++i) {
        auto rt = vec_rt[i];
        auto nodes = rt->nodes();
        std::string rt_result;
        for (const auto& n : nodes) {
            auto sfind = node_set.find(HexEncode(n->node_id));
            if (sfind != node_set.end()) {
                continue;
            }
            node_set.insert(HexEncode(n->node_id));
            std::string ninfo = base::StringUtil::str_fmt("endpoints:%s:%u nat_type:%u service_type:%llu nodeid:%s xip:%s\n",
                    n->public_ip.c_str(),
                    n->public_port,
                    n->nat_type,
                    n->service_type,
                    HexEncode(n->node_id).c_str(),
                    HexEncode(n->xip).c_str());
            rt_result += ninfo;
        }

        result += rt_result;
    }

    std::vector<kadmlia::NodeInfoPtr> node_vec;
    wrouter::ServiceNodes::Instance()->GetAllServicesNodes(node_vec);

    for (const auto& sn : node_vec) {
        auto sfind = node_set.find(HexEncode(sn->node_id));
        if (sfind != node_set.end()) {
            continue;
        }
        node_set.insert(HexEncode(sn->node_id));
        std::string ninfo = base::StringUtil::str_fmt("endpoints:%s:%u nat_type:%u service_type:%llu node_id:%s xip:%s\n",
                sn->public_ip.c_str(),
                sn->public_port,
                sn->nat_type,
                sn->service_type,
                HexEncode(sn->node_id).c_str(),
                HexEncode(sn->xip).c_str());
        result += ninfo;
    }
    result += "\n";
    return result;
}

std::string MultilayerNetworkChainQuery::AllNodes() {
    std::string result;
    std::vector<wrouter::NetNode> node_vec;
    wrouter::SmallNetNodes::Instance()->GetAllNode(node_vec);
    for (const auto& item : node_vec) {
        std::string ninfo = base::StringUtil::str_fmt("account:%s public_key:%s xip:%s node_id:%s gid:%u version:%llu\n",
                item.m_account.c_str(),
                HexEncode(item.m_public_key).c_str(),
                HexEncode(item.m_xip.xip()).c_str(),
                HexEncode(item.m_node_id).c_str(),
                item.m_associated_gid,
                item.m_version);
        result += ninfo;
    }
    result += "\n";
    return result;
}

std::string MultilayerNetworkChainQuery::OsInfo() {
    struct utsname sysinfo;
    uname(&sysinfo);
#ifdef __MAC_PLATFORM__
    std::string result = base::StringUtil::str_fmt("Os Name:%s\nHost Name:%s\nRelease(Kernel) Version:%s\nKernel Build Timestamp:%s\nMachine Arch:%s\n\n",
            sysinfo.sysname,
            sysinfo.nodename,
            sysinfo.release,
            sysinfo.version,
            sysinfo.machine);
#else
    std::string result = base::StringUtil::str_fmt("Os Name:%s\nHost Name:%s\nRelease(Kernel) Version:%s\nKernel Build Timestamp:%s\nMachine Arch:%s\n",
            sysinfo.sysname,
            sysinfo.nodename,
            sysinfo.release,
            sysinfo.version,
            sysinfo.machine);
#endif
    return result;
}

uint32_t MultilayerNetworkChainQuery::Broadcast(uint32_t msg_size, uint32_t count) {
    transport::protobuf::RoutingMessage message;
    message.set_broadcast(true);
    message.set_priority(enum_xpacket_priority_type_critical);
    message.set_is_root(true);

    auto root_ptr = wrouter::GetRoutingTable(kRoot, true);
    if (!root_ptr) {
        return 0;
    }
    message.set_src_node_id(root_ptr->get_local_node_info()->id());
    message.set_des_node_id(root_ptr->get_local_node_info()->id());
    message.set_type(kElectVhostRumorGossipMessage);
    message.set_id(kadmlia::CallbackManager::MessageId());
    message.set_data(RandomString(msg_size));

    auto gossip_block = message.mutable_gossip();
    gossip_block->set_neighber_count(4);
    gossip_block->set_stop_times(gossip::kGossipBloomfilter);
    gossip_block->set_max_hop_num(20);
    // next version delete all `set_evil_rate`
    gossip_block->set_evil_rate(0);
    if (message.is_root()) {
        gossip_block->set_ign_bloomfilter_level(gossip::kGossipBloomfilterIgnoreLevel);
    } else {
        gossip_block->set_ign_bloomfilter_level(0);
    }
    gossip_block->set_left_overlap(10);
    gossip_block->set_right_overlap(10);
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
            ++ sus;
        }
    }
    return sus;
}

std::string MultilayerNetworkChainQuery::NetInfo() {
    std::string result;
    std::vector<kadmlia::RoutingTablePtr> vec_rt;
    wrouter::GetAllRegisterRoutingTable(vec_rt);
    for (uint32_t i = 0; i < vec_rt.size(); ++i) {
        auto rt = vec_rt[i];
        auto kad_key = rt->get_local_node_info()->kadmlia_key();

        auto xnetwork_id = kad_key->xnetwork_id();
        auto zone_id     = kad_key->zone_id();
        auto cluster_id  = kad_key->cluster_id();
        auto group_id    = kad_key->group_id();

        std::string role;
        
        if (zone_id == 0 && cluster_id == 1 && group_id >= 1 && group_id < 64) {
            role = "Audit Network";
        }
        if (zone_id == 0 && cluster_id == 1 && group_id >= 64 && group_id < 127) {
            role = "Validate Network";
        }
        if (zone_id == 1 && cluster_id == 0 && group_id == 0) {
            role = "Root-beacon Network";
        }
        if (zone_id == 2 && cluster_id == 0 && group_id == 0) {
            role = "Sub-beacon Network";
        }
        if (zone_id == 14 && cluster_id == 1 && group_id == 1) {
            role = "Archive Network";
        }
        if (zone_id == 15 && cluster_id == 1 && group_id == 1) {
            role = "Edge Network";
        }

        std::string item_info = base::StringUtil::str_fmt("xnetwork_id[%u] zone_id[%u] cluster_id[%u] group_id[%u]    %s\n",
        xnetwork_id,
        zone_id,
        cluster_id,
        group_id,
        role.c_str());

        result += item_info;
    }
    if (vec_rt.size() == 0) {
        result = "The node has not yet been elected.";
    }
    return result;
}

std::string MultilayerNetworkChainQuery::Gid() {
    assert(global_xid);
    return HexEncode(global_xid->Get());
}

std::string MultilayerNetworkChainQuery::Account() {
    assert(!global_node_id.empty());
    return global_node_id;
}

std::string MultilayerNetworkChainQuery::P2pAddr() {
    assert(global_xid);
    auto nodeid = HexEncode(global_xid->Get());

    auto root_ptr = wrouter::GetRoutingTable(kRoot, true);
    if (!root_ptr) {
        return "";
    }

    auto ip   = root_ptr->get_local_node_info()->public_ip();
    auto port = root_ptr->get_local_node_info()->public_port();
    if (port == 0) {
        port = root_ptr->get_local_node_info()->local_port();
        ip   = root_ptr->get_local_node_info()->local_ip();
    }

    // such as tnode://b4062cb0463fb6cf6576337cebff8414387ed445739e458d23bb037f2aadc6c41205147f92@127.0.0.1:30303
    auto tnode = "tnode://" + nodeid + "@" + ip + ":" + std::to_string(port);
    return tnode;
}

std::string MultilayerNetworkChainQuery::HelpInfo() {
    std::string help = "\
NAME:\n\
    net\n\n\
COMMANDS:\n\
    help                         Show a list of commands or help for one command.\n\
    Joined                       Get the result of whether the node has joined the network.\n\
    xnetworkID                   Get xnetwork ID which the node joined in.\n\
    maxpeer                      Get the count of max connect peers.\n\
    peercount                    Get the count of connecting peers.\n\
    netID                        Print network IDs which the node joined in.\n\
    osInfo                       Print OS information.\n\
    accountAddr                  Print the node account address\n\
    nodeP2PAddr                  Print the nodes's P2P ID with IP:port.\n";

    return help;
}

} // end namespace elect

} // end namespace top
