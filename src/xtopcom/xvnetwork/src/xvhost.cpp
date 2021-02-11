// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xvnetwork/xvhost.h"

#include "xbase/xlog.h"
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
    if (common::has<common::xnode_type_t::archive>(src.type())) {
        assert(src.zone_id() == common::xarchive_zone_id);
        assert(src.cluster_id() == common::xdefault_cluster_id);
        assert(src.group_id() == common::xdefault_group_id);
    }

    if (dst.account_address().has_value() && common::has<common::xnode_type_t::archive>(dst.type())) {
        assert(dst.zone_id() == common::xarchive_zone_id);
        assert(dst.cluster_id() == common::xdefault_cluster_id);
        assert(dst.group_id() == common::xdefault_group_id);
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
            XMETRICS_COUNTER_INCREMENT("vhost_" + std::to_string(static_cast<std::uint16_t>(common::get_message_category(message.id()))) + "_out_vhost_send" +
                                           std::to_string(static_cast<std::uint32_t>(message.id())),
                                       1);

            XMETRICS_COUNTER_INCREMENT("vhost_" + std::to_string(static_cast<std::uint16_t>(common::get_message_category(message.id()))) + "_out_vhost_send_size" +
                                           std::to_string(static_cast<std::uint32_t>(message.id())),
                                       bytes_message.size());

            m_network_driver->send_to(dst.account_address(), bytes_message, transmission_property);
        } else if (dst.empty()) {
            broadcast(message, src);
        } else {
            // crose broadcast is not correct by using send_to
            if (dst.cluster_address() != src.cluster_address()) {
                XTHROW(xvnetwork_error_t, xvnetwork_errc_t::cluster_address_not_match, u8"src " + src.to_string() + u8" dst " + dst.to_string());
            }

            broadcast(message, src);
        }
    } catch (xvnetwork_error_t const & eh) {
        xwarn("%s", eh.what());
        throw;
    }
}

