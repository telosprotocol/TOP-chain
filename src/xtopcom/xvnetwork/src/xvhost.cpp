// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xvnetwork/xvhost.h"

#include "xbase/xlog.h"
#include "xbasic/xerror/xthrow_error.h"
#include "xbasic/xmemory.hpp"
#include "xbasic/xscope_executer.h"
#include "xbasic/xthreading/xbackend_thread.hpp"
#include "xbasic/xthreading/xutility.h"
#include "xbasic/xutility.h"
#include "xcodec/xmsgpack_codec.hpp"
#include "xcommon/xaddress.h"
#include "xelection/xcache/xgroup_element.h"
#include "xelection/xdata_accessor_error.h"
#include "xmetrics/xmetrics.h"
#include "xtxpool_v2/xtxpool_face.h"
#include "xutility/xhash.h"
#include "xvnetwork/xcodec/xmsgpack/xmessage_codec.hpp"
#include "xvnetwork/xcodec/xmsgpack/xvnetwork_message_codec.hpp"
#include "xvnetwork/xmessage.h"
#include "xvnetwork/xmessage_filter_manager.h"
#include "xvnetwork/xvnetwork_driver.h"
#include "xvnetwork/xvnetwork_error.h"
#include "xvnetwork/xvnetwork_error2.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cinttypes>
#include <functional>
#include <random>
#include <sstream>
#include <unordered_set>

NS_BEG2(top, vnetwork)

xtop_vhost::xtop_vhost(observer_ptr<network::xnetwork_driver_face_t> const & network_driver,
                       observer_ptr<time::xchain_time_face_t> const & chain_timer,
                       common::xnetwork_id_t const & nid,
                       observer_ptr<election::cache::xdata_accessor_face_t> const & election_cache_data_accessor,
                       std::unique_ptr<xmessage_filter_manager_face_t> message_filter_manager_ptr)
  : base_t{nid, chain_timer, election_cache_data_accessor}, m_network_driver{network_driver} {
    assert(m_network_driver != nullptr);

    if (message_filter_manager_ptr == nullptr) {
        m_filter_manager = top::make_unique<xmessage_filter_manager_t>(make_observer<xvhost_face_t>(this), election_cache_data_accessor);
    } else {
        // mock the m_filter_manager if you do not use the default para.
        m_filter_manager = std::move(message_filter_manager_ptr);
    }
}

common::xnode_id_t const & xtop_vhost::host_node_id() const noexcept {
    assert(m_network_driver);
    return m_network_driver->host_node_id();
}

void xtop_vhost::start() {
    assert(m_network_driver);
    m_network_driver->register_message_ready_notify(std::bind(&xtop_vhost::on_network_data_ready, shared_from_this(), std::placeholders::_1, std::placeholders::_2));
    assert(!running());
    m_filter_manager->start();
    running(true);
    assert(running());

    threading::xbackend_thread::spawn([this, self = shared_from_this()] {
#if defined DEBUG
        m_vhost_thread_id = std::this_thread::get_id();
#endif
        do_handle_network_data();
    });
}

void xtop_vhost::stop() {
    m_message_queue.push(xbyte_buffer_t{});

    assert(running());
    running(false);
    assert(!running());

    assert(m_network_driver);

    m_network_driver->unregister_message_ready_notify();
}

