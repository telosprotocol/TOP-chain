#include "xelect_net/include/elect_vhost.h"

#include "xcommon/xip.h"
#include "xconfig/xconfig_register.h"
#include "xconfig/xpredefined_configurations.h"
#include "xelect_net/include/elect_uitils.h"
#include "xelect_net/proto/elect_net.pb.h"
#include "xgossip/include/gossip_utils.h"
#include "xmetrics/xmetrics.h"
#include "xpbase/base/top_log.h"
#include "xpbase/base/top_utils.h"
#include "xsyncbase/xmessage_ids.h"
#include "xtransport/udp_transport/transport_util.h"
#include "xvnetwork/xcodec/xmsgpack/xvnetwork_message_codec.hpp"
#include "xvnetwork/xvnetwork_message.h"
#include "xwrouter/multi_routing/multi_routing.h"
#include "xwrouter/register_message_handler.h"
#include "xwrouter/xwrouter.h"

#include <cinttypes>
#include <memory>

static const uint32_t kBroadcastTypeNodesSize = 10;

namespace top {

using namespace vnetwork;
using namespace kadmlia;

namespace elect {

EcVHost::EcVHost(const uint32_t & xnetwork_id, const EcNetcardPtr & ec_netcard, common::xnode_id_t const & node_id)
  : elect::xnetwork_driver_face_t(), xnetwork_id_(xnetwork_id), ec_netcard_(ec_netcard), m_node_id_{node_id} {
}

common::xnode_id_t const & EcVHost::host_node_id() const noexcept {
    return m_node_id_;
}

// network::xnode_t EcVHost::host_node() const noexcept {
//     return {{}, {"", 999}};
// }
#if 0
base::KadmliaKeyPtr directly_address(vnetwork::xvnode_address_t const & address) {
    common::xip2_t xip{address.network_id(), address.zone_id(), address.cluster_id(), address.group_id(), address.slot_id(), address.group_size(), address.election_round().value()};
    base::KadmliaKeyPtr kad_key_ptr = base::GetKadmliaKey(xip);

    xdbg("adapt raw p2p_addr: %s platform_addr: %s service_type: %lld", address.to_string().c_str(), kad_key_ptr->Get().c_str(), kad_key_ptr->GetServiceType().value());
    return kad_key_ptr;
}

base::KadmliaKeyPtr adapt_address(const vnetwork::xvnode_address_t & address) {
    if (common::broadcast(address.network_id()) || common::broadcast(address.zone_id())) {
        return base::GetRootKadmliaKey(global_node_id);
    }

    common::xip2_t xip{address.network_id(), address.zone_id(), address.cluster_id(), address.group_id(), address.group_size(), address.election_round().value()};

    base::KadmliaKeyPtr kad_key_ptr;
    if (((xip.network_id() & common::xbroadcast_id_t::network) == common::xbroadcast_id_t::network) ||
        ((xip.zone_id() & common::xbroadcast_id_t::zone) == common::xbroadcast_id_t::zone)) {
        kad_key_ptr = base::GetRootKadmliaKey(global_node_id);
    } else {
        kad_key_ptr = base::GetKadmliaKey(xip);
    }

    xdbg("adapt raw p2p_addr: %s platform_addr: %s service_type: %lld", address.to_string().c_str(), kad_key_ptr->Get().c_str(), kad_key_ptr->GetServiceType().value());
    return kad_key_ptr;
}

base::KadmliaKeyPtr adapt_address(const common::xsharding_info_t & address) {
    if (common::broadcast(address.network_id()) || common::broadcast(address.zone_id())) {
        return base::GetRootKadmliaKey(global_node_id);
    }
    common::xip2_t xip{address.network_id(), address.zone_id(), address.cluster_id(), address.group_id()};

    base::KadmliaKeyPtr kad_key_ptr;
    if (((xip.network_id() & common::xbroadcast_id_t::network) == common::xbroadcast_id_t::network) ||
        ((xip.zone_id() & common::xbroadcast_id_t::zone) == common::xbroadcast_id_t::zone)) {
        kad_key_ptr = base::GetRootKadmliaKey(global_node_id);
    } else {
        kad_key_ptr = base::GetKadmliaKey(xip);
    }

    xdbg("adapt raw p2p_addr: %s platform_addr: %s service_type: %lld", address.to_string().c_str(), kad_key_ptr->Get().c_str(), kad_key_ptr->GetServiceType().value());
    return kad_key_ptr;
}

bool EcVHost::SyncMessageWhenStart(const vnetwork::xvnode_address_t & send_address,
                                   const vnetwork::xvnode_address_t & recv_address,
                                   const common::xmessage_id_t & message_type) const {
    if (send_address.network_id() == common::xnetwork_id_t{top::config::to_chainid(XGET_CONFIG(chain_name))} && send_address.zone_id() == common::xfrozen_zone_id &&
        send_address.cluster_id() == common::xdefault_cluster_id && send_address.group_id() == common::xdefault_group_id) {
        if (message_type == sync::xmessage_id_sync_frozen_gossip || message_type == sync::xmessage_id_sync_get_blocks || message_type == sync::xmessage_id_sync_blocks ||
            message_type == sync::xmessage_id_sync_frozen_broadcast_chain_state || message_type == sync::xmessage_id_sync_frozen_response_chain_state) {
            TOP_DEBUG("found static xip for sync");
            return true;
        }
    }
    return false;
}
#endif
void EcVHost::send_to(common::xip2_t const & src, common::xip2_t const & dst, xbyte_buffer_t const & byte_message, std::error_code & ec) const {
    assert((dst.network_id() & common::xbroadcast_id_t::network) != common::xbroadcast_id_t::network);
    assert((dst.zone_id() & common::xbroadcast_id_t::zone) != common::xbroadcast_id_t::zone);
    assert((dst.cluster_id() & common::xbroadcast_id_t::cluster) != common::xbroadcast_id_t::cluster);
    assert((dst.group_id() & common::xbroadcast_id_t::group) != common::xbroadcast_id_t::group);

    assert(dst.zone_id()!= common::xfrozen_zone_id);

    base::KadmliaKeyPtr send_kad_key = base::GetKadmliaKey(src);
    base::KadmliaKeyPtr recv_kad_key = base::GetKadmliaKey(dst);
    xinfo("[EcVHost][send] src_xip2:%s dst_xip2:%s src_key:%s recv_key:%s",
          src.to_string().c_str(),
          dst.to_string().c_str(),
          send_kad_key->Get().c_str(),
          recv_kad_key->Get().c_str());
    ec_netcard_->send_to(send_kad_key, recv_kad_key, byte_message, ec);
}

void EcVHost::send_to_through_root(common::xip2_t const & src, common::xnode_id_t const & dst_node_id, xbyte_buffer_t const & byte_message, std::error_code & ec) const {
    assert(src.zone_id() == common::xfrozen_zone_id);

    auto kroot_rt = wrouter::MultiRouting::Instance()->GetRootRoutingTable();
    if (!kroot_rt || kroot_rt->nodes_size() == 0) {
        TOP_WARN("network not joined, send failed, try again ...");
        return;
    }
    base::KadmliaKeyPtr send_kad_key = kroot_rt->get_local_node_info()->kadmlia_key();
    base::KadmliaKeyPtr recv_kad_key = nullptr;
    if (dst_node_id.length()) {
        recv_kad_key = base::GetRootKadmliaKey(dst_node_id.value());
    } else {
        auto recv_node = kroot_rt->GetRandomNode();
        recv_kad_key = base::GetKadmliaKey(recv_node->node_id);
    }
    xinfo("[EcVHost][send_to_through_root] src_xip2:%s dst_node_id:%s src_key:%s recv_key:%s",
          src.to_string().c_str(),
          dst_node_id.value().c_str(),
          send_kad_key->Get().c_str(),
          recv_kad_key->Get().c_str());
    ec_netcard_->send_to(send_kad_key, recv_kad_key, byte_message, ec);
}

void EcVHost::spread_rumor(common::xip2_t const & src, common::xip2_t const & dst, xbyte_buffer_t const & byte_message, std::error_code & ec) const {
    assert((dst.network_id() & common::xbroadcast_id_t::network) != common::xbroadcast_id_t::network);
    assert((dst.zone_id() & common::xbroadcast_id_t::zone) != common::xbroadcast_id_t::zone);
    assert((dst.cluster_id() & common::xbroadcast_id_t::cluster) != common::xbroadcast_id_t::cluster);
    assert((dst.group_id() & common::xbroadcast_id_t::group) != common::xbroadcast_id_t::group);

    assert(dst.zone_id()!= common::xfrozen_zone_id);

    base::KadmliaKeyPtr send_kad_key = base::GetKadmliaKey(src);
    base::KadmliaKeyPtr recv_kad_key = base::GetKadmliaKey(dst);
    xinfo("[EcVHost][spread_rumor] src_xip2:%s dst_xip2:%s src_key:%s recv_key:%s",
          src.to_string().c_str(),
          dst.to_string().c_str(),
          send_kad_key->Get().c_str(),
          recv_kad_key->Get().c_str());
    ec_netcard_->spread_rumor(send_kad_key, recv_kad_key, byte_message, ec);
}

void EcVHost::broadcast(common::xip2_t const & src, xbyte_buffer_t const & byte_message, std::error_code & ec) const {
    base::KadmliaKeyPtr send_kad_key = base::GetKadmliaKey(src);
    base::KadmliaKeyPtr recv_kad_key = base::GetRootKadmliaKey(global_node_id);
    // base::KadmliaKeyPtr recv_kad_key = base::GetKadmliaKey(dst);
    xinfo("[EcVHost][broadcast] src_xip2:%s src_key:%s recv_key:%s", src.to_string().c_str(), send_kad_key->Get().c_str(), recv_kad_key->Get().c_str());
    ec_netcard_->broadcast(send_kad_key, recv_kad_key, byte_message, ec);
}
#if 0
void EcVHost::send_to(common::xnode_id_t const & node_id, xbyte_buffer_t const & bytes_message, network::xtransmission_property_t const & transmission_property) const {
    auto new_hash_val = base::xhash32_t::digest(std::string((char *)bytes_message.data(), bytes_message.size()));
    auto vnetwork_message = top::codec::msgpack_decode<vnetwork::xvnetwork_message_t>(bytes_message);

    xdbg("[kadbridge] send to %s [hash: %u] [hash x64 %" PRIx64 "] msgid:%d",
         node_id.to_string().c_str(),
         new_hash_val,
         vnetwork_message.message().hash(),
         vnetwork_message.message().id());

    base::KadmliaKeyPtr send_kad_key = nullptr;
    base::KadmliaKeyPtr recv_kad_key = nullptr;

    // specially for sync module when node start
    if (SyncMessageWhenStart(vnetwork_message.sender(), vnetwork_message.receiver(), vnetwork_message.message_id())) {
        auto kroot_rt = wrouter::MultiRouting::Instance()->GetRootRoutingTable();
        if (!kroot_rt || kroot_rt->nodes_size() == 0) {
            TOP_WARN("network not joined, send failed, try again ...");
            return;
        }
        send_kad_key = kroot_rt->get_local_node_info()->kadmlia_key();
        if (node_id.length() == 0) {
            // usually sync request
            auto recv_node = kroot_rt->GetRandomNode();
            recv_kad_key = base::GetKadmliaKey(recv_node->node_id);
            TOP_DEBUG("static xip request");
        } else {
            // usually sync response
            recv_kad_key = base::GetRootKadmliaKey(node_id.to_string());
            TOP_DEBUG("static xip response");
        }
    } else {
        send_kad_key = directly_address(vnetwork_message.sender());
        recv_kad_key = directly_address(vnetwork_message.receiver());
    }

    auto msg = elect::xelect_message_t(bytes_message, vnetwork_message.message_id());
    assert(false);
    // ec_netcard_->send(send_kad_key, recv_kad_key, msg, false);
}

void EcVHost::spread_rumor(xbyte_buffer_t const & rumor) const {
    auto vnetwork_message = top::codec::msgpack_decode<vnetwork::xvnetwork_message_t>(rumor);
    auto hash_val = base::xhash32_t::digest(std::string((char *)rumor.data(), rumor.size()));
    xdbg("[kadbridge] spread_rumor to all [hash: %u] [hash x64 %" PRIx64 "] msgid:%x", hash_val, vnetwork_message.message().hash(), vnetwork_message.message().id());

    // specially for sync module when node start
    if (SyncMessageWhenStart(vnetwork_message.sender(), vnetwork_message.receiver(), vnetwork_message.message_id())) {
        auto kroot_rt = wrouter::MultiRouting::Instance()->GetRootRoutingTable();
        if (!kroot_rt || kroot_rt->nodes_size() == 0) {
            TOP_WARN("network not joined, send failed, try again ...");
            return;
        }
        auto send_kad_key = kroot_rt->get_local_node_info()->kadmlia_key();
        // usually sync request
        auto recv_node = kroot_rt->GetRandomNode();
        auto recv_kad_key = base::GetKadmliaKey(recv_node->node_id);
        TOP_DEBUG("static xip request");
        auto msg = elect::xelect_message_t(rumor, vnetwork_message.message_id());
        assert(false);
        // ec_netcard_->send(send_kad_key, recv_kad_key, msg, false);  // using p2p
        return;
    }

    auto recv_kad_key = adapt_address(vnetwork_message.receiver());
    auto send_kad_key = adapt_address(vnetwork_message.sender());
    auto msg = elect::xelect_message_t(rumor, vnetwork_message.message_id());
    assert(false);
    // ec_netcard_->send(send_kad_key, recv_kad_key, msg, true);
}

void EcVHost::forward_broadcast(const common::xsharding_info_t & shardInfo, common::xnode_type_t node_type, xbyte_buffer_t const & byte_msg) const {
    auto vnetwork_message = top::codec::msgpack_decode<vnetwork::xvnetwork_message_t>(byte_msg);
    auto new_hash_val = base::xhash32_t::digest(std::string((char *)byte_msg.data(), byte_msg.size()));
    auto recv_kad_key = adapt_address(shardInfo);
    auto send_kad_key = adapt_address(vnetwork_message.sender());
    xdbg("[kadbridge] forward to:%s [hash:%u] [vnetwork hash:%" PRIx64 "] msgid:%x",
         ((send_kad_key->Get())).c_str(),
         new_hash_val,
         vnetwork_message.hash(),
         vnetwork_message.message().id());

    auto msg = elect::xelect_message_t(byte_msg, vnetwork_message.message_id());
    assert(false);
    // ec_netcard_->send(send_kad_key, recv_kad_key, msg, true);
}

void EcVHost::spread_rumor(const common::xsharding_info_t & shardInfo, xbyte_buffer_t const & rumor) const {
    auto vnetwork_message = top::codec::msgpack_decode<vnetwork::xvnetwork_message_t>(rumor);
    auto hash_val = base::xhash32_t::digest(std::string((char *)rumor.data(), rumor.size()));
    xdbg("[kadbridge] spread_rumor to:%s [hash: %u] [vnetwork hash:%" PRIx64 "] msgid:%x",
         shardInfo.to_string().c_str(),
         hash_val,
         vnetwork_message.hash(),
         vnetwork_message.message().id());

    // specially for sync module when node start
    if (SyncMessageWhenStart(vnetwork_message.sender(), vnetwork_message.receiver(), vnetwork_message.message_id())) {
        auto kroot_rt = wrouter::MultiRouting::Instance()->GetRootRoutingTable();
        if (!kroot_rt || kroot_rt->nodes_size() == 0) {
            TOP_WARN("network not joined, send failed, try again ...");
            return;
        }
        auto send_kad_key = kroot_rt->get_local_node_info()->kadmlia_key();
        // usually sync request
        auto recv_node = kroot_rt->GetRandomNode();
        auto recv_kad_key = base::GetKadmliaKey(recv_node->node_id);
        TOP_DEBUG("static xip request");
        auto msg = elect::xelect_message_t(rumor, vnetwork_message.message_id());
        // ec_netcard_->send(send_kad_key, recv_kad_key, msg, false);  // using p2p
        assert(false);
        return;
    }

    auto recv_kad_key = adapt_address(shardInfo);
    auto send_kad_key = adapt_address(vnetwork_message.sender());
    auto msg = elect::xelect_message_t(rumor, vnetwork_message.message_id());
    assert(false);
    // ec_netcard_->send(send_kad_key, recv_kad_key, msg, true);
}

bool EcVHost::p2p_bootstrap(std::vector<network::xdht_node_t> const & seeds) const {
    return true;
}

void EcVHost::direct_send_to(network::xnode_t const & to, xbyte_buffer_t verification_data, network::xtransmission_property_t const & transmission_property) {
    xdbg("[kadbridge] direct_send_to");
}

std::vector<common::xnode_id_t> EcVHost::neighbors() const {
    std::vector<common::xnode_id_t> empty;
    return empty;
}

std::size_t EcVHost::neighbor_size_upper_limit() const noexcept {
    return 256;
}

network::p2p::xdht_host_face_t const & EcVHost::dht_host() const noexcept {
    static network::p2p::xdht_host_t shost(m_node_id_, nullptr, nullptr);
    return shost;
}
#endif
void EcVHost::register_message_ready_notify(network::xnetwork_message_ready_callback_t cb) noexcept {
    assert(cb != nullptr);
    ec_netcard_->register_message_ready_notify(cb, xnetwork_id_);
}

void EcVHost::unregister_message_ready_notify() {
    ec_netcard_->unregister_message_ready_notify(xnetwork_id_);
}

}  // namespace elect

}  // namespace top