void xtop_vhost::forward_broadcast_message(xmessage_t const & message, common::xnode_address_t const & src, common::xnode_address_t const & dst) {
    assert(common::has<common::xnode_type_t::group>(dst.cluster_address().type()));
    assert(src.cluster_address() != dst.cluster_address());
    assert(!dst.group_id().empty());

    assert(!src.empty());
    assert(!src.account_address().empty());
    assert(src.account_address() == host_node_id());
    assert(dst.account_address().empty());

    assert(message.id() != common::xmessage_id_t::invalid);

#if defined DEBUG
    if (common::has<common::xnode_type_t::archive>(src.type())) {
        assert(src.zone_id() == common::xarchive_zone_id);
        assert(src.cluster_id() == common::xdefault_cluster_id);
        assert(src.group_id() == common::xdefault_group_id);
    }

    if (common::has<common::xnode_type_t::archive>(dst.type())) {
        assert(dst.zone_id() == common::xarchive_zone_id);
        assert(dst.cluster_id() == common::xdefault_cluster_id);
        assert(dst.group_id() == common::xdefault_group_id);
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

        XMETRICS_COUNTER_INCREMENT("vhost_" + std::to_string(static_cast<std::uint16_t>(common::get_message_category(message.id()))) + "_out_vhost_forward_broadcast_message" +
                                       std::to_string(static_cast<std::uint32_t>(message.id())),
                                   1);

        XMETRICS_COUNTER_INCREMENT("vhost_" + std::to_string(static_cast<std::uint16_t>(common::get_message_category(message.id()))) + "_out_vhost_forward_broadcast_message_size" +
                                       std::to_string(static_cast<std::uint32_t>(message.id())),
                                   bytes_message.size());

        on_network_data_ready(host_node_id(), bytes_message);

        m_network_driver->forward_broadcast(dst.cluster_address().sharding_info(), dst.type(), bytes_message);
    } catch (vnetwork::xvnetwork_error_t const & eh) {
        xwarn("[vnetwork] forward_broadcast_message xvnetwork_error_t exception caught: %s; error code: %d", eh.what(), eh.code().value());
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
    if (common::has<common::xnode_type_t::archive>(src.type())) {
        assert(src.zone_id() == common::xarchive_zone_id);
        assert(src.cluster_id() == common::xdefault_cluster_id);
        assert(src.group_id() == common::xdefault_group_id);
    }

    if (common::has<common::xnode_type_t::archive>(dst.type())) {
        assert(dst.zone_id() == common::xarchive_zone_id);
        assert(dst.cluster_id() == common::xdefault_cluster_id);
        assert(dst.group_id() == common::xdefault_group_id);
    }
#endif

    try {
        assert(m_chain_timer != nullptr);
        xvnetwork_message_t const vnetwork_message{src, dst, message, m_chain_timer->logic_time()};
        auto const bytes_message = top::codec::msgpack_encode(vnetwork_message);

        on_network_data_ready(host_node_id(), bytes_message);  // broadcast to local vnet  // TODO remove codec part

        xdbg("[vnetwork] broadcast msg %x from:%s to:%s hash %" PRIx64, message.id(), src.to_string().c_str(), dst.to_string().c_str(), message.hash());

        XMETRICS_COUNTER_INCREMENT("vhost_" + std::to_string(static_cast<std::uint16_t>(common::get_message_category(message.id()))) + "_out_vhost_broadcast_to_all" +
                                       std::to_string(static_cast<std::uint32_t>(message.id())),
                                   1);

        XMETRICS_COUNTER_INCREMENT("vhost_" + std::to_string(static_cast<std::uint16_t>(common::get_message_category(message.id()))) + "_out_vhost_broadcast_to_all_size" +
                                       std::to_string(static_cast<std::uint32_t>(message.id())),
                                   bytes_message.size());

        assert(m_network_driver);
        m_network_driver->spread_rumor(bytes_message);
    } catch (xvnetwork_error_t const & eh) {
        xerror("[vnetwork] xvnetwork_error_t exception caught: %s; error code: %d", eh.what(), eh.code().value());
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
    if (common::has<common::xnode_type_t::archive>(src.type())) {
        assert(src.zone_id() == common::xarchive_zone_id);
        assert(src.cluster_id() == common::xdefault_cluster_id);
        assert(src.group_id() == common::xdefault_group_id);
    }

    if (common::has<common::xnode_type_t::archive>(dst.type())) {
        assert(dst.zone_id() == common::xarchive_zone_id);
        assert(dst.cluster_id() == common::xdefault_cluster_id);
        assert(dst.group_id() == common::xdefault_group_id);
    }
#endif

    assert(message.id() != common::xmessage_id_t::invalid);

    assert(m_chain_timer != nullptr);
    xvnetwork_message_t const vmsg{src, dst, message, m_chain_timer->logic_time()};
    xdbg("[vnetwork] add a rumor msg %" PRIx64 " sent from %s to %s", vmsg.hash(), src.to_string().c_str(), dst.to_string().c_str());

    auto packet_msg = top::codec::msgpack_encode(vmsg);
    auto new_hash_val = base::xhash32_t::digest(std::string((char *)packet_msg.data(), packet_msg.size()));
    xinfo("[vnetwork]xtop_vhost::broadcast [vnet hash: %" PRIx64 "] [hash: %u] [to_hash:%u]", vmsg.hash(), message.hash(), new_hash_val);

    XMETRICS_COUNTER_INCREMENT("vhost_" + std::to_string(static_cast<std::uint16_t>(common::get_message_category(message.id()))) + "_out_vhost_broadcast" +
                                   std::to_string(static_cast<std::uint32_t>(message.id())),
                               1);

    XMETRICS_COUNTER_INCREMENT("vhost_" + std::to_string(static_cast<std::uint16_t>(common::get_message_category(message.id()))) + "_out_vhost_broadcast_size" +
                                   std::to_string(static_cast<std::uint32_t>(message.id())),
                               packet_msg.size());

    m_network_driver->spread_rumor(src.cluster_address().sharding_info(), packet_msg);
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

        XMETRICS_COUNTER_INCREMENT("vhost_" + std::to_string(static_cast<std::uint16_t>(common::get_message_category(message.id()))) + "_out_vhost_broadcast" +
                                       std::to_string(static_cast<std::uint32_t>(message.id())),
                                   1);

        XMETRICS_COUNTER_INCREMENT("vhost_" + std::to_string(static_cast<std::uint16_t>(common::get_message_category(message.id()))) + "_out_vhost_broadcast_size" +
                                       std::to_string(static_cast<std::uint32_t>(message.id())),
                                   bytes.size());

        m_network_driver->spread_rumor(src.sharding_address().sharding_info(), bytes);
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

        XMETRICS_COUNTER_INCREMENT("vhost_" + std::to_string(static_cast<std::uint16_t>(common::get_message_category(message.id()))) + "_out_vhost_broadcast_to_all" +
                                       std::to_string(static_cast<std::uint32_t>(message.id())),
                                   1);

        XMETRICS_COUNTER_INCREMENT("vhost_" + std::to_string(static_cast<std::uint16_t>(common::get_message_category(message.id()))) + "_out_vhost_broadcast_to_all_size" +
                                       std::to_string(static_cast<std::uint32_t>(message.id())),
                                   bytes_message.size());

        assert(m_network_driver);
        m_network_driver->spread_rumor(bytes_message);
    } else if (common::broadcast(dst.slot_id())) {
        // broadcast between different shradings

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

        XMETRICS_COUNTER_INCREMENT("vhost_" + std::to_string(static_cast<std::uint16_t>(common::get_message_category(message.id()))) + "_out_vhost_forward_broadcast_message" +
                                       std::to_string(static_cast<std::uint32_t>(message.id())),
                                   1);

        XMETRICS_COUNTER_INCREMENT("vhost_" + std::to_string(static_cast<std::uint16_t>(common::get_message_category(message.id()))) + "_out_vhost_forward_broadcast_message_size" +
                                       std::to_string(static_cast<std::uint32_t>(message.id())),
                                   bytes_message.size());

        on_network_data_ready(host_node_id(), bytes_message);

        m_network_driver->forward_broadcast(to.cluster_address().sharding_info(), to.type(), bytes_message);
    } else {
        ec = xvnetwork_errc2_t::not_supported, xwarn("%s %s", ec.category().name(), ec.message().c_str());
    }
}

void xtop_vhost::on_network_data_ready(common::xnode_id_t const &, xbyte_buffer_t const & bytes) {
    XMETRICS_COUNTER_INCREMENT("vhost_on_data_ready_called", 1);

    try {
        if (!running()) {
            xwarn("[vnetwork] vhost not run");
            return;
        }

        XMETRICS_COUNTER_INCREMENT("vhost_total_size_of_all_messages", bytes.size());

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

                    auto const & message = vnetwork_message.message();
                    auto const & receiver = vnetwork_message.receiver();
                    auto const & sender = vnetwork_message.sender();
                    auto const msg_time = vnetwork_message.logic_time();

                    xdbg("[vnetwork] message hash: %" PRIx64 " , before filter:s&r sender is %s , receiver is %s",
                         vnetwork_message.hash(),
                         sender.to_string().c_str(),
                         receiver.to_string().c_str());

                    XMETRICS_COUNTER_INCREMENT("vhost_" + std::to_string(static_cast<std::uint16_t>(common::get_message_category(vnetwork_message.message().id()))) +
                                                   "_in_vhost_size" + std::to_string(static_cast<std::uint32_t>(vnetwork_message.message().id())),
                                               bytes.size());

                    m_filter_manager->filt_message(vnetwork_message);
                    if (vnetwork_message.empty()) {
                        continue;
                    }

                    xdbg("[vnetwork] message hash: %" PRIx64 " , after  filter:s&r sender is %s , receiver is %s",
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
                                XMETRICS_COUNTER_INCREMENT("vhost_discard_addr_not_match", 1);
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
                                char hex_msg_id[9] = {0};
                                snprintf(hex_msg_id, 8, "%x", vnetwork_message.message().id());
                                XMETRICS_TIME_RECORD_KEY_WITH_TIMEOUT("vhost_handle_data_callback", hex_msg_id, uint32_t(100000));
#endif
                                callback(sender, message, msg_time);
                            } catch (std::exception const & eh) {
                                xerror("[vnetwork] exception caught from callback: %s", eh.what());
                            }
                        } else {
                            xerror("[vnetwork] callback not registered");
                        }
                    }

                } catch (xvnetwork_error_t const & eh) {
                    xwarn("[vnetwork] catches vnetwork exception: %s", eh.what());
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

/*
 * this is the callback function, which is called by the lower module,
 * thus do not throw vnetwork exceptions
 */
/* Commented out on 2020.05.08 when refactoring the vnetwork message filter.
void
xtop_vhost::do_handle_network_data() {
    assert(m_vhost_thread_id == std::this_thread::get_id());

#if defined DEBUG
    xscope_executer_t do_handle_network_data_exit_verifier{ [this, self=shared_from_this()] {
        assert(!running());
    }};
#endif

    while (running()) {
        try {
            auto all_byte_messages = m_message_queue.wait_and_pop_all();

            XMETRICS_COUNTER_INCREMENT("vhost_handle_data_ready_called", all_byte_messages.size());

            XMETRICS_TIME_RECORD("vhost_handle_data_ready_called_time");

            for (auto & bytes : all_byte_messages) {
                try {
                    if (bytes.empty()) {
                        // this may be a stop notification message.
                        // anyway, for an empty message, just ignore it.
                        continue;
                    }

                    auto vnetwork_message = top::codec::msgpack_decode<xvnetwork_message_t>(bytes);
                    if (vnetwork_message.empty()) {
                        XMETRICS_COUNTER_INCREMENT("vhost_received_invalid", 1);
                        xwarn("[vnetwork] vnetwork message empty");
                        continue;
                    }

                    auto message = vnetwork_message.message();
                    auto receiver = vnetwork_message.receiver();
                    auto const & sender = vnetwork_message.sender();

                    XMETRICS_COUNTER_INCREMENT(std::to_string(static_cast<std::uint16_t>(common::get_message_category(message.id()))) + "_in_vhost"+
                                                        std::to_string(static_cast<std::uint32_t>(message.id())),
                                                        1);

                    XMETRICS_COUNTER_INCREMENT(std::to_string(static_cast<std::uint16_t>(common::get_message_category(message.id()))) + "_in_vhost_size"+
                                                        std::to_string(static_cast<std::uint32_t>(message.id())),
                                                        bytes.size());

                    xdbg("[vnetwork] recv message :%" PRIx32 " (hash %" PRIx64 " logic time %" PRIu64 ") from:%s to:%s",
                         static_cast<std::uint32_t>(message.id()),
                         message.hash(),
                         vnetwork_message.logic_time(),
                         sender.to_string().c_str(),
                         receiver.to_string().c_str());

                    // if (receiver.empty()) {
                    //     XMETRICS_COUNTER_INCREMENT("vhost_received_invalid", 1);
                    //     assert(false);
                    //     xwarn("[vnetwork] vnetwork message receiver address is empty");
                    //     continue;
                    // }

                    xdbg("[vnetwork] %s receives message %" PRIx64 " from %s msg id %" PRIx32 " logic time %" PRIu64,
                         host_node_id().to_string().c_str(),
                         vnetwork_message.hash(),
                         sender.to_string().c_str(),
                         static_cast<std::uint32_t>(vnetwork_message.message().id()),
                         vnetwork_message.logic_time());

                    if (vnetwork_message.empty()) {
                        XMETRICS_COUNTER_INCREMENT("vhost_received_invalid", 1);
                        assert(false);
                        xwarn("[vnetwork] receiving an empty message. msg id %" PRIx32, static_cast<std::uint32_t>(message.id()));
                        continue;
                    }


                    if (!common::broadcast(receiver.network_id()) && receiver.network_id() != network_id()) {
                        XMETRICS_COUNTER_INCREMENT("vhost_received_invalid", 1);
                        xdbg("[vnetwork] vnetwork message network id not matched: this network id %" PRIu32 "; sent to %" PRIu32,
                             static_cast<std::uint32_t>(network_id().value()),
                             static_cast<std::uint32_t>(receiver.network_id().value()));
                        continue;
                    }

                    auto const msg_time = vnetwork_message.logic_time();
                    xdbg("[vnetwork] message logic time %" PRIu64, msg_time);
                    auto const local_time = m_chain_timer->logic_time();

                    // the logic time is used to calc the version.  thus, if version not specified,
                    // logic time will be used to find the version, which means that the time
                    // should be verified!
                    if (receiver.version().empty()) {
                        constexpr std::uint64_t future_threshold{ 2 };
                        constexpr std::uint64_t past_threshold{ 6 };

                        if ((local_time != 0)                          &&
                            (local_time + future_threshold < msg_time) &&
                            message.id() != contract::xmessage_block_broadcast_id) {
                            // receive a message from future, ignore
                            xwarn("[vnetwork] receive a message whose logic time is %" PRIu64 " which is much more newer than current node %" PRIu64,
                                  msg_time,
                                  local_time);
                            XMETRICS_COUNTER_INCREMENT("vhost_discard_validation_failure", 1);
                            // assert(false);
                            continue;
                        }

                        if ((msg_time != 0)                          &&
                            (msg_time + past_threshold < local_time) &&
                            message.id() != contract::xmessage_block_broadcast_id) {
                            // receive a message from past, ignore
                            xwarn("[vnetwork] receive a message whose logic time is %" PRIu64 " which is much more older than current node %" PRIu64,
                                  msg_time,
                                  local_time);
                            XMETRICS_COUNTER_INCREMENT("vhost_discard_validation_failure", 1);
                            // assert(false);
                            continue;
                        }
                    }

                    auto const dst_type = receiver.type();
                    auto const src_type = sender.type();

                    if (common::has<common::xnode_type_t::consensus_validator>(dst_type)) {
                        // for a cross network / cluster / group communication, the special case is that the receiver is a consensus group or node.
                        // for a consensus node, the incoming message should be from its associated auditor, neighbors or from archive
                        if (sender.cluster_address() == receiver.cluster_address()) {
                            // message from neighbors.

                            if (sender.version().has_value() && receiver.version().has_value() && sender.version() != receiver.version()) {
                                XMETRICS_COUNTER_INCREMENT("vhost_discard_validation_failure", 1);
                                xwarn("[vnetwork] %s receives a message %" PRIx32 " hash %" PRIx64 " from %s to %s but version not match",
                                      host_node_id().to_string().c_str(),
                                      static_cast<std::uint32_t>(message.id()),
                                      message.hash(),
                                      sender.to_string().c_str(),
                                      receiver.to_string().c_str());
                                continue;
                            }

                            assert(sender.version().has_value() || receiver.version().has_value());

                            if (sender.version().empty()) {
                                XMETRICS_COUNTER_INCREMENT("vhost_discard_validation_failure", 1);
                                xwarn("[vnetwork] %s receives a message %" PRIx32 " hash %" PRIx64 " from %s to %s, but sender doesn't provide round version",
                                      host_node_id().to_string().c_str(),
                                      static_cast<std::uint32_t>(message.id()),
                                      message.hash(),
                                      sender.to_string().c_str(),
                                      receiver.to_string().c_str());
                                continue;
                            }

                            if (receiver.version().empty()) {
                                if (receiver.account_election_address().empty()) {
                                    receiver = common::xnode_address_t{ receiver.sharding_address(), sender.version(), receiver.sharding_size(), receiver.associated_blk_height() };
                                } else {
                                    receiver = common::xnode_address_t{ receiver.sharding_address(), receiver.account_election_address(), sender.version(),
receiver.sharding_size(), receiver.associated_blk_height() };
                                }
                            }

                            assert(sender.version().has_value() && receiver.version().has_value());
                        } else {
                            // the message is not from its neighbors, then it must be from its parent or form archive

                            if (!common::has<common::xnode_type_t::consensus_auditor>(src_type) &&
                                !common::has<common::xnode_type_t::archive>(src_type)) {
                                XMETRICS_COUNTER_INCREMENT("vhost_discard_validation_failure", 1);
                                xwarn("[vnetwork] %s received a message id %" PRIx32 " hash %" PRIx64 " from %s to %s which is not an auditor or archive node",
                                      host_node_id().to_string().c_str(),
                                      static_cast<std::uint32_t>(message.id()),
                                      message.hash(),
                                      sender.to_string().c_str(),
                                      receiver.to_string().c_str());
                                continue;
                            }

                            if (common::has<common::xnode_type_t::consensus_auditor>(src_type)) {
                                // if receiver has version, the associated auditor must be matched with the sender.
                                std::shared_ptr<election::cache::xgroup_element_t> associated_parent{ nullptr };

                                if (receiver.version().has_value()) {
                                    assert(m_election_cache_data_accessor != nullptr);
                                    std::error_code ec{ election::xdata_accessor_errc_t::success };
                                    associated_parent = m_election_cache_data_accessor->parent_group_element(receiver.sharding_address(),
                                                                                                             receiver.version(),
                                                                                                             ec);
                                    if (ec) {
                                        xdbg("[vnetwork] network %" PRIu32 " node %s receives msg sent to %s. ignored. error: %s",
                                             static_cast<std::uint32_t>(network_id().value()),
                                             host_node_id().value().c_str(),
                                             receiver.to_string().c_str(),
                                             ec.message().c_str());
                                        continue;
                                    }

                                    if (!(sender.cluster_address() == associated_parent->address().cluster_address() &&
                                          sender.version() == associated_parent->version())) {
                                        XMETRICS_COUNTER_INCREMENT("vhost_discard_validation_failure", 1);
                                        xwarn("[vnetwork] %s received a message id %" PRIx32 " hash %" PRIx64 " sent to %s from %s which is not its associated parent (%s)",
                                              host_node_id().to_string().c_str(),
                                              static_cast<std::uint32_t>(message.id()),
                                              message.hash(),
                                              receiver.to_string().c_str(),
                                              sender.to_string().c_str(),
                                              associated_parent->address().to_string().c_str());
                                        continue;
                                    }
                                } else {
                                    assert(receiver.version().empty());
                                    // associated_parent = vnetwork(sender.network_id())->zone_vnode(sender.zone_id())
                                    //                                                  ->cluster_vnode(sender.cluster_id())
                                    //                                                  ->group_vnode(sender.group_id(), sender.version());
                                    std::error_code ec{ election::xdata_accessor_errc_t::success };
                                    associated_parent = m_election_cache_data_accessor->group_element(sender.sharding_address(),
                                                                                                      sender.version(),
                                                                                                      ec);
                                    if (ec) {
                                        xdbg("[vnetwork] network %" PRIu32 " node %s receives msg sent to %s. ignored. error: %s",
                                             static_cast<std::uint32_t>(network_id().value()),
                                             host_node_id().value().c_str(),
                                             receiver.to_string().c_str(),
                                             ec.message().c_str());
                                        continue;
                                    }
                                }

                                assert(associated_parent != nullptr);
                                if (receiver.version().empty()) {
                                    auto const validator_children = associated_parent->associated_child_groups(msg_time);
                                    common::xversion_t max_validator_version;
                                    for (auto const & validator : validator_children) {
                                        if ((validator->address().cluster_address() == receiver.cluster_address()) &&
                                            (max_validator_version < validator->version())) {
                                            max_validator_version = validator->version();
                                        }
                                    }

                                    if (!max_validator_version.empty()) {
                                        if (receiver.account_address().empty()) {
                                            receiver = common::xnode_address_t{
                                                receiver.cluster_address(),
                                                max_validator_version,
                                                receiver.sharding_size(),
                                                receiver.associated_blk_height()
                                            };
                                        } else {
                                            receiver = common::xnode_address_t{
                                                receiver.cluster_address(),
                                                receiver.account_election_address(),
                                                max_validator_version,
                                                receiver.sharding_size(),
                                                receiver.associated_blk_height()
                                            };
                                        }
                                    } else {
                                        xwarn("[vnetwork] no validator (%s) found associated with auditor %s for msg %" PRIx32 " hash %" PRIx64 " (msg logic time %" PRIu64 ") at
logic time %" PRIu64, receiver.to_string().c_str(), sender.to_string().c_str(), static_cast<std::uint32_t>(message.id()), message.hash(), msg_time, last_logic_time()); continue;
                                    }
                                }
                            } else {
                                assert(common::has<common::xnode_type_t::archive>(src_type));
                                if (receiver.version().empty()) {
                                    std::error_code ec{ election::xdata_accessor_errc_t::success };
                                    auto const group = m_election_cache_data_accessor->group_element(receiver.sharding_address(),
                                                                                                             msg_time,
                                                                                                             ec);
                                    if (ec) {
                                        xdbg("[vnetwork] network %" PRIu32 " node %s receives msg sent to %s. ignored. error: %s",
                                             static_cast<std::uint32_t>(network_id().value()),
                                             host_node_id().value().c_str(),
                                             receiver.to_string().c_str(),
                                             ec.message().c_str());
                                        continue;
                                    }
                                    auto version = group->version();

                                    if (receiver.account_address().empty()) {
                                        receiver = common::xnode_address_t{
                                            receiver.cluster_address(),
                                            version,
                                            receiver.sharding_size(),
                                            receiver.associated_blk_height()
                                        };
                                    } else {
                                        receiver = common::xnode_address_t{
                                            receiver.cluster_address(),
                                            receiver.account_election_address(),
                                            version,
                                            receiver.sharding_size(),
                                            receiver.associated_blk_height()
                                        };
                                    }
                                }
                            }
                        }
                    } else if (common::has<common::xnode_type_t::consensus_auditor>(dst_type)) {
                        if (common::has<common::xnode_type_t::consensus_validator>(src_type)) {
                            // for a auditor node, if the incomming message is from validator, then this validator must be from its
                            // associated validator.

                            std::shared_ptr<election::cache::xgroup_element_t> auditor{ nullptr };
                            if (receiver.version().empty()) {
                                if (sender.version().empty()) {
                                    // if auditor version and validator version are both empty. it's not acceptable.
                                    xwarn("[vnetwork] invalid message sent from validator to auditor. message id %" PRIx32 " hash %" PRIx64,
                                          message.id(),
                                          message.hash());
                                    continue;
                                }

                                xdbg("[vnetwork] auditor received message %" PRIx64 " from validator but not specify the auditor round version.  calculating...");
                                //auditor = vnetwork(sender.network_id())->zone_vnode(sender.zone_id())
                                //                                       ->cluster_vnode(sender.cluster_id())
                                //                                       ->group_vnode(sender.group_id(), sender.version())
                                //                                       ->associated_parent_group();
                                std::error_code ec{ election::xdata_accessor_errc_t::success };
                                auditor = m_election_cache_data_accessor->parent_group_element(sender.sharding_address(),
                                                                                               sender.version(),
                                                                                               ec);
                                if (ec) {
                                    xdbg("[vnetwork] network %" PRIu32 " node %s receives msg sent to %s. ignored. error: %s",
                                         static_cast<std::uint32_t>(network_id().value()),
                                         host_node_id().value().c_str(),
                                         receiver.to_string().c_str(),
                                         ec.message().c_str());
                                    continue;
                                }
                            } else {
                                //auditor = vnetwork(receiver.network_id())->zone_vnode(receiver.zone_id())
                                //                                         ->cluster_vnode(receiver.cluster_id())
                                //                                         ->group_vnode(receiver.group_id(), receiver.version());
                                std::error_code ec{ election::xdata_accessor_errc_t::success };
                                auditor = m_election_cache_data_accessor->group_element(receiver.sharding_address(),
                                                                                        receiver.version(),
                                                                                        ec);
                                if (ec) {
                                    xdbg("[vnetwork] network %" PRIu32 " node %s receives msg sent to %s. ignored. error: %s",
                                         static_cast<std::uint32_t>(network_id().value()),
                                         host_node_id().value().c_str(),
                                         receiver.to_string().c_str(),
                                         ec.message().c_str());
                                    continue;
                                }
                            }
                            // should call xbasic_group_vnode_t::associated_child_groups() not
                            // xbasic_group_vnode_t::associated_child_groups(common::xlogic_time_t const);
                            // since the sender must have a version.
                            auto const validator_children = auditor->associated_child_groups(common::xjudgement_day);

                            bool valid_child{ false };
                            for (auto const & validator_child : validator_children) {
                                if (validator_child->version() != sender.version()) {
                                    continue;
                                }

                                if (validator_child->group_id() != sender.group_id()) {
                                    continue;
                                }

                                if (validator_child->cluster_id() != sender.cluster_id()) {
                                    xwarn("[vnetwork] recving msg from different area: src area %s dst area %s",
                                          validator_child->cluster_id().to_string().c_str(),
                                          sender.cluster_id().to_string().c_str());

                                    assert(false);
                                    continue;
                                }

                                if (validator_child->zone_id() != sender.zone_id()) {
                                    xwarn("[vnetwork] recving msg from different zone: src zone %s dst zone %s",
                                          validator_child->zone_id().to_string().c_str(),
                                          sender.zone_id().to_string().c_str());

                                    assert(false);
                                    continue;
                                }

                                if (validator_child->network_id() != sender.network_id()) {
                                    xwarn("[vnetwork] recving msg from different network: src network %s dst network %s",
                                          validator_child->network_id().to_string().c_str(),
                                          sender.network_id().to_string().c_str());

                                    assert(false);
                                    continue;
                                }

                                valid_child = true;
                                break;
                            }

                            if (!valid_child) {
                                XMETRICS_COUNTER_INCREMENT("vhost_discard_validation_failure", 1);
                                xwarn("[vnetwork] %s received a message id %" PRIx32 " hash %" PRIx64 " sent to %s from %s which is not its associated child",
                                      host_node_id().to_string().c_str(),
                                      static_cast<std::uint32_t>(message.id()),
                                      message.hash(),
                                      receiver.to_string().c_str(),
                                      sender.to_string().c_str());
                                // assert(false);   // TODO: revert after VNode destory enabled
                                continue;
                            }
                        }
                    }

                    XMETRICS_COUNTER_INCREMENT("vhost_received_valid", 1);

                    // for a _cross cluster_, _cross zone_ or _cross network_ communication, the send
                    // side don't need to know the election round the receive side is.  the rule here
                    // is the sender always send the message to the the receiver with latest election
                    // round.  but the sender knows nothing about the election round info on the
                    // receiver side, thus it sends to the receiver without version (election round)
                    // attached.  if the receiver sees an empty version it should make the version as
                    // the latest one.

                    if (receiver.version().empty()) {
                        // if receiver's round version is empty, it shouldn't be a consensus node.
                        // if it's a consensus node, the version should be resolved in the before-mentioned
                        // validation process.
                        if (common::has<common::xnode_type_t::consensus_validator>(dst_type)) {
                            xwarn("[xnetwork] receive empty version to validator!!!!");
                            continue;
                        }

                        std::error_code ec{ election::xdata_accessor_errc_t::success };
                        auto const group_element = m_election_cache_data_accessor->group_element(receiver.sharding_address(),
                                                                                                 msg_time,
                                                                                                 ec);
                        if (!ec) {
                            if (receiver.account_election_address().empty()) {
                                receiver = common::xnode_address_t{ receiver.sharding_address(), group_element->version(), group_element->sharding_size(),
group_element->associated_blk_height() }; } else { receiver = common::xnode_address_t{ receiver.sharding_address(), receiver.account_election_address(), group_element->version(),
group_element->sharding_size(), group_element->associated_blk_height() };
                            }
                        }

                        //try {
                        //    if (exists(receiver.network_id())) {
                        //        auto vnet = vnetwork(receiver.network_id());
                        //        assert(vnet);

                        //        if (vnet->exists(receiver.zone_id())) {
                        //            auto vzone = vnet->zone_vnode(receiver.zone_id());
                        //            assert(vzone);

                        //            if (vzone->exists(receiver.cluster_id())) {
                        //                auto cvnode = vzone->cluster_vnode(receiver.cluster_id());
                        //                assert(cvnode);

                        //                auto result = cvnode->exists(receiver.group_id(), msg_time);
                        //                if (top::get<bool>(result)) {
                        //                    auto version = top::get<common::xversion_t>(result);
                        //                    if (receiver.account_address().empty()) {
                        //                        receiver = common::xnode_address_t{ receiver.cluster_address(), version };
                        //                    } else {
                        //                        receiver = common::xnode_address_t{ receiver.cluster_address(), receiver.account_address(), version };
                        //                    }
                        //                }
                        //            }
                        //        }
                        //    }
                        //} catch (xvnetwork_error_t const & eh) {
                        //    xwarn("[vnetwork] xventwork_error_t exception caught when calculating the latest version for receiving address. eh: %s recv addr: %s",
                        //          eh.what(), receiver.to_string().c_str());
                        //}
                    }

                    xdbg("[vnetwork] receiver is %s", receiver.to_string().c_str());

                    std::vector<std::pair<common::xnode_address_t const, xmessage_ready_callback_t>> callbacks;
                    XLOCK_GUARD(m_callbacks_mutex) {
                        for (auto const & callback_info : m_callbacks) {
                            auto const & callback_addr = top::get<common::xnode_address_t const>(callback_info);
                            xdbg("[vnetwork] see callback address: %s", callback_addr.to_string().c_str());
                            // auto const & callback = top::get<xmessage_ready_callback_t>(callback_info);

                            // exact match
                            if (callback_addr == receiver) {
                                callbacks.push_back(callback_info);
                                continue;
                            }

                            auto contains = callback_addr.cluster_address().contains(receiver.cluster_address());
                            if (!contains) {
                                contains = receiver.cluster_address().contains(callback_addr.cluster_address());
                            }

                            if (!contains) {
                                XMETRICS_COUNTER_INCREMENT("vhost_discard_addr_not_match", 1);
                                continue;
                            }

                            assert(contains);

                            bool const version_matched = callback_addr.version().empty()                  ||
                                                         receiver.version().empty()                       ||
                                                         (callback_addr.version() == receiver.version());

                            bool const account_matched = callback_addr.account_address().empty() ||
                                                         receiver.account_address().empty()      ||
                                                         (callback_addr.account_address() == receiver.account_address());

                            if (version_matched && account_matched) {
                                callbacks.push_back(callback_info);
                            } else {
                                XMETRICS_COUNTER_INCREMENT("vhost_discard_addr_not_match", 1);
                                xdbg("[vnetwork] callback at address %s not matched", callback_addr.to_string().c_str());
                            }
                        }
                    }

                    if (callbacks.empty()) {
                        xwarn("[vnetwork] no callback found");
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

                                callback(sender, message, msg_time);
                            } catch (std::exception const & eh) {
                                xerror("[vnetwork] exception caught from callback: %s", eh.what());
                            }
                        } else {
                            xerror("[vnetwork] callback not registered");
                        }
                    }

                } catch (xvnetwork_error_t const & eh) {
                    xwarn("[vnetwork] catches vnetwork exception: %s", eh.what());
                } catch (std::exception const & eh) {
                    xwarn("[vnetwork] catches std::exception: %s", eh.what());
                } catch (...) {
                    xwarn("[vnetwork] catches an unknown exception");
                }
            }


        } catch (std::exception const & eh) {
            xwarn("[vnetwork] catches std::exception: %s", eh.what());
        } catch (...) {
            xwarn("[vnetwork] catches an unknown exception");
        }
    }
}
*/
NS_END2