void xtop_vhost::send(xmessage_t const & message,
                      common::xnode_address_t const & src,
                      common::xnode_address_t const & dst,
                      network::xtransmission_property_t const & transmission_property) {
    if (!running()) {
        xwarn("[vnetwork] vhost not run");
        return;
    }

    assert(!src.empty());
    assert(!src.account_address().empty());

#if defined DEBUG
    if (common::has<common::xnode_type_t::storage>(src.type())) {
        assert(src.zone_id() == common::xarchive_zone_id);
        assert(src.cluster_id() == common::xdefault_cluster_id);
        assert(src.group_id() == common::xarchive_group_id || src.group_id() == common::xfull_node_group_id);
    }

    if (dst.account_address().has_value() && common::has<common::xnode_type_t::storage>(dst.type())) {
        assert(dst.zone_id() == common::xarchive_zone_id);
        assert(dst.cluster_id() == common::xdefault_cluster_id);
        assert(dst.group_id() == common::xarchive_group_id || dst.group_id() == common::xfull_node_group_id);
    }
#endif

    try {
        if (dst.version() != src.version()) {
            xinfo("[vnetwork] dst version %s, src version %s", dst.version().to_string().c_str(), src.version().to_string().c_str());
        }

        // auto const target_vnetwork = vnetwork(xvnetwork_id_t{ m_network_info });
        // auto const target_vnetwork = vnetwork(m_network_id);
        // assert(target_vnetwork);

        if (!dst.account_address().empty()) {
            assert(m_chain_timer != nullptr);
            xvnetwork_message_t const vnetwork_message{src, dst, message, m_chain_timer->logic_time()};
            auto const bytes_message = top::codec::msgpack_encode(vnetwork_message);

            assert(m_network_driver);
            xdbg("[vnetwork] send msg from %s to %s msg hash %" PRIx64 " msg id %" PRIx32,
                 src.account_address().to_string().c_str(),
                 dst.account_address().to_string().c_str(),
                 vnetwork_message.hash(),
                 static_cast<std::uint32_t>(message.id()));
            auto new_hash_val = base::xhash32_t::digest(std::string((char *)bytes_message.data(), bytes_message.size()));
            xdbg("[vnetwork] send msg %" PRIx32 " [hash: %" PRIx64 "] [to_hash:%u]", static_cast<std::uint32_t>(message.id()), message.hash(), new_hash_val);
            #if VHOST_METRICS
            XMETRICS_COUNTER_INCREMENT("vhost_" + std::to_string(static_cast<std::uint16_t>(common::get_message_category(message.id()))) + "_out_vhost_send" +
                                           std::to_string(static_cast<std::uint32_t>(message.id())),
                                       1);
            #endif

            #if VHOST_METRICS
            XMETRICS_COUNTER_INCREMENT("vhost_" + std::to_string(static_cast<std::uint16_t>(common::get_message_category(message.id()))) + "_out_vhost_send_size" +
                                           std::to_string(static_cast<std::uint32_t>(message.id())),
                                       bytes_message.size());
            #endif

            m_network_driver->send_to(dst.account_address(), bytes_message, transmission_property);
        } else if (dst.empty()) {
            broadcast(message, src);
        } else {
            // crose broadcast is not correct by using send_to
            if (dst.cluster_address() != src.cluster_address()) {
                top::error::throw_error({ xvnetwork_errc_t::cluster_address_not_match }, "src " + src.to_string() + " dst " + dst.to_string());
            }

            broadcast(message, src);
        }
    } catch (top::error::xtop_error_t const & eh) {
        xwarn("xtop_error_t caught. cateogry: %s; exception:%s; error code: %d; error msg: %s", eh.code().category().name(), eh.what(), eh.code().value(), eh.code().message().c_str());
        throw;
    }
}

void xtop_vhost::forward_broadcast_message(xmessage_t const & message, common::xnode_address_t const & src, common::xnode_address_t const & dst) {
    assert(common::has<common::xnode_type_t::group>(dst.cluster_address().type()));
    assert(src.cluster_address() != dst.cluster_address());
    // assert(!dst.group_id().empty());

    assert(!src.empty());
    assert(!src.account_address().empty());
    assert(src.account_address() == host_node_id());
    assert(dst.account_address().empty());

    assert(message.id() != common::xmessage_id_t::invalid);

#if defined DEBUG
    if (common::has<common::xnode_type_t::storage>(src.type())) {
        assert(src.zone_id() == common::xarchive_zone_id);
        assert(src.cluster_id() == common::xdefault_cluster_id);
        assert(src.group_id() == common::xarchive_group_id || src.group_id() == common::xfull_node_group_id);
    }

    if (common::has<common::xnode_type_t::storage>(dst.type())) {
        assert(dst.zone_id() == common::xarchive_zone_id);
        assert(dst.cluster_id() == common::xdefault_cluster_id);
        assert(dst.group_id() == common::xarchive_group_id || dst.group_id() == common::xfull_node_group_id);
    }
#endif

    auto const src_type = src.type();
    auto const dst_type = dst.type();

    xinfo("[vnetwork] forward_broadcast_message from %s to %s", src.to_string().c_str(), dst.to_string().c_str());

    try {
        // auto const & src_cluster = address_cast<common::xnode_type_t::cluster>(src);
        // auto const & dst_cluster = address_cast<common::xnode_type_t::cluster>(dst);
        // auto const timer_height = m_chain_timer->logic_time();

        // auto dst_index = (src.hash() ^ dst.hash()) % target_vnodes.size();
        xvnetwork_message_t const vmsg{src, dst, message, m_chain_timer->logic_time()};
        auto const bytes_message = top::codec::msgpack_encode(vmsg);

        auto const hash_val = base::xhash64_t::digest(std::string((char *)bytes_message.data(), bytes_message.size()));
        xinfo("[vnetwork] %s forward_broadcast_message [type: %" PRIx32 "] [hash: %" PRIx64 "][to_hash:%" PRIx64 "][msgid:%" PRIx32 "][%s][%s]",
              host_node_id().to_string().c_str(),
              static_cast<std::uint32_t>(message.type()),
              message.hash(),
              hash_val,
              static_cast<std::uint32_t>(message.id()),
              src.to_string().c_str(),
              dst.to_string().c_str());

        #if VHOST_METRICS
        XMETRICS_COUNTER_INCREMENT("vhost_" + std::to_string(static_cast<std::uint16_t>(common::get_message_category(message.id()))) + "_out_vhost_forward_broadcast_message" +
                                       std::to_string(static_cast<std::uint32_t>(message.id())),
                                   1);
        #endif

        #if VHOST_METRICS
        XMETRICS_COUNTER_INCREMENT("vhost_" + std::to_string(static_cast<std::uint16_t>(common::get_message_category(message.id()))) + "_out_vhost_forward_broadcast_message_size" +
                                       std::to_string(static_cast<std::uint32_t>(message.id())),
                                   bytes_message.size());
        #endif

        on_network_data_ready(host_node_id(), bytes_message);
        if (dst.version().empty()) {
            m_network_driver->forward_broadcast(dst.cluster_address().sharding_info(), dst.type(), bytes_message);
        } else {
            m_network_driver->spread_rumor(bytes_message);
        }
        // m_network_driver->forward_broadcast(dst.cluster_address().sharding_info(), dst.type(), bytes_message);
    } catch (top::error::xtop_error_t const & eh) {
        xwarn("[vnetwork] forward_broadcast_message xtop_error_t exception caught: cateogry:%s; msg:%s; error code:%d; error msg:%s", eh.code().category().name(), eh.what(), eh.code().value(), eh.code().message().c_str());
    } catch (std::exception const & eh) {
        xwarn("[vnetwork] forward_broadcast_message std::exception caught: %s", eh.what());
    }
}

void xtop_vhost::broadcast_to_all(xmessage_t const & message, common::xnode_address_t const & src, common::xnode_address_t const & dst) {
    if (!running()) {
        xwarn("[vnetwork] vhost not run");
        return;
    }

    assert(!src.empty());
    assert(!src.account_address().empty());

#if defined DEBUG
    if (common::has<common::xnode_type_t::storage>(src.type())) {
        assert(src.zone_id() == common::xarchive_zone_id);
        assert(src.cluster_id() == common::xdefault_cluster_id);
        assert(src.group_id() == common::xarchive_group_id || src.group_id() == common::xfull_node_group_id);
    }

    if (common::has<common::xnode_type_t::storage>(dst.type())) {
        assert(dst.zone_id() == common::xarchive_zone_id);
        assert(dst.cluster_id() == common::xdefault_cluster_id);
        assert(dst.group_id() == common::xarchive_group_id || dst.group_id() == common::xfull_node_group_id);
    }
#endif

    try {
        assert(m_chain_timer != nullptr);
        xvnetwork_message_t const vnetwork_message{src, dst, message, m_chain_timer->logic_time()};
        auto const bytes_message = top::codec::msgpack_encode(vnetwork_message);

        on_network_data_ready(host_node_id(), bytes_message);  // broadcast to local vnet  // TODO remove codec part

        xdbg("[vnetwork] broadcast msg %x from:%s to:%s hash %" PRIx64, message.id(), src.to_string().c_str(), dst.to_string().c_str(), message.hash());

        #if VHOST_METRICS
        XMETRICS_COUNTER_INCREMENT("vhost_" + std::to_string(static_cast<std::uint16_t>(common::get_message_category(message.id()))) + "_out_vhost_broadcast_to_all" +
                                       std::to_string(static_cast<std::uint32_t>(message.id())),
                                   1);
        #endif
        #if VHOST_METRICS
        XMETRICS_COUNTER_INCREMENT("vhost_" + std::to_string(static_cast<std::uint16_t>(common::get_message_category(message.id()))) + "_out_vhost_broadcast_to_all_size" +
                                       std::to_string(static_cast<std::uint32_t>(message.id())),
                                   bytes_message.size());
        #endif
        assert(m_network_driver);
        m_network_driver->spread_rumor(bytes_message);
    } catch (top::error::xtop_error_t const & eh) {
        xwarn("[vnetwork] xtop_error_t exception caught: cateogry:%s; msg:%s; error code:%d; error msg:%s", eh.code().category().name(), eh.what(), eh.code().value(), eh.code().message().c_str());
        throw;
    }
}

void xtop_vhost::broadcast(xmessage_t const & message, common::xnode_address_t const & src) {
    if (!running()) {
        xwarn("[vnetwork] vhost not run");
        return;
    }

    assert(!src.empty());
    assert(!src.account_address().empty());

    auto const dst = address_cast<common::xnode_type_t::group>(src);
    assert(src.version() == dst.version());

#if defined DEBUG
    if (common::has<common::xnode_type_t::storage>(src.type())) {
        assert(src.zone_id() == common::xarchive_zone_id);
        assert(src.cluster_id() == common::xdefault_cluster_id);
        assert(src.group_id() == common::xarchive_group_id || src.group_id() == common::xfull_node_group_id);
    }

    if (common::has<common::xnode_type_t::storage>(dst.type())) {
        assert(dst.zone_id() == common::xarchive_zone_id);
        assert(dst.cluster_id() == common::xdefault_cluster_id);
        assert(dst.group_id() == common::xarchive_group_id || dst.group_id() == common::xfull_node_group_id);
    }
#endif

    assert(message.id() != common::xmessage_id_t::invalid);

    assert(m_chain_timer != nullptr);
    xvnetwork_message_t const vmsg{src, dst, message, m_chain_timer->logic_time()};
    xdbg("[vnetwork] add a rumor msg %" PRIx64 " sent from %s to %s", vmsg.hash(), src.to_string().c_str(), dst.to_string().c_str());

    auto packet_msg = top::codec::msgpack_encode(vmsg);
    auto new_hash_val = base::xhash32_t::digest(std::string((char *)packet_msg.data(), packet_msg.size()));
    xinfo("[vnetwork]xtop_vhost::broadcast [vnet hash: %" PRIx64 "] [hash: %u] [to_hash:%u]", vmsg.hash(), message.hash(), new_hash_val);

    #if VHOST_METRICS
    XMETRICS_COUNTER_INCREMENT("vhost_" + std::to_string(static_cast<std::uint16_t>(common::get_message_category(message.id()))) + "_out_vhost_broadcast" +
                                   std::to_string(static_cast<std::uint32_t>(message.id())),
                               1);
    #endif
    #if VHOST_METRICS
    XMETRICS_COUNTER_INCREMENT("vhost_" + std::to_string(static_cast<std::uint16_t>(common::get_message_category(message.id()))) + "_out_vhost_broadcast_size" +
                                   std::to_string(static_cast<std::uint32_t>(message.id())),
                               packet_msg.size());
    #endif

    // m_network_driver->spread_rumor(src.cluster_address().sharding_info(), packet_msg);
    m_network_driver->spread_rumor(packet_msg);
}

void xtop_vhost::send(common::xnode_address_t const & src, common::xip2_t const & dst, xmessage_t const & message, std::error_code & ec) {
    if (!running()) {
        ec = xvnetwork_errc2_t::vhost_not_run;
        xwarn("%s %s", ec.category().name(), ec.message().c_str());
        return;
    }

    if (common::broadcast(dst.network_id()) || common::broadcast(dst.zone_id()) || common::broadcast(dst.cluster_id()) || common::broadcast(dst.group_id()) ||
        common::broadcast(dst.slot_id())) {
        ec = xvnetwork_errc2_t::invalid_dst_address;
        xwarn("%s %s. dst address is a broadcast address %s", vnetwork_category2().name(), ec.message().c_str(), dst.to_string().c_str());
        return;
    }

    auto dst_node_id = m_election_cache_data_accessor->node_id_from(dst, ec);
    if (ec) {
        xwarn("%s ec category: %s ec msg: %s", vnetwork_category2().name(), ec.category().name(), ec.message().c_str());
        return;
    }

    auto dst_version = m_election_cache_data_accessor->version_from(dst, ec);
    if (ec) {
        xwarn("%s ec category: %s ec msg: %s", vnetwork_category2().name(), ec.category().name(), ec.message().c_str());
        return;
    }

    common::xnode_address_t to{common::xsharding_address_t{dst.network_id(), dst.zone_id(), dst.cluster_id(), dst.group_id()},
                               common::xaccount_election_address_t{dst_node_id, dst.slot_id()},
                               dst_version,
                               dst.size(),
                               dst.height()};

    xvnetwork_message_t const vmsg{src, to, message, m_chain_timer->logic_time()};
    auto bytes = codec::msgpack_encode(vmsg);
    m_network_driver->send_to(dst_node_id, bytes, {});
}

void xtop_vhost::broadcast(common::xnode_address_t const & src, common::xip2_t const & dst, xmessage_t const & message, std::error_code & ec) {
    if (!running()) {
        ec = xvnetwork_errc2_t::vhost_not_run;
        xwarn("%s %s", ec.category().name(), ec.message().c_str());
        return;
    }

    if (!common::broadcast(dst.network_id()) && !common::broadcast(dst.zone_id()) && !common::broadcast(dst.cluster_id()) && !common::broadcast(dst.group_id()) &&
        !common::broadcast(dst.slot_id())) {
        ec = xvnetwork_errc2_t::invalid_dst_address;
        xwarn("%s %s. dst address is not a broadcast address %s", ec.category().name(), ec.message().c_str(), dst.to_string().c_str());
        return;
    }

    if (src.network_id() == dst.network_id() && src.zone_id() == dst.zone_id() && src.cluster_id() == dst.cluster_id() && src.group_id() == dst.group_id()) {
        // broadcast message in the same sharding

        auto version = m_election_cache_data_accessor->version_from(dst, ec);
        if (ec) {
            xwarn("%s ec category: %s ec msg: %s", vnetwork_category2().name(), ec.category().name(), ec.message().c_str());
            version = src.version();
        }

        if (src.version() != version) {
            ec = xvnetwork_errc2_t::version_mismatch;
            xwarn("%s %s. src version %s dst version %s", ec.category().name(), ec.message().c_str(), src.version().to_string().c_str(), version.to_string().c_str());
            return;
        }

        common::xnode_address_t to{src.sharding_address(), version, dst.size(), dst.height()};

        xvnetwork_message_t const vmsg{src, to, message, m_chain_timer->logic_time()};
        auto bytes = codec::msgpack_encode(vmsg);

        auto new_hash_val = base::xhash32_t::digest(std::string((char *)bytes.data(), bytes.size()));
        xinfo("[vnetwork]xtop_vhost::broadcast [vnet hash: %" PRIx64 "] [msg hash: %u] [xxh32 to_hash:%" PRIu32 "]", vmsg.hash(), message.hash(), new_hash_val);

        #if VHOST_METRICS
        XMETRICS_COUNTER_INCREMENT("vhost_" + std::to_string(static_cast<std::uint16_t>(common::get_message_category(message.id()))) + "_out_vhost_broadcast" +
                                       std::to_string(static_cast<std::uint32_t>(message.id())),
                                   1);
        #endif
        #if VHOST_METRICS
        XMETRICS_COUNTER_INCREMENT("vhost_" + std::to_string(static_cast<std::uint16_t>(common::get_message_category(message.id()))) + "_out_vhost_broadcast_size" +
                                       std::to_string(static_cast<std::uint32_t>(message.id())),
                                   bytes.size());
        #endif
        m_network_driver->spread_rumor(/*src.sharding_address().sharding_info(),*/ bytes);
        // } else if (common::broadcast(dst.network_id())) {
        //     ec = xvnetwork_errc2_t::not_supported;
        //     xwarn("%s %s", ec.category().name(), ec.message().c_str());
    } else if (common::broadcast(dst.network_id()) || common::broadcast(dst.zone_id())) {
        // broadcast in the specified network

        common::xnode_address_t to{common::xsharding_address_t{dst.network_id(), dst.zone_id(), dst.cluster_id(), dst.group_id()}, common::xversion_t{}, dst.size(), dst.height()};

        xvnetwork_message_t const vnetwork_message{src, to, message, m_chain_timer->logic_time()};
        auto const bytes_message = top::codec::msgpack_encode(vnetwork_message);

        on_network_data_ready(host_node_id(), bytes_message);  // broadcast to local vnet  // TODO remove codec part

        xdbg("%s broadcast msg %x from:%s to:%s hash %" PRIx64, vnetwork_category2().name(), message.id(), src.to_string().c_str(), dst.to_string().c_str(), message.hash());

        #if VHOST_METRICS
        XMETRICS_COUNTER_INCREMENT("vhost_" + std::to_string(static_cast<std::uint16_t>(common::get_message_category(message.id()))) + "_out_vhost_broadcast_to_all" +
                                       std::to_string(static_cast<std::uint32_t>(message.id())),
                                   1);
        #endif
        #if VHOST_METRICS
        XMETRICS_COUNTER_INCREMENT("vhost_" + std::to_string(static_cast<std::uint16_t>(common::get_message_category(message.id()))) + "_out_vhost_broadcast_to_all_size" +
                                       std::to_string(static_cast<std::uint32_t>(message.id())),
                                   bytes_message.size());
        #endif
        assert(m_network_driver);
        m_network_driver->spread_rumor(bytes_message);
    } else if (common::broadcast(dst.slot_id())) {
        // broadcast between different shardings

        common::xnode_address_t to{common::xsharding_address_t{dst.network_id(), dst.zone_id(), dst.cluster_id(), dst.group_id()}, common::xversion_t{}, dst.size(), dst.height()};

        xvnetwork_message_t const vmsg{src, to, message, m_chain_timer->logic_time()};
        auto const bytes_message = top::codec::msgpack_encode(vmsg);

        auto const hash_val = base::xhash64_t::digest(std::string((char *)bytes_message.data(), bytes_message.size()));
        xinfo("%s %s forward_broadcast_message [type: %" PRIx32 "] [msg hash: %" PRIx64 "][bytes hash:%" PRIx64 "][msgid:%" PRIx32 "][%s][%s]",
              vnetwork_category2().name(),
              host_node_id().to_string().c_str(),
              static_cast<std::uint32_t>(message.type()),
              message.hash(),
              hash_val,
              static_cast<std::uint32_t>(message.id()),
              src.to_string().c_str(),
              to.to_string().c_str());

        #if VHOST_METRICS
        XMETRICS_COUNTER_INCREMENT("vhost_" + std::to_string(static_cast<std::uint16_t>(common::get_message_category(message.id()))) + "_out_vhost_forward_broadcast_message" +
                                       std::to_string(static_cast<std::uint32_t>(message.id())),
                                   1);
        #endif
        #if VHOST_METRICS
        XMETRICS_COUNTER_INCREMENT("vhost_" + std::to_string(static_cast<std::uint16_t>(common::get_message_category(message.id()))) + "_out_vhost_forward_broadcast_message_size" +
                                       std::to_string(static_cast<std::uint32_t>(message.id())),
                                   bytes_message.size());
        #endif
        on_network_data_ready(host_node_id(), bytes_message);

        // m_network_driver->forward_broadcast(to.cluster_address().sharding_info(), to.type(), bytes_message);
        m_network_driver->spread_rumor(bytes_message);
    } else {
        ec = xvnetwork_errc2_t::not_supported, xwarn("%s %s", ec.category().name(), ec.message().c_str());
    }
}

void xtop_vhost::on_network_data_ready(common::xnode_id_t const &, xbyte_buffer_t const & bytes) {
    #if VHOST_METRICS
        XMETRICS_COUNTER_INCREMENT("vhost_on_data_ready_called", 1);
    #endif
    try {
        if (!running()) {
            xwarn("[vnetwork] vhost not run");
            return;
        }

        #if VHOST_METRICS
        XMETRICS_COUNTER_INCREMENT("vhost_total_size_of_all_messages", bytes.size());
        #endif
        m_message_queue.push(bytes);
    } catch (std::exception const & eh) {
        xwarn("[vnetwork] std::exception exception caught: %s", eh.what());
    } catch (...) {
        xerror("[vnetwork] unknown exception caught");
    }
}

/*
 * this is the callback function ,which is called by the lower module,
 * refactor by @Charles.Liu 2020.05.08
 */
void xtop_vhost::do_handle_network_data() {
    assert(m_vhost_thread_id == std::this_thread::get_id());

#if defined DEBUG
    xscope_executer_t do_handle_network_data_exit_verifier{[this, self = shared_from_this()] { assert(!running()); }};
#endif
    while (running()) {
        try {
            auto all_byte_messages = m_message_queue.wait_and_pop_all();

            XMETRICS_FLOW_COUNT("vhost_handle_data_ready_called", all_byte_messages.size());

            XMETRICS_TIME_RECORD("vhost_handle_data_ready_called_time");

#if defined(XENABLE_VHOST_BENCHMARK)
            auto xxbegin = std::chrono::high_resolution_clock::now();
#endif
            for (auto & bytes : all_byte_messages) {
                if (!running()) {
                    xwarn("[vnetwork] vhost is not running!");
                    break;
                }

                try {
                    if (bytes.empty()) {
                        // this may be a stop notification message.
                        // anyway, for an empty message, just ignore it.
                        xwarn("[vnetwork] message byte empty!");
                        continue;
                    }

                    auto vnetwork_message = top::codec::msgpack_decode<xvnetwork_message_t>(bytes);
                    XMETRICS_GAUGE(metrics::vhost_recv_msg, 1);
                    auto const & message = vnetwork_message.message();
                    auto const & receiver = vnetwork_message.receiver();
                    auto const & sender = vnetwork_message.sender();
                    auto const msg_time = vnetwork_message.logic_time();

                    xdbg("[vnetwork] message hash: %" PRIx64 " , before filter:s&r sender is %s , receiver is %s",
                         vnetwork_message.hash(),
                         sender.to_string().c_str(),
                         receiver.to_string().c_str());

                    auto const message_category = common::get_message_category(vnetwork_message.message_id());
                    switch (message_category) {
#if defined(__clang__)
#    pragma clang diagnostic push
#    pragma clang diagnostic ignored "-Wswitch"
#elif defined(__GNUC__)
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wswitch"
#elif defined(_MSC_VER)
#    pragma warning(push, 0)
#endif
                    case xmessage_category_consensus:
                    {
                        XMETRICS_GAUGE(metrics::message_category_consensus_contains_duplicate, 1);
                        break;
                    }
                    case xmessage_category_timer:
                    {
                        XMETRICS_GAUGE(metrics::message_category_timer_contains_duplicate, 1);
                        break;
                    }
                    case xmessage_category_txpool:
                    {
                        XMETRICS_GAUGE(metrics::message_category_txpool_contains_duplicate, 1);
                        break;
                    }

                    case xmessage_category_rpc:
                    {
                        XMETRICS_GAUGE(metrics::message_category_rpc_contains_duplicate, 1);
                        break;
                    }

                    case xmessage_category_sync:
                    {
                        XMETRICS_GAUGE(metrics::message_category_sync_contains_duplicate, 1);
                        break;
                    }

                    case xmessage_block_broadcast:
                    {
                        XMETRICS_GAUGE(metrics::message_block_broadcast_contains_duplicate, 1);
                        break;
                    }
#if defined(__clang__)
#    pragma clang diagnostic pop
#elif defined(__GNUC__)
#    pragma GCC diagnostic pop
#elif defined(_MSC_VER)
#    pragma warning(pop)
#endif
                    default:
                    {
                        assert(false);
                        XMETRICS_GAUGE(metrics::message_category_unknown_contains_duplicate, 1);
                        break;
                    }
                    }
                    #if VHOST_METRICS
                    XMETRICS_COUNTER_INCREMENT("vhost_" + std::to_string(static_cast<std::uint16_t>(common::get_message_category(vnetwork_message.message().id()))) +
                                                   "_in_vhost_size" + std::to_string(static_cast<std::uint32_t>(vnetwork_message.message().id())),
                                               bytes.size());
                    #endif
                    if (vnetwork_message.message_id() != xtxpool_v2::xtxpool_msg_send_receipt &&
                        vnetwork_message.message_id() != xtxpool_v2::xtxpool_msg_recv_receipt) {
                        m_filter_manager->filt_message(vnetwork_message);
                        if (vnetwork_message.empty()) {
                            continue;
                        }
                    } else {
                        std::error_code ec{ election::xdata_accessor_errc_t::success };
                        auto const group_element = m_election_cache_data_accessor->group_element_by_logic_time(receiver.sharding_address(), msg_time, ec);
                        if (!ec) {
                            if (receiver.account_election_address().empty()) {
                                vnetwork_message.receiver(
                                    common::xnode_address_t{
                                        receiver.sharding_address(),
                                        group_element->version(),
                                        group_element->sharding_size(),
                                        group_element->associated_blk_height()
                                    });

                            } else {
                                vnetwork_message.receiver(
                                    common::xnode_address_t{
                                        receiver.sharding_address(),
                                        receiver.account_election_address(),
                                        group_element->version(),
                                        group_element->sharding_size(),
                                        group_element->associated_blk_height()
                                    });
                            }
                        }
                    }

                    xinfo("[vnetwork] message hash: %" PRIx64 " , after  filter:s&r sender is %s , receiver is %s",
                          vnetwork_message.hash(),
                          sender.to_string().c_str(),
                          receiver.to_string().c_str());

                    std::vector<std::pair<common::xnode_address_t const, xmessage_ready_callback_t>> callbacks;
                    XLOCK_GUARD(m_callbacks_mutex) {
                        for (auto const & callback_info : m_callbacks) {
                            auto const & callback_addr = top::get<common::xnode_address_t const>(callback_info);
                            xdbg("[vnetwork] see callback address: %s", callback_addr.to_string().c_str());

                            auto contains = callback_addr.contains(receiver);
                            if (!contains) {
                                contains = receiver.contains(callback_addr);
                            }

                            if (!contains) {
                                #if VHOST_METRICS
                                XMETRICS_COUNTER_INCREMENT("vhost_discard_addr_not_match", 1);
                                #endif
                                xdbg("[vnetwork] callback at address %s not matched", callback_addr.to_string().c_str());
                                continue;
                            }
                            callbacks.push_back(callback_info);
                        }
                    }

                    if (callbacks.empty()) {
                        xinfo("[vnetwork] no callback found for message hash: %" PRIx64 " , sender is %s , receiver is %s",
                              vnetwork_message.hash(),
                              sender.to_string().c_str(),
                              receiver.to_string().c_str());
                        continue;
                    }

                    for (auto & callback_info : callbacks) {
                        auto const & callback = top::get<xmessage_ready_callback_t>(callback_info);
                        if (callback) {
                            try {
                                // callback(sender, receiver, message);
                                xdbg("[vnetwork] send msg %" PRIx32 " (hash %" PRIx64 ") to callback %p at address %s",
                                     static_cast<std::uint32_t>(message.id()),
                                     message.hash(),
                                     &callback,
                                     top::get<common::xnode_address_t const>(callback_info).to_string().c_str());
#ifdef ENABLE_METRICS
                                char msg_info[30] = {0};
                                snprintf(msg_info, 29, "%x|%" PRIx64, vnetwork_message.message().id(), message.hash());
                                XMETRICS_TIME_RECORD_KEY_WITH_TIMEOUT("vhost_handle_data_callback", msg_info, uint32_t(100000));
#endif
                                XMETRICS_GAUGE(metrics::vhost_recv_callback, 1);
                                callback(sender, message, msg_time);
                            } catch (std::exception const & eh) {
                                xerror("[vnetwork] exception caught from callback: %s", eh.what());
                            }
                        } else {
                            xerror("[vnetwork] callback not registered");
                        }
                    }
                } catch (top::error::xtop_error_t const & eh) {
                    xwarn("[vnetwork] catches top error. category %s; error code %d; eh msg: %s; ec msg: %s", eh.code().category().name(), eh.code().value(), eh.what(), eh.code().message().c_str());
                } catch (std::exception const & eh) {
                    xwarn("[vnetwork] catches std::exception: %s", eh.what());
                } catch (...) {
                    xwarn("[vnetwork] catches an unknown exception");
                }
            }

#if defined(XENABLE_VHOST_BENCHMARK)
            auto xxend = std::chrono::high_resolution_clock::now();
            auto ms = static_cast<std::size_t>(std::chrono::duration_cast<std::chrono::milliseconds>(xxend - xxbegin).count());
            xwarn("vhost processed in %zu milliseconds against %zu packs => tps = %lf", ms, all_byte_messages.size(), static_cast<double>(all_byte_messages.size()) * 1000 / ms);
#endif


        } catch (std::exception const & eh) {
            xwarn("[vnetwork] catches std::exception: %s", eh.what());
        } catch (...) {
            xwarn("[vnetwork] catches an unknown exception");
        }
    }
}

NS_END2
